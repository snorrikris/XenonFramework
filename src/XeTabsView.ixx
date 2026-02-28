module;

#include "os_minimal.h"
#include <memory>
#include <string>
#include "XeResource.h"
//#include "CustomWndMsgs.h"

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
//import Xe.SendMessageToAllOtherAppInstances;
//import Xe.FileContainerIF;
//import Xe.ViewManagerIF_LVS;

constexpr int c_cxTAB_X_MARGIN = 2;	// X margin between tabs and buttons
constexpr int c_cxTEXT_X_MARGIN = 4;	// Text margin from icon left edge
constexpr int c_cyTEXT_Y_MARGIN = 9;	// Text Y margin from view bottom edge
constexpr int c_cxTEXT_R_MARGIN = 6;	// Text margin from text end to tab right edge
constexpr int c_cxCLOSE_R_MARGIN = 4;	// Close button right margin to tab right edge

export class CXeTabsView : public CXeD2DWndBase
{
protected:
	CXeViewManagerIF* m_pVwMgr = nullptr;

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

#pragma region Creation
public:
	CXeTabsView(CXeViewManagerIF* pVwMgr) : CXeD2DWndBase(pVwMgr->GetUIcolors()), m_pVwMgr(pVwMgr)
	{
		XeASSERT(pVwMgr);
		m_bUseDataSourceTextFgColorInTab = s_xeUIsettings[L"FilesListsSettings"].Get(L"UseColorForLogFileInTabTitle").getBool();
		m_listTabs = std::make_unique<CXeTabsList>();
	}
	virtual ~CXeTabsView() {}

	void Create(HWND hParentWnd, ETABVIEWID tabVwId, UINT uTabVwDlgCtrlId, UINT uViewDlgCtrlId,
			CXeTabsView* pOtherTabView)
	{
		XeASSERT(hParentWnd
			&& (tabVwId == ETABVIEWID::ePrimaryTabVw || tabVwId == ETABVIEWID::eSecondaryTabVw));
		m_eTabVwId = tabVwId;
		m_uTabVwDlgCtrlId = uTabVwDlgCtrlId;
		m_uViewDlgCtrlId = uViewDlgCtrlId;
		m_pOtherView = pOtherTabView;

		std::wstring classname = L"XeTabsView_WNDCLASS";
		m_xeUI->RegisterWindowClass(classname, D2DCtrl_WndProc);
		HWND hWnd = CreateD2DWindow(0, classname.c_str(), nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
				CRect(), hParentWnd, 0, true);
		m_fInitialUpdateDone = TRUE;
	}
#pragma endregion Creation

#pragma region Misc
public:
	bool IsVisible() { return ::GetDlgCtrlID(Hwnd()) == m_uTabVwDlgCtrlId; }

	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{
		if (m_fInitialUpdateDone)	// Initialized?
		{
			if (m_listTabs->GetTabCount())
			{
				_EnsureCurrentTabVisible();
			}
			_RedrawDirectly();
		}
		return CXeD2DWndBase::_OnSize(hWnd, wParam, lParam);
	}

	int RecalculateTabViewHeight(int cxAvailable)
	{
		_CalculateTabsViewUI(cxAvailable, *m_listTabs);
		return GetTabViewHeight();
	}

	int GetTabViewHeight() const { return _GetTabViewRowHeight() * GetNumTabRows(); }

	int GetNumTabRows() const { return m_listTabs->GetNumTabRows(); }

	ETABVIEWID GetTabVwId() { return m_eTabVwId; }

	// Return true if point (in screen coords.) is in a tab view or file view else false.
	bool IsPointInThisView(POINT& pt)
	{
		CXeFileVwIF* pView = _GetCurrentView();
		bool isPtInFileVw = pView ? pView->IsPointInThisView(pt) : false;
		CRect rcTab = _GetClientRect();
		ClientToScreen(rcTab);
		bool isPtInTabVw = rcTab.PtInRect(pt) != FALSE;
		return isPtInTabVw || isPtInFileVw;
	}

	// Called from LVSState when view with focus has changed.
	void ViewWithFocusChanged(dsid_t dwDataSourceId)
	{
		if (m_listTabs->UpdateIdOfViewWithFocus(dwDataSourceId))
		{
			_RedrawDirectly();
		}
	}

	bool IsChildOfCurrentView(HWND hWnd)
	{
		CXeFileVwIF* pCurView = _GetCurrentView();
		HWND hWndVw = pCurView ? pCurView->GetHwndOfView() : nullptr;
		return hWndVw ? ::IsChild(hWndVw, hWnd) : false;
	}

	void OnChangedSettings(const ChangedSettings& chg_settings)
	{
		if (chg_settings.IsChanged(L"FilesListsSettings", L"UseColorForLogFileInTabTitle")
			|| m_xeUI->IsFontSettingsChanged(chg_settings))
		{
			m_bUseDataSourceTextFgColorInTab = s_xeUIsettings[L"FilesListsSettings"].Get(L"UseColorForLogFileInTabTitle").getBool();
			_CalculateTabsViewUI(_GetClientRect().right, *m_listTabs);
			_RedrawDirectly();
		}
		std::vector<CXeFileVwIF*> allViews = GetAllViews();
		for (CXeFileVwIF* pView : allViews)
		{
			pView->OnChangedSettings(chg_settings);
		}
		if (chg_settings.IsChanged(L"Colors", L"Color"))
		{
			_RedrawDirectly();
		}
	}

