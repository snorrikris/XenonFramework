module;

#include "os_minimal.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <map>
#include <d2d1.h>
#include <dwrite.h>
#include "XeResource.h"

export module Xe.ListBoxExCommon;

import Xe.PopupCtrlBase;
import Xe.UIcolorsIF;
import Xe.D2DWndBase;
import Xe.ScrollBar;
import Xe.Helpers;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class LBType { Normal, Combo, DropDown, Menu };

struct ListBoxCheckBoxDimensions
{
	float cxMarginBox = 2.0f;
	float cxyCB = 0.0f;
	float cySpc = 0.0f;
	float cyM = 0.0f;
	float cxM = 0.0f;
	float cxMargin = 0.0f;

	void Calculate(float itemHeight)
	{
		cxMarginBox = 2.0f;
		cxyCB = itemHeight * 0.70f;
		cySpc = itemHeight - cxyCB;
		//if (cySpc < 2 || cySpc & 1) { --cxyCB; }

		// checkmark dimensions (inside the box)
		cyM = cxyCB * 0.55f;
		cxM = cxyCB * 0.45f;
		cxMargin = ((cxyCB - cxM) / 2.0f);
	}

	D2D1_RECT_F GetBoxRect(const D2D1_RECT_F& rcItem)
	{
		float x = rcItem.left + cxMarginBox;
		float y = rcItem.top + ((HeightOf(rcItem) - cxyCB) / 2.0f);
		return D2D1_RECT_F(x, y, x + cxyCB, y + cxyCB);
	}

	float GetCheckBoxCX() const { return cxMarginBox + cxyCB + cxMarginBox; }
};

struct ListBoxBottomButtons
{
	bool m_hasButtons = false;
	int m_yButtonsArea = 0;
	int m_cyButtonsArea = 0;
	int m_cyButtonsMargin = 0;
	int m_cyButtons = 0;
	int m_cxButtons = 0;
	int m_cxButtonsMaxMargin = 0;
	int m_cxMinForButtons = 0;
	CRect m_rcOkBtn, m_rcCancelBtn, m_rcBtnArea;
	bool m_isOkCancelButtons = true;
	std::wstring m_txtButton;	// Text of button (when custom text button used).

	int GetButtonsCY() const { return m_hasButtons ? m_cyButtonsArea : 0; }

	int GetMinCXforButtons() const { return m_hasButtons ? m_cxMinForButtons : 0; }

	bool HasOkCancelButtons() const { return m_hasButtons && m_isOkCancelButtons; }
	bool HasCustomButton() const { return m_hasButtons && !m_isOkCancelButtons; }
	
	void CalculateSize(CXeUIcolorsIF* xeUI)
	{
		const XeFontMetrics& tm = xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		m_cyButtonsArea = tm.GetHeight() * 2;
		m_cyButtons = (int)((double)tm.GetHeight() * 1.5 + 0.5);
		m_cyButtonsMargin = (m_cyButtonsArea - m_cyButtons) / 2;
		CSize sTxt = xeUI->GetTextSizeW(EXE_FONT::eUI_Font, (m_isOkCancelButtons ? L"Cancel" : m_txtButton.c_str()));
		m_cxButtons = sTxt.cx + (tm.m_aveCharWidth * 2);
		m_cxButtonsMaxMargin = tm.m_aveCharWidth * 3;
		m_cxMinForButtons = m_isOkCancelButtons ? m_cxButtons * 2 + 6 : m_cxButtons + 6;
		m_hasButtons = true;
	}

	void PlaceButtons(const CRect rcClient)
	{
		m_yButtonsArea = (rcClient.bottom - m_cyButtonsArea) + m_cyButtonsMargin;
		m_rcBtnArea.SetRect(rcClient.left, m_yButtonsArea, rcClient.right, rcClient.bottom);
		m_rcOkBtn.top = m_rcCancelBtn.top = m_yButtonsArea;
		m_rcOkBtn.bottom = m_rcCancelBtn.bottom = m_yButtonsArea + m_cyButtons;
		if (m_isOkCancelButtons)
		{
			m_rcOkBtn.left = rcClient.left;
			m_rcOkBtn.right = m_rcOkBtn.left + m_cxButtons;
			m_rcCancelBtn.left = m_rcOkBtn.right;
			m_rcCancelBtn.right = m_rcCancelBtn.left + m_cxButtons;
			if ((2 * m_cxButtons) < rcClient.Width())
			{
				int xOfs = (rcClient.Width() - (2 * m_cxButtons)) / 3;
				if (xOfs > m_cxButtonsMaxMargin)
				{
					xOfs = m_cxButtonsMaxMargin;
				}
				m_rcOkBtn.OffsetRect(xOfs, 0);
				m_rcCancelBtn.OffsetRect(xOfs * 2, 0);
			}
		}
		else
		{
			m_rcOkBtn.left = rcClient.left;
			m_rcOkBtn.right = m_rcOkBtn.left + m_cxButtons;
			m_rcCancelBtn.left = m_rcOkBtn.right;
			m_rcCancelBtn.right = m_rcCancelBtn.left;
			if (m_cxButtons < rcClient.Width())
			{
				int xOfs = (rcClient.Width() - m_cxButtons) / 2;
				if (xOfs > m_cxButtonsMaxMargin)
				{
					xOfs = m_cxButtonsMaxMargin;
				}
				m_rcOkBtn.OffsetRect(xOfs, 0);
			}
		}
	}
};

constexpr UINT c_VSB_ID = 1001;
constexpr UINT c_HSB_ID = 1002;

constexpr UINT TIMER_HOVER_ID = 1010;

constexpr UINT DELAY_PAINT_TIMER = 1020;

export typedef std::function<void(WORD nfCode, int item_idx)> NotifyComboboxCallbackFunc;

export struct TopLevelMenuNavigationParameters
{
	bool	m_isNavigationByMouse = true;
	CPoint	m_ptMouseScr;
	UINT	m_vk_key_code;
	TopLevelMenuNavigationParameters() = default;
	TopLevelMenuNavigationParameters(const CPoint& ptMouseScr)
		: m_isNavigationByMouse(true), m_ptMouseScr(ptMouseScr), m_vk_key_code(0) {}
	TopLevelMenuNavigationParameters(UINT vk_key_code)
		: m_isNavigationByMouse(false), m_vk_key_code(vk_key_code) {}
};
export typedef std::function<int(const TopLevelMenuNavigationParameters& params)> TopLevelMenuNavigationCallbackFunc;

export class CXeListBoxExCommon : public CXeD2DWndBase, public CXePopupCtrlBase
{
#pragma region Class_data
protected:
	bool m_isPopupCtrl = false;

	bool m_hasFocus = false, m_isEnabled = true;

	std::unique_ptr<CXeScrollBar> m_vScollBar;
	std::unique_ptr<CXeScrollBar> m_hScollBar;
	bool m_hasVscroll = false, m_hasHscroll = false;
	int m_cxSysSbV = 10, m_cySysSbH = 10;
	CRect m_rcHSB, m_rcVSB;

	std::vector<ListBoxExItem> m_items;

	size_t m_curIdx = 0xFFFFFFFF, m_topIdx = 0, m_curIdxOriginal = 0xFFFFFFFF;
	size_t m_mouseOverItemIdx = 0xFFFFFFFF;
	CPoint m_ptMouseHover;
	CPoint m_ptOfLbtnDown;

	float m_cxMaxStringWidth = 0.0f;
	float m_cyItem = 15.0f;
	float m_x_start = 2.0f, m_y_start = 0.0f;
	float m_cxMenuRmargin = 10.0f;
	bool m_hasIcons = false;
	float m_cxIcon = 0;
	bool m_HasColorSquares = false;

	ListBoxCheckBoxDimensions m_cbDim;
	bool m_isCheckBoxesMode = false;
	bool m_isSetCurItemEnabled = true;
	ListBoxBottomButtons m_buttonsHlpr;
	bool m_isOk_or_custom_button_clicked = false;

	NotifyComboboxCallbackFunc m_notifyComboBoxCallback = nullptr;
	bool m_isEnsureVisibleNeeded = false;

public:
	LBType m_type = LBType::Normal;

	CXeListBoxExCommon* m_pParentMenu = nullptr;
	CRect m_rcParentMenuItemScreenPos;

	bool m_isDestroyed = false;	// Set true when WM_DESTROY processed.

	// When 'this' is a listbox in a combo box and the user has seleced an item
	// and popup should be dismissed.
	bool m_isSelEndOk = false, m_isSelEndCancel = false;
	bool m_isSelEndCancelWithESCkey = false;
	bool m_isCloseThisMenu = false;

	// Menu shortcut keys. When user holds down the Alt key + another key to select a menu item.
	// key is the VK_KEY code, value is the command ID.
	// Note - if the command ID is a popup (submenu) ID - the popup menu is shown.
	std::map<WPARAM, UINT> m_menu_shortcut_keys;

	TopLevelMenuNavigationCallbackFunc m_topLevelMenuNavigationCallback = nullptr;
	int m_nTopLevelMenuNavigationItemIndex = -1;

	int m_suppressMouseMoveMessages = 1;

	UINT m_uSelectedItemCommandID = 0;

	std::vector<uint32_t> m_tab_stops;
	float m_cxDefaultTab = 100;
#pragma endregion Class_data

#pragma region Create
	CXeListBoxExCommon(CXeUIcolorsIF* pUIcolors) : CXeD2DWndBase(pUIcolors)
	{
		Initialize(LBType::Normal);
	}
	virtual ~CXeListBoxExCommon() {}

	void PreCreateWindow()
	{
		if (m_isDestroyed)
		{
			m_isDestroyed = false;
			if (m_vScollBar)
			{
				XeASSERT(m_vScollBar->Hwnd() == 0);
				m_vScollBar.reset();
			}
			if (m_hScollBar)
			{
				XeASSERT(m_hScollBar->Hwnd() == 0);
				m_hScollBar.reset();
			}
			m_hasVscroll = m_hasHscroll = false;
		}
	}

	void Initialize(LBType type, NotifyComboboxCallbackFunc notifyComboBoxCallback = nullptr)
	{
		m_type = type;
		m_notifyComboBoxCallback = notifyComboBoxCallback;
		_InitializeLBitemUIsize();
	}

protected:
	void _InitializeLBitemUIsize()
	{
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		float cyText = (float)tm.GetHeight();
		if (cyText == 0.0f)
		{
			cyText = 15.0f;
		}
		m_cxSysSbV = ::GetSystemMetrics(SM_CXVSCROLL);
		m_cySysSbH = ::GetSystemMetrics(SM_CYHSCROLL);
		m_cbDim.Calculate(cyText);
		m_cyItem = m_type == LBType::Menu ? cyText * 1.5f : cyText;
	}

	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		if (uMsg >= LB_ADDSTRING && uMsg <= LB_MSGMAX)
		{
			return _ProcessListBoxMessage(hWnd, uMsg, wParam, lParam);
		}
		return CXeD2DWndBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	// WM_TIMER
	virtual LRESULT _OnTimer(WPARAM wParam, LPARAM lParam)
	{
		UINT_PTR nIDEvent = wParam;
		if (nIDEvent == DELAY_PAINT_TIMER)
		{
			_OnDelayPaintTimer();
			::KillTimer(Hwnd(), DELAY_PAINT_TIMER);
		}
		else if (nIDEvent == TIMER_HOVER_ID)
		{
			_OnHoverTimer();
			::KillTimer(Hwnd(), TIMER_HOVER_ID);
		}
		return CXeD2DWndBase::_OnTimer(wParam, lParam);
	}

