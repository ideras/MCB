#include <stdlib.h>
#include <sstream>

#include "gcode-parser.h"

extern stringstream out_err;

void GCodeParser::SkipEOL()
{
	while (m_currentToken == TOK_EOL) 
		m_currentToken = m_lexer->NextToken();
}

char GCodeParser::TokenArgumentToName(unsigned int tkParam)
{
	switch (tkParam) {
		case TOK_XARGUMENT: return 'X';
		case TOK_YARGUMENT: return 'Y';
		case TOK_ZARGUMENT: return 'Z';
		case TOK_FARGUMENT: return 'F';
		case TOK_PARGUMENT: return 'P';
		case TOK_RARGUMENT: return 'R';
		case TOK_SARGUMENT: return 'S';
		case TOK_TARGUMENT: return 'T';
		default:
			return '?';
	}
}

bool GCodeParser::MatchToken(int token, string tokenName)
{
	if (m_currentToken != token) {
		out_err << "Expected '" << tokenName << "' at line " << m_lexer->GetLineNumber() << " found '" << m_lexer->GetLexeme() << "'" << endl;
		return false;
	}
	m_currentToken = m_lexer->NextToken();

	return true;
}

bool GCodeParser::ParseExpr(GExpr * &expr)
{
	GExpr *expr1;

	expr = 0;
	if (!ParseTerm(expr1))
		return false;

	while (m_currentToken == TOK_OPADD || m_currentToken == TOK_OPSUB) {
		GExpr *expr2;
		
		if (m_currentToken == TOK_OPADD)
			expr1 = new GAddExpr(expr1, 0);
		else
			expr1 = new GSubExpr(expr1, 0);

		m_currentToken = m_lexer->NextToken();
		
		if (!ParseTerm(expr2)) {
			delete expr1;

			return false;
		}

		((GBinaryExpr *)expr1)->SetRExpr(expr2);
	}

	expr = expr1;
	return true;
}

bool GCodeParser::ParseTerm(GExpr * &expr)
{
	GExpr *expr1;

	expr = 0;
	if (!ParseFactor(expr1))
		return false;

	while (m_currentToken == TOK_OPMUL || m_currentToken == TOK_OPDIV) {
		GExpr *expr2;
		
		if (m_currentToken == TOK_OPMUL)
			expr1 = new GMulExpr(expr1, 0);
		else
			expr1 = new GDivExpr(expr1, 0);

		m_currentToken = m_lexer->NextToken();
		
		if (!ParseFactor(expr2)) {
			delete expr1;

			return false;
		}

		((GBinaryExpr *)expr1)->SetRExpr(expr2);
	}

	expr = expr1;
	return true;
}

bool GCodeParser::ParseFactor(GExpr * &expr)
{
	expr = 0;
	switch (m_currentToken) {
		case TOK_OPADD: {
			m_currentToken = m_lexer->NextToken();
			return ParseFactor(expr);
		}
		case TOK_OPSUB: {
			GExpr *expr1;

			m_currentToken = m_lexer->NextToken();

			if (!ParseFactor(expr1))
				return false;

			expr = new GMulExpr(new GNumberExpr(-1.0), expr1);
			return true;
		}
		case TOK_NUMBER: {
			expr = new GNumberExpr(m_lexer->GetRealValue());
			m_currentToken = m_lexer->NextToken();

			return true;
		}
		case TOK_LBRACKET: {
			m_currentToken = m_lexer->NextToken();
			if (!ParseExpr(expr))
				return false;
			
			if (m_currentToken != TOK_RBRACKET) {
				out_err << "Expected ']' at line " << m_lexer->GetLineNumber() << ", found '" << m_lexer->GetLexeme() << "'" << endl;
				delete expr;
				expr = 0;
				return false;
			}
			m_currentToken = m_lexer->NextToken();
			return true;
		}
		case TOK_VAR: {
			string varName = m_lexer->GetLexeme();
			expr = new GVarRefExpr(varName);

			m_currentToken = m_lexer->NextToken();

			return true;
		}
		default:
			out_err << "Unexpected '" << m_lexer->GetLexeme() << "' at line " << m_lexer->GetLineNumber() << ", expected NUMBER, '[' or parameter" << endl;
			return false;
	}
}

bool GCodeParser::ParseParameterValue(GExpr * &expr)
{
	double mult = 1.0;

	if (m_currentToken == TOK_OPSUB || m_currentToken == TOK_OPADD) {
		mult = m_currentToken == TOK_OPSUB? -1.0 : 1.0;
		m_currentToken = m_lexer->NextToken();
	}

	switch (m_currentToken) {
		case TOK_NUMBER: {
			Real value = mult * m_lexer->GetRealValue();

			m_currentToken = m_lexer->NextToken();

			expr = new GNumberExpr(value);
			break;
		}
		case TOK_LBRACKET: {
			m_currentToken = m_lexer->NextToken();
			if (!ParseExpr(expr))
				return false;

			if (m_currentToken != TOK_RBRACKET) {
				out_err << "Error in command at line " << m_lexer->GetLineNumber() << ", expected ']'" << endl;
				delete expr;
				expr = 0;
				return false;
			}
			m_currentToken = m_lexer->NextToken();
			break;
		}

		default:
			out_err << "Error in command at line " << m_lexer->GetLineNumber() << ", expected NUMBER or '['" << endl;
			return false;
	}

	return true;
}