	bool IsTabPinned(dsid_t dwDataSourceId)
	{
		const CVwInfo* pVwNfo = m_listTabs->GetTab(dwDataSourceId);
		if (pVwNfo)
		{
			return pVwNfo->m_isPinned;
		}
		return false;
	}

protected:
	void _UpdateLayout(bool isRecalculateUI, bool notifyParent)
	{
		XeASSERT(m_uTabVwDlgCtrlId);

		if (isRecalculateUI)
		{
			_CalculateTabsViewUI(_GetClientRect().right, *m_listTabs);
		}

		bool hasViews = m_listTabs->GetTabCount() > 0;
		bool isAlreadyVisible = IsVisible();
		if (hasViews != isAlreadyVisible)
		{
			int dlgCtrlId = hasViews ? m_uTabVwDlgCtrlId : 0;
			int cmd = hasViews ? SW_SHOW : SW_HIDE;
			if (cmd == SW_HIDE && m_eTabVwId == ETABVIEWID::ePrimaryTabVw && m_pOtherView->IsVisible())
			{
				// 'this' is primary tab view that is now empty AND secondary tab view is visible (has tabs)
				CVwInfoList tabList = m_pOtherView->RemoveAllTabs(false);
				AttachTabs(tabList);
			}
			else
			{
				::SetWindowLong(Hwnd(), GWL_ID, dlgCtrlId);
				::ShowWindow(Hwnd(), cmd);
			}
		}

		if (notifyParent)
		{
			::SendMessage(GetParent(Hwnd()), WM_SIZE, 0, 0);	// Tell splitter window to reposition windows.

			_RedrawDirectly();
		}
	}
#pragma endregion Misc


#pragma region View_Management
public:
	// Add view to tab list, viewParams.makeThisCurrentView = true to make it the current view,
	// setFocusToView = true to set input focus to view (only when makeCurrentTab = true),
	// viewParams.insertBeforeDSid = -1 to add at the end, 0 to add at the begining, 
	// or datasource Id of tab to insert before.
	bool AttachView(CXeFileVwIF* pView, CreateViewParams viewParams, bool setFocusToView)
	{
		XeASSERT(pView);
		HWND hWndVw = pView->GetHwndOfView();
		::SetWindowLong(hWndVw, GWL_ID, 0);
		::ShowWindow(hWndVw, SW_HIDE);

		::SetParent(hWndVw, ::GetParent(Hwnd()));

		if (m_listTabs->GetTabCount() == 0)
		{
			viewParams.makeThisCurrentView = true;
		}

		CVwInfo vwNfo(pView);
		vwNfo.m_isPinned = viewParams.isTabPinned;
		m_listTabs->AddTab(vwNfo, viewParams.insertBeforeDSid);

		//TRACE("AttachView : %s - DSID: %u eTabVwId: %d, insertBeforeDSid: %d, makeFirstOpenedCurrentView: %d, gotoLastRowInGrid: %d\r\n",
		//	pView->GetContentFullPathName(), pView->GetDataSourceId(), viewParams.eTabVwId, viewParams.insertBeforeDSid, viewParams.makeFirstOpenedCurrentView, viewParams.gotoLastRowInGrid);
		if (viewParams.makeThisCurrentView)
		{
			SwitchToView(pView, setFocusToView);
		}
		else
		{
			_EnsureCurrentTabVisible();
		}

		_UpdateLayout(false, true);

		return true;
	}

	// Attach tabs from list. tabList MUST contain at least 1 item.
	void AttachTabs(CVwInfoList& tabList)
	{
		XeASSERT(tabList.size() > 0);
		int nID = m_uViewDlgCtrlId;

		CXeFileVwIF* pCurView = _GetCurrentView();
		if (pCurView)
		{
			HWND hWndVw = pCurView->GetHwndOfView();
			::SetWindowLong(hWndVw, GWL_ID, 0);	// Set Control ID.
			::ShowWindow(hWndVw, SW_HIDE);
		}

		for (CVwInfo& tabinfo : tabList)
		{
			XeASSERT(tabinfo.m_pView);
			HWND hWndVw = tabinfo.m_pView->GetHwndOfView();
			::SetParent(hWndVw, ::GetParent(Hwnd()));
			if (nID)
			{
				::SetWindowLong(hWndVw, GWL_ID, nID);	// Set Control ID.
				::ShowWindow(hWndVw, SW_SHOW);
				::SetFocus(hWndVw);
				_RedrawDirectly();
				nID = 0;
			}

			m_listTabs->AddTab(tabinfo, dsid_t());	// Add tab info struct to list.
		}

		_UpdateLayout(true, true);
	}

	// Remove all tabs and set 'this' as not-visible, notify parent to update layout.
	CVwInfoList RemoveAllTabs(bool notifyParent)
	{
		CXeFileVwIF* pCurView = _GetCurrentView();
		CVwInfoList tabList = m_listTabs->RemoveAllTabs();
		for (CVwInfo& tabInfo : tabList)
		{
			if (tabInfo.m_pView == pCurView)
			{
				HWND hWndVw = tabInfo.m_pView->GetHwndOfView();
				::SetWindowLong(hWndVw, GWL_ID, 0);	// Set Control ID.
				::ShowWindow(hWndVw, SW_HIDE);
			}
			tabInfo.ClearUIvars();
		}

		_UpdateLayout(true, notifyParent);

		return tabList;
	}

	// Note - for orderly exit: all merged views should be deleted before others.
	//void DeleteAllMergedTabsAndViews()
	//{
	//	_CloseAndDeleteTabs(m_listTabs->GetTabsOfType(DSType::GetMergedLogsTypes()));
	//}

	void CloseAndDeleteTabs(const std::vector<CVwInfo>& tabsList)
	{
		for (const CVwInfo& tabInfo : tabsList)
		{
			_DeleteTabAndView(tabInfo.m_pView);
		}
	}

