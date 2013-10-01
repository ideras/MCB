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

#include <cstdlib>
#include <sstream>

#include "gcode-lexer.h"

stringstream out_err;

string GCodeLexer::ParseInt()
{
	stringstream ss;

	while (isdigit(m_currentCh) && m_currentCh != EOF) {
			ss << m_currentCh;
			m_currentCh =  GetNextChar();
	}

	UngetChar();

	return ss.str();
}

Real GCodeLexer::ParseReal()
{
	stringstream ss;
	
	if (m_currentCh == '-' || m_currentCh == '+') {
		ss << m_currentCh;
		
		m_currentCh = GetNextChar();
	}
	while ( (isdigit(m_currentCh) || m_currentCh == '.') && m_currentCh != EOF ) {
			ss << m_currentCh;
			m_currentCh = GetNextChar();
	}
	UngetChar();

	string s_value = ss.str();

	m_tokenLexeme << s_value;

	return atof(s_value.c_str());
}

int GCodeLexer::NextToken()
{
	m_tokenLexeme.str("");

	while (1) {
		m_currentCh = GetNextChar();

		if (m_currentCh == EOF)
			return TOK_EOF;

		if (m_currentCh == ' ' || m_currentCh == '\t')
			continue;

		m_tokenLexeme << m_currentCh;
		m_currentCh = toupper(m_currentCh);

		switch (m_currentCh) {
			case '(': {
				m_currentCh = GetNextChar();
				while (m_currentCh != ')' && m_currentCh != EOF) {
					if (m_currentCh == '(') {
						out_err << "Nested comment at line " << m_lineNumber << endl;
						return TOK_ERROR;
					}
					m_currentCh = GetNextChar();
				}
				m_tokenLexeme.str("");
				continue;
			}
			case '\r': {
				m_currentCh = GetNextChar();

				if (m_currentCh != '\n')
					UngetChar();
				
				m_lineNumber ++;
				return TOK_EOL;
			}
			case '\n': {
				m_lineNumber ++;
				return TOK_EOL;
			}
			case 'N': {
				m_currentCh = GetNextChar();
				string s_value = ParseInt();
				m_value.m_intValue = atoi(s_value.c_str());
				m_tokenLexeme << s_value;
							
				return TOK_LINENUMBER;	  
			}
			case 'G': {
				int number1, number2;

				m_currentCh = GetNextChar();
				string s_value = ParseInt();
				number1 = atoi(s_value.c_str());
				m_tokenLexeme << s_value;

				m_currentCh = GetNextChar();
				if (m_currentCh == '.') {	
					m_currentCh = GetNextChar();
					s_value = ParseInt();
					number2 = atoi(s_value.c_str());
					m_tokenLexeme << s_value;

					return __G(number1, number2);
				}
				else
					UngetChar();
				
				return _G(number1);
			}
			case 'M': {

				m_currentCh = GetNextChar();
				string s_value = ParseInt();
				int number = atoi(s_value.c_str());
				m_tokenLexeme << s_value;
							
				return _M(number);
			}
			case 'O': {
				
				m_currentCh = GetNextChar();
				string s_value = ParseInt();
				int number = atoi(s_value.c_str());
				m_tokenLexeme << s_value;
							
				return _O(number);		  
			}
			case '#': {
				stringstream ss;

				m_currentCh = GetNextChar();
				string s_value = ParseInt();
				m_tokenLexeme << s_value;

				return TOK_VAR;
			}
			case EOF: return TOK_EOF;
			case '=': return TOK_OPEQ;
			case '[': return TOK_LBRACKET;
			case ']': return TOK_RBRACKET;
			case '+': return TOK_OPADD;
			case '-': return TOK_OPSUB;
			case '*': return TOK_OPMUL;
			case '/': return TOK_OPDIV;

			/* Command parameters */
			case 'X': return TOK_XARGUMENT;
			case 'Y': return TOK_YARGUMENT;
			case 'Z': return TOK_ZARGUMENT;
			case 'F': return TOK_FARGUMENT;
			case 'P': return TOK_PARGUMENT;
			case 'R': return TOK_RARGUMENT;
			case 'S': {
				stringstream ss;
				m_currentCh = GetNextChar();

				if (isalpha(m_currentCh)) {
					char ch;

					ss << "s";
					while (isalpha(m_currentCh) && m_currentCh != EOF) {
						ch = tolower(m_currentCh);
						ss << ch;
						m_tokenLexeme << m_currentCh;
						m_currentCh = GetNextChar();
					}
					UngetChar();
					string str = ss.str();

					if (str == "sub")
						return KW_SUB;
					else
						return TOK_ERROR;
				} else {
					UngetChar();
				
					return TOK_SARGUMENT;
				}
			}
			case 'T': return TOK_TARGUMENT;

			default: {
				
				if (isdigit(m_currentCh)) {
					m_value.m_realValue = ParseReal();
					return TOK_NUMBER;
				} else if (isalpha(m_currentCh)) {
					char ch;
					stringstream ss;

					while (isalpha(m_currentCh) && m_currentCh != EOF ) {
						ch = tolower(m_currentCh);
						ss << ch;
						m_tokenLexeme << m_currentCh;
						m_currentCh = GetNextChar();
					}
					UngetChar();
					string str = ss.str();

					if (str == "endsub")
						return KW_ENDSUB;
					else if (str == "call")
						return KW_CALL;
					else {
						out_err << "Invalid keyword '" << str << "' detected at line " << m_lineNumber << endl;
						return TOK_ERROR;
					}
				} else {
					out_err << "Invalid symbol '" << m_currentCh << "' detected at line " << m_lineNumber << " (0x" << hex << ((int)m_currentCh) << ")" << endl;
					return TOK_ERROR;
				}
			}

		}
	}
}
