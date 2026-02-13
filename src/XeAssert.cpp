#include "XeAssert.h"

#define NOMINMAX
#include <Windows.h>
#pragma comment(lib, "user32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool XeTRACE(const char* format, ...)
{
	char buffer[1000];

	va_list argptr;
	va_start(argptr, format);
	::wvsprintfA(buffer, format, argptr);
	va_end(argptr);

	::OutputDebugStringA(buffer);

	return true;
}
