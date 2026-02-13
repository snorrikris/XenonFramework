module;

#include "os_minimal.h"
#include <algorithm>
#include <d2d1.h>

export module Xe.WindowStyleRects;

import Xe.WindowStyleValue;
import Xe.mfc_types;
import Xe.UItypesPID;
import Xe.UIcolorsIF;

export constexpr DWORD HT_TOOLBAR = 101;
export constexpr DWORD HT_WINDOW = HTNOWHERE;

export enum class EIB_HIT
{
	eNoHit = 0,
	eIcon,
	eMinZe,
	eMaxZe,
	eClose,
	eToolBar
};

export class XeWindowStyleRects
{
protected:
	CXeUIcolorsIF* m_xeUI = nullptr;

	CRect m_rcWnd;					// Window rect in screen coords.

	CRect m_rcClient;				// Client rect in screen coords.

	// Border thickness (0 if window does not have border).
	int m_cxyBorder = 0;

	// Calculated rects (from window style) window UI elements.
	// Borders, Caption, Icon, Buttons and Toolbar (if present).

	// Border rects (not including corners) - in screen coords.
	CRect m_rcLborder, m_rcRborder, m_rcTborder, m_rcBborder;

	// Corner rects - in screen coords.
	CRect m_rcTLcorner, m_rcTRcorner, m_rcBLcorner, m_rcBRcorner;

	CRect m_rcCaption;	// Caption rect in screen coords.

	CRect m_rcToolBar;	// ToolBar (if present) rect in screen coords.

	// Icon and button rects in screen coords.
	CRect m_rcIcon;		// Note: width = 0 unless window has WS_SYSMENU style.
	CRect m_rcMinZe;	// Note: width = 0 unless window has WS_MINIMIZEBOX style.
	CRect m_rcMaxZe;	// Note: width = 0 unless window has WS_MAXIMIZEBOX style.
	CRect m_rcClose;

	// Size of toolbar (if present - in window caption area).
	CSize m_sizeToolbar;

public:
	void SetXeUIptr(CXeUIcolorsIF* pUIcolors)
	{
		m_xeUI = pUIcolors;
	}

	void Calculate(const XeWindowStyleValue& style, HWND hWnd)
	{
		XeASSERT(m_xeUI);
		::GetWindowRect(hWnd, &m_rcWnd);
		_CalculateBorderThickness(style, hWnd);
		_CalculateNCrects(style);
	}

	void SetToolbarSize(CSize sizeToolbar)
	{
		m_sizeToolbar = sizeToolbar;
	}
	CSize GetToolbarSize() const { return m_sizeToolbar; }
	bool HasToolbar() const { return m_sizeToolbar.cy; }

	CPoint GetSysMenuPosition() const { return CPoint(m_rcIcon.left, m_rcIcon.bottom); }

	int GetCurrentBorderThickness() const { return m_cxyBorder; }

	void CalculateNCsize(CRect& rcW, const XeWindowStyleValue& style)
	{
		int cxyBorder = m_cxyBorder;
		if (style.hasBorder())
		{
			if (style.isMaximized())
			{	// 'subtract' (system standard width) borders from rect when maximized.
				rcW.left += cxyBorder;
				rcW.right -= cxyBorder;
				rcW.top += cxyBorder;
				rcW.bottom -= cxyBorder;
			}
			else if (!style.isMinimized())
			{	// 'subtract' our custom borders from rect when 'normal' window 
				// (i.e. not maximized nor minimized).
				rcW.left += cxyBorder;
				rcW.right -= cxyBorder;
				rcW.top += cxyBorder;
				rcW.bottom -= cxyBorder;
			}
			if (!style.isMinimized()		// 'Normal' or Maximized window?
					&& style.hasCaption())	// AND has caption?
			{
				// 'subtract' (custom) caption height from rect when 'normal' window or maximized.
				rcW.top += m_rcCaption.Height();
			}
		}
	}

