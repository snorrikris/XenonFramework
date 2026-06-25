module;

#include "os_minimal.h"
#include <memory>
#include <string>
#include "XeResource.h"
#include "d2d1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma warning(disable:5201)

export module Xe.TabsView;

#pragma warning(default:5201)

import Xe.D2DWndBase;
import Xe.FileVwIF;
import Xe.TabsList;
import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.UserSettingsForUI;
import Xe.FileHelpers;
import Xe.PopupCtrl;
import Xe.Menu;
import Xe.ViewManagerIF;
import Xe.MainFrameIF;
//import Xe.SendMessageToAllOtherAppInstances;
//import Xe.FileContainerIF;
//import Xe.ViewManagerIF_LVS;

constexpr UINT XSCROLL_TIMERID = 1003;		// Mouse wheel x scroll timer - to reset current view visible after scrolling.
constexpr UINT XSCROLL_TIME = 5000;			// ms - time until current view visible reset.

constexpr UINT XSB_LBTN_DOWN_TIMERID	= 1001;		// Auto repeat X scroll timer.
constexpr UINT XSB_LBTN_DOWN_TIME		= 200;		// ms - time to first auto repeat.
constexpr UINT XSB_LBTN_REPT_TIMERID	= 1002;
constexpr UINT XSB_LBTN_REPT_TIME		= 50;		// ms - repeat interval.

export class CXeTabsView : public CXeD2DWndBase, public CXeTabsViewIF
{
protected:
	CXeMainFrameIF* m_pMainFrm = nullptr;

	CXeViewManagerIF* m_pVwMgr = nullptr;

	HWND m_hParentOfViews = nullptr;

	BOOL m_fInitialUpdateDone = FALSE;

	std::unique_ptr<CXeTabsList> m_listTabs;

	BOOL m_fCurrentViewHasFocus = FALSE;

	// Pointer to 'other' tab view - note: is initialized at startup.
	CXeTabsView* m_pOtherView = nullptr;

	ETABVIEWID m_eTabVwId = ETABVIEWID::eAnyTabVw;

	// Dialog control Id of 'this' tab view (when it's visible in the splitter window).
	UINT m_uTabVwDlgCtrlId = 0;

	// Dialog control Id of the currently visible file view in the splitter window.
	UINT m_uViewDlgCtrlId = 0;

	bool m_bUseDataSourceTextFgColorInTab = false;

	HWND m_hOldFocusWnd = 0;	// Handle of window that had focus before view list drop down was shown.

	CPoint m_ptDragStart;
	bool m_isDraggingTab = false;
	dsid_t m_draggedTab;
	bool m_isDragTabOrderChanged = false;

	CPoint m_ptLbtnDown;	// point where L button down (only do btn up event when +/-5px)

	bool m_isLeftXscrollBtnDn = false, m_isRightXscrollBtnDn = false;

#pragma region Creation
public:
	CXeTabsView(CXeMainFrameIF* pMainFrm, CXeViewManagerIF* pVwMgr)
			: CXeD2DWndBase(pVwMgr->GetUIcolors()), m_pVwMgr(pVwMgr), m_pMainFrm(pMainFrm)
	{
		XeASSERT(m_pVwMgr && m_pMainFrm);
		m_bUseDataSourceTextFgColorInTab = s_xeUIsettings[L"GeneralSettings"].Get(L"UseTextColorForFileInTabTitle").getBool();
		m_listTabs = std::make_unique<CXeTabsList>(m_xeUI);
	}
	virtual ~CXeTabsView() {}

	void Create(HWND hParentWnd, ETABVIEWID tabVwId, UINT uTabVwDlgCtrlId, UINT uViewDlgCtrlId,
			CXeTabsView* pOtherTabView)
	{
		XeASSERT(hParentWnd
			&& (tabVwId == ETABVIEWID::ePrimaryTabVw || tabVwId == ETABVIEWID::eSecondaryTabVw));
		m_hParentOfViews = hParentWnd;
		m_eTabVwId = tabVwId;
		m_uTabVwDlgCtrlId = uTabVwDlgCtrlId;
		m_uViewDlgCtrlId = uViewDlgCtrlId;
		m_pOtherView = pOtherTabView;

		std::wstring classname = L"XeTabsView_WNDCLASS";
		m_xeUI->RegisterWindowClass(classname, D2DCtrl_WndProc);
		HWND hWnd = CreateD2DWindow(0, classname.c_str(), nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
				CRect(), hParentWnd, 0, true);
		m_listTabs->OnTabViewCreated(m_hParentOfViews, m_uViewDlgCtrlId);
		m_fInitialUpdateDone = TRUE;
	}
#pragma endregion Creation

#pragma region Misc
public:
	virtual HWND GetTabWindowHandle() const override { return Hwnd(); }

	virtual void RedrawTabView() override
	{
		_RedrawDirectly();
	}
	//int RecalculateTabViewHeight(int cxAvailable)
	//{
	//	m_listTabs->CalculateUI(cxAvailable);
	//	return GetTabViewHeight();
	//}

	// Called from LVSState when view with focus has changed.
	virtual void ViewWithFocusChanged(dsid_t dwDataSourceId) override
	{
		if (m_listTabs->UpdateIdOfViewWithFocus(dwDataSourceId))
		{
			_RedrawDirectly();
		}
	}

	virtual void OnViewRenamed(CXeFileVwIF* pView) override
	{
		_RecalculateUIandRepositionWindowsIfNeeded();
	}

	virtual void SendMessageToAllViews(UINT uMessage, WPARAM wParam, LPARAM lParam) override
	{
		m_listTabs->SendMessageToAllViews(uMessage, wParam, lParam);
		SendMessageW(uMessage, wParam, lParam);

		if (uMessage == WMU_NOTIFY_CHANGES)
		{
			switch (wParam)
			{
			case NFCHG_CODE_SEL_THEME:
			case NFCHG_CODE_LOGFILE_COLORS_CHANGED:
				m_bUseDataSourceTextFgColorInTab = s_xeUIsettings[L"GeneralSettings"].Get(L"UseTextColorForFileInTabTitle").getBool();
				_RedrawDirectly();
				break;
			}
		}
	}

	virtual void SetTabProgressIndication(dsid_t dwDataSourceId, UINT uProgress) override
	{
		_RedrawDirectly();
	}

	virtual void ClearProgressOnAllTabs() override
	{
		_RedrawDirectly();
	}

	void OnChangedSettings(const ChangedSettings& chg_settings)
	{
		std::vector<CXeFileVwIF*> allViews = GetAllViews();
		for (CXeFileVwIF* pView : allViews)
		{
			pView->OnChangedSettings(chg_settings);
		}
		if (chg_settings.IsChanged(L"GeneralSettings", L"UseTextColorForFileInTabTitle")
				|| m_xeUI->IsFontSettingsChanged(chg_settings))
		{
			m_bUseDataSourceTextFgColorInTab = s_xeUIsettings[L"GeneralSettings"].Get(L"UseTextColorForFileInTabTitle").getBool();
			m_listTabs->CalculateUI(m_listTabs->GetTabViewWidth());	// Recalculate tab row height.
			_RecalculateUIandRepositionWindowsIfNeeded();
		}
		if (chg_settings.IsChanged(L"Colors", L"Color"))
		{
			_RedrawDirectly();
		}
	}

protected:
	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{
		//UINT nType = (UINT)wParam;
		int cx = GET_X_LPARAM(lParam);
		//int cy = GET_Y_LPARAM(lParam);
		if (m_fInitialUpdateDone)	// Initialized?
		{
			m_listTabs->CalculateUI(cx);
			m_listTabs->MakeCurrentTabVisible(_GetCurrentView());
			_RedrawDirectly();
		}
		return CXeD2DWndBase::_OnSize(hWnd, wParam, lParam);
	}

