module;

#include "os_minimal.h"
#include <vector>
#include <optional>
#include <functional>
#include <string>

export module Xe.ViewManagerIF;

export import Xe.FileVwIF;
export import Xe.DefData;
import Xe.UIcolorsIF;
import Xe.D2DToolbarIF;
import Xe.UserSettings_Changed;
//import Xe.WorkContext;
import Xe.DefData;
import Xe.Utils;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class ViewNavigate { Prev, Next };

export class CXeViewManagerIF
{
public:
	virtual void OnMainWindowCreate() = 0;
	virtual void OnMainWindowDestroy() = 0;
	virtual void Destroy() = 0;

	// Set pointers to main window and main window toolbar, create tab views.
	virtual void CreateTabViews(HWND hMainWnd, CXeD2DToolbarIF* pMainWndToolBar) = 0;

	virtual void On_Timer_1S() = 0;

	virtual int GetTabViewHeight(int idx) = 0;
	virtual int RecalculateTabViewHeight(int idx, int cxAvailable) = 0;

	virtual void SetFocusToCurrentView() = 0;

	//virtual void OnViewClosing(dsid_t dwDataSourceId, DSType eType) = 0;
	//virtual void OnViewClosed(dsid_t dwDataSourceId, DSType eType) = 0;
	virtual void OnViewClosing(dsid_t dwDataSourceId) = 0;
	virtual void OnViewClosed(dsid_t dwDataSourceId) = 0;

	virtual void OnViewRenamed(CXeFileVwIF* pView, const std::wstring& strNewTitle) = 0;

	virtual void DeleteAllViews() = 0;

	virtual void FindAndDeleteViewWithDataSourceId(dsid_t dwDataSourceId) = 0;

	virtual void OnChangedSettings(const ChangedSettings& chg_settings) = 0;

	virtual CXeUIcolorsIF* GetUIcolors() = 0;
	virtual bool SwitchToView(CXeFileVwIF* pView, bool setFocusToView) = 0;
	virtual bool SwitchToViewWithDataSourceId(dsid_t dwDataSourceId, bool isSetFocus) = 0;
	virtual bool SwitchToViewAtIndex(ETABVIEWID tabVwId, UINT uIndex) = 0;
	virtual bool SwitchToView(ViewNavigate navigate, dsid_t dwDataSourceIdOfViewWithFocus) = 0;
	virtual ETABVIEWID GetTabViewIdOfView(dsid_t dwDataSourceId) const = 0;
	virtual size_t GetTabViewCount(ETABVIEWID vwId) const = 0;
	virtual size_t GetTabViewCurrentIndex(ETABVIEWID vwId) const = 0;

	virtual UINT GetTotalViewCount() = 0;

	// Get tab view from point (screen coords.), return tab view id if point is in a tab view or file view else eAnyTabVw.
	virtual ETABVIEWID GetTabVwFromPoint(POINT& pt) = 0;

	virtual CXeFileVwIF* GetViewFromDataSourceId(dsid_t dwDataSourceId) = 0;
	virtual const CXeFileVwIF* GetViewFromDataSourceIdConst(dsid_t dwDataSourceId) const = 0;

	virtual std::vector<CXeFileVwIF*> GetAllViews() = 0;
	virtual std::vector<CXeFileVwIF*> GetAllForegroundViews() = 0;
	virtual std::vector<CXeFileVwIF*> GetAllNonForegroundViews() = 0;
	virtual std::vector<CXeFileVwIF*> GetAllViewsFromDataSourceIds(const std::vector<dsid_t>& dataSourceIds) = 0;

	virtual void SendMessageToAllViews(UINT uMessage, WPARAM wParam, LPARAM lParam) = 0;

	virtual CRect GetTabViewWindow(ETABVIEWID tabVwId) = 0;

	virtual void ViewWithFocusChanged(dsid_t dwDataSourceId) = 0;

	virtual void SetFocusToView(dsid_t dwDataSourceId) = 0;

	// Is foreground window (current) in any tab view?
	virtual bool IsViewVisible(dsid_t dwDataSourceId) = 0;

	// Is view with focus or last view with focus (when LVS is not the app with focus)?
	virtual bool IsLastViewWithFocus(dsid_t dwDataSourceId) = 0;

	// Tab view calls here to notify of tab order change (and possible tab view height change).
	virtual void OnTabOrderChanged() = 0;

	virtual bool OnViewGotFocus(dsid_t dwDataSourceId) = 0;

	virtual void OnViewLostFocus(dsid_t dwDataSourceId) = 0;

	virtual void ProcessCommand(UINT uCmdId, UINT param) = 0;

	virtual void OnTabCtxMenuCmd(UINT uCmdId, ETABVIEWID tabId, dsid_t datasourceId) = 0;

	virtual CXeCancelEvent* DoWorkThreadWork(WorkThreadWorkCallbackFunc) = 0;
	virtual CXeCancelEvent* GetWorkThreadWorkCancelEvent() = 0;

	virtual bool CanOpenContainingFolder(dsid_t dsId) = 0;
	virtual void OpenContainingFolder(dsid_t dsId) = 0;

	virtual void CopyInfoToClipboard(ECLIPBRDOP operation, dsid_t dsId) = 0;

	virtual int ShowMessageBox(HWND hParent, const wchar_t* pMsg, const wchar_t* pTitle = nullptr,
		UINT uType = MB_OK, CRect* pRectCell = nullptr) = 0;

	virtual std::optional<std::wstring> AskStringDlg(HWND hParent, const std::wstring& title,
		const std::wstring& string, CRect* pRectCell = nullptr) = 0;

	virtual void OnNotifyChanges(WPARAM wParam, LPARAM lParam) = 0;

	virtual void OnUIcreateComplete() = 0;

	virtual void OnDroppedFiles(CPoint ptScreenCoords, const std::vector<std::wstring>& dropped_files) = 0;

	virtual void OpenFileFromMRUlist(size_t idx) = 0;

	virtual void ProcessCommandLine(const std::wstring& cmd_line) = 0;
};

