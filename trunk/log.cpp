// log.cpp - diagnostic logging
//
#include "windows.h"
#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <new>

void Log(const char* fmt, ...)
{
	static char buffer[8192], *bufptr=buffer;
	va_list arg_ptr;
	va_start( arg_ptr, fmt );
	if (_vsnprintf(buffer, sizeof buffer, fmt, arg_ptr) < 0) {
		// presumably buffer overflow
		const int BIGSIZE = 65536;
		bufptr = new char[BIGSIZE];
		if (!bufptr) {
			throw std::bad_alloc();
		}
		if (_vsnprintf(bufptr, BIGSIZE, fmt, arg_ptr) < 0) {
			delete[] bufptr;
			throw "CodeGenerator::emitf buffer overflow";
		}
	}
	fputs(bufptr, stderr);
#ifdef _WIN32
	OutputDebugString(bufptr);
#endif
	if (bufptr != buffer) {
		delete[] bufptr;
	}
	va_end(arg_ptr);
}
