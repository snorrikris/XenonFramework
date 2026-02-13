module;

#include "os_minimal.h"

export module Xe.WindowStyle;

import Xe.UIcolorsIF;
import Xe.WindowStyleValue;
import Xe.WindowStyleRects;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Helper class - Window style, calculates non-client area UI elements.
// Borders, Caption, Icon, Buttons and Toolbar (if present).
export class XeWindowStyle
{
#pragma region class_data
protected:
	CXeUIcolorsIF* m_xeUI = nullptr;

	HWND m_hWnd = 0;

	XeWindowStyleValue m_WS;

	XeWindowStyleRects m_ncR;

	EIB_HIT m_eIcnBtn_Hot = EIB_HIT::eNoHit;		// Current button or icon 'hot' state.
	EIB_HIT m_eIcnBtn_Dwn = EIB_HIT::eNoHit;		// Current button 'down' state.
	// Note Icon has no 'down' state because menu is shown when
	// left mouse button down. The Min/Max/Close buttons fire
	// on mouse button UP message.

	HRGN m_hRgn = 0;	// Custom window region.
#pragma endregion class_data

public:
	bool IsInitialized() const { return m_hWnd != 0; }

	const XeWindowStyleValue& WS() const { return m_WS; }

	void Set(HWND hWnd, DWORD dwStyle, DWORD dwExStyle, CXeUIcolorsIF* pUIcolors)
	{
		m_xeUI = pUIcolors;
		XeASSERT(m_xeUI);
		m_ncR.SetXeUIptr(m_xeUI);
		m_hWnd = hWnd;
		m_WS.Set(dwStyle, dwExStyle);
		m_ncR.Calculate(m_WS, m_hWnd);
	}

	bool Update()
	{
		XeASSERT(m_hWnd && m_xeUI);
		bool isStyleChanged = m_WS.Update(::GetWindowLongW(m_hWnd, GWL_STYLE), ::GetWindowLongW(m_hWnd, GWL_EXSTYLE));
		bool isWndRectChanged = _IsWindowRectChanged();
		if (isStyleChanged || isWndRectChanged)
		{
			m_ncR.Calculate(m_WS, m_hWnd);
		}
		if (m_WS.hasCaption() && isWndRectChanged)
		{
			_UpdateCustomWindowRegion();
		}
		return isStyleChanged || isWndRectChanged;
	}

	void SetToolbarSize(const CSize& sizeToolbar)
	{
		m_ncR.SetToolbarSize(sizeToolbar);
		m_ncR.Calculate(m_WS, m_hWnd);
	}
	CSize GetToolbarSize() const { return m_ncR.GetToolbarSize(); }

	const XeWindowStyleRects& GetRects() const { return m_ncR; }

protected:
	bool _IsWindowRectChanged() const
	{
		CRect rcWnd;
		::GetWindowRect(m_hWnd, &rcWnd);
		return m_ncR.GetRect(HTNOWHERE) != rcWnd;
	}

#pragma region NC_Helpers
public:
	// WM_NCCALCSIZE helper - calculate window client area.
	void NcCalcSize(NCCALCSIZE_PARAMS* lpncsp)
	{
		/****************************************************************************************
		Note:
		As near as I can figure it's best to imagine the first rect from lpncsp as the 'proposed'
		client area. This rect contains the entire window in screen coords. on entry. On exit it
		should contain the coordinates of the window client area. I.e. if we don't call Default()
		and don't modify the rect then the resulting window will have no borders and no caption.

		I noticed a little bit strange behaviour when the window is maximized. The 'proposed'
		client area was calculated by windows as if the 'size' borders were outside the monitor.
		For example in Windows7 left,top = -8,-8 on entry (thickframe border = 8 in Windows7)
		and right,bottom = 1928,1048, Default() calculates left,top as 0,22 (caption cy=22 in
		Windows7) and right,bottom as 1920,1040.

		Another interesting thing...
		When window is minimized the rect values (Windows7) are left,top = -32000,-32000 and
		right,bottom = -31840,-31973.
		****************************************************************************************/

		// Make a reference to THE rect we need to modify.
		CRect& rcW = (CRect&)lpncsp->rgrc[0];
		m_ncR.CalculateNCsize(rcW, m_WS);
	}

