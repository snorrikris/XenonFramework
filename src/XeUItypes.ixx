module;

#include "os_minimal.h"
#include <cstdint>
#include <string>
#include <functional>
#include "XeAssert.h"
#include "ColorIDs.h"	// CID enum

export module Xe.UItypes;

export import Xe.UItypesPID;
//import Xe.LogDefs;

export CID;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class EXE_FONT
{
	eUI_Font,
	eUI_FontBold,
	eMonospacedFont,
	eMediumBig,
	eTabListTitleFont,
	eBigMsgFont,
	eLastFont
};

// Callback used to filter keyboard messages for Scintilla edit control.
export typedef std::function<bool(const MSG& msg)> FilterKeyboardMessageCallbackFunc;

//export PID GetProgressStateImgId(EPROGRESSSTATE progressState)
//{
//	switch (progressState)
//	{
//	case EPROGRESSSTATE::eRequested:                return PID::eRequested;
//	case EPROGRESSSTATE::eQueued:                   return PID::eQueued;
//	case EPROGRESSSTATE::eOpening:                  return PID::eOpening;
//	case EPROGRESSSTATE::eUnzipping:                return PID::eUnzipping;
//	case EPROGRESSSTATE::eParsing:                  return PID::eParsing;
//	case EPROGRESSSTATE::eMerging:                  return PID::eMerging;
//	case EPROGRESSSTATE::eIndexing:                 return PID::eIndexing;
//	case EPROGRESSSTATE::eDone:                     return PID::eDone;
//	case EPROGRESSSTATE::eDoneParseFailed:          return PID::eDone;
//	case EPROGRESSSTATE::eDoneAlreadyOpen:          return PID::eDoneAlreadyOpen;
//	case EPROGRESSSTATE::eErrorFileNotFound:        return PID::eErrorFileNotFound;
//	case EPROGRESSSTATE::eErrorNetworkPathInvalid:  return PID::eErrorNetworkPathInvalid;
//	case EPROGRESSSTATE::eErrorAccessDenied:        return PID::eErrorAccessDenied;
//	case EPROGRESSSTATE::eErrorOther:               return PID::eErrorOther;
//	case EPROGRESSSTATE::eErrorUnsupportedFile:     return PID::eErrorUnsupportedFile;
//	case EPROGRESSSTATE::eCancel:                   return PID::Cancelled;
//	case EPROGRESSSTATE::eErrorOutOfMemory:         return PID::eErrorOutOfMemory;
//	case EPROGRESSSTATE::eRm_Connecting:            return PID::eRm_Connecting;
//	case EPROGRESSSTATE::eRm_Reading:               return PID::eRm_Reading;
//	case EPROGRESSSTATE::eErrorRmConnect:           return PID::eErrorRmConnect;
//	case EPROGRESSSTATE::eErrorRmAuthenticate:      return PID::eErrorAccessDenied;
//	case EPROGRESSSTATE::eErrorRmReadFailed:        return PID::eErrorRmReadFailed;
//	case EPROGRESSSTATE::eErrorRmWriteLocalFailed:  return PID::eErrorRmWriteLocalFailed;
//	}
//	return (PID)0;
//}

// Custom XE controls notify message.
export constexpr UINT XE_CONTROL_NOTIFY_MSG = WM_USER + 200;
// wParam = LOWORD(Control ID), HIWORD specifies the notification code.

export constexpr WORD XE_CTRL_NOTIFY_FIRST = 0xA000;

// When listbox (combobox) has checkboxes, XE_CONTROL_NOTIFY message is sent
// to the parent when the user has changed the checkmark state of an item
// with notify code LB_EX_NOTIFY_ITEM_CHECKED.
// lParam = index of item that has changed.
export constexpr WORD LB_EX_NOTIFY_ITEM_CHECKED = XE_CTRL_NOTIFY_FIRST + 1;

// When listbox is a menu the notify parent callback is called to notify that a
// sub-menu needs to be displayed.
// lParam = index of item that has the sub-menu flag.
export constexpr WORD LB_EX_NOTIFY_SUBMENU_SELECTED = XE_CTRL_NOTIFY_FIRST + 2;
export constexpr WORD LB_EX_NOTIFY_SUBMENU_SELECTED_BY_KEYBOARD = XE_CTRL_NOTIFY_FIRST + 3;

// List box item background pattern.
export enum class LBitemBgPattern : uint8_t
{
	None = 0,
	GrayXhatch1,	// Short pitch gray cross hatch pattern
	GrayXhatch2,	// Long pitch gray cross hatch pattern
	RedXhatch1,		// Short pitch red cross hatch pattern
	RedXhatch2,		// Long pitch red cross hatch pattern
};

