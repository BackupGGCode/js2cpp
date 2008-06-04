// js2cpp.cpp : javascript to C++ translator
//

#include <stdio.h>
#include "version.h"
#include "sugar.h"
#include "jsargs.h"
#include "jssrctxt.h"
#include "jsparse.h"
#include "ASTprint.h"
#include "AST.h"
#include "codegen.h"

#ifdef _WIN32
#include "windows.h"
#endif

using namespace js2cpp;

class SourceFile : public SourceText
{
public:
	SourceFile();
	bool Open(const char *filename);
	virtual bool ReadLine(char *buffer, int buflen);
	virtual void Close(void);
	const char *LastError(void);

private:
	FILE   *m_file;
	bool	m_bWide;		// source file is wide
	char	m_szErr[128];	// last error, if any
};

class CodeOut : public CodeSink
{
public:
	CodeOut(FILE* f) : m_f(f) {}
	~CodeOut() { fclose(m_f); m_f=NULL; }

	virtual void emit(const char *s);
private:
	FILE *m_f;
};

class ErrorOut : public ErrorSink
{
public:
	ErrorOut(const char *pref);
	~ErrorOut();
	int Errors(void) const { return nErrors; }

	virtual void Error(const char *sourceName, int line, int ichar, int err, const char *msg);
	virtual void Warning(const char *sourceName, int line, int ichar, int err, const char *msg);

private:
	char*	prefix;
	int		nErrors;
	int		nWarnings;
};


void translate(SourceFile *psrc, CodeSink* pcpp, ErrorSink *perr, JsArgs &args)
{
	Lexer lex(psrc, perr);
	Parser parser(&lex, perr);
	AST* tree = parser.Parse();
	FILE *lst = fopen("c:\\parsetree.txt", "w");
	if (lst) {
		fprintf(lst, "----- Parse Tree -----\n");
		TreePrint(lst, tree);
		fprintf(lst, "----------------------\n");
		fclose(lst);
	}
	CodeGenerator coder(pcpp, perr);
	coder.Program(tree);
	delete tree;
} // translate


int main(int argc, char* argv[])
{
	int nExit = 0;

	printf("js2cpp v%d.%02d.%d  built %s\n", APP_MAJOR, APP_MINOR, APP_BUILD, __DATE__);

	JsArgs args(argc, argv);
	const char *pzSrc = args.Filename[0];
	SourceFile src;

	if (args.szErr[0]) {
		// argument error
		fprintf(stderr, "argument error: %s\n", args.szErr);
		nExit = 1;
	} else if (args.nFiles < 1) {
		fprintf(stderr, "no source files specified\n");
		nExit = 2;
	} else if (!src.Open(pzSrc)) {
		fprintf(stderr, "%s\n", src.LastError());
		nExit = 3;
	} else {
		char szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szName[_MAX_FNAME], szExt[_MAX_EXT];
		char szCpp[_MAX_PATH];
		char szSourceFile[_MAX_PATH];
		_splitpath(pzSrc, szDrive, szDir, szName, szExt);
		_makepath(szCpp, szDrive, szDir, szName, "cpp");
		_makepath(szSourceFile, NULL, NULL, szName, szExt);
		
		FILE *cf = fopen(szCpp, "w");
		if (!cf) {
			fprintf(stderr, "%s: %s", strerror(errno), szCpp);
			nExit = 4;
		} else {
			CodeOut code(cf);
			ErrorOut errOut(szSourceFile);
			translate(&src, &code, &errOut, args);
			if (errOut.Errors() > 0) {
				nExit = 21;
			}
		}
	}
	return nExit;
} // main


///////////////////////////////////////////////////////////////////////
// Code sink adapter

void CodeOut::emit(const char *s)
{
	fputs(s, m_f);
#ifdef _WIN32
#ifdef _DEBUG
	OutputDebugString(s);
#endif
#endif
}

///////////////////////////////////////////////////////////////////////
// Error sink adapter

ErrorOut::ErrorOut(const char *pref)
: prefix(strdup(pref))
{
	nErrors = nWarnings = 0;
}

ErrorOut::~ErrorOut()
{
	char buf[256];
	sprintf(buf, "\n%s - %d error(s), %d warning(s)\n", prefix, nErrors, nWarnings);
	fputs(buf, stderr);
#ifdef _WIN32
	OutputDebugString(buf);
#endif
	free(prefix);
}

void ErrorOut::Error(const char *sourceName, int line, int ichar, int err, const char *msg)
{
	char buf[256];
	sprintf(buf, "%s(%d,%d) : error %d: %s\n", sourceName, line, ichar, err, msg);
	fputs(buf, stderr);
#ifdef _WIN32
	OutputDebugString(buf);
#endif
	nErrors++;
}

void ErrorOut::Warning(const char *sourceName, int line, int ichar, int err, const char *msg)
{
	char buf[256];
	sprintf(buf, "%s(%d,%d) : warning %d: %s\n", sourceName, line, ichar, err, msg);
	fputs(buf, stderr);
#ifdef _WIN32
	OutputDebugString(buf);
#endif
	nWarnings++;
}




///////////////////////////////////////////////////////////////////////
// SourceFile adapter class

SourceFile::SourceFile()
: m_file(NULL)
{
}

const char *SourceFile::LastError(void)
{
	return m_szErr;
}

bool SourceFile::Open(const char *filename)
{
	Close();
	m_szErr[0] = '\0';
	m_file = fopen(filename, "r");
	if (!m_file) {
		_snprintf(m_szErr, LENGTH(m_szErr), "%s: %s", strerror(errno), filename);
		return false;
	}
	// Clear the end-of-file flag:
	m_bEOF = false;
	m_pzTitle = strdup(filename);
	return true;
} // Open

void SourceFile::Close(void)
{
	if (m_file) {
		fclose(m_file); m_file = NULL;
	}
	if (m_pzTitle) {
		free(m_pzTitle); m_pzTitle = NULL;
	}
	SourceText::Close();
} // Close

bool SourceFile::ReadLine(char *buffer, int buflen)
{
	if (m_bEOF || !fgets(buffer, buflen, m_file)) {
		m_bEOF = true;
		*buffer = '\0';
		return false;
	}
	return true;
} // ReadLine
