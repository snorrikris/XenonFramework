module;

#include "os_minimal.h"
#include <vector>
#include <memory>
#include <string>
#include <d2d1.h>
#include <dwrite.h>

export module Xe.ScrollBar;

import Xe.UIcolorsIF;
import Xe.D2DWndBase;
import Xe.Helpers;
//import Xe.HelpersMFC;
import Xe.FileHelpers;

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// XeScrollBarBase.cpp  Version 1.1
//     This class is designed as a base class for another class that will 
//     do all painting. This class handles all business logic. No painting
//     is done in this class - except in debug mode.
//
// Author:  Snorri Kristjansson
//          snorrikris@gmail.com
//
// History
//     Version 1.1 - 2010 October 21
//     - Changed base class to CScrollBar (was CWnd).
//     - Fixed many issues to make this scroll bar behave (almost) the same as windows
//       scroll bar.
//
//     Version 1.0 - 2009
//     - Never released.
//
// Acknowledgements:
//     Thanks to Hans Dietrich for his CXScrollBar class,
//     which I used as the starting point for CXeScrollBarBase:
//         http://www.codeproject.com/KB/miscctrl/XScrollBar.aspx
//     (I don't think any of his code survived into this version - but thanks all the same).
//
// License:
//     This software is released into the public domain.  You are free to use
//     it in any way you like, except that you may not sell this source code.
//
//     This software is provided "as is" with no expressed or implied warranty.
//     I accept no liability for any damage or loss of business that this
//     software may cause.
//
/////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************************
TODO:
	H Mouse wheel support (L/R wheel push) - WM_MOUSEHWHEEL (new message Vista and later).

	Change code to pure WIN32 - no MFC - use as base base class for CXeScrollBarBase.
****************************************************************************************/

/****************************************************************************************
Research resources:

Scroll bar MSDN WIN32 reference.
http://msdn.microsoft.com/en-us/library/bb787529(VS.85).aspx

Scroll Bar Controls in Win32
http://msdn.microsoft.com/en-us/library/ms997557.aspx

How/why to handle WM_CANCELMODE message.
http://support.microsoft.com/kb/74548/en-us

How/why to handle WM_GETDLGCODE message.
http://support.microsoft.com/kb/83302	<- dead link
http://web.archive.org/web/20150215154132/http://support.microsoft.com:80/kb/83302

Discussion about thumb size calculations and more (good one).
Broken link - http://blogs.msdn.com/oldnewthing/archive/2009/09/21/9897553.aspx
https://devblogs.microsoft.com/oldnewthing/20090921-00/?p=16653

Discussion about thumb size calculations.
http://social.msdn.microsoft.com/forums/en-US/wpf/thread/415eacf6-481e-4ebd-a6b0-2953e851183d/

From the November 2001 issue of MSDN Magazine.
Understanding CControlView, Changing Scroll Bar Color in MFC Apps
by Paul DiLascia
http://msdn.microsoft.com/en-us/magazine/cc301457.aspx

Handling Keyboard and Mouse Application Buttons in WTL
By Michael Dunn
http://www.codeproject.com/KB/wtl/WTLAppButtons.aspx

XScrollBar - Scroll bar like Windows Media Player's
By Hans Dietrich
http://www.codeproject.com/KB/miscctrl/XScrollBar.aspx

Developing a Custom Windows Scrollbar in MFC
By Don Metzler, DDJ June 01, 2003
http://www.drdobbs.com/windows/184416659

SkinControls 1.1 - A journey in automating the skinning of Windows controls
by .dan.g.
http://www.codeproject.com/KB/library/SkinCtrl.aspx

Custom Scrollbar Library version 1.1
by James Brown
http://www.codeproject.com/KB/dialog/coolscroll.aspx

Replace a Window's Internal Scrollbar with a customdraw scrollbar Control
By flyhigh
http://www.codeproject.com/KB/dialog/skinscrollbar.aspx
****************************************************************************************/

/****************************************************************************************
64 bit scrollbar.

SBM_GETSCROLLINFO uses SCROLLINFO64
SBM_SETSCROLLINFO uses SCROLLINFO64

Notes regarding WM_HSCROLL and WM_VSCROLL messages sent from 'this':
SB_THUMBPOSITION, SB_THUMBTRACK ignore the pos info - get using SBM_GETSCROLLINFO
****************************************************************************************/

/****************************************************************************************
CXeScrollBarBase32_64 class.
This class implements a very close copy of the standard windows scroll bar.
This class is designed as a base class for another class that will do all painting.
This class handles all the business logic.
No painting is done in this class - except in debug mode.

The derived class needs to do the following:
*	MUST override DrawScrollBar member function to do all the painting.
	Example:
	Create memory DC - same size as client area.
	XSB_EDRAWELEM eState;
	const CRect *prcElem = 0;
	stXSB_AREA stArea;
	for( int nElem = eTLbutton; nElem <= eThumb; nElem++ )	// loop through all UI elements.
	{
		stArea.eArea = (eXSB_AREA)nElem;

		// Get bounding rect of UI element to draw (client coords.)
		prcElem = GetUIelementDrawState( stArea.eArea, eState );
		if( !prcElem || eState == eNotDrawn )	// Rect empty or area not drawn?
			continue;

		// stArea.eArea identifies UI element to draw:
		//     eTLbutton or eTLchannel or eThumb or eBRchannel or eBRbutton.

		// eState identifes in what state the UI element is drawn:
		//     eDisabled or eNormal or eDown or eHot.

		// m_bHorizontal is TRUE if 'this' is a horizontal scroll bar.

		// Draw UI element to memory DC. (using prcElem rect).
		// Note - use m_bDrawGripper to determine if 'gripper' is drawn on thumb box.
		//        This is used to implement 'blinking' to show scroll bar has input
		//        focus. (every 500mS).
	}
	Copy memory DC to pDC.

*	(optional) Set m_uArrowWH to the width of the arrow button in horizontal scrollbar,
	note - height of arrow button in vertical scrollbar is the same.
	If this member is left unchanged (= 0) the arrow button width in horizontal scrollbar
	is assumed to be equal to the height of the window. Same for vertical scroll bar
	except the width of the window is arrow button height.
	This number is needed to calculate sizes of other UI elements of the scroll bar.

*	(optional) Set m_uThumbMinHW to the minimum allowed width of the thumb box in a
	horizontal scroll bar, this value is also the minimum height of the thumb box in a
	vertical scroll bar. The default is 8 pixels.
****************************************************************************************/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

constexpr wchar_t XESCROLLBARWND_CLASSNAME[] = L"XeScrollBarWndClass";  // Window class name

#define XSB_LBTN_DOWN_TIMERID	1001
#define XSB_LBTN_DOWN_TIME		200		// mS - time to first auto repeat.
#define XSB_LBTN_REPT_TIMERID	1002
#define XSB_LBTN_REPT_TIME		50		// mS - repeat interval.
#define XSB_FOCUS_TIMERID		1003
#define XSB_FOCUS_TIME			500		// mS - blink 'gripper' interval.

// Enum - UI element state - helps with drawing scrollbar.
enum class XSB_EDRAWELEM
{
	eNotDrawn = 0,	// UI element is not visible. (UI Rect empty).
	eDisabled,		// Element should be drawn in disabled state.
	eNormal,		// Element should be drawn in normal state.
	eDown,			// Element should be drawn in down state.
	eHot			// Element should be drawn in hot state.
					// Note - Scroll bar channel is usually not drawn 'hot'.
};

// Enum - For Scrollbar five UI elements:
//		Top (Left) arrow button.
//		Top (Left) channel (or shaft).
//		Thumb track button.
//		Bottom (Right) channel (or shaft).
//		Bottom (Right) arrow button.
enum class XSB_EAREA
{
	eNone = 0,
	eTLbutton,	// Top (Left) arrow button.
	eBRbutton,	// Bottom (Right) arrow button.
	eTLchannel,	// Top (Left) channel (or shaft).
	eBRchannel,	// Bottom (Right) channel (or shaft).
	eThumb		// Thumb track button.
};

// 'Helper' data structure - helps make the code readable.
struct XSB_AREA
{
	XSB_AREA() { eArea = XSB_EAREA::eNone; }
	XSB_AREA(XSB_EAREA e) : eArea(e) {}
	void operator=(XSB_EAREA e) { eArea = e; }
	void operator=(XSB_AREA& st) { eArea = st.eArea; }

	bool operator==(XSB_AREA& stB) { return (eArea == stB.eArea); }
	bool operator!=(XSB_AREA& stB) { return (eArea != stB.eArea); }

	bool IsNone() const { return (eArea == XSB_EAREA::eNone); }

	bool IsButton() const { return (eArea == XSB_EAREA::eTLbutton || eArea == XSB_EAREA::eBRbutton); }

	bool IsThumb() const { return (eArea == XSB_EAREA::eThumb); }

	bool IsChannel() const { return (eArea == XSB_EAREA::eTLchannel || eArea == XSB_EAREA::eBRchannel); }

	bool IsLeftButton() const { return (eArea == XSB_EAREA::eTLbutton); }

	bool IsRightButton() const { return (eArea == XSB_EAREA::eBRbutton); }

	bool IsUpButton() const { return (eArea == XSB_EAREA::eTLbutton); }

	bool IsDownButton() const { return (eArea == XSB_EAREA::eBRbutton); }

	bool IsLeftChannel() const { return (eArea == XSB_EAREA::eTLchannel); }

	bool IsRightChannel() const { return (eArea == XSB_EAREA::eBRchannel); }

	bool IsUpChannel() const { return (eArea == XSB_EAREA::eTLchannel); }

	bool IsDownChannel() const { return (eArea == XSB_EAREA::eBRchannel); }

	bool IsTLButton() const { return (eArea == XSB_EAREA::eTLbutton); }

	bool IsBRButton() const { return (eArea == XSB_EAREA::eBRbutton); }

	bool IsTLChannel() const { return (eArea == XSB_EAREA::eTLchannel); }

	bool IsBRChannel() const { return (eArea == XSB_EAREA::eBRchannel); }

	XSB_EAREA eArea;	// <- the only data member of this structure.
};

struct XeScrollBar_AreaInfo
{
	XSB_AREA		m_area;
	const CRect*	m_prcArea = nullptr;
	XSB_EDRAWELEM	m_eState;

	XeScrollBar_AreaInfo() = default;
	XeScrollBar_AreaInfo(XSB_EAREA eArea) : m_area(eArea) {}
};

export struct XeScrollBar_PaintInfo
{
	//CXeUIcolorsIF* m_xeUI = nullptr;
	//CDC*		m_pDC = nullptr;
	//CRect		m_rcC;
	//CBitmap*	m_pOldBm = nullptr;
	//
	//CDC			m_dcMem;	// Derived class will paint into this DC

	BOOL		m_bHorizontal;
	BOOL		m_bDrawGripper;
	std::vector<XeScrollBar_AreaInfo> m_areaInfo;

	ID2D1RenderTarget* m_pRT;
	D2D1_RECT_F rcC;
	
	//XeScrollBar_PaintInfo(CXeUIcolorsIF* pUIcolors, CWnd* pWnd, CDC* pDC, BOOL bHorizontal, BOOL bDrawGripper)
	//	: m_xeUI(pUIcolors), m_pDC(pDC), m_bHorizontal(bHorizontal), m_bDrawGripper(bDrawGripper)
	XeScrollBar_PaintInfo(ID2D1RenderTarget* pRT, D2D1_RECT_F rc, BOOL bHorizontal, BOOL bDrawGripper)
		: m_pRT(pRT), rcC(rc), m_bHorizontal(bHorizontal), m_bDrawGripper(bDrawGripper)
	{
		//pWnd->GetClientRect(&m_rcC);

		//// Draw to memory DC
		//m_dcMem.CreateCompatibleDC(pDC);
		//CBitmap bmMem;
		//bmMem.CreateCompatibleBitmap(pDC, m_rcC.Width(), m_rcC.Height());
		//m_pOldBm = m_dcMem.SelectObject(&bmMem);
	}

