module;

#include "os_minimal.h"

export module Xe.PopupCtrlBase;

import Xe.mfc_types;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Popup window offset preference
// - needed by CXePopupCtrl when it needs to place the drop window above the parent control.
export enum class DropWndOffset
{
	AboveParentCtrl,	// drop window should be placed fully above the parent control window.
	OnParentCtrl		// drop window should be placed on the parent control window bottom edge.
};

// Special message that the popup window control can send - to notify the popup window size needs to change.
export constexpr UINT NF_POPUP_WND_SIZE_CHANGE = WM_USER + 2345;

// Base class for any kind of popup window control - such as listbox (combobox), menu, color picker etc.
export class CXePopupCtrlBase
{
public:
	virtual HWND GetHwnd() const = 0;
	virtual HWND GetVscrollHwnd() const = 0;
	virtual HWND GetHscrollHwnd() const = 0;

	// This member is called after the popup control sent the NF_POPUP_WND_SIZE_CHANGE message.
	virtual CSize GetWindowPreferredSize() = 0;

	virtual void AdjustPopupWindowFinalSize(
		CRect& rcScreenPos, const CRect& rcParentCtrlScreenPos) = 0;

	// Called by CXePopupCtrl when it needs to place the drop window above the parent control.
	// Derived class should return AboveParentCtrl if the drop window should be placed fully above
	// the parent control window.
	// Derived class should return OnParentCtrl if the drop window should be placed on the parent
	// control window bottom edge (as is the case for menu).
	virtual DropWndOffset GetDropWndOffsetPreference() const = 0;

	// Called by XePopupParent when processing the NC_CREATE message.
	virtual void PopupDropDown(HWND hWnd, DWORD dwStyle, DWORD dwExStyle) = 0;

	// Return true if 'this' is the first instance of the 'control'.
	// The derived control should return true - unless 'this' instance is a 'sub-control' window (e.g. sub-menu).
	virtual bool IsFirstInstance() const = 0;

	// Return true when user has made any kind of choice in the control that should close the popup window.
	virtual bool IsPopupEndCondition() = 0;

	// Called by XePopupParent before popup window created (to tell the control that it's in a popup window).
	virtual void PopupOpening() = 0;

	// Called by XePopupParent after popup window destroyed.
	virtual void PopupClosed() = 0;

	virtual LRESULT ProcessPopupWndMessage(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam) = 0;
};