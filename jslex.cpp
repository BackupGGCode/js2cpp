// jslex.cpp - Javascript tokenizer

#include "jslex.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define LENGTH(a) (sizeof(a)/sizeof((a)[0]))

const char * pzTokName[] = {
	"<end-of-file>",
	"<identifier>",
	"<number>",
	"<string>",
	"<regex>",
	"(",
	")",
	"{",
	"}",
	"[",
	"]",
	";",
	".",
	"new",
	"delete",
	"typeof",
	"void",
	"++",
	"--",
	"~",
	"!",
	"*",
	"/",
	"%",
	"+",
	"-",
	"<<",
	">>",
	">>>",
	"<",
	"<=",
	">",
	">=",
	"instanceof",
	"in",
	"==",
	"!=",
	"===",
	"!==",
	"&",
	"^",
	"|",
	"&&",
	"||",
	"?",
	":",
	"=",
	"+=",
	"-=",
	"/=",
	"*=",
	"%=",
	"<<=",
	">>=",
	">>>=",
	"&=",
	"^=",
	"|=",
	"&&=",
	"||=",
	",",
	"break",
	"do",
	"if",
	"switch",
	"case",
	"else",
	"this",
	"var",
	"catch",
	"false",
	"throw",
	"continue",
	"finally",
	"true",
	"while",
	"default",
	"for",
	"null",
	"try",
	"with",
	"function",
	"return",
	// reserved for future use
	"abstract",
	"boolean",
	"byte",
	"char",
	"class",
	"const",
	"debugger",
	"double",
	"enum",
	"export",
	"extends",
	"final",
	"float",
	"goto",
	"implements",
	"import",
	"int",
	"interface",
	"long",
	"native",
	"package",
	"private",
	"protected",
	"public",
	"short",
	"static",
	"super",
	"synchronized",
	"throws",
	"transient",
	"volatile",
	// pseudo-tokens to disambiguate nodes
	"<StatementList>",
	"<FunctionExpression>",
	"<ArrayLiteral>",
	"<invalid>",
};

namespace js2cpp {

	const char *Name(TokenType tt)
	{
		if (tt < 0 || tt >= LENGTH(pzTokName)) {
			return "?unknown tt?";
		}
		return pzTokName[tt];
	}
	
	bool isUnaryOp(TokenType tt)
	{
		return (tt >= tNEW && tt <= tBANG) || tt==tMINUS || tt==tPLUS;
	}