	//void EndPaint()
	//{
	//	// Copy bitmap in memory to the screen
	//	m_pDC->BitBlt(0, 0, m_rcC.Width(), m_rcC.Height(), &m_dcMem, 0, 0, SRCCOPY);

	//	m_dcMem.SelectObject(m_pOldBm);
	//}
};

///////////////////////////////////////////////////////////////////////////
// CXeScrollBarBase32_64 class.
// lowest level base class template for both 32bit and 64bit scroll bars.

// T_int is int32_t, T_uint is uint32_t for 32bit scrollbar.
// T_int is int64_t, T_uint is uint64_t for 64bit scrollbar.
template <typename T_int, typename T_uint>
class CXeScrollBarBase32_64 : public CXeD2DCtrlBase
{
public:
	bool m_isThisScrollBarInComboBoxDropDown = false;

#pragma region Create
	///////////////////////////////////////////////////////////////////////////
	// Construction / destruction

	CXeScrollBarBase32_64(CXeUIcolorsIF* pUIcolors) : CXeD2DCtrlBase(pUIcolors)
	{
		m_hParent = 0;
		m_bEnabled = m_bHorizontal = m_bDrawGripper = TRUE;
		m_bHasFocus = FALSE;
		m_bNoScroll = m_bDragging = m_bNeedEndScroll = FALSE;
		m_nPos = m_nTrackPos = m_nMinPos = m_nMaxPos = m_nMaxReportedPos = 0;
		m_uPage = 1;
		m_rectClient.SetRectEmpty();
		m_rectThumb.SetRectEmpty();
		m_rectTLArrow.SetRectEmpty();
		m_rectBRArrow.SetRectEmpty();
		m_rectTLChannel.SetRectEmpty();
		m_rectBRChannel.SetRectEmpty();
		m_uArrowWH = 0;
		m_uThumbMinHW = 8;				// Minimum thumb width or height
		m_dblPx_SU = 0;
		m_ptMenu.SetPoint(0, 0);
		m_xyThumbDragOffset = 0;
	}

	//virtual ~CXeScrollBarBase32_64();

	///////////////////////////////////////////////////////////////////////////
	// Create

	// Create 'this' type of scroll bar.
	// [IN]  dwStyle = Window style. (if SBS_VERT present a vertical scroll bar is created).
	// [IN]  rect = position and size of window in parent client coords.
	// [IN]  pParentWnd = Parent window. XeASSERTs if not valid.
	// [IN]  uID = Control identifier.
	// Returns TRUE if create was successful.
	// returns FALSE if create failed.
	bool Create(DWORD dwStyle, const CRect& rect, HWND hParentWnd, UINT nID, const wchar_t* tooltip)
	{
		// Note - CS_DBLCLKS style not used so we don't need to
		//        process WM_LBUTTONDBLCLK message.
		if (!m_xeUI->RegisterWindowClass(XESCROLLBARWND_CLASSNAME, D2DCtrl_WndProc,
				0 /* no class style - don't want double click */))
		{
			return false;
		}

		XeASSERT(hParentWnd);
		XeASSERT(IsWindow(hParentWnd));
		m_hParent = hParentWnd;

		m_bHorizontal = (dwStyle & SBS_VERT) ? FALSE : TRUE;
		// Note - SBS_HORZ is defined as 0 in winuser.h.

		m_bEnabled = (dwStyle & WS_DISABLED) ? FALSE : TRUE;

		HWND hWnd = CreateD2DCtrl(0, XESCROLLBARWND_CLASSNAME, nullptr, dwStyle, rect, m_hParent, nID, tooltip);
		if (hWnd)
		{
			RecalcRects();
		}
		else
		{
			//XeTRACE(L"ERROR - failed to create %s\n", XESCROLLBARWND_CLASSNAME);
			XeASSERT(FALSE);
		}
		return hWnd != 0;
	}

	// Create 'this' type of scroll bar from existing (windows standard) scroll bar.
	// New scroll bar is created by using the following from the existing scroll bar:
	//		* Window style. (if SBS_VERT present a vertical scroll bar is created).
	//		* Window size and position.
	//		* Control ID.
	//		* Z-order.
	//		* Scroll bar parameters (range, page and position).
	// [IN]  pParentWnd = Parent window. XeASSERTs if not valid.
	// [IN]  nID = Control identifier of exisiting scroll bar. XeASSERTs if not found.
	// [IN]  bUseDefaultWH = TRUE to use system default scroll bar width/height.
	//                     = FALSE to use width/height from existing scroll bar.
	// Returns new hWnd if create was successful, note - existing scroll has been destroyed.
	// returns 0 if create failed.
	//BOOL CreateFromExisting(CXeUIcolorsIF* pUIcolors, HWND hParentWnd, UINT nID, BOOL bUseDefaultWH = TRUE)
	//{
	//	m_xeUI = pUIcolors;
	//	if constexpr (sizeof(T_int) == 8)
	//	{
	//		XeASSERT(FALSE);	// Not (really) supported when 64bit scrollbar
	//	}
	//	XeASSERT(hParentWnd);
	//	if (!::IsWindow(hParentWnd))
	//	{
	//		XeASSERT(FALSE);
	//		return 0;
	//	}
	//	HWND hWndExistingSB = ::GetDlgItem(hParentWnd, nID);
	//	if (!hWndExistingSB)
	//	{
	//		XeASSERT(FALSE);
	//		return 0;
	//	}

	//	DWORD dwStyle = ::GetWindowLong(hWndExistingSB, GWL_STYLE);

	//	RECT rect;
	//	::GetWindowRect(hWndExistingSB, &rect);
	//	::ScreenToClient(hParentWnd, (LPPOINT)&rect);
	//	::ScreenToClient(hParentWnd, ((LPPOINT)&rect) + 1);
	//	if (bUseDefaultWH)
	//	{	// Set width or height to system standard scroll bar width/height.
	//		if (dwStyle & SBS_VERT)
	//		{
	//			// Get width of 'sys' vert. SB.
	//			int cxSysSbV = ::GetSystemMetrics(SM_CXVSCROLL);
	//			rect.right = rect.left + cxSysSbV;	// Set width of vert. SB to 'normal'.
	//		}
	//		else
	//		{
	//			// Get height of 'sys' horz. SB.
	//			int cySysSbH = ::GetSystemMetrics(SM_CYHSCROLL);
	//			rect.bottom = rect.top + cySysSbH;	// Set height of horz. SB to 'normal'.
	//		}
	//	}

	//	// Get current range, page and position from existing scrollbar.
	//	SCROLLINFO info;
	//	info.cbSize = sizeof(SCROLLINFO);
	//	info.fMask = SIF_ALL;
	//	::SendMessage(hWndExistingSB, SBM_GETSCROLLINFO, 0, (LPARAM)&info);

	//	// Create new scroll bar of 'this' type. Note - Control ID = 0 until old SB destroyed.
	//	if (!Create(dwStyle, rect, hParentWnd, 0))
	//	{
	//		return 0;
	//	}

	//	// Note - new scroll bar is now the last window in our parent window child Z-order.

	//	HWND hWndNewSB = GetSafeHwnd();
	//	XeASSERT(hWndNewSB);

	//	// Set Z-order of new scroll bar - insert after the existing scroll bar.
	//	::SetWindowPos(hWndNewSB, hWndExistingSB, 0, 0, 0, 0,
	//		SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

	//	// Destroy existing (old) scroll bar.
	//	::DestroyWindow(hWndExistingSB);

	//	// Set Control ID of new scroll bar.
	//	::SetWindowLong(hWndNewSB, GWL_ID, nID);

	//	// Set range, page and position parameters in scroll bar.
	//	//SetScrollInfo(&info);
	//	OnSbmSetScrollInfo(TRUE, (LPARAM)&info);
	//	return hWndNewSB;
	//}
#pragma endregion Create

	bool IsDraggingThumb() const
	{
		return m_eMouseDownArea.IsThumb();
	}

protected:
	// Called when user Shift + L button click in scrollbar channel area.
	// xyOfs = x or y offset into channel area.
	// Return true if processed else false.
	virtual bool OnLeftShiftClick(int xyOfs) { return false; }

	///////////////////////////////////////////////////////////////////////////
	// Message map functions
	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
			case SBM_ENABLE_ARROWS:		return OnSbmEnableArrows(wParam, lParam);
			case SBM_GETPOS:			return OnSbmGetPos(wParam, lParam);
			case SBM_GETRANGE:			return OnSbmGetRange(wParam, lParam);
			case SBM_GETSCROLLBARINFO:	return OnSbmGetScrollBarInfo(wParam, lParam);
			case SBM_GETSCROLLINFO:		return OnSbmGetScrollInfo(wParam, lParam);
			case SBM_SETPOS:			return OnSbmSetPos(wParam, lParam);
			case SBM_SETRANGE:			return OnSbmSetRange(wParam, lParam);
			case SBM_SETRANGEREDRAW:	return OnSbmSetRangeRedraw(wParam, lParam);
			case SBM_SETSCROLLINFO:		return OnSbmSetScrollInfo(wParam, lParam);
		}
		return CXeD2DCtrlBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender) override
	{
		OnMenuCommands(wID);
		return CXeD2DCtrlBase::_OnWmCommand(wSource, wID, sender);
	}

#pragma region MFC_Functions_Replacements
public:
	BOOL SetScrollInfo(LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE)
	{
		XeASSERT(lpScrollInfo != NULL);

		lpScrollInfo->cbSize = sizeof(*lpScrollInfo);
		::SetScrollInfo(Hwnd(), SB_CTL, lpScrollInfo, bRedraw);
		return TRUE;
	}

	BOOL GetScrollInfo(LPSCROLLINFO lpScrollInfo, UINT nMask = SIF_ALL)
	{
		XeASSERT(lpScrollInfo != NULL);

		lpScrollInfo->cbSize = sizeof(*lpScrollInfo);
		lpScrollInfo->fMask = nMask;
		return ::GetScrollInfo(Hwnd(), SB_CTL, lpScrollInfo);
	}

	int SetScrollPos(int nPos, BOOL bRedraw = TRUE)
	{
		return ::SetScrollPos(Hwnd(), SB_CTL, nPos, bRedraw);
	}
#pragma endregion MFC_Functions_Replacements

