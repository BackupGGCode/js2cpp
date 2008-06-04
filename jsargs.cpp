
#include "jsargs.h"
#include <stdio.h>
#include "sugar.h"

JsArgs::JsArgs(int argc, char *argv[])
: nFiles(0)
{
	szErr[0] = (char)0;
	memset(Filename, 0, sizeof Filename);
	for (int i = 1; (i < argc) && !szErr[0]; i++) {
		const char *arg = argv[i];
		if (*arg=='-' || *arg=='/') {
			// option or switch
			_snprintf(szErr, LENGTH(szErr), "unrecognized switch: %s", arg);
		} else if (nFiles==MAX_SRC_FILES) {
			if (!szErr[0]) {
				_snprintf(szErr, LENGTH(szErr), "more than %d source files", MAX_SRC_FILES);
			}
		} else {
			Filename[nFiles++] = strdup(arg);
		}
	}
}

JsArgs::~JsArgs()
{
	for (int i = 0; i < nFiles; i++) {
		free((void*)Filename[i]);
	}
}