export struct ListBoxItemData
{
	union {
		int64_t m_i64 = 0;
		struct {	// first line = bit0
			uint64_t	m_rgbColor			: 24;	// Text FG color
			uint64_t	m_hasColor			: 1;	// has Text FG color
			uint64_t	m_hasIcon			: 1;	// has icon (m_iconPID is valid)
			uint64_t	m_isSectionStart	: 1;	// Item is section start marker
			uint64_t	m_sectionNumber		: 5;	// Section number the item belongs to.
			uint64_t	m_iconPID			: 16;	// Icon PID
			uint64_t	m_isCheckbox		: 1;	// Item is checkbox
			uint64_t	m_isChecked			: 1;	// Item is checkbox and is checked
			uint64_t	m_isSubMenu			: 1;	// Item has sub-menu indicator (in menu)
			uint64_t	m_isItemDisabled	: 1;	// Item is disabled (in menu)
			uint64_t	m_isMenuSeparator	: 1;	// Item is menu separator (in menu)
			uint64_t	m_isColorIdColor	: 1;	// m_rgbColor is color ID value (CID)
			uint64_t	m_isColorSquare		: 1;	// Use color as a square color icon (used in theme editor)
			uint64_t	m_isChanged			: 1;	// Indicate item has changed with '*'.
			uint64_t	m_bg_pattern		: 4;	// Item background pattern.
			uint64_t	m_isSelected		: 1;	// Item is selected (in LB with LBS_MULTIPLESEL style).
			uint64_t	m_unused			: 3;
		};
	};
	ListBoxItemData() = default;
	ListBoxItemData(PID pid)
	{
		SetIcon(pid);
	}
	void SetColor(/*COLORREF*/uint32_t rgbColor)
	{
		m_rgbColor = rgbColor;
		m_hasColor = 1;
	}
	void SetColorID(CID colorId)
	{
		m_rgbColor = (uint64_t)colorId;
		m_hasColor = 1;
		m_isColorIdColor = 1;
	}
	CID GetColorID() const
	{
		XeASSERT(m_hasColor == 1 && m_isColorIdColor == 1);
		return (CID)m_rgbColor;
	}
	void SetIcon(PID pid)
	{
		unsigned int id = (unsigned int)pid;
		XeASSERT(id <= 65535);
		m_iconPID = id;
		m_hasIcon = 1;
	}
	PID GetIconPID() const
	{
		return (PID)m_iconPID;
	}
	void SetBgPattern(LBitemBgPattern pat)
	{
		uint64_t u = (uint64_t)pat;
		XeASSERT(u <= 15);
		m_bg_pattern = u;
	}
	LBitemBgPattern GetBgPattern() const
	{
		return (LBitemBgPattern)m_bg_pattern;
	}
};
static_assert(sizeof(ListBoxItemData) == 8, "ListBoxItemData size incorrect");

export enum class IsEnabled { yes, no };
export enum class IsChecked { yes, no };
export enum class IsSeparator { yes, no };
export enum class IsSubMenu { yes, no };

export struct ListBoxExItem
{
	std::wstring m_string;
	ListBoxItemData m_item_data;
	uint64_t m_extra_data = 0;

	ListBoxExItem() = default;
	ListBoxExItem(const wchar_t* pStr) : m_string(pStr) {}
	ListBoxExItem(const std::wstring& str) : m_string(str) {}

	// ctor used when listbox is a menu.
	ListBoxExItem(UINT uCmdID, const std::wstring& str,
		IsSeparator isSeparator = IsSeparator::no,	// Separator line drawn above item
		IsChecked isChecked = IsChecked::no,
		IsEnabled isEnabled = IsEnabled::yes,
		IsSubMenu isSubMenu = IsSubMenu::no,
		PID imageId = PID::None, uint32_t color = 0xFFFFFFFF,
		LBitemBgPattern pattern = LBitemBgPattern::None)
		: m_string(str), m_extra_data(uCmdID)
	{
		m_item_data.m_isChecked = isChecked == IsChecked::yes ? 1 : 0;
		m_item_data.m_isCheckbox = 1;		// Note - ignored when 'this' is a menu item.
		m_item_data.m_isItemDisabled = isEnabled == IsEnabled::yes ? 0 : 1;
		m_item_data.m_isMenuSeparator = isSeparator == IsSeparator::yes ? 1 : 0;
		m_item_data.m_isSubMenu = isSubMenu == IsSubMenu::yes ? 1 : 0;
		if (imageId != PID::None)
		{
			m_item_data.SetIcon(imageId);
		}
		if (color != 0xFFFFFFFF)
		{
			m_item_data.SetColor(color);
		}
		m_item_data.SetBgPattern(pattern);
	}

	void EnableItem(bool isEnable) { m_item_data.m_isItemDisabled = !isEnable; }
	void SetCheckMark(bool isCheck) { m_item_data.m_isChecked = isCheck; }

	void SetItemSelectedState(bool isSelected) { m_item_data.m_isSelected = isSelected; }
	bool IsItemSelected() const { return m_item_data.m_isSelected; }