#pragma region ScrollbarMessage_SBM_XXX_handlers
protected:
	///////////////////////////////////////////////////////////////////////////
	// Scroll bar message (SBM_XXX) handlers

	// SBM_ENABLE_ARROWS message handler.
	LRESULT OnSbmEnableArrows(WPARAM wparam, LPARAM lparam)
	{
		XeASSERT(::IsWindow(Hwnd()));
		/* ImplNote - during testing the windows scrollbar behaved strangely when only one
		button was disabled. For that reason only enable/disable both is supported here. */
		EnableWindow((wparam & ESB_DISABLE_BOTH) ? FALSE : TRUE);
		// wParam Specifies whether the scroll bar arrows are enabled or disabled and 
		// indicates which arrows are enabled or disabled. 
		return TRUE;
	}

	// SBM_GETPOS message handler.
	LRESULT OnSbmGetPos(WPARAM wparam, LPARAM lparam)
	{
		XeASSERT(::IsWindow(Hwnd()));
		return m_nPos;
	}

	// SBM_GETRANGE message handler.
	LRESULT OnSbmGetRange(WPARAM wparam, LPARAM lparam)
	{
		if constexpr (sizeof(T_int) == 8)
		{
			XeASSERT(FALSE);	// not supported for 64 bit scroll bar
		}
		XeASSERT(::IsWindow(Hwnd()));
		int* lpMinPos = (int*)wparam;
		int* lpMaxPos = (int*)lparam;
		*lpMinPos = (int)m_nMinPos;
		*lpMaxPos = (int)m_nMaxPos;
		return 0;
	}

	// SBM_GETSCROLLBARINFO message handler.
	LRESULT OnSbmGetScrollBarInfo(WPARAM wparam, LPARAM lparam)
	{
		XeASSERT(::IsWindow(Hwnd()));
		SCROLLBARINFO* psbi = (SCROLLBARINFO*)lparam;
		if (!psbi || psbi->cbSize != sizeof(SCROLLBARINFO))
			return FALSE;

		/* Note - information on how to implement this is a little sparse from MS.
		Need make a few educated guesses.
		Note - testing (comparing 'this' to WIN SBs) has shown that:
				rcScrollBar is in screen coords.
				dxyLineButton is arrow button width when horz. SB.
				dxyLineButton is arrow button height when vert. SB.
		*/

		psbi->rcScrollBar = m_rectClient;				// Coordinates of the scroll bar.
		ClientToScreen(&psbi->rcScrollBar);			// In screen coords.

		if (m_bHorizontal)
		{
			psbi->dxyLineButton = m_rectTLArrow.Width();// arrow button width.
			psbi->xyThumbTop = m_rectThumb.left;		// Position of the left of the thumb. 
			psbi->xyThumbBottom = m_rectThumb.right;	// Position of the right of the thumb. 
		}
		else
		{
			psbi->dxyLineButton = m_rectTLArrow.Height();// arrow button height.
			psbi->xyThumbTop = m_rectThumb.top;			// Position of the top of the thumb. 
			psbi->xyThumbBottom = m_rectThumb.bottom;	// Position of the bottom of the thumb. 
		}

		// psbi->rgstate - An array of DWORD elements. Each element indicates the state of a 
		// scroll bar component. The following values show the scroll bar component that 
		// corresponds to each array index. Index Scroll bar component 
		// 0 The scroll bar itself. 
		// 1 The top or right arrow button. 
		// 2 The page up or page right region. 
		// 3 The scroll box (thumb). 
		// 4 The page down or page left region. 
		// 5 The bottom or left arrow button. 
		DWORD dwState = (m_bEnabled) ? 0 : STATE_SYSTEM_UNAVAILABLE;
		DWORD dwTLchSt = dwState, dwBRchSt = dwState, dwThumbSt = dwState;
		DWORD dwTLbtnSt = dwState, dwBRbtnSt = dwState;
		if (m_rectTLChannel.IsRectEmpty())
			dwTLchSt |= STATE_SYSTEM_INVISIBLE;
		if (m_rectBRChannel.IsRectEmpty())
			dwBRchSt |= STATE_SYSTEM_INVISIBLE;
		if (m_rectThumb.IsRectEmpty())
			dwThumbSt |= STATE_SYSTEM_INVISIBLE;
		if (m_eMouseDownArea.IsTLButton())
			dwTLbtnSt |= STATE_SYSTEM_PRESSED;
		if (m_eMouseDownArea.IsTLChannel())
			dwTLchSt |= STATE_SYSTEM_PRESSED;
		if (m_eMouseDownArea.IsThumb())
			dwThumbSt |= STATE_SYSTEM_PRESSED;
		if (m_eMouseDownArea.IsBRChannel())
			dwBRchSt |= STATE_SYSTEM_PRESSED;
		if (m_eMouseDownArea.IsBRButton())
			dwBRbtnSt |= STATE_SYSTEM_PRESSED;
		psbi->rgstate[0] = dwState;
		psbi->rgstate[1] = dwTLbtnSt;
		psbi->rgstate[2] = dwTLchSt;
		psbi->rgstate[3] = dwThumbSt;
		psbi->rgstate[4] = dwBRchSt;
		psbi->rgstate[5] = dwBRbtnSt;

		// The DWORD element for each scroll bar component can include a combination of the 
		// following bit flags.
		// STATE_SYSTEM_INVISIBLE - For the scroll bar itself, indicates the specified 
		//		vertical or horizontal scroll bar does not exist. For the page up or page 
		//		down regions, indicates the thumb is positioned such that the region does 
		//		not exist.
		// STATE_SYSTEM_OFFSCREEN - For the scroll bar itself, indicates the window is sized 
		//		such that the specified vertical or horizontal scroll bar is not currently 
		//		displayed. (SK note - applies to NC scroll bars only).
		// STATE_SYSTEM_PRESSED - The arrow button or page region is pressed.
		// STATE_SYSTEM_UNAVAILABLE - The component is disabled.

		return TRUE;
	}

	// SBM_GETSCROLLINFO message handler.
	LRESULT OnSbmGetScrollInfo(WPARAM wparam, LPARAM lparam)
	{
		if constexpr (sizeof(T_int) == 8)
		{
			XeASSERT(FALSE);	// Never use this message for 64bit scrollbar because windows copies the scroll info struct as SCROLLINFO (32bit)!
		}
		//XeASSERT(::IsWindow(Hwnd()));
		//LPSCROLLINFO64 lpScrollInfo = (LPSCROLLINFO64)lparam;
		//if (lpScrollInfo->cbSize != sizeof(SCROLLINFO64))
		//{
		//	XeASSERT(FALSE);	// expected SCROLLINFO64 pointer
		//	return FALSE;
		//}
		//GetScrollInfo64(lpScrollInfo);
		//return TRUE;

		XeASSERT(::IsWindow(Hwnd()));
		LPSCROLLINFO lpScrollInfo = (LPSCROLLINFO)lparam;
		if (lpScrollInfo->cbSize != sizeof(SCROLLINFO))
			return FALSE;
		lpScrollInfo->nMin = (int)m_nMinPos;
		lpScrollInfo->nMax = (int)m_nMaxPos;
		lpScrollInfo->nPage = (UINT)m_uPage;
		lpScrollInfo->nPos = (int)m_nPos;
		lpScrollInfo->nTrackPos = (int)m_nTrackPos;
		return TRUE;
	}

	// SBM_SETPOS message handler.
	LRESULT OnSbmSetPos(WPARAM wparam, LPARAM lparam)
	{
		XeASSERT(::IsWindow(Hwnd()));
		T_int nPos = (T_int)wparam;
		BOOL bRedraw = (BOOL)lparam;
		T_int nOldPos = m_nPos;

		m_nPos = nPos;

		if (m_nPos < m_nMinPos)
			m_nPos = m_nMinPos;
		else if (m_nPos > m_nMaxReportedPos)
			m_nPos = m_nMaxReportedPos;

		if (m_bNoScroll && !m_bEnabled)	// SB disabled because of SIF_DISABLENOSCROLL?
		{	// SBM_SETPOS cancels SIF_DISABLENOSCROLL.
			m_bNoScroll = FALSE;
			EnableWindow(TRUE);
		}

		RecalcRects();

		if (bRedraw)
		{
			_RedrawDirectly();
		}

		return nOldPos;
	}

	// SBM_SETRANGE message handler.
	LRESULT OnSbmSetRange(WPARAM wparam, LPARAM lparam)
	{
		XeASSERT(::IsWindow(Hwnd()));
		T_int nMinPos = (T_int)wparam;
		T_int nMaxPos = (T_int)lparam;
		m_nMinPos = nMinPos;
		m_nMaxPos = nMaxPos;
		if (m_nMaxPos < m_nMinPos)
			m_nMaxPos = m_nMinPos;
		T_int nSUrange = std::abs(m_nMaxPos - m_nMinPos) + 1;
		if (m_uPage > (T_uint)nSUrange)
			m_uPage = (T_uint)nSUrange;

		if (m_uPage == 0)
			m_nMaxReportedPos = m_nMaxPos;
		else
			m_nMaxReportedPos = m_nMaxPos - ((T_int)m_uPage - 1);

		T_int nOldPos = m_nPos;
		if (m_nPos < m_nMinPos)
			m_nPos = m_nMinPos;
		else if (m_nPos > m_nMaxReportedPos)
			m_nPos = m_nMaxReportedPos;

		if (m_bNoScroll && !m_bEnabled)	// SB disabled because of SIF_DISABLENOSCROLL?
		{	// SBM_SETRANGE cancels SIF_DISABLENOSCROLL.
			m_bNoScroll = FALSE;
			EnableWindow(TRUE);
		}

		RecalcRects();

		if (nOldPos != m_nPos)
			return nOldPos;
		return 0;
	}

	// SBM_SETRANGEREDRAW message handler.
	LRESULT OnSbmSetRangeRedraw(WPARAM wparam, LPARAM lparam)
	{
		XeASSERT(::IsWindow(Hwnd()));
		LRESULT lResult = OnSbmSetRange(wparam, lparam);
		_RedrawDirectly();
		return lResult;
	}

	// SBM_SETSCROLLINFO message handler.
	LRESULT OnSbmSetScrollInfo(WPARAM wparam, LPARAM lparam)
	{
		if constexpr (sizeof(T_int) == 8)
		{
			XeASSERT(FALSE);	// Never use this message for 64bit scrollbar because windows copies the scroll info struct as SCROLLINFO (32bit)!
		}
		//XeASSERT(::IsWindow(Hwnd()));
		//BOOL bRedraw = (BOOL)wparam;
		//LPSCROLLINFO64 lpScrollInfo = (LPSCROLLINFO64)lparam;
		//if (lpScrollInfo->cbSize != sizeof(SCROLLINFO64))
		//{
		//	XeASSERT(FALSE);	// expected SCROLLINFO64 pointer
		//	return 0;
		//}
		//return SetScrollInfo64(lpScrollInfo, bRedraw);

		XeASSERT(::IsWindow(Hwnd()));
		BOOL bRedraw = (BOOL)wparam;
		LPSCROLLINFO lpScrollInfo = (LPSCROLLINFO)lparam;
		if (lpScrollInfo->cbSize != sizeof(SCROLLINFO))
			return 0;
		if (lpScrollInfo->fMask & SIF_PAGE)
		{
			m_uPage = lpScrollInfo->nPage;
			// Note - windows scrollbars can have a page size = 0.
		}
		if (lpScrollInfo->fMask & SIF_RANGE)
		{
			m_nMinPos = lpScrollInfo->nMin;
			m_nMaxPos = lpScrollInfo->nMax;
		}
		if (lpScrollInfo->fMask & SIF_POS)
		{
			m_nPos = lpScrollInfo->nPos;
		}
		if (lpScrollInfo->fMask & SIF_DISABLENOSCROLL)
		{
			BOOL bEnable = ((int)m_uPage < (m_nMaxPos - m_nMinPos)) ? TRUE : FALSE;
			m_bNoScroll = !bEnable;
			EnableWindow(bEnable);
		}

		if (m_nMaxPos < m_nMinPos)
			m_nMaxPos = m_nMinPos;
		T_int nSUrange = std::abs(m_nMaxPos - m_nMinPos) + 1;
		if (m_uPage > (T_uint)nSUrange)
			m_uPage = (T_uint)nSUrange;

		if (m_uPage == 0)
			m_nMaxReportedPos = m_nMaxPos;
		else
			m_nMaxReportedPos = m_nMaxPos - ((T_int)m_uPage - 1);

		if (m_nPos < m_nMinPos)
			m_nPos = m_nMinPos;
		else if (m_nPos > m_nMaxReportedPos)
			m_nPos = m_nMaxReportedPos;

		RecalcRects();

		if (bRedraw)
		{
			_RedrawDirectly();
		}

		return m_nPos;
	}
#pragma endregion ScrollbarMessage_SBM_XXX_handlers