	// WM_TIMER message handler.
	virtual LRESULT _OnTimer(WPARAM ww, LPARAM ll) override
	{
		UINT_PTR nIDEvent = ww;
		XeASSERT(::IsWindow(Hwnd()));
		if (nIDEvent == XSCROLL_TIMERID)
		{
			_OnXscrollTimer();
			return 0;
		}
		if (nIDEvent == XSB_LBTN_DOWN_TIMERID)
		{	// First auto repeat timer event.
			KillTimer(XSB_LBTN_DOWN_TIMERID);
			SetTimer(XSB_LBTN_REPT_TIMERID, XSB_LBTN_REPT_TIME, 0);
		}
		if (nIDEvent == XSB_LBTN_DOWN_TIMERID || nIDEvent == XSB_LBTN_REPT_TIMERID)
		{	// Auto repeat
			_DoXscroll(m_isLeftXscrollBtnDn);
		}
		return CXeD2DWndBase::_OnTimer(ww, ll);
	}
#pragma endregion Misc

#pragma region Info
public:
	//bool IsVisible() const { return ::GetDlgCtrlID(Hwnd()) == m_uTabVwDlgCtrlId; }

	int GetTabViewHeight() const { return m_listTabs->GetTabViewHeight(); }

	int GetNumTabRows() const { return m_listTabs->GetNumTabRows(); }

	virtual ETABVIEWID GetTabVwId() const override { return m_eTabVwId; }

	// Return true if point (in screen coords.) is in a tab view or file view else false.
	virtual bool IsPointInThisView(POINT& pt) const override
	{
		CXeFileVwIF* pView = _GetCurrentView();
		bool isPtInFileVw = pView ? pView->IsPointInThisView(pt) : false;
		CRect rcTab = _GetClientRect();
		ClientToScreen(rcTab);
		bool isPtInTabVw = rcTab.PtInRect(pt) != FALSE;
		return isPtInTabVw || isPtInFileVw;
	}

	//bool IsChildOfCurrentView(HWND hWnd) const
	//{
	//	CXeFileVwIF* pCurView = _GetCurrentView();
	//	HWND hWndVw = pCurView ? pCurView->GetHwndOfView() : nullptr;
	//	return hWndVw ? ::IsChild(hWndVw, hWnd) : false;
	//}

	//bool IsTabPinned(dsid_t dataSourceId) const { return m_listTabs->IsTabPinned(dataSourceId); }

	virtual UINT GetIndexOfCurrentTab() const override
	{
		return m_listTabs->GetIndexOfCurrentTab();
	}

	virtual int GetTabCount() const override { return m_listTabs->GetTabCount(); }

	int GetTotalTabCount() const { return GetTabCount() + (m_pOtherView ? m_pOtherView->GetTabCount() : 0); }

	virtual std::vector<CVwInfo> GetAllTabs() const override { return m_listTabs->GetAllTabs(); }

	virtual std::vector<CXeFileVwIF*> GetAllViews() const override
	{
		std::vector<CXeFileVwIF*> views;
		for (auto item : m_listTabs->GetAllTabs())
		{
			views.push_back(item.m_pView);
		}
		return views;
	}

	virtual const CVwInfo* FindView(CXeFileVwIF* pView) const override
	{
		return m_listTabs->GetTab(pView);
	}

	virtual CXeFileVwIF* FindView(dsid_t dwDataSourceId) const override
	{
		const CVwInfo* pTabInfo = m_listTabs->GetTabFromDataSourceId(dwDataSourceId);
		if (pTabInfo)
		{
			return pTabInfo->m_pView;
		}
		return nullptr;
	}

	virtual CXeFileVwIF* GetCurrentView() const override { return _GetCurrentView(); }

	virtual dsid_t GetDataSourceIdOfCurrentView() const override
	{
		CXeFileVwIF* pCurView = _GetCurrentView();
		return pCurView ? pCurView->GetDataSourceId() : dsid_t();
	}

protected:
	CXeFileVwIF* _GetCurrentView() const
	{	// view is in row below tab view.
		HWND hWndCurVw = ::GetDlgItem(m_hParentOfViews, m_uViewDlgCtrlId);
		const CVwInfo* pVwNfo = m_listTabs->GetTab(hWndCurVw);
		return pVwNfo ? pVwNfo->m_pView : nullptr;
	}

	dsid_t _GetCurrentViewId() const
	{
		CXeFileVwIF* pCurrentView = _GetCurrentView();
		return pCurrentView ? pCurrentView->GetDataSourceId() : dsid_t();
	}

	// Parameter: point is in 'this' window client coordinates.
	bool _IsPointInOtherTabView(CPoint point) const
	{
		if (m_pOtherView)
		{
			CRect rcOtherView;
			m_pOtherView->GetClientRect(rcOtherView);
			m_pOtherView->ClientToScreen(rcOtherView);
			CPoint ptScreen = point;
			ClientToScreen(&ptScreen);
			return rcOtherView.PtInRect(ptScreen) == TRUE;
		}
		return false;
	}

	const CVwInfo* _GetTabAtPointInOtherView(CPoint point) const
	{
		if (m_pOtherView)
		{
			CPoint ptOtherView = point;
			ClientToScreen(&ptOtherView);
			m_pOtherView->ScreenToClient(&ptOtherView);
			const CVwInfo* pTabInfo = m_pOtherView->m_listTabs->GetTabAtPoint(ptOtherView);
			if (pTabInfo)
			{
				return pTabInfo;
			}
		}
		return nullptr;
	}

	dsid_t _GetTabInsertDS_InOtherView(CPoint point) const
	{
		if (m_pOtherView)
		{
			CPoint ptOtherView = point;
			ClientToScreen(&ptOtherView);
			m_pOtherView->ScreenToClient(&ptOtherView);
			if (m_pOtherView->m_listTabs->m_rcLeftTabListDropDownBtn.PtInRect(ptOtherView))
			{
				return dsid_t();	// dsid 0 => insert at front.
			}
			if (m_pOtherView->m_listTabs->IsPointInXscrollButtons(ptOtherView))
			{
				return dsid_t::MakeDSID(0xFFFFFFFF);	// => insert at back.
			}
			const CVwInfo* pTabInfo = _GetTabAtPointInOtherView(point);
			if (pTabInfo)
			{
				return pTabInfo->m_dsid;
			}
			// Note - point is not in any tab nor in drop down buttons - so insert point is assumed at back.
		}
		return dsid_t::MakeDSID(0xFFFFFFFF);	// => insert at back.
	}

	bool _CanMoveToOtherView() const
	{
		if (GetTotalTabCount() <= 1)
		{
			return false;
		}
		return true;
	}
#pragma endregion Info

#pragma region View_Management
public:
	// Add view to tab list, viewParams.makeThisCurrentView = true to make it the current view,
	// setFocusToView = true to set input focus to view (only when makeCurrentTab = true),
	// viewParams.insertBeforeDSid = -1 to add at the end, 0 to add at the begining, 
	// or datasource Id of tab to insert before.
	virtual bool AttachView(CXeFileVwIF* pView, CreateViewParams viewParams, bool setFocusToView) override
	{
		bool setAsCurrentView = viewParams.makeThisCurrentView || m_listTabs->GetTabCount() == 0;

		_AttachView(pView, viewParams.insertBeforeDSid, setAsCurrentView, setFocusToView);

		//XeTRACE("AttachView : %s - DSID: %u eTabVwId: %d, insertBeforeDSid: %d, makeFirstOpenedCurrentView: %d, gotoLastRowInGrid: %d\r\n",
		//		xet::to_astr(pView->GetViewName()).c_str(), pView->GetDataSourceId(), viewParams.eTabVwId, viewParams.insertBeforeDSid, viewParams.makeFirstOpenedCurrentView, viewParams.gotoLastRowInGrid);

		return true;
	}

	virtual void CloseAndDeleteTabs(const std::vector<CVwInfo>& tabsList) override
	{
		for (const CVwInfo& tabInfo : tabsList)
		{
			_DeleteTabAndView(tabInfo.m_pView);
		}
	}

