#pragma once

extern "C"
{

#ifdef _DEBUG
#define XeASSERT(cond) { if (! (cond)) { __debugbreak(); } }
#else
#define XeASSERT(cond) ((void) 0)
#endif

}	// extern "C"

bool XeTRACE(const char* format, ...);
