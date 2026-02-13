module;

#include "os_minimal.h"
#include <memory>
#include <algorithm>
#include <iterator>
#include <vector>
#include <string>
#include <map>
#include <optional>
#include <sstream>
#include "logging.h"
#include "XeResource.h"
//#include "CustomWndMsgs.h"
#include <boost/algorithm/string/erase.hpp>

export module Xe.ViewManager;

export import Xe.ViewManagerIF;
//import Xe.FileVw;
import Xe.TabsView;
import Xe.FileHelpers;
import Xe.FileDlgHelpers;
//import Xe.FindHistory;
import Xe.UIcolorsIF;
//import Xe.LogGridCtrlBase;
import Xe.OpenFileExplorerAsync;
import Xe.UIWorkThread;
import Xe.UserSettingsForUI;
import Xe.MessageBoxDlg;
import Xe.AskStringDlg;
import Xe.ViewsListWnd;
import Xe.MRUList;
import Xe.ThemeEditorDlg;
import Xe.SettingsDlg;
import Xe.VerifyDirectoryAccessible;
import Xe.Menu;
import Xe.Helpers;
//import Xe.FileContainerIF;
import Xe.Utils;
import Xe.StringTools;

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif

// Automatic software upgrade code is only enabled for release builds.
#ifndef _DEBUG
#define UPGRADE_ENABLED
#endif // DEBUG

// Un-comment the following to enable software upgrade in debug mode.
//#define UPGRADE_ENABLED

export class CXeViewManager : public virtual CXeViewManagerIF
{
private:
	VSRL::Logger& logger() { return VSRL::s_pVSRL->GetInstance("CXeViewManager"); }

protected:
	HWND m_hMainWnd = nullptr;
	CXeD2DToolbarIF* m_pMainWndToolbar = nullptr;

	CXeUIcolorsIF* m_xeUI = nullptr;
	std::vector<std::unique_ptr<CXeTabsView>> m_tabViews;

	std::map<dsid_t, std::unique_ptr<CXeFileVwIF>> m_views;

	CXeUIWorkThread m_uiWorkThread;

	CXeOpenFileExplorerAsync m_openFileExplorerAsync;

	HWND m_hParentOfTabVw = nullptr;

	// DataSourceId of view that has focus.
	dsid_t m_dwLastViewWithFocus;

	std::unique_ptr<CViewsListWnd> m_viewListWnd;
	HWND m_hOldFocusWnd = 0;	// Handle of window that had focus before tab list dlg shown.

	std::unique_ptr<CXeThemeEditorDlg> m_themeEditorDlg;

	UINT m_uNextListId = 202;

	CMRUList m_MRUList;

	CVerifyDirectoryAccessible m_verifyDirAccessible;

	std::wstring m_strWindowTitle;
	std::wstring m_strAppName = L"Log Viewer Studio";
	std::wstring m_strAppShortName = L"LVS";

	std::wstring m_settings_dlg_initial_setting_name;

public:
	CXeViewManager(CXeUIcolorsIF* pUIcolors) : m_xeUI(pUIcolors) {}
	virtual ~CXeViewManager() {}

	// Delete assignment operator and copy constructor to prevent object copy.
	CXeViewManager(const CXeViewManager&) = delete;
	CXeViewManager& operator=(const CXeViewManager&) = delete;

	int Create(HWND hParentOfTabVw, UINT uLeftTabDlgCtrlId, UINT uRightTabDlgCtrlId,
			UINT uLeftViewDlgCtrlId, UINT uRightViewDlgCtrlId)
	{
		m_hParentOfTabVw = hParentOfTabVw;
		XeASSERT(hParentOfTabVw && m_tabViews.size() == 0);

		std::wstring settings_path = GetCurrentUserAppDataFolder(m_xeUI->GetAppName());

		m_MRUList.SetMaxSize(s_xeUIsettings[L"GeneralSettings"].Get(L"MRU_Items").getU32());
		m_MRUList.LoadFromFile(settings_path + L"OpenFileMRUlist.json");
		
		m_uiWorkThread.StartThread();

		m_tabViews.push_back(std::make_unique<CXeTabsView>(this));
		m_tabViews.push_back(std::make_unique<CXeTabsView>(this));
		m_tabViews[0]->Create(hParentOfTabVw, ETABVIEWID::ePrimaryTabVw, uLeftTabDlgCtrlId, uLeftViewDlgCtrlId, m_tabViews[1].get());
		m_tabViews[1]->Create(hParentOfTabVw, ETABVIEWID::eSecondaryTabVw, uRightTabDlgCtrlId, uRightViewDlgCtrlId, m_tabViews[0].get());
		return m_tabViews[0]->GetTabViewHeight();
	}

	virtual void OnMainWindowCreate() override
	{
	}

	virtual void OnMainWindowDestroy() override
	{
		_CloseAllViews();
	}