	virtual void DeleteAllTabsAndViews() override
	{
		_DeleteAllTabsAndViews();
	}

	virtual bool FindAndDeleteTabAndView(dsid_t dwDataSourceId) override
	{
		const CVwInfo* pTabInfo = m_listTabs->GetTabFromDataSourceId(dwDataSourceId);
		if (pTabInfo)
		{
			_DeleteTabAndView(pTabInfo->m_pView);
			return true;
		}
		return false;
	}

	virtual bool SwitchToView(CXeFileVwIF* pView, bool setFocusToView) override
	{
		XeASSERT(pView);
		//XeTRACE("TabId=%d, SwitchToView : %s, setFocusToView: %d\r\n", m_eTabVwId, xet::to_astr(pView->GetViewName()).c_str(), setFocusToView);
		if (!m_listTabs->GetTab(pView))	// Is view found in the tab list?
		{
			XeASSERT(false);
			return false;
		}
		_SwitchViews(pView, setFocusToView);
		_RecalculateUIandRepositionWindowsIfNeeded();
		return true;
	}

	virtual bool SwitchToView(dsid_t dwDataSourceId, bool setFocusToView) override
	{
		//TRACE("SwitchToView : dwDataSourceId: %d, setFocusToView: %d\r\n", dwDataSourceId, setFocusToView);
		const CVwInfo* pTabInfo = m_listTabs->GetTabFromDataSourceId(dwDataSourceId);
		XeASSERT(pTabInfo);
		return pTabInfo ? SwitchToView(pTabInfo->m_pView, setFocusToView) : false;
	}

	virtual bool SwitchToViewAtIndex(UINT uIndex) override
	{
		//TRACE("SwitchToView : uIndex: %d\r\n", uIndex);
		const CVwInfo* pTabInfo = m_listTabs->GetTabAtIndex(uIndex);
		XeASSERT(pTabInfo);
		return pTabInfo ? SwitchToView(pTabInfo->m_pView, true) : false;
	}

	virtual bool SwitchToView(ViewNavigate navigate) override
	{
		CXeFileVwIF* pCurView = _GetCurrentView();
		if (pCurView)
		{
			UINT uIdx = m_listTabs->GetIndexOfTab(pCurView);
			if (navigate == ViewNavigate::Prev && uIdx > 0)
			{
				return SwitchToViewAtIndex(uIdx - 1);
			}
			else if (navigate == ViewNavigate::Next)
			{
				return SwitchToViewAtIndex(uIdx + 1);
			}
		}
		return false;
	}

protected:
	void _AttachView(CXeFileVwIF* pView, dsid_t insertBeforeDataSourceId, bool setAsCurrentView, bool setFocusToView)
	{
		_HideView(pView);	// Ensure view is hidden.

		::SetParent(pView->GetHwndOfView(), m_hParentOfViews);

		bool isAttachingFirstView = m_listTabs->GetTabCount() == 0;
		if (isAttachingFirstView)
		{
			_ShowThisTabView();	// Make 'this' window visible and set 'our' dlg control ID on it.
		}

		m_listTabs->AddTab(CVwInfo(pView), insertBeforeDataSourceId);

		if (setAsCurrentView)
		{
			_SwitchViews(pView, setFocusToView);
		}

		_RecalculateUIandRepositionWindowsIfNeeded();
	}

	// Move view to other tab view.
	// selViewId = view to move.
	// insertBeforeDataSourceId = data source ID of view in other tab view that pSelView will be inserted before,
	// = 0 : is inserted at front, -1 : is inserted at back of tab list.
	void _MoveToOtherView(dsid_t selViewId, dsid_t insertBeforeDataSourceId)
	{
		CXeFileVwIF* pSelView = m_listTabs->GetTabViewPtr(selViewId);
		XeASSERT(m_pOtherView && pSelView);

		if (pSelView == _GetCurrentView())
		{
			_HideView(pSelView);
		}

		// Remove tab from tab control.
		CXeFileVwIF* pNextView = m_listTabs->RemoveTab(pSelView);

		/////////////////////////////////////////////////////////////////////////////////////
		// Move to other view

		// Add view to other tab view.
		m_pOtherView->_AttachView(pSelView, insertBeforeDataSourceId, true, false);

		/////////////////////////////////////////////////////////////////////////////////////

		if (pNextView)
		{	// Switch to next view
			_ShowView(pNextView, true);
		}

		_RecalculateUIandRepositionWindowsIfNeeded();

		m_pVwMgr->OnTabOrderChanged();	// Notify LVSState that tab order has changed.
	}

	void _MoveTab(EMOVETAB eMove, CXeFileVwIF* pViewToMove)
	{
		if (!m_listTabs->MoveTab(eMove, pViewToMove))
		{
			return;
		}

		_RecalculateUIandRepositionWindowsIfNeeded();
		m_pVwMgr->OnTabOrderChanged();	// Notify LVSState that tab order has changed.
	}

	void _MoveTab(CXeFileVwIF* pViewToMove, CXeFileVwIF* pViewAt)
	{
		if (!m_listTabs->MoveTab(pViewToMove, pViewAt))
		{
			return;
		}

		_RecalculateUIandRepositionWindowsIfNeeded();
		m_pVwMgr->OnTabOrderChanged();	// Notify LVSState that tab order has changed.
	}

	void _DeleteTabAndView(CXeFileVwIF* pDelView)
	{
		// Note - the view being deleted is not necessarily the current view
		// (i.e. the user can close a view that is not current).
		XeASSERT(pDelView);
		dsid_t dwDataSourceIdOfClosingView = pDelView->GetDataSourceId();

		m_pVwMgr->OnViewClosing(dwDataSourceIdOfClosingView);

		CXeFileVwIF* pCurrentView = _GetCurrentView();
		bool isDelViewCurrent = pDelView == pCurrentView;

		CXeFileVwIF* pNextView = m_listTabs->RemoveTab(pDelView);	// Remove tab from tab control.
		if (isDelViewCurrent)
		{
			if (pNextView)
			{
				bool setFocusToView = isDelViewCurrent;
				_SwitchViews(pNextView, true);
			}
			else
			{
				_HideView(pDelView);
			}
		}

		::DestroyWindow(pDelView->GetHwndOfView());

		m_pVwMgr->OnViewClosed(dwDataSourceIdOfClosingView);

		if (m_listTabs->GetTabCount() == 0)					// Last view was removed?
		{
			_OnLastTabRemoved();
		}
		_RecalculateUIandRepositionWindowsIfNeeded();	// Note - does nothing if 'this' view not visible.
	}

	void _DeleteAllTabsAndViews()
	{
		// Notify LVSState that views are closing.
		std::vector<CVwInfo> tabInfos = m_listTabs->GetAllTabs();
		for (CVwInfo& tabInfo : tabInfos)
		{
			XeASSERT(tabInfo.m_pView && tabInfo.m_pView->GetHwndOfView());
			m_pVwMgr->OnViewClosing(tabInfo.m_dsid);
		}

		CXeFileVwIF* pCurView = _GetCurrentView();
		if (pCurView)
		{
			_HideView(pCurView);
		}

		std::vector<CVwInfo> tabList = m_listTabs->RemoveAllTabs();
		XeASSERT(tabList.size() == tabInfos.size());

		// Destroy all views.
		for (CVwInfo& tabInfo : tabList)
		{
			::DestroyWindow(tabInfo.m_pView->GetHwndOfView());
		}

		// Notify ViewManager that views are closed (ViewManager will delete the view object).
		for (CVwInfo& tabInfo : tabList)
		{
			// Note - LVSState will notify CXeFilesManager - that will close and delete the (log file) container object.
			m_pVwMgr->OnViewClosed(tabInfo.m_dsid);
		}

		_OnLastTabRemoved();	// All tabs have been removed from 'this'.
		
		_RecalculateUIandRepositionWindowsIfNeeded();	// Note - does nothing if 'this' view not visible.
	}

