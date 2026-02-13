module;

#include "os_minimal.h"
#include <dwmapi.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <d2d1.h>
#include <dwrite.h>
#include <d2d1effects.h>
#include <d2d1_1.h>
#include "XeAssert.h"

export module Xe.D2DWndBase;

import Xe.UIcolorsIF;
export import Xe.D2DRenderContext;
import Xe.WindowStyle;
import Xe.Helpers;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif

// "global" pointer to UI object - used by D2DCtrl_WndProc.
CXeUIcolorsIF* s_xeUI = nullptr;

// Base class for window that uses DirectWrite for painting.
export class CXeD2DWndBase : public CXeD2DRenderContext
{
#pragma region Class_data
private:
	HWND m_hWnd = 0;

	HDC m_hWindowDC = 0;

	HDC m_hClientDC = 0;

public:
	void SetHwnd(HWND hWnd)
	{
		m_hWnd = hWnd;
	}

	HWND Hwnd() const
	{
		return m_hWnd;
	}

	HWND GetSafeHwnd() const
	{
		if (this)
		{
			return m_hWnd;
		}
		return 0;
	}

	XeWindowStyle m_style;

protected:
	bool m_uiResourcesCreatedWnd = false, m_uiResourcesCreatedDC = false;

	bool m_isHovering = false, m_isTracking = false;

	// Icon in the caption - when sys menu.
	Microsoft::WRL::ComPtr<IWICBitmap> m_d2d_icon_bitmap;

	BOOL m_bActive = FALSE;		// Window active or not.
	BOOL m_bTrackNcMouseLeave = FALSE;	// TRUE when TrackMouseEvent called.

	CXeTooltipIF* m_xtooltip = nullptr;
	bool m_isHideTooltipOnMouseLeave = true;

	// If border was specified in the style when windows was created.
	// Note - border style flags have been removed - when 'this' is a control (or child window).
	bool m_control_has_border = false;
#pragma endregion Class_data

#pragma region Create
public:
	CXeD2DWndBase(CXeUIcolorsIF* pUIcolors)
		: CXeD2DRenderContext(pUIcolors)
	{
		if (s_xeUI == nullptr)
		{
			s_xeUI = pUIcolors;
		}
		// Note - derived class can create more fonts at this point
		// (to use for calculating the window size needed).
	}
	virtual ~CXeD2DWndBase()
	{
		if (m_renderTargetClientDC.Get())
		{
			m_renderTargetClientDC.Reset();
		}
		if (m_hWnd)
		{
			XeASSERT(false);
			// Object deleted but window still exists.
			// Meaning that DestroyWindow was not called for 'this' window.
			// Note - hWnd is set to 0 when WM_NCDESTROY is processed.
		}
	}

	HWND CreateD2DWindow(DWORD dwExStyle, const std::wstring& class_name, const wchar_t* pTitle, DWORD dwStyle,
			const CRect& rcWnd, HWND hParentWnd, UINT uID, bool isEnableTooltip = false)
	{
		XeWindowStyleValue style;
		style.Set(dwStyle, dwExStyle);
		if (!(style.hasCaption() || style.hasThickBorder()) && style.hasBorder())
		{
			// Remove border style from the style vars.
			// We do this because - we don't want NC borders for controls (and child windows).
			// All the XeSomething constrols paint their own borders inside the client area.
			dwStyle &= ~WS_BORDER;
			dwExStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);

			m_control_has_border = true;
		}
		m_style.Set(0, dwStyle, dwExStyle, m_xeUI);
		HINSTANCE hInstance = m_xeUI->GetInstanceHandle();
		HWND hWnd = ::CreateWindowExW(dwExStyle, class_name.c_str(), pTitle, dwStyle,
				rcWnd.left, rcWnd.top, rcWnd.Width(), rcWnd.Height(), hParentWnd, (HMENU)(uint64_t)uID, hInstance,
				this);  // <-- pass 'this' pointer as parameter (to WM_NCCREATE)
		// Note - WM_NCCREATE is the first message sent to the new window.
		// When D2DCtrl_WndProc handles that message - the m_hWnd and m_style members are set.
		XeASSERT(m_hWnd && m_style.IsInitialized());
		if (isEnableTooltip)
		{
			_EnableTooltip(class_name);
		}
		return hWnd;
	}

protected:
	void _EnableTooltip(const std::wstring& nameForLogging)
	{
		m_xtooltip = m_xeUI->CreateTooltip(nameForLogging, GetSafeHwnd());
	}
#pragma endregion Create

#pragma region Behaviour
public:
	// This member is called (by some parent of 'this') when user has changed UI settings.
	// Derived class should override - if more than just repaint is needed.
	virtual void OnChangedUIsettings(bool isFontsChanged, bool isColorsChanged)
	{
		_RedrawDirectly();
	}
#pragma endregion Behaviour