	static ListBoxExItem MakeCheckBox(const std::wstring& str, bool isChecked)
	{
		ListBoxExItem item(str);
		item.m_item_data.m_isCheckbox = 1;
		item.m_item_data.m_isChecked = isChecked;
		return item;
	}

	bool operator<(const ListBoxExItem& rhs) const
	{
		return std::lexicographical_compare(
			std::begin(m_string), std::end(m_string),
			std::begin(rhs.m_string), std::end(rhs.m_string),
			[](const wchar_t& char1, const wchar_t& char2) {
				return std::tolower(char1) < std::tolower(char2);
			}
		);
	}
};

// Size of char for mono-spaced UI font.
export struct MonospacedCharSize
{
	//int cySingleLineRowHeight = 0;			// Height of a single-row cell in grid.
	int cyMultiLineTextHeight = 0;			// Height of 1 line in multi-line cell in grid.
	//int cyMultiLineTextTopMargin = 0;		// Top margin for multi-line cell text in grid.
	//int cyMultiLineTextBottomMargin = 0;	// Bottom margin for multi-line cell text in grid.
	int cxGridCharWidth = 0;				// Width of a single letter in cell grid (note is mono-spaced font).

	bool operator==(const MonospacedCharSize& rhs) const noexcept
	{
		return //cySingleLineRowHeight == rhs.cySingleLineRowHeight
			/*&&*/ cyMultiLineTextHeight == rhs.cyMultiLineTextHeight
			&& cxGridCharWidth == rhs.cxGridCharWidth;
	}

	bool operator!=(const MonospacedCharSize& rhs) const noexcept
	{
		return !operator==(rhs);
	}
};

export struct XeStaticString
{
	static const wchar_t* FilterDeActivateTooltip(bool isGetActivateString)
	{
		return isGetActivateString
			? L"Activate filter"
			: L"(temporarily) Deactivate filter";
	}
};

// Menu ID's - Note: same ID's as menu from user32.dll.
export constexpr UINT XSB_IDM_SCRHERE			=4100;	// Scroll Here		Scroll Here
export constexpr UINT XSB_IDM_SCR_TL			=4102;	// Top				Left Edge
export constexpr UINT XSB_IDM_SCR_BR			=4103;	// Bottom			Right Edge
export constexpr UINT XSB_IDM_SCR_PG_UL			=4098;	// Page Up			Page Left
export constexpr UINT XSB_IDM_SCR_PG_DR			=4099;	// Page Down		Page Right
export constexpr UINT XSB_IDM_SCR_UL			=4096;	// Scroll Up		Scroll Left
export constexpr UINT XSB_IDM_SCR_DR			=4097;	// Scroll Down		Scroll Right

// DlgCtrlId of windows in the split window.
export constexpr int VW_ID_BASE   = 0xE900;	// Same as AFX_IDW_PANE_FIRST
export constexpr int VW_ID_HVIEW  = VW_ID_BASE + (2 * 16) + 0;
export constexpr int VW_ID_TABS_0 = VW_ID_BASE + (0 * 16) + 1;
export constexpr int VW_ID_TABS_1 = VW_ID_BASE + (0 * 16) + 2;
export constexpr int VW_ID_VIEW_0 = VW_ID_BASE + (1 * 16) + 1;
export constexpr int VW_ID_VIEW_1 = VW_ID_BASE + (1 * 16) + 2;
export constexpr int VW_ID_SIDE   = VW_ID_BASE + (0 * 16) + 0;	// "row" 0, "col" 0

export enum class XeViewProp
{
	WIDTH = 1,		// Get current width of view (only needed for side view).
	RECALC_CY = 2,	// Recalculate view height, cx available as param (needed for tabs vw).
	TOTAL_CY = 3,	// Total view height (needed for HVIEW/timeline vw).
	MIN_CY = 4,		// Minimum view height (needed for HVIEW/timeline vw).
	MAX_CY = 5,		// Maximum view height (needed for HVIEW/timeline vw).
	MIN_CX = 6,		// Minimum view width (needed for VIEW_0/1 vw).
};
//export constexpr int VW_PROP_WIDTH = 1;		// Get current width of view (only needed for side view).
//export constexpr int VW_PROP_RECALC_CY = 2;	// Recalculate view height, cx available as param (needed for tabs vw).
//export constexpr int VW_PROP_TOTAL_CY = 3;	// Total view height (needed for HVIEW/timeline vw).
//export constexpr int VW_PROP_MIN_CY = 4;	// Minimum view height (needed for HVIEW/timeline vw).
//export constexpr int VW_PROP_MAX_CY = 5;	// Maximum view height (needed for HVIEW/timeline vw).
//export constexpr int VW_PROP_MIN_CX = 6;	// Minimum view width (needed for VIEW_0/1 vw).