	virtual void Destroy() override
	{
		// ?? TODO - update this comment - is it correct ?? Note all windows have been closed already.

		m_uiWorkThread.StopThread();
	}

protected:
	virtual void _CloseAllViews()
	{
		DeleteAllViews();
	}

	// Close all windows menu cmd.
	virtual void _OnCloseAllWindows()
	{
		_CloseAllViews();
	}

public:
	virtual void SetMainWindowPtr(HWND hMainWnd, CXeD2DToolbarIF* pMainWndToolBar) override
	{
		m_hMainWnd = hMainWnd;
		m_pMainWndToolbar = pMainWndToolBar;
		XeASSERT(m_hMainWnd && m_pMainWndToolbar);

		m_pMainWndToolbar->SetUpdateMenuCallback(
			[this](CXeMenu* pMenu, size_t top_level_index) { _UpdateMenuCallback(pMenu, top_level_index); });

		::SetWindowText(m_hMainWnd, m_strAppName.c_str());
	}

	virtual void On_Timer_1S() override
	{
	}

	void _UpdateMainWindowTitle(const std::wstring& title)
	{
		m_strWindowTitle = title;
		std::wstring txt = m_strWindowTitle.size() ? m_strAppShortName + L" - " + title : m_strAppName;
		::SetWindowTextW(m_hMainWnd, txt.c_str());
	}

	virtual void SetFocusToCurrentView() override
	{
		if (GetTotalViewCount() > 0)
		{
			SetFocusToView(m_dwLastViewWithFocus);
		}
	}

	virtual int GetTabViewHeight(int idx) override { return m_tabViews.size() > idx ? m_tabViews[idx]->GetTabViewHeight() : 0; }
	virtual int RecalculateTabViewHeight(int idx, int cxAvailable) override
	{
		return m_tabViews.size() > idx ? m_tabViews[idx]->RecalculateTabViewHeight(cxAvailable) : 0;
	}

	virtual void OnViewClosing(dsid_t dwDataSourceId) override
	{
	}

	virtual void OnViewClosed(dsid_t dataSourceId) override
	{
		m_views.erase(dataSourceId);
	}

	virtual void DeleteAllViews() override
	{
		_GetTabView(ETABVIEWID::eSecondaryTabVw)->DeleteAllTabsAndViews();

		_GetTabView(ETABVIEWID::ePrimaryTabVw)->DeleteAllTabsAndViews();
	}

	virtual void FindAndDeleteViewWithDataSourceId(dsid_t dwDataSourceId) override
	{
		for (auto& pTabView : _GetAllTabViews())
		{
			if (pTabView->FindAndDeleteTabAndView(dwDataSourceId))
			{
				break;
			}
		}
	}

	virtual void OnViewRenamed(CXeFileVwIF* pView, const std::wstring& strNewTitle) override
	{
		CXeTabsView* pTabVw = _GetTabViewContainingView(pView);
		XeASSERT(pTabVw);
		pTabVw->RenameTab(pView, strNewTitle);
	}

	virtual void OnChangedSettings(const ChangedSettings& chg_settings) override
	{
		for (auto& pTabView : m_tabViews)
		{
			pTabView->OnChangedSettings(chg_settings);
		}

		if (chg_settings.IsChanged(L"GeneralSettings", L"MRU_Items"))
		{
			uint32_t mru_items = s_xeUIsettings[L"GeneralSettings"].Get(L"MRU_Items").getU32();
			m_MRUList.SetMaxSize(mru_items);
		}

		m_pMainWndToolbar->OnChangedSettings(chg_settings);
	}

	virtual CXeUIcolorsIF* GetUIcolors() override { return m_xeUI; }

	virtual bool SwitchToView(CXeFileVwIF* pView, bool setFocusToView) override
	{
		CXeTabsView* pTabVw = _GetTabViewContainingView(pView);
		if (pTabVw)
		{
			return pTabVw->SwitchToView(pView, setFocusToView) ? true : false;
		}
		return false;
	}

	virtual bool SwitchToViewWithDataSourceId(dsid_t dwDataSourceId, bool isSetFocus) override
	{
		CXeFileVwIF* pView = GetViewFromDataSourceId(dwDataSourceId);
		bool result = SwitchToView(pView, false);
		if (result && isSetFocus)
		{
			pView->SetFocusToView();
		}
		return result;
	}

	virtual bool SwitchToViewAtIndex(ETABVIEWID tabVwId, UINT uIndex) override
	{
		CXeTabsView* pTabsVw = _GetTabView(tabVwId);
		XeASSERT(pTabsVw);
		if (pTabsVw)
		{
			return pTabsVw->SwitchToViewAtIndex(uIndex);
		}
		return false;
	}