#pragma region MessageHandlers
public:
	LRESULT _OnMessageD2DBase(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		DWORD dwMsgPos = ::GetMessagePos();
		CPoint msg_pt(LOWORD(dwMsgPos), HIWORD(dwMsgPos));
		bool isMouseMessage = IsMouseMessage(uMsg);
		if (isMouseMessage)
		{
			if ((m_xtooltip && m_xtooltip->RelayMessageToTooltip(uMsg, msg_pt))
					|| _FilterMouseMessage(uMsg, wParam, lParam, msg_pt))
			{
				return 0;
			}
		}
		switch (uMsg)
		{
		case WM_NCCREATE:
			return _OnNcCreate(hWnd, wParam, lParam);

		case WM_NCDESTROY:
			return _OnNcDestroy(hWnd, wParam, lParam);

		case WM_NCCALCSIZE:
			return _OnNcCalcSize(wParam, lParam);

		case WM_NCPAINT:
			return _OnNcPaint(wParam, lParam);

		case WM_NCACTIVATE:
			return _OnNcActivate(wParam, lParam);

		case WM_NCMOUSEMOVE:
			return _OnNcMouseMove(wParam, lParam);

		case WM_NCMOUSELEAVE:
			return _OnNcMouseLeave();

		case WM_NCLBUTTONDOWN:
			return _OnNcLbuttonDown(wParam, lParam);

		case WM_NCLBUTTONUP:
			return _OnNcLbuttonUp(wParam, lParam);

		case WM_NCHITTEST:
			return _OnNcHitTest(wParam, lParam);

		case WM_CREATE:
			return _OnCreate((CREATESTRUCTW*)lParam);

		case WM_CLOSE:
			return _OnClose();

		case WM_DESTROY:
			return _OnDestroy();

		case WM_QUERYENDSESSION:
			return _OnQueryEndSession(wParam, lParam);

		case WM_NOTIFY:
			return _OnNotify(wParam, lParam);

		case WM_STYLECHANGED:
			return _OnStyleChanged(wParam, lParam);

		case WM_GETMINMAXINFO:
			return _OnGetMinMaxInfo(wParam, lParam);

		case WM_EXITSIZEMOVE:
			return _OnExitSizeMove();

		case WM_SIZE:
			return _OnSize(hWnd, wParam, lParam);

		case WM_MOVE:
			return _OnMove(wParam, lParam);

		case WM_COMMAND:
			return _OnWmCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);

		case WM_SYSCOMMAND:
			return _OnSysCommand(wParam, lParam);

		case WM_DROPFILES:
			return _OnDropFiles(wParam, lParam);

		case WM_SHOWWINDOW:
			return _OnShowWindow(wParam, lParam);

		case WM_SETCURSOR:
			return _OnSetCursor(wParam, lParam);

		case WM_SETFOCUS:
			return _OnSetFocus((HWND)wParam);

		case WM_KILLFOCUS:
			return _OnKillFocus((HWND)wParam);

		case WM_CANCELMODE:
			return _OnCancelMode();

		case WM_ENABLE:
			return _OnEnableWindow((bool)wParam);

		case WM_SETTEXT:
			return _OnSetText(wParam, lParam);

		case WM_TIMER:
			return _OnTimer(wParam, lParam);

		case WM_MOUSEACTIVATE:
			return _OnMouseActivate(wParam, lParam);
		case WM_MOUSELEAVE:
			return _OnMouseLeave(wParam, lParam);
		case WM_MOUSEHOVER:
			return _OnMouseHover(wParam, lParam);

		case WM_MOUSEMOVE:
			return _OnMouseMove((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_LBUTTONDOWN:
			return _OnLeftDown((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_RBUTTONDOWN:
			return _OnRightDown((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_MBUTTONDOWN:
			return _OnMiddleDown((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_MOUSEWHEEL:
			return _OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam),
				CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_MOUSEHWHEEL:
			return _OnMouseHWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam),
				CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_LBUTTONUP:
			return _OnLeftUp((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_RBUTTONUP:
			return _OnRightUp((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_MBUTTONUP:
			return _OnMiddleUp((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_LBUTTONDBLCLK:
			return _OnLeftDoubleClick((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_RBUTTONDBLCLK:
			return _OnRightDoubleClick((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
		case WM_MBUTTONDBLCLK:
			return _OnMiddleDoubleClick((UINT)wParam, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));

		case WM_SYSKEYDOWN:
			return _OnSysKeyDown(wParam, lParam);
		case WM_SYSKEYUP:
			return _OnSysKeyUp(wParam, lParam);
		case WM_KEYDOWN:
			return _OnKeyDown(wParam, lParam);
		case WM_KEYUP:
			return _OnKeyUp(wParam, lParam);
		case WM_CHAR:
			return _OnChar(wParam, lParam);

		case WM_VSCROLL:
			return _OnVScroll(wParam, lParam);
		case WM_HSCROLL:
			return _OnHScroll(wParam, lParam);

		case WM_GETDLGCODE:
			return _OnGetDlgCode(wParam, lParam);

		case WM_CONTEXTMENU:
			return _OnContextMenu(wParam, lParam);

		case WM_ERASEBKGND:
			return 1;

		case WM_PRINT:
		case WM_PRINTCLIENT:	// Message received when AnimateWindow is called.
			return _OnPrintClient(hWnd, (HDC)wParam);

		case WM_PAINT:
			return _OnWmPaint(hWnd);
		}
		return _OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

protected:
	virtual bool _FilterMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, const CPoint& msg_pt)
	{
		return false;	// Message should be processed normally
	}

	virtual bool _PreTranslateMessage(MSG* pMsg)
	{
		return false;	// Message should be processed normally
	}

	// Derived class should override this to handle any other special messages.
	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	virtual LRESULT _OnNcCreate(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		if (m_style.WS().hasCaption())
		{
			// Disable DWM (Desktop Window Manager) rendering of NC window area.
			DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
			::DwmSetWindowAttribute(m_hWnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
		}

		return ::DefWindowProc(hwnd, WM_NCCREATE, wParam, lParam);
	}

	// Note - derived class MUST call here - if override this method.
	virtual LRESULT _OnNcDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		m_d2d_icon_bitmap.Reset();
		SetHwnd(0);
		m_xeUI->RemoveHwndD2DBasePtr(hwnd);
		return ::DefWindowProc(hwnd, WM_NCDESTROY, wParam, lParam);
	}

	virtual LRESULT _OnNcCalcSize(WPARAM wParam, LPARAM lParam)
	{
		BOOL bCalcValidRects = (BOOL)wParam;
		NCCALCSIZE_PARAMS* lpncsp = (NCCALCSIZE_PARAMS*)lParam;
		if (bCalcValidRects)
		{
			m_style.NcCalcSize(lpncsp);
		}
		// Consider returning WVR_VALIDRECTS
		return 0;			// We don't want any border (client area == window area).
	}

	virtual LRESULT _OnNcPaint(WPARAM wParam, LPARAM lParam)
	{
		if (!::IsIconic(m_hWnd))
		{
			_DoNcPaint();
			return 0;
		}
		return ::DefWindowProc(m_hWnd, WM_NCPAINT, wParam, lParam);
	}

	// Returns width of toolbar.
	virtual float _NCPaintToolbar()
	{
		return 0.0f;
	}

	virtual LRESULT _OnNcActivate(WPARAM wParam, LPARAM lParam)
	{
		BOOL bActive = (BOOL)wParam;
		if (m_style.WS().hasCaption())
		{
			if (!::IsWindowEnabled(m_hWnd))	// but not if disabled
			{
				bActive = FALSE;
			}

			if (bActive == m_bActive)
			{
				return (LRESULT)TRUE;					// nothing to do - but message was handled.
			}

			BOOL bDoPaint = FALSE;
			if (!::IsIconic(m_hWnd))	// Window not minimized?
			{
				// Turn WS_VISIBLE off before calling DefWindowProc,
				// so DefWindowProc won't paint and thereby cause flicker.
				DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
				if (dwStyle & WS_VISIBLE)
					::SetWindowLong(m_hWnd, GWL_STYLE, (dwStyle & ~WS_VISIBLE));
				::DefWindowProc(m_hWnd, WM_NCACTIVATE, bActive, 0L);
				if (dwStyle & WS_VISIBLE)
					::SetWindowLong(m_hWnd, GWL_STYLE, dwStyle);
				bDoPaint = TRUE;
				// At this point, nothing has happened (since WS_VISIBLE was off).
			}

			m_bActive = bActive;			// update state

			if (bDoPaint)	// Now it's time to paint.
			{	// Paint when 'normal' or 'maximized'.
				::SendMessage(m_hWnd, WM_NCPAINT, 0, 0);	// paint non-client area (frame too)
				return (LRESULT)TRUE;
			}
			// Message not handled (do default when minimized).
		}
		return ::DefWindowProc(m_hWnd, WM_NCACTIVATE, wParam, lParam);
	}

	LRESULT _OnNcMouseMove(WPARAM wParam, LPARAM lParam)
	{
		if (!::IsIconic(m_hWnd))
		{
			if (!m_bTrackNcMouseLeave)
			{	// We want a WM_NCMOUSELEAVE message when mouse leaves our NC area.
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(tme);
				tme.hwndTrack = m_hWnd;
				tme.dwFlags = TME_LEAVE | TME_NONCLIENT;
				tme.dwHoverTime = 0;

				// Note - _TrackMouseEvent is from comctl32.dll - emulates TrackMouseEvent
				// if it does not exist.
				m_bTrackNcMouseLeave = TrackMouseEvent(&tme);

				// Note - Need both TME_LEAVE and TME_NONCLIENT flags otherwise we don't get 
				// WM_NCMOUSELEAVE message.
			}

			CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			EIB_HIT eHit;
			bool isRepaintNC = m_style.NcMouseMove(point, eHit);

			isRepaintNC |= _OnNcMouseMoveXe(eHit, point);
			if (isRepaintNC)
			{
				// Repaint NC area to show 'hot' state.
				::SendMessage(m_hWnd, WM_NCPAINT, 0, 0);	// paint non-client area (frame too)
			}
		}
		return ::DefWindowProc(m_hWnd, WM_NCMOUSEMOVE, wParam, lParam);
	}

	// Return true if NC paint needed.
	virtual bool _OnNcMouseMoveXe(EIB_HIT eHit, const CPoint& point)
	{
		return false;
	}

	LRESULT _OnNcMouseLeave()
	{
		m_bTrackNcMouseLeave = FALSE;
		if (!::IsIconic(m_hWnd))
		{
			bool isRepaintNC = m_style.NcMouseLeave();
			isRepaintNC |= _OnNcMouseLeaveXe();
			if (isRepaintNC)
			{
				// Repaint NC area to show 'normal' state.
				::SendMessage(m_hWnd, WM_NCPAINT, 0, 0);	// paint non-client area (frame too)
			}
		}
		return ::DefWindowProc(m_hWnd, WM_NCMOUSELEAVE, 0, 0);
	}

	virtual bool _OnNcMouseLeaveXe()
	{
		return false;
	}

	LRESULT _OnNcLbuttonDown(WPARAM wParam, LPARAM lParam)
	{
		if (!::IsIconic(m_hWnd))
		{
			CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			EIB_HIT eHit;
			bool isRepaintRC = m_style.NcLButtonDown(point, eHit);	// Hit test for Icon or buttons.
			isRepaintRC |= _OnNcLbuttonDownXe(eHit, point);
			if (isRepaintRC)
			{
				// Repaint NC area to show button 'down' state.
				::SendMessage(m_hWnd, WM_NCPAINT, 0, 0);	// paint non-client area (frame too)

				return 0;
				//return TRUE;	// Return TRUE so we will get an UP message.
			}
			//return FALSE;	// Note - we will not get OnNcLButtonUp call when FALSE returned.
		}
		return ::DefWindowProc(m_hWnd, WM_NCLBUTTONDOWN, wParam, lParam);
	}

	// Return true if NC paint needed.
	virtual bool _OnNcLbuttonDownXe(EIB_HIT eHit, const CPoint& point)
	{
		return false;
	}

	LRESULT _OnNcLbuttonUp(WPARAM wParam, LPARAM lParam)
	{
		if (!::IsIconic(m_hWnd))
		{
			CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			WPARAM wParm;
			EIB_HIT eHit;
			bool isRepaintRC = m_style.NcLButtonUp(point, eHit, wParm);
			isRepaintRC |= _OnNcLbuttonUpXe(eHit, point);

			if (wParm)	// Send WM_SYSCOMMAND message for button command.
			{
				::SendMessage(m_hWnd, WM_SYSCOMMAND, wParm, 0);
				return 0;	// WM_NCLBUTTONUP message handled.
			}

			if (isRepaintRC)	// Repaint NC area if needed.
			{
				// Repaint NC area to show button 'normal' state.
				::SendMessage(m_hWnd, WM_NCPAINT, 0, 0);	// paint non-client area (frame too)
			}
		}
		return ::DefWindowProc(m_hWnd, WM_NCLBUTTONUP, wParam, lParam);
	}

	// Return true if NC paint needed.
	virtual bool _OnNcLbuttonUpXe(EIB_HIT eHit, const CPoint& point)
	{
		return false;
	}

	virtual LRESULT _OnNcHitTest(WPARAM wParam, LPARAM lParam)
	{
		if (!::IsIconic(m_hWnd))
		{
			CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return m_style.NcHitTest(point);
			//return HTCLIENT;
		}
		return ::DefWindowProc(m_hWnd, WM_NCHITTEST, wParam, lParam);
	}

	virtual LRESULT _OnCreate(CREATESTRUCTW* pCS)
	{
		return 0;
	}

	virtual LRESULT _OnClose()
	{
		return ::DefWindowProc(m_hWnd, WM_CLOSE, 0, 0);
	}

	virtual LRESULT _OnDestroy()
	{
		m_xeUI->DestroyTooltip(GetSafeHwnd());

		return 0;
	}

	virtual LRESULT _OnStyleChanged(WPARAM wParam, LPARAM lParam)
	{
		m_style.Update();
		return 0;
	}

	virtual LRESULT _OnMove(WPARAM wParam, LPARAM lParam)
	{
		m_style.Update();
		//return ::DefWindowProc(m_hWnd, WM_MOVE, wParam, lParam);
		return 0;
	}

	// Derived class MUST call here - if overriding this member.
	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
	{
		//UINT nType = (UINT)wParam;
		//int cx = GET_X_LPARAM(lParam);
		//int cy = GET_Y_LPARAM(lParam);

		m_style.Update();
		return 0;
	}

	virtual LRESULT _OnExitSizeMove()
	{
		m_style.Update();
		return ::DefWindowProc(m_hWnd, WM_EXITSIZEMOVE, 0, 0);
	}

	virtual LRESULT _OnQueryEndSession(WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProc(m_hWnd, WM_QUERYENDSESSION, wParam, lParam);
	}

	virtual LRESULT _OnNotify(WPARAM wParam, LPARAM lParam)
	{
		NMHDR* pNMhdr = (NMHDR*)lParam;
		if (pNMhdr->code == UDM_TOOLTIP_NEED_TT)
		{
			NM_PPTOOLTIP_NEED_TT* pNeedTT = (NM_PPTOOLTIP_NEED_TT*)pNMhdr;
			return _OnNotify_NeedTooltip(pNeedTT);
		}
		return ::DefWindowProc(m_hWnd, WM_NOTIFY, wParam, lParam);
	}

	virtual LRESULT _OnGetMinMaxInfo(WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProc(m_hWnd, WM_GETMINMAXINFO, wParam, lParam);
	}

	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender)
	{
		return 0;
	}

	virtual LRESULT _OnSysCommand(WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProc(m_hWnd, WM_SYSCOMMAND, wParam, lParam);
	}

	virtual LRESULT _OnDropFiles(WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProc(m_hWnd, WM_DROPFILES, wParam, lParam);
	}

	virtual LRESULT _OnShowWindow(WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProc(m_hWnd, WM_SHOWWINDOW, wParam, lParam);
	}

	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam)
	{
		//HWND hWnd = (HWND)wParam;
		//UINT nHitTest = LOWORD(lParam);
		//UINT message = HIWORD(lParam);
		return ::DefWindowProc(m_hWnd, WM_SETCURSOR, wParam, lParam);
	}

	virtual LRESULT _OnSetFocus(HWND hOldWnd)
	{
		Redraw();
		return 0;
	}

	virtual LRESULT _OnKillFocus(HWND hNewWnd)
	{
		Redraw();
		return 0;
	}

	virtual LRESULT _OnCancelMode()
	{
		// Need to handle WM_CANCELMODE message.
		// See -> http://support.microsoft.com/kb/74548/en-us

		m_isTracking = m_isHovering = false;
		Redraw();
		return ::DefWindowProc(m_hWnd, WM_CANCELMODE, 0, 0);
	}

	virtual LRESULT _OnEnableWindow(bool isEnabled)
	{
		Redraw();
		return 0;
	}

	virtual LRESULT _OnSetText(WPARAM wParam, LPARAM lParam)
	{
		if (m_style.WS().hasCaption() && !::IsIconic(m_hWnd))
		{
			// Turn WS_VISIBLE style off before calling Windows to
			// set the text, then turn it back on again after.
			//DWORD dwStyle = m_pWnd->GetStyle();
			DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
			if (dwStyle & WS_VISIBLE)
				::SetWindowLong(m_hWnd, GWL_STYLE, dwStyle & ~WS_VISIBLE);
			::DefWindowProc(m_hWnd, WM_SETTEXT, wParam, lParam);
			if (dwStyle & WS_VISIBLE)
				::SetWindowLong(m_hWnd, GWL_STYLE, dwStyle);

			// Repaint NC area to show changed title text.
			::SendMessage(m_hWnd, WM_NCPAINT, 0, 0);	// paint non-client area (frame too)
			return TRUE;	// Message handled.
		}
		LRESULT result = ::DefWindowProc(m_hWnd, WM_SETTEXT, wParam, lParam);
		Redraw();
		return result;
	}

	// Note - derived class should return 0 if message processed,
	// otherwise MUST call base class.
	virtual LRESULT _OnTimer(WPARAM wParam, LPARAM lParam)
	{
		//UINT_PTR nIDEvent = wParam;
		return ::DefWindowProc(m_hWnd, WM_TIMER, wParam, lParam);
	}

	virtual LRESULT _OnMouseActivate(WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProc(m_hWnd, WM_MOUSEACTIVATE, wParam, lParam);
	}

	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point)
	{
		if (!m_isTracking)
		{
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.hwndTrack = m_hWnd;
			tme.dwFlags = TME_LEAVE | TME_HOVER;
			tme.dwHoverTime = 1;
			m_isTracking = TrackMouseEvent(&tme);
		}
		return 0;
	}

	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam)
	{
		m_isTracking = m_isHovering = false;
		if (m_isHideTooltipOnMouseLeave)
		{
			_HideTooltip();
		}
		return 0;
	}

	virtual LRESULT _OnMouseHover(WPARAM wparam, LPARAM lparam)
	{
		m_isHovering = true;
		return 0;
	}

	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnRightDown(UINT nFlags, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnRightUp(UINT nFlags, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnRightDoubleClick(UINT nFlags, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnMiddleDown(UINT nFlags, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnMiddleUp(UINT nFlags, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnMiddleDoubleClick(UINT nFlags, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnMouseHWheel(WORD fwKeys, short zDelta, CPoint point)
	{
		return 0;
	}

	virtual LRESULT _OnSysKeyDown(WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}
	virtual LRESULT _OnSysKeyUp(WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}
	virtual LRESULT _OnKeyDown(WPARAM wParam, LPARAM lParam)
	{
		//UINT nChar = (UINT)wParam;
		//UINT nRepCnt = (UINT)lParam & 0xFFFF;
		//UINT nFlags = (UINT)(lParam >> 16) & 0xFFFF;
		return 0;
	}
	virtual LRESULT _OnKeyUp(WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}
	virtual LRESULT _OnChar(WPARAM wParam, LPARAM lParam)
	{
		//UINT nChar = (UINT)wParam;
		return 0;
	}
	virtual LRESULT _OnVScroll(WPARAM wParam, LPARAM lParam)
	{
		//UINT nSBCode = LOWORD(wParam);
		//UINT nPos = HIWORD(wParam);
		//HWND hSBwnd = (HWND)lParam;
		return 0;
	}
	virtual LRESULT _OnHScroll(WPARAM wParam, LPARAM lParam)
	{
		//UINT nSBCode = LOWORD(wParam);
		//UINT nPos = HIWORD(wParam);
		//HWND hSBwnd = (HWND)lParam;
		return 0;
	}
	virtual LRESULT _OnGetDlgCode(WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProc(m_hWnd, WM_GETDLGCODE, wParam, lParam);
	}

	virtual LRESULT _OnContextMenu(WPARAM wParam, LPARAM lParam)
	{
		//HWND hWndSource = (HWND)wParam;
		//CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return ::DefWindowProc(m_hWnd, WM_CONTEXTMENU, wParam, lParam);
	}

	virtual LRESULT _OnNotify_NeedTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT)
	{
		return 0;
	}
#pragma endregion MessageHandlers

#pragma region Helpers
public:
	void Redraw() const
	{
		if (m_hWnd)
		{
			::InvalidateRect(m_hWnd, 0, FALSE);
		}
	}

protected:
	CRect _GetClientRect() const
	{
		CRect rcClient;
		XeASSERT(::IsWindow(m_hWnd));
		::GetClientRect(m_hWnd, &rcClient);
		return rcClient;
	}

	void _TakeFocusAndRedraw() const
	{
		if (::GetFocus() != m_hWnd)
		{
			::SetFocus(m_hWnd);	// Note - CXeD2DWndBase class will call Redraw() in _OnSetFocus.
		}
		else
		{
			Redraw();	// Already have focus - only redraw needed.
		}
	}

	void _HideTooltip()
	{
		if (m_xtooltip)
		{
			m_xtooltip->HideTooltip();
		}
	}
#pragma endregion Helpers

#pragma region MFC_Functions_Replacements
public:
	LRESULT SendMessageW(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::SendMessageW(m_hWnd, message, wParam, lParam);
	}

	void ModifyStyle(DWORD dwRemove, DWORD dwAdd)
	{
		XeASSERT(m_hWnd != NULL);
		DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
		DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
		if (dwStyle == dwNewStyle)
			return;

		::SetWindowLong(m_hWnd, GWL_STYLE, dwNewStyle);
	}

	void MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE)
	{
		XeASSERT(::IsWindow(m_hWnd));
		::MoveWindow(m_hWnd, x, y, nWidth, nHeight, bRepaint);
	}

	void MoveWindow(LPCRECT lpRect, BOOL bRepaint = TRUE)
	{
		XeASSERT(::IsWindow(m_hWnd));
		::MoveWindow(m_hWnd, lpRect->left, lpRect->top, (lpRect->right - lpRect->left),
				(lpRect->bottom - lpRect->top), bRepaint);
	}

	BOOL SetWindowPos(HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags)
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::SetWindowPos(m_hWnd, hWndInsertAfter, x, y, cx, cy, nFlags);
	}

	BOOL SetWindowPos(HWND hWndInsertAfter, const CRect& rect, UINT nFlags)
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::SetWindowPos(m_hWnd, hWndInsertAfter, rect.left, rect.top, rect.Width(), rect.Height(), nFlags);
	}

	BOOL ShowWindow(int nCmdShow)
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::ShowWindow(m_hWnd, nCmdShow);
	}

	BOOL IsWindowVisible() const
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::IsWindowVisible(m_hWnd);
	}

	BOOL IsWindowEnabled() const
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::IsWindowEnabled(m_hWnd);
	}

	BOOL EnableWindow(BOOL isEnable = TRUE)
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::EnableWindow(m_hWnd, isEnable);
	}

	int GetDlgCtrlID() const
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::GetDlgCtrlID(m_hWnd);
	}

	HWND GetDlgItem(int nID) const
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::GetDlgItem(m_hWnd, nID);
	}

	void SetWindowText(const std::wstring& str)
	{
		XeASSERT(::IsWindow(m_hWnd));
		::SetWindowTextW(m_hWnd, str.c_str());
	}

	void GetWindowRect(LPRECT lpRect) const
	{
		XeASSERT(::IsWindow(m_hWnd));
		::GetWindowRect(m_hWnd, lpRect);
	}
	void GetClientRect(LPRECT lpRect) const
	{
		XeASSERT(::IsWindow(m_hWnd));
		::GetClientRect(m_hWnd, lpRect);
	}
	void ClientToScreen(LPPOINT lpPoint) const
	{
		XeASSERT(::IsWindow(m_hWnd));
		::ClientToScreen(m_hWnd, lpPoint);
	}
	void ClientToScreen(LPRECT lpRect) const
	{
		XeASSERT(::IsWindow(m_hWnd));
		::ClientToScreen(m_hWnd, (LPPOINT)lpRect);
		::ClientToScreen(m_hWnd, ((LPPOINT)lpRect) + 1);
	}
	void ScreenToClient(LPPOINT lpPoint) const
	{
		XeASSERT(::IsWindow(m_hWnd));
		::ScreenToClient(m_hWnd, lpPoint);
	}
	void ScreenToClient(LPRECT lpRect) const
	{
		XeASSERT(::IsWindow(m_hWnd));
		::ScreenToClient(m_hWnd, (LPPOINT)lpRect);
		::ScreenToClient(m_hWnd, ((LPPOINT)lpRect) + 1);
	}
	void Invalidate(BOOL bErase = TRUE)
	{
		XeASSERT(::IsWindow(m_hWnd));
		::InvalidateRect(m_hWnd, NULL, bErase);
	}
	void InvalidateRect(LPCRECT lpRect, BOOL bErase = FALSE)
	{
		XeASSERT(::IsWindow(m_hWnd));
		::InvalidateRect(m_hWnd, lpRect, bErase);
	}
	void ValidateRect(LPCRECT lpRect)
	{
		XeASSERT(::IsWindow(m_hWnd));
		::ValidateRect(m_hWnd, lpRect);
	}
	BOOL RedrawWindow(LPCRECT lpRectUpdate = NULL,
		HRGN hrgnUpdate = NULL,
		UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE)
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::RedrawWindow(m_hWnd, lpRectUpdate, hrgnUpdate, flags);
	}
	void UpdateWindow()
	{
		XeASSERT(::IsWindow(m_hWnd));
		::InvalidateRect(m_hWnd, nullptr, FALSE);
	}
	HWND SetCapture()
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::SetCapture(m_hWnd);
	}
	HWND SetFocus()
	{
		return ::SetFocus(m_hWnd);
	}
	UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT nElapse,
		void (CALLBACK* lpfnTimer)(HWND, UINT, UINT_PTR, DWORD))
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::SetTimer(m_hWnd, nIDEvent, nElapse, lpfnTimer);
	}
	BOOL KillTimer(UINT_PTR nIDEvent)
	{
		XeASSERT(::IsWindow(m_hWnd));
		return ::KillTimer(m_hWnd, nIDEvent);
	}

	WINDOWPLACEMENT GetCurrentWindowPlacement()
	{
		WINDOWPLACEMENT	wp;
		::ZeroMemory(&wp, sizeof(wp));
		wp.length = sizeof wp;
		::GetWindowPlacement(m_hWnd, &wp);
		return wp;
	}
#pragma endregion MFC_Functions_Replacements

#pragma region Create_UI_resources
protected:
	// Called when paint for the first time.
	// Derived class should override, call base, then create any additional resources it needs for painting.
	virtual void _CreatePaintResources(ID2D1RenderTarget* pRT)
	{
	}
#pragma endregion Create_UI_resources

#pragma region NC_UI_Drawing
private:
	void _DoNcPaint()
	{
		HDC hDC = ::GetWindowDC(m_hWnd);
		HRESULT hr = S_OK;
		Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> renderTargetDC;
		hr = m_xeUI->D2D_CreateDCRenderTarget(renderTargetDC.GetAddressOf());
		XeASSERT(hr == S_OK);
		CRect rcWnd = m_style.GetRects().GetRectTL00(HT_WINDOW);
		//CRect rcWnd(0, 0, m_style.m_rcWnd.Width(), m_style.m_rcWnd.Height());
		if (SUCCEEDED(hr) && rcWnd.Width() && rcWnd.Height())
		{
			hr = renderTargetDC->BindDC(hDC, &rcWnd);
			XeASSERT(hr == S_OK);
			if (SUCCEEDED(hr))
			{
				renderTargetDC->BeginDraw();
				_OnPrepareForPainting(renderTargetDC.Get());
				_NCPaintF(renderTargetDC.Get(), RectFfromRect(rcWnd));
				_OnEndPainting();
				renderTargetDC->EndDraw();
			}
		}
		::ReleaseDC(m_hWnd, hDC);
	}

protected:
	virtual void _NCPaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc)
	{
		const XeWindowStyleRects& ncR = m_style.GetRects();
		if (m_style.WS().hasCaption())
		{
			//-- Paint caption bg (incl. top border) using bitmap pattern.
			D2D1_RECT_F rcCaption = ncR.GetRectTL00F(HTCAPTION);
			D2D1_RECT_F rcTL = ncR.GetRectTL00F(HTTOPLEFT);
			D2D1_RECT_F rcTR = ncR.GetRectTL00F(HTTOPRIGHT);

			// Offset Icon and buttons by border width,height when window maximized.
			int cxySysBorder = ncR.GetCurrentBorderThickness();
			CSize sizeOfs(cxySysBorder, cxySysBorder);

			if (!m_style.WS().isMaximized())
			{	// Paint borders when 'normal' window - i.e. not maximized.
				sizeOfs.cx = 0;	// No offset needed when window 'normal'.
				sizeOfs.cy = 0;
			}
			rcCaption.left = rcTL.left;
			rcCaption.right = rcTR.right;
			rcCaption.top = rcTL.top;
			pRT->FillRectangle(rcCaption, GetBrush(CID::CtrlBorder));

			D2D1_RECT_F rcIcon = ncR.GetRectTL00F(HTSYSMENU);
			D2D1_RECT_F rcMinZe = ncR.GetRectTL00F(HTMINBUTTON);
			D2D1_RECT_F rcMaxZe = ncR.GetRectTL00F(HTMAXBUTTON);
			D2D1_RECT_F rcClose = ncR.GetRectTL00F(HTCLOSE);
			D2D1_RECT_F rcTitle = rcCaption;
			rcTitle.top = rcTitle.bottom - (float)ncR.GetRect(HTCAPTION).Height();
			if (m_style.WS().hasIcon())
			{
				rcTitle.left = rcIcon.right + 6;
			}
			else
			{
				rcTitle.left += 6;
			}
			if (WidthOf(rcMinZe) > 0)
			{
				rcTitle.right = rcMinZe.left - 4;
			}
			else if (WidthOf(rcMaxZe) > 0)
			{
				rcTitle.right = rcMaxZe.left - 4;
			}
			else if (WidthOf(rcClose) > 0)
			{
				rcTitle.right = rcClose.left - 4;
			}

			rcTitle.left += _NCPaintToolbar();

			//-- Paint Title text
			if (m_style.WS().isMaximized()) { rcTitle.left += 4; }
			if (rcTitle.right > rcTitle.left)
			{
				std::wstring sTitle = GetWindowTextStdW(m_hWnd);
				pRT->PushAxisAlignedClip(rcTitle, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
				_CreateAndDrawTextLayout(pRT, sTitle, rcTitle,
					(m_bActive ? CID::ActWndTitleTxt : CID::WndTitleTxt), EXE_FONT::eUI_FontBold);
				pRT->PopAxisAlignedClip();
			}

			//-- Draw Icon and buttons into memory DC - note icon has no 'hot' or 'down' image.
			if (m_style.WS().hasIcon())
			{
				if (!m_d2d_icon_bitmap)
				{
					m_d2d_icon_bitmap = _CreateFromHicon(m_xeUI->GetAppIconHandle());
				}
				if (m_d2d_icon_bitmap)
				{
					float cyIcon = HeightOf(rcIcon);
					float cyIcnOfs = (HeightOf(rcTitle) - cyIcon) / 2;
					rcIcon.top = rcTitle.top + cyIcnOfs;
					rcIcon.bottom = rcIcon.top + cyIcon;
					_DrawIcon(pRT, rcIcon, m_d2d_icon_bitmap.Get());
				}
			}

			PID uID_MinZe = m_style.GetMinimizeBtnImg();
			_DrawButtonImage(pRT, uID_MinZe, rcMinZe, false, true, false);

			PID uID_MaxZe = m_style.GetMaximizeBtnImg();
			_DrawButtonImage(pRT, uID_MaxZe, rcMaxZe, false, true, false);

			PID uID_Close = m_style.GetCloseBtnImg();
			_DrawButtonImage(pRT, uID_Close, rcClose, false, true, false);
		}

		//-- Paint left, right and bottom borders if needed.
		float x1, y1, x1a, y1a, x2, y2;
		if (!m_style.WS().isMaximized())
		{	// Only paint borders when 'normal' window - i.e. not maximized.
			D2D1_RECT_F rcCaption = ncR.GetRectTL00F(HTCAPTION);
			D2D1_RECT_F rcT = ncR.GetRectTL00F(HTTOP);
			D2D1_RECT_F rcL = ncR.GetRectTL00F(HTLEFT);
			rcL.top = rcCaption.bottom;
			D2D1_RECT_F rcR = ncR.GetRectTL00F(HTRIGHT);
			rcR.top = rcCaption.bottom;
			D2D1_RECT_F rcB = ncR.GetRectTL00F(HTBOTTOM);
			rcB.left = ncR.GetRectTL00F(HTBOTTOMLEFT).left;
			rcB.right = ncR.GetRectTL00F(HTBOTTOMRIGHT).right;
			pRT->FillRectangle(rcL, GetBrush(CID::CtrlBorder));
			pRT->FillRectangle(rcR, GetBrush(CID::CtrlBorder));
			pRT->FillRectangle(rcB, GetBrush(CID::CtrlBorder));
			x1 = rcL.left;
			y1 = rcT.top;
			int cxyBorder = ncR.GetCurrentBorderThickness();
			x1a = x1 + cxyBorder + 1;
			y1a = y1 + cxyBorder + 1;
			//x1a = x1 + m_cxyBorder + 1;
			//y1a = y1 + m_cxyBorder + 1;
			x2 = rcR.right - 0.5f;
			y2 = rcB.bottom - 0.5f;
		}

		// draw outline when 'this' window is not maximized.
		if (m_style.WS().hasCaption() && !m_style.WS().isMaximized())
		{
			ID2D1SolidColorBrush* pBrsh = GetBrush(m_bActive ? CID::ActWndBorderPen : CID::WndBorderPen);
			pRT->DrawLine(D2D1_POINT_2F(x1, y1a), D2D1_POINT_2F(x1a, y1), pBrsh, 1.0f);
			pRT->DrawLine(D2D1_POINT_2F(x1a, y1), D2D1_POINT_2F(x2, y1), pBrsh, 1.0f);
			pRT->DrawLine(D2D1_POINT_2F(x2, y1), D2D1_POINT_2F(x2, y2), pBrsh, 1.0f);
			pRT->DrawLine(D2D1_POINT_2F(x2, y2), D2D1_POINT_2F(x1, y2), pBrsh, 1.0f);
			pRT->DrawLine(D2D1_POINT_2F(x1, y2), D2D1_POINT_2F(x1, y1a), pBrsh, 1.0f);
		}
	}
#pragma endregion NC_UI_Drawing

#pragma region UI_Drawing
public:
	void RedrawDirectly()
	{
		_RedrawDirectly();
	}

private:
	void _CreateClientDCRenderTargetAndBeginDraw(HDC dc, const CRect& rc)
	{
		XeASSERT(dc);
		HRESULT hr = S_OK;
		if (!m_renderTargetClientDC)
		{
			hr = m_xeUI->D2D_CreateDCRenderTarget(m_renderTargetClientDC.GetAddressOf());
			XeASSERT(hr == S_OK);
		}
		hr = m_renderTargetClientDC->BindDC(dc, &rc);
		XeASSERT(hr == S_OK);
		if (SUCCEEDED(hr))
		{
			m_renderTargetClientDC->BeginDraw();
			if (!m_uiResourcesCreatedDC)
			{
				_CreatePaintResources(m_renderTargetClientDC.Get());
				m_uiResourcesCreatedDC = true;
			}
		}
	}

	void _EndDCRenderTargetDraw()
	{
		HRESULT hr = m_renderTargetClientDC->EndDraw();
		XeASSERT(hr == S_OK || hr == D2DERR_RECREATE_TARGET);
		if (D2DERR_RECREATE_TARGET == hr)
		{
			m_renderTargetClientDC.Reset();
		}
	}

protected:
	// Get 'this' window client area DC, create D2D render target and call begin draw.
	// Note - caller MUST call _EndWndDC_Draw when finished drawing.
	ID2D1RenderTarget* _GetClientDCandBeginDraw()
	{
		XeASSERT(m_hWindowDC == 0);
		CRect rc;
		::GetClientRect(m_hWnd, &rc);
		m_hClientDC = ::GetDC(m_hWnd);
		XeASSERT(m_hClientDC);
		if (m_hClientDC)
		{
			_CreateClientDCRenderTargetAndBeginDraw(m_hClientDC, rc);
			_OnPrepareForPainting(m_renderTargetClientDC.Get());
			return m_renderTargetClientDC.Get();
		}
		return nullptr;
	}

	// End window DC drawing.
	// Call after calling _GetClientDCandBeginDraw.
	void _EndClientDC_Draw()
	{
		XeASSERT(m_hClientDC != 0);
		if (m_hClientDC)
		{
			_EndDCRenderTargetDraw();
			_OnEndPainting();
			::ReleaseDC(m_hWnd, m_hClientDC);
			m_hClientDC = 0;
		}
	}

	// Redraw 'this' window (using client DC)
	// See: https://www.codeproject.com/articles/Guide-to-WIN32-Paint-for-Intermediates
	void _RedrawDirectly()
	{
		if (::IsWindow(m_hWnd))
		{
			CRect rc;
			::GetClientRect(m_hWnd, &rc);
			HDC dc = ::GetDC(m_hWnd);
			XeASSERT(dc);
			if (dc)
			{
				_CreateClientDCRenderTargetAndBeginDraw(dc, rc);
				_PaintU(m_renderTargetClientDC.Get(), rc);
				_EndDCRenderTargetDraw();
				::ReleaseDC(m_hWnd, dc);
			}
		}
	}

	// WM_PRINT and WM_PRINTCLIENT message handler.
	// Message received when AnimateWindow is called.
	virtual LRESULT _OnPrintClient(HWND hh, HDC hDC)
	{
		CRect rc;
		::GetClientRect(hh, &rc);
		_CreateClientDCRenderTargetAndBeginDraw(hDC, rc);
		_PaintU(m_renderTargetClientDC.Get(), rc);
		_EndDCRenderTargetDraw();
		return 0;
	}

	virtual LRESULT _OnWmPaint(HWND hh)
	{
		// Ignore all WM_PAINT messages - we draw everything directly.
		_RedrawDirectly();

		::ValidateRect(hh, nullptr);
		return 0;
	}
#pragma endregion UI_Drawing
};

export class CXeD2DCtrlBase : public CXeD2DWndBase
{
protected:
	std::wstring m_tooltip_string;

	HWND CreateD2DCtrl(DWORD dwExStyle, const std::wstring& class_name, const wchar_t* pTitle, DWORD dwStyle,
		const CRect& rect, HWND hParentWnd, UINT uID, const wchar_t* tooltip)
	{
		m_tooltip_string = tooltip != nullptr ? tooltip : LoadResourceString(uID);
		bool isEnableTooltip = m_tooltip_string.size() > 0;
		dwStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		return CreateD2DWindow(dwExStyle, class_name, pTitle, dwStyle,
			rect, hParentWnd, uID, isEnableTooltip);
	}

public:
	CXeD2DCtrlBase(CXeUIcolorsIF* pUIcolors) : CXeD2DWndBase(pUIcolors) {}

#pragma region Tooltip
protected:
	virtual LRESULT _OnNotify_NeedTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT) override
	{
		XeASSERT(pNeedTT);
		if (pNeedTT && m_tooltip_string.size())
		{
			CPoint pt(*pNeedTT->pt);

			pNeedTT->ti->sTooltip = m_tooltip_string;
			pNeedTT->ti->hTWnd = GetSafeHwnd();
			pNeedTT->ti->ptTipOffset.x = pt.x;
			return 1;
		}
		return 0;
	}
#pragma endregion Tooltip

#pragma region Misc
protected:
	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override
	{
		LRESULT result = CXeD2DWndBase::_OnMouseLeave(wparam, lparam);
		::InvalidateRect(Hwnd(), nullptr, true);
		return result;
	}

	virtual LRESULT _OnMouseHover(WPARAM wparam, LPARAM lparam) override
	{
		LRESULT result = CXeD2DWndBase::_OnMouseHover(wparam, lparam);
		::InvalidateRect(Hwnd(), nullptr, true);
		return result;
	}
#pragma endregion Misc
};

