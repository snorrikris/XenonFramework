module;

#include "os_minimal.h"

export module Xe.MainFrameIF;

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
};