#pragma region Paint
protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		XeASSERT(::IsWindow(Hwnd()));

		XeScrollBar_PaintInfo ps(pRT, rc, m_bHorizontal, m_bDrawGripper);
		ps.m_areaInfo.push_back(GetUIelementDrawState(XSB_EAREA::eTLbutton));
		ps.m_areaInfo.push_back(GetUIelementDrawState(XSB_EAREA::eTLchannel));
		ps.m_areaInfo.push_back(GetUIelementDrawState(XSB_EAREA::eThumb));
		ps.m_areaInfo.push_back(GetUIelementDrawState(XSB_EAREA::eBRchannel));
		ps.m_areaInfo.push_back(GetUIelementDrawState(XSB_EAREA::eBRbutton));

		_DrawScrollBarBackLayer(ps);
		_DrawScrollBarMiddleLayer(ps);
		_DrawScrollBarThumbLayer(ps);
		_DrawScrollBarTopLayer(ps);
	}

	// This is the only member a derived class needs to override.
	// Note - derived class should NOT call DrawScrollBar in this class.
	virtual void _DrawScrollBarBackLayer(XeScrollBar_PaintInfo& ps)
	{
		// Draw scrollbar as solid color rects - in debug mode ONLY!
		XeASSERT(FALSE);
#ifdef _DEBUG
#define X_GRBTN RGB(140,140,140)
#define X_GRCH  RGB(180,180,180)
#define X_GRTHM RGB(96,96,96)
#define X_NRMBT	RGB(128,0,128)
#define X_NRMCH RGB(128,128,0)
#define X_NRMTH RGB(0,192,0)
#define X_HOTBT	RGB(192,0,192)
#define X_HOTCH RGB(192,192,0)
#define X_HOTTH RGB(0,255,0)
#define X_DWNBT (~X_NRMBT & 0xFFFFFF)
#define X_DWNCH (~X_NRMCH & 0xFFFFFF)
#define X_DWNTH (~X_NRMTH & 0xFFFFFF)
		COLORREF rgbarr[5][6] = {
			//	eNone	eTLbutton	eBRbutton	eTLchannel	eBRchannel	eThumb
			//	  0		1			2			3			4			5
				{ 0,	0,			0,			0,			0,			0 },		// 0 eNotDrawn
				{ 0,	X_GRBTN,	X_GRBTN,	X_GRCH,		X_GRCH,		X_GRTHM },	// 1 eDisabled
				{ 0,	X_NRMBT,	X_NRMBT,	X_NRMCH,	X_NRMCH,	X_NRMTH },	// 2 eNormal
				{ 0,	X_DWNBT,	X_DWNBT,	X_DWNCH,	X_DWNCH,	X_DWNTH },	// 3 eDown
				{ 0,	X_HOTBT,	X_HOTBT,	X_HOTCH,	X_HOTCH,	X_HOTTH }	// 4 eHot
		};
		XSB_EDRAWELEM eState;
		const CRect* prcElem = 0;
		for (int nElem = (int)XSB_EAREA::eTLbutton;
			nElem <= (int)XSB_EAREA::eThumb; nElem++)
		{
			prcElem = GetUIelementDrawState((XSB_EAREA)nElem, eState);
			if (!prcElem || eState == XSB_EDRAWELEM::eNotDrawn)
				continue;
			COLORREF rgb = rgbarr[(int)eState][nElem];
			// TODO - to fix thi - create the brushed needed (CreateSolidColorBrush)
			D2D1_RECT_F rcArea = RectFfromRect(*prcElem);
			//ps.m_pRT->FillRectangle(rcArea, xxx);
		}
#endif
	}

	virtual void _DrawScrollBarMiddleLayer(XeScrollBar_PaintInfo& ps) {}
	virtual void _DrawScrollBarThumbLayer(XeScrollBar_PaintInfo& ps) {}
	virtual void _DrawScrollBarTopLayer(XeScrollBar_PaintInfo& ps) {}

	// Draw scroll bar (used by derived classes).
	// isDrawBackLayer = true to draw backmost layer (buttons, channel), = false to draw topmost layer (thumb).
	void _DrawScrollBar(XeScrollBar_PaintInfo& ps, bool isDrawBackLayer)
	{
		//XeASSERT(ps.m_xeUI);
		const CRect* prcElem = 0;
		for (const XeScrollBar_AreaInfo& ai : ps.m_areaInfo)
		{
			// Get bounding rect of area to draw (client coords.)
			prcElem = ai.m_prcArea;
			if (ai.m_eState == XSB_EDRAWELEM::eNotDrawn)	// Rect empty or area not drawn?
				continue;

			CID cidBg = CID::CtrlBg, cidArwFg, cidThm;
			switch (ai.m_eState)
			{
			case XSB_EDRAWELEM::eDisabled:
				cidArwFg = CID::CtrlTxtDis;
				cidThm = CID::CtrlTxtDis;
				break;
			case XSB_EDRAWELEM::eNormal:
				cidArwFg = CID::CtrlBgDis;
				cidThm = CID::CtrlBgDis;
				break;
			case XSB_EDRAWELEM::eDown:
				cidArwFg = CID::CtrlCurItemBg;
				cidThm = CID::CtrlTxtDis;
				break;
			case XSB_EDRAWELEM::eHot:
				cidArwFg = CID::CtrlCurItemBg;
				cidThm = CID::CtrlTxtDis;
				break;
			default:
				XeASSERT(FALSE);	// Unknown state!
			}

			// Draw scrollbar element (area)
			if (ai.m_area.IsButton())
			{
				if (isDrawBackLayer)
				{
					D2D1_RECT_F rcBtn = RectFfromRect(*prcElem);
					ps.m_pRT->FillRectangle(rcBtn, GetBrush(cidBg));
					BtnTp tp = ai.m_area.eArea == XSB_EAREA::eTLbutton
						? (ps.m_bHorizontal ? BtnTp::left : BtnTp::up)
						: (ps.m_bHorizontal ? BtnTp::right : BtnTp::down);
					_DrawScrollbarButton(ps.m_pRT, rcBtn, cidArwFg, tp);
				}
			}
			else if (ai.m_area.IsChannel())
			{
				if (isDrawBackLayer)
				{
					D2D1_RECT_F rcChannel = RectFfromRect(*prcElem);
					ps.m_pRT->FillRectangle(rcChannel, GetBrush(cidBg));
				}
			}
			else	// Is thumb
			{
				D2D1_RECT_F rcThumb = RectFfromRect(*prcElem);
				if (isDrawBackLayer)
				{
					ps.m_pRT->FillRectangle(rcThumb, GetBrush(cidBg));
				}
				else
				{
					if (ps.m_bHorizontal)
					{
						rcThumb.right -= ((HeightOf(rcThumb) - 9.0f) / 2.0f);
					}
					else	// Vertical thumb
					{
						rcThumb.bottom -= ((WidthOf(rcThumb) - 9.0f) / 2.0f);
					}
					ps.m_pRT->FillRectangle(rcThumb, GetBrush(cidThm));
				}
			}
		}
	}

	enum class BtnTp { left, right, up, down };
	void _DrawScrollbarButton(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rcBtn, CID fgColor, BtnTp tp)
	{
		bool isHorz = tp == BtnTp::left || tp == BtnTp::right;
		float cxTri = isHorz ? 5.0f : 9.0f, cyTri = isHorz ? 9.0f : 5.0f;
		float cyTriMargin = (HeightOf(rcBtn) - cyTri) / 2.0f;
		float cxTriMargin = (WidthOf(rcBtn) - cxTri) / 2.0f;
		float x1 = rcBtn.left + cxTriMargin, x2 = x1 + cxTri;
		float y1 = HeightOf(rcBtn) / 2.0f, y2 = rcBtn.top + cyTriMargin;
		float x3 = x2, y3 = y2 + cyTri;
		if (tp == BtnTp::right)
		{
			std::swap(x1, x2);
			x3 = x2;
		}
		else if (!isHorz)
		{
			x1 = (rcBtn.left + WidthOf(rcBtn)) / 2.0f;
			x2 = rcBtn.left + cxTriMargin;
			y1 = rcBtn.top + cyTriMargin, y2 = y1 + cyTri;
			x3 = x2 + cxTri, y3 = y2;
			if (tp == BtnTp::down)
			{
				std::swap(y1, y2);
				y3 = y2;
			}
		}

		Microsoft::WRL::ComPtr<ID2D1PathGeometry> tri = _MakeTriangle(D2D1_POINT_2F(x1, y1), D2D1_POINT_2F(x2, y2), D2D1_POINT_2F(x3, y3));
		pRT->DrawGeometry(tri.Get(), GetBrush(fgColor), 1.0f);
		pRT->FillGeometry(tri.Get(), GetBrush(fgColor));
	}
#pragma endregion Paint

#pragma region KeyboardMessages
	///////////////////////////////////////////////////////////////////////////
	// Keyboard messages

	// WM_KEYDOWN message handler.
	virtual LRESULT _OnKeyDown(WPARAM wParam, LPARAM lParam) override
	{	// WM_KEYDOWN message handler.
		XeASSERT(::IsWindow(Hwnd()));
		WORD wSBcode = 0xFFFF;
		// Note - SB_PAGELEFT == SB_PAGEUP etc. see winuser.h
		switch (wParam)
		{
		case VK_PRIOR:
			wSBcode = SB_PAGELEFT;
			break;

		case VK_NEXT:
			wSBcode = SB_PAGERIGHT;
			break;

		case VK_HOME:
			wSBcode = SB_LEFT;
			break;

		case VK_END:
			wSBcode = SB_RIGHT;
			break;

		case VK_LEFT:
			wSBcode = SB_LINELEFT;
			break;

		case VK_RIGHT:
			wSBcode = SB_LINERIGHT;
			break;

		case VK_UP:
			wSBcode = SB_LINEUP;
			break;

		case VK_DOWN:
			wSBcode = SB_LINEDOWN;
			break;
		}

		if (wSBcode != 0xFFFF)
		{
			SendScrollMsg(wSBcode);
			m_bNeedEndScroll = TRUE;	// Send SB_ENDSCROLL on WM_KEYUP.
		}

		return 0;	// Indicate we processed this message (eat all key msgs).
		// Note - testing shows that windows looks for another child control to process
		// keyboard input if we don't return 0 here (and we lose focus).
	}

	// WM_KEYUP message handler.
	virtual LRESULT _OnKeyUp(WPARAM wParam, LPARAM lParam) override
	{	// WM_KEYUP message handler.
		if (m_bNeedEndScroll)
		{
			SendScrollMsg(SB_ENDSCROLL);
			m_bNeedEndScroll = FALSE;
		}
		return 0;	// Indicate we processed this message (eat all key msgs).
	}
#pragma endregion KeyboardMessages

