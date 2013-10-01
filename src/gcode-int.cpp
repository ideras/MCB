/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QProgressDialog>
#include <QMessageBox>
#include <QTime>
#include <limits>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _MSC_VER
#include <io.h>
#endif

#ifndef _WIN32
//Linux treat binary and text the same, Windows doesn't
#define _O_BINARY 0

#include <unistd.h>

#define tell(fd) lseek(fd, 0, SEEK_CUR)
#endif

#include "gcode-int.h"

using namespace std;

extern stringstream out_err;

static inline void UpdateBoardArea(Real x, Real y, GCodeInfo &gi)	
{
        if (x < gi.BoardMinX)
            gi.BoardMinX = x;
		
        if (x > gi.BoardMaxX)
            gi.BoardMaxX = x;
		
        if (y < gi.BoardMinY)
            gi.BoardMinY = y;
		
        if (y > gi.BoardMaxY)
            gi.BoardMaxY = y;
}

GCodeInt::GCodeInt(string filePath)
{
	m_filePath = filePath;
	m_gparser = 0;
	itCurrentStmt = slist.end();
	probePoints = new list<Position>();
}

GCodeInt::~GCodeInt(void)
{
	if (m_gparser != NULL)
		delete m_gparser;

	itCurrentStmt = slist.begin();
	while (itCurrentStmt != slist.end()) {
		GCodeStmt *gstmt = *itCurrentStmt;

		delete gstmt;
		itCurrentStmt++;
	}

	slist.clear();
	probePoints->clear();
	gparameters.clear();

	delete probePoints;
}

bool GCodeInt::LoadFile()
{
	int fileHandle = open(m_filePath.c_str(), O_RDONLY|_O_BINARY);

	/*m_in.open(m_filePath, ifstream::in);

    if (!m_in.is_open()) {
        out_err << "Unable to open file: " << m_filePath << endl;
        return false;
    }*/

	if (fileHandle == -1) {
		out_err << "Unable to open file: " << m_filePath << endl;
        return false;
	}
	
	QProgressDialog *dialog = new QProgressDialog();

	int size = lseek(fileHandle, 0, SEEK_END);

	dialog->setRange(0, size);
	dialog->setModal(false);
	dialog->show();

	lseek(fileHandle, 0, SEEK_SET);

    bool definedMillRouteDepth = false;

	GCodeLexer *lexer = new GCodeLexer(fileHandle);
	m_gparser = new GCodeParser(lexer);

    gi.BoardMinX = numeric_limits<Real>::infinity();
    gi.BoardMinY = numeric_limits<Real>::infinity();
    gi.BoardMaxX = -numeric_limits<Real>::infinity();
    gi.BoardMaxY = -numeric_limits<Real>::infinity();

	//Preprocess command list
	QTime time;

	time.start();

	m_gparser->Init();
	while (!m_gparser->IsAtEnd()) {
		
		GCodeStmt *gs;

		if (!m_gparser->GetNextStatement(gs)) {
			dialog->close();
			return false;
		}

		dialog->setValue(tell(fileHandle));

		if (gs == NULL)
			continue;

		switch (gs->GetKind()) {
			case ASSIGN_STMT: {
				GCodeAssign *assign_stmt = (GCodeAssign *)gs;
				string varname = assign_stmt->GetVariable();
				Real value = EvalExpr(assign_stmt->GetExpr());

				gparameters[varname] = value;
				delete gs;
				break;
			}
			case COMMAND_STMT: {
				GCodeCommand *cmd_stmt = (GCodeCommand *)gs;

				switch ( cmd_stmt->GetOpcode() ) {
                    case G20: gi.UnitType = UNIT_INCHES; slist.push_back(gs); break;
                    case G21: gi.UnitType = UNIT_MM; slist.push_back(gs); break;

					case G82:
                    case G81:
						moveTo(*cmd_stmt, gi.Pos);
						UpdateBoardArea(gi.Pos.x, gi.Pos.y, gi);
						
						slist.push_back(gs);
						break;
					default:
						if (cmd_stmt->IsMotionCommand()) {
							/*
							 * We have a move command, if our z is below zero then this will
							 * count towards our area
							 */
                            moveToWithEval(*cmd_stmt, gi.Pos);

							if (gi.Pos.z < 0) {
								if (!definedMillRouteDepth || (gi.Pos.z < gi.MillRouteDepth))
									gi.MillRouteDepth = gi.Pos.z;

								UpdateBoardArea(gi.Pos.x, gi.Pos.y, gi);
							}

                        }
                        slist.push_back(gs);

						break;
				}
				break;
			}
			case SUBCALL_STMT: {
				GCodeSubCall *subcall_stmt = (GCodeSubCall *)gs;

				/* Is this a probe point? */
				if (subcall_stmt->GetSubID() == _O(100) &&
					subcall_stmt->GetArgumentCount() > 2) {

					GExpr *arg0 = subcall_stmt->GetArgument(0); // First argument is X coordinate
					GExpr *arg1 = subcall_stmt->GetArgument(1); // Second argument is Y coordinate

					Real x_value = EvalExpr(arg0);
					Real y_value = EvalExpr(arg1);
					Position p;

					p.x = x_value;
					p.y = y_value;

					UpdateBoardArea(x_value, y_value, gi);

					probePoints->push_back(p);
				}
				delete gs;
				break;
			}
		}
    }

	dialog->close();
	delete lexer;
	close(fileHandle);

	int difference = time.elapsed();
	QMessageBox::information(NULL, "Duration", QString("Elapsed Time ") + QString::number(difference) + "ms");
 
	return true;
}

