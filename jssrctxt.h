// jssrctxt.h - abstract text source

#ifndef jssrctxt_h
#define jssrctxt_h

namespace js2cpp {

class SourceText
{
public:
	SourceText();

	virtual ~SourceText();
	// Note: Destructor calls Close

	virtual const char *Title() { return m_pzTitle; }
	// name (for error reporting)

	virtual bool ReadLine(char *buffer, int buflen) = 0;
	// Read next line from source text into buffer.
	// Include EOL character(s), and append a NUL(0).
	// Return true if something was read, false if at EOF.

	virtual bool AtEOF(void) { return m_bEOF; }

	virtual void Close(void);
	// by default just sets AtEOF.

protected:
	bool	m_bEOF;
	char*	m_pzTitle;
}; // SourceText


} // namespace js2cpp

#endif
