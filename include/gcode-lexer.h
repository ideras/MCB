/* 
 * File:   gcode-lexer.h
 * Author: Ivan Deras
 *
 * Created on June 121, 2013, 8:18 AM
 */

#ifndef GCODE_LEXER_H
#define	GCODE_LEXER_H

#include <fstream>
#include <sstream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _MSC_VER
#include <io.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif

using namespace std;

typedef long double Real;

/* Token Definitions */
#define _G(n)			((unsigned int)((1 << 13) | (n & 0x00FF)))
#define __G(n1, n2)		((unsigned int)((1 << 13) | ((n2 << 8) & 0x1F00) | (n1 & 0x00FF)))
#define _O(n)			((unsigned int)((1 << 14) | (n & 0x3FFF)))
#define _M(n)			((unsigned int)((1 << 15) | (n & 0xFF)))
#define IsGCommand(n)	( (n & (1 << 13)) != 0 )
#define IsOCommand(n)	( (n & (1 << 14)) != 0 )
#define IsMCommand(n)	( (n & (1 << 15)) != 0 )

/* Common GCodes definitions */
#define G00		_G(0)
#define G01		_G(1)
#define G20		_G(20)
#define G21		_G(21)
#define G38_2	__G(38, 2)
#define G82		_G(82)
#define G81		_G(81)

#define GNOP			0x3FFF

#define TOK_EOF				(1 << 16)
#define TOK_XARGUMENT		(2 << 16)
#define TOK_YARGUMENT		(3 << 16)
#define TOK_ZARGUMENT		(4 << 16)
#define TOK_FARGUMENT		(5 << 16)
#define TOK_PARGUMENT		(6 << 16)
#define TOK_RARGUMENT		(7 << 16)
#define TOK_SARGUMENT		(8 << 16)
#define TOK_TARGUMENT		(9 << 16)
#define TOK_LINENUMBER	(20 << 16)
#define TOK_VAR			(21 << 16)
#define TOK_OPEQ		(22 << 16)
#define TOK_LBRACKET	(23 << 16)
#define TOK_RBRACKET	(24 << 16)
#define TOK_OPADD		(25 << 16)
#define TOK_OPSUB		(26 << 16)
#define TOK_OPMUL		(27 << 16)
#define TOK_OPDIV		(28 << 16)

#define KW_SUB			(30 << 16)
#define KW_ENDSUB		(31 << 16)
#define KW_CALL			(32 << 16)

#define TOK_NUMBER		(40 << 16)
#define TOK_EOL			(50 << 16)
#define TOK_ERROR		(51 << 16)

#define BUF_SIZE 4096

/* GCode Tokenizer */
class GCodeLexer
{
public:
	GCodeLexer(int fhandle) { 
		m_fhandle = fhandle; 
		m_lineNumber = 1; 
		FillBuffer(1);
		FillBuffer(2);
		ptr = &buf1[0];
		fillInactiveBuffer = false;
	}

	Real GetRealValue() { return m_value.m_realValue; }
	Real GetIntValue() { return m_value.m_intValue; }
	string GetLexeme() { return m_tokenLexeme.str(); }
	int GetLineNumber() { return m_lineNumber; }
	int NextToken();

private:
	string ParseInt();
	Real ParseReal();

	void FillBuffer(int buffNumber) {
		int bytes_read;

		char *bptr = (buffNumber == 1)? &buf1[0] : &buf2[0];
		bytes_read = read(m_fhandle, bptr, BUF_SIZE);

		if (bytes_read < BUF_SIZE)
			bptr[bytes_read] = EOF;
	}

	void IncPosition() { 
		if (ptr == &buf1[BUF_SIZE-1]) {
			if (fillInactiveBuffer)
				FillBuffer(2);
			ptr = &buf2[0];
			fillInactiveBuffer = true;
		} else if (ptr == &buf2[BUF_SIZE-1]) {
			if (fillInactiveBuffer)
				FillBuffer(1);

			fillInactiveBuffer = true;
			ptr = &buf1[0];
		} else
			ptr++;
	}

	char GetNextChar() {
		m_currentCh = *ptr; 
		IncPosition(); 
		return m_currentCh; 
	}

	void UngetChar() {
		if (ptr == &buf2[0]) { 
			ptr = &buf1[BUF_SIZE-1];
			fillInactiveBuffer = false;
		}
		else if (ptr == &buf1[0]) {
			ptr = &buf2[BUF_SIZE-1];
			fillInactiveBuffer = false;
		}
		else 
			ptr--;
	};

	stringstream m_tokenLexeme;
	
	union {
		int m_intValue;
		Real m_realValue;
	} m_value;

	char buf1[BUF_SIZE];
	char buf2[BUF_SIZE];
	char *ptr;
	bool fillInactiveBuffer;
	int m_lineNumber;
	char m_currentCh;
	int m_fhandle; //ifstream is slow for file access, maybe later I'll try memory mapped files
};

#endif
