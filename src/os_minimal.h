#pragma once

#define NOMINMAX
#include <Windows.h>

//bool _TRACE(const char* format, ...);

#include <stdint.h>
#include <commctrl.h>

#define __WRL_NO_DEFAULT_LIB__
#define __WRL_CLASSIC_COM_STRICT__
#include <wrl/client.h>

#include "XeAssert.h"

#include "Xe_res.h"

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif

// Undocumented windows messages.
constexpr UINT WM_NCUAHDRAWCAPTION = 0x00AE;
constexpr UINT WM_NCUAHDRAWFRAME = 0x00AF;