	void DeleteAllTabsAndViews()
	{
		// Notify LVSState that views are closing.
		std::vector<CVwInfo> tabInfos = m_listTabs->GetAllTabs();
		for (CVwInfo& tabInfo : tabInfos)
		{
			XeASSERT(tabInfo.m_pView && tabInfo.m_pView->GetHwndOfView());
			m_pVwMgr->OnViewClosing(tabInfo.m_dsid);
		}

		CVwInfoList tabList = m_listTabs->RemoveAllTabs();
		XeASSERT(tabList.size() == tabInfos.size());

		// Hide all views.
		for (CVwInfo& tabInfo : tabList)
		{
			XeASSERT(tabInfo.m_pView && tabInfo.m_pView->GetHwndOfView());
			HWND hWndVw = tabInfo.m_pView->GetHwndOfView();
			::SetWindowLong(hWndVw, GWL_ID, 0);	// Set Control ID.
			::ShowWindow(hWndVw, SW_HIDE);
		}

		// Destroy all views.
		for (CVwInfo& tabInfo : tabList)
		{
			::DestroyWindow(tabInfo.m_pView->GetHwndOfView());
		}

		// Notify LVSState that views are closed and delete the view object.
		for (CVwInfo& tabInfo : tabList)
		{
			// Note - LVSState will notify CXeFilesManager - that will close and delete the (log file) container object.
			m_pVwMgr->OnViewClosed(tabInfo.m_dsid);
		}

		_UpdateLayout(true, true);
	}

	bool FindAndDeleteTabAndView(dsid_t dwDataSourceId)
	{
		const CVwInfo* pTabInfo = m_listTabs->GetTabFromDataSourceId(dwDataSourceId);
		if (pTabInfo)
		{
			_DeleteTabAndView(pTabInfo->m_pView);
			return true;
		}
		return false;
	}

	//void CloseAllViewsOfList(UINT uListId)
	//{
	//	_CloseAndDeleteTabs(m_listTabs->GetTabsWithListId(uListId));
	//}

	bool SwitchToView(CXeFileVwIF* pView, bool setFocusToView)
	{
		XeASSERT(pView);
		//TRACE("SwitchToView : %s, setFocusToView: %d\r\n", pView->GetContentFullPathName(), setFocusToView);
		CXeFileVwIF* pCurrentView = _GetCurrentView();
		if (pView == pCurrentView)
		{
			if (setFocusToView)
			{
				pView->SetFocusToView();
			}
			return true;
		}

		if (!m_listTabs->GetTab(pView))	// Is view found in the tab list?
		{
			return false;
		}

		// Switch views

		// Hide current view
		if (pCurrentView)
		{
			HWND hWndVw = pCurrentView->GetHwndOfView();
			::SetWindowLong(hWndVw, GWL_ID, 0);	// Set Control ID.
			::ShowWindow(hWndVw, SW_HIDE);
		}

		// Show selected view
		HWND hWndVw = pView->GetHwndOfView();
		::SetWindowLong(hWndVw, GWL_ID, m_uViewDlgCtrlId);	// Set Control ID.
		::ShowWindow(hWndVw, SW_SHOW);

		if (setFocusToView)
		{
			pView->SetFocusToView();
		}

		_EnsureCurrentTabVisible();

		::SendMessage(::GetParent(Hwnd()), WM_SIZE, 0, 0);

		_RedrawDirectly();

		return TRUE;
	}

	bool SwitchToView(dsid_t dwDataSourceId, bool setFocusToView)
	{
		//TRACE("SwitchToView : dwDataSourceId: %d, setFocusToView: %d\r\n", dwDataSourceId, setFocusToView);
		const CVwInfo* ptabInfo = m_listTabs->GetTabFromDataSourceId(dwDataSourceId);
		return ptabInfo ? SwitchToView(ptabInfo->m_pView, setFocusToView) : false;
	}

	UINT GetIndexOfCurrentTab() const
	{
		return m_listTabs->GetIndexOfCurrentTab();
	}

	bool SwitchToViewAtIndex(UINT uIndex)
	{
		//TRACE("SwitchToView : uIndex: %d\r\n", uIndex);
		const CVwInfo* pTabInfo = m_listTabs->GetTabAtIndex(uIndex);
		if (pTabInfo)
		{
			SwitchToView(pTabInfo->m_dsid, true);
			return true;
		}
		return false;
	}

	bool SwitchToView(ViewNavigate navigate)
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

	void RenameTab(CXeFileVwIF* pView, const std::wstring& strNewTitle)
	{
		const CVwInfo* pTabInfo = m_listTabs->GetTab(pView);
		if (pTabInfo)
		{
			_CalculateTabsViewUI(_GetClientRect().right, *m_listTabs);
			_RedrawDirectly();
		}
	}

	int GetTabCount() const { return m_listTabs->GetTabCount(); }

	int GetTotalTabCount() const { return GetTabCount() + (m_pOtherView ? m_pOtherView->GetTabCount() : 0); }

	std::vector<CVwInfo> GetAllTabs() const { return m_listTabs->GetAllTabs(); }

	std::vector<CXeFileVwIF*> GetAllViews()
	{
		std::vector<CXeFileVwIF*> views;
		for (auto item : m_listTabs->GetAllTabs())
		{
			views.push_back(item.m_pView);
		}
		return views;
	}

	const CVwInfo* FindView(CXeFileVwIF* pView)
	{
		return m_listTabs->GetTab(pView);
	}

	CXeFileVwIF* FindView(dsid_t dwDataSourceId)
	{
		const CVwInfo* pTabInfo = m_listTabs->GetTabFromDataSourceId(dwDataSourceId);
		if (pTabInfo)
		{
			return pTabInfo->m_pView;
		}
		return nullptr;
	}

	CXeFileVwIF* GetCurrentView() { return _GetCurrentView(); }

	dsid_t GetDataSourceIdOfCurrentView()
	{
		CXeFileVwIF* pCurView = _GetCurrentView();
		return pCurView ? pCurView->GetDataSourceId() : dsid_t();
	}