	// Called when all tabs have been removed from 'this' tab view.
	void _OnLastTabRemoved()
	{
		if (m_eTabVwId == ETABVIEWID::ePrimaryTabVw)	// If 'this' is Primary tab view?
		{
			if (m_pOtherView->GetTabCount() > 0)		// Other view has tabs?
			{
				_MoveAllTabsFromOtherTabViewToThis();	// Move all tabs from other view to 'this'.
			}
			else
			{
				_HideThisTabView();						// else - hide 'this' tab view.
			}
		}
		else
		{
			_HideThisTabView();							// 'this' is other view with no tabs - hide it.
		}
	}

	// Remove all tabs, used when moving all tabs in secondary tab view to the primary tab view.
	std::vector<CVwInfo> _RemoveAllTabs()
	{
		XeASSERT(m_eTabVwId == ETABVIEWID::eSecondaryTabVw && m_listTabs->GetTabCount() > 0);
		if (!(m_eTabVwId == ETABVIEWID::eSecondaryTabVw && m_listTabs->GetTabCount() > 0))
		{
			return std::vector<CVwInfo>();
		}
		_HideView(_GetCurrentView());
		std::vector<CVwInfo> tabList = m_listTabs->RemoveAllTabs();
		_HideThisTabView();
		return tabList;
	}

	void _MoveAllTabsFromOtherTabViewToThis()
	{
		// 'this' is the primary tab view that is now empty AND the secondary tab view is visible (and has tabs)
		XeASSERT(m_eTabVwId == ETABVIEWID::ePrimaryTabVw && m_listTabs->GetTabCount() == 0);
		CXeFileVwIF* pCurView = m_pOtherView->_GetCurrentView();
		std::vector<CVwInfo> tabList = m_pOtherView->_RemoveAllTabs();
		// Note - the secondary tab view is now invisible.
		XeASSERT(tabList.size() > 0 && pCurView);
		for (CVwInfo& tabinfo : tabList)
		{
			tabinfo.ClearUIvars();
			m_listTabs->AddTab(tabinfo, dsid_t());	// Add tab info struct to list.

			::SetParent(tabinfo.m_pView->GetHwndOfView(), m_hParentOfViews);
		}
		_ShowView(pCurView, true);
		m_listTabs->RecalculateUI();
		m_listTabs->MakeCurrentTabVisible(_GetCurrentView());
		m_pMainFrm->RecalculateWindowsRects();
		_SetThisTabViewPosition();
		_SetCurrentViewPosition();
	}

	void _PinTab(const CVwInfo* pTabInfo)
	{
		if (pTabInfo && m_listTabs->PinTab(!pTabInfo->m_isPinned, pTabInfo->m_pView))
		{
			_RecalculateUIandRepositionWindowsIfNeeded();
			m_pVwMgr->OnTabOrderChanged();
		}
	}

	void _RecalculateUIandRepositionWindowsIfNeeded()
	{
		if (::GetDlgCtrlID(Hwnd()) == m_uTabVwDlgCtrlId)	// Is 'this' view visible?
		{
			bool isTabHeightChanged = m_listTabs->RecalculateUI();
			CXeFileVwIF* pCurView = _GetCurrentView();
			if (pCurView)
			{
				m_listTabs->MakeCurrentTabVisible(pCurView);
			}
			if (isTabHeightChanged)
			{
				// Height needed for tabs has changed.
				m_pMainFrm->RecalculateWindowsRects();
				_SetThisTabViewPosition();
				_SetCurrentViewPosition();
			}
			else
			{
				_RedrawDirectly();
			}
		}
	}

	// Make 'this' window invisible and set dlg control id on it to zero.
	void _HideThisTabView()
	{
		::ShowWindow(Hwnd(), SW_HIDE);
		::SetWindowLong(Hwnd(), GWL_ID, 0);	// Make 'this' tab view invisible.
		m_pMainFrm->RecalculateWindowsRects();
	}

	// Make 'this' window visible and set 'our' dlg control id on it.
	// Note - when the main frame is calculating the window sizes
	// - it will 'see' a view as 'visible' when it has the proper dlg control ID set.
	void _ShowThisTabView()
	{
		::SetWindowLong(Hwnd(), GWL_ID, m_uTabVwDlgCtrlId);	// Make 'this' tab view visible.
		::ShowWindow(Hwnd(), SW_SHOW);
		m_pMainFrm->RecalculateWindowsRects();
	}

	void _SetThisTabViewPosition()
	{
		HWND hWndtabVw = ::GetDlgItem(m_hParentOfViews, m_uTabVwDlgCtrlId);
		XeASSERT(hWndtabVw && hWndtabVw == Hwnd());
		CRect rcTabVw = m_pMainFrm->GetViewWindowRect(m_uTabVwDlgCtrlId);
		::SetWindowPos(hWndtabVw, HWND_TOP, rcTabVw.left, rcTabVw.top, rcTabVw.Width(), rcTabVw.Height(), SWP_NOCOPYBITS | SWP_SHOWWINDOW);
	}

	void _SetCurrentViewPosition()
	{
		HWND hWndCurVw = ::GetDlgItem(m_hParentOfViews, m_uViewDlgCtrlId);
		XeASSERT(hWndCurVw);
		CRect rcVw = m_pMainFrm->GetViewWindowRect(m_uViewDlgCtrlId);
		::SetWindowPos(hWndCurVw, HWND_TOP, rcVw.left, rcVw.top, rcVw.Width(), rcVw.Height(), SWP_NOCOPYBITS | SWP_SHOWWINDOW);
	}

	void _HideView(const CXeFileVwIF* pView)
	{
		XeASSERT(pView && pView->GetHwndOfView());
		HWND hWndVw = pView->GetHwndOfView();
		::SetWindowLong(hWndVw, GWL_ID, 0);	// Set Control ID.
		::ShowWindow(hWndVw, SW_HIDE);
	}

	void _ShowView(const CXeFileVwIF* pView, bool isSetFocusToView = false)
	{
		XeASSERT(pView && pView->GetHwndOfView());
		if (!pView) { return; }
		HWND hWndVw = pView->GetHwndOfView();
		::SetWindowLong(hWndVw, GWL_ID, m_uViewDlgCtrlId);	// Set Control ID.
		_SetCurrentViewPosition();
		if (isSetFocusToView)
		{
			pView->SetFocusToView();
		}

		// Sanity check.
		const std::vector<CVwInfo>& tabList = m_listTabs->GetTabList();
		for (const CVwInfo& tabInfo : tabList)
		{
			XeASSERT(tabInfo.m_pView && tabInfo.m_pView->GetHwndOfView());
			HWND hWndView = tabInfo.m_pView->GetHwndOfView();
			if (::IsWindowVisible(hWndView))
			{
				XeASSERT(hWndVw == hWndView);	// Only ONE view should be visible.
			}
		}
	}

	void _SwitchViews(CXeFileVwIF* pView, bool setFocusToView)
	{
		CXeFileVwIF* pCurrentView = _GetCurrentView();
		if (pView == pCurrentView)	// View already current view?
		{
			if (setFocusToView)
			{
				pView->SetFocusToView();
			}
			return;
		}

		// Switch views

		if (pCurrentView)
		{
			_HideView(pCurrentView);		// Hide current view
		}
	
		_ShowView(pView, setFocusToView);	// Show selected view
	}
#pragma endregion View_Management

#pragma region Painting
protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override
	{
		XeASSERT(m_xeUI);

		pRT->Clear(m_xeUI->GetColorF(CID::TabBg)); // Fill background
		_DrawTabsViewUI(pRT);
	}

	CRect _GetTabsRect()
	{
		CRect rcTabs = _GetClientRect();
		rcTabs.left = m_listTabs->m_rcLeftTabListDropDownBtn.right;
		rcTabs.right = m_listTabs->m_rcLeftXscrollBtn.left;
		return rcTabs;
	}