	LRESULT _ProcessListBoxMessage(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uiMsg)
		{
		case LB_RESETCONTENT:			return OnResetContentMsg(wParam, lParam);
		case LB_GETCOUNT:				return OnGetCountMsg(wParam, lParam);
		case LB_GETCURSEL:				return OnGetCurSelMsg(wParam, lParam);
		case LB_SETCURSEL:				return OnSetCurSelMsg(wParam, lParam);
		case LB_GETTOPINDEX:			return OnGetTopIdxMsg(wParam, lParam);
		case LB_ADDSTRING:				return OnAddStringMsg(wParam, lParam);
		case LB_SETITEMDATA:			return OnSetItemDataMsg(wParam, lParam);
		case LB_GETITEMDATA:			return OnGetItemDataMsg(wParam, lParam);
		case LB_GETSEL:					return OnGetSelMsg(wParam, lParam);
		case LB_GETSELCOUNT:			return OnGetSelCountMsg(wParam, lParam);
		case LB_GETTEXT:				return OnGetTextMsg(wParam, lParam);
		case LB_GETTEXTLEN:				return OnGetTextLenMsg(wParam, lParam);
		case LB_DELETESTRING:			return OnDeleteStringMsg(wParam, lParam);
		case LB_INSERTSTRING:			return OnInsertStringMsg(wParam, lParam);
		case LB_ITEMFROMPOINT:			return OnItemFromPointMsg(wParam, lParam);
		case LB_SELECTSTRING:			return OnSelectStringMsg(wParam, lParam);
		case LB_SETSEL:					return OnSetSelMsg(wParam, lParam);
		case LB_SETTOPINDEX:			return OnSetTopIndexMsg(wParam, lParam);
		case LB_FINDSTRING:				return OnFindStringMsg(wParam, lParam);
		case LB_FINDSTRINGEXACT:		return OnFindStringExactMsg(wParam, lParam);
		case LB_ADDFILE:				return OnNotSupportedMsg(wParam, lParam);
		case LB_DIR:					return OnNotSupportedMsg(wParam, lParam);
		case LB_GETANCHORINDEX:			return OnNotSupportedMsg(wParam, lParam);
		case LB_GETCARETINDEX:			return OnNotSupportedMsg(wParam, lParam);
		case LB_GETHORIZONTALEXTENT:	return OnNotSupportedMsg(wParam, lParam);
		case LB_GETITEMHEIGHT:			return OnNotSupportedMsg(wParam, lParam);
		case LB_GETITEMRECT:			return OnNotSupportedMsg(wParam, lParam);
		case LB_GETLISTBOXINFO:			return OnNotSupportedMsg(wParam, lParam);
		case LB_GETLOCALE:				return OnNotSupportedMsg(wParam, lParam);
		case LB_GETSELITEMS:			return OnNotSupportedMsg(wParam, lParam);
		case LB_INITSTORAGE:			return OnNotSupportedMsg(wParam, lParam);
		case LB_SELITEMRANGE:			return OnNotSupportedMsg(wParam, lParam);
		case LB_SELITEMRANGEEX:			return OnNotSupportedMsg(wParam, lParam);
		case LB_SETANCHORINDEX:			return OnNotSupportedMsg(wParam, lParam);
		case LB_SETCARETINDEX:			return OnNotSupportedMsg(wParam, lParam);
		case LB_SETCOLUMNWIDTH:			return OnNotSupportedMsg(wParam, lParam);
		case LB_SETCOUNT:				return OnNotSupportedMsg(wParam, lParam);
		case LB_SETHORIZONTALEXTENT:	return OnNotSupportedMsg(wParam, lParam);
		case LB_SETITEMHEIGHT:			return OnNotSupportedMsg(wParam, lParam);
		case LB_SETLOCALE:				return OnNotSupportedMsg(wParam, lParam);
		case LB_SETTABSTOPS:			return OnNotSupportedMsg(wParam, lParam);
		}
		return ::DefWindowProc(hWnd, uiMsg, wParam, lParam);
	}

	virtual LRESULT _OnDestroy() override
	{
		m_isDestroyed = true;
		//m_hWnd = 0;

		m_hasFocus = false;
		m_isEnabled = true;

		//m_curIdx = m_curIdxOriginal = m_mouseOverItemIdx = 0xFFFFFFFF;
		//m_topIdx = 0;
		//m_cxMaxStringWidth = 0.0f;
		//m_cyItem = 15.0f;
		m_isEnsureVisibleNeeded = false;
		return 0;
	}

	// WM_SIZE message handler.
	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_style.Update();
		XeASSERT(!m_style.WS().hasLBS_UnsupportedStyle());

		if (m_isEnsureVisibleNeeded)
		{
			if (m_curIdx < m_items.size())
			{
				_EnsureIsVisible(m_curIdx);
			}
			m_isEnsureVisibleNeeded = false;
		}
		if (m_buttonsHlpr.m_hasButtons)
		{
			CRect rcClient = _GetClientRect();
			m_buttonsHlpr.PlaceButtons(rcClient);
		}
		_AdjustScrollBars();
		return CXeD2DWndBase::_OnSize(hWnd, wParam, lParam);
	}

	// WM_ENABLE message handler.
	virtual LRESULT _OnEnableWindow(bool bEnable) override
	{
		XeASSERT(::IsWindow(Hwnd()));

		m_isEnabled = (bool)bEnable;
		if (m_vScollBar)
		{
			m_vScollBar->EnableWindow(bEnable);
		}
		if (m_hScollBar)
		{
			m_hScollBar->EnableWindow(bEnable);
		}
		_RedrawDirectly();
		return 0;
	}
#pragma endregion Create

#pragma region CustomListBoxSupport
public:
	// Set LB style - needed by CXePopupCtrl
	void SetStyle(DWORD dwStyle, DWORD dwExStyle)
	{
		//XeASSERT(m_xeUI);
		//m_style.Set(Hwnd(), dwStyle, dwExStyle, m_xeUI);
	}

	void SetItemsList2(const std::vector<ListBoxExItem>& list, bool isKeepSelected = false)
	{
		int curSel = isKeepSelected ? (int)OnGetCurSelMsg(0, 0) : -1;
		size_t topIdx = isKeepSelected ? OnGetTopIdxMsg(0, 0) : 0xFFFFFFFF;
		SetItemsList(list, curSel, topIdx);
	}

	void SetItemsList(const std::vector<ListBoxExItem>& list, int nSelIdx = -1, size_t topIdx = 0xFFFFFFFF)
	{
		if (topIdx != 0xFFFFFFFF && topIdx >= list.size()) { topIdx = 0xFFFFFFFF; }
		if (nSelIdx != -1 && nSelIdx >= (int)list.size()) { nSelIdx = -1; }
		m_curIdx = nSelIdx >= 0 ? (size_t)nSelIdx : 0xFFFFFFFF;
		m_topIdx = topIdx == 0xFFFFFFFF ? 0 : topIdx;
		m_items.clear();
		m_items = list;
		_Update_cx_max(0xFFFFFFFF);
		_AdjustScrollBars();
		_RedrawDirectly();
	}

	const std::vector<ListBoxExItem>& GetItemsList() const { return m_items; }

	const ListBoxExItem* GetItemDataAtIndexConst(int idx) const
	{
		if (idx >= 0 && idx < m_items.size())
		{
			return &(m_items[idx]);
		}
		return nullptr;
	}

	ListBoxExItem* GetItemDataAtIndex(int idx)
	{
		if (idx >= 0 && idx < m_items.size())
		{
			return &(m_items[idx]);
		}
		return nullptr;
	}

	const ListBoxExItem* GetCurSelItemDataConst() const
	{
		size_t curSel = OnGetCurSelMsg(0, 0);
		return GetItemDataAtIndexConst(curSel == 0xFFFFFFFF ? -1 : (int)curSel);
	}

	bool SetItemDataAtIndex(int idx, const ListBoxExItem& item)
	{
		ListBoxExItem* pItem = GetItemDataAtIndex(idx);
		if (pItem)
		{
			*pItem = item;
			return true;
		}
		return false;
	}

	CRect GetItemScreenRect(size_t idx) const
	{
		return _GetItemScreenRect(idx);
	}

	void CheckAllCheckboxes(bool isChecked)
	{
		for (ListBoxExItem& item : m_items)
		{
			item.m_item_data.m_isChecked = isChecked;
		}
		_RedrawDirectly();
	}

	// In "CheckBoxes" mode:
	// - selecting an item does not make it the current item.
	// - clicking on an item does not trigger select end ok.
	//   (if the listbox is in a popup window the window is not dismissed).
	void SetCheckBoxesMode(bool isCheckBoxesMode, bool hasOkCancelButtons,
			bool isSetCurItemEnabled, const wchar_t* pTxtCustomButton = nullptr)
	{
		m_isSetCurItemEnabled = isSetCurItemEnabled;
		m_isCheckBoxesMode = isCheckBoxesMode;
		if (hasOkCancelButtons)
		{
			m_buttonsHlpr.CalculateSize(m_xeUI);
		}
		else if (pTxtCustomButton)
		{
			m_buttonsHlpr.m_isOkCancelButtons = false;
			m_buttonsHlpr.m_txtButton = pTxtCustomButton;	// Text of button (when custom text button used).
			m_buttonsHlpr.CalculateSize(m_xeUI);
		}
	}

	// Get cx space used up for the checkbox.
	int GetCheckBoxCX() const { return (int)m_cbDim.GetCheckBoxCX(); }

	bool IsOkOrCustomButtonClicked() const { return m_isOk_or_custom_button_clicked; }

	CRect CalcPopupWindowSize(int x, int y, int cxMinWidth, double dblMaxWidthMultiplier,
		int numItemsToShow)
	{
		_InitializeLBitemUIsize();
		int cyHeight = numItemsToShow * GetItemHeight() + m_buttonsHlpr.GetButtonsCY();
		int cxVscroll = m_items.size() > numItemsToShow ? GetVscrollbarWidth() : 0;
		int cxCheckBox = m_isCheckBoxesMode ? GetCheckBoxCX() : 0;
		int cxNeeded = cxCheckBox + GetMaxStringWidth() + cxVscroll;
		if (m_buttonsHlpr.GetMinCXforButtons() > cxMinWidth)
		{
			cxMinWidth = m_buttonsHlpr.GetMinCXforButtons();
		}
		if (cxNeeded > cxMinWidth)
		{
			int cxMax = (int)((double)cxMinWidth * dblMaxWidthMultiplier + 0.5);
			cxNeeded = cxNeeded > cxMax ? cxMax : cxNeeded;
		}
		else
		{
			cxNeeded = cxMinWidth;
		}
		return CRect(x, y, x + cxNeeded, y + cyHeight);
	}

	CRect CalcPopupMenuSize(int x, int y)
	{
		_InitializeLBitemUIsize();
		int numItemsToShow = (int)m_items.size();
		int cyHeight = numItemsToShow * GetItemHeight();
		int cxNeeded = GetCheckBoxCX() + GetMaxStringWidth() + (int)m_cxMenuRmargin;
		return CRect(x, y, x + cxNeeded, y + cyHeight);
	}
#pragma endregion CustomListBoxSupport