	void SendMessageToAllViews(UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		m_listTabs->SendMessageToAllViews(uMessage, wParam, lParam);

		if (uMessage == WMU_NOTIFY_CHANGES)
		{
			switch (wParam)
			{
			case NFCHG_CODE_SEL_THEME:
			case NFCHG_CODE_LOGFILE_COLORS_CHANGED:
				m_bUseDataSourceTextFgColorInTab = s_xeUIsettings[L"FilesListsSettings"].Get(L"UseColorForLogFileInTabTitle").getBool();
				_RedrawDirectly();
				break;
			}
		}
	}

	void SetTabProgressIndication(dsid_t dwDataSourceId, UINT uProgress)
	{
		_RedrawDirectly();
	}

	void ClearProgressOnAllTabs()
	{
		_RedrawDirectly();
	}

	// Find insert position in tab list for log file.
	// Return DataSourceId of tab that should have strPathName tab before it.
	// Return 0 if insert position not found.
	//dsid_t FindTabInsertForLogFile(const FileMetadata& md_of_file)
	//{
	//	bool isMerge = md_of_file.m_dataSourceType.IsMergedLogsType();
	//	std::vector<CVwInfo> tabList = m_listTabs->GetTabsOfType(DSType::GetLogTypesExceptMerged());
	//	if (isMerge && tabList.size())	// Merge view? - put at front
	//	{
	//		return tabList[0].m_dsid;
	//	}
	//	int nOurKLFindex = md_of_file.m_file_list_index;
	//	if (nOurKLFindex < 0)
	//	{
	//		return dsid_t::MakeDSID(0xFFFFFFFF);
	//	}
	//	for (CVwInfo& tabInfo : tabList)
	//	{
	//		XeASSERT(tabInfo.m_pView);
	//		if (tabInfo.m_pView)
	//		{
	//			const FileMetadata& md = tabInfo.m_pView->GetConstFileContainerUI_IF()->GetMetadata();
	//			int nTabFileKLFindex = md.m_file_list_index;
	//			if (nTabFileKLFindex >= 0 && nOurKLFindex < nTabFileKLFindex)
	//			{
	//				return md.m_dwDataSourceId;
	//			}
	//		}
	//	}
	//	return dsid_t::MakeDSID(0xFFFFFFFF);
	//}

protected:
	// Move view to other tab view.
	// pSelView = view to move.
	// insertBeforeDataSourceId = data source ID of view in other tab view that pSelView will be inserted before,
	// = 0 : is inserted at front, -1 : is inserted at back of tab list.
	void _MoveToOtherView(dsid_t selViewId, dsid_t insertBeforeDataSourceId)
	{
		CXeFileVwIF* pSelView = m_listTabs->GetTabViewPtr(selViewId);
		XeASSERT(m_pOtherView && pSelView);

		if (pSelView == _GetCurrentView())
		{
			HWND hWndVw = pSelView->GetHwndOfView();
			::SetWindowLong(hWndVw, GWL_ID, 0);	// Set Control ID.
			::ShowWindow(hWndVw, SW_HIDE);
		}

		// Remove tab from tab control.
		CXeFileVwIF* pNextView = m_listTabs->RemoveTab(pSelView);

		/////////////////////////////////////////////////////////////////////////////////////
		// Move to other view

		// Add tab info struct to list.
		m_pOtherView->m_listTabs->AddTab(CVwInfo(pSelView), insertBeforeDataSourceId);

		m_pOtherView->_UpdateLayout(true, true);

		m_pOtherView->SwitchToView(pSelView, false);

		/////////////////////////////////////////////////////////////////////////////////////

		if (pNextView)
		{	// Switch to next view
			HWND hWndVw = pSelView->GetHwndOfView();
			::SetWindowLong(hWndVw, GWL_ID, m_uViewDlgCtrlId);	// Set Control ID.
			::ShowWindow(hWndVw, SW_SHOW);
			::SetFocus(hWndVw);
		}

		_EnsureCurrentTabVisible();
		_UpdateLayout(false, true);

		// Notify LVSState that tab order has changed.
		m_pVwMgr->OnTabOrderChanged();
	}

	bool _MoveTab(EMOVETAB eMove, CXeFileVwIF* pViewToMove)
	{
		if (!m_listTabs->MoveTab(eMove, pViewToMove))
		{
			return false;
		}

		_PostMoveTab();
		return true;
	}

	bool _MoveTab(CXeFileVwIF* pViewToMove, CXeFileVwIF* pViewAt)
	{
		if (!m_listTabs->MoveTab(pViewToMove, pViewAt))
		{
			return false;
		}

		_PostMoveTab();
		return true;
	}

	void _PostMoveTab()
	{
		_EnsureCurrentTabVisible();

		_RedrawDirectly();
	}

	BOOL _HasCurrentViewFocus(dsid_t* pdwTrackId = nullptr)
	{
		if (pdwTrackId) { pdwTrackId->reset(); }
		CXeFileVwIF* pView = _GetCurrentView();
		if (!pView)
			return FALSE;

		HWND hFocusWnd = ::GetFocus();
		HWND hWndVw = pView->GetHwndOfView();
		BOOL fHasFocus = ::IsChild(hWndVw, hFocusWnd);
		if (fHasFocus && pdwTrackId)
		{
			*pdwTrackId = pView->GetDataSourceId();
		}
		return fHasFocus;
	}