	virtual bool SwitchToView(ViewNavigate navigate, dsid_t dwDataSourceIdOfViewWithFocus) override
	{
		ETABVIEWID tabVwId = _GetTabVwOfView(dwDataSourceIdOfViewWithFocus);
		if (tabVwId != ETABVIEWID::eAnyTabVw)
		{
			CXeTabsView* pTabsVw = _GetTabView(tabVwId);
			if (pTabsVw)
			{
				return pTabsVw->SwitchToView(navigate);
			}
		}
		return false;
	}

	virtual ETABVIEWID GetTabViewIdOfView(dsid_t dwDataSourceId) const override
	{
		return _GetTabVwOfView(dwDataSourceId);
	}

	virtual size_t GetTabViewCount(ETABVIEWID vwId) const override
	{
		CXeTabsView* pTabsVw = _GetTabView(vwId);
		if (pTabsVw)
		{
			return pTabsVw->GetTabCount();
		}
		return 0;
	}

	virtual size_t GetTabViewCurrentIndex(ETABVIEWID vwId) const override
	{
		CXeTabsView* pTabsVw = _GetTabView(vwId);
		if (pTabsVw)
		{
			return pTabsVw->GetIndexOfCurrentTab();
		}
		return 0xFFFFFFFF;
	}

	virtual UINT GetTotalViewCount() override
	{
		UINT uViewCount = 0;
		for (auto& pTabView : m_tabViews)
		{
			uViewCount += (UINT)pTabView->GetTabCount();
		}
		return uViewCount;
	}

	// Get tab view from point (screen coords.), return tab view id if point is in a tab view or file view else eAnyTabVw.
	virtual ETABVIEWID GetTabVwFromPoint(POINT& pt) override
	{
		for (auto& pTabView : m_tabViews)
		{
			if (pTabView->IsPointInThisView(pt))
			{
				return pTabView->GetTabVwId();
			}
		}
		return ETABVIEWID::eAnyTabVw;
	}

	virtual CXeFileVwIF* GetViewFromDataSourceId(dsid_t dataSourceId) override
	{
		auto it = m_views.find(dataSourceId);
		if (it != m_views.end())
		{
			return it->second.get();
		}
		return nullptr;
	}

	virtual const CXeFileVwIF* GetViewFromDataSourceIdConst(dsid_t dataSourceId) const
	{
		auto it = m_views.find(dataSourceId);
		if (it != m_views.end())
		{
			return it->second.get();
		}
		return nullptr;
	}

	// Send message to all views.
	virtual void SendMessageToAllViews(UINT uMessage, WPARAM wParam, LPARAM lParam) override
	{
		for (auto& pTabView : m_tabViews)
		{
			pTabView->SendMessageToAllViews(uMessage, wParam, lParam);
			pTabView->SendMessageW(uMessage, wParam, lParam);
		}
		HWND hWndHorzVw = ::GetDlgItem(m_hMainWnd, VW_ID_HVIEW);
		if (hWndHorzVw)
		{
			::SendMessageW(hWndHorzVw, uMessage, wParam, lParam);
		}
		HWND hWndSideVw = ::GetDlgItem(m_hMainWnd, VW_ID_SIDE);
		if (hWndSideVw)
		{
			::SendMessageW(hWndSideVw, uMessage, wParam, lParam);
		}
	}

	// Get tab view window rect in screen coords.
	virtual CRect GetTabViewWindow(ETABVIEWID tabVwId) override
	{
		CRect rcTabVw;
		CXeTabsView* pTabVw = _GetTabView(tabVwId);
		if (pTabVw)
		{
			pTabVw->GetWindowRect(&rcTabVw);
		}
		return rcTabVw;
	}

	virtual CXeCancelEvent* DoWorkThreadWork(WorkThreadWorkCallbackFunc wt_callback) override
	{
		return m_uiWorkThread.DoWorkThreadWork(wt_callback);
	}

	virtual CXeCancelEvent* GetWorkThreadWorkCancelEvent() override
	{
		return m_uiWorkThread.GetCancelEvent();
	}

	// Called from LVSState when view with focus has changed.
	virtual void ViewWithFocusChanged(dsid_t dwDataSourceId) override
	{
		for (auto& pTabView : m_tabViews)
		{
			pTabView->ViewWithFocusChanged(dwDataSourceId);
		}
	}

	// Called from CXeSplitWndLVS::OnSetFocus - set focus to last view with focus.
	virtual void SetFocusToView(dsid_t dwDataSourceId) override
	{
		for (auto& pTabView : m_tabViews)
		{
			CXeFileVwIF* pView = pTabView->GetCurrentView();
			if (pView)
			{
				//const CXeFileContainerUI_IF* ui_ds = pView->GetConstFileContainerUI_IF();
				if (pView->GetDataSourceId() == dwDataSourceId)
				{
					//if (!ui_ds->GetIsBusyMergingOnWTFlag())
					{
						::SetFocus(pView->GetHwndOfView());
					}
					break;
				}
			}
		}
	}