#pragma region CXePopupCtrlBase_implementation
public:
	virtual HWND GetHwnd() const override { return Hwnd(); }
	virtual HWND GetVscrollHwnd() const override { return m_hasVscroll ? m_vScollBar->GetSafeHwnd() : 0; }
	virtual HWND GetHscrollHwnd() const override { return m_hasHscroll ? m_hScollBar->GetSafeHwnd() : 0; }

	virtual CSize GetWindowPreferredSize() override
	{
		XeASSERT(FALSE);	// Not supported.
		return CSize();
	}

	virtual void AdjustPopupWindowFinalSize(CRect& rcScreenPos, const CRect& rcParentCtrlScreenPos) override
	{
		m_isCloseThisMenu = false;
		int cyItem = GetItemHeight();
		size_t num_items = m_items.size();
		int cyItems = (int)num_items * cyItem;
		int cyAvailableItemsSize = rcScreenPos.Height() - m_buttonsHlpr.GetButtonsCY();
		if (cyItems < cyAvailableItemsSize)
		{
			rcScreenPos.bottom = rcScreenPos.top + cyItems + 2 + m_buttonsHlpr.GetButtonsCY();
			if (GetMaxStringWidth() > rcScreenPos.Width())
			{
				rcScreenPos.bottom += GetHscrollbarHeight();
			}
		}
	}

	virtual DropWndOffset GetDropWndOffsetPreference() const override
	{
		return m_type == LBType::Menu ? DropWndOffset::OnParentCtrl : DropWndOffset::AboveParentCtrl;
	}

	virtual void PopupDropDown(HWND hWnd, DWORD dwStyle, DWORD dwExStyle) override
	{
		XeASSERT(::IsWindow(hWnd));
		SetHwnd(hWnd);
		XeASSERT(Hwnd() && m_xeUI);
		m_style.Set(Hwnd(), dwStyle, dwExStyle, m_xeUI);
		m_control_has_border = true;
		m_isSelEndOk = m_isSelEndCancel = m_isSelEndCancelWithESCkey = false;
		m_curIdxOriginal = m_curIdx;
		m_isEnsureVisibleNeeded = true;
		_NotifyParent(CBN_DROPDOWN, -1);
	}

	// Called by XePopupParent before popup window created (to tell the control that it's in a popup window).
	virtual void PopupOpening() override
	{
		m_isPopupCtrl = true;
	}

	virtual void PopupClosed() override
	{
		_NotifyParent(CBN_CLOSEUP, -1);
		SetHwnd(0);
		m_isPopupCtrl = false;
		m_vScollBar.reset();
		m_hScollBar.reset();
		m_hasVscroll = m_hasHscroll = false;
	}

	// Return true if 'this' is the first instance of the 'control'.
	// The derived control should return true - unless 'this' instance is a 'sub-control' window (e.g. sub-menu).
	virtual bool IsFirstInstance() const override
	{
		return m_pParentMenu == nullptr;	// Is 'this' first menu (parent of sub-menu) or LB popup.
	}

	// Return true when user has made any kind of choice in the control that should close the popup window.
	virtual bool IsPopupEndCondition() override
	{
		// If user selected item in LB or cancelled, then stop.
		if (m_isSelEndOk || m_isSelEndCancel)
		{
			if (m_pParentMenu)	// Does 'this' (sub) menu have a parent menu?
			{
				m_pParentMenu->_SetSelEndCancel();	// Close parent menu.
				// Note - the parent menu of 'this' will in turn call ClosePopup
				// on it's parent - and so on...
			}
			return true;
		}

		if (m_isCloseThisMenu)
		{
			return true;	// Close this menu only (not parent menu).
		}
		return false;
	}

	virtual LRESULT ProcessPopupWndMessage(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam) override
	{
		if (uiMsg == WM_MOUSEMOVE || uiMsg == WM_LBUTTONUP)
		{
			if (m_vScollBar && m_vScollBar->IsDraggingThumb())
			{
				return m_vScollBar->SendMessage(uiMsg, wParam, lParam);
			}
		}
		return CXeD2DWndBase::_OnMessageD2DBase(hwnd, uiMsg, wParam, lParam);
	}
#pragma endregion CXePopupCtrlBase_implementation

#pragma region Painting
protected:
	// Called when drop down window shown.
	virtual LRESULT _OnPrintClient(HWND hh, HDC hDC) override
	{
		LRESULT result = CXeD2DWndBase::_OnPrintClient(hh, hDC);

		::SetTimer(Hwnd(), DELAY_PAINT_TIMER, 20, nullptr);
		return result;
	}

	void _OnDelayPaintTimer()
	{
		if (m_hasVscroll)
		{
			m_vScollBar->RedrawDirectly();
		}
		if (m_hasHscroll)
		{
			m_hScollBar->RedrawDirectly();
		}
	}

	inline COLORREF _GetItemColor(const ListBoxExItem& item)
	{
		return item.m_item_data.m_isColorIdColor
			? m_xeUI->GetColor((CID)item.m_item_data.m_rgbColor)
			: (COLORREF)item.m_item_data.m_rgbColor;
	}

	inline std::tuple<std::wstring, std::wstring> _SplitOnTab(const std::wstring& str)
	{
		auto pos = str.find(L'\t');
		bool hasTab = pos != std::string::npos;
		std::wstring lhs = hasTab ? str.substr(0, pos) : str;
		std::wstring rhs = hasTab && str.size() > pos ? str.substr(pos + 1) : std::wstring(L"");
		return std::tuple<std::wstring, std::wstring>(lhs, rhs);
	}

	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcWnd) override
	{
		CRect rcReal = _GetRealClientRect(); // Not incl. buttons at bottom nor scrollbars
		int xOffset = _GetHscrollInfo().nPos;
		D2D1_RECT_F rcPaintF = RectFfromRect(rcReal);

		D2D1_RECT_F rcClipSB(rcWnd);
		if (m_hasVscroll)
		{
			rcClipSB.right = (float)m_rcVSB.left;
		}
		if (m_hasHscroll)
		{
			rcClipSB.bottom = (float)m_rcHSB.top;
		}
		pRT->PushAxisAlignedClip(rcClipSB, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

		pRT->FillRectangle(rcPaintF, GetBrush(CID::CtrlBg));

		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);

		bool isComboBoxDropDown = m_notifyComboBoxCallback != nullptr;

		bool isAltKeyDown = (::GetAsyncKeyState(VK_MENU) & 0x8000) ? true : false;
		bool isUseTabStops = m_style.WS().hasLBS_USETABSTOPS();

		std::vector<float> tab_stops;
		for (uint32_t& ts : m_tab_stops)
		{
			tab_stops.push_back((float)ts);
		}

		for (size_t i = m_topIdx; i < m_items.size(); ++i)
		{
			const ListBoxExItem& item = m_items.at(i);
			D2D1_RECT_F rcTxtF(rcPaintF);
			rcTxtF.left -= (float)xOffset;
			rcTxtF.top = m_y_start + (m_cyItem * (float)(i - m_topIdx));
			rcTxtF.bottom = rcTxtF.top + m_cyItem;
			bool isCurSelItem = i == m_curIdx;
			bool isMouseOverItem = isComboBoxDropDown && i == m_mouseOverItemIdx;
			LBitemBgPattern bg_pattern = item.m_item_data.GetBgPattern();
			if (bg_pattern != LBitemBgPattern::None)
			{
				D2D1_RECT_F rcPat(rcTxtF);
				if (m_type == LBType::Menu)
				{
					rcPat.left += m_cbDim.GetCheckBoxCX() + m_x_start + (m_hasIcons ? m_cxIcon : 0.0f);
					rcPat.right -= m_cxMenuRmargin;
					float y_mrg = (HeightOf(rcPat) - (float)tm.GetHeight()) / 2.0f;
					rcPat.top += y_mrg;
					rcPat.bottom -= y_mrg;
				}
				_DrawPattern(pRT, bg_pattern, rcPat);
			}
			if (item.m_item_data.m_isSectionStart)
			{
				pRT->DrawRectangle(rcTxtF, GetBrush(CID::CtrlTxtDis));
			}
			else
			{
				if (isCurSelItem && m_isEnabled)
				{
					D2D1_RECT_F selRectF(rcTxtF);
					pRT->FillRectangle(selRectF, GetBrush(CID::CtrlCurItemBg));
					if (m_hasFocus)
					{
						_DrawFocusRect(pRT, selRectF, CID::CtrlTxt);
					}
				}
				else if (isMouseOverItem && m_isEnabled)
				{
					D2D1_RECT_F mouseOverRectF(rcTxtF);
					CID cid = item.m_item_data.m_isItemDisabled ? CID::CtrlBg : CID::CtrlCurItemBg;
					if (m_type == LBType::Menu)
					{
						if (!item.m_item_data.m_isItemDisabled)
						{
							mouseOverRectF.bottom -= 4.0f;
							pRT->FillRectangle(mouseOverRectF, GetBrush(cid));
						}
					}
					else
					{
						pRT->FillRectangle(mouseOverRectF, GetBrush(cid));
					}
				}
				else if (item.IsItemSelected() && m_isEnabled)
				{
					D2D1_RECT_F selRectF(rcTxtF);
					pRT->FillRectangle(selRectF, GetBrush(CID::CtrlSelItemBg));
				}
			}
			if (item.m_item_data.m_isMenuSeparator)
			{
				float cxMargin = m_cbDim.GetCheckBoxCX();
				D2D1_POINT_2F p1((rcTxtF.left + cxMargin), rcTxtF.top);
				D2D1_POINT_2F p2((rcTxtF.right - m_cxMenuRmargin), rcTxtF.top);
				pRT->DrawLine(p1, p2, GetBrush(CID::XTbHotBrd));
			}
			if (item.m_item_data.m_isCheckbox)
			{
				CID clrId_FG = !m_isEnabled ? CID::CtrlTxtDis : isCurSelItem ? CID::CtrlTxt : CID::CtrlCurItemTxt;
				D2D1_RECT_F rcChkBox = m_cbDim.GetBoxRect(rcTxtF);
				DWORD dwBtnState = item.m_item_data.m_isChecked ? BST_CHECKED : BST_UNCHECKED;
				bool isDrawCBrect = m_type == LBType::Menu ? false : true;
				_DrawCheckBox(pRT, rcChkBox, dwBtnState, clrId_FG, CID::Invalid, false, isDrawCBrect);
				rcTxtF.left += m_cbDim.GetCheckBoxCX();
			}
			if (item.m_item_data.m_hasIcon)
			{
				D2D1_RECT_F rc1(rcTxtF);
				float cyDec = m_type == LBType::Menu ? 6.0f : 2.0f;
				rc1.top += cyDec;
				rc1.bottom -= cyDec;
				if (m_type != LBType::Menu)
				{
					rc1.left += 3.0f;
				}
				bool isDrawEnabled = !item.m_item_data.m_isItemDisabled;
				bool isOnLeft = true;
				D2D1_RECT_F rcImg = _DrawButtonImage(pRT, item.m_item_data.GetIconPID(), rc1,
						isOnLeft, isDrawEnabled, false);
				if (m_type != LBType::Menu)
				{
					rcTxtF.left += WidthOf(rcImg) + 2.0f;
				}
			}
			else if (item.m_item_data.m_isColorSquare)
			{
				D2D1_RECT_F colorRect(rcTxtF);
				float cxyColorRect = 12.0f;
				colorRect.right = colorRect.left + cxyColorRect;
				colorRect.top += ((HeightOf(rcTxtF) - cxyColorRect) / 2.0f);
				colorRect.bottom = colorRect.top + cxyColorRect;
				COLORREF rgbBlk = _GetItemColor(item);
				XeD2D1_COLOR_F clrD2;
				clrD2.SetFromRGB(rgbBlk, false);
				_DrawColorRect(pRT, clrD2, colorRect);
				rcTxtF.left += (cxyColorRect + 2.0f);
			}
			if (item.m_item_data.m_isSubMenu)
			{
				CID clrId_FG = !m_isEnabled || item.m_item_data.m_isItemDisabled ? CID::CtrlTxtDis : CID::CtrlTxt;
				float cxTri = (float)m_xeUI->GetValue(UIV::cxComboBoxButtonArrow);
				D2D1_RECT_F rcArw(rcTxtF);
				rcArw.left = rcArw.right - m_cxMenuRmargin - cxTri;
				_DrawSubMenuArrow(pRT, rcArw, clrId_FG);
			}
			COLORREF rgbFg = m_xeUI->GetColor(isCurSelItem ? CID::CtrlTxt : CID::CtrlCurItemTxt);
			if (item.m_item_data.m_hasColor && !item.m_item_data.m_isColorSquare)
			{
				rgbFg = _GetItemColor(item);
			}
			if (!m_isEnabled || item.m_item_data.m_isItemDisabled)
			{
				rgbFg = m_xeUI->GetColor(CID::CtrlTxtDis);
			}
			rcTxtF.left += m_x_start;
			auto [lhs, rhs] = _SplitOnTab(item.m_string);
			int idxUnderscore = -1;	// When menu and Alt key down - show item 'hot' key underscored.
			if (m_type == LBType::Menu)
			{
				if (m_hasIcons)
				{
					rcTxtF.left += (m_cxIcon + 2.0f);
				}
				idxUnderscore = PrepareMenuTextForDraw(lhs);
				if (!isAltKeyDown)
				{
					idxUnderscore = -1;
				}
			}
			if (item.m_item_data.m_isChanged)
			{
				lhs = L"* " + lhs;
			}
			D2D1_SIZE_F sizeTxtF{ 0, 0 };
			Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout = _CreateTextLayout(
					lhs, SizeOf(rcTxtF), EXE_FONT::eUI_Font);
			if (textLayout.Get())
			{
				if (idxUnderscore >= 0)
				{
					textLayout->SetUnderline(TRUE, DWRITE_TEXT_RANGE((UINT32)idxUnderscore, 1));
				}
				DWRITE_TEXT_METRICS textMetrics;
				HRESULT hr = textLayout->GetMetrics(&textMetrics);
				sizeTxtF = D2D1::SizeF((float)ceil(textMetrics.widthIncludingTrailingWhitespace),
						(float)ceil(textMetrics.height));
				_DrawTextLayout(pRT, textLayout.Get(), { rcTxtF.left, rcTxtF.top }, rcTxtF, rgbFg);
			}
			if (m_type == LBType::Menu && rhs.size())
			{
				rcTxtF.right -= m_cxMenuRmargin;
				_CreateAndDrawTextLayout(pRT, rhs, rcTxtF, rgbFg, EXE_FONT::eUI_Font, DWRITE_TEXT_ALIGNMENT_TRAILING);
			}
			else if (isUseTabStops && rhs.size())
			{
				size_t ts_index = 0;
				float x_tab = rcTxtF.left + (ts_index < tab_stops.size() ? tab_stops[ts_index++] : m_cxDefaultTab);
				std::wstring s = rhs;
				while (s.size())
				{
					auto [lhs, rhs] = _SplitOnTab(s);
					rcTxtF.left = x_tab;
					_CreateAndDrawTextLayout(pRT, lhs, rcTxtF, rgbFg);
					s = rhs;
					x_tab += (ts_index < tab_stops.size() ? tab_stops[ts_index++] : m_cxDefaultTab);
				}
			}
			else if (rhs.size())
			{
				rcTxtF.left += sizeTxtF.width;
				_CreateAndDrawTextLayout(pRT, rhs, rcTxtF, rgbFg);
			}
			if (rcTxtF.bottom >= rcPaintF.bottom)
			{
				break;
			}
		}

		if (m_buttonsHlpr.m_hasButtons)
		{
			_DrawBottomButtons(pRT, m_isEnabled);
		}

		pRT->PopAxisAlignedClip();

		if (m_hasVscroll && m_hasHscroll)	// Need to paint bottom right corner?
		{
			CRect rcBRcorner(m_rcHSB.right, m_rcVSB.bottom, m_rcVSB.right, m_rcHSB.bottom);
			pRT->FillRectangle(RectFfromRect(rcBRcorner), GetBrush(CID::CtrlBg));
		}

		if (m_control_has_border)
		{
			pRT->DrawRectangle(rcWnd, GetBrush(CID::CtrlTxtDis));
		}
	}

	void _DrawBottomButtons(ID2D1RenderTarget* pRT, bool isEnabled)
	{
		D2D1_RECT_F rcBtnAreaF = RectFfromRect(m_buttonsHlpr.m_rcBtnArea);
		CRect rcReal = _GetRealClientRect(); // Not incl. buttons at bottom nor scrollbars
		rcBtnAreaF.top = (float)rcReal.bottom;
		pRT->FillRectangle(rcBtnAreaF, GetBrush(CID::CtrlBg));
		D2D1_RECT_F rcOk = RectFfromRect(m_buttonsHlpr.m_rcOkBtn);
		D2D1_RECT_F rcCancel = RectFfromRect(m_buttonsHlpr.m_rcCancelBtn);
		pRT->DrawRectangle(rcOk, GetBrush(CID::CtrlBorder), 2.0f);
		if (m_buttonsHlpr.m_isOkCancelButtons)
		{
			pRT->DrawRectangle(rcCancel, GetBrush(CID::CtrlBorder), 2.0f);
		}
		CID cidFg = isEnabled ? CID::CtrlTxt : CID::CtrlTxtDis;
		_CreateAndDrawTextLayout(pRT, (m_buttonsHlpr.m_isOkCancelButtons ? L"Ok" : m_buttonsHlpr.m_txtButton.c_str()),
				rcOk, cidFg, EXE_FONT::eUI_Font, DWRITE_TEXT_ALIGNMENT_CENTER);
		if (m_buttonsHlpr.m_isOkCancelButtons)
		{
			_CreateAndDrawTextLayout(pRT, L"Cancel", rcCancel, cidFg, EXE_FONT::eUI_Font, DWRITE_TEXT_ALIGNMENT_CENTER);
		}
	}
