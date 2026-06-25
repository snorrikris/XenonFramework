module;

#include "os_minimal.h"
#include <vector>

export module Xe.MainFrameIF;

export import Xe.TabsViewIF;
import Xe.mfc_types;
import Xe.D2DToolbarIF;

export class CXeMainFrameIF
{
public:
	virtual HWND GetMainFrameHandle() const = 0;

	virtual CXeD2DToolbarIF* GetToolbarIF() const = 0;

	virtual void RecalculateAndRepositionWindows() = 0;

	virtual void RecalculateWindowsRects() = 0;

	virtual CRect GetViewWindowRect(int view_id) const = 0;

	virtual CXeTabsViewIF* GetTabView(ETABVIEWID eTabVwId) const = 0;
	virtual std::vector<CXeTabsViewIF*> GetAllTabViews() const = 0;
};