	void _DrawTabsViewUI(ID2D1RenderTarget* pRT)
	{
		CRect rcCli = _GetClientRect();
		m_listTabs->m_curVwId = _GetCurrentViewId();
		int cxVw = rcCli.Width(), cyVw = rcCli.Height();
		pRT->DrawLine({ 0.0f, (float)(cyVw - 1) }, { (float)cxVw, (float)(cyVw - 1) }, GetBrush(CID::TabBtmBorder));

		int cyTabRow = m_listTabs->GetTabViewRowHeight();
		D2D1_RECT_F rcClipF = RectFfromRect(_GetTabsRect());
		rcClipF.top = rcClipF.bottom - (float)cyTabRow;

		// Draw tabs
		bool isClippingApplied = false;
		int lastRow = m_listTabs->GetNumTabRows() - 1;
		int xOffset = m_listTabs->GetXoffsetForPaint();
		CRect rcTest;
		for (CVwInfo& tabInfo : m_listTabs->m_list)
		{
			if (m_listTabs->m_isOneRowUI || tabInfo.m_isPinned)
			{
				_DrawTab(pRT, tabInfo, 0);
			}
			else
			{
				if (!isClippingApplied && tabInfo.m_nRow == lastRow)
				{
					pRT->PushAxisAlignedClip(rcClipF, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
					isClippingApplied = true;
				}

				if (rcCli.IsIntersectRectWithOffset(tabInfo.m_rcTab, -xOffset, 0))
				{
					_DrawTab(pRT, tabInfo, xOffset);
				}
			}
		}

		if (isClippingApplied)
		{
			pRT->PopAxisAlignedClip();
		}

		// Draw Left Tab list drop down button.
		_DrawChevronButton(pRT, RectFfromRect(m_listTabs->m_rcLeftTabListDropDownBtn),
				m_listTabs->m_isMouseOverLeftTabListDropDownBtn, CID::TabBg);
		CRect rcBtnSpcR(m_listTabs->m_rcLeftXscrollBtn);
		rcBtnSpcR.bottom = cyVw;
		rcBtnSpcR.top = rcBtnSpcR.bottom - cyTabRow;
		rcBtnSpcR.left -= 2;
		pRT->FillRectangle(RectFfromRect(rcBtnSpcR), GetBrush(CID::TabBg));

		if (!m_listTabs->m_isOneRowUI)
		{
			CID cidXL = m_listTabs->m_isMouseOverLeftXscrollButton ? CID::CtrlCurItemBg : CID::CtrlBgDis;
			_DrawScrollbarButton(pRT, RectFfromRect(m_listTabs->m_rcLeftXscrollBtn), cidXL, BtnTp::left);
			CID cidXR = m_listTabs->m_isMouseOverRightXscrollButton ? CID::CtrlCurItemBg : CID::CtrlBgDis;
			_DrawScrollbarButton(pRT, RectFfromRect(m_listTabs->m_rcRightXscrollBtn), cidXR, BtnTp::right);
		}

		CRect rcFocusLine(0, cyVw - 2, cxVw, cyVw);
		pRT->FillRectangle(RectFfromRect(rcFocusLine),
				GetBrush(m_listTabs->m_isFocusVwInThis ? CID::TabFocusBg : CID::TabCurBg));
	}

	void _DrawTab(ID2D1RenderTarget* pRT, const CVwInfo& tabInfo, int xOffset)
	{
		CRect rc = tabInfo.m_rcTab;
		rc.OffsetRect(-xOffset, 0);
		rc.left += 2;

		bool isCurrentTab = tabInfo.m_dsid == m_listTabs->m_curVwId;
		bool isAlertState = false; // ui_ds->GetErrorInfo().m_isAlertState;
		CID tabBg = isCurrentTab ? CID::ActTabTitleBg : CID::TabTitleBg;
		if (isAlertState)
		{
			CRect rcT2 = rc;
			rcT2.left += 1;
			rcT2.right -= 1;
			rcT2.bottom -= 1;
			rcT2.top += 4;
			pRT->FillRectangle(RectFfromRect(rcT2), GetBrush(CID::RebuildMergeBg));
			tabBg = CID::RebuildMergeBg;
		}
		else
		{
			CRect rcTab(tabInfo.m_rcTab);
			rcTab.OffsetRect(-xOffset, 0);
			rcTab.left += 2;
			rcTab.right += 2;
			rcTab.top += 4;
			pRT->FillRectangle(RectFfromRect(rcTab), GetBrush(tabBg));
		}

		EXE_FONT font = isCurrentTab || tabInfo.m_bIsMouseOver ? EXE_FONT::eUI_FontBold : EXE_FONT::eUI_Font;
		bool isUseFileColor = m_bUseDataSourceTextFgColorInTab && !isAlertState;
		COLORREF rgbTxt = tabInfo.m_pView->GetViewTitleTextColor();
		CRect rcProg;
		//if (ctx.m_progress > 0 && !(ctx.m_progressState == EPROGRESSSTATE::eDone
		//	|| ctx.m_progressState == EPROGRESSSTATE::eDoneParseFailed))
		//{
		//	INT32 progress = std::min(ctx.m_progress, (INT32)100);
		//	rcProg = rc;
		//	rcProg.right = rcProg.left + (int)((double)(rc.Width() * progress) / 100 + 0.5);
		//}
		PID pid = tabInfo.m_pView->GetViewPID();
		CRect rcIcnTxt(rc);
		rcIcnTxt.OffsetRect(4, 0);
		_DrawIconAndFilename(pRT, tabInfo.m_pView->GetViewName(), rcIcnTxt, font, rgbTxt, pid, true, false, rcProg);

		// Draw close button if 'this' is current tab or mouse over tab.
		CRect rcPinBtn = tabInfo.m_rcPinBtn;
		rcPinBtn.OffsetRect(-xOffset, 0);
		if (isCurrentTab || tabInfo.m_bIsMouseOver)
		{
			CRect rcCloseBtn = tabInfo.m_rcCloseBtn;
			rcCloseBtn.OffsetRect(-xOffset, 0);
			_DrawCloseButton(pRT, RectFfromRect(rcCloseBtn), tabInfo.m_bIsMouseOverCloseBtn, tabBg);
			if (tabInfo.m_bIsMouseOver || tabInfo.m_isPinned)
			{
				_DrawPinButton(pRT, RectFfromRect(rcPinBtn), tabInfo.m_isPinned, tabInfo.m_bIsMouseOverPinBtn, tabBg);
			}
		}
		else if (tabInfo.m_isPinned)
		{
			_DrawPinButton(pRT, RectFfromRect(rcPinBtn), true, false, tabBg);
		}

		if (isCurrentTab)
		{
			// Draw thick line at top of tab to indicate "current" tab.
			CRect rcCurTab(rc.left, rc.top + 3, rc.right, rc.top + 6);
			pRT->FillRectangle(RectFfromRect(rcCurTab),
					GetBrush(m_listTabs->m_isFocusVwInThis ? CID::TabFocusBg : CID::TabCurBg));
		}
	}
#pragma endregion Painting

#pragma region Mouse_Processing
public:
	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam) override
	{
		if (m_isDraggingTab)
		{
			SetCursor(::LoadCursor(NULL, IDC_UPARROW));
		}
		else
		{
			SetCursor(m_xeUI->GetAppCursor());
		}
		return TRUE;
	}

	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint point) override
	{
		SetCapture();
		m_ptLbtnDown = point;
		m_isLeftXscrollBtnDn = m_listTabs->m_rcLeftXscrollBtn.PtInRect(point);
		m_isRightXscrollBtnDn = m_listTabs->m_rcRightXscrollBtn.PtInRect(point);
		if (m_isLeftXscrollBtnDn || m_isRightXscrollBtnDn)
		{
			_DoXscroll(m_isLeftXscrollBtnDn);
			SetTimer(XSB_LBTN_DOWN_TIMERID, XSB_LBTN_DOWN_TIME, 0);
			return 0;
		}

		if (m_listTabs->m_rcLeftTabListDropDownBtn.PtInRect(point))
		{
			return 0;	// Tab list drop down button click is handled in LbtnUp
		}

		CVwInfo* pTabInfo = m_listTabs->GetTabAtPoint(point);
		if (pTabInfo)
		{
			if (nFlags == MK_LBUTTON)	// L button only?
			{
				m_ptDragStart = point;
				m_draggedTab = pTabInfo->m_dsid;
				m_isDragTabOrderChanged = false;
			}
		}
		return 0;
	}

	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint point) override
	{
		ReleaseCapture();
		KillTimer(XSB_LBTN_DOWN_TIMERID);
		KillTimer(XSB_LBTN_REPT_TIMERID);
		m_isLeftXscrollBtnDn = m_isRightXscrollBtnDn = false;

		if (!m_isDraggingTab)
		{
			if (std::abs(point.x - m_ptLbtnDown.x) > 5 || std::abs(point.y - m_ptLbtnDown.y) > 5)
			{
				return 0;	// Ignore button up event if to far away from where L button down.
			}
			if (m_listTabs->m_rcLeftTabListDropDownBtn.PtInRect(point))
			{
				_OnTabListButton();
				return 0;
			}
		}

		if (m_isDraggingTab)
		{
			m_isDraggingTab = false;
			if (_IsPointInOtherTabView(point))
			{
				dsid_t insertBeforeDSid = _GetTabInsertDS_InOtherView(point);
				_MoveToOtherView(m_draggedTab, insertBeforeDSid);
				m_isDragTabOrderChanged = true;
			}
			if (m_isDragTabOrderChanged)
			{
				m_pVwMgr->OnTabOrderChanged();
			}
			m_draggedTab.reset();
			m_isDragTabOrderChanged = false;
			m_ptDragStart.SetPoint(0, 0);
		}
		else
		{
			bool isPointOverCloseButton, isPointOverPinButton;
			CVwInfo* pTabInfo = m_listTabs->GetTabAtPoint(point, &isPointOverCloseButton, &isPointOverPinButton);
			if (pTabInfo)
			{
				XeASSERT(pTabInfo->m_pView && pTabInfo->m_pView->GetHwndOfView());

				bool isCurrentView = pTabInfo->m_pView == _GetCurrentView();
				if (isPointOverPinButton)
				{
					_PinTab(pTabInfo);
				}
				else if (!isPointOverCloseButton)	// Cursor not over close button?
				{
					SwitchToView(pTabInfo->m_pView, true);
				}
				else
				{
					if (abs(m_ptDragStart.x - point.x) < 5		// Mouse is close to where Lbtn clicked
							&& isPointOverCloseButton)			// AND Cursor over close button?
					{
						_DeleteTabAndView(pTabInfo->m_pView);
					}
				}
			}
		}
		return 0;
	}

	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint point) override
	{
		return 0;
	}

	virtual LRESULT _OnRightDown(UINT nFlags, CPoint point) override
	{
		bool isPointOverCloseButton;
		CVwInfo* pTabInfo = m_listTabs->GetTabAtPoint(point, &isPointOverCloseButton);
		if (pTabInfo)
		{
			if (isPointOverCloseButton)
			{
				return 0;
			}
			XeASSERT(pTabInfo->m_pView->GetHwndOfView());
			if (pTabInfo->m_pView != _GetCurrentView())	// Only switch when differrent
			{
				SwitchToView(pTabInfo->m_pView, true);
			}

			_OnTabContextMenu(point, pTabInfo->m_pView);
		}
		return 0;
	}

	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{
		if (nFlags == MK_LBUTTON)	// L button only?
		{
			if (m_isDraggingTab)
			{
				_DoMouseOverDragTab(point);
			}
			else if (!(m_isLeftXscrollBtnDn || m_isRightXscrollBtnDn))
			{
				CPoint ptDiff = m_ptDragStart - point;
				CXeFileVwIF* pDraggedTabVw = m_listTabs->GetTabViewPtr(m_draggedTab);
				if (pDraggedTabVw && (abs(ptDiff.x) >= 10 || abs(ptDiff.y) >= 10))
				{
					SwitchToView(pDraggedTabVw, true);
					m_isDraggingTab = true;
					SetCursor(::LoadCursor(NULL, IDC_UPARROW));
				}
			}
		}
		else if (nFlags == 0)
		{
			bool isMouseOverTabIdxChanged;
			if (m_listTabs->DoMouseOverProcessing(point, isMouseOverTabIdxChanged))
			{
				if (isMouseOverTabIdxChanged)
				{
					_HideTooltip();
				}
				_RedrawDirectly();
			}
		}
		return CXeD2DWndBase::_OnMouseMove(nFlags, point);
	}

	void _DoMouseOverDragTab(const CPoint& point)
	{
		LPWSTR cursor = IDC_UPARROW;
		CRect rcClient;
		GetClientRect(rcClient);
		if (rcClient.PtInRect(point))
		{
			CVwInfo* pTabAtInfo = m_listTabs->GetTabAtPoint(point);
			CXeFileVwIF* pViewAt = pTabAtInfo ? pTabAtInfo->m_pView : nullptr;
			CXeFileVwIF* pDraggedTabVw = m_listTabs->GetTabViewPtr(m_draggedTab);
			if (m_listTabs->CanMoveTabAt(pDraggedTabVw, pViewAt))
			{
				_MoveTab(pDraggedTabVw, pViewAt);
				m_isDragTabOrderChanged = true;
				Sleep(300);
			}
		}
		else
		{
			cursor = _IsPointInOtherTabView(point) ? IDC_PIN : IDC_NO;
		}
		SetCursor(::LoadCursor(NULL, cursor));
	}

	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override
	{
		bool isMouseOverTabIdxChanged;
		if (m_listTabs->DoMouseOverProcessing(CPoint(-1000000, -1000000), isMouseOverTabIdxChanged))
		{
			_RedrawDirectly();
		}
		return CXeD2DWndBase::_OnMouseLeave(wparam, lparam);
	}

	virtual LRESULT _OnMouseHover(WPARAM wparam, LPARAM lparam) override
	{
		return CXeD2DWndBase::_OnMouseHover(wparam, lparam);
	}

	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint pt) override
	{
		if (!m_listTabs->m_isOneRowUI && !m_isDraggingTab && !(m_isLeftXscrollBtnDn || m_isRightXscrollBtnDn))
		{
			int xScroll = zDelta < 0 ? 20 : -20;
			m_listTabs->Hscroll(xScroll);
			bool isMouseOverTabIdxChanged;
			if (m_listTabs->DoMouseOverProcessing(pt, isMouseOverTabIdxChanged))
			{
				if (isMouseOverTabIdxChanged)
				{
					_HideTooltip();
				}
			}
			_RedrawDirectly();
			::SetTimer(Hwnd(), XSCROLL_TIMERID, XSCROLL_TIME, 0);
		}
		return 0;
	}

	void _OnXscrollTimer()
	{
		::KillTimer(Hwnd(), XSCROLL_TIMERID);
		m_listTabs->MakeCurrentTabVisible(_GetCurrentView());
		_RedrawDirectly();
	}

	void _DoXscroll(bool isLeft)
	{
		int xScroll = isLeft ? -20 : 20;
		m_listTabs->Hscroll(xScroll);
		_RedrawDirectly();
		::SetTimer(Hwnd(), XSCROLL_TIMERID, XSCROLL_TIME, 0);
	}