#pragma region MouseMessages
	///////////////////////////////////////////////////////////////////////////
	// Mouse messages

	// WM_LBUTTONDOWN message handler.
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint point) override
	{	// WM_LBUTTONDOWN message handler.
		XeASSERT(::IsWindow(Hwnd()));
		BOOL bHasTabStop = (::GetWindowLong(Hwnd(), GWL_STYLE) & WS_TABSTOP) ? TRUE : FALSE;
		if (!m_isThisScrollBarInComboBoxDropDown)
		{
			SetCapture();
			if (bHasTabStop)
				SetFocus();		// Only 'steal' focus if 'this' has Tab stop.
		}

		m_eMouseDownArea = GetAreaFromPoint(point);

		WORD wSBcode = 0xFFFF;
		// Note - SB_PAGELEFT == SB_PAGEUP etc. see winuser.h
		if (m_eMouseDownArea.eArea == XSB_EAREA::eTLbutton)
			wSBcode = SB_LINELEFT;
		else if (m_eMouseDownArea.eArea == XSB_EAREA::eTLchannel)
			wSBcode = SB_PAGELEFT;
		else if (m_eMouseDownArea.eArea == XSB_EAREA::eBRchannel)
			wSBcode = SB_PAGERIGHT;
		else if (m_eMouseDownArea.eArea == XSB_EAREA::eBRbutton)
			wSBcode = SB_LINERIGHT;

		if ((m_eMouseDownArea.IsChannel() || m_eMouseDownArea.IsThumb()) && nFlags & MK_SHIFT)
		{
			int xyOfs = m_bHorizontal ? point.x - m_rectTLArrow.right : point.y - m_rectTLArrow.bottom;
			if (!OnLeftShiftClick(xyOfs))
			{
				ScrollHere(xyOfs);
			}
		}
		else
		{
			if (wSBcode != 0xFFFF)
			{
				SendScrollMsg(wSBcode);
				m_bNeedEndScroll = TRUE;	// Send SB_ENDSCROLL on WM_LBUTTONUP message.
			}

			if (m_eMouseDownArea.IsThumb())	// Store X or Y offset from thumb edge.
				m_xyThumbDragOffset = (m_bHorizontal) ? point.x - m_rectThumb.left
				: point.y - m_rectThumb.top;

			if (!m_isThisScrollBarInComboBoxDropDown)
			{
				// Set timer for first auto repeat - when button or channel clicked.
				if (m_eMouseDownArea.IsButton() || m_eMouseDownArea.IsChannel())
					SetTimer(XSB_LBTN_DOWN_TIMERID, XSB_LBTN_DOWN_TIME, 0);
			}
		}
		return 0;
	}

	// WM_LBUTTONUP message handler.
	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint point) override
	{	// WM_LBUTTONUP message handler.
		XeASSERT(::IsWindow(Hwnd()));
		if (!m_isThisScrollBarInComboBoxDropDown)
		{
			ReleaseCapture();
			KillTimer(XSB_LBTN_DOWN_TIMERID);
			KillTimer(XSB_LBTN_REPT_TIMERID);
		}

		m_eMouseDownArea = XSB_EAREA::eNone;

		if (m_bDragging)					// Did we send any SB_THUMBTRACK messages?
		{
			SendScrollMsg(SB_THUMBPOSITION, (WORD)m_nTrackPos);
			m_bDragging = FALSE;
			RecalcRects();	// Reset thumb pos. to current scroll pos.
		}

		if (m_bNeedEndScroll)
		{
			SendScrollMsg(SB_ENDSCROLL);	// Let parent know scrolling has ended.
			m_bNeedEndScroll = FALSE;
		}

		_RedrawDirectly();

		return 0;
	}

	// WM_MOUSEWHEEL message handler.
	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint pt) override
	{	// WM_MOUSEWHEEL message handler.
		XeASSERT(::IsWindow(Hwnd()));
		short xPos = (short)pt.x;
		short yPos = (short)pt.y;

		if (!m_bHorizontal)	// Mouse wheel messages only apply to vertical scrollbar.
		{
			WORD wSBcode = 0xFFFF;
			if (zDelta >= WHEEL_DELTA)
			{
				wSBcode = SB_LINEUP;
			}
			else if (zDelta <= -WHEEL_DELTA)
			{
				wSBcode = SB_LINEDOWN;
				zDelta = -zDelta;
			}
			if (wSBcode != 0xFFFF)
			{
				do
				{
					SendScrollMsg(wSBcode);
				} while ((zDelta -= WHEEL_DELTA) >= WHEEL_DELTA);
				SendScrollMsg(SB_ENDSCROLL);
			}
			return 0;	// Message was processed.
		}
		return 1;	// Message not processed.
	}

	// WM_MOUSEMOVE message handler.
	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{	// WM_MOUSEMOVE message handler.
		XeASSERT(::IsWindow(Hwnd()));
		CXeD2DCtrlBase::_OnMouseMove(nFlags, point);

		XSB_AREA eOldMouseArea = m_eMouseOverArea;

		m_eMouseOverArea = GetAreaFromPoint(point);	// Update mouse over area.

		if (m_eMouseDownArea.IsThumb() && m_dblPx_SU > 0)
		{	// User is dragging the thumb.
			BOOL bStartDrag = FALSE;
			if (!m_bDragging)
			{
				bStartDrag = TRUE;	// Start of thumb dragging.
				m_bDragging = TRUE;
			}
			T_int nTrackPos;
			double dblScrollPos;
			if (m_bHorizontal)
			{	// X pos of left edge of thumb (0...?)
				int xPos = point.x - m_xyThumbDragOffset - m_rectTLArrow.right;
				dblScrollPos = (double)xPos / m_dblPx_SU;
				nTrackPos = round_ud_dbl(dblScrollPos) + m_nMinPos;
			}
			else
			{	// Y pos of top edge of thumb (0...?)
				int yPos = point.y - m_xyThumbDragOffset - m_rectTLArrow.bottom;
				dblScrollPos = (double)yPos / m_dblPx_SU;
				nTrackPos = round_ud_dbl(dblScrollPos) + m_nMinPos;
			}
			if (nTrackPos < m_nMinPos)
				nTrackPos = m_nMinPos;
			else if (nTrackPos > m_nMaxReportedPos)
				nTrackPos = m_nMaxReportedPos;
			if (bStartDrag || m_nTrackPos != nTrackPos)
			{	// Send scroll message when user starts dragging
				// OR when track pos has changed.
				m_nTrackPos = nTrackPos;
				SendScrollMsg(SB_THUMBTRACK, (WORD)m_nTrackPos);
				m_bNeedEndScroll = TRUE;
			}
			// Recalculate thumb XY pos. and redraw if pos. changed.
			if (RecalcRectsThumbTrack(point))
			{
				_RedrawDirectly();
			}
		}

		if (m_eMouseOverArea != eOldMouseArea)
		{
			_RedrawDirectly();
		}

		return 0;
	}

	// WM_MOUSELEAVE message handler.
	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override
	{	// WM_MOUSELEAVE message handler.
		XeASSERT(::IsWindow(Hwnd()));
		m_eMouseOverArea = XSB_EAREA::eNone;
		_RedrawDirectly();
		return CXeD2DCtrlBase::_OnMouseLeave(wparam, lparam);
	}
#pragma endregion MouseMessages

#pragma region OtherMessages
	///////////////////////////////////////////////////////////////////////////
	// Other messages

	// WM_GETDLGCODE message handler.
	virtual LRESULT _OnGetDlgCode(WPARAM wParam, LPARAM lParam) override
	{	// WM_GETDLGCODE message handler.
		XeASSERT(::IsWindow(Hwnd()));
		BOOL bHasTabStop = (::GetWindowLong(Hwnd(), GWL_STYLE) & WS_TABSTOP) ? TRUE : FALSE;

		//LRESULT lResult = Default();
		LRESULT lResult = ::DefWindowProcW(Hwnd(), WM_GETDLGCODE, wParam, lParam);
		if (lParam)	// lParam points to an MSG structure?
		{
			LPMSG lpmsg = (LPMSG)lParam;
			if ((lpmsg->message == WM_KEYDOWN	// Keyboard input?
				|| lpmsg->message == WM_KEYUP)
				&& lpmsg->wParam != VK_TAB)	// AND NOT TAB key?
			{
				if (bHasTabStop)				// 'this' window has Tab stop?
				{
					lResult |= DLGC_WANTMESSAGE;	// We want keyboard input (except TAB).
					// Note - windows will set focus to 'this' (and send WM_SETFOCUS)
					//        if we return DLGC_WANTMESSAGE here.
				}
				else
				{	// 'this' windows does NOT have TAB stop.
					// Note - windows standard scroll bar implements a special behaviour
					// for scroll bars when no tab stop for the UP, DOWN, LEFT, RIGHT keys.
					if (m_bHorizontal)
					{
						if (lpmsg->wParam == VK_LEFT || lpmsg->wParam == VK_RIGHT)
							lResult |= DLGC_WANTMESSAGE;
					}
					else
					{
						if (lpmsg->wParam == VK_UP || lpmsg->wParam == VK_DOWN)
							lResult |= DLGC_WANTMESSAGE;
					}
				}
			}
		}
		else
		{
			if (bHasTabStop)
				lResult |= DLGC_WANTTAB;	// 'this' has WS_TABSTOP style - we want focus.
			else
				lResult |= DLGC_STATIC;		// no tab stop - we don't want focus.
		}
		return lResult;
	}

	// WM_CANCELMODE message handler.
	//afx_msg void OnCancelMode()
	virtual LRESULT _OnCancelMode() override
	{	// WM_CANCELMODE message handler.
		XeASSERT(::IsWindow(Hwnd()));
		//CScrollBar::OnCancelMode();

		// Need to handle WM_CANCELMODE message.
		// See -> http://support.microsoft.com/kb/74548/en-us

		if (!m_eMouseDownArea.IsNone())	// Mouse L button down?
			_OnLeftUp(0, CPoint(0, 0));	// Do L btn up processing now.

		return CXeD2DCtrlBase::_OnCancelMode();
	}

	// WM_CONTEXTMENU message handler.
	//afx_msg void OnContextMenu(CWnd* pWnd, CPoint point)
	virtual LRESULT _OnContextMenu(WPARAM wParam, LPARAM lParam) override
	{	// WM_CONTEXTMENU message handler.
		XeASSERT(::IsWindow(Hwnd()));
		CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		m_ptMenu = point;
		ScreenToClient(&m_ptMenu);

		m_xeUI->ShowScrollbarMenu(Hwnd(), m_bHorizontal, point);
		return 0;
	}

	// ON_COMMAND_RANGE message map handler - for menu commands.
	void OnMenuCommands(UINT uID)
	{
		XeASSERT(::IsWindow(Hwnd()));
		int xyOfs;
		WORD wSBcode = 0xFFFF;
		switch (uID)
		{
		case XSB_IDM_SCRHERE:
			if (m_bHorizontal)
				xyOfs = m_ptMenu.x - m_rectTLArrow.right;
			else
				xyOfs = m_ptMenu.y - m_rectTLArrow.bottom;
			ScrollHere(xyOfs);
			break;
		case XSB_IDM_SCR_TL:
			wSBcode = SB_TOP;
			break;
		case XSB_IDM_SCR_BR:
			wSBcode = SB_BOTTOM;
			break;
		case XSB_IDM_SCR_PG_UL:
			wSBcode = SB_PAGEUP;
			break;
		case XSB_IDM_SCR_PG_DR:
			wSBcode = SB_PAGEDOWN;
			break;
		case XSB_IDM_SCR_UL:
			wSBcode = SB_LINEUP;
			break;
		case XSB_IDM_SCR_DR:
			wSBcode = SB_LINEDOWN;
			break;
		}
		if (wSBcode != 0xFFFF)
		{
			SendScrollMsg(wSBcode);
			SendScrollMsg(SB_ENDSCROLL);
		}
	}

	// WM_TIMER message handler.
	virtual LRESULT _OnTimer(WPARAM wParam, LPARAM lParam) override
	{	// WM_TIMER message handler.
		XeASSERT(::IsWindow(Hwnd()));
		UINT_PTR nIDEvent = (UINT_PTR)wParam;
		if (nIDEvent == XSB_LBTN_DOWN_TIMERID)
		{	// First auto repeat timer event.
			KillTimer(XSB_LBTN_DOWN_TIMERID);
			SetTimer(XSB_LBTN_REPT_TIMERID, XSB_LBTN_REPT_TIME, 0);
		}
		if (nIDEvent == XSB_LBTN_DOWN_TIMERID || nIDEvent == XSB_LBTN_REPT_TIMERID)
		{	// Auto repeat
			CPoint ptCurMouse;
			if (::GetCursorPos(&ptCurMouse))
			{
				::ScreenToClient(GetSafeHwnd(), &ptCurMouse);
				m_eMouseOverArea = GetAreaFromPoint(ptCurMouse);	// Update mouse over area.
			}
			if (m_eMouseDownArea.IsTLButton())
			{
				if (m_eMouseOverArea.IsTLButton())	// Mouse still over button?
					SendScrollMsg(SB_LINELEFT);
			}
			else if (m_eMouseDownArea.IsBRButton())
			{
				if (m_eMouseOverArea.IsBRButton())	// Mouse still over button?
					SendScrollMsg(SB_LINERIGHT);
			}
			else if (m_eMouseDownArea.IsTLChannel())
			{
				if (m_eMouseOverArea.IsTLChannel())	// Mouse still over channel?
					SendScrollMsg(SB_PAGELEFT);
			}
			else if (m_eMouseDownArea.IsBRChannel())
			{
				if (m_eMouseOverArea.IsBRChannel())	// Mouse still over channel?
					SendScrollMsg(SB_PAGERIGHT);
			}
			// Note - SB_LINELEFT == SB_LINEUP etc. see winuser.h
		}
		if (nIDEvent == XSB_FOCUS_TIMERID)
		{	// Blinking focus timer.
			if (m_bNeedEndScroll)
			{	// Draw normal thumb box while user is scrolling.
				if (!m_bDrawGripper)
				{	// Redraw scroll bar if currently drawn without 'gripper'.
					m_bDrawGripper = TRUE;
					_RedrawDirectly();// Draw 'blinking' focus.
				}
			}
			else
			{	// Draw blinking 'gripper' to indicate 'focus'.
				m_bDrawGripper = !m_bDrawGripper;
				_RedrawDirectly();	// Draw 'blinking' focus.
			}
		}
		return CXeD2DCtrlBase::_OnTimer(wParam, lParam);
	}

	// WM_SIZE message handler.
	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{	// WM_SIZE message handler.
		XeASSERT(::IsWindow(Hwnd()));

		if (Hwnd())
			RecalcRects();

		return CXeD2DCtrlBase::_OnSize(hWnd, wParam, lParam);
	}

	// WM_SETFOCUS message handler.
	virtual LRESULT _OnSetFocus(HWND hOldWnd) override
	{	// WM_SETFOCUS message handler.
		XeASSERT(::IsWindow(Hwnd()));
		m_bHasFocus = TRUE;
		m_bDrawGripper = FALSE;
		SetTimer(XSB_FOCUS_TIMERID, XSB_FOCUS_TIME, 0);
		_RedrawDirectly();
		return 0;
	}

	// WM_KILLFOCUS message handler.
	virtual LRESULT _OnKillFocus(HWND hNewWnd) override
	{	// WM_KILLFOCUS message handler.
		XeASSERT(::IsWindow(Hwnd()));
		m_bHasFocus = FALSE;
		m_bDrawGripper = TRUE;
		KillTimer(XSB_FOCUS_TIMERID);
		_RedrawDirectly();
		return 0;
	}

	// WM_ENABLE message handler.
	virtual LRESULT _OnEnableWindow(bool isEnabled) override
	{	// WM_ENABLE message handler
		XeASSERT(::IsWindow(Hwnd()));

		m_bEnabled = isEnabled;

		RecalcRects();	// Need to recalc. because no thumb is shown when disabled.

		_RedrawDirectly();
		return 0;
	}