bool GCodeParser::ParseArguments(GCodeCommand *gcmd)
{
	while (1) {
		if ( (m_currentToken == TOK_EOF) || 
			 (m_currentToken == TOK_EOL) ||
			 IsGCommand(m_currentToken) || 
			 IsMCommand(m_currentToken) ||
			 IsOCommand(m_currentToken) )
			return true;

		switch (m_currentToken) {			
			case TOK_XARGUMENT:
			case TOK_YARGUMENT:
			case TOK_ZARGUMENT:
			case TOK_FARGUMENT:
			case TOK_PARGUMENT:
			case TOK_RARGUMENT:
			case TOK_SARGUMENT: 
			case TOK_TARGUMENT: {
				char argName = TokenArgumentToName(m_currentToken);
				GExpr *paramValue = NULL;

				m_currentToken = m_lexer->NextToken();

				if (!ParseParameterValue(paramValue))
					return false;

				gcmd->SetArgument(argName, paramValue);
				break;
			}
			case TOK_ERROR: return false;
			default: 
				out_err << "Expected argument or command at line " << m_lexer->GetLineNumber() << ", found '" << m_lexer->GetLexeme() << "'" << endl;
				return false;
		}
	}
}

bool  GCodeParser::ParseNextStatement(GCodeStmt *&stmt)
{
	stmt = NULL;
	if (IsGCommand( m_currentToken ) || IsMCommand (m_currentToken) ) {
		GCodeCommand *gcmd = new GCodeCommand();

        m_lastCommand = m_currentToken;
		gcmd->SetOpcode(m_currentToken);
		gcmd->SetName(m_lexer->GetLexeme());

		/* Now we parse the command parameters, if any */
		m_currentToken = m_lexer->NextToken();

		if (!ParseArguments(gcmd)) {
			delete gcmd;
			return false;
		}

		stmt = gcmd;
		return true;

	} else if IsOCommand(m_currentToken) {
		int subID = m_currentToken;
		string subName = m_lexer->GetLexeme();

		m_currentToken = m_lexer->NextToken();
		switch (m_currentToken) {
			case KW_SUB: {
				/* Subroutine declaration.  For now we just ignore them */
				m_currentToken = m_lexer->NextToken();
				while (m_currentToken != subID && m_currentToken != TOK_EOF)
					m_currentToken = m_lexer->NextToken();

				m_currentToken = m_lexer->NextToken();
				if (!MatchToken(KW_ENDSUB, "endsub"))
					return false;

				stmt = NULL;
				return true;
			}
			case KW_ENDSUB: {
				/* Oops dangling 'endsub' */
				out_err << "'endsub' without a previous subroutine declaration" << endl;
				stmt = 0;
				return false;
			}
			case KW_CALL: {
				GCodeSubCall *gcall = new GCodeSubCall(subName, subID);

				m_currentToken = m_lexer->NextToken();
				while (m_currentToken != TOK_EOL && m_currentToken != TOK_EOF) {
					GExpr *argExpr;

					if (!ParseExpr(argExpr)) {
						delete gcall;

						return false;
					}
					gcall->AddArgument(argExpr);
				}

				stmt = gcall;
				return true;
			}
			default:
				out_err << "Error at line " << m_lexer->GetLineNumber() << ", unexpected '" << m_lexer->GetLexeme() << "', expected 'sub', 'endsub', 'call'" << endl;
				return false;
		}
	
	} else if (m_currentToken == TOK_VAR) {
		string varName = m_lexer->GetLexeme();
		GExpr *expr = NULL;

		m_currentToken = m_lexer->NextToken();
		if (!MatchToken(TOK_OPEQ, "="))
			return false;

		if (!ParseExpr(expr))
			return false;

		stmt = new GCodeAssign(varName, expr);

		return true;
	} else {
		GCodeCommand *gcmd = new GCodeCommand();
        gcmd->SetOpcode( m_lastCommand );

		if (!ParseArguments(gcmd)) {
			delete gcmd;
			return false;
		}

		stmt = gcmd;
		return true;
	}
}

bool GCodeParser::ParseAll(list<GCodeStmt *> &slist)
{
	GCodeStmt *gs;

	m_currentToken = m_lexer->NextToken();
	SkipEOL();

	while (m_currentToken != TOK_EOF) {
		
		if (!ParseNextStatement(gs))
			return false;

		if (gs == NULL)
			continue;

		slist.push_back(gs);
		gs = 0;

		SkipEOL();
    }

	return true;

}