#pragma endregion Mouse_Processing

#pragma region Context_Menu
public:
	virtual void UpdateMenuItem(ListBoxExItem& menu_item, CXeFileVwIF* pView) const override
	{
		// IMPORTANT! pView can be null ! ! ! ! ! ! ! ! ! ! !

		dsid_t dataSourceId = pView ? dataSourceId = pView->GetDataSourceId() : dsid_t();
		UINT uCmdID = (UINT)menu_item.m_extra_data;
		switch (uCmdID)
		{
		case ID_CLOSE_CURRENT_FILE:
			menu_item.EnableItem(pView != nullptr);
			break;
		case ID__MOVETOFIRSTTAB:
			menu_item.EnableItem(m_listTabs->CanMoveTab(EMOVETAB::eFIRST, pView));
			break;
		case ID__MOVETOLASTTAB:
			menu_item.EnableItem(m_listTabs->CanMoveTab(EMOVETAB::eLAST, pView));
			break;
		case ID__MOVELEFT:
			menu_item.EnableItem(m_listTabs->CanMoveTab(EMOVETAB::eLEFT, pView));
			break;
		case ID__MOVERIGHT:
			menu_item.EnableItem(m_listTabs->CanMoveTab(EMOVETAB::eRIGHT, pView));
			break;
		case ID__CLOSEOTHERTABS:
			menu_item.EnableItem(m_listTabs->CanCloseTabs(ECLOSETABS::eCLOSEOTHERS, pView));
			break;
		case ID__CLOSEALLTOTHELEFT:
			menu_item.EnableItem(m_listTabs->CanCloseTabs(ECLOSETABS::eCLOSETOLEFT, pView));
			break;
		case ID__CLOSEALLTOTHERIGHT:
			menu_item.EnableItem(m_listTabs->CanCloseTabs(ECLOSETABS::eCLOSETORIGHT, pView));
			break;
		case ID__CLOSEALLBUTPINNED:
			menu_item.EnableItem(m_listTabs->GetPinnedTabCount() > 0);
			break;
		case ID__MOVETOOTHER:
			menu_item.EnableItem(_CanMoveToOtherView());
			break;
		case ID__OPENCONTAININGFOLDER:
			menu_item.EnableItem(m_pVwMgr->CanOpenContainingFolder(dataSourceId));
			break;
		case ID__FILENAMETOCLIPBOARD:
			menu_item.EnableItem(pView && pView->CanCopyInfoToClipboard(ECLIPBRDOP::eFILENAME));
			break;
		case ID__FULLFILEPATHTOCLIPBOARD:
			menu_item.EnableItem(pView && pView->CanCopyInfoToClipboard(ECLIPBRDOP::eFULLPATH));
			break;
		//case ID__SAVEACOPYAS:	// Save a copy only supported for log files.
		//	menu_item.EnableItem(dataSourceType == DSType::LOGFILE);
		//	break;
		//case ID__EXPORT_BOOKMARKS:	// Export bookmarks only supported for log files and event log files.
		//	menu_item.EnableItem(false);
		//	if (pView)
		//	{
		//		bool hasBookmarks = pView->GetConstFileContainerUI_IF()->GetNumberOfVisibleBookmarks() > 0;
		//		menu_item.EnableItem(hasBookmarks);
		//	}
		//	break;
		//case ID__SETFILECOLOR:	// Set file color only supported for log files.
		//	menu_item.EnableItem(dataSourceType == DSType::LOGFILE);
		//	break;
		//case ID__OPEN_IN_NEW_INSTANCE:
		//	menu_item.EnableItem(pView && pView->CanOpenInAnotherInstance() && GetTotalTabCount() > 1);
		//	break;
		//case ID__MOVE_TO_NEW_INSTANCE:
		//	menu_item.EnableItem(pView && pView->CanOpenInAnotherInstance() && GetTotalTabCount() > 1);
		//	break;
		//case ID__IGNORE_TIME_JUMPS: {
		//	if (pView)
		//	{
		//		const FileMetadata& md = pView->GetConstFileContainerUI_IF()->GetMetadata();
		//		menu_item.EnableItem(md.m_hasTimeJumps);
		//		if (md.m_isIgnoreTimeJumpsInUI)
		//		{
		//			menu_item.m_item_data.m_isChecked = 1;
		//		}
		//	}
		//	else
		//	{
		//		menu_item.EnableItem(false);
		//	}
		//	} break;
		}
	}