void GCodeInt::Init()
{
	itCurrentStmt = slist.begin();
    m_currentPos.reset();
}

bool GCodeInt::ExecuteNextStatement()
{
	if (itCurrentStmt == slist.end())
		return false;

	m_curCmd = NULL;
	GCodeStmt *gstmt = *itCurrentStmt;
	switch (gstmt->GetKind()) {
			/* 
			 * At this point we only consider motion and drill commands, all other (assigments, probe points)
			 * we ignore them here, they were processed when the file was loaded.
			 */
			case COMMAND_STMT: {
				m_curCmd = (GCodeCommand *)gstmt;

				if (m_curCmd->IsMotionCommand())
					moveTo(*m_curCmd, m_currentPos);
                else if ( m_curCmd->IsA(G82) )
					moveTo(*m_curCmd, m_currentPos);
                else if ( m_curCmd->IsA(G81) )
                    moveTo(*m_curCmd, m_currentPos);
			}
			default:
				/* Nothing to do with the other staments */
				break;
	}

	itCurrentStmt++;
	return true;
}

static inline Real DoOperation(Real val1, Real val2, int op)
{
	switch (op ) {
		case ADD_EXPR: return val1 + val2;
		case SUB_EXPR: return val1 - val2;
		case MUL_EXPR: return val1 * val2;
		case DIV_EXPR: return val1 / val2;
		default:
			return 0.0;
	}
}

Real GCodeInt::EvalExpr(GExpr *expr)
{
	switch (expr->GetKind()) {
		case NUMBER_EXPR: {
			GNumberExpr *nexpr = (GNumberExpr *)expr;
			return nexpr->GetValue();
		}
		case VREF_EXPR: {
			GVarRefExpr *vrexpr = (GVarRefExpr *)expr;
			string varName = vrexpr->GetVarName();

            Real value = 0.0;
            if (gparameters.find(varName) != gparameters.end())
                value = gparameters[varName];

            vrexpr->SetValue(value);

            return value;
		}
		case ADD_EXPR:
		case SUB_EXPR:
		case MUL_EXPR:
		case DIV_EXPR: {
            GBinaryExpr *bexpr = (GBinaryExpr *)expr;
            GExpr *lexpr = bexpr->GetLExpr();
            GExpr *rexpr = bexpr->GetRExpr();
			Real val1 = EvalExpr(lexpr);
			Real val2 = EvalExpr(rexpr);

            Real value = DoOperation(val1, val2, expr->GetKind());
            bexpr->SetValue(value);

            return value;
		}
		default:
			return 0.0;
	}
}