	// Hit test from point (pt) in screen coords.
	// Returns one of HTXXXXX constants.
	// Note - HTSYSMENU, HTMINBUTTON, HTMAXBUTTON or HTCLOSE are never returned
	//        because Windows will paint it's ugly buttons if we do.
	// Note also - when HTCAPTION and message is WM_NCLBUTTONDOWN - the WM_NCLBUTTONUP
	// message is never received because the mouse is captured (start of drag window).
	LRESULT NcHitTest(CPoint pt, const XeWindowStyleValue& style)
	{
		LRESULT lResult = HTNOWHERE;
		if (m_rcLborder.PtInRect(pt))
		{
			lResult = HTLEFT;
			if (pt.y >= (m_rcLborder.bottom - 10))	// Make XY resize area bigger.
			{
				lResult = HTBOTTOMLEFT;
			}
		}
		if (m_rcRborder.PtInRect(pt))
		{
			lResult = HTRIGHT;
			if (pt.y < (m_rcRborder.top + 10))	// Make XY resize area bigger.
			{
				lResult = HTTOPRIGHT;
			}
			else if (pt.y >= (m_rcRborder.bottom - 10))	// Make XY resize area bigger.
			{
				lResult = HTBOTTOMRIGHT;
			}
		}
		if (m_rcTborder.PtInRect(pt))
		{
			lResult = HTTOP;

			// The top border above the buttons is DMZ.
			if (pt.x >= m_rcMinZe.left && pt.x < m_rcClose.right)
			{
				lResult = HTOBJECT;
			}
			if (pt.x >= (m_rcTborder.right - 10))	// Make XY resize area bigger.
			{
				lResult = HTTOPRIGHT;
			}
		}
		if (m_rcBborder.PtInRect(pt))
		{
			lResult = HTBOTTOM;
			if (pt.x < (m_rcBborder.left + 10))	// Make XY resize area bigger.
			{
				lResult = HTBOTTOMLEFT;
			}
			else if (pt.x >= (m_rcBborder.right - 10))	// Make XY resize area bigger.
			{
				lResult = HTBOTTOMRIGHT;
			}
		}
		if (m_rcTLcorner.PtInRect(pt))
		{
			lResult = HTTOPLEFT;
		}
		else if (m_rcTRcorner.PtInRect(pt))
		{
			lResult = HTTOPRIGHT;
		}
		else if (m_rcBLcorner.PtInRect(pt))
		{
			lResult = HTBOTTOMLEFT;
		}
		else if (m_rcBRcorner.PtInRect(pt))
		{
			lResult = HTBOTTOMRIGHT;
		}
		else if (m_rcToolBar.PtInRect(pt))
		{
			lResult = HTOBJECT;
		}
		else if (m_rcIcon.PtInRect(pt))
		{
			lResult = HTOBJECT;
		}
		else if (m_rcCaption.PtInRect(pt))
		{
			lResult = HTCAPTION;
			int cxyCorner = m_cxyBorder;
			if (pt.x < (m_rcCaption.left + cxyCorner))	// Make XY resize area bigger.
			{
				lResult = HTTOPLEFT;
			}
		}
		if (!style.hasThickBorder()
				&& (lResult >= HTLEFT && lResult <= HTBOTTOMRIGHT))
		{	// Only return 'size' hit test values when window has thickframe border.
			lResult = HTNOWHERE;
		}
		if (m_rcClient.PtInRect(pt))
		{
			lResult = HTCLIENT;
		}
		return lResult;
	}

	// Hit test for Icon or buttons.
	EIB_HIT IB_HitTest(CPoint point)
	{
		if (m_rcToolBar.PtInRect(point))
		{
			return EIB_HIT::eToolBar;
		}
		else if (m_rcIcon.PtInRect(point))
		{
			return EIB_HIT::eIcon;
		}
		else if (m_rcMinZe.PtInRect(point))
		{
			return EIB_HIT::eMinZe;
		}
		else if (m_rcMaxZe.PtInRect(point))
		{
			return EIB_HIT::eMaxZe;
		}
		else if (m_rcClose.PtInRect(point))
		{
			return EIB_HIT::eClose;
		}
		return EIB_HIT::eNoHit;
	}