protected:
	void _OnTabContextMenu(CPoint pt, CXeFileVwIF* pView)
	{
		CXeMenu menu(m_xeUI, IDR_TAB_CONTEXT_MENU);
		CXeSubMenu* pSubMenu = menu.GetTopLevelMenuItem(0);
		for (ListBoxExItem& menu_item : pSubMenu->m_items)
		{
			UpdateMenuItem(menu_item, pView);
		}

		// Add submenus for "Open in another instance" and "Move to another instance".
		//UINT uFirstOMid = 0, uLastOMid = 0;
		//std::vector<OtherAppInfo> otherLVSs = GetAllOtherInstancesInfoes(m_xeUI->GetMainWindowHandle());
		//bool canOpenMove = pView->CanOpenInAnotherInstance() && GetTotalTabCount() > 1;
		//if (canOpenMove && otherLVSs.size() > 0)
		//{
		//	std::vector<ListBoxExItem> listOpen, listMove;
		//	UINT idx = 0;
		//	for (OtherAppInfo& info : otherLVSs)
		//	{
		//		info.menuIDopen = ID_STATE_CMD_LAST + idx;
		//		info.menuIDmove = ID_STATE_CMD_LAST + idx + 1;
		//		if (uFirstOMid == 0) { uFirstOMid = info.menuIDopen; }
		//		uLastOMid = info.menuIDmove;
		//		std::wstring str = L"Title: \"" + info.strWindowTitle + L"\"";
		//		listOpen.push_back(ListBoxExItem(info.menuIDopen, str));
		//		listMove.push_back(ListBoxExItem(info.menuIDmove, str));
		//		idx += 2;
		//	}
		//	ListBoxExItem popup_open(0, L"Open in another instance", IsSeparator::no, IsChecked::no,
		//		IsEnabled::yes, IsSubMenu::yes);
		//	ListBoxExItem popup_move(0, L"Move to another instance", IsSeparator::no, IsChecked::no,
		//		IsEnabled::yes, IsSubMenu::yes);
		//	menu.AppendPopupMenu(0, popup_open, listOpen);
		//	menu.AppendPopupMenu(0, popup_move, listMove);
		//}

		ClientToScreen(&pt);
		UINT selItem = menu.ShowMenu(Hwnd(), pt, 0);
		if (selItem == 0)
		{
			return;
		}

		//if (selItem >= uFirstOMid && selItem <= uLastOMid)
		//{
		//	_OnOpenMoveInAnotherInstance(selItem, pView, otherLVSs);
		//	return;
		//}

		const CVwInfo* pTabInfo = m_listTabs->GetTab(pView);
		//const FileMetadata& md = pView->GetConstFileContainerUI_IF()->GetMetadata();
		dsid_t datasourceId = pView->GetDataSourceId();
		switch (selItem)
		{
		case ID__OPENCONTAININGFOLDER:
			m_pVwMgr->OpenContainingFolder(datasourceId);
			break;
		case ID__MOVETOOTHER:
			_MoveToOtherView(datasourceId, dsid_t());
			break;
		case ID__MOVETOFIRSTTAB:
			_MoveTab(EMOVETAB::eFIRST, pView);
			break;
		case ID__MOVETOLASTTAB:
			_MoveTab(EMOVETAB::eLAST, pView);
			break;
		case ID__MOVELEFT:
			_MoveTab(EMOVETAB::eLEFT, pView);
			break;
		case ID__MOVERIGHT:
			_MoveTab(EMOVETAB::eRIGHT, pView);
			break;
		case ID__CLOSETHISTAB:
			_DeleteTabAndView(pView);
			break;
		case ID__CLOSEOTHERTABS:
			CloseAndDeleteTabs(m_listTabs->GetTabsToClose(ECLOSETABS::eCLOSEOTHERS, pView));
			break;
		case ID__CLOSEALLTOTHELEFT:
			CloseAndDeleteTabs(m_listTabs->GetTabsToClose(ECLOSETABS::eCLOSETOLEFT, pView));
			break;
		case ID__CLOSEALLTOTHERIGHT:
			CloseAndDeleteTabs(m_listTabs->GetTabsToClose(ECLOSETABS::eCLOSETORIGHT, pView));
			break;
		case ID__CLOSEALLBUTPINNED:
			CloseAndDeleteTabs(m_listTabs->GetTabsToClose(ECLOSETABS::eCLOSEALLBUTPINNED, pView));
			break;
		case ID__PINTAB:
			if (pTabInfo) { _PinTab(pTabInfo); }
			break;
		case ID__FILENAMETOCLIPBOARD:
			pView->OnCopyInfoToClipboard(ECLIPBRDOP::eFILENAME);
			break;
		case ID__FULLFILEPATHTOCLIPBOARD:
			pView->OnCopyInfoToClipboard(ECLIPBRDOP::eFULLPATH);
			break;

		//case ID__OPEN_IN_NEW_INSTANCE:
		//	_OnOpenMoveInNewInstance(false, pView);
		//	break;
		//case ID__MOVE_TO_NEW_INSTANCE:
		//	_OnOpenMoveInNewInstance(true, pView);
		//	break;
		default:
			m_pVwMgr->OnTabCtxMenuCmd(selItem, m_eTabVwId, datasourceId);
			break;
		}
	}

	//void _OnOpenMoveInNewInstance(bool isMove, CXeFileVwIF* pView)
	//{
	//	std::wstring strParams = L"\"/OPEN_NEW_INSTANCE\" \""
	//		+ pView->GetConstFileContainerUI_IF()->GetMetadata().m_strPathName + L"\"";
	//	if (RunAnotherInstance(strParams) && isMove)
	//	{
	//		_DeleteTabAndView(pView);
	//	}
	//}

	//void _OnOpenMoveInAnotherInstance(UINT selMenuItemId, CXeFileVwIF* pView,
	//	std::vector<OtherAppInfo>& otherLVSs)
	//{
	//	std::wstring strPathname = pView->GetConstFileContainerUI_IF()->GetMetadata().m_strPathName;
	//	for (OtherAppInfo& info : otherLVSs)
	//	{
	//		if (selMenuItemId == info.menuIDopen)
	//		{
	//			// Open in another instance
	//			SendCmdLineToAnotherInstance(info.hWnd, strPathname);
	//			break;
	//		}
	//		else if (selMenuItemId == (info.menuIDmove))
	//		{
	//			// Move to another instance
	//			if (SendCmdLineToAnotherInstance(info.hWnd, strPathname))
	//			{
	//				_DeleteTabAndView(pView);
	//			}
	//			break;
	//		}
	//	}
	//}