	void _DeleteTabAndView(CXeFileVwIF* pDelView)
	{
		// Note - the view being deleted is not necessarily the current view (i.e. the user can close a view that is not current).
		XeASSERT(pDelView);
		dsid_t dwDataSourceIdOfClosingView = pDelView->GetDataSourceId();

		m_pVwMgr->OnViewClosing(dwDataSourceIdOfClosingView);

		CXeFileVwIF* pCurrentView = _GetCurrentView();
		bool isDelViewCurrent = pDelView == pCurrentView;

		CXeFileVwIF* pNextView = m_listTabs->RemoveTab(pDelView);	// Remove tab from tab control.
		if (!isDelViewCurrent)
		{
			pNextView = pCurrentView;
		}
		if (pNextView)
		{
			bool setFocusToView = isDelViewCurrent; // IsChild(GetFocus());
			SwitchToView(pNextView, setFocusToView);
		}
		else
		{
			// Hide current view
			HWND hWndVw = pDelView->GetHwndOfView();
			::SetWindowLong(hWndVw, GWL_ID, 0);	// Set Control ID.
			::ShowWindow(hWndVw, SW_HIDE);
		}

		m_listTabs->DecrementFirstVisibleTabWhenNeeded(_GetTabsRect());

		::DestroyWindow(pDelView->GetHwndOfView());

		m_pVwMgr->OnViewClosed(dwDataSourceIdOfClosingView);

		_EnsureCurrentTabVisible();
		_UpdateLayout(true, true);
	}

	CXeFileVwIF* _GetCurrentView() const
	{	// view is in row below tab view.
		HWND hWndCurVw = ::GetDlgItem(::GetParent(Hwnd()), m_uViewDlgCtrlId);
		const CVwInfo* pVwNfo = m_listTabs->GetTab(hWndCurVw);
		//ASSERT(pVwNfo);
		return pVwNfo ? pVwNfo->m_pView : nullptr;
	}

	dsid_t _GetCurrentViewId() const
	{
		CXeFileVwIF* pCurrentView = _GetCurrentView();
		return pCurrentView ? pCurrentView->GetDataSourceId() : dsid_t();
	}

	// Calculate tab's size, try to make current tab totally visible.
	void _EnsureCurrentTabVisible()
	{
		CXeFileVwIF* pCurrentView = _GetCurrentView();
		if (pCurrentView)
		{
			_CalculateTabsViewUI(_GetClientRect().right, *m_listTabs);
			m_listTabs->MakeCurrentTabVisible(pCurrentView, _GetTabsRect());
			_CalculateTabsViewUI(_GetClientRect().right, *m_listTabs);
		}
	}

#pragma endregion View_Management


#pragma region Painting
protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override
	{
		XeASSERT(m_xeUI);

		pRT->FillRectangle(rcClient, GetBrush(CID::TabBg));
		_DrawTabsViewUI(pRT, *m_listTabs, _GetCurrentViewId());
	}

	CRect _GetTabsRect()
	{
		CRect rcTabs = _GetClientRect();
		rcTabs.left = m_listTabs->m_rcLeftTabListDropDownBtn.right;
		rcTabs.right = m_listTabs->m_rcRightTabListDropDownBtn.left;
		return rcTabs;
	}

	static CSize _GetTabViewDropListButtonSize() { return CSize(20, 20); }
	static CSize _GetTabViewCloseAndPinButtonSize() { return CSize(15, 15); }

	int _GetTabViewRowHeight() const
	{
		return std::max((int)20, m_xeUI->GetFontMetric(EXE_FONT::eUI_Font).GetHeight()) + c_cyTEXT_Y_MARGIN;
	}

	void _CalculateTabsViewUI(int cxVw, CXeTabsList& tabsList)
	{
		//TRACE("CalculateTabsViewUI - first visible unpinned tab: %d\n", m_listTabs.m_nFirstVisibleUnpinnedTab);
		int idxFirstVisibleUnpinnedTab = tabsList.m_nFirstVisibleUnpinnedTab;
		int cyTabRow = _GetTabViewRowHeight();

		/* *** Fitting model ***
		* Pinned tabs and unpinned tabs will occupy one row if all pinned tabs + all unpinned tabs
		* fit on the row (along with the drop down buttons).
		* If pinned tabs need more than one row then unpinned tabs will not share a row with the
		* pinned tabs.
		*/

		int x_firstTabAfterLB = _SetDropListButtonRects(cxVw, tabsList, 0 /*row*/, cyTabRow);
		int x_nextTab = x_firstTabAfterLB;

		bool isOneRowEnough = true;
		bool hasAnyPinnedTabs = tabsList.GetPinnedTabCount() > 0;
		bool hasAnyUnpinnedTabs = tabsList.GetUnPinnedTabCount() > 0;
		int idx = 0, x_limit = tabsList.m_rcRightTabListDropDownBtn.left;
		for (CVwInfo& tabInfo : tabsList.m_list)
		{
			_CalculateTabWidth(tabInfo);

			bool canFit = hasAnyPinnedTabs
				? (x_nextTab + tabInfo.m_cxTabNeeded) < x_limit
				: x_nextTab < x_limit;
			if (canFit)
			{
				if (tabInfo.m_isPinned || idx >= idxFirstVisibleUnpinnedTab)
				{
					x_nextTab = _SetTabRects(tabInfo, x_nextTab, 0 /*row*/, cyTabRow);
				}
				else
				{
					tabInfo.ClearUIvars();
				}
			}
			else
			{
				tabInfo.ClearUIvars();
				isOneRowEnough = hasAnyPinnedTabs ? false : true;
			}
			++idx;
		}

		if (!isOneRowEnough)
		{
			x_nextTab = 0;	// First tab x pos.
			idx = 0;
			int row = 0;
			bool isFittingPinnedTabs = hasAnyPinnedTabs, isAnyPinnedTabFitted = false;;
			for (CVwInfo& tabInfo : tabsList.m_list)
			{
				if (isFittingPinnedTabs && !tabInfo.m_isPinned)	// All pinned tabs fitted?
				{
					isFittingPinnedTabs = false;
					++row;										// Place unpinned tabs on next row down.
					x_nextTab = x_firstTabAfterLB;
				}
				if (isFittingPinnedTabs)
				{
					if ((x_nextTab + tabInfo.m_cxTabNeeded) > cxVw
						&& isAnyPinnedTabFitted /* make sure row 0 has a tab */)
					{
						++row;
						x_nextTab = 0;
					}
					x_nextTab = _SetTabRects(tabInfo, x_nextTab, row, cyTabRow);
					isAnyPinnedTabFitted = true;
				}
				else	// Fitting unpinned tabs
				{
					if (idx >= idxFirstVisibleUnpinnedTab && x_nextTab < x_limit)
					{
						x_nextTab = _SetTabRects(tabInfo, x_nextTab, row, cyTabRow);
					}
					else
					{
						tabInfo.ClearUIvars();
					}
				}

				idx++;
			}

			int lastRow = tabsList.GetNumTabRows() - 1;

			if (!hasAnyUnpinnedTabs)	// No unpinned tabs?
			{
				// Need to reposition last row tabs to fit L drop down button.
				for (CVwInfo& tabInfo : tabsList.m_list)
				{
					if (tabInfo.m_nRow == lastRow)
					{
						_SetTabRects(tabInfo, tabInfo.m_rcTab.left + x_firstTabAfterLB, lastRow, cyTabRow);
					}
				}
			}
			_SetDropListButtonRects(cxVw, tabsList, lastRow, cyTabRow);	// Put drop down buttons on last row
		}
	}

