module;

#include "os_minimal.h"
#include <string>

export module Xe.TooltipIF;

import Xe.mfc_types;

//The behaviours
export constexpr UINT PPTOOLTIP_NOCLOSE_OVER	=	0x00000008; //No close tooltip if mouse over him

export struct PPTOOLTIP_INFO
{
	PPTOOLTIP_INFO() = default;
	PPTOOLTIP_INFO(HWND hWnd_ContainingTool, const std::wstring& tooltip) : hTWnd(), sTooltip(tooltip) {}

	CRect			rectBounds;				// Bounding rect for toolinfo to be displayed
	std::wstring	sTooltip;				// The string of the tooltip
	UINT			nBehaviour = PPTOOLTIP_NOCLOSE_OVER /*| PPTOOLTIP_CLOSE_LEAVEWND | PPTOOLTIP_MULTIPLE_SHOW*/;		// The tooltip's behaviour
	HWND			hTWnd = NULL;			// +++hd
	CPoint			ptTipOffset;			// +++sk offset coords. for tool tip (from left,bottom).
};

// This structure sent to PPTooltip parent in a WM_NOTIFY - UDM_TOOLTIP_NEED_TT message
// when no tooltip found at mouse coords.
// Parent can make a tooltip by filling in the ti structure and returning 1 to the message.
// Note - it's important to set the bounds rect so tooltip is hidden when mouse has left
// the tooltip 'control'. See -> CXeToolBar::OnPPTTNF_NeedTT for example.
export struct NM_PPTOOLTIP_NEED_TT
{
	NMHDR hdr;
	LPPOINT pt;				// Mouse coords.
	PPTOOLTIP_INFO* ti;
};

// UDM_TOOLTIP_NEED_TT is sent as WM_NOTIFY message to parent when no tooltip found.
// lParam = pointer to NM_PPTOOLTIP_NEED_TT structure.
export constexpr UINT UDM_TOOLTIP_NEED_TT = (WM_USER + 104);

export class CXeTooltipIF
{
public:
	virtual BOOL RelayMessageToTooltip(UINT message, CPoint pt_msg) = 0;

	virtual void HideTooltip() = 0;

	virtual bool IsMouseOverTooltip() = 0;
};