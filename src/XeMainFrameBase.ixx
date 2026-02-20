module;

#include "os_minimal.h"
#include <memory>
#include <string>
#include <string_view>
#include "XeResource.h"

export module Xe.MainFrameBase;

import Xe.UIcolorsIF;
import Xe.D2DWndBase;
import Xe.D2DToolbar;
import Xe.UserSettingsForUI;
import Xe.Helpers;
import Xe.ViewManager;
//import Xe.LogFormatsManager;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Honestly "stolen" from MFC afxwin.h and appcore.cpp
export class CXeCommandLineInfo
{
public:
	void ParseParam(const wchar_t* pszParam, BOOL bFlag, BOOL bLast)
	{
		if (bFlag)	// param has flag '-' or '/'
		{
			ParseParamFlag(pszParam);
		}
		else
		{
			ParseParamNotFlag(pszParam);
		}

		if (bLast)
		{
			ParseLast();
		}
	}

	enum {
		FileNew, FileOpen/*, FilePrint, FilePrintTo, FileDDE, FileDDENoShow, AppRegister,
		AppUnregister, RestartByRestartManager, FileNothing = -1*/
	} m_nShellCommand = FileNew;

	std::wstring m_strFileName;

protected:
	void ParseParamFlag(const wchar_t* pszParam)
	{
		XeASSERT(false); // No flags supported
	}

	void ParseParamNotFlag(const wchar_t* pszParam)
	{
		if (m_strFileName.size() == 0)
		{
			m_strFileName = pszParam;
		}
	}

	void ParseLast()
	{
		if (m_nShellCommand == FileNew && m_strFileName.size() > 0)
			m_nShellCommand = FileOpen;
	}
};

constexpr auto SplitBarPx = 6;

int _GetValueInValidRange(int newValue, int minValue, int maxValue)
{
	return (newValue < minValue) ? minValue : (newValue > maxValue) ? maxValue : newValue;
}

constexpr int VIEW_CX_MIN = 50;

export class CXeMainFrameBase : public CXeD2DWndBase
{
#pragma region class_data
protected:
	std::wstring m_mainWndClassName;	// Derived class MUST set this.

	UINT m_uIDR_MAINFRAME = 0;

	std::unique_ptr<CXeD2DToolbar> m_pToolBar;

	bool m_isClientWndCreated = false;	// Set TRUE in OnCreateClient(...)

	// Last known good restore position and size. Note - is empty if not set (yet).
	UINT m_lastKnownShowWindowCmd = 0;
	CRect m_rcLastKnownRestorePos;

	CXeViewManagerIF* m_pVwMgr = nullptr;

	// Splitter window data.
	bool m_bTracking = false, m_bIsTrackerV, m_bIsLogSplit;
	CRect m_rcVsplit;		// Vertical splitter (above timeline)
	CRect m_rcH_BM_split;	// Horizontal splitter between bookmarks view and log view 0
	CRect m_rcH_Log_split;	// Horizontal splitter between log views.
	CRect m_rcTracker;
	int m_cxUserSelectWidthCol0 = -1, m_cyUserSelectHeightTimeline = -1;
#pragma endregion class_data

#pragma region WindowCreation
public:
	CXeMainFrameBase(CXeUIcolorsIF* pUIcolors) : CXeD2DWndBase(pUIcolors)
	{
		s_xeUIsettings.SetNotifyCallback(
				[this](const ChangedSettings& chg_settings) { this->_OnChangedSettings(chg_settings); });

		m_pToolBar = std::make_unique<CXeD2DToolbar>(m_xeUI);
	}
	//virtual ~CXeMainFrameBase() override {}