#pragma endregion OtherMessages

#pragma region Helpers
	///////////////////////////////////////////////////////////////////////////
	// Helpers

	// Send WM_HSCROLL or WM_VSCROLL message to parent window.
	// [IN]  wSBcode = SB_XXX message (LOWORD of WPARAM).
	// [IN]  wHiWPARAM = Scroll pos when SB_THUMBTRACK or SB_THUMBPOSITION.
	void SendScrollMsg(WORD wSBcode, WORD wHiWPARAM = 0)
	{
		XeASSERT(::IsWindow(Hwnd()));
		if (m_hParent && ::IsWindow(m_hParent))
		{
			::SendMessage(m_hParent, (m_bHorizontal) ? WM_HSCROLL : WM_VSCROLL,
				MAKELONG(wSBcode, wHiWPARAM), (LPARAM)Hwnd());
		}
	}

	// Get UI area (element) from point.
	// Returns UI area enum if point is within a UI element else eNone.
	XSB_EAREA GetAreaFromPoint(CPoint point)
	{
		XeASSERT(::IsWindow(Hwnd()));
		if (!m_rectClient.PtInRect(point))
			return XSB_EAREA::eNone;
		if (m_rectThumb.PtInRect(point))
			return XSB_EAREA::eThumb;
		if (m_rectTLArrow.PtInRect(point))
			return XSB_EAREA::eTLbutton;
		if (m_rectBRArrow.PtInRect(point))
			return XSB_EAREA::eBRbutton;
		if (m_rectTLChannel.PtInRect(point))
			return XSB_EAREA::eTLchannel;
		if (m_rectBRChannel.PtInRect(point))
			return XSB_EAREA::eBRchannel;
		return XSB_EAREA::eNone;
	}

	// Get 'state' of UI element.
	// [IN]  eElem = UI area (element).
	// [OUT] eState = enumerated 'state' of requested area.
	// Returns pointer to CRect of UI element or NULL if eElem invalid enum.
	const CRect* GetUIelementDrawState(XSB_EAREA eElem, XSB_EDRAWELEM& eState)
	{
		XeASSERT(::IsWindow(Hwnd()));
		CRect* prcElem = 0;
		eState = XSB_EDRAWELEM::eNotDrawn;
		switch (eElem)
		{
		case XSB_EAREA::eTLbutton:
			prcElem = &m_rectTLArrow;
			break;
		case XSB_EAREA::eBRbutton:
			prcElem = &m_rectBRArrow;
			break;
		case XSB_EAREA::eTLchannel:
			prcElem = &m_rectTLChannel;
			break;
		case XSB_EAREA::eBRchannel:
			prcElem = &m_rectBRChannel;
			break;
		case XSB_EAREA::eThumb:
			prcElem = &m_rectThumb;
			break;
		}
		if (!prcElem || prcElem->IsRectEmpty())
			eState = XSB_EDRAWELEM::eNotDrawn;
		if (!m_bEnabled)
			eState = XSB_EDRAWELEM::eDisabled;
		else if (m_eMouseDownArea.eArea == eElem)
			eState = XSB_EDRAWELEM::eDown;
		else if (m_eMouseOverArea.eArea == eElem)
			eState = XSB_EDRAWELEM::eHot;
		else
			eState = XSB_EDRAWELEM::eNormal;
		return prcElem;
	}

	XeScrollBar_AreaInfo GetUIelementDrawState(XSB_EAREA eElem)
	{
		XeScrollBar_AreaInfo ai(eElem);
		ai.m_prcArea = GetUIelementDrawState(eElem, ai.m_eState);
		return ai;
	}

	// Calculate thumb size and position - used by RecalcRects().
	// [IN]  cxyChannel = channel width or height (excluding arrow buttons).
	// [OUT] xyThumb = Thumb x or y pos. (0 = first pixel in channel).
	// [OUT] cxyThumb = Thumb width or height.
	// Returns TRUE if channel W/H big enough for a thumb.
	// Returns FALSE if no space for thumb (xyThumb and cxyThumb unchanged).
	BOOL CalcThumb(int cxyChannel, int& xyThumb, int& cxyThumb)
	{
		// Range in 'scroll units' (SU) - Note +1 because min/max are 'inclusive' values.
		T_int nSU_Range = std::abs(m_nMaxPos - m_nMinPos) + 1;
		if (nSU_Range == 0							// No thumb when scroll range is 0
			|| cxyChannel <= (int)m_uThumbMinHW		// No space for thumb.
			|| !m_bEnabled)						// No thumb when disabled.
			return FALSE;

		// We have space for thumb.

		// thumb size = scroll bar size * page size      / scroll bar range 
		// (pixels)     (pixels)          (scroll units)   (scroll units)

		// When page size is 0 thumb size is set to minimum size.

		m_dblPx_SU = (double)cxyChannel / (double)nSU_Range;	// Pixels per scroll unit.

		double dblXY = (double)(m_nPos - m_nMinPos) * m_dblPx_SU;
		xyThumb = (int)dblXY;
		if (fabs_dbl(dblXY - (double)xyThumb) > 0.5)
			xyThumb++;

		double dblCXY = (double)m_uPage * m_dblPx_SU;
		cxyThumb = (int)dblCXY;
		if (fabs_dbl(dblCXY - (double)cxyThumb) > 0.5)
			cxyThumb++;

		//if( m_uPage == 0 )
		//	cxyThumb = GetCXYarrow();	// Thumb is same as arrow button when page = 0.
		// Note - WIN SBs show thumb box same size as arrow button when PAGE = 0.

		if (cxyThumb < (int)m_uThumbMinHW)
		{
			int nErrCXY = (int)m_uThumbMinHW - cxyThumb;
			cxyThumb = (int)m_uThumbMinHW;

			// Calculate new thumb X or Y position when 'error' in position.
			double dblErr_Px = (double)nErrCXY / (double)cxyChannel;
			double dblXYoffs = dblErr_Px * xyThumb;
			int xyOffs = (int)dblXYoffs;
			if (fabs_dbl(dblXYoffs - (double)xyOffs) > 0.5)
				xyOffs++;
			xyThumb -= xyOffs;
		}

		// Sometimes it's needed to adjust the size and or position because scroll bar
		// parameters are in error. 

		// Calculate last possible X or Y for thumb.
		int xyLastPossible = cxyChannel - cxyThumb;
		if (xyThumb > xyLastPossible)
			xyThumb = xyLastPossible;

		if (xyThumb < 0)
			xyThumb = 0;

		if ((xyThumb + cxyThumb) > cxyChannel)
			cxyThumb = cxyChannel - xyThumb;

		return TRUE;
	}

	// Recalculate UI elements size and positions.
	// Note - thumb XY position is calculated from current scroll pos (m_nPos).
	// Called from Create(...), SetScrollPos(...), SetScrollRange(...), 
	//             SetScrollInfo(...), OnSize(...).
	void RecalcRects()
	{
		if (!GetSafeHwnd())
			return;

		GetClientRect(&m_rectClient);	// Update client rect.

		BOOL bHasThumb = FALSE;
		m_rectThumb.SetRectEmpty();

		if (m_bHorizontal)
		{	// Calc. arrows
			int cxClient = m_rectClient.Width();
			int cxArrow = GetCXYarrow();	// Arrow button width.

			if (cxClient < (2 * cxArrow))
				cxArrow = cxClient / 2;	// Limit arrow width to available area.

			m_rectTLArrow.SetRect(m_rectClient.left, m_rectClient.top,
				m_rectClient.left + cxArrow, m_rectClient.bottom);

			m_rectBRArrow.SetRect(m_rectClient.right - cxArrow, m_rectClient.top,
				m_rectClient.right, m_rectClient.bottom);

			// Calc. thumb size and position
			int xThumb, cxThumb, cxChannel = cxClient - (2 * cxArrow);
			bHasThumb = CalcThumb(cxChannel, xThumb, cxThumb);
			if (bHasThumb)
			{	// We have space for thumb.
				xThumb += m_rectTLArrow.right;
				m_rectThumb = m_rectTLArrow;
				m_rectThumb.left = xThumb;
				m_rectThumb.right = xThumb + cxThumb;
			}

			// Calc. channels
			m_rectTLChannel = m_rectTLArrow;
			m_rectBRChannel = m_rectBRArrow;
			m_rectTLChannel.left = m_rectTLArrow.right;
			if (bHasThumb)
			{
				m_rectTLChannel.right = m_rectThumb.left;
				m_rectBRChannel.left = m_rectThumb.right;
				m_rectBRChannel.right = m_rectBRArrow.left;
			}
			else
			{
				m_rectTLChannel.right = m_rectBRArrow.left;	// L channel reaches to R arrow.
				m_rectBRChannel.SetRectEmpty();				// No thumb - so R channel not needed.
			}
		}
		else // Vertical scroll bar.
		{	// Calc. arrows
			int cyClient = m_rectClient.Height();
			int cyArrow = GetCXYarrow();	// Arrow button height.

			if (cyClient < (2 * cyArrow))
				cyArrow = cyClient / 2;	// Limit arrow height to available area.

			m_rectTLArrow.SetRect(m_rectClient.left, m_rectClient.top,
				m_rectClient.right, m_rectClient.top + cyArrow);

			m_rectBRArrow.SetRect(m_rectClient.left, m_rectClient.bottom - cyArrow,
				m_rectClient.right, m_rectClient.bottom);

			// Calc. thumb size and position
			int yThumb, cyThumb, cyChannel = cyClient - (2 * cyArrow);
			bHasThumb = CalcThumb(cyChannel, yThumb, cyThumb);
			if (bHasThumb)
			{	// We have space for thumb.
				yThumb += m_rectTLArrow.bottom;
				m_rectThumb = m_rectTLArrow;
				m_rectThumb.top = yThumb;
				m_rectThumb.bottom = yThumb + cyThumb;
			}

			// Calc. channels
			m_rectTLChannel = m_rectTLArrow;
			m_rectBRChannel = m_rectBRArrow;
			m_rectTLChannel.top = m_rectTLArrow.bottom;
			if (bHasThumb)
			{
				m_rectTLChannel.bottom = m_rectThumb.top;
				m_rectBRChannel.top = m_rectThumb.bottom;
				m_rectBRChannel.bottom = m_rectBRArrow.top;
			}
			else
			{
				m_rectTLChannel.bottom = m_rectBRArrow.top;	// T channel reaches to B arrow.
				m_rectBRChannel.SetRectEmpty();				// No thumb - so T channel not needed.
			}
		}
	}

	// Recalculate thumb and channel rects - used while user is dragging the thumb.
	// [IN]  point = current mouse coords. in client coords.
	// Returns TRUE if thumb XY position has changed, else FALSE.
	// Note - Only thumb position changes, not the width/height.
	// Note - called from within OnMouseMove(...).
	BOOL RecalcRectsThumbTrack(CPoint point)
	{
		XeASSERT(m_bDragging && !m_rectThumb.IsRectEmpty());	// Sanity check.
		if (m_bHorizontal)
		{	// Horizontal scroll bar.
			// X pos of left edge of thumb (0...?)
			int xPos = point.x - m_xyThumbDragOffset;
			if (xPos < m_rectTLArrow.right)
				xPos = m_rectTLArrow.right;
			int nThumbWidth = m_rectThumb.Width();
			if (xPos > (m_rectBRArrow.left - nThumbWidth))
				xPos = (m_rectBRArrow.left - nThumbWidth);
			if (xPos == m_rectThumb.left)
				return FALSE;						// No change.
			m_rectThumb.left = xPos;
			m_rectThumb.right = m_rectThumb.left + nThumbWidth;
			m_rectTLChannel.right = m_rectThumb.left;
			m_rectBRChannel.left = m_rectThumb.right;
		}
		else
		{	// Vertical scroll bar.
			// Y pos of top edge of thumb (0...?)
			int yPos = point.y - m_xyThumbDragOffset;
			if (yPos < m_rectTLArrow.bottom)
				yPos = m_rectTLArrow.bottom;
			int nThumbHeight = m_rectThumb.Height();
			if (yPos > (m_rectBRArrow.top - nThumbHeight))
				yPos = (m_rectBRArrow.top - nThumbHeight);
			if (yPos == m_rectThumb.top)
				return FALSE;						// No change.
			m_rectThumb.top = yPos;
			m_rectThumb.bottom = m_rectThumb.top + nThumbHeight;
			m_rectTLChannel.bottom = m_rectThumb.top;
			m_rectBRChannel.top = m_rectThumb.bottom;
		}
		return TRUE;
	}

	void ScrollHere(int xyOffset)
	{
		// Calculate pos (in scroll units)
		if (xyOffset < 0)
			xyOffset = 0;
		if (m_rectThumb.IsRectEmpty() || !(m_dblPx_SU > 0))
			return;	// Can't 'Scroll Here'.
		double dblScrollPos = (double)xyOffset / m_dblPx_SU;
		T_int nScrollHerePos = m_nMinPos + round_ud_dbl(dblScrollPos);
		if (nScrollHerePos < m_nMinPos)
			nScrollHerePos = m_nMinPos;
		else if (nScrollHerePos > m_nMaxReportedPos)
			nScrollHerePos = m_nMaxReportedPos;
		m_nTrackPos = nScrollHerePos;
		SendScrollMsg(SB_THUMBPOSITION, (WORD)m_nTrackPos);
		SendScrollMsg(SB_ENDSCROLL);
	}
