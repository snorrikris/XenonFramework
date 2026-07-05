module;

// NOTE - the original source code for the tooltips is from XSuperTooltip by Hans Dietrich on CodeProject.

//
//	Class:		CPPTooltip
//
//	Compiler:	Visual C++
//	Tested on:	Visual C++ 6.0
//				Visual C++ .NET 2003
//
//	Version:	See GetVersionC() or GetVersionI()
//
//	Created:	xx/xxxx/2004
//	Updated:	21/November/2004
//
//	Author:		Eugene Pustovoyt	pustovoyt@mail.ru
//
//	Disclaimer
//	----------
//	THIS SOFTWARE AND THE ACCOMPANYING FILES ARE DISTRIBUTED "AS IS" AND WITHOUT
//	ANY WARRANTIES WHETHER EXPRESSED OR IMPLIED. NO REPONSIBILITIES FOR POSSIBLE
//	DAMAGES OR EVEN FUNCTIONALITY CAN BE TAKEN. THE USER MUST ASSUME THE ENTIRE
//	RISK OF USING THIS SOFTWARE.
//
//	Terms of use
//	------------
//	THIS SOFTWARE IS FREE FOR PERSONAL USE OR FREEWARE APPLICATIONS.
//	IF YOU WISH TO THANK MY WORK, YOU MAY DONATE ANY SUM OF MONEY TO ME 
//  FOR SUPPORT OF DEVELOPMENT OF THIS CLASS.
//	IF YOU USE THIS SOFTWARE IN COMMERCIAL OR SHAREWARE APPLICATIONS YOU
//	ARE GENTLY ASKED TO DONATE ANY SUM OF MONEY TO THE AUTHOR:
//
//
//--- History ------------------------------ 
// 2004/03/01 *** Releases version 2.0 ***
//------------------------------------------
//		2004/04/04 [ADD] Added method SetCssStyles(DWORD dwIdCssStyle, LPCTSTR lpszPathDll /* = NULL */)
//		2004/04/14 [FIX] Fixed correct drawing for some tooltip's directions
//		2004/04/15 [FIX] Fixed changing a z-order of the some windows by show a tooltip on Win9x
//		2004/04/27 [FIX] Corrected a work with a tooltip's directions with a large tooltip
//		2004/04/28 [ADD] Disables a message translation if object was't created (thanks to Stoil Todorov)
//		2004/07/02 [UPD] Changes a GetWndFromPoint mechanism of the window's searching
//		2004/09/01 [ADD] New SetMaxTipWidth method was added
//		2004/10/12 [FIX] Now a tooltip has a different methods to show a menu's tooltip and other 
//							control's tooltip
//------------------------------------------
// 2004/11/19 *** Releases version 2.1 ***
//------------------------------------------
//		2004/11/30 [FIX] Corrected the debug window drawing
//		           [FIX] Changes a GetWndFromPoint mechanism of the window's searching
////////////////////////////////////////////////////////////////////
//
// "SmoothMaskImage" and "GetPartialSums" functions by Denis Sarazhinsky (c)2003
// Modified by Eugene Pustovoyt to use with image's mask instead of full color image.
//
/////////////////////////////////////////////////////////////////////
//

//
//--- History ------------------------------ 
// 2004/03/01 *** Releases version 2.0 ***
//------------------------------------------
// 2004/04/04 [ADD] Added method SetCssStyles(DWORD dwIdCssStyle, LPCTSTR lpszPathDll /* = NULL */)
// 2004/04/14 [FIX] Fixed correct drawing for some tooltip's directions
// 2004/04/15 [FIX] Fixed changing a z-order of the some windows by show a tooltip on Win9x
// 2004/04/27 [FIX] Corrected a work with a tooltip's directions with a large tooltip
// 2004/04/28 [ADD] Disables a message translation if object was't created (thanks to Stoil Todorov)
// 2004/07/02 [UPD] Changes a GetWndFromPoint mechanism of the window's searching
// 2004/09/01 [ADD] New SetMaxTipWidth method was added
// 2004/10/12 [FIX] Now a tooltip has a different methods to show a menu's tooltip and other 
//					control's tooltip
// 2006/07/31 [UPD] Updated by Hans Dietrich for XSuperTooltip - see +++hd comments