#pragma endregion Painting

#pragma region Focus
protected:
	// WM_SETFOCUS message handler.
	//afx_msg LRESULT OnSetFocus(WPARAM wParam, LPARAM lParam)
	virtual LRESULT _OnSetFocus(HWND hOldWnd)
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_hasFocus = true;
		_RedrawDirectly();
		_NotifyParent(LBN_SETFOCUS, -1);
		return 0;
	}

	// WM_KILLFOCUS message handler.
	//afx_msg LRESULT OnKillFocus(WPARAM wParam, LPARAM lParam)
	virtual LRESULT _OnKillFocus(HWND hNewWnd)
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_hasFocus = false;
		_RedrawDirectly();
		_NotifyParent(LBN_KILLFOCUS, -1);
		return 0;
	}

	// Q83302: HOWTO: Use the WM_GETDLGCODE Message
	// https://jeffpar.github.io/kbarchive/kb/083/Q83302/
	// Q104637: HOWTO: Trap Arrow Keys in an Edit Control of a Dialog Box
	// https://jeffpar.github.io/kbarchive/kb/104/Q104637/
	// WM_GETDLGCODE message handler.
	virtual LRESULT _OnGetDlgCode(WPARAM wParam, LPARAM lParam) override
	{
		if (!::IsWindow(Hwnd())) { return 0; }
		BOOL bHasTabStop = (::GetWindowLong(Hwnd(), GWL_STYLE) & WS_TABSTOP) ? TRUE : FALSE;

		LRESULT lResult = DLGC_WANTARROWS | DLGC_WANTCHARS;
		if (lParam)	// lParam points to an MSG structure?
		{
			LPMSG lpmsg = (LPMSG)lParam;
			if ((lpmsg->message == WM_KEYDOWN	// Keyboard input?
				|| lpmsg->message == WM_KEYUP)
				&& lpmsg->wParam != VK_TAB		// AND NOT TAB key?
				&& lpmsg->wParam != VK_ESCAPE)	// AND NOT ESC key?
			{
				if (bHasTabStop)				// 'this' window has Tab stop?
				{
					lResult |= DLGC_WANTMESSAGE;	// We want keyboard input (except TAB and ESC).
					// Note - windows will set focus to 'this' (and send WM_SETFOCUS)
					//        if we return DLGC_WANTMESSAGE here.
				}
			}
		}
		else
		{
			if (bHasTabStop)
				lResult |= DLGC_WANTTAB;	// 'this' has WS_TABSTOP style - we want focus.
			else
				lResult |= DLGC_STATIC;		// no tab stop - we don't want focus.
		}
		return lResult;
	}
#pragma endregion Focus

#pragma region Helpers
public:
	int GetItemHeight() const { return (int)(m_cyItem + 0.5f); }
	int GetMaxStringWidth() const { return (int)(m_cxMaxStringWidth + 0.5f); }
	int GetHscrollbarHeight() const { return m_cxSysSbV; }
	int GetVscrollbarWidth() const { return m_cySysSbH; }