	Lexer::Lexer(SourceText *psrc, ErrorSink *perr)
	{
		m_nesting = 0;
		m_psrc = psrc;
		m_errSink = perr;
		m_ichar = m_line = 0;
		m_buf[0] = 0;
		m_sourceName = strdup(psrc->Title());
		m_ttPrev = tEOF;
		// Insert keywords into name table
		for (int i = tMIN_KEYWORD; i <= tMAX_KEYWORD; i++) {
			TokenType tt = (TokenType)i;
			Keyword(Name(tt), tt);
		}
		// And some guys that fall outside the normal keyword range:
		Keyword(Name(tIN), tIN);
		Keyword(Name(tINSTANCEOF), tINSTANCEOF);
		Keyword(Name(tNEW), tNEW);
		Keyword(Name(tTYPEOF), tTYPEOF);

#ifdef _DEBUG
		assert(0==tEOF);
		assert(0==strcmp(pzTokName[tEOF], "<end-of-file>"));
		assert(0==strcmp(pzTokName[tIDENT], "<identifier>"));
		assert(0==strcmp(pzTokName[tNUMBER], "<number>"));
		assert(0==strcmp(pzTokName[tSTRING], "<string>"));
		assert(0==strcmp(pzTokName[tREGEX], "<regex>"));
		assert((tREGEX-tIDENT)==3);
		assert(tNUMBER > tIDENT && tNUMBER < tREGEX);
		assert(tSTRING > tIDENT && tSTRING < tREGEX);
		assert(0==strcmp(pzTokName[tLPAREN], "("));
		assert(0==strcmp(pzTokName[tRPAREN], ")"));
		assert(0==strcmp(pzTokName[tLBRACE], "{"));
		assert(0==strcmp(pzTokName[tRBRACE], "}"));
		assert(0==strcmp(pzTokName[tLBRACKET], "["));
		assert(0==strcmp(pzTokName[tRBRACKET], "]"));
		assert(0==strcmp(pzTokName[tSEMI], ";"));
		assert(0==strcmp(pzTokName[tCOLON], ":"));
		assert(0==strcmp(pzTokName[tDOT], "."));
		assert(0==strcmp(pzTokName[tPLUSPLUS], "++"));
		assert(0==strcmp(pzTokName[tMINUSMINUS], "--"));
		assert(0==strcmp(pzTokName[tWIGGLE], "~"));
		assert(0==strcmp(pzTokName[tBANG], "!"));
		assert(0==strcmp(pzTokName[tSPLAT], "*"));
		assert(0==strcmp(pzTokName[tDIV], "/"));
		assert(0==strcmp(pzTokName[tREM], "%"));
		assert(0==strcmp(pzTokName[tPLUS], "+"));
		assert(0==strcmp(pzTokName[tMINUS], "-"));
		assert(0==strcmp(pzTokName[tSL], "<<"));
		assert(0==strcmp(pzTokName[tSRSX], ">>"));
		assert(0==strcmp(pzTokName[tSRZX], ">>>"));
		assert(0==strcmp(pzTokName[tLT], "<"));
		assert(0==strcmp(pzTokName[tLTE], "<="));
		assert(0==strcmp(pzTokName[tGT], ">"));
		assert(0==strcmp(pzTokName[tGTE], ">="));
		assert(0==strcmp(pzTokName[tINSTANCEOF], "instanceof"));
		assert(0==strcmp(pzTokName[tIN], "in"));
		assert(0==strcmp(pzTokName[tEQUALEQUAL], "=="));
		assert(0==strcmp(pzTokName[tNOTEQUAL], "!="));
		assert(0==strcmp(pzTokName[tIDENTITY], "==="));
		assert(0==strcmp(pzTokName[tNONIDENTITY], "!=="));
		assert(0==strcmp(pzTokName[tAND], "&"));
		assert(0==strcmp(pzTokName[tXOR], "^"));
		assert(0==strcmp(pzTokName[tOR], "|"));
		assert(0==strcmp(pzTokName[tLAND], "&&"));
		assert(0==strcmp(pzTokName[tLOR], "||"));
		assert(0==strcmp(pzTokName[tFROGMARK], "?"));
		assert(0==strcmp(pzTokName[tASSIGN], "="));
		assert(0==strcmp(pzTokName[tASSPLUS], "+="));
		assert(0==strcmp(pzTokName[tASSMINUS], "-="));
		assert(0==strcmp(pzTokName[tASSDIV], "/="));
		assert(0==strcmp(pzTokName[tASSMUL], "*="));
		assert(0==strcmp(pzTokName[tASSREM], "%="));
		assert(0==strcmp(pzTokName[tASSSL], "<<="));
		assert(0==strcmp(pzTokName[tASSSRSX], ">>="));
		assert(0==strcmp(pzTokName[tASSSRZX], ">>>="));
		assert(0==strcmp(pzTokName[tASSAND], "&="));
		assert(0==strcmp(pzTokName[tASSXOR], "^="));
		assert(0==strcmp(pzTokName[tASSOR], "|="));
		assert(0==strcmp(pzTokName[tASSLAND], "&&="));
		assert(0==strcmp(pzTokName[tASSLOR], "||="));
		assert(0==strcmp(pzTokName[tCOMMA], ","));
		assert(0==strcmp(pzTokName[tBREAK], "break"));
		assert(0==strcmp(pzTokName[tDO], "do"));
		assert(0==strcmp(pzTokName[tIF], "if"));
		assert(0==strcmp(pzTokName[tSWITCH], "switch"));
		assert(0==strcmp(pzTokName[tTYPEOF], "typeof"));
		assert(0==strcmp(pzTokName[tCASE], "case"));
		assert(0==strcmp(pzTokName[tELSE], "else"));
		assert(0==strcmp(pzTokName[tTHIS], "this"));
		assert(0==strcmp(pzTokName[tVAR], "var"));
		assert(0==strcmp(pzTokName[tCATCH], "catch"));
		assert(0==strcmp(pzTokName[tFALSE], "false"));
		assert(0==strcmp(pzTokName[tTHROW], "throw"));
		assert(0==strcmp(pzTokName[tVOID], "void"));
		assert(0==strcmp(pzTokName[tCONTINUE], "continue"));
		assert(0==strcmp(pzTokName[tFINALLY], "finally"));
		assert(0==strcmp(pzTokName[tNEW], "new"));
		assert(0==strcmp(pzTokName[tTRUE], "true"));
		assert(0==strcmp(pzTokName[tWHILE], "while"));
		assert(0==strcmp(pzTokName[tDEFAULT], "default"));
		assert(0==strcmp(pzTokName[tFOR], "for"));
		assert(0==strcmp(pzTokName[tNULL], "null"));
		assert(0==strcmp(pzTokName[tTRY], "try"));
		assert(0==strcmp(pzTokName[tWITH], "with"));
		assert(0==strcmp(pzTokName[tDELETE], "delete"));
		assert(0==strcmp(pzTokName[tFUNCTION], "function"));
		assert(0==strcmp(pzTokName[tRETURN], "return"));
		assert(0==strcmp(pzTokName[tVOLATILE], "volatile"));
		assert(0==strcmp(pzTokName[tSTATLIST], "<StatementList>"));
		assert(0==strcmp(pzTokName[tFUNEX], "<FunctionExpression>"));
		assert(0==strcmp(pzTokName[tARRAYLIT], "<ArrayLiteral>"));

		assert(isUnaryOp(tNEW));
		assert(isUnaryOp(tVOID));
		assert(isUnaryOp(tMINUSMINUS));
		assert(isUnaryOp(tPLUSPLUS));
		assert(isUnaryOp(tMINUS));
		assert(isUnaryOp(tPLUS));
		assert(isUnaryOp(tWIGGLE));
		assert(isUnaryOp(tBANG));
		assert(isUnaryOp(tDELETE));
		assert(isUnaryOp(tTYPEOF));
		assert(!isUnaryOp(tDOT));
		assert(!isUnaryOp(tSPLAT));

#endif
	}