	// Get rect - screen coordinates.
	// dwSel = one of HTLEFT, HTRIGHT, HTTOP, HTBOTTOM, 
	//                HTTOPLEFT, HTTOPRIGHT, HTBOTTOMLEFT, HTBOTTOMRIGHT,
	//                HTCAPTION, HTSYSMENU, HTMINBUTTON, HTMAXBUTTON, HTCLOSE.
	//                HTNOWHERE or HT_WINDOW return window rect.
	const CRect& GetRect(DWORD dwSel) const
	{
		switch (dwSel)
		{
		case HTLEFT:		return m_rcLborder;
		case HTRIGHT:		return m_rcRborder;
		case HTTOP:			return m_rcTborder;
		case HTBOTTOM:		return m_rcBborder;
		case HTTOPLEFT:		return m_rcTLcorner;
		case HTTOPRIGHT:	return m_rcTRcorner;
		case HTBOTTOMLEFT:	return m_rcBLcorner;
		case HTBOTTOMRIGHT:	return m_rcBRcorner;
		case HTCAPTION:		return m_rcCaption;
		case HTSYSMENU:		return m_rcIcon;
		case HTMINBUTTON:	return m_rcMinZe;
		case HTMAXBUTTON:	return m_rcMaxZe;
		case HTCLOSE:		return m_rcClose;
		case HT_TOOLBAR:	return m_rcToolBar;
		case HTCLIENT:		return m_rcClient;
		default:			return m_rcWnd;
		}
	}

	// Get rect - relative to TL = 0,0.
	// dwSel = one of HTLEFT, HTRIGHT, HTTOP, HTBOTTOM, 
	//                HTTOPLEFT, HTTOPRIGHT, HTBOTTOMLEFT, HTBOTTOMRIGHT,
	//                HTCAPTION, HTSYSMENU, HTMINBUTTON, HTMAXBUTTON, HTCLOSE.
	CRect GetRectTL00(DWORD dwSel) const
	{	// Get rect - relative to TL = 0,0. dwSel is one of HTXXX.
		CRect rc = GetRect(dwSel);
		rc -= m_rcWnd.TopLeft();	// make rect relative to TL = 0,0.
		return rc;
	}