protected:
	/* LBN_SELCHANGE notify message.
	 * This notification code is sent only by a list box that has the LBS_NOTIFY style.
	 * This notification code is not sent if the LB_SETSEL, LB_SETCURSEL, LB_SELECTSTRING,
	   LB_SELITEMRANGE or LB_SELITEMRANGEEX message changes the selection.
	 * For a multiple-selection list box, the LBN_SELCHANGE notification code is sent whenever
	   the user presses an arrow key, even if the selection does not change.
	*/
	bool _SetSelectedPosition(size_t idx, bool isNotify = true, const CXeShiftCtrlAltKeyHelper* psca = nullptr)
	{
		bool isSelectingMultipleItems = psca ? (psca->IsOnlyShiftDown() || psca->IsOnlyCtrlDown()) : false;
		if (m_style.WS().hasLBS_MULTIPLESEL() && !isSelectingMultipleItems)
		{
			_ClearItemsSelectFlag();
		}
		bool isValidIndex = idx < m_items.size();
		if (m_isSetCurItemEnabled)
		{
			if (m_style.WS().hasLBS_MULTIPLESEL())
			{
				if (psca && psca->IsOnlyShiftDown())
				{
					_SetSelectedFlagOnItemRange(m_curIdx, idx);
					m_curIdx = idx;
				}
				else if (psca && psca->IsOnlyCtrlDown())
				{
					_ToggleSelectedFlagOnItem(idx);
					isNotify = false;
				}
				else
				{
					_SetSelectedFlagOnItem(idx);
					m_curIdx = idx;
				}
			}
			else
			{
				m_curIdx = idx;
			}
		}
		if (::IsWindow(Hwnd()))
		{
			if (isValidIndex && !_IsVisible(idx))
			{
				_EnsureIsVisible(idx);
			}
			_RedrawDirectly();
		}
		if (isNotify)
		{
			_NotifyParent(LBN_SELCHANGE, -1);
		}
		return isValidIndex;
	}

	bool _NotifyParent(UINT nfCode, int item_idx)
	{
		if (m_notifyComboBoxCallback)
		{
			m_notifyComboBoxCallback(nfCode, item_idx);
			return true;
		}
		if (m_style.WS().hasLBS_NOTIFY())
		{
			WPARAM wParam = MAKELONG(::GetDlgCtrlID(Hwnd()), nfCode);
			if (nfCode > XE_CTRL_NOTIFY_FIRST)
			{
				::SendMessage(::GetParent(Hwnd()), XE_CONTROL_NOTIFY_MSG, wParam, item_idx);
			}
			else
			{
				if (!(m_type == LBType::Menu && wParam == ID__APP_EXIT))	// App exit is special - handled by toolbar.
				{
					::SendMessage(::GetParent(Hwnd()), WM_COMMAND, wParam, (LPARAM)Hwnd());
				}
			}
			return true;
		}
		return false;
	}

	void _SetSelEndOk()
	{
		m_isSelEndOk = true;
		if (m_curIdx < m_items.size())
		{
			m_uSelectedItemCommandID = (UINT)m_items[m_curIdx].m_extra_data;
		}
		// Post a dummy message to ensure that the CXePopupCtrl::ShowPopup
		// message loop sees the "end" condition.
		::PostMessage(Hwnd(), WM_NULL, 0, 0);
	}

	void _SetSelEndCancel()
	{
		m_isSelEndCancel = true;
		// Post a dummy message to ensure that the CXePopupCtrl::ShowPopup
		// message loop sees the "end" condition.
		::PostMessage(Hwnd(), WM_NULL, 0, 0);
	}
	
	void _CloseThisMenu()
	{
		m_isCloseThisMenu = true;
		// Post a dummy message to ensure that the CXePopupCtrl::ShowPopup
		// message loop sees the "end" condition.
		::PostMessage(Hwnd(), WM_NULL, 0, 0);
	}

	bool _IsVisible(size_t idx) const
	{
		if (!::IsWindow(Hwnd())) { return true; }
		CRect rcClient = _GetRealClientRect();
		CRect rcItem = _GetItemRect(idx);
		return rcItem.top >= rcClient.top && rcItem.bottom <= rcClient.bottom;
	}

	void _EnsureIsVisible(size_t idx)
	{
		if (idx < m_topIdx)
		{
			m_topIdx = idx;
		}
		else
		{
			while (m_topIdx < m_items.size())
			{
				if (_IsVisible(idx))
				{
					break;
				}
				++m_topIdx;
			}
		}
		if (m_vScollBar)
		{
			BOOL fRedraw = ::IsWindowVisible(Hwnd());
			m_vScollBar->SetScrollPos((int)m_topIdx, fRedraw);
		}
	}

	CRect _GetClientScreenRect() const
	{
		CRect rcClient;
		::GetClientRect(Hwnd(), &rcClient);
		CPoint ptTL(rcClient.left, rcClient.top);
		CPoint ptBR(rcClient.right, rcClient.bottom);
		::ClientToScreen(Hwnd(), &ptTL);
		::ClientToScreen(Hwnd(), &ptBR);
		CRect rcScr(ptTL, ptBR);
		return rcScr;
	}

	CRect _GetRealClientRect() const
	{
		XeASSERT(::IsWindow(Hwnd()));
		CRect rcClient = _GetClientRect();
		rcClient.bottom -= m_buttonsHlpr.GetButtonsCY();
		rcClient.right = m_hasVscroll ? m_rcVSB.left : rcClient.right;
		rcClient.bottom = m_hasHscroll ? m_rcHSB.top : rcClient.bottom;
		return rcClient;
	}

	int _GetClientRightEdge() const
	{
		XeASSERT(::IsWindow(Hwnd()));
		CRect rcClient = _GetClientRect();
		int xClientRight = m_hasVscroll ? m_rcVSB.left : rcClient.right;
		return xClientRight;
	}

	bool _IsPointInClientRect(const CPoint& pt) const
	{
		CRect rcClient = _GetClientRect();
		return rcClient.PtInRect(pt);
	}

	CRect _GetItemRect(size_t idx) const
	{
		int y = (int)(m_y_start + (m_cyItem * (idx - m_topIdx)));
		return CRect(0, y, _GetClientRightEdge(), y + (int)(m_cyItem+ 0.5f));
	}

	CRect _GetItemScreenRect(size_t idx) const
	{
		if (idx >= m_items.size()) { return CRect(); }
		CRect rcItem = _GetItemRect(idx);
		CPoint ptTL(rcItem.left, rcItem.top);
		CPoint ptBR(rcItem.right, rcItem.bottom);
		::ClientToScreen(Hwnd(), &ptTL);
		::ClientToScreen(Hwnd(), &ptBR);
		CRect rcScr(ptTL, ptBR);
		return rcScr;
	}

	// Returns index into m_items if an LB item is found at x, y coords.
	// 0xFFFFFFFF is returned if no item found.
	size_t _GetItemIndexAtPosition(int x, int y) const
	{
		CRect rcRealClient = _GetRealClientRect();
		bool isXinRect = (x >= rcRealClient.left && x < rcRealClient.right);
		bool isYinRect = (y >= rcRealClient.top && y < rcRealClient.bottom);
		size_t idx = isXinRect && isYinRect && m_cyItem > 0.0f ? m_topIdx + (y / (int)(m_cyItem + 0.5f)) : 0xFFFFFFFF;
		return idx;
	}

	size_t _GetItemIndexAtPosition(const CPoint& pt) const
	{
		return _GetItemIndexAtPosition(pt.x, pt.y);
	}

	size_t _GetItemIndexFromCommandID(UINT uID) const
	{
		size_t index = 0;
		for (const ListBoxExItem& item : m_items)
		{
			if (uID == (UINT)item.m_extra_data)
			{
				return index;
			}
			++index;
		}
		return 0xFFFFFFFF;
	}

	ListBoxExItem* _GetItemPtrAtPosition(const CPoint& pt)
	{
		size_t idx = _GetItemIndexAtPosition(pt);
		ListBoxExItem* pItem = idx < m_items.size() ? &(m_items[idx]) : nullptr;
		return pItem;
	}

	const ListBoxExItem* _GetItemConstPtrAtPosition(const CPoint& pt) const
	{
		size_t idx = _GetItemIndexAtPosition(pt);
		const ListBoxExItem* pItem = idx < m_items.size() ? &(m_items[idx]) : nullptr;
		return pItem;
	}

	ListBoxExItem* _GetItemPtrAtIndex(size_t idx)
	{
		ListBoxExItem* pItem = idx < m_items.size() ? &(m_items[idx]) : nullptr;
		return pItem;
	}

	const ListBoxExItem* _GetItemConstPtrAtIndex(size_t idx) const
	{
		const ListBoxExItem* pItem = idx < m_items.size() ? &(m_items[idx]) : nullptr;
		return pItem;
	}

	bool _IsItemAtIndex_A_SubMenu(size_t idx) const
	{
		const ListBoxExItem* pItem = _GetItemConstPtrAtIndex(idx);
		if (m_type == LBType::Menu && pItem && pItem->m_item_data.m_isSubMenu)
		{
			return true;
		}
		return false;
	}

	bool _Update_cx_max(size_t new_item_idx)
	{
		bool isUseTabStops = m_style.WS().hasLBS_USETABSTOPS();

		std::vector<uint32_t> cxTextCol;
		std::function<void(size_t, const std::wstring&)>
			set_cxTextCol = [&](size_t tabIdx, const std::wstring& str)
		{
			CSize sizeStr = m_xeUI->GetTextSizeW(EXE_FONT::eUI_Font, str.c_str());
			if (cxTextCol.size() < tabIdx + 1) { cxTextCol.resize(tabIdx + 1); }
			if (cxTextCol[tabIdx] < (uint32_t)sizeStr.cx) { cxTextCol[tabIdx] = (uint32_t)sizeStr.cx; }
		};

		std::function<void(const std::wstring&)>
			addStr = [&](const std::wstring& str)
		{
			if (isUseTabStops || m_type == LBType::Menu)
			{
				auto [lhs, rhs] = _SplitOnTab(str);
				set_cxTextCol(0, lhs);
				if (m_type == LBType::Menu && rhs.size())	// Menu can only have one tab.
				{
					set_cxTextCol(1, rhs);
				}
				else if (rhs.size())
				{
					size_t tabIdx = 1;
					std::wstring s = rhs;
					while (s.size())
					{
						auto [lhs, rhs] = _SplitOnTab(s);
						set_cxTextCol(tabIdx++, lhs);
						s = rhs;
					}
				}
			}
			else
			{
				set_cxTextCol(0, str);
			}
		};

		std::function<void(const ListBoxExItem&)> addItem = [&](const ListBoxExItem& item)
		{
			addStr(item.m_string);
			m_hasIcons |= item.m_item_data.m_hasIcon ? true : false;
			m_HasColorSquares |= item.m_item_data.m_isColorSquare ? true : false;
		};

		std::function<uint32_t(size_t)> getTabXpos = [&](size_t col)
		{
			std::vector<uint32_t> cxTabSetting = m_tab_stops;
			while (cxTabSetting.size() < col)
			{
				size_t n = cxTabSetting.size();
				cxTabSetting.push_back((n > 0 ? cxTabSetting[n - 1] : 0) + (int)m_cxDefaultTab);
			}
			UIScaleFactor sf = m_xeUI->GetUIScaleFactor();
			for (uint32_t& ts : cxTabSetting)
			{
				ts = sf.ScaleX(ts);
			}
			uint32_t tab_X_pos = 0;	// col[0] X pos is 0
			std::vector<uint32_t> tabXpos{ tab_X_pos };
			for (uint32_t cxTab : cxTabSetting)
			{
				tab_X_pos += cxTab;
				tabXpos.push_back(tab_X_pos);
			}
			return tabXpos[col];
		};

		if (new_item_idx < m_items.size())
		{
			addItem(m_items[new_item_idx]);
		}
		else
		{
			m_hasIcons = m_HasColorSquares = false;
			for (const ListBoxExItem& item : m_items)
			{
				addItem(item);
			}
		}
		m_cxIcon = m_hasIcons && m_type == LBType::Menu ? m_cyItem * 0.8f : m_HasColorSquares ? 15.0f : 0.0f;
		float cx = m_cxIcon + (float)(cxTextCol.size() > 0 ? cxTextCol[0] : 0) + 15.0f;
		if (m_type == LBType::Menu && cxTextCol.size() > 1)
		{
			cx = m_cxIcon + (float)(cxTextCol[0] + cxTextCol[1] + 15);
		}
		else if (isUseTabStops && cxTextCol.size() > 1)
		{
			size_t last_col = cxTextCol.size() - 1;
			uint32_t X_tab = getTabXpos(last_col);
			// cx for LB is last tab stop + longest string cx from last tab.
			cx = m_cxIcon + (float)(X_tab + cxTextCol[last_col] + 15);
		}
		if (cx > m_cxMaxStringWidth)
		{
			m_cxMaxStringWidth = cx;
			return true;
		}
		return false;
	}

	void _ClearItemsSelectFlag()
	{
		for (ListBoxExItem& item : m_items)
		{
			item.SetItemSelectedState(false);
		}
	}

	void _SetSelectedFlagOnItemRange(size_t firstIdx, size_t lastIdx)
	{
		if (firstIdx > lastIdx)
		{
			std::swap(firstIdx, lastIdx);
		}
		if (firstIdx < m_items.size() && lastIdx < m_items.size())
		{
			for (size_t i = firstIdx; i <= lastIdx; ++i)
			{
				m_items[i].SetItemSelectedState(true);
			}
		}
	}

	void _SetSelectedFlagOnItem(size_t idx)
	{
		ListBoxExItem* pItem = _GetItemPtrAtIndex(idx);
		if (pItem)
		{
			pItem->SetItemSelectedState(true);
		}
	}

	void _ToggleSelectedFlagOnItem(size_t idx)
	{
		ListBoxExItem* pItem = _GetItemPtrAtIndex(idx);
		if (pItem)
		{
			pItem->SetItemSelectedState(!pItem->IsItemSelected());
		}
	}
#pragma endregion Helpers

