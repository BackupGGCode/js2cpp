// jssrctxt.cpp

#include "jssrctxt.h"

namespace js2cpp {

SourceText::SourceText()
: m_bEOF(true), m_pzTitle(0)
{
}

void SourceText::Close(void)
{
	m_bEOF = true;
}

SourceText::~SourceText()
{
	Close();
}

} // namespace js2cpp