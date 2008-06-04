// errcodes.h - error (and warning) codes
//
#ifndef ERRCODES_H
#define ERRCODES_H

namespace js2cpp {

	typedef enum {
		W_EXTRA_SEMI,		// extra semicolon
		W_MISSING_SEMI,		// missing semicolon
		E_EXPECTED,			// expected <token-type> at <token>
		E_SEMANTIC,			// error found during semantic analysis
	} ErrorCode;

	// virtual class for accepting errors
	class ErrorSink
	{
	public:
		virtual void Error(const char *sourceName, int line, int ichar, int err, const char *msg) = 0;
		// Report error code err with message msg in source file sourceName at line and char position ichar.
		
		virtual void Warning(const char *sourceName, int line, int ichar, int err, const char *msg) = 0;
		
	}; // ErrorSink
	

} // namespace

#endif