#pragma region Mouse
protected:
	// WM_LBUTTONDOWN
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint pt) override
	{
		m_ptOfLbtnDown = pt;

		if (!_IsPointInClientRect(pt))
		{
			_SetSelEndCancel();
		}
		if (!m_style.WS().hasLBS_COMBOBOX() && m_type != LBType::Menu)
		{
			::SetFocus(Hwnd());
		}
		return 0;
	}

	// WM_LBUTTONUP
	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint pt) override
	{
		CXeShiftCtrlAltKeyHelper sca;
		// If point of L button up is not near L button down point - ignore this message.
		if (abs(pt.x - m_ptOfLbtnDown.x) > 8 || abs(pt.y - m_ptOfLbtnDown.y) > 8)
		{
			return 0;
		}
		size_t idx = _GetItemIndexAtPosition(pt);
		ListBoxExItem* pItem = _GetItemPtrAtIndex(idx);
		bool isPointInCheckbox = false;
		if (pItem && !pItem->m_item_data.m_isItemDisabled)
		{
			if (m_type != LBType::Menu && pItem->m_item_data.m_isCheckbox)
			{
				if (pt.x < m_cbDim.GetCheckBoxCX())
				{
					pItem->m_item_data.m_isChecked = pItem->m_item_data.m_isChecked == 1 ? 0 : 1;
					isPointInCheckbox = true;
					_NotifyParent(LB_EX_NOTIFY_ITEM_CHECKED, (int)idx);
				}
			}
			if (m_type == LBType::Menu && pItem->m_item_data.m_isSubMenu)
			{
				_NotifyParent(LB_EX_NOTIFY_SUBMENU_SELECTED, (int)idx);
				// Note - when sub-menu shown - we don't return here until sub-menu finished
			}
			else
			{
				_SetSelectedPosition(idx, true, &sca);
				if (m_isSetCurItemEnabled && !isPointInCheckbox)
				{
					_SetSelEndOk();
				}
			}
		}
		if (m_buttonsHlpr.m_hasButtons)
		{
			if (m_buttonsHlpr.m_rcOkBtn.PtInRect(pt))
			{
				m_isOk_or_custom_button_clicked = true;
				m_curIdx = -1;
				_SetSelEndOk();
			}
			else if (m_buttonsHlpr.m_rcCancelBtn.PtInRect(pt))
			{
				_SetSelEndCancel();
			}
		}
		return 0;
	}

	// WM_LBUTTONDBLCLK
	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint pt) override
	{
		_NotifyParent(LBN_DBLCLK, -1);
		if (m_isSetCurItemEnabled)
		{
			_SetSelEndOk();
		}
		return 0;
	}

	// WM_RBUTTONDOWN
	virtual LRESULT _OnRightDown(UINT nFlags, CPoint pt) override
	{
		if (!_IsPointInClientRect(pt))
		{
			_SetSelEndCancel();
		}
		return 0;
	}

	// WM_MOUSEWHEEL message handler.
	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint pt) override
	{
		XeASSERT(::IsWindow(Hwnd()));

		WORD wSBcode = 0xFFFF;
		if (zDelta >= WHEEL_DELTA)
		{
			wSBcode = SB_LINEUP;
		}
		else if (zDelta <= -WHEEL_DELTA)
		{
			wSBcode = SB_LINEDOWN;
			zDelta = -zDelta;
		}
		if (wSBcode != 0xFFFF && m_vScollBar)
		{
			do
			{
				::SendMessage(Hwnd(), WM_VSCROLL, MAKELONG(wSBcode, 0), (LPARAM)m_vScollBar->Hwnd());
			} while ((zDelta -= WHEEL_DELTA) >= WHEEL_DELTA);
			::SendMessage(Hwnd(), WM_VSCROLL, MAKELONG(SB_ENDSCROLL, 0), (LPARAM)m_vScollBar->Hwnd());
		}
		return 0;
	}

	// WM_MOUSEHWHEEL message handler.
	virtual LRESULT _OnMouseHWheel(WORD fwKeys, short zDelta, CPoint pt) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		if (m_hScollBar && m_hScollBar->GetSafeHwnd())
		{
			UINT uSBcode = zDelta < 0 ? SB_LINELEFT : SB_LINERIGHT;
			::SendMessage(Hwnd(), WM_HSCROLL, MAKELONG(uSBcode, 0), (LPARAM)m_hScollBar->Hwnd());
			::SendMessage(Hwnd(), WM_HSCROLL, MAKELONG(SB_ENDSCROLL, 0), (LPARAM)m_hScollBar->Hwnd());
		}
		return 0;
	}

	// WM_MOUSEMOVE
	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint pt) override
	{
		if (m_suppressMouseMoveMessages)
		{
			// Note - need to suppress the first (few) mouse move message - because:
			// When 'this' is a sub-menu that was initiated by keyboard VK_LEFT - the sub-menu 
			// is closed immediately if the mouse cursor in an "unfortunate" position.
			// The same happes when top level menu keyboard navigation.
			--m_suppressMouseMoveMessages;
			return 0;
		}
		m_ptMouseHover = pt;
		::KillTimer(Hwnd(), TIMER_HOVER_ID);
		::SetTimer(Hwnd(), TIMER_HOVER_ID, 400, nullptr);

		size_t idx = _GetItemIndexAtPosition(pt);
		if (m_mouseOverItemIdx != idx)
		{
			m_mouseOverItemIdx = idx;
			if (m_notifyComboBoxCallback)	// Is 'this' combo box drop down list?
			{
				_RedrawDirectly();
			}
		}

		CPoint ptScr(pt);
		::ClientToScreen(Hwnd(), &ptScr);
		if (m_pParentMenu)	// Is 'this a sub-menu? (because has parent).
		{
			// Note - OnMouseMove is called in the last sub-menu instance.
			// Meaning that OnMouseMove of parent or grand-parents is not called.
			CRect rcScrParentClient, rcScrClient;
			CXeListBoxExCommon* pMenu = this;
			while (pMenu && pMenu->m_pParentMenu)
			{
				rcScrParentClient = pMenu->m_pParentMenu->_GetClientScreenRect();
				rcScrClient = pMenu->_GetClientScreenRect();
				// Is cursor not in 'pMenu' menu AND is cursor in parent (of pMenu) menu
				// AND is cursor not in sub-menu item (that points to pMenu)?
				if (!rcScrClient.PtInRect(ptScr) && rcScrParentClient.PtInRect(ptScr)
					&& !pMenu->m_rcParentMenuItemScreenPos.PtInRect(ptScr))
				{
					// Close all sub-menus below pMenu.
					CXeListBoxExCommon* pMenuToClose = this;
					while (pMenuToClose)
					{
						pMenuToClose->_CloseThisMenu();
						if (pMenuToClose == pMenu)
						{
							break;
						}
						pMenuToClose = pMenuToClose->m_pParentMenu;
					}
					break;
				}
				pMenu = pMenu->m_pParentMenu;
			}

		}

		_ProcessTopLevelMenuNavigation(TopLevelMenuNavigationParameters(ptScr));
		return 0;
	}

	void _OnHoverTimer()
	{
		CPoint ptMouse;
		::GetCursorPos(&ptMouse);
		::ScreenToClient(Hwnd(), &ptMouse);
		if (std::abs(ptMouse.x - m_ptMouseHover.x) < 5 && std::abs(ptMouse.y - m_ptMouseHover.y) < 5)
		{
			size_t idx = _GetItemIndexAtPosition(ptMouse);
			if (_IsItemAtIndex_A_SubMenu(idx))
			{
				_NotifyParent(LB_EX_NOTIFY_SUBMENU_SELECTED, (int)idx);
				// Note - when sub-menu shown - we don't return here until sub-menu finished
			}
		}
	}

	// WM_SETCURSOR
	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam) override
	{
		HWND hWnd = (HWND)wParam;
		WORD hitTest = LOWORD(lParam);
		WORD msg = HIWORD(lParam);
		if (hitTest == HTCLIENT)
		{
			::SetCursor(m_xeUI->GetAppCursor(false));
			return TRUE;
		}
		return FALSE;
	}

	// WM_CANCELMODE message handler.
	virtual LRESULT _OnCancelMode() override
	{
		XeASSERT(::IsWindow(Hwnd()));

		// Need to handle WM_CANCELMODE message.
		// See -> http://support.microsoft.com/kb/74548/en-us

		_NotifyParent(LBN_SELCANCEL, -1);
		return CXeD2DWndBase::_OnCancelMode();
	}
#pragma endregion Mouse

#pragma region Keyboard
protected:
	// WM_KEYDOWN message handler.
	virtual LRESULT _OnKeyDown(WPARAM wParam, LPARAM lParam) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		SCROLLINFO si = _GetVscrollInfo();
		bool isCurIdxValid = m_curIdx < m_items.size();
		CXeShiftCtrlAltKeyHelper sca;
		switch (wParam)
		{
		case VK_PRIOR:
			if (sca.IsNoneDown())
			{
				_SetSelectedPosition(isCurIdxValid
					? (m_curIdx < si.nPage ? 0 : m_curIdx - si.nPage) : 0);
			}
			break;

		case VK_NEXT:
			if (sca.IsNoneDown())
			{
				_SetSelectedPosition(isCurIdxValid
					? (m_curIdx + si.nPage > m_items.size() - 1 ? m_items.size() - 1 : m_curIdx + si.nPage) : 0);
			}
			break;

		case VK_HOME:
			if (sca.IsNoneDown())
			{
				_SetSelectedPosition(0);
			}
			break;

		case VK_END:
			if (sca.IsNoneDown())
			{
				_SetSelectedPosition(m_items.size() - 1);
			}
			break;

		case VK_LEFT:
			if (sca.IsNoneDown())
			{
				if (m_pParentMenu)	// Is 'this' a sub-menu?
				{
					_CloseThisMenu();
				}
				else
				{
					_ProcessTopLevelMenuNavigation(TopLevelMenuNavigationParameters((UINT)wParam));
				}
			}
			break;

		case VK_RIGHT:
			if (sca.IsNoneDown())
			{
				if (_IsItemAtIndex_A_SubMenu(m_curIdx))
				{
					_NotifyParent(LB_EX_NOTIFY_SUBMENU_SELECTED_BY_KEYBOARD, (int)m_curIdx);
					// Note - when sub-menu shown - we don't return here until sub-menu finished
				}
				else
				{
					_ProcessTopLevelMenuNavigation(TopLevelMenuNavigationParameters((UINT)wParam));
				}
			}
			break;

		case VK_UP:
			if (sca.IsNoneDown())
			{
				_SetSelectedPosition(isCurIdxValid ? (m_curIdx - 1) : (m_items.size() - 1));
			}
			break;

		case VK_DOWN:
			if (sca.IsNoneDown())
			{
				_SetSelectedPosition(isCurIdxValid ? (m_curIdx + 1) : 0);
			}
			break;

		case VK_RETURN:
			if (m_isSetCurItemEnabled && isCurIdxValid && !m_items[m_curIdx].m_item_data.m_isItemDisabled)
			{
				_SetSelEndOk();
			}
			break;

		case VK_ESCAPE:
			if (m_notifyComboBoxCallback)
			{
				m_notifyComboBoxCallback(CBN_SELENDCANCEL, -1);
				m_isSelEndCancelWithESCkey = true;
				_SetSelEndCancel();
				_SetSelectedPosition(m_curIdxOriginal);
			}
			break;
		}

		if (m_type == LBType::Menu && sca.IsOnlyAltDown())	// Process short-cut keys for menu.
		{
			_ProcessMenuShortcutKey(wParam, lParam);
		}

		return 0;	// Indicate we processed this message (eat all key msgs).
		// Note - testing shows that windows looks for another child control to process
		// keyboard input if we don't return 0 here (and we lose focus).
	}

	// WM_KEYUP message handler.
	virtual LRESULT _OnKeyUp(WPARAM wParam, LPARAM lParam) override
	{
		return 0;	// Indicate we processed this message (eat all key msgs).
	}

	virtual LRESULT _OnSysKeyDown(WPARAM wParam, LPARAM lParam) override
	{
		if (wParam == VK_MENU)
		{
			_RedrawDirectly();	// Redraw window when ALT key up/down changes
		}
		return 0;	// Indicate we processed this message (eat all key msgs).
	}

	virtual LRESULT _OnSysKeyUp(WPARAM wParam, LPARAM lParam) override
	{
		if (wParam == VK_MENU)
		{
			_RedrawDirectly();	// Redraw window when ALT key up/down changes
		}
		CXeShiftCtrlAltKeyHelper sca;
		if (m_type == LBType::Menu && sca.IsOnlyAltDown())	// Process short-cut keys for menu.
		{
			_ProcessMenuShortcutKey(wParam, lParam);
		}
		return 0;	// Indicate we processed this message (eat all key msgs).
	}

	void _ProcessMenuShortcutKey(WPARAM wParam, LPARAM lParam)
	{
		if (m_menu_shortcut_keys.contains(wParam))
		{
			UINT uID = m_menu_shortcut_keys.at(wParam);
			size_t index = _GetItemIndexFromCommandID(uID);
			const ListBoxExItem* pItem = _GetItemConstPtrAtIndex(index);
			if (pItem && !pItem->m_item_data.m_isItemDisabled)
			{
				if (pItem->m_item_data.m_isSubMenu)
				{
					_NotifyParent(LB_EX_NOTIFY_SUBMENU_SELECTED_BY_KEYBOARD, (int)index);
					// Note - when sub-menu shown - we don't return here until sub-menu finished
				}
				else
				{
					_SetSelectedPosition(index);
					_SetSelEndOk();
				}
			}
		}
	}

	// See: CXeMenu::SetTopLevelMenuNavigationCallback.
	void _ProcessTopLevelMenuNavigation(const TopLevelMenuNavigationParameters& params)
	{
		if (m_topLevelMenuNavigationCallback)
		{
			int nOtherTopLevelMenuIndex = m_topLevelMenuNavigationCallback(params);
			if (nOtherTopLevelMenuIndex >= 0)
			{
				// Keyboard navigation or Mouse flyby over another top level toolbar menu item.
				m_nTopLevelMenuNavigationItemIndex = nOtherTopLevelMenuIndex;

				// Close all sub-menus.
				CXeListBoxExCommon* pMenuToClose = this;
				while (pMenuToClose)
				{
					pMenuToClose->_CloseThisMenu();
					pMenuToClose = pMenuToClose->m_pParentMenu;
				}
			}
		}
	}