	// Set Tab rect, pin button rect, close button rect. Return X pos of next tab.
	static int _SetTabRects(CVwInfo& tabInfo, int x, int row, int cyTabRow)
	{
		CSize closeBtn = _GetTabViewCloseAndPinButtonSize();
		int yBtn = 2 + (int)((double)(cyTabRow - closeBtn.cy) / 2 + 0.5);
		int xTabRightEdge = x + tabInfo.m_cxTabNeeded;
		int y = row * cyTabRow;
		int xBtn = xTabRightEdge - closeBtn.cx - c_cxCLOSE_R_MARGIN - 1;
		tabInfo.m_rcCloseBtn.SetRect(xBtn, y + yBtn, xBtn + closeBtn.cx, y + yBtn + closeBtn.cy);
		xBtn -= closeBtn.cx;
		tabInfo.m_rcPinBtn.SetRect(xBtn, y + yBtn, xBtn + closeBtn.cx, y + yBtn + closeBtn.cy);
		tabInfo.m_rcTab.SetRect(x, y, xTabRightEdge, y + cyTabRow - 1);
		tabInfo.m_nRow = row;
		return tabInfo.m_rcTab.right + c_cxTAB_X_MARGIN;
	};

	// Calculate left and right tab list drop down buttons coords. for row.
	// Return X pos. for first tab after Left drop button.
	static int _SetDropListButtonRects(int cxVw, CXeTabsList& tabsList, int row, int cyTabRow)
	{
		CRect& rcLB = tabsList.m_rcLeftTabListDropDownBtn;
		CRect& rcRB = tabsList.m_rcRightTabListDropDownBtn;
		int xListBtnL = c_cxTAB_X_MARGIN;
		CSize listBtn = _GetTabViewDropListButtonSize();
		int yListBtns = (int)((double)(cyTabRow - listBtn.cy) / 2 + 0.5);
		yListBtns += (cyTabRow * row);
		rcLB.SetRect(xListBtnL, yListBtns, xListBtnL + listBtn.cx, yListBtns + listBtn.cy);
		int xListBtnR = cxVw - listBtn.cx;
		rcRB.SetRect(xListBtnR, yListBtns, xListBtnR + listBtn.cx, yListBtns + listBtn.cy);
		return rcLB.right + c_cxTAB_X_MARGIN;	// First tab x pos. after L drop button.
	}

	void _CalculateTabWidth(CVwInfo& tabInfo)
	{
		XeASSERT(tabInfo.m_pView);
		if (tabInfo.m_pView)
		{
			CSize closeBtn = _GetTabViewCloseAndPinButtonSize();

			tabInfo.m_cxTabNeeded = c_cxTEXT_X_MARGIN
				+ 18	// Add space for icon
				+ m_xeUI->GetTextSizeW(EXE_FONT::eUI_FontBold, tabInfo.m_pView->GetViewName().c_str()).cx
				+ c_cxTEXT_R_MARGIN
				+ (closeBtn.cx * 2)		// Pin button and Close button
				+ c_cxCLOSE_R_MARGIN;	// add space for close button
		}
	}

	void _DrawTabsViewUI(ID2D1RenderTarget* pRT, CXeTabsList& tabsList, dsid_t curVwId)
	{
		CRect rcCli = _GetClientRect();
		CVwInfoList& list = tabsList.m_list;
		tabsList.m_curVwId = curVwId;
		int cxVw = rcCli.Width(), cyVw = rcCli.Height();
		pRT->DrawLine({ 0.0f, (float)(cyVw - 1) }, { (float)cxVw, (float)(cyVw - 1) }, GetBrush(CID::TabBtmBorder));

		// Draw tabs
		for (CVwInfo& tabInfo : list)
		{
			if (!tabInfo.m_rcTab.IsRectEmpty())
			{
				_DrawTab(pRT, tabInfo, tabsList);
			}
		}

		// Draw Left and Right Tab list drop down buttons.
		_DrawChevronButton(pRT, RectFfromRect(tabsList.m_rcLeftTabListDropDownBtn),
				tabsList.m_isMouseOverLeftTabListDropDownBtn, CID::TabBg);
		CRect rcBtnSpcR(tabsList.m_rcRightTabListDropDownBtn);
		rcBtnSpcR.bottom = cyVw;
		rcBtnSpcR.top = rcBtnSpcR.bottom - _GetTabViewRowHeight();
		rcBtnSpcR.left -= 2;
		pRT->FillRectangle(RectFfromRect(rcBtnSpcR), GetBrush(CID::TabBg));
		_DrawChevronButton(pRT, RectFfromRect(tabsList.m_rcRightTabListDropDownBtn),
				tabsList.m_isMouseOverRightTabListDropDownBtn, CID::TabBg);

		CRect rcFocusLine(0, cyVw - 2, cxVw, cyVw);
		pRT->FillRectangle(RectFfromRect(rcFocusLine),
				GetBrush(tabsList.m_isFocusVwInThis ? CID::TabFocusBg : CID::TabCurBg));
	}