	bool Create(UINT idr_mainframe, const std::wstring& title, const CRect& sizeAndPosition)
	{
		m_uIDR_MAINFRAME = idr_mainframe;
		// Register 'this' window class if not already defined.
		XeASSERT(m_mainWndClassName.size() > 0);	// Derived class MUST set this.
		LPCTSTR pszIcon = MAKEINTRESOURCE(m_uIDR_MAINFRAME);
		HINSTANCE hInst = m_xeUI->GetInstanceHandle(); // AfxFindResourceHandle(pszIcon, ATL_RT_GROUP_ICON);
		XeASSERT(hInst != NULL);
		m_xeUI->RegisterWindowClass(m_mainWndClassName, D2DCtrl_WndProc, CS_DBLCLKS, ::LoadIcon(hInst, pszIcon),
				::CreateSolidBrush(m_xeUI->GetColor(CID::XTbBarBgCtrl)));

		int x = CW_USEDEFAULT, y = CW_USEDEFAULT, cx = CW_USEDEFAULT, cy = CW_USEDEFAULT;
		if (!sizeAndPosition.IsRectEmpty())
		{
			x = sizeAndPosition.left;
			y = sizeAndPosition.top;
			cx = sizeAndPosition.Width();
			cy = sizeAndPosition.Height();
		}
		HWND hWnd = CreateD2DWindow(0, m_mainWndClassName.c_str(), title.c_str(),
				WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CRect(x, y, x + cx, y+ cy), NULL, NULL, true);
		m_xeUI->SetMainWindowHandle(hWnd);

		::ShowWindow(hWnd, SW_HIDE);	// Set show state to hide - while we create the views.

		::DragAcceptFiles(hWnd, true);

		m_xeUI->SetGlobalKeyboardFilterCallback(
				[this](const MSG& msg) { return _OnGlobalKeyboardFilter(msg); });

		XeASSERT(m_pVwMgr);	// Derived class MUST set the pointer after creating the view manager.
		m_pVwMgr->CreateTabViews(hWnd, m_pToolBar.get());

		//m_pVwMgr->Create(Hwnd(), VW_ID_TABS_0, VW_ID_TABS_1, VW_ID_VIEW_0, VW_ID_VIEW_1);

		_OnCreateClient();	// Derived class creates its view window
		m_isClientWndCreated = true;

		// Load window placement structure from settings json file - it was saved on last run.
		CXeUserSettings settings(L"MainFrameWindow");
		if (settings.Exists(L"WNDPLC_RestoreRect"))
		{
			CRect rc;
			settings.Get(L"WNDPLC_RestoreRect").getBinary((uint8_t*)&rc, sizeof(rc));
			if (::MonitorFromRect(rc, MONITOR_DEFAULTTONULL))
			{
				m_rcLastKnownRestorePos = rc;
			}
		}
		WINDOWPLACEMENT wpc;
		if (settings.Get(L"WNDPLC").getBinary((uint8_t*)&wpc, sizeof(wpc)))
		{
			::SetWindowPlacement(Hwnd(), &wpc);// Restore window position size and maximized state.
		}
		else
		{
			::ShowWindow(hWnd, SW_SHOW);
		}
		// Note - the call to base class will actually show the main window.

		// Parse command line for standard shell commands, DDE, file open
		CXeCommandLineInfo cmdInfo;
		_ParseCommandLine(cmdInfo);
		_ProcessCommandLine(cmdInfo);

		return hWnd != 0;
	}

protected:
	// Derived class should override this and process command line parameters.
	virtual void _ProcessCommandLine(const CXeCommandLineInfo& cmdInfo)
	{
	}

	// Derived class should override this to create it's view window.
	virtual void _OnCreateClient() {  }

	virtual LRESULT _OnDestroy() override
	{
		CXeD2DWndBase::_OnDestroy();

		// Save main window size, position and maximized state in registry.
		WINDOWPLACEMENT	wp = GetCurrentWindowPlacement();
		// Save data structure in json settings as binary data - it is used on next time run.
		CXeUserSettings settings(L"MainFrameWindow");
		settings.SetBinary(L"WNDPLC", &wp, sizeof(wp));
		if (!m_rcLastKnownRestorePos.IsRectEmpty())
		{
			settings.SetBinary(L"WNDPLC_RestoreRect", &m_rcLastKnownRestorePos, sizeof(m_rcLastKnownRestorePos));
		}
		::PostQuitMessage(0);
		return 0;
	}

	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender) override
	{
		m_pVwMgr->ProcessCommand(wID, 0);
		return 0;
	}

	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WMU_NOTIFY_ID_APP_EXIT:
			return _OnNotifyAppExit(wParam, lParam);

		case WM_NCUAHDRAWCAPTION:
		case WM_NCUAHDRAWFRAME:
			return 0;	// Eat message.
		}
		return CXeD2DWndBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	// WMU_NOTIFY_ID_APP_EXIT message handler
	// (posted from toolbar - that can't process ID__APP_EXIT normally - for reasons...).
	LRESULT _OnNotifyAppExit(WPARAM wParam, LPARAM lParam)
	{
		::SendMessage(Hwnd(), WM_SYSCOMMAND, SC_CLOSE, 0);
		return 0;
	}
#pragma endregion WindowCreation