#pragma endregion Keyboard

#pragma region Scrolling
protected:
	void _AdjustScrollBars()
	{
		if (!::IsWindow(Hwnd())) { return; }	// Not created (yet).
		if (m_type == LBType::Menu) { return; }		// Menu never has scrollbars
		CRect rcClient = _GetClientRect();
		rcClient.bottom -= m_buttonsHlpr.GetButtonsCY();
		CRect rcCopy(rcClient);
		size_t itemsInViewCount = rcCopy.Height() / (int)(m_cyItem + 0.5f);
		m_hasVscroll = m_items.size() > itemsInViewCount;
		if (m_hasVscroll)
		{
			rcCopy.right -= m_cxSysSbV;
		}
		m_hasHscroll = (int)m_cxMaxStringWidth > rcCopy.Width();
		if (m_hasHscroll)
		{
			rcCopy.bottom -= m_cySysSbH;
			itemsInViewCount = rcCopy.Height() / (int)(m_cyItem + 0.5f);
			if (!m_hasVscroll)
			{
				m_hasVscroll = m_items.size() > itemsInViewCount;
			}
		}

		_AdjustVscroll(rcClient, (int)itemsInViewCount);
		_AdjustHscroll(rcClient, rcCopy.Width());
	}

	void _AdjustVscroll(const CRect& rcClient, int nPage)
	{
		if (m_hasVscroll)
		{
			m_rcVSB = rcClient;
			m_rcVSB.bottom = m_hasHscroll ? rcClient.bottom - m_cySysSbH : rcClient.bottom;
			m_rcVSB.left = m_rcVSB.right - m_cxSysSbV;
			if (m_control_has_border)
			{
				++m_rcVSB.top;
				--m_rcVSB.bottom;
				--m_rcVSB.left;
				--m_rcVSB.right;
			}
			if (m_vScollBar)
			{
				if (m_vScollBar->GetSafeHwnd())
				{
					m_vScollBar->ShowWindow(SW_SHOW);
					m_vScollBar->SetWindowPos(nullptr, m_rcVSB.left, m_rcVSB.top, m_rcVSB.Width(), m_rcVSB.Height(),
						SWP_NOCOPYBITS | SWP_NOZORDER);
				}
			}
			else
			{
				m_vScollBar = std::make_unique<CXeScrollBar>(m_xeUI);
				if (m_notifyComboBoxCallback)
				{
					m_vScollBar->m_isThisScrollBarInComboBoxDropDown = true;
				}
				m_vScollBar->Create(WS_CHILD | WS_VISIBLE | SBS_VERT, m_rcVSB, Hwnd(), c_VSB_ID, nullptr);
			}
			SCROLLINFO si{ 0 };
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = (int)m_items.size() - 1;
			si.nPage = nPage;
			si.nPos = (int)m_topIdx;
			si.nTrackPos = 0;
			m_vScollBar->SetScrollInfo(&si, TRUE);
		}
		else
		{
			if (m_vScollBar && m_vScollBar->GetSafeHwnd())
			{
				m_vScollBar->ShowWindow(SW_HIDE);
			}
		}
	}

	SCROLLINFO _GetVscrollInfo() const
	{
		SCROLLINFO si{ 0 };
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nPage = 1;
		if (m_vScollBar && m_hasVscroll)
		{
			m_vScollBar->GetScrollInfo(&si);
		}
		return si;
	}

	// WM_VSCROLL
	virtual LRESULT _OnVScroll(WPARAM wParam, LPARAM lParam) override
	{
		UINT nSBCode = LOWORD(wParam), nPos = HIWORD(wParam);
		HWND hWndSB = (HWND)lParam;
		UINT uSB_ID = ::GetDlgCtrlID(hWndSB);
		if (m_vScollBar && uSB_ID == m_vScollBar->GetDlgCtrlID())	// our scrollbar?
		{
			SCROLLINFO si = _GetVscrollInfo();

			size_t topIdx = 0xFFFFFFFF;
			switch (nSBCode)
			{
			case SB_LINELEFT:
				topIdx = m_topIdx > 0 ? m_topIdx - 1 : 0;
				break;

			case SB_LINERIGHT:
				topIdx = m_topIdx + 1;
				break;

			case SB_PAGELEFT:
				topIdx = m_topIdx >= si.nPage ? m_topIdx - si.nPage : 0;
				break;

			case SB_PAGERIGHT:
				topIdx = m_topIdx + si.nPage;
				break;

			case SB_THUMBTRACK:
				topIdx = si.nTrackPos;
				break;
			}
			if (topIdx < m_items.size())
			{
				size_t topIdxMax = m_items.size() - si.nPage;
				m_topIdx = topIdx > topIdxMax ? topIdxMax : topIdx;
				m_vScollBar->SetScrollPos((int)m_topIdx, TRUE);
				_RedrawDirectly();
			}
		}
		return 0;
	}

	void _AdjustHscroll(const CRect& rcClient, int nPage)
	{
		if (m_hasHscroll)
		{
			m_rcHSB = rcClient;
			m_rcHSB.top = m_rcHSB.bottom - m_cySysSbH;
			m_rcHSB.right = m_hasVscroll ? rcClient.right - m_cxSysSbV : rcClient.right;
			if (m_control_has_border)
			{
				--m_rcHSB.top;
				--m_rcHSB.bottom;
				++m_rcHSB.left;
				--m_rcHSB.right;
			}
			if (m_hScollBar)
			{
				if (m_hScollBar->GetSafeHwnd())
				{
					m_hScollBar->ShowWindow(SW_SHOW);
					m_hScollBar->SetWindowPos(nullptr, m_rcHSB.left, m_rcHSB.top, m_rcHSB.Width(), m_rcHSB.Height(),
						SWP_NOCOPYBITS | SWP_NOZORDER);
				}
			}
			else
			{
				m_hScollBar = std::make_unique<CXeScrollBar>(m_xeUI);
				if (m_notifyComboBoxCallback)
				{
					m_hScollBar->m_isThisScrollBarInComboBoxDropDown = true;
				}
				m_hScollBar->Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, m_rcHSB, Hwnd(), c_HSB_ID, nullptr);
			}
			SCROLLINFO si{ 0 };
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = (int)m_cxMaxStringWidth;
			si.nPage = nPage;
			si.nPos = (int)0;
			si.nTrackPos = 0;
			m_hScollBar->SetScrollInfo(&si, TRUE);
		}
		else
		{
			if (m_hScollBar && m_hScollBar->GetSafeHwnd())
			{
				m_hScollBar->ShowWindow(SW_HIDE);
			}
		}
	}

	SCROLLINFO _GetHscrollInfo() const
	{
		SCROLLINFO si{ 0 };
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nPage = 1;
		if (m_hScollBar && m_hasHscroll)
		{
			m_hScollBar->GetScrollInfo(&si);
		}
		return si;
	}

	// WM_HSCROLL
	virtual LRESULT _OnHScroll(WPARAM wParam, LPARAM lParam) override
	{
		UINT nSBCode = LOWORD(wParam), nPos = HIWORD(wParam);
		HWND hWndSB = (HWND)lParam;
		UINT uSB_ID = ::GetDlgCtrlID(hWndSB);
		if (m_hScollBar && uSB_ID == m_hScollBar->GetDlgCtrlID())	// our scrollbar?
		{
			SCROLLINFO si = _GetHscrollInfo();

			int newX = si.nPos;
			switch (nSBCode)
			{
			case SB_LINELEFT:
				newX -= 10;
				break;

			case SB_LINERIGHT:
				newX += 10;
				break;

			case SB_PAGELEFT:
				newX -= si.nPage;
				break;

			case SB_PAGERIGHT:
				newX += si.nPage;
				break;

			case SB_THUMBTRACK:
				newX = si.nTrackPos;
				break;
			}
			int maxX = (int)m_cxMaxStringWidth - (int)si.nPage;
			if (newX < 0)
			{
				newX = 0;
			}
			else if (newX > maxX)
			{
				newX = maxX;
			}
			m_hScollBar->SetScrollPos(newX, TRUE);
			_RedrawDirectly();
		}
		return 0;
	}
#pragma endregion Scrolling