	// Return true if NC paint needed.
	bool NcMouseMove(const CPoint& point, EIB_HIT& eHit)
	{
		eHit = m_ncR.IB_HitTest(point);
		bool isRepaintNC = false;
		if (m_eIcnBtn_Hot != eHit)
		{	// 'hot' status changed.
			m_eIcnBtn_Hot = eHit;
			isRepaintNC = true;
		}
		return isRepaintNC;
	}

	// Return true if NC paint needed.
	bool NcMouseLeave()
	{
		if (m_eIcnBtn_Hot != EIB_HIT::eNoHit || m_eIcnBtn_Dwn != EIB_HIT::eNoHit
				|| m_ncR.HasToolbar())	// Always redraw toolbar after mouse leaves.
		{
			m_eIcnBtn_Hot = m_eIcnBtn_Dwn = EIB_HIT::eNoHit;
			return true;
		}
		return false;
	}

	// Return true if NC paint needed.
	bool NcLButtonDown(const CPoint& point, EIB_HIT& eHit)
	{
		bool isRepaintNC = false;
		eHit = m_ncR.IB_HitTest(point);	// Hit test for Icon or buttons.
		//TRACE("NcLbtn dn eHit: %d\n", eHit);
		if (eHit == EIB_HIT::eIcon)
		{	// L button down on icon (system menu).
			CPoint ptMenu = m_ncR.GetSysMenuPosition();
			LPARAM lParmCoords = MAKELONG(ptMenu.x, ptMenu.y);

			// Display system menu just below icon.
			::SendMessage(m_hWnd, WM_CONTEXTMENU, (WPARAM)m_hWnd, lParmCoords);
		}
		else if (eHit == EIB_HIT::eMinZe || eHit == EIB_HIT::eMaxZe || eHit == EIB_HIT::eClose
				|| eHit == EIB_HIT::eToolBar)
		{	// L button down on a button.
			if (m_eIcnBtn_Dwn != eHit)
			{
				m_eIcnBtn_Dwn = eHit;				// New 'down' state.
				m_eIcnBtn_Hot = EIB_HIT::eNoHit;	// Clear 'hot' state.

				// Repaint NC area to show button 'down' state.
				isRepaintNC = true;
			}
			return TRUE;	// Return TRUE so we will get an UP message.
		}
		return isRepaintNC;
	}

	// Return true if NC paint needed.
	bool NcLButtonUp(CPoint point, EIB_HIT& eHit, WPARAM& wParm)
	{
		bool isRepaintNC = false;
		eHit = m_ncR.IB_HitTest(point);	// Hit test for Icon or buttons.
		wParm = GetWM_SYSCOMMAND(eHit);
		//TRACE("NcLbtn up eHit: %d\n", eHit);
		if (wParm)
		{
			m_eIcnBtn_Dwn = EIB_HIT::eNoHit;
		}

		if (m_eIcnBtn_Dwn != EIB_HIT::eNoHit)	// Repaint NC area if needed.
		{
			m_eIcnBtn_Dwn = EIB_HIT::eNoHit;

			// Repaint NC area to show button 'normal' state.
			isRepaintNC = true;
		}
		return isRepaintNC;
	}

	// Hit test from point (pt) in screen coords.
	// Returns one of HTXXXXX constants.
	// Note - HTSYSMENU, HTMINBUTTON, HTMAXBUTTON or HTCLOSE are never returned
	//        because Windows will paint it's ugly buttons if we do.
	LRESULT NcHitTest(CPoint pt)
	{
		bool isWndRectChanged = _IsWindowRectChanged();
		if (isWndRectChanged)
		{
			m_ncR.Calculate(m_WS, m_hWnd);
		}
		return m_ncR.NcHitTest(pt, m_WS);
	}

