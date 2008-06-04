// jsargs.h

#include <string>

#define MAX_SRC_FILES 128

class JsArgs
{
public:
	JsArgs(int argc, char* argv[]);
	~JsArgs();

	int			nFiles;						// number of source files
	const char* Filename[MAX_SRC_FILES];	// table of source files
	char		szErr[128];					// error message, if any
};