	virtual bool IsViewVisible(dsid_t dwDataSourceId) override
	{
		CXeTabsView* pTabVw = _GetTabViewContainingView(dwDataSourceId);
		if (pTabVw)
		{
			return pTabVw->GetDataSourceIdOfCurrentView() == dwDataSourceId;
		}
		return false;
	}

	virtual bool IsLastViewWithFocus(dsid_t dwDataSourceId) override
	{
		return dwDataSourceId == m_dwLastViewWithFocus;
	}

	virtual void OnTabOrderChanged() override
	{
		::SendMessage(m_hMainWnd, WMU_REPOSITION_WINDOWS, 0, 0);
	}

	virtual bool OnViewGotFocus(dsid_t dwDataSourceId) override
	{
		if (GetViewFromDataSourceId(dwDataSourceId))	// Is valid view
		{
			//TRACE("View got focus DS: %d\n", dwDataSourceId);
			m_dwLastViewWithFocus = dwDataSourceId;
			ViewWithFocusChanged(m_dwLastViewWithFocus);
			return true;
		}
		return false;
	}

	virtual void OnViewLostFocus(dsid_t dwDataSourceId)
	{
		//TRACE("View lost focus DS: %d\n", dwDataSourceId);
	}

#pragma region UpdateMenuItems
protected:
	// Update menu callback - called from menu in toolbar when sub-menu is needed.
	virtual void _UpdateMenuCallback(CXeMenu* pMenu, size_t top_level_index)
	{
		XeASSERT(pMenu);
		if (pMenu)
		{
			CXeSubMenu* pSubMenu = pMenu->GetTopLevelMenuItem(top_level_index);
			for (ListBoxExItem& menu_item : pSubMenu->m_items)
			{
				bool isSubMenu = menu_item.m_item_data.m_isSubMenu;
				std::string menu_text(xet::to_astr(menu_item.m_string));
				boost::erase_all(menu_text, "&");
				if (menu_text.starts_with("Recent files"))
				{
					_OnUpdateRecentFilesMenu(pSubMenu, menu_item);
				}
				else if (isSubMenu && menu_text.starts_with("Theme"))
				{
					_OnUpdateThemeListmenu(pSubMenu, menu_item);
				}
				else if (isSubMenu)
				{
					_OnUpdateSubMenu(pSubMenu, menu_item);
				}
				else
				{
					_UpdateMenuItem(menu_item);
				}
			}
		}
	}

	// Update main menu item state
	void _OnUpdateSubMenu(CXeSubMenu* pSubMenu, ListBoxExItem& menu_item)
	{
		if (pSubMenu->m_sub_menus.contains((UINT)menu_item.m_extra_data))
		{
			std::vector<ListBoxExItem>& popup_list = pSubMenu->m_sub_menus.at((UINT)menu_item.m_extra_data);
			for (ListBoxExItem& submenu_item : popup_list)
			{
				if (submenu_item.m_item_data.m_isSubMenu)
				{
					_OnUpdateSubMenu(pSubMenu, submenu_item);
				}
				else
				{
					_UpdateMenuItem(submenu_item);
				}
			}
		}
	}

	void _OnUpdateRecentFilesMenu(CXeSubMenu* pSubMenu, ListBoxExItem& menu_item)
	{
		std::vector<ListBoxExItem> list;
		std::vector<std::wstring> mru = m_MRUList.GetMRU_MenuItems();
		UINT uID = ID_MRU_ITEM1;
		for (const std::wstring& menu_item_text : mru)
		{
			list.push_back(ListBoxExItem(uID++, menu_item_text));
		}
		pSubMenu->m_sub_menus.insert_or_assign((UINT)menu_item.m_extra_data, list);
	}

	void _OnUpdateThemeListmenu(CXeSubMenu* pSubMenu, ListBoxExItem& menu_item)
	{
		XeThemeIF* pCurTheme = m_xeUI->GetCurrentTheme();
		bool isThemeEditorOpen = pCurTheme->IsInEditMode();
		std::wstring cur_theme_name = pCurTheme->GetThemeName();
		std::vector<ListBoxExItem> list;
		std::vector<std::wstring> list_themes = m_xeUI->GetThemeNames();
		UINT uNewID = ID_THEME_LIST_ITEM1;
		for (const std::wstring& theme_name : list_themes)
		{
			IsChecked isChecked = theme_name == cur_theme_name ? IsChecked::yes : IsChecked::no;
			IsEnabled isEnabled = (wcscmp(theme_name.c_str(), L"Light theme") == 0 ? IsEnabled::no : IsEnabled::yes);
#ifdef _DEBUG
			isEnabled = IsEnabled::yes;
#endif
			if (isThemeEditorOpen) { isEnabled = IsEnabled::no; }
			list.push_back(ListBoxExItem(uNewID++, theme_name, IsSeparator::no, isChecked, isEnabled));
		}
		pSubMenu->m_sub_menus.insert_or_assign((UINT)menu_item.m_extra_data, list);
	}

