#include "windows.h"

extern int jsmain_(...);

extern char *pzAppTitle_;

static int common_main(void)
{
	char buffer[_MAX_PATH];
	::GetModuleFileName(NULL, buffer, _MAX_PATH);
	char appname[_MAX_PATH];
	_splitpath(buffer, NULL, NULL, appname, NULL);
	pzAppTitle_ = strdup(appname);

	int nret = jsmain_();

	free(pzAppTitle_);
	return nret;
} // common_main

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	return common_main();
} // WinMain

int main(int argc, char *argv[])
{
	return common_main();
} // main


