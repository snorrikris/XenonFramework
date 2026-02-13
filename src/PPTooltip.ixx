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

#include <string>
#include <format>
#include <d2d1.h>
#include <dwrite.h>

#include "PPHtmlDrawer.h"

#include "PPTooltipDefs.h"

//#include <boost/algorithm/string.hpp>
//#include "XeStringTools.h"

export module Xe.PPTooltip;

import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.D2DWndBase;
import Xe.Monitors;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TIMER_HIDE		0x101 //the identifier of the timer for hide the tooltip
#define TIMER_SHOW		0x100 //the identifier of the timer for show the tooltip

constexpr wchar_t PPTOOLTIP_CLASSNAME[] = L"PPToolTipWnd";

export class CPPToolTip : public CXeD2DWndBase, public CXeTooltipIF
{
#pragma region Class_data
protected:
	enum TooltipState {
		PPTOOLTIP_STATE_HIDDEN = 0,
		PPTOOLTIP_STATE_SHOWN
	};

	std::unique_ptr<CPPHtmlDrawer> m_drawer; //HTML drawer object

	HWND m_hParentWnd = 0; // The handle of the parent window

	CPoint m_ptOriginal;

	// Info about last displayed tool
	HWND  m_hwndDisplayedTool = 0;
	PPTOOLTIP_INFO m_tiDisplayed; //Info about displayed tooltip

	// Info about last displayed tool
	HWND  m_hwndNextTool = 0;
	PPTOOLTIP_INFO m_tiNextTool; //Info about next tooltip

	// Info about current tool
	TooltipState m_nTooltipState = PPTOOLTIP_STATE_HIDDEN;

	//Default values for the window
	DWORD m_dwTimeAutoPop = 60000; //Retrieve the length of time the tool tip window remains visible if the pointer is stationary within a tool's bounding rectangle
	DWORD m_dwTimeInitial = 500; //Retrieve the length of time the pointer must remain stationary within a tool's bounding rectangle before the tool tip window appears

	CRect m_rcTipArea; //The bound rect around the tip's area in the client coordinates.
	CRect m_rcTooltip; //The bound rect around the body of the tooltip in the client coordinates.

	std::wstring m_logName;
#pragma endregion Class_data

#pragma region Create
public:
	CPPToolTip(CXeUIcolorsIF* pUIcolors) : CXeD2DWndBase(pUIcolors)
	{
		m_drawer = std::make_unique<CPPHtmlDrawer>(pUIcolors);
		m_drawer->SetMaxWidth(0);
	}

	virtual ~CPPToolTip() {}

	BOOL Create(const wchar_t* szNameForLogging, HWND hParentWnd, BOOL bBalloon = TRUE)
	{
		CPPTOOLTIP_TRACE("CPPToolTip::Create\n");

		m_logName = szNameForLogging;

		// Register the window class if it has not already been registered.
		m_xeUI->RegisterWindowClass(PPTOOLTIP_CLASSNAME, D2DCtrl_WndProc);

		DWORD dwStyle = WS_POPUP;
		DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
		m_hParentWnd = hParentWnd;
		XeASSERT(m_hParentWnd);	// Parent window not created yet.

		HWND hWnd = CreateD2DWindow(dwExStyle, PPTOOLTIP_CLASSNAME,
				nullptr, dwStyle, CRect(), m_hParentWnd, NULL);
		if (!hWnd)
		{
			return false;
		}

		return TRUE;
	}

	void DestroyWindow() { ::DestroyWindow(Hwnd()); }

protected:
	LRESULT _OnDestroy() override
	{
		CPPTOOLTIP_TRACE("DestroyWindow()\n");
		Pop();
		return 0;
	}
#pragma endregion Create

#pragma region CXeTooltipIF_impl
public:
	virtual BOOL RelayMessageToTooltip(UINT message, CPoint pt_msg) override
	{
		return RelayEvent(message, pt_msg);
	}

	virtual void HideTooltip() override
	{
		_HideTooltip();
	}

	virtual bool IsMouseOverTooltip() override
	{
		return IsCursorOverTooltip();
	}
#pragma endregion CXeTooltipIF_impl

protected:
	LRESULT SendNotifyNeedTT(LPPOINT pt, PPTOOLTIP_INFO& ti)
	{
		CPPTOOLTIP_TRACE("CPPToolTip::SendNotifyNeedTT()\n");
		// Make sure this is a valid window  
		if (!IsWindow(GetSafeHwnd()))
			return 0L;

		NM_PPTOOLTIP_NEED_TT lpnm;
		lpnm.pt = pt;
		lpnm.ti = &ti;
		lpnm.hdr.hwndFrom = Hwnd();
		lpnm.hdr.idFrom = ::GetDlgCtrlID(Hwnd());
		lpnm.hdr.code = UDM_TOOLTIP_NEED_TT;

		return ::SendMessage(m_hParentWnd, WM_NOTIFY, lpnm.hdr.idFrom, (LPARAM)&lpnm);
	}