	virtual void _UpdateMenuItem(ListBoxExItem& menu_item)
	{
		// Note to self - remember to update the menu in the .rc file if any ID is changed.
		UINT uCmdID = (UINT)menu_item.m_extra_data;
		if (uCmdID >= ID_TAB_1STTAB && uCmdID <= ID_TAB_NEXTTAB)
		{
			ETABVIEWID tabVwId = GetTabViewIdOfView(m_dwLastViewWithFocus);
			size_t tabCount = GetTabViewCount(tabVwId);
			size_t curTabIdx = GetTabViewCurrentIndex(tabVwId);
			bool hasTabs = tabCount > 0;
			if (uCmdID == ID_TAB_PREVTAB)
			{
				menu_item.EnableItem(hasTabs && curTabIdx > 0);
			}
			else if (uCmdID == ID_TAB_NEXTTAB)
			{
				menu_item.EnableItem(hasTabs && curTabIdx < (tabCount - 1));
			}
			else
			{
				size_t idx = uCmdID - ID_TAB_1STTAB;
				menu_item.EnableItem(idx < tabCount);
			}
			return;
		}
		switch (uCmdID)
		{
		case ID_FILE_SET_TITLE:
			menu_item.EnableItem(GetTotalViewCount() > 0);
			break;
		case ID_FILE_CLOSEALLWINDOWS:
			menu_item.EnableItem(GetTotalViewCount() > 0);
			break;
		case ID_TOOLS_THEMEEDITOR:
			menu_item.EnableItem(m_xeUI->GetCurrentTheme()->IsInEditMode() ? false : true);
			break;
		default: {
			CXeFileVwIF* pView = GetViewFromDataSourceId(m_dwLastViewWithFocus);
			CXeTabsView* pTab = _GetTabViewContainingView(pView);
			if (!pTab)
			{
				pTab = _GetTabView(ETABVIEWID::ePrimaryTabVw);
			}
			pTab->UpdateMenuItem(menu_item, pView);
		} break;
		}
	}
#pragma endregion UpdateMenuItems

#pragma region CommandProcessing
public:
	// Process menu command or LVS_COMMAND (hot key cmd)
	virtual void ProcessCommand(UINT uCmdId, UINT param) override
	{
		// http://chabster.blogspot.com/2010/03/focus-and-window-activation-in-win32.html
		HWND hOldFocusWnd = ::GetFocus();
		bool isRestoreFocusNeeded = false;

		if (uCmdId >= ID_MRU_ITEM1 && uCmdId <= ID_MRU_ITEM30)
		{
			OpenFileFromMRUlist(uCmdId - ID_MRU_ITEM1);
			return;
		}
		if (uCmdId >= ID_THEME_LIST_ITEM1 && uCmdId <= ID_THEME_LIST_ITEM99)
		{
			_OnThemeMenuSelect(uCmdId);
			return;
		}
		//if (uCmdId >= ID_TOGGLE_BOOKMARK && uCmdId <= ID_CLEAR_BOOKMARKS)
		//{
		//	::SendMessage(::GetFocus(), WM_COMMAND, uCmdId, 0);	// Forward to log grid
		//}
		switch (uCmdId)
		{
		case ID_FILE_CLOSEALLWINDOWS:
			_OnCloseAllWindows();
		case ID__OPENCONTAININGFOLDER:
			OpenContainingFolder(m_dwLastViewWithFocus);
			break;
		case ID_CLOSE_CURRENT_FILE:
			FindAndDeleteViewWithDataSourceId(m_dwLastViewWithFocus);
			break;
		case ID_FILE_SET_TITLE:
			isRestoreFocusNeeded = true;
			_OnSetWindowTitle();
			break;
		case ID_TOOLS_THEMEEDITOR:
			_ShowThemeEditorDlg();
			break;
		case ID_SHOW_SETTINGS:
			_ShowSettingsDlg();
			break;
		case ID_FONT_SIZE:
			_ChangeFontSize();
			break;
		case ID_TAB_1STTAB:
		case ID_TAB_2NDTAB:
		case ID_TAB_3RDTAB:
		case ID_TAB_4THTAB:
		case ID_TAB_5THTAB:
		case ID_TAB_6THTAB:
		case ID_TAB_7THTAB:
		case ID_TAB_8THTAB:
		case ID_TAB_9THTAB:
			ProcessCommand(XE_CMD_NAVIGATE_TAB_1_9, uCmdId - ID_TAB_1STTAB);
			break;
		case ID_TAB_PREVTAB:
			ProcessCommand(XE_CMD_NAVIGATE_TAB_PREV, 0);
			break;
		case ID_TAB_NEXTTAB:
			ProcessCommand(XE_CMD_NAVIGATE_TAB_NEXT, 0);
			break;
		case XE_CMD_NAVIGATE_TAB_1_9:
			SwitchToViewAtIndex(GetTabViewIdOfView(m_dwLastViewWithFocus), param);
			break;
		case XE_CMD_NAVIGATE_TAB_PREV:
			SwitchToView(ViewNavigate::Prev, m_dwLastViewWithFocus);
			break;
		case XE_CMD_NAVIGATE_TAB_NEXT:
			SwitchToView(ViewNavigate::Next, m_dwLastViewWithFocus);
			break;
		case XE_CMD_NAVIGATE_TAB_ID:
			SwitchToViewWithDataSourceId(dsid_t::MakeDSID(param), true);
			break;
		case XE_CMD_SHOW_TABS_LIST_DLG:
			_ShowTabsListDlg();
			break;
		default:
			//XeASSERT(FALSE);	// Unknown command. Need to add support for it?
			break;
		}
		if (isRestoreFocusNeeded && hOldFocusWnd)
		{
			::SetFocus(hOldFocusWnd);
		}
	}