	Lexer::~Lexer()
	{
		free((void*)m_sourceName);
	}

	void Lexer::Push(void)
	{
		m_stack[m_nesting++] = *this;
	} // Push

	void Lexer::Pop(void)
	{
		assert(m_nesting > 0);
		if (m_nesting > 0) {
			*(SourceState*)this = m_stack[--m_nesting];
		}
	}

	void Lexer::Include(SourceText* psrc)
	{
		Push();
		m_psrc = psrc;
		m_buf[0] = 0;
		m_ichar = 0;
		m_line = 0;
		m_sourceName = strdup(psrc->Title());
	} // Include

	TokenType Lexer::GetToken(Token &token)
	{
		// Handle whitespace
		while (true) {
			char c = m_buf[m_ichar++];
			switch (c) {
			case '/':
				if (m_buf[m_ichar]=='*') {
					// start of multiline comment.
					// look for closing '*/'
					m_ichar++;
					while (m_buf[m_ichar]!='*' || m_buf[m_ichar+1]!='/') {
						if (m_buf[m_ichar]==0) {
							// end of buffer
							m_ichar = 0;
							if (m_psrc->ReadLine(m_buf, MAX_LINE)) {
								m_line++;
								fprintf(stderr, "%s[%03d] %s", m_sourceName, m_line, m_buf);
								continue;
							}
							Error(ERR_EOF_IN_COMMENT, token);
							strcpy(m_buf, "*/");
							break;
						} else {
							m_ichar++;
						}
					} // while
					// skip over ending */
					m_ichar += 2;
					// go back to skipping whitespace:
					continue;
				}
				if (m_buf[m_ichar]!='/') {
					// Slash not followed by '*' or '/' - start of token.
					break;
				}
				// Otherwise - fall through!
			case 0:
			case 0x0A:		// LF
			case 0x0C:		// FF
			case 0x0D:		// CR
				// end of line
				m_ichar = 0;
				if (m_psrc->ReadLine(m_buf, MAX_LINE)) {
					m_line++;
					// echo source line to output
					fprintf(stderr, "%s[%03d] %s", m_sourceName, m_line, m_buf);
					continue;
				}
				m_buf[m_ichar++] = 0;
				break;

			// whitespace - skip
			case 0x09:		// HT
			case ' ':
				continue;
			default:
				break;
			} // switch
			--m_ichar;
			break;
		} // while

		token.m_sourceName = m_sourceName;
		token.m_line = m_line;
		token.m_ichar = m_ichar;
		token.m_name = NULL;
		char c = m_buf[m_ichar++];
		switch (c) {
		case 0:
			token.m_type = tEOF;
			break;
		case '(':
			token.m_type = tLPAREN;
			break;
		case ')':
			token.m_type = tRPAREN;
			break;
		case ';':
			token.m_type = tSEMI;
			break;
		case '{':
			token.m_type = tLBRACE;
			break;
		case '}':
			token.m_type = tRBRACE;
			break;
		case '[':
			token.m_type = tLBRACKET;
			break;
		case ']':
			token.m_type = tRBRACKET;
			break;
		case ',':
			token.m_type = tCOMMA;
			break;
		case ':':
			token.m_type = tCOLON;
			break;
		case '?':
			token.m_type = tFROGMARK;
			break;
		case '~':
			token.m_type = tWIGGLE;
			break;
		case '%':
			token.m_type = tREM;
			break;
		case '^':
			token.m_type = tXOR;
			break;
		case '.':
			token.m_type = tDOT;
			break;
		case '!':
			if (m_buf[m_ichar]=='=') {
				m_ichar++;
				if (m_buf[m_ichar]=='=') {
					m_ichar++;
					token.m_type = tNONIDENTITY;
				} else {
					token.m_type = tNOTEQUAL;
				}
			} else {
				token.m_type = tBANG;
			}
			break;
		case '=':
			if (m_buf[m_ichar]=='=') {
				m_ichar++;
				if (m_buf[m_ichar]=='=') {
					m_ichar++;
					token.m_type = tIDENTITY;
				} else {
					token.m_type = tEQUALEQUAL;
				}
			} else {
				token.m_type = tASSIGN;
			}
			break;
		case '<':
			if (m_buf[m_ichar]=='=') {
				m_ichar++;
				token.m_type = tLTE;
			} else if (m_buf[m_ichar]=='<') {
				m_ichar++;
				if (m_buf[m_ichar]=='=') {
					token.m_type = tASSSL;
				} else {
					token.m_type = tSL;
				}
			} else {
				token.m_type = tLT;
			}
			break;
		case '>':
			if (m_buf[m_ichar]=='=') {
				m_ichar++;
				token.m_type = tGTE;
			} else if (m_buf[m_ichar]=='>') {
				m_ichar++;
				if (m_buf[m_ichar]=='>') {
					m_ichar++;
					if (m_buf[m_ichar]=='=') {
						m_ichar++;
						token.m_type = tASSSRZX;
					} else {
						token.m_type = tSRZX;
					}
				} else {
					token.m_type = tSRSX;
				}
			} else {
				token.m_type = tGT;
			}
			break;
		case '+':
			if (m_buf[m_ichar]=='+') {
				m_ichar++;
				token.m_type = tPLUSPLUS;
			} else if (m_buf[m_ichar]=='=') {
				m_ichar++;
				token.m_type = tASSPLUS;
			} else {
				token.m_type = tPLUS;
			}
			break;
		case '-':
			if (m_buf[m_ichar]=='-') {
				m_ichar++;
				token.m_type = tMINUSMINUS;
			} else if (m_buf[m_ichar]=='=') {
				m_ichar++;
				token.m_type = tASSMINUS;
			} else {
				token.m_type = tMINUS;
			}
			break;
		case '*':
			if (m_buf[m_ichar]=='=') {
				m_ichar++;
				token.m_type = tASSMUL;
			} else {
				token.m_type = tSPLAT;
			}
			break;
		case '&':
			if (m_buf[m_ichar]=='&') {
				m_ichar++;
				if (m_buf[m_ichar]=='=') {
					m_ichar++;
					token.m_type = tASSLAND;
				} else {
					token.m_type = tLAND;
				}
			} else if (m_buf[m_ichar]=='=') {
				m_ichar++;
				token.m_type = tASSAND;
			} else {
				token.m_type = tAND;
			}
			break;
		case '|':
			if (m_buf[m_ichar]=='|') {
				m_ichar++;
				if (m_buf[m_ichar]=='=') {
					m_ichar++;
					token.m_type = tASSLOR;
				} else {
					token.m_type = tLOR;
				}
			} else if (m_buf[m_ichar]=='=') {
				m_ichar++;
				token.m_type = tASSOR;
			} else {
				token.m_type = tOR;
			}
			break;

		case '"':
		case '\'':
			// quoted string
			token.m_type = tSTRING;
			while (m_buf[m_ichar]!=c && m_buf[m_ichar]) {
				if (m_buf[m_ichar]=='\\' && m_buf[m_ichar+1]) {
					m_ichar++;
				}
				m_ichar++;
			}
			if (m_buf[m_ichar]==c) {
				// eat up ending quote
				m_ichar++;
			} else {
				Error(ERR_UNTERM_STRING, token);
			}
			SetTokenText(token);
			break;

		case '/':
			// either division operator /
			// or division-assignment /=
			// or! start of regular expression - regex
			if (isAssOp(m_ttPrev) ||
				m_ttPrev == tLPAREN ||
				m_ttPrev == tCOMMA ||
				m_ttPrev == tCOLON) {
				// probably regex
				token.m_type = tREGEX;
				while (m_buf[m_ichar]!=c && m_buf[m_ichar]) {
					if (m_buf[m_ichar]=='\\' && m_buf[m_ichar+1]) {
						m_ichar++;
					}
					m_ichar++;
				}
				if (m_buf[m_ichar]==c) {
					m_ichar++;
					// pick up any flags ('i' 'g' or 'm' following closing slash)
					while (m_buf[m_ichar]=='i' || m_buf[m_ichar]=='g' || m_buf[m_ichar]=='m') {
						m_ichar++;
					}
				} else {
					Error(ERR_UNTERM_REGEX, token);
				}
				SetTokenText(token);
			} else if (m_buf[m_ichar]=='=') {
				// assignment
				m_ichar++;
				token.m_type = tASSDIV;
			} else {
				// assuming operator
				token.m_type = tDIV;
			}
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				token.m_type = tNUMBER;
				while (isdigit(m_buf[m_ichar])) {
					m_ichar++;
				}
				if (m_buf[m_ichar]=='.') {
					// decimal point, parse fractional digits
					m_ichar++;
					if (!isdigit(m_buf[m_ichar])) {
						Error(ERR_DIGIT_AFTER_DOT, token);
					}
					while (isdigit(m_buf[m_ichar])) {
						m_ichar++;
					}
				}
				if (m_buf[m_ichar]=='e' || m_buf[m_ichar]=='E') {
					// exponent part
					m_ichar++;
					if (m_buf[m_ichar]=='+' || m_buf[m_ichar]=='-') {
						m_ichar++;
					}
					if (!isdigit(m_buf[m_ichar])) {
						Error(ERR_NO_DIGITS_IN_EXP, token);
					}
					while (isdigit(m_buf[m_ichar])) {
						m_ichar++;
					}
				}
				SetTokenText(token);
				break;
			}

		default:
			if (isalpha(c) || c=='_' || c=='$') {
				// start of identifier
				token.m_type = tIDENT;
				c = m_buf[m_ichar];
				while (isalpha(c) || c=='_' || isdigit(c) || c=='$') {
					c = m_buf[++m_ichar];
				}
				SetTokenText(token);
			} else {
				Error(ERR_UNK_CHAR, token);
				token.m_type = tEOF;
			}
			break;
		}
		if (!token.m_name) {
			token.m_name = Name(token.m_type);
		}
		if (token.m_type==tEOF && m_nesting > 0) {
			Pop();
			return GetToken(token);
		}
		m_ttPrev = token.m_type;
		return token.m_type;
	} // GetToken

	void Lexer::SetTokenText(Token &token)
	{
		int n = (m_ichar - token.m_ichar);
		const char *pz = m_buf+token.m_ichar;
		STAtom *atom = m_strtab.Intern(pz, n);
		token.m_name = atom->m_name;
		if (token.m_type==tIDENT && atom->m_id!=0) {
			// It's already in the table, and with a special id
			token.m_type = (TokenType)atom->m_id;
		}
	}

	void Lexer::Keyword(const char *str, TokenType tt)
	{
		STAtom *atom = m_strtab.Intern(str);
		atom->m_id = (int)tt;
	}

	void Lexer::Error(int err, const Token &token)
	{
		const char *msg;
		switch (err) {
		case ERR_EOF_IN_COMMENT:
			msg = "EOF in /*..*/ comment";
			break;
		case ERR_UNTERM_STRING:
			msg = "string literal, EOL inside string or missing close-quote";
			break;
		case ERR_UNTERM_REGEX:
			msg = "regular expression literal: no closing '/'";
			break;
		case ERR_DIGIT_AFTER_DOT:
			msg = "in number, decimal point not followed by digit";
			break;
		case ERR_NO_DIGITS_IN_EXP:
			msg = "in number, no digits in exponent";
			break;
		case ERR_UNK_CHAR:
			msg = "unknown character";
			break;
		}
		m_errSink->Error(m_sourceName, token.m_line, token.m_ichar, err, msg);
	}

}; // namespace js2cpp