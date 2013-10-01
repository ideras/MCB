#ifndef GCODE_INT_H
#define GCODE_INT_H
#include <string>
#include <list>
#include <map>
#include "gcode-parser.h"
#include "gcode-ir.h"

using namespace std;

struct Position {
    Real x;
    Real y;
    Real z;

	Position() { reset(); }

	void reset() { x = y = z = 0; }
};

struct GCodeInfo
{
    int UnitType; //Milimiters by default
    Position Pos;
    
    /* Board boundaries */
    Real BoardMinX;
    Real BoardMinY;
    Real BoardMaxX;
    Real BoardMaxY;

    //Route Depth
    Real MillRouteDepth;    
};

class GCodeInt
{
public:
	GCodeInt(string filePath);
	~GCodeInt(void);
	bool LoadFile();
	bool ExecuteNextStatement();
	Position GetCurrentPos() { return m_currentPos; }
	bool HasProbePoints() { return !probePoints->empty(); }
	int GetMeasureUnits() { return gi.UnitType; }
	list<Position> *GetProbePoints() { return probePoints; }
	bool HasStatements() { return !slist.empty(); }
	GCodeCommand *GetCurrentCommand() { return m_curCmd; }
	GCodeInfo *GetGCodeInfo() { return &gi; }
	string GetFilePath() { return m_filePath; }
    int GetStatementCount() { return slist.size(); }
	void Init();

private:

    void moveToWithEval(GCodeCommand &cmd, Position &pos) {

		if (cmd.HasArgument('X')) {
			Real value = EvalExpr(cmd.GetArgument('X'));
			pos.x = value;
		}

		if (cmd.HasArgument('Y')) {
			Real value = EvalExpr(cmd.GetArgument('Y'));
			pos.y = value;
		}

		if (cmd.HasArgument('Z')) {
			Real value = EvalExpr(cmd.GetArgument('Z'));
			pos.z = value;
		}
	}

    void moveTo(GCodeCommand &cmd, Position &pos) {

        if (cmd.HasArgument('X')) {
            Real value = cmd.GetArgument('X')->GetValue();
            pos.x = value;
        }

        if (cmd.HasArgument('Y')) {
            Real value = cmd.GetArgument('Y')->GetValue();
            pos.y = value;
        }

        if (cmd.HasArgument('Z')) {
            Real value = cmd.GetArgument('Z')->GetValue();
            pos.z = value;
        }
    }

	Real EvalExpr(GExpr *expr);

	string m_filePath;
	ifstream m_in;
	GCodeParser *m_gparser;
	map<string, Real> gparameters;	//Symbol table to store the value of GCODE parameters
	Position m_currentPos;
	GCodeInfo gi;
	GCodeCommand *m_curCmd;
	list<Position> *probePoints;
	list<GCodeStmt *> slist;
	list<GCodeStmt *>::iterator itCurrentStmt;
};

#endif