export LRESULT CALLBACK D2DCtrl_WndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	CXeD2DWndBase* pThis = nullptr;
	if (uiMsg == WM_NCCREATE)
	{
		CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
		pThis = (CXeD2DWndBase*)pCS->lpCreateParams;
		XeASSERT(pThis);	// 'this' pointer is passed to 'us' as lpParam in CreateWindowExW.
		s_xeUI->SetHwndD2DBasePtr(hwnd, pThis, pCS->lpszClass);
		pThis->SetHwnd(hwnd);
		pThis->m_style.Set(hwnd, pCS->style, pCS->dwExStyle, s_xeUI);
	}
	else if (uiMsg == WM_GETMINMAXINFO)
	{
		// Note - need special handling of WM_GETMINMAXINFO - because it's the first message
		// sent when the window is popup and resizable (is sent before WM_NCCREATE).
		pThis = (CXeD2DWndBase*)s_xeUI->GetHwndD2DBasePtrIfExists(hwnd);
	}
	else
	{
		pThis = (CXeD2DWndBase*)s_xeUI->GetHwndD2DBasePtr(hwnd);
	}
	if (pThis)
	{
		return pThis->_OnMessageD2DBase(hwnd, uiMsg, wParam, lParam);
	}
	return ::DefWindowProc(hwnd, uiMsg, wParam, lParam);
}