#pragma endregion Helpers

#pragma region DataMembers
	///////////////////////////////////////////////////////////////////////////
	// Data members
	HWND		m_hParent;			// Parent window.

	BOOL		m_bEnabled;			// Window enabled state - TRUE = enabled.
									// Note - Updated on WM_ENABLE message.

	BOOL		m_bHasFocus;		// TRUE when 'this' has input focus.
									// Set TRUE in OnSetFocus.
									// Set FALSE in OnKillFocus.

	BOOL		m_bDrawGripper;		// TRUE when 'gripper' shown on thumb box.
									// FALSE when 'gripper' not drawn.
									// Note - helps implement 'blinking' focus.

	BOOL		m_bHorizontal;		// TRUE = 'this' is horizontal scroll bar

	T_int		m_nPos;				// Current thumb position in scroll units
	T_int		m_nTrackPos;		// Current thumb position (while dragging).
	T_uint		m_uPage;			// Scroll 'page' size in scroll units (min = 0).
	T_int		m_nMinPos;			// Minimum scrolling position
	T_int		m_nMaxPos;			// Maximum scrolling position

	BOOL		m_bNoScroll;		// Set TRUE if 'this' scroll bar was disabled because
									// of SIF_DISABLENOSCROLL flag in SBM_SETSCROLLINFO msg.

	T_int		m_nMaxReportedPos;	// Max. pos scrollbar can report.
									// = m_nMaxPos - (m_uPage - 1).
									// = m_nMaxPos when m_uPage == 0.

	BOOL		m_bDragging;		// TRUE = thumb is being dragged

	BOOL		m_bNeedEndScroll;	// TRUE if sending SB_ENDSCROLL is needed.
									// Set TRUE in OnKeyDown if a SB_XXX message was sent.
									// Set TRUE in OnLButtonDown if a SB_XXX message was sent.
									// Set TRUE in OnMouseMove if a SB_XXX message was sent.
									// Note - SB_ENDSCROLL is sent in OnKeyDown and in
									//        OnLButtonUp if m_bNeedEndScroll is TRUE.

	XSB_AREA	m_eMouseOverArea;	// Where mouse is 'now'.
	XSB_AREA	m_eMouseDownArea;	// Where mouse is when L btn down.

	CPoint		m_ptMenu;			// Client coords. of context menu.
									// Needed for 'Scroll Here' command.

	//===================================================================================
	// These vars. are calculated when RecalcRects() called.
	CRect		m_rectClient;		// client rect - updated when RecalcRects() called.
	CRect		m_rectThumb;		// current rect for thumb
	CRect		m_rectTLArrow;		// top or left arrow rect
	CRect		m_rectBRArrow;		// bottom or right arrow rect
	CRect		m_rectTLChannel;	// top or left channel rect
	CRect		m_rectBRChannel;	// bottom or right channel rect

	double		m_dblPx_SU;			// Number of pixels in one scroll unit.
									// Note - not valid unless m_rectThumb is not empty.
	//===================================================================================

	UINT		m_uArrowWH;			// width or height of arrow button
									// - set by derived class.
									// If = 0 and Horz
									//		button width = client area height.
									// If = 0 and Vert
									//		button height = client area width.
									// Set to 0 by ctor.
	UINT GetCXYarrow()
	{
		if (m_uArrowWH)		// Has derived class set this value?
			return m_uArrowWH;	// Use arrow button width/height set by derived class.

		// If m_uArrowWH == 0 we must assume the arrow button is same width or height as
		// the scrollbar window.
		if (m_bHorizontal)
			return m_rectClient.Height();	// Horz. arrow button is same height as window.
		return m_rectClient.Width();		// Vert. arrow button is same width as window.
	}

	UINT		m_uThumbMinHW;		// Minimum width or height of thumb (pixels).
									// Set to 8 pixels by ctor.

	int			m_xyThumbDragOffset;// Offset (x or y) into m_rectThumb.
	// When user presses L button down in the thumb - we need to capture the
	// relative X or Y position of the mouse cursor in m_rectThumb.
	// This is used while dragging so mouse x or y rel. pos. in thumb is unchanged.

	double fabs_dbl(double dbl)
	{
		if (dbl >= 0)
			return dbl;
		return (-dbl);
	}

	// Round a double number up or down.
	T_int round_ud_dbl(double dbl)
	{
		BOOL bNeg = FALSE;
		if (dbl < 0)
		{
			bNeg = TRUE;
			dbl = -dbl;
		}
		T_int n = (T_int)dbl;
		double fract = dbl - (double)n;
		if (fract > 0.5)
			n++;
		if (bNeg)
			n = -n;
		return n;
	}
#pragma endregion DataMembers

	///////////////////////////////////////////////////////////////////////////
	//DECLARE_MESSAGE_MAP()
};

//// Special macro to enable use of two template parameters - see: MFC BEGIN_TEMPLATE_MESSAGE_MAP macro.
//#define BEGIN_TEMPLATE2_MESSAGE_MAP(theClass, type_name1, type_name2, baseClass)		\
//	PTM_WARNING_DISABLE																	\
//	template < typename type_name1, typename type_name2 >								\
//	const AFX_MSGMAP* theClass< type_name1, type_name2 >::GetMessageMap() const			\
//		{ return GetThisMessageMap(); }													\
//	template < typename type_name1, typename type_name2 >								\
//	const AFX_MSGMAP* PASCAL theClass< type_name1, type_name2 >::GetThisMessageMap()	\
//	{																					\
//		typedef theClass< type_name1, type_name2 > ThisClass;							\
//		typedef baseClass TheBaseClass;													\
//		__pragma(warning(push))															\
//		__pragma(warning(disable: 4640)) /* message maps can only be called by single threaded message pump */ \
//		static const AFX_MSGMAP_ENTRY _messageEntries[] =								\
//		{
//
//BEGIN_TEMPLATE2_MESSAGE_MAP(CXeScrollBarBase32_64, T_int, T_uint, CScrollBar)
//	ON_MESSAGE(SBM_ENABLE_ARROWS, &CXeScrollBarBase32_64::OnSbmEnableArrows)
//	ON_MESSAGE(SBM_GETPOS, &CXeScrollBarBase32_64::OnSbmGetPos)
//	ON_MESSAGE(SBM_GETRANGE, &CXeScrollBarBase32_64::OnSbmGetRange)
//	ON_MESSAGE(SBM_GETSCROLLBARINFO, &CXeScrollBarBase32_64::OnSbmGetScrollBarInfo)
//	ON_MESSAGE(SBM_GETSCROLLINFO, &CXeScrollBarBase32_64::OnSbmGetScrollInfo)
//	ON_MESSAGE(SBM_SETPOS, &CXeScrollBarBase32_64::OnSbmSetPos)
//	ON_MESSAGE(SBM_SETRANGE, &CXeScrollBarBase32_64::OnSbmSetRange)
//	ON_MESSAGE(SBM_SETRANGEREDRAW, &CXeScrollBarBase32_64::OnSbmSetRangeRedraw)
//	ON_MESSAGE(SBM_SETSCROLLINFO, &CXeScrollBarBase32_64::OnSbmSetScrollInfo)
//	ON_WM_PAINT()
//	ON_WM_ERASEBKGND()
//	ON_MESSAGE(WM_KEYDOWN, &CXeScrollBarBase32_64::OnKeyDown)
//	ON_MESSAGE(WM_KEYUP, &CXeScrollBarBase32_64::OnKeyUp)
//	ON_WM_LBUTTONDOWN()
//	ON_WM_LBUTTONUP()
//	ON_MESSAGE(WM_MOUSEWHEEL, &CXeScrollBarBase32_64::OnMouseWheel)
//	ON_WM_MOUSEMOVE()
//	ON_MESSAGE(WM_MOUSELEAVE, &CXeScrollBarBase32_64::OnMouseLeave)
//	ON_MESSAGE(WM_GETDLGCODE, &CXeScrollBarBase32_64::OnGetDlgCode)
//	ON_WM_CANCELMODE()
//	ON_WM_CONTEXTMENU()
//	ON_COMMAND_RANGE(XSB_IDM_SCR_UL, XSB_IDM_SCR_BR, &CXeScrollBarBase32_64::OnMenuCommands)
//	ON_WM_TIMER()
//	ON_WM_SIZE()
//	ON_WM_SETFOCUS()
//	ON_WM_KILLFOCUS()
//	ON_WM_ENABLE()
//END_MESSAGE_MAP()

class CXeScrollBarBase : public CXeScrollBarBase32_64<int32_t, uint32_t>
{
public:
	CXeScrollBarBase(CXeUIcolorsIF* pUIcolors) : CXeScrollBarBase32_64<int32_t, uint32_t>(pUIcolors) {}

	//DECLARE_MESSAGE_MAP()
};

//BEGIN_MESSAGE_MAP(CXeScrollBarBase, CXeScrollBarBase32_64)
//END_MESSAGE_MAP()

export struct SCROLLINFO64
{
	UINT    cbSize;
	UINT    fMask;
	INT64   nMin;
	INT64   nMax;
	UINT64  nPage;
	INT64   nPos;
	INT64   nTrackPos;
};

