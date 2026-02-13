module;

#include "os_minimal.h"

export module Xe.PopupCtrl;

import Xe.UIcolorsIF;
export import Xe.PopupCtrlBase;
import Xe.mfc_types;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define XEPOPUPWND_CLASSNAME L"XePopupWndClass"  // Window class name

constexpr DWORD XEPOPUPWND_SIGNATURE = 0x500DFEED;

LRESULT CALLBACK Popup_WndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

export enum class XeShowPopup { Normal, FadeIn80 };

export class CXePopupCtrl
{
protected:
	CXeUIcolorsIF* m_xeUI = nullptr;

	//CWnd* m_pParentWnd = nullptr;
	HWND m_hwndOwner = 0, m_hwndPopup = 0, m_hWndVscroll = 0, m_hWndHscroll = 0;
	CRect m_rcScreenPos, m_rcScreenPosFirst, m_rcParentCtrlScreenPos, m_rcVscroll, m_rcHscroll;
	bool m_hasVscroll = false, m_hasHscroll = false;

	XeShowPopup m_showPopupWindowStyle = XeShowPopup::Normal;

public:
	CXePopupCtrlBase* m_pCtrl = nullptr;

	DWORD m_signature = XEPOPUPWND_SIGNATURE;

	// 'callers' - if showing the popup window using animate call (fade in) is wanted set style to FadeIn80.
	// Note - if the popup has child windows (e.g. scrollbar) then animate window (may) will not work.
	CXePopupCtrl(CXeUIcolorsIF* pUIcolors, CXePopupCtrlBase* pCtrl, XeShowPopup style)
		: m_xeUI(pUIcolors), m_pCtrl(pCtrl), m_showPopupWindowStyle(style)
	{
	}

	CXePopupCtrl(CXeUIcolorsIF* pUIcolors, XeShowPopup style)
		: m_xeUI(pUIcolors), m_showPopupWindowStyle(style)
	{
	}

	void SetPopupPointer(CXePopupCtrlBase* pCtrl)
	{
		m_pCtrl = pCtrl;
		XeASSERT(m_pCtrl);
	}

	HWND GetPopupHwnd() const { return m_pCtrl ? m_pCtrl->GetHwnd() : NULL; }