	void _DrawTab(ID2D1RenderTarget* pRT, const CVwInfo& tabInfo, const CXeTabsList& tabsList)
	{
		//if (!_Init(tabInfo.m_pView)) { return; }

		CRect rc = tabInfo.m_rcTab;
		rc.left += 2;

		bool isCurrentTab = tabInfo.m_dsid == tabsList.m_curVwId;
		//CXeFileContainerUI_IF* ui_ds = tabInfo.m_pView->GetFileContainerUI_IF();
		//const FileMetadata& md = ui_ds->GetMetadata();
		//const CWorkContext& ctx = ui_ds->GetWorkContextConst();
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
			rcTab.left += 2;
			rcTab.right += 2;
			rcTab.top += 4;
			pRT->FillRectangle(RectFfromRect(rcTab), GetBrush(tabBg));
		}

		EXE_FONT font = isCurrentTab || tabInfo.m_bIsMouseOver ? EXE_FONT::eUI_FontBold : EXE_FONT::eUI_Font;
		bool isUseFileColor = m_bUseDataSourceTextFgColorInTab && !isAlertState;
		COLORREF rgbTxt = tabInfo.m_pView->GetViewTitleTextColor(); // md.m_rgbLogFileColor;
		CRect rcProg;
		//if (ctx.m_progress > 0 && !(ctx.m_progressState == EPROGRESSSTATE::eDone
		//	|| ctx.m_progressState == EPROGRESSSTATE::eDoneParseFailed))
		//{
		//	INT32 progress = std::min(ctx.m_progress, (INT32)100);
		//	rcProg = rc;
		//	rcProg.right = rcProg.left + (int)((double)(rc.Width() * progress) / 100 + 0.5);
		//}
		PID pid = tabInfo.m_pView->GetViewPID(); // PID::None;
		//if (md.m_dataSourceType.IsMergedLogsType())	// Is Merge?
		//{
		//	pid = md.m_isMergeRebuildNeeded ? PID::RebuildMergeNeededPng : PID::MergePng;
		//}
		//else if (md.m_fileIcon != FileIcon::None)
		//{
		//	pid = m_xeUI->GetFileIconPID(md.m_fileIcon);
		//}
		_DrawIconAndFilename(pRT, tabInfo.m_pView->GetViewName(), rc, font, rgbTxt, pid, true, rcProg);

		// Draw close button if 'this' is current tab or mouse over tab.
		if (isCurrentTab || tabInfo.m_bIsMouseOver)
		{
			_DrawCloseButton(pRT, RectFfromRect(tabInfo.m_rcCloseBtn), tabInfo.m_bIsMouseOverCloseBtn, tabBg);
			if (tabInfo.m_bIsMouseOver || tabInfo.m_isPinned)
			{
				_DrawPinButton(pRT, RectFfromRect(tabInfo.m_rcPinBtn), tabInfo.m_isPinned, tabInfo.m_bIsMouseOverPinBtn, tabBg);
			}
		}
		else if (tabInfo.m_isPinned)
		{
			_DrawPinButton(pRT, RectFfromRect(tabInfo.m_rcPinBtn), true, false, tabBg);
		}

