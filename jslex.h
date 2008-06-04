// jslex.h - Javascript tokenizer
//
// This lexer converts a line-oriented stream of text
// into corresponding sequential Javascript tokens.
// The input stream must be UTF-8 encoded.
// Each token contains pointers to the text of the token
// and the name of the source stream: These pointers
// become invalid when the lexer is destroyed.

#ifndef JSLEX_H
#define JSLEX_H

#include "jssrctxt.h"
#include "errcodes.h"
#include "stringtab.h"
#include <stddef.h>		// for NULL mainly

namespace js2cpp {


#define MAX_LINE 256

class Token;
class ErrorSink;

typedef enum {
	tEOF,			// end of file
	tIDENT,			// identifier (not keyword)
	tNUMBER,		// number
	tSTRING,		// string
	tREGEX,			// regular expression
	tLPAREN,		// (
	tRPAREN,		// )
	tLBRACE,		// {
	tRBRACE,		// }
	tLBRACKET,		// [
	tRBRACKET,		// ]
	tSEMI,			// ;
	tDOT,			// .
	// unary operators
	tNEW,
	tDELETE,
	tTYPEOF,
	tVOID,
	tPLUSPLUS,		// ++
	tMINUSMINUS,	// --
	tWIGGLE,		// ~ (bitwise not)
	tBANG,			// !
	// begin binary operators
	// precedence 13
	tSPLAT,			// *
	tDIV,			// /
	tREM,			// % (remainder)
	// precedence 12
	tPLUS,			// +
	tMINUS,			// -
	// precedence 11
	tSL,			// << (shift left)
	tSRSX,			// >> (shift right sign extend)
	tSRZX,			// >>> (shift right zero extend)
	// precedence 10
	tLT,			// <
	tLTE,			// <=
	tGT,			// >
	tGTE,			// >=
	tINSTANCEOF,
	tIN,
	// precedence 9
	tEQUALEQUAL,	// ==
	tNOTEQUAL,		// !=
	tIDENTITY,		// ===
	tNONIDENTITY,	// !==
	// precedence 8
	tAND,			// &
	// precedence 7
	tXOR,			// ^
	// precedence 6
	tOR,			// |
	// precedence 5
	tLAND,			// &&
	// precedence 4
	tLOR,			// ||
	// precedence 3
	tFROGMARK,		// ?
	tCOLON,			// :
	// precedence 2
	tASSIGN,		// =
	tASSPLUS,		// +=
	tASSMINUS,		// -=
	tASSDIV,		// /=
	tASSMUL,		// *=
	tASSREM,		// %=
	tASSSL,			// <<=
	tASSSRSX,		// >>=
	tASSSRZX,		// >>>=
	tASSAND,		// &=
	tASSXOR,		// ^=
	tASSOR,			// |=
	tASSLAND,		// &&=
	tASSLOR,		// ||=
	// precedence 1
	tCOMMA,			// ,
	// end binary operators
	// Keywords
	tBREAK,
	tDO,
	tIF,
	tSWITCH,
	tCASE,
	tELSE,
	tTHIS,
	tVAR,
	tCATCH,
	tFALSE,
	tTHROW,
	tCONTINUE,
	tFINALLY,
	tTRUE,
	tWHILE,
	tDEFAULT,
	tFOR,
	tNULL,
	tTRY,
	tWITH,
	tFUNCTION,
	tRETURN,
	//
	// reserved for possible future language versions or extensions
	tABSTRACT,
	tBOOLEAN,
	tBYTE,
	tCHAR,
	tCLASS,
	tCONST,
	tDEBUGGER,
	tDOUBLE,
	tENUM,
	tEXPORT,
	tEXTENDS,
	tFINAL,
	tFLOAT,
	tGOTO,
	tIMPLEMENTS,
	tIMPORT,
	tINT,
	tINTERFACE,
	tLONG,
	tNATIVE,
	tPACKAGE,
	tPRIVATE,
	tPROTECTED,
	tPUBLIC,
	tSHORT,
	tSTATIC,
	tSUPER,
	tSYNCHRONIZED,
	tTHROWS,
	tTRANSIENT,
	tVOLATILE,
	// special pseudo-tokens needed for ASTs, these
	// are used to disambiguate AST nodes that could
	// otherwise be confused, or that (like a statement sequence)
	// do not have a defining token.
	tSTATLIST,			// a sequence of statements
	tFUNEX,				// function expression (otherwise same as tFUNCTION)
	tARRAYLIT,			// literal array
	tINVALID,			// an invalid tree/node
} TokenType;

#define tMIN_KEYWORD tBREAK
#define tMAX_KEYWORD tVOLATILE

const char *Name(TokenType tt);
// Return a string (suitable for error reporting) describing the token type.

inline bool isAssOp(TokenType tt)
// true if tt is an assignment operator (=, +=, -=, *=, etc.)
{
	return (tt >= tASSIGN && tt <= tASSLOR);
}

bool isUnaryOp(TokenType tt);

typedef enum {
	ERR_EOF_IN_COMMENT,
	ERR_UNTERM_STRING,
	ERR_UNTERM_REGEX,
	ERR_DIGIT_AFTER_DOT,
	ERR_NO_DIGITS_IN_EXP,
	ERR_UNK_CHAR,
} LexicalError;


class SourceState
{
public:
	SourceText*			m_psrc;			// source text stream
	char				m_buf[MAX_LINE];
	int					m_line;			// current line number (from 1)
	int					m_ichar;		// char position in line
	const char*			m_sourceName;	// title of source stream
};

class Lexer : private SourceState
{
public:
	Lexer(SourceText *psrc, ErrorSink *perr);
	~Lexer();

	TokenType GetToken(Token &token);

	void Include(SourceText *psrc);		// insert a text stream at current token position

private:
	SourceState			m_stack[32];	// source stream stack
	int					m_nesting;		// stack depth
	ErrorSink*			m_errSink;		// object that accepts errors
	TokenType			m_ttPrev;		// type of previous token
	StringTable			m_strtab;		// shared string table

	void Push(void);
	void Pop(void);
	// Push/pop the current source state onto the stack

	void SetTokenText(Token &token);
	// Take the text collected so far in the buffer and
	// turn it into a token.

	void Keyword(const char *str, TokenType tt);

	void Error(int err, const Token &token);

}; // Lexer

class Token
{
public:
	TokenType		m_type;
	const char	   *m_name;			// name (note, pointer is unique)
	int				m_line;			// line in source
	int				m_ichar;		// character in line
	const char	   *m_sourceName;	// name of source

}; // Token

} // namespace

#endif