	// pParentWnd is the parent window of the dropdown
	// rcScreenPos is the rect for the dropdown in screen coords.
	// rcParentCtrlScreenPos is the rect for the parent control in screen coords.
	// the final screen position of the dropdown is decided before it is shown.
	// it can be above or below the parent control and also can be shifted left or right
	// to fit inside the screen boundraries.
	int ShowPopup(HWND hParentWnd, CRect rcScreenPos, CRect rcParentCtrlScreenPos)
	{
		XeASSERT(m_pCtrl && hParentWnd);
		if (!m_pCtrl || !hParentWnd)
		{
			return -1;
		}
		m_xeUI->RegisterWindowClass(XEPOPUPWND_CLASSNAME, Popup_WndProc);
		//_InitData(pParentWnd, rcScreenPos, rcParentCtrlScreenPos);
		m_hwndOwner = hParentWnd;
		m_rcScreenPos = rcScreenPos;
		m_rcParentCtrlScreenPos = rcParentCtrlScreenPos;

		// Early check:  We must be on same thread as the owner so we can see
		// its mouse and keyboard messages when we set capture to it.
		if (::GetCurrentThreadId() != ::GetWindowThreadProcessId(m_hwndOwner, NULL))
		{
			// Error: 'this' must be on same thread as parent window.
			return -1;
		}

		// Call CXePopupCtrlBase derived class to allow it to change the size (or position) of the final window size.
		// In case of listbox type control this will adjust the height if item count is less than height/itemCY
		m_pCtrl->AdjustPopupWindowFinalSize(m_rcScreenPos, m_rcParentCtrlScreenPos);
		m_rcScreenPosFirst = m_rcScreenPos;
		_AdjustWindowPopupFinalPosition();

		m_rcScreenPos.InflateRect(2, 2);	// Adjust final size to include borders

		_CreatePopupWindow();

		// Let the control know that it's in a popup window.
		m_pCtrl->PopupOpening();

		// Show the window but don't activate it!
		if (m_showPopupWindowStyle == XeShowPopup::FadeIn80)
		{
			::AnimateWindow(m_hwndPopup, 80, AW_BLEND/*AW_SLIDE | AW_VER_POSITIVE*/);
		}
		else	// XeShowPopup::Normal
		{
			::ShowWindow(m_hwndPopup, SW_SHOWNOACTIVATE);
		}

		// We want to receive all mouse messages, but since only the active
		// window can capture the mouse, we have to set the capture to our
		// owner window, and then steal the mouse messages out from under it.
		if (m_pCtrl->IsFirstInstance())	// Is 'this' first instance of the control.
		{
			// Capture mouse messages - unless 'this' is a sub-menu (already captured).
			::SetCapture(m_hwndOwner);
		}
		::SetCursor(m_xeUI->GetAppCursor(false));

		_InitScrollBarsData();

		// Go into a message loop that filters all the messages it receives
		// and route the interesting ones to the listbox window.
		MSG msg;
		while (::GetMessageW(&msg, NULL, 0, 0))
		{
			//TRACE("Message - 0x%.4X\n", msg.message);

			// If our owner stopped being the active window (e.g. the user Alt+Tab'd to
			// another window in the meantime), then stop.
			HWND hwndActive = ::GetActiveWindow();
			if (hwndActive != m_hwndOwner && !::IsChild(hwndActive, m_hwndOwner))
			{
				break;
			}

			// If window that has mouse messages captured is not 'this' window, then stop.
			// Unless it's a sub-menu of ours. Then we need to re-capture the mouse messages.
			HWND hwndCapture = ::GetCapture();
			if (hwndCapture != m_hwndOwner)
			{
				break;
			}

			// If user selected item in the 'control' or cancelled, then stop.
			if (m_pCtrl->IsPopupEndCondition())
			{
				//::ShowWindow(m_hwndPopup, SW_HIDE);
				break;
			}

			if (msg.message == NF_POPUP_WND_SIZE_CHANGE)
			{
				_ProcessNotifyPopupWndSizeChangeMessage();
			}

			// At this point, we get to snoop at all input messages before
			// they get dispatched. This allows us to route all input to our
			// popup window even if it really belongs to some other window.

			// All mouse messages are remunged and directed at our popup window.
			// If the mouse message arrives as client coordinates, then
			// we have to convert it from the client coordinates of the original
			// target to the client coordinates of the new target.
			RetargetMessageIfNeeded(msg);

			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		if (m_pCtrl->IsFirstInstance())	// Is 'this' first instance of the control.
		{
			// Clean up the capture we created.
			::ReleaseCapture();
		}

		::DestroyWindow(m_hwndPopup);
		//TRACE("Popup DestroyWindow - 0x%.8X\n", m_hwndPopup);

		m_pCtrl->PopupClosed();

		// If we got a WM_QUIT message, then re-post it so the caller's message
		// loop will see it.
		if (msg.message == WM_QUIT)
		{
			::PostQuitMessage((int)msg.wParam);
		}

		return -1;
	}

	LRESULT OnMessage(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
	{
		XeASSERT(m_pCtrl);
		if (uiMsg == WM_NCCREATE)
		{
			return OnNcCreate(hwnd, uiMsg, wParam, lParam);
		}
		else if (uiMsg == WM_MOUSEACTIVATE)
		{
			return MA_NOACTIVATE;
		}
		return m_pCtrl->ProcessPopupWndMessage(hwnd, uiMsg, wParam, lParam);
	}

	LRESULT OnNcCreate(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
	{
		XeASSERT(m_pCtrl);
		CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
		::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)this);
		m_pCtrl->PopupDropDown(hwnd, pCS->style, pCS->dwExStyle);
		return ::DefWindowProc(hwnd, uiMsg, wParam, lParam);
	}

protected:
	//void _InitData(CWnd* pParentWnd, const CRect& rcScreenPos, const CRect& rcParentCtrlScreenPos)
	//{
	//	m_pParentWnd = pParentWnd;
	//	m_hwndOwner = pParentWnd->GetSafeHwnd();
	//	m_rcScreenPos = rcScreenPos;
	//	m_rcParentCtrlScreenPos = rcParentCtrlScreenPos;
	//}

	void _InitScrollBarsData()
	{
		m_hWndVscroll = m_pCtrl->GetVscrollHwnd();
		m_hWndHscroll = m_pCtrl->GetHscrollHwnd();
		if (m_hWndVscroll) { ::GetClientRect(m_hWndVscroll, m_rcVscroll); }
		if (m_hWndHscroll) { ::GetClientRect(m_hWndHscroll, m_rcHscroll); }
		m_hasVscroll = m_hWndVscroll != 0;
		m_hasHscroll = m_hWndHscroll != 0;
	}

	void _CreatePopupWindow()
	{
		// Set up the style and extended style we want to use.
		DWORD dwStyle = WS_POPUP | /*WS_BORDER |*/ LBS_COMBOBOX | LBS_NOTIFY | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		DWORD dwExStyle = WS_EX_TOOLWINDOW |      // So it doesn't show up in taskbar
			/*WS_EX_WINDOWEDGE |*/
			WS_EX_TOPMOST;          // So it isn't obscured

		HINSTANCE hInstance = m_xeUI->GetInstanceHandle();

		m_hwndPopup = ::CreateWindowExW(
			dwExStyle,
			XEPOPUPWND_CLASSNAME,
			L"",
			dwStyle,
			m_rcScreenPos.left, m_rcScreenPos.top,
			m_rcScreenPos.Width(), m_rcScreenPos.Height(),
			m_hwndOwner,
			NULL,
			hInstance,
			this);
		//TRACE("_CreatePopupWindow - 0x%.8X, parent: 0x%.8X\n", m_hwndPopup, m_pParentWnd->m_hWnd);
	}

	void _AdjustWindowPopupFinalPosition()
	{
		HMONITOR hMonitor = ::MonitorFromRect(m_rcParentCtrlScreenPos, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		::GetMonitorInfo(hMonitor, &mi);
		CRect rcMonitor(mi.rcMonitor);
		if (m_rcScreenPos.Width() > rcMonitor.Width())
		{
			m_rcScreenPos.right = m_rcScreenPos.left + rcMonitor.Width();
		}
		if (m_rcScreenPos.Height() > rcMonitor.Height())
		{
			m_rcScreenPos.bottom = m_rcScreenPos.top + rcMonitor.Height();
		}
		if (m_rcScreenPos.left < rcMonitor.left)
		{
			m_rcScreenPos.OffsetRect(rcMonitor.left - m_rcScreenPos.left, 0);
		}
		if (m_rcScreenPos.right > rcMonitor.right)
		{
			m_rcScreenPos.OffsetRect(rcMonitor.right - m_rcScreenPos.right, 0);
		}
		if (rcMonitor.PtInRect(m_rcScreenPos.BottomRight())
			&& rcMonitor.PtInRect(m_rcScreenPos.TopLeft()))
		{
			return;	// dropdown is fully inside the monitor rect
		}
		int cyBelowParent = rcMonitor.bottom - m_rcParentCtrlScreenPos.bottom;
		int cyAboveParent = m_rcParentCtrlScreenPos.top - rcMonitor.top;
		if (!rcMonitor.PtInRect(m_rcScreenPos.BottomRight()))
		{
			if (cyAboveParent > cyBelowParent)
			{
				DropWndOffset offsetPreference = m_pCtrl->GetDropWndOffsetPreference();
				int cyOffset = -(m_rcScreenPos.Height() + m_rcParentCtrlScreenPos.Height());
				if (offsetPreference == DropWndOffset::OnParentCtrl)
				{
					cyOffset = -(m_rcScreenPos.Height());
				}
				m_rcScreenPos.OffsetRect(0, cyOffset);
				if (m_rcScreenPos.top < rcMonitor.top)
				{
					m_rcScreenPos.top = rcMonitor.top;
				}
			}
			else if (m_rcScreenPos.bottom > rcMonitor.bottom)
			{
				m_rcScreenPos.bottom = rcMonitor.bottom;
			}
		}
	}

	void RetargetMessageIfNeeded(MSG& msg)
	{
		POINT pt;
		bool isPtMapped = false;
		switch (msg.message)
		{
		// These mouse messages arrive in client coordinates, so in
		// addition to stealing the message, we also need to convert the
		// coordinates.
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_MOUSEHOVER:
			pt.x = (short)LOWORD(msg.lParam);
			pt.y = (short)HIWORD(msg.lParam);
			if (m_hasVscroll)
			{
				CPoint pt2(pt);
				::MapWindowPoints(msg.hwnd, m_hWndVscroll, &pt2, 1);
				if (m_rcVscroll.PtInRect(pt2))
				{
					msg.hwnd = m_hWndVscroll;
					msg.lParam = MAKELPARAM(pt2.x, pt2.y);
					isPtMapped = true;
				}
			}
			if (!isPtMapped && m_hasHscroll)
			{
				CPoint pt2(pt);
				::MapWindowPoints(msg.hwnd, m_hWndHscroll, &pt2, 1);
				if (m_rcHscroll.PtInRect(pt2))
				{
					msg.hwnd = m_hWndHscroll;
					msg.lParam = MAKELPARAM(pt2.x, pt2.y);
					isPtMapped = true;
				}
			}
			if (!isPtMapped)
			{
				::MapWindowPoints(msg.hwnd, m_hwndPopup, &pt, 1);
				msg.hwnd = m_hwndPopup;
				msg.lParam = MAKELPARAM(pt.x, pt.y);
			}
			break;

			// These mouse messages arrive in screen coordinates, so we just
			// need to steal the message.
		case WM_NCMOUSEMOVE:
		case WM_NCLBUTTONDOWN:
		case WM_NCLBUTTONUP:
		case WM_NCLBUTTONDBLCLK:
		case WM_NCRBUTTONDOWN:
		case WM_NCRBUTTONUP:
		case WM_NCRBUTTONDBLCLK:
		case WM_NCMBUTTONDOWN:
		case WM_NCMBUTTONUP:
		case WM_NCMBUTTONDBLCLK:
		case WM_MOUSELEAVE:
			msg.hwnd = m_hwndPopup;
			break;

			// We need to steal all keyboard messages, too.
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
		case WM_DEADCHAR:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_SYSCHAR:
		case WM_SYSDEADCHAR:
			msg.hwnd = m_hwndPopup;
			break;
		}
	}

	void _ProcessNotifyPopupWndSizeChangeMessage()
	{
		CSize sPopupWnd = m_pCtrl->GetWindowPreferredSize();
		if (sPopupWnd.cx && sPopupWnd.cy)
		{
			m_rcScreenPos = m_rcScreenPosFirst;
			m_rcScreenPos.right = m_rcScreenPos.left + sPopupWnd.cx;
			m_rcScreenPos.bottom = m_rcScreenPos.top + sPopupWnd.cy;
			_AdjustWindowPopupFinalPosition();
			m_rcScreenPos.InflateRect(2, 2);	// Adjust final size to include borders
			CRect r(m_rcScreenPos);
			::MoveWindow(m_hwndPopup, r.left, r.top, r.Width(), r.Height(), TRUE);
			::RedrawWindow(m_hwndPopup, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
		}
	}
};

LRESULT CALLBACK Popup_WndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	CXePopupCtrl* pThis = nullptr;
	if (uiMsg == WM_NCCREATE)
	{
		CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
		pThis = (CXePopupCtrl*)pCS->lpCreateParams;
	}
	else
	{
		pThis = (CXePopupCtrl*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
	}
	if (pThis && pThis->m_signature == XEPOPUPWND_SIGNATURE)
	{
		return pThis->OnMessage(hwnd, uiMsg, wParam, lParam);
	}
	return ::DefWindowProc(hwnd, uiMsg, wParam, lParam);
}