class CXeScrollBarBase64 : public CXeScrollBarBase32_64<int64_t, uint64_t>
{
public:
	CXeScrollBarBase64(CXeUIcolorsIF* pUIcolors) : CXeScrollBarBase32_64<int64_t, uint64_t>(pUIcolors) {}

	///////////////////////////////////////////////////////////////////////////
	// 64bit scrollbar functions

	// Set scroll info. Returns scrollpos.
	INT64 SetScrollInfo64(SCROLLINFO64* pScrollInfo, BOOL fRedraw)
	{
		if (pScrollInfo->fMask & SIF_PAGE)
		{
			m_uPage = pScrollInfo->nPage;
			// Note - windows scrollbars can have a page size = 0.
		}
		if (pScrollInfo->fMask & SIF_RANGE)
		{
			m_nMinPos = pScrollInfo->nMin;
			m_nMaxPos = pScrollInfo->nMax;
		}
		if (pScrollInfo->fMask & SIF_POS)
		{
			m_nPos = pScrollInfo->nPos;
		}
		if (pScrollInfo->fMask & SIF_DISABLENOSCROLL)
		{
			BOOL bEnable = ((INT64)m_uPage < (m_nMaxPos - m_nMinPos)) ? TRUE : FALSE;
			m_bNoScroll = !bEnable;
			EnableWindow(bEnable);
		}

		if (m_nMaxPos < m_nMinPos)
			m_nMaxPos = m_nMinPos;
		INT64 nSUrange = abs(m_nMaxPos - m_nMinPos) + 1;
		if (m_uPage > (UINT64)nSUrange)
			m_uPage = (UINT64)nSUrange;

		if (m_uPage == 0)
			m_nMaxReportedPos = m_nMaxPos;
		else
			m_nMaxReportedPos = m_nMaxPos - ((INT64)m_uPage - 1);

		if (m_nPos < m_nMinPos)
			m_nPos = m_nMinPos;
		else if (m_nPos > m_nMaxReportedPos)
			m_nPos = m_nMaxReportedPos;

		RecalcRects();

		if (fRedraw)
		{
			_RedrawDirectly();
		}

		return m_nPos;
	}

	// Set range and page, if current nPos is invalid it's set to a valid value (nMin or max reported pos.).
	// return true if any changes.
	bool SetRangeAndPage64(INT64 nMin, INT64 nMax, UINT64 nPage, BOOL fRedraw)
	{
		SCROLLINFO64 siBefore = { 0 }, si = { 0 };
		siBefore.cbSize = sizeof(SCROLLINFO64);
		GetScrollInfo64(&siBefore);

		si.cbSize = sizeof(SCROLLINFO64);
		si.fMask = SIF_PAGE | SIF_RANGE;
		si.nPage = nPage;
		si.nMin = nMin;
		si.nMax = nMax;
		SetScrollInfo64(&si, fRedraw);

		return siBefore.nMin != nMin || siBefore.nMax != nMax || siBefore.nPage != nPage;
	}

	void SetScrollPos64(INT64 nPos, BOOL fRedraw)
	{
		SCROLLINFO64 si = { 0 };
		si.cbSize = sizeof(SCROLLINFO64);
		si.fMask = SIF_POS;
		si.nPos = nPos;
		SetScrollInfo64(&si, fRedraw);
	}

	INT64 GetScrollPos64()
	{
		return m_nPos;
	}

	INT64 GetTrackPos64()
	{
		return m_nTrackPos;
	}

	void GetScrollInfo64(SCROLLINFO64* pScrollInfo)
	{
		pScrollInfo->cbSize = sizeof(SCROLLINFO64);
		pScrollInfo->fMask = SIF_ALL;
		pScrollInfo->nMin = m_nMinPos;
		pScrollInfo->nMax = m_nMaxPos;
		pScrollInfo->nPage = m_uPage;
		pScrollInfo->nPos = m_nPos;
		pScrollInfo->nTrackPos = m_nTrackPos;
	}

	// Scroll to position by sending SB_THUMBPOSITION (nTrackPos=nPos) followed by SB_ENDSCROLL.
	void SimulateScrollToPos(INT64 nPos)
	{
		if (nPos < m_nMinPos)
			nPos = m_nMinPos;
		else if (nPos > m_nMaxReportedPos)
			nPos = m_nMaxReportedPos;
		m_nTrackPos = nPos;
		SendScrollMsg(SB_THUMBPOSITION, (WORD)m_nTrackPos);
		SendScrollMsg(SB_ENDSCROLL);
	}

	bool IsDraggingThumb() { return m_bDragging; }

	//DECLARE_MESSAGE_MAP()
};

//BEGIN_MESSAGE_MAP(CXeScrollBarBase64, CXeScrollBarBase32_64)
//END_MESSAGE_MAP()

//enum class BtnTp { left, right, up, down };
//void DrawScrollbarButton(CXeUIcolorsIF* xeUI, CDC& dc, const CRect& rcBtn, CID fgColor, BtnTp tp)
//{
//	bool isHorz = tp == BtnTp::left || tp == BtnTp::right;
//	int cxTri = isHorz ? 5 : 9, cyTri = isHorz ? 9 : 5;
//	int cyTriMargin = (int)((double)(rcBtn.Height() - cyTri) / 2 + 0.5);
//	int cxTriMargin = (int)((double)(rcBtn.Width() - cxTri) / 2 + 0.5);
//	int x1 = rcBtn.left + cxTriMargin, x2 = x1 + cxTri;
//	int y1 = (int)((double)rcBtn.Height() / 2 + 0.5), y2 = rcBtn.top + cyTriMargin;
//	int x3 = x2, y3 = y2 + cyTri;
//	if (tp == BtnTp::right)
//	{
//		std::swap(x1, x2);
//		x3 = x2;
//	}
//	else if (!isHorz)
//	{
//		x1 = (int)((double)rcBtn.left + (double)rcBtn.Width() / 2 + 0.5);
//		x2 = rcBtn.left + cxTriMargin;
//		y1 = rcBtn.top + cyTriMargin, y2 = y1 + cyTri;
//		x3 = x2 + cxTri, y3 = y2;
//		if (tp == BtnTp::down)
//		{
//			std::swap(y1, y2);
//			y3 = y2;
//		}
//	}
//
//	POINT tr[4];
//	tr[0].x = x1;
//	tr[0].y = y1;
//	tr[1].x = x2;
//	tr[1].y = y2;
//	tr[2].x = x3;
//	tr[2].y = y3;
//	tr[3].x = x1;
//	tr[3].y = y1;
//	dc.SelectObject(xeUI->GetPen(fgColor));
//	dc.SelectObject(xeUI->GetBrush(fgColor));
//	dc.SetPolyFillMode(ALTERNATE);
//	dc.Polygon(tr, 4);
//}

// Draw scroll bar.
// isDrawBackLayer = true to draw backmost layer (buttons, channel), = false to draw topmost layer (thumb).
//void DrawScrollBarHelper(XeScrollBar_PaintInfo& ps, bool isDrawBackLayer)
//{
//	XeASSERT(ps.m_xeUI);
//	const CRect* prcElem = 0;
//	for (const XeScrollBar_AreaInfo& ai : ps.m_areaInfo)
//	{
//		// Get bounding rect of area to draw (client coords.)
//		prcElem = ai.m_prcArea;
//		if (ai.m_eState == XSB_EDRAWELEM::eNotDrawn)	// Rect empty or area not drawn?
//			continue;
//
//		CID cidBg = CID::CtrlBg, cidArwFg, cidThm;
//		switch (ai.m_eState)
//		{
//		case XSB_EDRAWELEM::eDisabled:
//			cidArwFg = CID::CtrlTxtDis;
//			cidThm = CID::CtrlTxtDis;
//			break;
//		case XSB_EDRAWELEM::eNormal:
//			cidArwFg = CID::CtrlBgDis;
//			cidThm = CID::CtrlBgDis;
//			break;
//		case XSB_EDRAWELEM::eDown:
//			cidArwFg = CID::CtrlCurItemBg;
//			cidThm = CID::CtrlTxtDis;
//			break;
//		case XSB_EDRAWELEM::eHot:
//			cidArwFg = CID::CtrlCurItemBg;
//			cidThm = CID::CtrlTxtDis;
//			break;
//		default:
//			XeASSERT(FALSE);	// Unknown state!
//		}
//
//		// Draw scrollbar element (area)
//		if (ai.m_area.IsButton())
//		{
//			if (isDrawBackLayer)
//			{
//				ps.m_dcMem.FillSolidRect(*prcElem, ps.m_xeUI->GetColor(cidBg));
//				BtnTp tp = ai.m_area.eArea == XSB_EAREA::eTLbutton
//					? (ps.m_bHorizontal ? BtnTp::left : BtnTp::up)
//					: (ps.m_bHorizontal ? BtnTp::right : BtnTp::down);
//				DrawScrollbarButton(ps.m_xeUI, ps.m_dcMem, *prcElem, cidArwFg, tp);
//			}
//		}
//		else if (ai.m_area.IsChannel())
//		{
//			if (isDrawBackLayer)
//			{
//				ps.m_dcMem.FillSolidRect(*prcElem, ps.m_xeUI->GetColor(cidBg));
//			}
//		}
//		else	// Is thumb
//		{
//			CRect rcThm(*prcElem);
//			if (isDrawBackLayer)
//			{
//				ps.m_dcMem.FillSolidRect(*prcElem, ps.m_xeUI->GetColor(cidBg));
//			}
//			else
//			{
//				if (ps.m_bHorizontal)
//				{
//					rcThm.DeflateRect(0, ((rcThm.Height() - 9) / 2));
//				}
//				else	// Vertical thumb
//				{
//					rcThm.DeflateRect(((rcThm.Width() - 9) / 2), 0);
//				}
//				ps.m_dcMem.FillSolidRect(rcThm, ps.m_xeUI->GetColor(cidThm));
//			}
//		}
//	}
//}

export class CXeScrollBar : public CXeScrollBarBase
{
public:
	CXeScrollBar(CXeUIcolorsIF* pUIcolors) : CXeScrollBarBase(pUIcolors) {}
	//virtual ~CXeScrollBar() {}
	
protected:
	// Override base class member.
	virtual void _DrawScrollBarBackLayer(XeScrollBar_PaintInfo& ps) override
	{
		//DrawScrollBarHelper(ps, true);
		_DrawScrollBar(ps, true);
	}

	virtual void _DrawScrollBarMiddleLayer(XeScrollBar_PaintInfo& ps) override
	{
	}

	virtual void _DrawScrollBarThumbLayer(XeScrollBar_PaintInfo& ps) override
	{
		//DrawScrollBarHelper(ps, false);
		_DrawScrollBar(ps, false);
	}

	virtual void _DrawScrollBarTopLayer(XeScrollBar_PaintInfo& ps) override
	{
	}

	//DECLARE_MESSAGE_MAP()
};

//BEGIN_MESSAGE_MAP(CXeScrollBar, CXeScrollBarBase)
//END_MESSAGE_MAP()

export class CXeScrollBar64 : public CXeScrollBarBase64
{
public:
	CXeScrollBar64(CXeUIcolorsIF* pUIcolors) : CXeScrollBarBase64(pUIcolors) {}
	virtual ~CXeScrollBar64() {}

protected:
	// Override base class member.
	virtual void _DrawScrollBarBackLayer(XeScrollBar_PaintInfo& ps) override
	{
		//DrawScrollBarHelper(ps, true);
		_DrawScrollBar(ps, true);
	}

	virtual void _DrawScrollBarMiddleLayer(XeScrollBar_PaintInfo& ps) override
	{
	}

	virtual void _DrawScrollBarThumbLayer(XeScrollBar_PaintInfo& ps) override
	{
		//DrawScrollBarHelper(ps, false);
		_DrawScrollBar(ps, false);
	}

	virtual void _DrawScrollBarTopLayer(XeScrollBar_PaintInfo& ps) override
	{
	}

	//DECLARE_MESSAGE_MAP()
};

//BEGIN_MESSAGE_MAP(CXeScrollBar64, CXeScrollBarBase64)
//END_MESSAGE_MAP()