	WPARAM GetWM_SYSCOMMAND(EIB_HIT eHit)
	{
		WPARAM wParm = 0;
		if (eHit == EIB_HIT::eMinZe)
		{	// L button up on Minimize button.
			wParm = SC_MINIMIZE;
		}
		else if (eHit == EIB_HIT::eMaxZe)
		{	// L button up on Maximize/Restore button.
			wParm = (m_WS.isMaximized()) ? SC_RESTORE : SC_MAXIMIZE;
		}
		else if (eHit == EIB_HIT::eClose)
		{	// L button up on Close button.
			wParm = SC_CLOSE;
		}
		return wParm;
	}

	void _UpdateCustomWindowRegion()
	{
		// destroy old region
		::DeleteObject(m_hRgn);

		CRect rc = m_ncR.GetRect(HT_WINDOW);
		rc -= rc.TopLeft();        // convert to window coords: top left = (0,0)
		if (m_WS.isNormalWithCaption())	// Window is 'normal' AND has caption?
		{
			// Create Custom Window Frame region.
			int cxyCorner = m_ncR.GetCurrentBorderThickness();
			CPoint ptVert[6];
			ptVert[0].x = rc.left + cxyCorner;
			ptVert[0].y = rc.top;
			ptVert[1].x = rc.right;
			ptVert[1].y = rc.top;
			ptVert[2].x = rc.right;
			ptVert[2].y = rc.bottom;
			ptVert[3].x = rc.left;
			ptVert[3].y = rc.bottom;
			ptVert[4].x = rc.left;
			ptVert[4].y = rc.top + cxyCorner;
			ptVert[5].x = ptVert[0].x;
			ptVert[5].y = ptVert[0].y;
			m_hRgn = ::CreatePolygonRgn(ptVert, 6, ALTERNATE);

			::SetWindowRgn(m_hWnd, m_hRgn, TRUE);  // set window region to make rounded window
		}
		else if (m_WS.isMaximized())
		{	// Apply 'square' region. Exclude invisible borders.
			int cxyBorder = m_ncR.GetCurrentBorderThickness();
			rc.DeflateRect(cxyBorder, cxyBorder, cxyBorder, cxyBorder);
			m_hRgn = ::CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
			::SetWindowRgn(m_hWnd, m_hRgn, TRUE);
		}
		else
		{
			::SetWindowRgn(m_hWnd, NULL, TRUE);
		}
	}

	PID GetMinimizeBtnImg()
	{
		PID uID_MinZe = (m_eIcnBtn_Hot == EIB_HIT::eMinZe)
			? PID::CWF_BTN_MIN_HOT			// 'hot' image ID
			: (m_eIcnBtn_Dwn == EIB_HIT::eMinZe)
			? PID::CWF_BTN_MIN_DN 			// 'down' image ID
			: PID::CWF_BTN_MIN_UP;			// 'up' image ID
		return uID_MinZe;
	}
	PID GetMaximizeBtnImg()
	{
		PID uID_MaxZe = (m_eIcnBtn_Hot == EIB_HIT::eMaxZe) ? PID::CWF_BTN_MAX_HOT
			: (m_eIcnBtn_Dwn == EIB_HIT::eMaxZe) ? PID::CWF_BTN_MAX_DN : PID::CWF_BTN_MAX_UP;
		if (m_WS.isMaximized())
		{	// Show 'restore' button.
			uID_MaxZe = (m_eIcnBtn_Hot == EIB_HIT::eMaxZe) ? PID::CWF_BTN_RST_HOT
				: (m_eIcnBtn_Dwn == EIB_HIT::eMaxZe) ? PID::CWF_BTN_RST_DN : PID::CWF_BTN_RST_UP;
		}
		return uID_MaxZe;
	}
	PID GetCloseBtnImg()
	{
		PID uID_Close = (m_eIcnBtn_Hot == EIB_HIT::eClose) ? PID::CWF_BTN_CLS_HOT
			: (m_eIcnBtn_Dwn == EIB_HIT::eClose) ? PID::CWF_BTN_CLS_DN : PID::CWF_BTN_CLS_UP;
		return uID_Close;
	}
#pragma endregion NC_Helpers
};