#pragma region ListBox_message_handlers
public:
	LRESULT OnResetContentMsg(WPARAM wParam, LPARAM lParam)
	{
		m_curIdx = 0xFFFFFFFF;
		m_topIdx = 0;
		m_items.clear();
		_AdjustScrollBars();
		_RedrawDirectly();
		return 0;
	}

	LRESULT OnGetCountMsg(WPARAM wParam, LPARAM lParam) const
	{
		return m_items.size();
	}

	LRESULT OnGetCurSelMsg(WPARAM wParam, LPARAM lParam) const
	{
		if (m_curIdx < m_items.size())
		{
			return m_curIdx;
		}
		return LB_ERR;
	}

	LRESULT OnSetCurSelMsg(WPARAM wParam, LPARAM lParam)
	{
		size_t curSel = wParam;
		if (_SetSelectedPosition(curSel, false))
		{
			return 0;
		}
		m_curIdx = 0xFFFFFFFF;	// No item is select.
		return 0;
	}

	/* Gets the index of the first visible item in a list box.
	Initially the item with index 0 is at the top of the list box,
	but if the list box contents have been scrolled another item may be at the top.
	The first visible item in a multiple-column list box is the top-left item. */
	LRESULT OnGetTopIdxMsg(WPARAM wParam, LPARAM lParam) const
	{
		return m_topIdx;
	}

	/* LB_ADDSTRING message
	Adds a string to a list box. If the list box does not have the LBS_SORT style, the string
	is added to the end of the list. Otherwise, the string is inserted into the list and the
	list is sorted.

	Parameters
	wParam=This parameter is not used.
	lParam=A pointer to the null-terminated string that is to be added.
	If the list box has an owner-drawn style but not the LBS_HASSTRINGS style, this parameter
	is stored as item data instead of a string. You can send the LB_GETITEMDATA and
	LB_SETITEMDATA messages to retrieve or modify the item data.

	Return the zero-based index of the string in the list box. If an error occurs, the return
	value is LB_ERR. If there is insufficient space to store the new string, the return value
	is LB_ERRSPACE.	*/
	LRESULT OnAddStringMsg(WPARAM wParam, LPARAM lParam)
	{
		const wchar_t* pStr = (const wchar_t*)lParam;
		ListBoxExItem new_item(pStr);
		size_t str_idx = LB_ERR;
		bool isInsertAtEnd = false;
		if (m_style.WS().hasLBS_SORT() && m_items.size())
		{
			str_idx = 0;
			auto it = m_items.begin();
			for (; it < m_items.end(); ++it)
			{
				if (new_item < *it)
				{
					it = m_items.insert(it, new_item);
					break;
				}
				++str_idx;
			}
			isInsertAtEnd = it == m_items.end();
		}
		else
		{
			isInsertAtEnd = true;
		}
		if (isInsertAtEnd)
		{
			m_items.push_back(new_item);
			str_idx = m_items.size() - 1;
		}
		_Update_cx_max(m_items.size() - 1);
		_AdjustScrollBars();
		_RedrawDirectly();
		return str_idx;
	}

	/* wParam=The zero-based index of the string to be deleted.
	lParam=This parameter is not used.
	Return value - The return value is a count of the strings remaining in the list.
	The return value is LB_ERR if the wParam parameter specifies an index greater than the number
	of items in the list. */
	LRESULT OnDeleteStringMsg(WPARAM wParam, LPARAM lParam)
	{
		size_t idx = wParam;
		if (idx < m_items.size())
		{
			m_items.erase(m_items.begin() + idx);
			_Update_cx_max(idx);
			_AdjustScrollBars();
			_RedrawDirectly();
			return m_items.size();
		}
		return LB_ERR;
	}

	/* LB_INSERTSTRING
	wParam=The zero-based index of the position at which to insert the string.
	If this parameter is -1, the string is added to the end of the list.
	lParam=A pointer to the null-terminated string to be inserted. If the list box has an
	owner-drawn style but not the LBS_HASSTRINGS style, this parameter is stored as item data
	instead of a string. You can send the LB_GETITEMDATA and LB_SETITEMDATA messages to retrieve
	or modify the item data.
	Return value - The return value is the index of the position at which the string was inserted.
	If an error occurs, the return value is LB_ERR. If there is insufficient space to store the
	new string, the return value is LB_ERRSPACE. */
	LRESULT OnInsertStringMsg(WPARAM wParam, LPARAM lParam)
	{
		const wchar_t* pStr = (const wchar_t*)lParam;
		ListBoxExItem item(pStr);
		size_t idx = wParam;
		if (idx < m_items.size())
		{
			m_items.insert(m_items.begin() + idx, item);
			if (m_curIdx >= idx)
			{
				++m_curIdx;
			}
			_Update_cx_max(idx);
		}
		else
		{
			m_items.push_back(item);
			_Update_cx_max(0xFFFFFFFF);
			idx = m_items.size() - 1;
		}
		_AdjustScrollBars();
		_RedrawDirectly();
		return idx;
	}

	LRESULT OnSetItemDataMsg(WPARAM wParam, LPARAM lParam)
	{
		size_t idx = wParam;
		if (idx < m_items.size())
		{
			m_items[idx].m_item_data.m_i64 = lParam;
		}
		_RedrawDirectly();
		return 0;
	}

	/* wParam=item idx.
	The return value is the value associated with the item, or LB_ERR if an error occurs.
	If the item is in an owner-drawn list box and was created without the LBS_HASSTRINGS style,
	this value was in the lParam parameter of the LB_ADDSTRING or LB_INSERTSTRING message that
	added the item to the list box.
	Otherwise, it is the value in the lParam of the LB_SETITEMDATA message. */
	LRESULT OnGetItemDataMsg(WPARAM wParam, LPARAM lParam) const
	{
		size_t idx = wParam;
		if (idx < m_items.size())
		{
			return m_items[idx].m_item_data.m_i64;
		}
		return LB_ERR;
	}

	/* wParam=item.
	If an item is selected, the return value is greater than zero; otherwise, it is zero.
	If an error occurs, the return value is LB_ERR.*/
	LRESULT OnGetSelMsg(WPARAM wParam, LPARAM lParam) const
	{
		size_t idx = wParam;
		return idx == m_curIdx ? 1 : 0;
	}

	/* The return value is the count of selected items in the list box. If the list box is a
	single-selection list box, the return value is LB_ERR. */
	LRESULT OnGetSelCountMsg(WPARAM wParam, LPARAM lParam) const
	{
		return LB_ERR;
	}

	/* wParam=item idx.
	lParam=A pointer to the buffer that will receive the string; it is type LPTSTR which is
	subsequently cast to an LPARAM. The buffer must have sufficient space for the string and
	a terminating null character. An LB_GETTEXTLEN message can be sent before the LB_GETTEXT
	message to retrieve the length, in TCHARs, of the string.
	Return value - The return value is the length of the string, in TCHARs, excluding the
	terminating null character.
	If wParam does not specify a valid index, the return value is LB_ERR.*/
	LRESULT OnGetTextMsg(WPARAM wParam, LPARAM lParam) const
	{
		size_t idx = wParam;
		if (idx < m_items.size())
		{
			size_t str_len = m_items[idx].m_string.size();
			size_t str_mem_len = str_len * sizeof(wchar_t);
			wchar_t* pBuf = (wchar_t*)lParam;
			std::memcpy(pBuf, m_items[idx].m_string.c_str(), str_mem_len);
			pBuf[str_len] = 0;
			return str_len;
		}
		return LB_ERR;
	}

	/* wParam=item idx. If the list box has an owner-drawn style, but not the LBS_HASSTRINGS style,
	the return value is always the size, in bytes, of a DWORD. */
	LRESULT OnGetTextLenMsg(WPARAM wParam, LPARAM lParam) const
	{
		size_t idx = wParam;
		if (idx < m_items.size())
		{
			return m_items[idx].m_string.size();
		}
		return LB_ERR;
	}

	/* wParam=This parameter is not used.
	lParam=The LOWORD specifies the x-coordinate of a point, relative to the upper-left corner of
	the client area of the list box. The HIWORD specifies the y-coordinate of a point, relative to
	the upper-left corner of the client area of the list box.
	Return value - The return value contains the index of the nearest item in the LOWORD.
	The HIWORD is zero if the specified point is in the client area of the list box, or one if
	it is outside the client area. */
	LRESULT OnItemFromPointMsg(WPARAM wParam, LPARAM lParam) const
	{
		XeASSERT(FALSE);	// Not supported in this implementation of a listbox.
		return 0;
	}

	/* wParam=The zero-based index of the item before the first item to be searched. When the search
	reaches the bottom of the list box, it continues from the top of the list box back to the item
	specified by the wParam parameter. If wParam is -1, the entire list box is searched from the
	beginning.
	lParam=A pointer to the null-terminated string that contains the prefix for which to search.
	The search is case independent, so this string can contain any combination of uppercase and
	lowercase letters.
	Return value - If the search is successful, the return value is the index of the selected item.
	If the search is unsuccessful, the return value is LB_ERR and the current selection is not changed.
	Remarks - The list box is scrolled, if necessary, to bring the selected item into view.
	Do not use this message with a list box that has the LBS_MULTIPLESEL or the LBS_EXTENDEDSEL styles.
	An item is selected only if its initial characters from the starting point match the characters in
	the string specified by the lParam parameter. If the list box has the owner-drawn style but not the
	LBS_HASSTRINGS style, the action taken by LB_SELECTSTRING depends on whether the LBS_SORT style is
	used. If LBS_SORT is used, the system sends WM_COMPAREITEM messages to the list box owner to
	determine which item matches the specified string. Otherwise, LB_SELECTSTRING attempts to find an
	item that has a long value (supplied as the lParam parameter of the LB_ADDSTRING or LB_INSERTSTRING
	message) that matches the lParam parameter. */
	LRESULT OnSelectStringMsg(WPARAM wParam, LPARAM lParam)
	{
		XeASSERT(FALSE);	// Not supported in this implementation of a listbox.
		return 0;
	}

	/* Selects an item in a multiple-selection list box and, if necessary, scrolls the item into view.
	Parameters:
	wParam=Specifies how to set the selection. If this parameter is TRUE, the item is selected and
	highlighted; if it is FALSE, the highlight is removed and the item is no longer selected.
	lParam=Specifies the zero-based index of the item to set. If this parameter is -1, the selection
	is added to or removed from all items, depending on the value of wParam, and no scrolling occurs.
	Return value - If an error occurs, the return value is LB_ERR.
	Remarks - Use this message only with multiple-selection list boxes. */
	LRESULT OnSetSelMsg(WPARAM wParam, LPARAM lParam)
	{
		XeASSERT(FALSE);	// Not supported in this implementation of a listbox.
		return 0;
	}

	/* Ensures that the specified item in a list box is visible.
	Parameters:
	wParam=The zero-based index of the item in the list box.
	lParam=This parameter is not used.
	Return value - If an error occurs, the return value is LB_ERR.
	Remarks - The system scrolls the list box contents so that either the specified item appears at
	the top of the list box or the maximum scroll range has been reached. */
	LRESULT OnSetTopIndexMsg(WPARAM wParam, LPARAM lParam)
	{
		XeASSERT(FALSE);	// Not supported in this implementation of a listbox.
		return 0;
	}

	/* Finds the first string in a list box that begins with the specified string.
	Parameters:
	wParam=The zero-based index of the item before the first item to be searched. When the search
	reaches the bottom of the list box, it continues searching from the top of the list box back
	to the item specified by the wParam parameter. If wParam is -1, the entire list box is searched
	from the beginning.
	lParam=A pointer to the null-terminated string that contains the string for which to search.
	The search is case independent, so this string can contain any combination of uppercase and
	lowercase letters.
	Return value - The return value is the index of the matching item, or LB_ERR if the search was
	unsuccessful.
	Remarks - If the list box has the owner-drawn style but not the LBS_HASSTRINGS style, the action
	taken by LB_FINDSTRING depends on whether the LBS_SORT style is used. If LBS_SORT is used, the
	system sends WM_COMPAREITEM messages to the list box owner to determine which item matches the
	specified string. Otherwise, LB_FINDSTRING attempts to find an item that has a long value
	(supplied as the lParam parameter of the LB_ADDSTRING or LB_INSERTSTRING message) that matches
	the lParam parameter. */
	LRESULT OnFindStringMsg(WPARAM wParam, LPARAM lParam) const
	{
		XeASSERT(FALSE);	// Not supported in this implementation of a listbox.
		return 0;
	}

	/* Finds the first list box string that exactly matches the specified string, except that the
	search is not case sensitive.
	Parameters:
	wParam=The zero-based index of the item before the first item to be searched. When the search
	reaches the bottom of the list box, it continues searching from the top of the list box back
	to the item specified by the wParam parameter. If wParam is -1, the entire list box is searched
	from the beginning.
	lParam=A pointer to the null-terminated string for which to search. The search is not case
	sensitive, so this string can contain any combination of uppercase and lowercase letters.
	Return value - The return value is the zero-based index of the matching item, or LB_ERR if the
	search was unsuccessful.
	Remarks - This function is only successful if the specified string and a list box item have the
	same length (except for the null at the end of the specified string) and have exactly the same
	characters. If the list box has the owner-drawn style but not the LBS_HASSTRINGS style, the action
	taken by LB_FINDSTRINGEXACT depends on whether the LBS_SORT style is used. If LBS_SORT is used,
	the system sends WM_COMPAREITEM messages to the list box owner to determine which item matches
	the specified string. Otherwise, LB_FINDSTRINGEXACT attempts to find an item that has a long value
	(supplied as the lParam parameter of the LB_ADDSTRING or LB_INSERTSTRING message) that matches the
	lParam parameter. */
	LRESULT OnFindStringExactMsg(WPARAM wParam, LPARAM lParam) const
	{
		std::wstring s((const wchar_t*)lParam);
		size_t num_items = m_items.size();
		for (size_t i = 0; i < num_items; ++i)
		{
			if (m_items[i].m_string == s)
			{
				return i;
			}
		}
		return LB_ERR;
	}

	LRESULT OnNotSupportedMsg(WPARAM wParam, LPARAM lParam)
	{
		XeASSERT(FALSE);	// Not supported in this implementation of a listbox.
		return 0;
	}
#pragma endregion ListBox_message_handlers
};