	void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		CPPTOOLTIP_TRACE("CPPToolTip::OnPaint()\n");

		//Copying info about current tool to displayed
		m_hwndDisplayedTool = m_hwndNextTool;
		m_tiDisplayed = m_tiNextTool;

		TT_Render rndr;
		rndr.m_pRT = pRT;
		rndr.m_font = m_xeUI->D2D_GetFont(EXE_FONT::eUI_Font);
		rndr.m_text_brush = GetBrush(CID::TT_Text);
		rndr.m_bg_brush = GetBrush(CID::TT_Bg);
		rndr.m_border_brush = GetBrush(CID::TT_BgBorder);

		pRT->FillRectangle(rc, rndr.m_bg_brush);

		m_drawer->DrawPreparedOutput(rndr, &m_rcTipArea);

		D2D1_ROUNDED_RECT rr;
		rr.rect = rc;
		rr.radiusX = 2;
		rr.radiusY = 2;
		pRT->DrawRoundedRectangle(rr, rndr.m_border_brush, 1.0f);
	}

	bool _FilterMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, const CPoint& msg_pt) override
	{
		RelayEvent(uMsg, msg_pt);
		return false;	// Message should be processed normally
	}

	BOOL RelayEvent(UINT message, CPoint pt_msg) 
	{
		//ENG: Disables a message translation if object was't created (thanks to Stoil Todorov)
		//RUS: Запрет обработки сообщений если объект не создан
		if (NULL == GetSafeHwnd())
			return FALSE;

		XeASSERT(m_hParentWnd);

		switch (message)
		{
		case WM_RBUTTONDOWN:
			// The user has interrupted the current tool - dismiss it
			CPPTOOLTIP_TRACE("Pop() in RelayEvent because WM_RBUTTONDOWN\n");
			Pop();
			break;
		case WM_LBUTTONDOWN:
			CPPTOOLTIP_TRACE("CPPToolTip::WM_LBUTTONDOWN\n");
			if (IsCursorOverTooltip())
			{
				//Left Button was pressed over the tooltip
				CPoint pt = pt_msg;
				::ScreenToClient(Hwnd(), &pt);
				m_drawer->OnLButtonDown(&pt, 0);
			} //if
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_NCLBUTTONDOWN:
		case WM_NCLBUTTONDBLCLK:
		case WM_NCRBUTTONDOWN:
		case WM_NCRBUTTONDBLCLK:
		case WM_NCMBUTTONDOWN:
		case WM_NCMBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
			//		// The user has interrupted the current tool - dismiss it
			//		if (!(m_tiDisplayed.nBehaviour & PPTOOLTIP_NOCLOSE_MOUSEDOWN))
			CPPTOOLTIP_TRACE("Pop() in RelayEvent because mouse or keyb\n");
			Pop();
			break;
		case WM_NCMOUSEMOVE:
		case WM_MOUSEMOVE:
			_OnRelayMsg_MouseMove(pt_msg);
			break;
		} //switch

		return FALSE;
	} //End RelayEvent

	void _OnRelayMsg_MouseMove(const CPoint& pt_msg)
	{
		if (m_ptOriginal.x == pt_msg.x && m_ptOriginal.y == pt_msg.y)
		{
			return;
		}
		// The mouse pointer's position was changed

		//Initialize values
		HWND hWnd = NULL;
		CPoint pt;
		CRect rect;
		PPTOOLTIP_INFO ti;
		std::wstring strTemp;

		m_ptOriginal = pt = pt_msg;
		::ScreenToClient(m_hParentWnd, &pt);
		if (IsCursorOverTooltip() /*&& !(m_tiDisplayed.nBehaviour & PPTOOLTIP_TRACKING_MOUSE)*/)
		{
			MM_CPPTOOLTIP_TRACE("over tooltip ==========\n");

			//ENG: Mouse over a tooltip and tracking mode was disabled
			//RUS: Курсор над тултипом при выключенном режиме "тракинга"
			if (!(m_tiDisplayed.nBehaviour & PPTOOLTIP_NOCLOSE_OVER))
			{
				//ENG: A tooltip don't hides when mouse over him
				//RUS: Если не установлен стиль не закрывания тултипа если курсор над ним
				CPPTOOLTIP_TRACE("over tooltip == HideTooltip\n");
				_HideTooltip();
			}
			else
			{
				//ENG: Resetup autopop timer
				//RUS: Переустанавливаем таймер автозакрытия тултипа
				SetAutoPopTimer();

				//+++hd
				// since mouse is now being captured, we
				// need this code here
				//CPoint ptClient;
				//::GetCursorPos(&ptClient);
				//::ScreenToClient(Hwnd(), &ptClient);

				//if (m_drawer->OnSetCursor(&ptClient))
				//{
				//	MM_CPPTOOLTIP_TRACE("cursor over hyperlink\n");
				//}

			} //if
		}
		else
		{
			MM_CPPTOOLTIP_TRACE("not over tooltip ==========\n");
			//ENG: Searching a toolbar's item
			//RUS: Ищем элемент на панели инструментов
			if (NULL == hWnd)	// +++sk
			{
				if (SendNotifyNeedTT(&pt, ti))
				{
					hWnd = ti.hTWnd;
				}
			} //if
			if (NULL == hWnd)
			{
				//ENG: An item with a tooltip wasn't found
				//RUS: Ни один элемент, отображающий тултип, не найден
				m_hwndDisplayedTool = NULL;
				m_tiDisplayed.rectBounds.SetRectEmpty();
				MM_CPPTOOLTIP_TRACE("KillTimer(TIMER_SHOW) An item with a tooltip wasn't found\n");
				::KillTimer(Hwnd(), TIMER_SHOW);
				_HideTooltip();
			}
			else
			{
				MM_CPPTOOLTIP_TRACE("found tool =========\n");
				if ((hWnd != m_hwndDisplayedTool) || (ti.rectBounds != m_tiDisplayed.rectBounds/* m_rcDisplayedTool*/))
				{
					//ENG: Sets new tooltip for the new window or for the new window's item
					//RUS: Если новое окно или новый элемент окна, то установить новый тултип
					SetNewTooltip(hWnd, ti);
				}
				else
				{
					//ENG: Nothing was changed
					//RUS: Если ни окно, ни элемент окна не изменялись
					MM_CPPTOOLTIP_TRACE("calling SetAutoPopTimer ==========\n");
					//ENG: A tooltip don't must when a mouse is over window
					//RUS: Тултип не должен прятаться пока находится над окном
					SetAutoPopTimer();
				} //if
			} //if
		} //if
	}

	void SetNewTooltip(HWND hWnd, const PPTOOLTIP_INFO& ti, BOOL bDisplayWithDelay = TRUE)
	{
		CPPTOOLTIP_TRACE("CPPToolTip::SetNewTooltip(hWnd=0x%08X, CRect(left=%d, top=%d, right=%d, bottom=%d))\n",
			hWnd, ti.rectBounds.left, ti.rectBounds.top, ti.rectBounds.right, ti.rectBounds.bottom);

		CPPTOOLTIP_TRACE("m_nTooltipState = %d\n", m_nTooltipState);

		//ENG: Hides a tooltip
		//RUS: Прячем тултип если он показан или показывается
		if (PPTOOLTIP_STATE_SHOWN == m_nTooltipState)
		{
			CPPTOOLTIP_TRACE("SHOWING or SHOWN == HideTooltip\n");
			_HideTooltip();
		}

		//+++sk
		// Prevent tootip from showing if any mouse button down or if shift, ctrl or alt down.
		if (PreventTooltipShowing())
		{
			CPPTOOLTIP_TRACE("PreventTooltipShowing\n");
			return;
		}

		//ENG: If a tooltip wasn't hidden
		//RUS: Если тултип еще не спрятан, ждем ...
		m_hwndNextTool = hWnd;
		m_tiNextTool = ti;

		//ENG: Start the show timer
		//RUS: Начинаем показ нового тултипа
		if (bDisplayWithDelay && m_dwTimeInitial)
		{
			CPPTOOLTIP_TRACE("Start the show timer - delay=%d\n", m_dwTimeInitial);
			::SetTimer(Hwnd(), TIMER_SHOW, m_dwTimeInitial, NULL);
		}
		else
		{
			_OnTimer(TIMER_SHOW, 0);
		}
	} //End of SetNewTooltip

	BOOL PreventTooltipShowing()
	{
		BOOL fLmouse = (::GetKeyState(VK_LBUTTON) & 0x8000) ? TRUE : FALSE;
		BOOL fRmouse = (::GetKeyState(VK_RBUTTON) & 0x8000) ? TRUE : FALSE;
		BOOL fMmouse = (::GetKeyState(VK_MBUTTON) & 0x8000) ? TRUE : FALSE;
		BOOL fShiftKeyDown = (::GetKeyState(VK_SHIFT) & 0x8000) ? TRUE : FALSE;
		BOOL fMenuKeyDown = (::GetKeyState(VK_MENU) & 0x8000) ? TRUE : FALSE;
		if (fLmouse || fRmouse || fMmouse || fShiftKeyDown || fMenuKeyDown)
			return TRUE;
		return FALSE;
	}

	LRESULT _OnTimer(WPARAM wParam, LPARAM lParam) override
	{
		UINT_PTR nIDEvent = wParam;
		POINT pt;
		switch (nIDEvent)
		{
		case TIMER_SHOW:
			CPPTOOLTIP_TRACE("OnTimerShow\n");
			//ENG: Kill SHOW timer 
			//RUS: Убить таймер ожидания показа тултипа
			::KillTimer(Hwnd(), TIMER_SHOW);
			//ENG: Get current mouse coordinates
			//RUS: Получить текущее положение тултипа
			GetCursorPos(&pt);
			if ((pt.x != m_ptOriginal.x) || (pt.y != m_ptOriginal.y))
			{
				//ENG: If mouse coordinates was changed
				//RUS: Если курсор сдвинулся, то уничтожить тултип
				CPPTOOLTIP_TRACE("OnTimerShow(HideTooltip) mouse has moved\n");
				_HideTooltip();
			}
			else if (PPTOOLTIP_STATE_HIDDEN == m_nTooltipState)
			{
				// Hide all other tooltips owned by 'this' process.
				m_xeUI->HideOtherTooltips(GetSafeHwnd());

				//Display first step
				PrepareDisplayTooltip(m_ptOriginal);

				CPPTOOLTIP_TRACE("OnTimerShow(Shown)\n");
				m_nTooltipState = PPTOOLTIP_STATE_SHOWN; //Tooltip is already show
				SetAutoPopTimer();
			} //if
			break;
		case TIMER_HIDE:
			CPPTOOLTIP_TRACE("OnTimerHide()\n");
			Pop();
			break;
		default:
			return CXeD2DWndBase::_OnTimer(wParam, lParam);
			break;
		} //switch
		return 0;	// Message was processed.
	} //End of the WM_TIMER handler

	void _HideTooltip()
	{
		if (m_nTooltipState == PPTOOLTIP_STATE_SHOWN)
		{
			CPPTOOLTIP_TRACE("CPPToolTip::HideTooltip(CurState=%d)\n", m_nTooltipState);
			_OnTimer(TIMER_HIDE, 0);
		}
	} //End of HideTooltip

	void SetAutoPopTimer()
	{
		if (m_dwTimeAutoPop /*&& !(m_tiDisplayed.nBehaviour & PPTOOLTIP_DISABLE_AUTOPOP)*/)
		{
			CPPTOOLTIP_TRACE("CPPToolTip::SetAutoPopTimer()\n");
			::SetTimer(Hwnd(), TIMER_HIDE, m_dwTimeAutoPop, NULL);
		}
	} //End of SetAutoPopTimer

	void Pop()
	{
		CPPTOOLTIP_TRACE("CPPToolTip::Pop()\n");

		m_nTooltipState = PPTOOLTIP_STATE_HIDDEN;
		::KillTimer(Hwnd(), TIMER_SHOW);
		::KillTimer(Hwnd(), TIMER_HIDE);
		if (IsWindowVisible())
		{
			::ShowWindow(Hwnd(), SW_HIDE);
		} //if
		m_hwndDisplayedTool = NULL;
		m_tiDisplayed = PPTOOLTIP_INFO();
		m_hwndNextTool = NULL;
		m_tiNextTool = PPTOOLTIP_INFO();
	} //End of Pop

	void PrepareDisplayTooltip(CPoint pt)	// in screen coordinates
	{
		CPPTOOLTIP_TRACE("CPPToolTip::PrepareDisplayTooltip()\n");

		//If string and icon are not exist then exit
		if (m_tiNextTool.sTooltip.empty())
			return;

		HWND hWnd = m_tiNextTool.hTWnd;	//+++sk
		if (hWnd == NULL)
		{
			pt.x += 24;
			pt.y += 22;
		}
		else
		{
			CRect rcCtrl;
			::GetWindowRect(hWnd, rcCtrl);
			pt.x = rcCtrl.left + 1;
			pt.y = rcCtrl.bottom;
			pt.x += m_tiNextTool.ptTipOffset.x;
			pt.y += m_tiNextTool.ptTipOffset.y;
		}

		//calculate the width and height of the box dynamically
		CSize sz(0, 0);
		m_drawer->PrepareOutput(m_tiNextTool.sTooltip.c_str(), &sz);

		m_rcTipArea.SetRect(0, 0, sz.cx, sz.cy);
		m_rcTooltip = m_rcTipArea;

		//Inflates on MARGIN_CX and MARGIN_CY sizes
		int nMarginY = 4, nMarginX = 6;
		m_rcTipArea.OffsetRect(nMarginX, nMarginY);
		m_rcTooltip.InflateRect(0, 0, 2 * nMarginX, 2 * nMarginY);

		//ENG: Sets a tooltip on the screen
		//RUS: Устанавливаем тултип на экране
		CRect rect = _AdjustTooltipOnScreenPosition(pt, m_rcTooltip.Size());

		SetWindowPos(NULL, rect, SWP_SHOWWINDOW | SWP_NOACTIVATE);
	} //End of PrepareDisplayTooltip

	[[nodiscard]] CRect _AdjustTooltipOnScreenPosition(const CPoint& pt_onscreen, const CSize& sizeTooltip)
	{
		CRect rect{ pt_onscreen, sizeTooltip };
		CMonitor mon = CMonitors::GetNearestMonitor(pt_onscreen);
		CRect rMonitor;
		mon.GetMonitorRect(&rMonitor);
		CPPTOOLTIP_TRACE("rMonitor:  left=%d  top=%d  right=%d  bottom=%d --------\n",
			rMonitor.left, rMonitor.top, rMonitor.right, rMonitor.bottom);
		CPPTOOLTIP_TRACE("rect:  left=%d  top=%d  right=%d  bottom=%d --------\n",
			rect.left, rect.top, rect.right, rect.bottom);

		if (rect.right > rMonitor.right)
		{
			rect.OffsetRect(rMonitor.right - rect.right, 0);
		}
		if (rect.left < rMonitor.left)
		{
			rect.OffsetRect(rMonitor.left - rect.left, 0);
		}
		if (rect.bottom > rMonitor.bottom)
		{
			CPPTOOLTIP_TRACE("off the screen -----------\n");
			rect.OffsetRect(0, rMonitor.bottom - rect.bottom);
			CPPTOOLTIP_TRACE("rect:  left=%d  top=%d  right=%d  bottom=%d ----------\n",
				rect.left, rect.top, rect.right, rect.bottom);
		}
		if (rect.top < rMonitor.top)
		{
			rect.OffsetRect(0, rMonitor.top - rect.top);
		}
		return rect;
	}

	BOOL IsCursorOverTooltip() const
	{
		XeASSERT(m_hParentWnd);
		if (IsWindowVisible())	// Is tooltip visible?
		{
			POINT pt;
			::GetCursorPos(&pt);
			HWND hWnd = ::WindowFromPoint(pt);
			return hWnd == Hwnd();
		}
		return FALSE;
	}

public:
	/////////////////////////////////////////////////////////////////////////////
	//  CPPToolTip::SetTextStyles()
	//    Applies a CSS-like style for the tooltip's HTML
	//---------------------------------------------------------------------------
	//  Parameters:
	//		lpszStyleName	- Pointer to a null-terminated string that specifies
	//						  a name of CSS style
	//		lpszStyleValue  - Pointer to a null-terminated string that specifies 
	//						  CSS-lite style for drawing a tooltip text.
	/////////////////////////////////////////////////////////////////////////////
	void SetTextStyle(LPCTSTR lpszStyleName, LPCTSTR lpszStyleValue)
	{
		m_drawer->SetTextStyle(lpszStyleName, lpszStyleValue);
	}

	////////////////////////////////////////////////////////////////////
	// CPPToolTip::EnableEscapeSequences()
	//		Enables the escape sequences. If the escape sequences was disabled
	//	HTML-lite compiler will ignore the codes less then 0x20 (such \n, \r, \t).
	////////////////////////////////////////////////////////////////////
	void EnableEscapeSequences(BOOL bEnable)
	{
		m_drawer->EnableEscapeSequences(bEnable);
	} //End of EnableEscapeSequences
};

