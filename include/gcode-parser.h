/* 
 * File:   parser.h
 * Author: ideras
 *
 * Created on January 18, 2013, 6:52 PM
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