	D2D1_RECT_F GetRectTL00F(DWORD dwSel) const
	{
		CRect rect = GetRectTL00(dwSel);
		D2D1_RECT_F rcF;
		rcF.left = (float)rect.left;
		rcF.right = (float)rect.right;
		rcF.top = (float)rect.top;
		rcF.bottom = (float)rect.bottom;
		return rcF;
	}

protected:
	void _CalculateBorderThickness(const XeWindowStyleValue& style, HWND hWnd)
	{
		m_cxyBorder = 0;
		if (style.isMaximized())
		{
			// Calculate thickness of invisible border of maximized window.
			int nBrd = 0;
			HMONITOR hmon = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof(mi) };
			if (GetMonitorInfo(hmon, &mi))
			{
				// Border thickness = difference between monitor "work" left coords
				// and window rect left coords.
				nBrd = abs(mi.rcWork.left - m_rcWnd.left);
			}
			if (nBrd == 0 || nBrd > 16)
			{
				nBrd = ::GetSystemMetrics(SM_CXSIZEFRAME);	// fall back value.
			}
			m_cxyBorder = nBrd;
		}
		else if (style.isMinimized())
		{
			m_cxyBorder = 0;
		}
		else if (style.hasThickBorder())
		{
			m_cxyBorder = m_xeUI->GetValue(UIV::cxyThickBorder);
		}
		else if (style.hasBorder())
		{
			m_cxyBorder = m_xeUI->GetValue(UIV::cxyThinBorder);
		}
	}

	void _CalculateNCrects(const XeWindowStyleValue& style)
	{
		_ResetNCrects();

		int cyCaption = 0;
		if (style.hasCaption())
		{
			int cyNeeded = m_xeUI->GetValue(UIV::cyWindowCaption);
			if (m_sizeToolbar.cy)
			{
				cyNeeded = std::max((int)m_sizeToolbar.cy + m_cxyBorder, cyNeeded);
			}

			cyCaption = cyNeeded;
			m_rcCaption.bottom = m_rcCaption.top + cyCaption;

			if (style.hasIcon())
			{
				m_rcIcon = m_rcCaption;
				m_rcIcon.left += m_cxyBorder;
				int cxyIcon = 16;
				m_rcIcon.right = m_rcIcon.left + cxyIcon;
				double icn_ofs = m_rcIcon.Height() - cxyIcon;
				m_rcIcon.top += (int)(icn_ofs / 2 + 0.5);
				m_rcIcon.bottom = m_rcIcon.top + cxyIcon;
			}
			if (m_sizeToolbar.cy)
			{
				double tb_ofs = m_rcCaption.bottom - m_rcCaption.top - m_sizeToolbar.cy;
				m_rcToolBar.top = m_rcCaption.top + (int)(tb_ofs / 2 + 0.5);
				m_rcToolBar.bottom = m_rcToolBar.top + m_sizeToolbar.cy;
				m_rcToolBar.left = m_rcIcon.right + m_cxyBorder;
				m_rcToolBar.right = m_rcToolBar.left + m_sizeToolbar.cx;
			}

			// Has close button when has caption
			CSize sizeClose = m_xeUI->GetPngImageSize(PID::CWF_BTN_CLS_UP);
			m_rcClose = m_rcCaption;
			m_rcClose.right -= 2;								// Close button right margin.
			m_rcClose.left = m_rcClose.right - sizeClose.cx;	// Close button width.
			m_rcClose.top += (cyCaption - sizeClose.cy) / 2;
			m_rcClose.bottom = m_rcClose.top + sizeClose.cy;	// Close button height.

			if (style.hasMaximizeBtn())
			{
				CSize sizeMaxZe = m_xeUI->GetPngImageSize(PID::CWF_BTN_MAX_UP);
				m_rcMaxZe = m_rcClose;
				m_rcMaxZe.right = m_rcMaxZe.left;
				m_rcMaxZe.right -= 2;								// Margin.
				m_rcMaxZe.left = m_rcMaxZe.right - sizeMaxZe.cx;	// Button width.
			}
			if (style.hasMinimizeBtn())
			{
				CSize sizeMinZe = m_xeUI->GetPngImageSize(PID::CWF_BTN_MIN_UP);
				m_rcMinZe = m_rcMaxZe;
				m_rcMinZe.right = m_rcMinZe.left;
				m_rcMinZe.right -= 2;								// Margin.
				m_rcMinZe.left = m_rcMinZe.right - sizeMinZe.cx;	// Button width.
			}
			if (!style.isMaximized())
			{	// Need to compensate for borders in the caption when 'normal' window.
				int yOfs = -(m_cxyBorder / 2);
				m_rcIcon.OffsetRect(0, yOfs);
				m_rcMinZe.OffsetRect(0, yOfs);
				m_rcMaxZe.OffsetRect(0, yOfs);
				m_rcClose.OffsetRect(0, yOfs);
			}
		}
		m_rcClient.SetRect(m_rcLborder.right, m_rcCaption.bottom, m_rcRborder.left, m_rcBborder.top);
	}

	void _ResetNCrects()
	{
		m_rcLborder = m_rcRborder = m_rcTborder = m_rcBborder = m_rcWnd;
		m_rcTLcorner = m_rcTRcorner = m_rcBLcorner = m_rcBRcorner = m_rcCaption = m_rcWnd;

		int cxyBorder = m_cxyBorder;
		m_rcLborder.right = m_rcLborder.left + cxyBorder;
		m_rcLborder.top += cxyBorder;
		m_rcLborder.bottom -= cxyBorder;

		m_rcRborder.left = m_rcRborder.right - cxyBorder;
		m_rcRborder.top += cxyBorder;
		m_rcRborder.bottom -= cxyBorder;

		m_rcTborder.bottom = m_rcTborder.top + cxyBorder;
		m_rcTborder.left += cxyBorder;
		m_rcTborder.right -= cxyBorder;

		m_rcBborder.top = m_rcBborder.bottom - cxyBorder;
		m_rcBborder.left += cxyBorder;
		m_rcBborder.right -= cxyBorder;

		m_rcTLcorner.right = m_rcTLcorner.left + cxyBorder;
		m_rcTLcorner.bottom = m_rcTLcorner.top + cxyBorder;

		m_rcTRcorner.left = m_rcTRcorner.right - cxyBorder;
		m_rcTRcorner.bottom = m_rcTRcorner.top + cxyBorder;

		m_rcBLcorner.right = m_rcBLcorner.left + cxyBorder;
		m_rcBLcorner.top = m_rcBLcorner.bottom - cxyBorder;

		m_rcBRcorner.left = m_rcBRcorner.right - cxyBorder;
		m_rcBRcorner.top = m_rcBRcorner.bottom - cxyBorder;

		m_rcCaption.left += cxyBorder;
		m_rcCaption.right -= cxyBorder;
		m_rcCaption.top += cxyBorder;
		m_rcCaption.bottom = m_rcCaption.top;

		m_rcToolBar.SetRectEmpty();
		m_rcIcon = m_rcMinZe = m_rcMaxZe = m_rcClose = m_rcToolBar;
	}
};