#pragma region Custom_Window_Frame
protected:
	//////////////////////////////////////////////////////////////////////////////
	// UI - Custom Window Frame - support.
	virtual LRESULT _OnNcCreate(HWND hwnd, WPARAM wParam, LPARAM lParam) override
	{	// WM_NCCREATE handler.
		CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;

		// Create toolbar for custom frame NC area.
		m_pToolBar->Create(hwnd, CRect(), 201, m_uIDR_MAINFRAME, true);
		_OnCreateToolbar();
		m_pToolBar->RecalculateItems();

		return CXeD2DWndBase::_OnNcCreate(hwnd, wParam, lParam);
	}

	virtual LRESULT _OnNcDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam) override
	{
		m_xeUI->SetMainWindowHandle(0);
		return CXeD2DWndBase::_OnNcDestroy(hwnd, wParam, lParam);
	}

	virtual LRESULT _OnNcCalcSize(WPARAM wParam, LPARAM lParam) override
	{
		m_pToolBar->RecalculateItems();
		m_style.SetToolbarSize(m_pToolBar->GetToolBarSize());
		return CXeD2DWndBase::_OnNcCalcSize(wParam, lParam);
	}
		
	// Derived class should override this to add more controls to the main window toolbar.
	virtual void _OnCreateToolbar() {}

	// Returns width of toolbar.
	virtual float _NCPaintToolbar() override
	{
		if (m_pToolBar)
		{
			D2D1_RECT_F rcToolBar = m_style.GetRects().GetRectTL00F(HT_TOOLBAR);
			m_pToolBar->_PaintInternal(this, rcToolBar);

			return WidthOf(rcToolBar);
		}
		return 0.0f;
	}

	// Return true if NC paint needed.
	virtual bool _OnNcMouseMoveXe(EIB_HIT eHit, const CPoint& point) override
	{
		if (m_pToolBar && eHit == EIB_HIT::eToolBar)
		{
			CPoint ptForTB = point;
			const CRect& rcToolbar = m_style.GetRects().GetRect(HT_TOOLBAR);
			ptForTB.Offset(-rcToolbar.left, -rcToolbar.top);
			return m_pToolBar->DoOnMouseMove(GetMouseMsgFlags(), ptForTB);
		}
		return false;
	}

	virtual bool _OnNcMouseLeaveXe() override
	{
		if (m_pToolBar)
		{
			::SendMessage(m_pToolBar->GetSafeHwnd(), WM_MOUSELEAVE, 0, 0);
			return true;
		}
		return false;
	}

	// Return true if NC paint needed.
	virtual bool _OnNcLbuttonDownXe(EIB_HIT eHit, const CPoint& point) override
	{
		if (m_pToolBar && eHit == EIB_HIT::eToolBar)
		{
			CPoint ptForTB = point;
			const CRect& rcToolbar = m_style.GetRects().GetRect(HT_TOOLBAR);
			ptForTB.Offset(-rcToolbar.left, -rcToolbar.top);
			m_pToolBar->DoOnLButtonDown(GetMouseMsgFlags(), ptForTB);
			return true;
		}
		return false;
	}

	// Return true if NC paint needed.
	virtual bool _OnNcLbuttonUpXe(EIB_HIT eHit, const CPoint& point) override
	{
		if (m_pToolBar && eHit == EIB_HIT::eToolBar)
		{
			CPoint ptForTB = point;
			const CRect& rcToolbar = m_style.GetRects().GetRect(HT_TOOLBAR);
			ptForTB.Offset(-rcToolbar.left, -rcToolbar.top);
			m_pToolBar->DoOnLButtonUp(GetMouseMsgFlags(), ptForTB);
			return true;
		}
		return false;
	}
	
	virtual LRESULT _OnSysCommand(WPARAM wParam, LPARAM lParam) override
	{
		WINDOWPLACEMENT	wp = GetCurrentWindowPlacement();
		UINT nID = (UINT)wParam;
		if ((nID & 0xFFF0) == SC_MAXIMIZE)
		{
			m_rcLastKnownRestorePos = wp.rcNormalPosition;
			m_lastKnownShowWindowCmd = SW_SHOWMAXIMIZED;
			wp.showCmd = SW_SHOWMAXIMIZED;
			::SetWindowPlacement(Hwnd(), &wp);
			::RedrawWindow(Hwnd(), nullptr, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		}
		else if ((nID & 0xFFF0) == SC_RESTORE)
		{
			if (!m_rcLastKnownRestorePos.IsRectEmpty())
			{
				wp.rcNormalPosition = m_rcLastKnownRestorePos;
			}
			if (wp.showCmd == SW_MAXIMIZE)
			{
				wp.showCmd = SW_NORMAL;
			}
			else
			{
				wp.showCmd = m_lastKnownShowWindowCmd != 0 ? m_lastKnownShowWindowCmd : SW_SHOWNORMAL;
			}
			::SetWindowPlacement(Hwnd(), &wp);
			::RedrawWindow(Hwnd(), nullptr, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		}
		else if ((nID & 0xFFF0) == SC_MINIMIZE)
		{
			if (wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWNORMAL)
			{
				m_lastKnownShowWindowCmd = wp.showCmd;
			}
			m_rcLastKnownRestorePos = wp.rcNormalPosition;
			wp.showCmd = SW_SHOWMINIMIZED;
			::SetWindowPlacement(Hwnd(), &wp);
		}
		else if ((nID & 0xFFF0) == SC_KEYMENU)
		{
			return 0;	// Don't allow app to enter into menu loop (WM_ENTERMENULOOP).
		}
		//return 0;
		return ::DefWindowProc(Hwnd(), WM_SYSCOMMAND, wParam, lParam);
	}
#pragma endregion Custom_Window_Frame

#pragma region WindowSizeHandling
	virtual LRESULT _OnGetMinMaxInfo(WPARAM wParam, LPARAM lParam) override
	{
		MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
		pMMI->ptMinTrackSize = CPoint(500, 500);
		return CXeD2DWndBase::_OnGetMinMaxInfo(wParam, lParam);
	}

	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{
		UINT nType = (UINT)wParam;
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);

		if (!m_isClientWndCreated)	// Panes created yet?
			return 0;

		RepositionWindows();

		// Custom window frame stuff.
		if (m_pToolBar)
		{
			CRect rcTB = m_style.GetRects().GetRect(HT_TOOLBAR);
			ScreenToClient(&rcTB);
			::MoveWindow(m_pToolBar->GetSafeHwnd(), rcTB.left, rcTB.top, rcTB.Width(), rcTB.Height(), false);
		}
		return CXeD2DWndBase::_OnSize(hWnd, wParam, lParam);
	}

	virtual LRESULT _OnExitSizeMove() override
	{
		LRESULT lResult = CXeD2DWndBase::_OnExitSizeMove();
		if (m_pToolBar)
		{
			CRect rcTB = m_style.GetRects().GetRect(HT_TOOLBAR);
			ScreenToClient(&rcTB);
			::MoveWindow(m_pToolBar->GetSafeHwnd(), rcTB.left, rcTB.top, rcTB.Width(), rcTB.Height(), false);
		}

		return lResult;
	}
#pragma endregion WindowSizeHandling

#pragma region SplitterWindowSupport
public:
	virtual void RepositionWindows()
	{
		CRect rcClient = _GetClientRect();
		int cxCli = rcClient.Width();
		int cyCli = rcClient.Height();
		//HWND hTimelineWnd = GetDlgItem(VW_ID_HVIEW);
		HWND hHorzVwWnd = GetDlgItem(VW_ID_HVIEW);
		int nTimelineHeight = 0;
		int yViews = 0;
		m_rcVsplit.SetRectEmpty();
		HDWP hDWP = ::BeginDeferWindowPos(6);
		CXeUserSettings settings(L"MainFrameWindow");
		bool isHorzViewAtTop = settings.GetBool_or_Val(L"ShowHorzViewAtTop", false);
		//bool isTimelineAtTop = s_xeUIsettings[L"TimelineSettings"].Get(L"ShowTimeLineViewAtTop").getBool();
		if (hHorzVwWnd)
		{
			int nTimelineTotalHeight = _GetViewProp(VW_ID_HVIEW, XeViewProp::TOTAL_CY);
			int nTimelineMinHeight = _GetViewProp(VW_ID_HVIEW, XeViewProp::MIN_CY);
			int nTimelineMaxHeight = _GetViewProp(VW_ID_HVIEW, XeViewProp::MAX_CY);
			if (m_cyUserSelectHeightTimeline > 0 && m_cyUserSelectHeightTimeline > nTimelineTotalHeight)
			{
				// User selected timeline height but it is greater than current max timeline height.
				// (meaning that timeline height has decreased below user set value, e.g. view closed)
				m_cyUserSelectHeightTimeline = nTimelineTotalHeight;
			}
			nTimelineHeight = m_cyUserSelectHeightTimeline > 0
				? m_cyUserSelectHeightTimeline
				: nTimelineTotalHeight > nTimelineMaxHeight ? nTimelineMaxHeight : nTimelineTotalHeight;
			int yTlWnd = isHorzViewAtTop ? 0 : cyCli - nTimelineHeight;
			hDWP = ::DeferWindowPos(hDWP, hHorzVwWnd, NULL, 0, yTlWnd, cxCli, nTimelineHeight, SWP_NOZORDER | SWP_NOACTIVATE);
			if (isHorzViewAtTop)
			{
				int yTlSplt = yTlWnd + nTimelineHeight;
				m_rcVsplit.SetRect(0, yTlSplt, cxCli, yTlSplt + SplitBarPx);
				yViews = yTlSplt + SplitBarPx;
			}
			else
			{
				m_rcVsplit.SetRect(0, yTlWnd - SplitBarPx, cxCli, yTlWnd);
			}
		}
		else
		{
			m_cyUserSelectHeightTimeline = -1;
		}

		HWND hBkMkVw = GetDlgItem(VW_ID_SIDE);
		m_rcH_BM_split.SetRectEmpty();
		int cxBkMkVwTotal = 0;	// width of bookmarks view incl. splitter.
		int yViewsBottom = isHorzViewAtTop ? cyCli : cyCli - nTimelineHeight;
		int cxBkMkVw = 0, cyBkMkVw = 0;
		if (hBkMkVw)
		{
			cxBkMkVw = _GetViewProp(VW_ID_SIDE, XeViewProp::WIDTH);
			cyBkMkVw = nTimelineHeight > 0 ? cyCli - nTimelineHeight - SplitBarPx : cyCli;
			cxBkMkVwTotal = cxBkMkVw + SplitBarPx;
		}

		HWND hTabVw0 = GetDlgItem(VW_ID_TABS_0);
		HWND hTabVw1 = GetDlgItem(VW_ID_TABS_1);
		bool isDual = hTabVw0 && hTabVw1;
		int cxViewMin = _GetViewProp(VW_ID_VIEW_0, XeViewProp::MIN_CX);
		int cxLogVwTotal = cxCli - cxBkMkVwTotal;	// Width of log view(s).
		int cxLogViewMin = isDual ? (2 * cxViewMin + SplitBarPx) : cxViewMin;
		if (cxLogVwTotal < cxLogViewMin)
		{
			cxBkMkVw = cxCli - cxLogViewMin;
			cxBkMkVwTotal = cxBkMkVw + SplitBarPx;
			cxLogVwTotal = cxCli - cxBkMkVwTotal;
		}
		int xLogVw0 = cxBkMkVwTotal;				// X pos of log view 0
		int cxLogVw0 = cxLogVwTotal, cxLogVw1 = 0, xLogVw1 = 0;
		m_rcH_Log_split.SetRectEmpty();
		if (isDual)
		{
			cxLogVw0 = m_cxUserSelectWidthCol0 > 0 ? m_cxUserSelectWidthCol0 : (cxLogVwTotal / 2) - (SplitBarPx / 2);
			cxLogVw1 = cxLogVwTotal - cxLogVw0 - (SplitBarPx / 2);
			if (cxLogVw1 < cxViewMin)
			{
				cxLogVw1 = cxViewMin + SplitBarPx;
				cxLogVw0 = cxLogVwTotal - cxLogVw1;
			}
			xLogVw1 = xLogVw0 + cxLogVw0 + SplitBarPx;
			m_rcH_Log_split.SetRect(xLogVw1 - SplitBarPx, yViews, xLogVw1, yViewsBottom);
		}
		else
		{
			m_cxUserSelectWidthCol0 = -1;
		}
		int cyTabView0 = _GetViewProp(VW_ID_TABS_0, XeViewProp::RECALC_CY, cxLogVw0);
		int cyVw0 = nTimelineHeight > 0 ? cyCli - cyTabView0 - nTimelineHeight - SplitBarPx : cyCli - cyTabView0;
		int yVw0 = yViews + cyTabView0;
		int cyTabView1 = _GetViewProp(VW_ID_TABS_1, XeViewProp::RECALC_CY, cxLogVw1);
		int cyVw1 = nTimelineHeight > 0 ? cyCli - cyTabView1 - nTimelineHeight - SplitBarPx : cyCli - cyTabView1;
		int yVw1 = yViews + cyTabView1;
		if (hBkMkVw)
		{
			hDWP = ::DeferWindowPos(hDWP, hBkMkVw, NULL, 0, yViews, cxBkMkVw, cyBkMkVw, SWP_NOZORDER | SWP_NOACTIVATE);
			m_rcH_BM_split.SetRect(cxBkMkVw, yViews, cxBkMkVwTotal, yViewsBottom);
		}
		if (hTabVw0)
		{
			hDWP = ::DeferWindowPos(hDWP, hTabVw0, NULL, xLogVw0, yViews, cxLogVw0, cyTabView0, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		if (hTabVw1)
		{
			hDWP = ::DeferWindowPos(hDWP, hTabVw1, NULL, xLogVw1, yViews, cxLogVw1, cyTabView1, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		HWND hView0 = GetDlgItem(VW_ID_VIEW_0);
		if (hView0)
		{
			hDWP = ::DeferWindowPos(hDWP, hView0, NULL, xLogVw0, yVw0, cxLogVw0, cyVw0, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		HWND hView1 = GetDlgItem(VW_ID_VIEW_1);
		if (hView1)
		{
			hDWP = ::DeferWindowPos(hDWP, hView1, NULL, xLogVw1, yVw1, cxLogVw1, cyVw1, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		::EndDeferWindowPos(hDWP);

		if (!(hTabVw0 || hTabVw1 || hView0 || hView1 || hBkMkVw || hHorzVwWnd))	// No child windows?
		{
			_RedrawDirectly();
		}
	}

protected:
	virtual int _GetViewProp(int view_id, XeViewProp view_property_id, int param = 0)
	{
		bool isTabsVwId = view_id == VW_ID_TABS_0 || view_id == VW_ID_TABS_1;
		bool isViewsVwId = view_id == VW_ID_VIEW_0 || view_id == VW_ID_VIEW_1;
		if (isTabsVwId && view_property_id == XeViewProp::RECALC_CY)
		{
			return m_pVwMgr->RecalculateTabViewHeight(view_id - VW_ID_TABS_0, param);
		}
		else if (isTabsVwId || isViewsVwId)
		{
			if (view_property_id == XeViewProp::MIN_CX)
			{
				return VIEW_CX_MIN;
			}
		}
		XeASSERT(false);	// Not (needed) implemented.
		return 0;
	}
	//virtual int _GetViewProp(int view_id, XeViewProp view_property_id, int param = 0)
	//{
	//	XeASSERT(false);	// Derived class should implement this.
	//	return 0;
	//}

	virtual void _SetSideViewWidth(int cxViewWidth)
	{
		XeASSERT(false);	// Derived class should implement this.
	}

	bool _IsViewVisible(int view_id)
	{
		return GetDlgItem(view_id) != nullptr;
	}
#pragma endregion SplitterWindowSupport

#pragma region Mouse_Processing
	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint pt) override
	{
		if (m_bTracking)
		{
			if (::GetCapture() != Hwnd())
			{
				m_bTracking = false;
			}

			// move tracker to current cursor position
			CRect rcClient = _GetClientRect();
			if (m_bIsTrackerV)
			{
				int cyTlTotalHeight = _GetViewProp(VW_ID_HVIEW, XeViewProp::TOTAL_CY);
				int cyTlMinHeight = _GetViewProp(VW_ID_HVIEW, XeViewProp::MIN_CY);
				int yMinTrackerTop = rcClient.bottom - (cyTlTotalHeight + SplitBarPx);
				int yMaxTrackerTop = rcClient.bottom - (cyTlMinHeight + SplitBarPx);
				//if (s_xeUIsettings[L"TimelineSettings"].Get(L"ShowTimeLineViewAtTop").getBool())
				CXeUserSettings settings(L"MainFrameWindow");
				if (settings.GetBool_or_Val(L"ShowHorzViewAtTop", false))
				{
					yMinTrackerTop = cyTlMinHeight + SplitBarPx;
					yMaxTrackerTop = cyTlTotalHeight + SplitBarPx;
				}
				int yNewTop = _GetValueInValidRange(pt.y - (SplitBarPx / 2), yMinTrackerTop, yMaxTrackerTop);
				m_rcTracker.top = yNewTop;
				m_rcTracker.bottom = m_rcTracker.top + SplitBarPx;
			}
			else
			{
				// Range when bookmarks list not visible and we are doing log views splitter.
				// Also when doing bookmarks splitter and log view 1 is not visible.
				int cxVwMin = _GetViewProp(VW_ID_VIEW_0, XeViewProp::MIN_CX);
				int xMaxTrackerLeft = rcClient.Width() - cxVwMin;
				int xMinTrackerLeft = cxVwMin;
				if (m_bIsLogSplit && _IsViewVisible(VW_ID_SIDE))
				{
					// Range when bookmarks list is visible and we are doing log views splitter.
					xMinTrackerLeft = m_rcH_BM_split.right + cxVwMin;
				}
				else if (!m_bIsLogSplit && _IsViewVisible(VW_ID_VIEW_1))
				{
					// Range when log view 1 visible and we are doing bookmarks view splitter.
					xMinTrackerLeft = cxVwMin;
					xMaxTrackerLeft = m_rcH_Log_split.left - cxVwMin;
				}
				int xNewLeft = _GetValueInValidRange(pt.x - (SplitBarPx / 2), xMinTrackerLeft, xMaxTrackerLeft);
				m_rcTracker.left = xNewLeft;
				m_rcTracker.right = m_rcTracker.left + SplitBarPx;
			}
			_RepositionWindowsWhenTracking();
		}
		else
		{
			// hit-test and set cursor
			bool isInHsplit = m_rcH_Log_split.PtInRect(pt) || m_rcH_BM_split.PtInRect(pt);
			LPCTSTR cursor = isInHsplit ? IDC_SIZEWE : m_rcVsplit.PtInRect(pt) ? IDC_SIZENS : IDC_ARROW;
			SetCursor(::LoadCursor(NULL, cursor));
		}
		return CXeD2DWndBase::_OnMouseMove(nFlags, pt);
	}

	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint pt) override
	{
		if (m_bTracking)
			return 0;

		if (m_rcH_BM_split.PtInRect(pt))
		{
			m_rcTracker = m_rcH_BM_split;
			m_bIsTrackerV = false;
			m_bIsLogSplit = false;
		}
		else if (m_rcH_Log_split.PtInRect(pt))
		{
			m_rcTracker = m_rcH_Log_split;
			m_bIsTrackerV = false;
			m_bIsLogSplit = true;
		}
		else if (m_rcVsplit.PtInRect(pt))
		{
			m_rcTracker = m_rcVsplit;
			m_bIsTrackerV = true;
		}
		else
		{
			return 0;
		}

		m_bTracking = true;
		SetCapture();
		SetFocus();
		return 0;
	}

	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint pt) override
	{
		if (!m_bTracking)
			return 0;

		ReleaseCapture();
		m_rcTracker.SetRectEmpty();
		m_bTracking = false;
		return 0;
	}

	void _RepositionWindowsWhenTracking()
	{
		if (m_bIsTrackerV)
		{
			CRect rcClient;
			GetClientRect(rcClient);
			CXeUserSettings settings(L"MainFrameWindow");
			m_cyUserSelectHeightTimeline
				= settings.GetBool_or_Val(L"ShowHorzViewAtTop", false)
				? m_rcTracker.bottom : rcClient.bottom - m_rcTracker.bottom;
			//m_cyUserSelectHeightTimeline
			//	= s_xeUIsettings[L"TimelineSettings"].Get(L"ShowTimeLineViewAtTop").getBool()
			//	? m_rcTracker.bottom : rcClient.bottom - m_rcTracker.bottom;
		}
		else
		{
			if (m_bIsLogSplit)
			{
				if (_IsViewVisible(VW_ID_SIDE))
				{
					m_cxUserSelectWidthCol0 = m_rcTracker.left - m_rcH_BM_split.right;
				}
				else
				{
					m_cxUserSelectWidthCol0 = m_rcTracker.left;
				}
			}
			else
			{
				// Set new width for bookmarks view.
				_SetSideViewWidth(m_rcTracker.left);
			}
		}
		RepositionWindows();
	}

	virtual LRESULT _OnCancelMode() override
	{
		if (m_bTracking && ::GetCapture() == Hwnd())
		{
			::ReleaseCapture();
			m_bTracking = false;
		}
		return CXeD2DWndBase::_OnCancelMode();
	}
#pragma endregion Mouse_Processing

#pragma region Paint
protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override
	{
		if (m_rcH_BM_split.IsRectEmpty() && m_rcH_Log_split.IsRectEmpty() && m_rcVsplit.IsRectEmpty())
		{
			pRT->FillRectangle(rcClient, GetBrush(CID::XTbBarBgCtrl));
		}
		_DrawSplitter(pRT, m_rcH_BM_split);
		_DrawSplitter(pRT, m_rcH_Log_split);
		_DrawSplitter(pRT, m_rcVsplit);
	}

	void _DrawSplitter(ID2D1RenderTarget* pRT, CRect rcSplitter)
	{
		if (!rcSplitter.IsRectEmpty())
		{
			pRT->FillRectangle(RectFfromRect(rcSplitter), GetBrush(CID::CtrlBorder));
		}
	}
#pragma endregion Paint

#pragma region UI_OpenFiles
	virtual LRESULT _OnDropFiles(WPARAM wParam, LPARAM lParam) override
	{
		HDROP dropInfo = (HDROP)wParam;
		POINT ptDrop = { 0 };
		DragQueryPoint(dropInfo, &ptDrop);
		::ClientToScreen(Hwnd(), &ptDrop);

		int nFilesDropped = DragQueryFile(dropInfo, 0xFFFFFFFF, NULL, 0);	// Get the number of files dropped
		wchar_t buffer[MAX_PATH + 1];
		DWORD nBuffer = 0;
		std::vector<std::wstring> dropped_files;
		for (int i = 0; i < nFilesDropped; i++)
		{
			nBuffer = DragQueryFile(dropInfo, i, NULL, 0);	// Get the buffer size of the file.
			XeASSERT(nBuffer <= MAX_PATH);
			if (nBuffer > MAX_PATH) { continue; }

			DragQueryFile(dropInfo, i, buffer, nBuffer + 1);	// Get path and name of the file
			std::wstring_view svw(buffer, nBuffer);
			std::wstring sFile(svw);
			dropped_files.push_back(sFile);
		}
		DragFinish(dropInfo);	// Free the memory block containing the dropped-file information

		OnDroppedFiles(ptDrop, dropped_files);

		return 0;
	}

	virtual void OnDroppedFiles(const CPoint& ptDrop, const std::vector<std::wstring>& dropped_files) {}
#pragma endregion UI_OpenFiles

#pragma region CommandProcessing
protected:
	void _ParseCommandLine(CXeCommandLineInfo& rCmdInfo)
	{
		for (int i = 1; i < __argc; i++)
		{
			const wchar_t* pszParam = __wargv[i];
			BOOL bFlag = FALSE;
			BOOL bLast = ((i + 1) == __argc);
			if (pszParam[0] == '-' || pszParam[0] == '/')
			{
				// remove flag specifier
				bFlag = TRUE;
				++pszParam;
			}
			rCmdInfo.ParseParam(pszParam, bFlag, bLast);
		}
	}

#pragma endregion CommandProcessing

#pragma region MiscStuff
protected:
	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam) override
	{
		HWND hWnd = (HWND)wParam;
		UINT nHitTest = LOWORD(lParam);
		UINT message = HIWORD(lParam);
		return m_xeUI->OnSetCursorHelper(Hwnd(), hWnd, nHitTest, message);
	}

	// Filter WM_KEYDOWN or WM_SYSKEYDOWN or WM_KEYUP or WM_SYSKEYUP or WM_CHAR
	// Called from within the message loop (in the main app or modal dialog message loop).
	virtual bool _OnGlobalKeyboardFilter(const MSG& msg)
	{
		CXeShiftCtrlAltKeyHelper sca;
		UINT uKey = (UINT)msg.wParam;
		if (msg.message == WM_SYSKEYDOWN)
		{
			bool wasKeyDownBefore = msg.lParam & 0x40000000;
			bool hasToolbar = m_pToolBar && m_pToolBar->GetSafeHwnd();
			if (uKey == VK_MENU && hasToolbar)
			{
				// Repaint NC area to show 'Alt' menu state.
				if (!wasKeyDownBefore)
				{
					::SendMessage(Hwnd(), WM_NCPAINT, 0, 0);	// paint non-client area (frame too)
				}
			}
		}
		else if (msg.message == WM_SYSKEYUP)
		{
			bool hasToolbar = m_pToolBar && m_pToolBar->GetSafeHwnd();
			if (uKey == VK_MENU && hasToolbar)
			{
				// Repaint NC area to show 'Alt' menu state.
				::SendMessage(Hwnd(), WM_NCPAINT, 0, 0);	// paint non-client area (frame too)
			}
			else if (uKey == 32 /* space */)
			{
				// Display system menu just below icon.
				CPoint ptMenu = m_style.GetRects().GetSysMenuPosition();
				LPARAM lParmCoords = MAKELONG(ptMenu.x, ptMenu.y);
				::SendMessage(Hwnd(), WM_CONTEXTMENU, (WPARAM)Hwnd(), lParmCoords);
				//SendMessage(WM_SYSCOMMAND, SC_TASKLIST);	// Activate "system" menu
			}
			else if (hasToolbar && m_pToolBar->ProcessAcceleratorKey(uKey))
			{
				return true;	// "eat" message.
			}
		}

		return false;	// Process message normally
	}

	virtual LRESULT _OnNotify_NeedTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT) override
	{
		XeASSERT(pNeedTT);
		if (pNeedTT)
		{
			if (pNeedTT->pt->y < 0)	// Coords. in NC area?
			{
				::ClientToScreen(Hwnd(), pNeedTT->pt);
				if (GetNCTooltip(pNeedTT/*, sti*/))
				{
					return 1;
				}
			}
		}
		return 0;
	}

	// Get tool tip for NC title bar area. Note - mouse coords. are in screen coords.
	bool GetNCTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT/*, SUPER_TOOLTIP_INFO& sti*/)
	{
		if (m_pToolBar)
		{
			const CRect& rcToolbar = m_style.GetRects().GetRect(HT_TOOLBAR);
			pNeedTT->pt->x += -rcToolbar.left;
			pNeedTT->pt->y += -rcToolbar.top;
			return m_pToolBar->GetTooltip(pNeedTT/*, sti*/);
		}
		return false;
	}

protected:
	// Set or update font list drop down in toolbar.
	// uID_FONT_SIZE = toolbar control identifier
	void _SetToolBarFontSizeList(UINT uID_FONT_SIZE)
	{
		std::vector<ListBoxExItem> list;
		const CXeUserSetting& ui_font_setting = s_xeUIsettings[L"GeneralSettings"].Get(L"UI_FontPointSize");
		std::vector<std::wstring> range = ui_font_setting.getRangeStrings();
		int curSelIdx = -1, idx = 0;
		for (const std::wstring s : range)
		{
			if (s == ui_font_setting.getString())
			{
				curSelIdx = idx;
			}
			list.push_back(ListBoxExItem(s));
			++idx;
		}
		m_pToolBar->SetDropListButtonList(uID_FONT_SIZE, list, curSelIdx);
	}

	// This function is "the" callback for OnChangedSettings - called from CXeUserSettingsForUI
	void _OnChangedSettings(const ChangedSettings& chg_settings)
	{
		// Note - if fonts have changed - we hide 'this' window (and all others) - before processing the change.
		// This is needed because (for some reason) we get (sometimes) endless paint messages - and kills "us".
		bool isFontSettingsChanged = m_xeUI->IsFontSettingsChanged(chg_settings);
		if (isFontSettingsChanged)
		{
			ShowWindow(SW_HIDE);
		}

		m_xeUI->OnChangedSettings(chg_settings);
		m_pToolBar->OnChangedSettings(chg_settings);

		if (isFontSettingsChanged)
		{
			_SetToolBarFontSizeList(ID_FONT_SIZE);

			// Trigger NC size recalculation.
			SetWindowPos(0, 0 /* x */, 0 /* y */, 0 /* cx */, 0 /* cy */,
				SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
		}

		m_pVwMgr->OnChangedSettings(chg_settings);

		if (chg_settings.IsChanged(L"Colors", L"Color"))
		{
			_OnNcPaint(0, 0);
		}

		if (isFontSettingsChanged)	// Show window again - after processing fonts changed in all child windows.
		{
			ShowWindow(SW_SHOW);
			RedrawWindow();
		}
	}
#pragma endregion MiscStuff
};

