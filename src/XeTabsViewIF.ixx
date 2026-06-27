module;
#include "os_minimal.h"
#include <vector>

export module Xe.TabsViewIF;

export import Xe.FileVwIF;
export import Xe.DefData;
import Xe.UItypes;

export enum class ViewNavigate { Prev, Next };

export class CXeTabsViewIF
{
public:
	virtual HWND GetTabWindowHandle() const = 0;

	virtual void RedrawTabView() = 0;

	virtual void ViewWithFocusChanged(dsid_t dwDataSourceId) = 0;

	virtual void OnViewRenamed(CXeFileVwIF* pView) = 0;

	virtual void SendMessageToAllViews(UINT uMessage, WPARAM wParam, LPARAM lParam) = 0;

	virtual void SetTabProgressIndication(dsid_t dwDataSourceId, UINT uProgress) = 0;
	virtual void ClearProgressOnAllTabs() = 0;

	virtual ETABVIEWID GetTabVwId() const = 0;

	// Return true if point (in screen coords.) is in a tab view or file view else false.
	virtual bool IsPointInThisView(POINT& pt) const = 0;

	virtual UINT GetIndexOfCurrentTab() const = 0;

	virtual int GetTabCount() const = 0;

	virtual std::vector<CVwInfo> GetAllTabs() const = 0;

	virtual std::vector<CXeFileVwIF*> GetAllViews() const = 0;

	virtual const CVwInfo* FindView(CXeFileVwIF* pView) const = 0;

	virtual CXeFileVwIF* FindView(dsid_t dwDataSourceId) const = 0;

	virtual CXeFileVwIF* GetCurrentView() const = 0;

	virtual dsid_t GetDataSourceIdOfCurrentView() const = 0;

	virtual bool AttachView(CXeFileVwIF* pView, CreateViewParams viewParams, bool setFocusToView) = 0;

	virtual void CloseAndDeleteTabs(const std::vector<CVwInfo>& tabsList) = 0;

	virtual void DeleteAllTabsAndViews() = 0;

	virtual bool FindAndDeleteTabAndView(dsid_t dwDataSourceId) = 0;

	virtual bool SwitchToView(CXeFileVwIF* pView, bool setFocusToView) = 0;

	virtual bool SwitchToView(dsid_t dwDataSourceId, bool setFocusToView) = 0;

	virtual bool SwitchToViewAtIndex(UINT uIndex) = 0;

	virtual bool SwitchToView(ViewNavigate navigate) = 0;

	virtual void UpdateMenuItem(ListBoxExItem& menu_item, CXeFileVwIF* pView) const = 0;
};

