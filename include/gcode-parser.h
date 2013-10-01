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

#ifndef PARSER_H
#define	PARSER_H

#include <string>
#include <list>
#include <map>

#include "gcode-lexer.h"
#include "gcode-ir.h"

using namespace std;

/* Supported measurement units */
#define UNIT_INCHES     0
#define UNIT_MM         1

class GCodeLexer;

/* GCode Parser */
class GCodeParser
{
public:
    GCodeParser(GCodeLexer *lexer) { m_lexer = lexer; m_lastCommand = GNOP; }
    ~GCodeParser() { }
	bool ParseAll(list<GCodeStmt *> &slist);
	void Init() { m_currentToken = m_lexer->NextToken(); SkipEOL(); }
	bool IsAtEnd() { return m_currentToken == TOK_EOF; }
	
	bool GetNextStatement(GCodeStmt *&stmt) { 
		bool result = ParseNextStatement(stmt);
		SkipEOL();

		return result;
	}
private:
	void SkipEOL();
	bool ParseArguments(GCodeCommand *gcmd);
	bool ParseNextStatement(GCodeStmt *&stmt);
	bool ParseParameterValue(GExpr * &expr);
	bool ParseExpr(GExpr * &expr);
	bool ParseTerm(GExpr * &expr);
	bool ParseFactor(GExpr * &expr);
	bool MatchToken(int token, string tokenName);
	char TokenArgumentToName(unsigned int tkParam);

	/* Member fields */
	GCodeLexer *m_lexer;
	int m_currentToken;
    int m_lastCommand;
};

#endif	/* PARSER_H */