	virtual void OpenFileFromMRUlist(size_t idx) override
	{
	}

	void _ShowTabsListDlg()
	{
		if (m_viewListWnd.get() == nullptr)
		{
			std::vector<CXeFileVwIF*> allViews = GetAllViews();
			if (allViews.size())
			{
				m_hOldFocusWnd = ::GetFocus();
				CRect rcMaxSize, rcTabVw = GetTabViewWindow(ETABVIEWID::ePrimaryTabVw);
				::GetWindowRect(m_hMainWnd, &rcMaxSize);
				rcMaxSize.top = rcTabVw.bottom;
				rcMaxSize.bottom -= 10;
				dsid_t dwDataSourceIdOfCurrentView = m_dwLastViewWithFocus;
				m_viewListWnd = std::make_unique<CViewsListWnd>(m_xeUI, m_hMainWnd, allViews, rcMaxSize,
						dwDataSourceIdOfCurrentView,
						[this](dsid_t dwSelectedDataSourceId)
						{ this->_OnNotifyTabListWndDestroyed(dwSelectedDataSourceId); });
			}
		}
	}

	void _OnNotifyTabListWndDestroyed(dsid_t dwSelectedDataSourceId)
	{
		if (!SwitchToViewWithDataSourceId(dwSelectedDataSourceId, true)
			&& m_hOldFocusWnd)
		{
			::SetFocus(m_hOldFocusWnd);
			m_hOldFocusWnd = 0;
		}
		m_viewListWnd.reset();
	}

	virtual void _OnSetWindowTitle()
	{
		std::optional<std::wstring> str = AskStringDlg(m_hMainWnd, L"Set Window Title",
				GetWindowTextStdW(m_hMainWnd));
		if (str.has_value())
		{
			_UpdateMainWindowTitle(str.value());
		}
	}

	void _ShowSettingsDlg()
	{
		CRect rcBtn = m_pMainWndToolbar->GetItemRect(ID_XE_FILE_OPEN);
		rcBtn.OffsetRect(-30, 8);
		CXeSettingsDlg dlg(this, &rcBtn, m_hMainWnd);
		dlg.m_initial_setting_name = m_settings_dlg_initial_setting_name;
		dlg.DoModal();
		m_settings_dlg_initial_setting_name = dlg.m_initial_setting_name;

#ifdef UPGRADE_ENABLED
		// Check if SW upgrade setting was changed.
		bool isSWupgradeEnabled = s_xeUIsettings[L"SoftwareUpgradeSettings"].Get(L"AutoUpgradeEnabled").getBool();
		if (isSWupgradeEnabled && !m_pLVSupdater)
		{
			m_pLVSupdater = std::make_unique<CLVSupdater>(m_hMainWnd, WMU_NOTIFY_SOFTWARE_UPGRADE);
		}
		else if (!isSWupgradeEnabled && m_pLVSupdater)	// disable SW upgrade AND upgrader is running?
		{
			m_pLVSupdater.reset();
		}
#endif // UPGRADE_ENABLED
	}

	void _ChangeFontSize()
	{
		std::optional<std::wstring> selSize = m_pMainWndToolbar->GetDropListButtonCurSel(ID_FONT_SIZE);
		if (selSize.has_value())
		{
			CXeUserSettingsForUI all_settings_for_ui_copy = s_xeUIsettings;
			all_settings_for_ui_copy[L"GeneralSettings"].Set(L"UI_FontPointSize", selSize.value());
			all_settings_for_ui_copy[L"LogFileGridSettings"].Set(L"GridFontPointSize", selSize.value());
			s_xeUIsettings.SaveChangedSettings(all_settings_for_ui_copy);
		}
	}