////////////////////////////////////////////////////////////////////
//
// "SmoothMaskImage" and "GetPartialSums" functions by Denis Sarazhinsky (c)2003
// Modified by Eugene Pustovoyt to use with image's mask instead of full color image.
//
/////////////////////////////////////////////////////////////////////

#if 0  // Example of generated HTML:
< font color = #000000 > <b>Merge and Center< / b>
<indent>
Joins the selected cells into one larger cell and centers the contents in the new cell. < / indent>
<indent>
This is often used to create labels that span multiple columns.< / indent>< / font><indent size = -50><br>
<hr width = 500px color = #69696B>< / indent>
<table>
<tr>
<td width = 20><ilst idres = 131 mask cx = 16 cy = 16 width = 100 % height = 100 %>< / td><br>
<td><b><font color = #000000>Press F1 for more help< / b>< / td>< / font>< / tr>< / table>
#endif // end example

#include "os_minimal.h"
#include <string>
#include <format>
#include <d2d1.h>
#include <dwrite.h>

export module Xe.PPTooltip;

import Xe.UIcolorsIF;
import Xe.Helpers;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define CPPTOOLTIP_TRACE XeTRACE
#define CPPTOOLTIP_TRACE (__noop)		// +++hd
//#define MM_CPPTOOLTIP_TRACE XeTRACE
#define MM_CPPTOOLTIP_TRACE (__noop)	// +++sk

export class CPPToolTip
{
#pragma region Class_data
protected:
	CXeUIcolorsIF* m_xeUI = nullptr;

	HWND m_hParentWnd = 0; // The handle of the parent window

	CPoint m_ptOriginal;
#pragma endregion Class_data

#pragma region Create
public:
	CPPToolTip(CXeUIcolorsIF* pUIcolors, HWND hParentWnd)
			: m_xeUI(pUIcolors), m_hParentWnd(hParentWnd)
	{
		CPPTOOLTIP_TRACE("CPPToolTip::Create\n");
		XeASSERT(m_hParentWnd);	// Parent window not created yet.
	}
	//virtual ~CPPToolTip() {}
#pragma endregion Create

public:
	BOOL RelayMessageToTooltip(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		XeASSERT(m_hParentWnd);
		CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		if (uMsg == WM_NCMOUSEMOVE)
		{
			::ScreenToClient(hWnd, &pt);
		}
		CPPTOOLTIP_TRACE("CPPToolTip::RelayEvent - msg=0x%04X, pt=(%d,%d)\n", uMsg, pt.x, pt.y);
		if (!(uMsg == WM_NCMOUSEMOVE || uMsg == WM_MOUSEMOVE)
				|| (m_ptOriginal.x == pt.x && m_ptOriginal.y == pt.y))
		{
			return false;
		}
		// The mouse pointer's position was changed

		PPTOOLTIP_INFO ti;
		NM_PPTOOLTIP_NEED_TT lpnm(&pt, &ti);
		CPPTOOLTIP_TRACE("CPPToolTip::SendNotifyNeedTT()\n");
		if (::SendMessage(m_hParentWnd, WM_NOTIFY, lpnm.hdr.idFrom, (LPARAM)&lpnm))
		{
			CPPTOOLTIP_TRACE("CPPToolTip - SetNewTooltip(hWnd=0x%08X, (left=%d, top=%d, right=%d, bottom=%d))\n",
				ti.hWndTTparent, ti.rectBounds.left, ti.rectBounds.top, ti.rectBounds.right, ti.rectBounds.bottom);

			// Prevent tootip from showing if any mouse button down or if shift or alt down.
			CXeMouseKeyHelper mkh;
			CXeShiftCtrlAltKeyHelper sca;
			if (!mkh.IsNoneDown() || sca.IsShiftOrAltDown())
			{
				CPPTOOLTIP_TRACE("CPPToolTip - PreventTooltipShowing\n");
				return false;
			}

			m_ptOriginal = pt;
			m_xeUI->SetNewTooltip(m_hParentWnd, pt, ti);
		}
		return false;
	}
};