		if (isCurrentTab)
		{
			// Draw thick line at top of tab to indicate "current" tab.
			CRect rcCurTab(rc.left, rc.top + 3, rc.right, rc.top + 6);
			pRT->FillRectangle(RectFfromRect(rcCurTab),
					GetBrush(tabsList.m_isFocusVwInThis ? CID::TabFocusBg : CID::TabCurBg));
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
		m_ptLbtnDown = point;
		if (m_listTabs->IsPointInDropListButtons(point))
		{
			return 0;	// Drop down buttons click are handled in LbtnUp
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
		if (!m_isDraggingTab)
		{
			if (std::abs(point.x - m_ptLbtnDown.x) > 5 || std::abs(point.y - m_ptLbtnDown.y) > 5)
			{
				return 0;	// Ignore button up event if to far away from where L button down.
			}
			if (m_listTabs->m_rcLeftTabListDropDownBtn.PtInRect(point))
			{
				_OnTabListButton(true);
				return 0;
			}
			if (m_listTabs->m_rcRightTabListDropDownBtn.PtInRect(point))
			{
				_OnTabListButton(false);
				return 0;
			}
		}

		ReleaseCapture();

		if (m_isDraggingTab)
		{
			m_isDraggingTab = false;
			if (_MousePointInOtherTabView(point))
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
			CVwInfo* pTabInfo = m_listTabs->GetTabAtPoint(point);
			if (pTabInfo)
			{
				XeASSERT(pTabInfo->m_pView && pTabInfo->m_pView->GetHwndOfView());

				bool isCurrentView = pTabInfo->m_pView == _GetCurrentView();
				bool isPointOverCloseButton = pTabInfo->m_rcCloseBtn.PtInRect(point);
				bool isPointOverPinButton = pTabInfo->m_rcPinBtn.PtInRect(point);
				if (isPointOverPinButton)
				{
					if (m_listTabs->PinTab(!pTabInfo->m_isPinned, pTabInfo->m_pView))
					{
						_UpdateLayout(true, true);
						m_pVwMgr->OnTabOrderChanged();
					}
				}
				else if (!isPointOverCloseButton		// Cursor not over close button?
					&& !isCurrentView)			// Only switch when different
				{
					SwitchToView(pTabInfo->m_pView, true);
				}
				else
				{
					if (abs(m_ptDragStart.x - point.x) < 5		// Mouse is close to where Lbtn clicked
						&& isPointOverCloseButton)				// AND Cursor over close button?
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
		CVwInfo* pTabInfo = m_listTabs->GetTabAtPoint(point);
		if (pTabInfo)
		{
			if (pTabInfo->m_rcCloseBtn.PtInRect(point))
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
			else
			{
				CPoint ptDiff = m_ptDragStart - point;
				CXeFileVwIF* pDraggedTabVw = m_listTabs->GetTabViewPtr(m_draggedTab);
				if (pDraggedTabVw && (abs(ptDiff.x) >= 10 || abs(ptDiff.y) >= 10))
				{
					SwitchToView(pDraggedTabVw, true);
					m_isDraggingTab = true;
					SetCapture();
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
			cursor = _MousePointInOtherTabView(point) ? IDC_PIN : IDC_NO;
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

protected:
	bool _MousePointInOtherTabView(CPoint point)
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

	CVwInfo* _GetTabAtPointInOtherView(CPoint point)
	{
		if (m_pOtherView)
		{
			CPoint ptOtherView = point;
			ClientToScreen(&ptOtherView);
			m_pOtherView->ScreenToClient(&ptOtherView);
			CVwInfo* pTabInfo = m_pOtherView->m_listTabs->GetTabAtPoint(ptOtherView);
			if (pTabInfo)
			{
				return pTabInfo;
			}
		}
		return nullptr;
	}

	dsid_t _GetTabInsertDS_InOtherView(CPoint point)
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
			if (m_pOtherView->m_listTabs->m_rcRightTabListDropDownBtn.PtInRect(ptOtherView))
			{
				return dsid_t::MakeDSID(0xFFFFFFFF);	// => insert at back.
			}
			CVwInfo* pTabInfo = _GetTabAtPointInOtherView(point);
			if (pTabInfo)
			{
				return pTabInfo->m_dsid;
			}
			// Note - point is not in any tab nor in drop down buttons - so insert point is assumed at back.
		}
		return dsid_t::MakeDSID(0xFFFFFFFF);	// => insert at back.
	}
#pragma endregion Mouse_Processing


#pragma region Context_Menu
public:
	void UpdateMenuItem(ListBoxExItem& menu_item, CXeFileVwIF* pView)
	{
		// IMPORTANT! pView can be null ! ! ! ! ! ! ! ! ! ! !

		dsid_t dataSourceId;
		//DSType dataSourceType = DSType::UNKNOWN;
		if (pView)
		{
			dataSourceId = pView->GetDataSourceId();
			//dataSourceType = pView->GetDataSourceType();
		}
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
		CXeMenu menu(m_xeUI, MAKEINTRESOURCE(IDR_TAB_CONTEXT_MENU));
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

		bool isTabOrderChanged = false;
		const CVwInfo* pTabInfo = m_listTabs->GetTab(pView);
		//const FileMetadata& md = pView->GetConstFileContainerUI_IF()->GetMetadata();
		dsid_t datasourceId = pView->GetDataSourceId();
		switch (selItem)
		{
		case ID__OPENCONTAININGFOLDER:
			m_pVwMgr->OpenContainingFolder(datasourceId);
			break;
		case ID__MOVETOOTHER:
			isTabOrderChanged = _OnMoveToOtherView(datasourceId);
			break;
		case ID__MOVETOFIRSTTAB:
			isTabOrderChanged = _MoveTab(EMOVETAB::eFIRST, pView);
			break;
		case ID__MOVETOLASTTAB:
			isTabOrderChanged = _MoveTab(EMOVETAB::eLAST, pView);
			break;
		case ID__MOVELEFT:
			isTabOrderChanged = _MoveTab(EMOVETAB::eLEFT, pView);
			break;
		case ID__MOVERIGHT:
			isTabOrderChanged = _MoveTab(EMOVETAB::eRIGHT, pView);
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
			if (pTabInfo && m_listTabs->PinTab(!pTabInfo->m_isPinned, pView))
			{
				_UpdateLayout(true, true);
				isTabOrderChanged = true;
			}
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

		if (isTabOrderChanged)
		{
			m_pVwMgr->OnTabOrderChanged();
		}
	}

	bool _CanMoveToOtherView()
	{
		if (GetTotalTabCount() <= 1)
		{
			return false;
		}
		return true;
	}

	bool _OnMoveToOtherView(dsid_t dsid)
	{
		_MoveToOtherView(dsid, dsid_t());
		return true;
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

			if (m_listTabs->IsPointInDropListButtons(pt))
			{
				return 0;	// Drop down buttons have no tooltips
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

	void _OnTabListButton(bool isLeftButton)
	{
		m_listTabs->m_isMouseOverLeftTabListDropDownBtn = m_listTabs->m_isMouseOverRightTabListDropDownBtn = false;

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
			//const FileMetadata& md = pView->GetFileContainerUI_IF()->GetMetadata();
			ListBoxExItem item = ListBoxExItem::MakeCheckBox(pView->GetViewName(), false);
			item.m_extra_data = pView->GetDataSourceId().get();
			item.m_item_data.SetIcon(pView->GetViewPID());
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

		CRect rcParentCtrl = isLeftButton
			? m_listTabs->m_rcLeftTabListDropDownBtn : m_listTabs->m_rcRightTabListDropDownBtn;
		ClientToScreen(rcParentCtrl);
		CRect rc;
		GetWindowRect(rc);
		int x = isLeftButton ? rc.left : rc.right - cxMaxName;
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