	virtual void OnTabCtxMenuCmd(UINT uCmdId, ETABVIEWID tabId, dsid_t datasourceId) override
	{
	}
#pragma endregion CommandProcessing

#pragma region UI_Theme
public:
	virtual void ChangeTheme(const std::wstring& theme_name)
	{
		if (m_xeUI->SetTheme(theme_name))
		{
			s_xeLastUsedUIsettings.Set(L"LastUsedTheme", theme_name);
			::SendMessage(m_hMainWnd, WMU_NOTIFY_CHANGES, NFCHG_CODE_SEL_THEME, NFCHG_SENDER_MAINFRAME);
			ChangedSettings chg_settings;
			chg_settings.m_chg_settings.push_back(ChangedSetting(L"Colors", L"Color"));
			chg_settings.m_chg_settings.push_back(ChangedSetting(L"GeneralSettings", L"UI_FontName"));
			chg_settings.m_chg_settings.push_back(ChangedSetting(L"GeneralSettings", L"UI_FontPointSize"));
			s_xeUIsettings.TriggerNotifyCallback(chg_settings);
		}
	}

protected:
	void _ShowThemeEditorDlg()
	{
		if (m_themeEditorDlg.get() != nullptr)
		{
			HWND hDlgWnd = m_themeEditorDlg->Hwnd();
			if (::IsIconic(hDlgWnd))
			{
				WINDOWPLACEMENT wplc;
				wplc.length = sizeof(WINDOWPLACEMENT);
				::GetWindowPlacement(hDlgWnd, &wplc);
				wplc.showCmd = SW_RESTORE;
				::SetWindowPlacement(hDlgWnd, &wplc);
			}
			::SetFocus(hDlgWnd);
		}
		else
		{
			m_themeEditorDlg = std::make_unique<CXeThemeEditorDlg>(this, m_hMainWnd,
				[this]() { _OnThemeEditorNotifyColorChanged(); },
				[this](const std::wstring& theme_name) { _OnThemeEditorDlg_SaveTheme(theme_name); });
			m_themeEditorDlg->CreateModelessDialog([this]() { _OnThemeEditorDlgDestroyed(); });
			m_pMainWndToolbar->SetItemEnabledState(ID_FONT_SIZE, FALSE);
		}
	}

	void _OnThemeEditorDlgDestroyed()
	{
		m_themeEditorDlg.reset();
		m_pMainWndToolbar->SetItemEnabledState(ID_FONT_SIZE, TRUE);
	}

	void _OnThemeEditorNotifyColorChanged()
	{
		ChangedSettings chg_settings;
		chg_settings.m_chg_settings.push_back(ChangedSetting(L"Colors", L"Color"));
		s_xeUIsettings.TriggerNotifyCallback(chg_settings);
	}

	void _OnThemeEditorDlg_SaveTheme(const std::wstring& theme_name)
	{
		m_xeUI->ReloadThemeJsonFiles();
		ChangeTheme(theme_name);
	}

	void _OnThemeMenuSelect(UINT uThemeNameMenuID)
	{
		size_t idx = uThemeNameMenuID >= ID_THEME_LIST_ITEM1 ? uThemeNameMenuID - ID_THEME_LIST_ITEM1 : 0xFFFF;
		std::vector<std::wstring> list_themes = m_xeUI->GetThemeNames();
		if (idx < list_themes.size())
		{
			ChangeTheme(list_themes.at(idx));
		}
	}
#pragma endregion UI_Theme

	virtual bool CanOpenContainingFolder(dsid_t dsId) override
	{
		return false;
	}

	virtual void OpenContainingFolder(dsid_t dsId) override
	{
	}

	virtual void CopyInfoToClipboard(ECLIPBRDOP operation, dsid_t dsId) override
	{
		CXeFileVwIF* pView = GetViewFromDataSourceId(dsId);
		if (pView)
		{
			pView->OnCopyInfoToClipboard(operation);
		}
	}

	//virtual int ShowMessageBox(CWnd* pParent, const wchar_t* pMsg, const wchar_t* pTitle = nullptr,
	//	UINT uType = MB_OK, CRect* pRectCell = nullptr) override
	//{
	//	return XeMessageBox(this, pParent, pMsg, pTitle, uType, pRectCell);
	//}
	virtual int ShowMessageBox(HWND hParent, const wchar_t* pMsg, const wchar_t* pTitle = nullptr,
		UINT uType = MB_OK, CRect* pRectCell = nullptr) override
	{
		return XeMessageBox(this, hParent, pMsg, pTitle, uType, pRectCell);
	}

	virtual std::optional<std::wstring> AskStringDlg(HWND hParent, const std::wstring& title,
			const std::wstring& string, CRect* pRectCell = nullptr) override
	{
		CXeAskStringDlg dlgAsk(this, title, string, hParent, pRectCell);
		INT_PTR dlgResult = dlgAsk.DoModal();
		if (dlgResult == IDOK)
		{
			return dlgAsk.m_str2ask;
		}
		return std::nullopt;
	}

protected:
	std::vector<CXeTabsView*> _GetAllTabViews()
	{
		std::vector<CXeTabsView*> views;
		for (auto& pTabView : m_tabViews)
		{
			views.push_back(pTabView.get());
		}
		return views;
	}