#pragma endregion Context_Menu

#pragma region Tooltips
public:
	virtual LRESULT _OnNotify_NeedTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT) override
	{
		XeASSERT(pNeedTT);
		if (pNeedTT)
		{
			CPoint pt(*pNeedTT->pt);

			if (m_listTabs->IsPointInTabListUIbuttons(pt))
			{
				return 0;	// Tab list UI buttons have no tooltips
			}

			CVwInfo* pTabInfo = m_listTabs->GetTabAtPoint(pt);
			if (pTabInfo)
			{
				std::wstring sTT = pTabInfo->m_pView ? pTabInfo->m_pView->GetTooltipForTab() : L"";
				if (sTT.size())
				{
					pNeedTT->ti->sTooltip = sTT;
					pNeedTT->ti->hTWnd = GetSafeHwnd();
					pNeedTT->ti->ptTipOffset.x = pt.x;
					return 1;
				}
			}
		}
		return 0;
	}
#pragma endregion Tooltips

#pragma region Tablist_DropDown_UI
protected:
	void _OnLBnotify(WORD nfCode, int item_idx) {}

	void _OnTabListButton()
	{
		m_listTabs->m_isMouseOverLeftTabListDropDownBtn = /*m_listTabs->m_isMouseOverRightTabListDropDownBtn =*/ false;

		CXeFileVwIF* pCurView = GetCurrentView();
		dsid_t dwCurViewDSId = pCurView != nullptr ? pCurView->GetDataSourceId() : dsid_t();

		std::vector<CXeFileVwIF*> allViews = GetAllViews();

		CXeListBoxExCommon listBox(m_xeUI);
		listBox.Initialize(LBType::DropDown,
				[this](WORD nfCode, int item_idx) { _OnLBnotify(nfCode, item_idx); });
		std::vector<ListBoxExItem> list;
		int idx = 0, cur_idx = -1;
		int cxMaxName = 0;
		for (CXeFileVwIF* pView : allViews)
		{
			ListBoxExItem item = ListBoxExItem::MakeCheckBox(pView->GetViewName(), false);
			item.m_extra_data = pView->GetDataSourceId().get();
			PID pid = pView->GetViewPID();
			if (pid != PID::None)
			{
				item.m_item_data.SetIcon(pid);
			}
			if (m_bUseDataSourceTextFgColorInTab)
			{
				item.m_item_data.SetColor(pView->GetViewTitleTextColor());
			}
			list.push_back(item);
			CSize sizeTxt = m_xeUI->GetTextSizeW(EXE_FONT::eUI_Font, pView->GetViewName().c_str());
			cxMaxName = std::max((int)sizeTxt.cx, cxMaxName);
			if (pView->GetDataSourceId() == dwCurViewDSId)
			{
				cur_idx = idx;
			}
			++idx;
		}
		listBox.SetItemsList(list);
		listBox.SetCheckBoxesMode(true, false, true, L"Close selected");
		listBox.OnSetCurSelMsg(cur_idx, 0);

		CRect rcParentCtrl = m_listTabs->m_rcLeftTabListDropDownBtn;
		ClientToScreen(rcParentCtrl);
		CRect rc;
		GetWindowRect(rc);
		int x = rc.left;
		CSize sizeXtra = m_xeUI->GetTextSizeW(EXE_FONT::eUI_Font, L"XXXXXX");
		rc = listBox.CalcPopupWindowSize(x, rc.bottom, cxMaxName + sizeXtra.cx, 1, 20);
		CXePopupCtrl popup(m_xeUI, &listBox, XeShowPopup::FadeIn80);
		popup.ShowPopup(Hwnd(), rc, rcParentCtrl);
		if (listBox.m_isSelEndOk)
		{
			if (listBox.IsOkOrCustomButtonClicked())	// Close selected?
			{
				std::vector<CVwInfo> viewsToClose;
				const std::vector<ListBoxExItem>& items_list = listBox.GetItemsList();
				for (const ListBoxExItem& item : items_list)
				{
					if (item.m_item_data.m_isChecked)
					{
						const CVwInfo* pVwInfo = m_listTabs->GetTabFromDataSourceId(
								dsid_t::MakeDSID((uint32_t)item.m_extra_data));
						if (pVwInfo)
						{
							viewsToClose.push_back(*pVwInfo);
						}
					}
				}
				if (viewsToClose.size())	// Any to close?
				{
					CloseAndDeleteTabs(viewsToClose);
				}
				return;
			}
			int sel_idx = (int)listBox.OnGetCurSelMsg(0, 0);
			const ListBoxExItem* pSelItem = listBox.GetItemDataAtIndexConst(sel_idx);
			if (pSelItem && pSelItem->m_extra_data != dwCurViewDSId.get())
			{
				dsid_t sel_view_dsid = dsid_t::MakeDSID((uint32_t)pSelItem->m_extra_data);
				if (!SwitchToView(sel_view_dsid, true) && m_hOldFocusWnd)
				{
					::SetFocus(m_hOldFocusWnd);
					m_hOldFocusWnd = 0;
				}
			}
		}
	}
#pragma endregion Tablist_DropDown_UI
};

