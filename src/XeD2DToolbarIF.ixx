module;

#include "os_minimal.h"
#include <string>
#include <optional>

export module Xe.D2DToolbarIF;

import Xe.Menu;
import Xe.UserSettings_Changed;
export import Xe.mfc_types;

export class CXeD2DToolbarIF
{
public:
	virtual void SetUpdateMenuCallback(UpdateMenuCallbackFunc updateMenuCallback) = 0;
	virtual void OnChangedSettings(const ChangedSettings& chg_settings) = 0;
	virtual void SetItemEnabledState(UINT uID, bool fEnabled) = 0;
	virtual void SetItemVisibleState(UINT uID, bool visible) = 0;
	virtual void SetItemFlags(UINT uID, uint8_t flags) = 0;
	virtual uint8_t GetItemFlags(UINT uID) = 0;
	virtual CRect GetItemRect(UINT uID) = 0;
	virtual std::optional<std::wstring> GetDropListButtonCurSel(UINT uID) = 0;
	virtual HWND GetItemHwnd(UINT uID) = 0;
	virtual HWND GetToolbarHwnd() = 0;
};