	CXeTabsView* _GetTabView(ETABVIEWID tabVwId) const
	{
		XeASSERT(m_tabViews.size() == 2
			&& (tabVwId == ETABVIEWID::eAnyTabVw || tabVwId == ETABVIEWID::ePrimaryTabVw
				|| tabVwId == ETABVIEWID::eSecondaryTabVw));
		return tabVwId == ETABVIEWID::ePrimaryTabVw ? m_tabViews[0].get()
			: tabVwId == ETABVIEWID::eSecondaryTabVw ? m_tabViews[1].get()
			: tabVwId == ETABVIEWID::eAnyTabVw ? m_tabViews[0].get() : nullptr;
	}

	CXeTabsView* _GetTabViewContainingView(CXeFileVwIF* pView)
	{
		for (auto& pTabView : m_tabViews)
		{
			if (pTabView->FindView(pView))
			{
				return pTabView.get();
			}
		}
		return nullptr;
	}

	CXeTabsView* _GetTabViewContainingView(dsid_t dwDataSourceId)
	{
		for (auto& pTabView : m_tabViews)
		{
			if (pTabView->FindView(dwDataSourceId))
			{
				return pTabView.get();
			}
		}
		return nullptr;
	}

	std::vector<CVwInfo> _CopyAllTabsInfos()
	{
		std::vector<CVwInfo> allTabs;
		for (auto& pTabView : m_tabViews)
		{
			std::vector<CVwInfo> tabs = pTabView->GetAllTabs();
			allTabs.insert(allTabs.end(), tabs.begin(), tabs.end());
		}
		return allTabs;
	}

	ETABVIEWID _GetTabVwOfView(dsid_t dwDataSourceId) const
	{
		for (auto& pTabView : m_tabViews)
		{
			if (pTabView->FindView(dwDataSourceId) != nullptr)
			{
				return pTabView->GetTabVwId();
			}
		}
		return ETABVIEWID::eAnyTabVw;
	}

	virtual std::vector<CXeFileVwIF*> GetAllViews() override
	{
		std::vector<CXeFileVwIF*> allViews;
		std::vector<CVwInfo> allTabInfos = _CopyAllTabsInfos();
		std::for_each(allTabInfos.begin(), allTabInfos.end(),
			[&](const CVwInfo& tabInfo)
			{ allViews.push_back(tabInfo.m_pView); });
		return allViews;
	}

	virtual std::vector<CXeFileVwIF*> GetAllForegroundViews() override
	{
		std::vector<CXeFileVwIF*> allFgViews;
		for (auto& pTabView : m_tabViews)
		{
			CXeFileVwIF* pView = pTabView->GetCurrentView();
			if (pView)
			{
				allFgViews.push_back(pView);
			}
		}
		return allFgViews;
	}

	virtual std::vector<CXeFileVwIF*> GetAllNonForegroundViews() override
	{
		std::vector<CXeFileVwIF*> allFgViews = GetAllForegroundViews();
		_SortOnDataSourceId(allFgViews);
		std::vector<CXeFileVwIF*> allViews = GetAllViews();
		_SortOnDataSourceId(allViews);
		std::vector<CXeFileVwIF*> nonFgViews;
		std::set_difference(allViews.begin(), allViews.end(), allFgViews.begin(), allFgViews.end(),
			std::inserter(nonFgViews, nonFgViews.begin()),
			[](CXeFileVwIF* pA, CXeFileVwIF* pB)
			{
				return pA->GetDataSourceId() < pB->GetDataSourceId();
			});
		return nonFgViews;
	}

	virtual std::vector<CXeFileVwIF*> GetAllViewsFromDataSourceIds(const std::vector<dsid_t>& dataSourceIds) override
	{
		std::vector<dsid_t> dsIds = dataSourceIds;
		std::sort(dsIds.begin(), dsIds.end());
		std::vector<CXeFileVwIF*> views;
		for (CXeFileVwIF* pView : GetAllViews())
		{
			dsid_t dwDataSourceId = pView->GetDataSourceId();
			auto it = std::lower_bound(dsIds.begin(), dsIds.end(), dwDataSourceId);
			if (it != dsIds.end() && *it == dwDataSourceId)
			{
				views.push_back(pView);
			}
		}
		return views;
	}

	void _SortOnDataSourceId(std::vector<CXeFileVwIF*>& views)
	{
		std::sort(views.begin(), views.end(), [](CXeFileVwIF* pA, CXeFileVwIF* pB)
			{
				return pA->GetDataSourceId()
					< pB->GetDataSourceId();
			});
	}

#pragma region Misc
public:
	// WMU_NOTIFY_CHANGES message handler.
	virtual void OnNotifyChanges(WPARAM wParam, LPARAM lParam) override
	{
		SendMessageToAllViews(WMU_NOTIFY_CHANGES, wParam, lParam);
	}

	// This is called once after UI is fully visible and ready.
	virtual void OnUIcreateComplete() override
	{
	}
#pragma endregion Misc

};

