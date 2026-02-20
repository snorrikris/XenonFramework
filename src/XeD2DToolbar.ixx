module;

#include "os_minimal.h"
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <boost/algorithm/string.hpp>
#include "XeResource.h"

export module Xe.D2DToolbar;

import Xe.D2DToolbarItems;
import Xe.D2DToolbarIF;
import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.D2DWndBase;
import Xe.Menu;
import Xe.ListBoxExCommon;
import Xe.ComboBoxEx;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

constexpr wchar_t XED2DTOOLBARWND_CLASSNAME[] = L"XeD2DToolbarWndClass";  // Window class name

// Note - if menu is used:
//		The 'owning' app MUST call SetUpdateMenuCallback to have each sub-menu item updated that
//		needs a checkmark or enabled state changed.

// Note - to remove 'main' menu from an application:
//		Override Create in CMainFrame, call base class with 0 as menu name.

export constexpr UINT WMU_NOTIFY_ID_APP_EXIT = WM_USER + 13000;

constexpr UINT WMU_SHOW_TOP_LEVEL_MENU_ITEM = WM_USER + 13001;

export class CXeD2DToolbar : public CXeD2DWndBase, public CXeD2DToolbarIF
{
protected:
	bool m_isToolBarInMainFrame = false;

	int m_cyItemsHeight = 24, m_cyTopMargin = 2, m_cyBottomMargin = 0;

	std::vector<std::unique_ptr<XETBItem>> m_itemlist;

	std::unique_ptr<CXeMenu> m_menu;
	int m_menu_first_index = -1, m_menu_item_count = 0;
	int m_nCurrentlyShowingTopLevelMenuItem = -1;
	bool m_isShowingSubMenu = false;

	int m_nHotItemIdx = -1;			// Index of item where mouse is hovering. (-1 if none).

	int m_nLbtnDownItemIdx = -1;	// Index of item when L button down.

	CPoint m_ptLbtnDown;	// point where L button down (only do btn up event when +/-5px)

	int m_xLastTBitem = 0;	// Calculated X pos. to the right of the last toolbar item (incl. margin).

#pragma region Creation
public:
	CXeD2DToolbar(CXeUIcolorsIF* pUIcolors) : CXeD2DWndBase(pUIcolors)
	{
		m_xeUI->RegisterWindowClass(XED2DTOOLBARWND_CLASSNAME, D2DCtrl_WndProc, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW);
	}
	//virtual ~CXeD2DToolbar() {}

	// Create CXeToolBarWnd.
	// uCtrlId = Dialog control ID for 'this' window.
	// nPresetHeight = preset height of tool bar in pixels.
	// uIDMenu = Resource ID of menu to put in toolbar - usually IDR_MAINFRAME.
	//           Note - if menu items added a separator is added as a last item.
	// isToolBarInMainFrame = true if 'this' toolbar is in the main frame.
	bool Create(HWND hParentWnd, const CRect& rect, UINT uCtrlId,
			UINT uIDMenu = 0, bool isToolBarInMainFrame = false)
	{
		m_isToolBarInMainFrame = isToolBarInMainFrame;

		DWORD dwExStyle = 0;
		DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		HWND hWnd = CreateD2DWindow(dwExStyle, XED2DTOOLBARWND_CLASSNAME, nullptr, dwStyle,
				rect, hParentWnd, uCtrlId, true);
		// Add menu items (menubar) to list of toolbar items
		if (uIDMenu)
		{
			m_menu = std::make_unique<CXeMenu>(m_xeUI, MAKEINTRESOURCE(uIDMenu));
			m_menu_first_index = (int)m_itemlist.size();
			m_menu_item_count = (int)m_menu->GetTopLevelMenuItemCount();
			for (int i = 0; i < (int)m_menu->GetTopLevelMenuItemCount(); i++)
			{
				const CXeSubMenu* pDDmenu = m_menu->GetTopLevelMenuItemConst(i);
				std::wstring s = pDDmenu->m_text;
				std::wstring tt(s + L" menu");
				boost::erase_all(tt, "&");
				m_itemlist.push_back(std::make_unique<XETBItem_Menu>(m_xeUI, hWnd, i /* index into menu = uID */,
						s.c_str(), tt.c_str(), _GetNextXpos(), m_cyTopMargin, m_cyItemsHeight));	
			}
			AddSeparator();
		}

		return hWnd != 0;
	}

	virtual LRESULT _OnDestroy() override
	{
		for (auto& item : m_itemlist)
		{
			item->OnDestroy();
		}
		return CXeD2DWndBase::_OnDestroy();
	}

	virtual void SetUpdateMenuCallback(UpdateMenuCallbackFunc updateMenuCallback) override
	{
		XeASSERT(m_menu);
		if (m_menu)
		{
			m_menu->SetUpdateMenuCallback(updateMenuCallback);
		}
	}
#pragma endregion Creation

#pragma region CalculateUIsize
public:
	CSize GetToolBarSize() const
	{
		return CSize(m_xLastTBitem, m_cyItemsHeight + m_cyTopMargin + m_cyBottomMargin);
	}

	void RecalculateItems()
	{
		m_cyItemsHeight = 24;
		int x = 2;
		for (auto& item : m_itemlist)
		{
			x = item->CalculateItemRect(x, m_cyTopMargin, m_cyItemsHeight);
		}
		for (auto& item : m_itemlist)
		{
			m_cyItemsHeight = std::max((int)(item->m_rItem.Height()), m_cyItemsHeight);
		}
		x = 2;
		for (auto& item : m_itemlist)
		{
			x = item->CalculateItemRect(x, m_cyTopMargin, m_cyItemsHeight);

			item->MoveWindowToCalculatedPosition();
		}
		m_xLastTBitem = x;	// Save toolbar 'last' X pos.
	}
	
	int _GetNextXpos() const
	{
		return m_itemlist.size() ? m_itemlist.back()->GetNextControlXpos() : 2;
	}
#pragma endregion CalculateUIsize

#pragma region Add_Controls
public:
	// Add a 'Standard' XePngButton to toolbar.
	void AddStdButton(const wchar_t* szText, UINT uIDcommand, const wchar_t* szTooltip)
	{
		m_itemlist.push_back(std::make_unique<XETBItem_Button>(m_xeUI, Hwnd(), XETBItemType::STD_BUTTON, uIDcommand, szText,
				szTooltip, _GetNextXpos(), m_cyTopMargin, m_cyItemsHeight));
	}

	// Add a CheckBox to toolbar.
	void AddCheckBox(const wchar_t* szText, UINT uIDcommand, const wchar_t* szTooltip)
	{
		m_itemlist.push_back(std::make_unique<XETBItem_Button>(m_xeUI, Hwnd(), XETBItemType::CHECKBOX, uIDcommand, szText, szTooltip,
				_GetNextXpos(), m_cyTopMargin, m_cyItemsHeight));
	}

	// Add "PNG" Icon button.
	// szPngName = "PNG" resource name of image - MUST be 24x24pixels, RGBA.
	// Note - 'Hot' and 'Disabled' images are generated automatically.
	void AddPngIconButton(PID uPngImgId, UINT uIDcommand, const wchar_t* szTooltip)
	{
		m_itemlist.push_back(std::make_unique<XETBItem_Button>(m_xeUI, Hwnd(), XETBItemType::PNG_ICON_BUTTON, uIDcommand,
			uPngImgId, szTooltip, _GetNextXpos(), m_cyTopMargin, m_cyItemsHeight));
	}

	// Add narrow "PNG" image button.
	// szPngName = "PNG" resource name of image - MUST be 24x24pixels, RGBA.
	// Note - 'Hot' and 'Disabled' images are generated automatically.
	void AddPngNrwButton(PID uPngImgId, UINT uIDcommand, const wchar_t* szTooltip)
	{
		m_itemlist.push_back(std::make_unique<XETBItem_Button>(m_xeUI, Hwnd(), XETBItemType::PNG_NRW_BUTTON, uIDcommand,
				uPngImgId, szTooltip, _GetNextXpos(), m_cyTopMargin, m_cyItemsHeight));
	}

	void AddDropListPngButton(PID uPngImgId, UINT uIDcommand, const wchar_t* szTooltip)
	{
		std::unique_ptr<XETBItem_DropButton> item
				= std::make_unique<XETBItem_DropButton>(m_xeUI, Hwnd(), uIDcommand,
				uPngImgId, szTooltip, _GetNextXpos(), m_cyTopMargin, m_cyItemsHeight);
		m_itemlist.push_back(std::move(item));
	}

	void AddComboBox(CSize size, int cyDropDown, UINT uIDcommand, const wchar_t* szTooltip,
		DWORD dwStyle = (WS_BORDER | WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DROPDOWN))
	{
		m_itemlist.push_back(std::make_unique<XETBItem_ComboBox>(m_xeUI, Hwnd(), dwStyle, uIDcommand, szTooltip, size, 
				cyDropDown, _GetNextXpos(), m_cyTopMargin, m_cyItemsHeight));
	}

	// Add a Label to toolbar.
	void AddLabel(const wchar_t* szText, bool isBorder, UINT uIDcommand, const wchar_t* szTooltip)
	{
		m_itemlist.push_back(std::make_unique<XETBItem_Label>(m_xeUI, Hwnd(), uIDcommand, szText, szTooltip, isBorder,
				_GetNextXpos(), m_cyTopMargin, m_cyItemsHeight));
	}

	void AddSeparator()
	{
		m_itemlist.push_back(std::make_unique<XETBItem_Separator>(m_xeUI, _GetNextXpos(), m_cyTopMargin, m_cyItemsHeight));
	}
#pragma endregion Add_Controls

#pragma region Manipulate_Controls
	PID GetPngIconBtnImageId(UINT uIDcommand)
	{
		XETBItem* pItem = _GetItemFromCmdID(uIDcommand);
		XeASSERT(pItem && (pItem->m_itemType == XETBItemType::PNG_ICON_BUTTON || pItem->m_itemType == XETBItemType::PNG_NRW_BUTTON
				|| pItem->m_itemType == XETBItemType::DROP_LIST_BTN));
		return pItem ? pItem->GetBtnImageId() : PID::None;
	}

	void ChangePngIconBtnImage(PID uPngImgId, UINT uIDcommand)
	{
		XETBItem* pItem = _GetItemFromCmdID(uIDcommand);
		XeASSERT(pItem && pItem->m_itemType == XETBItemType::PNG_ICON_BUTTON);
		if (pItem)
		{
			pItem->SetBtnImageId(uPngImgId);
			//_RedrawToolBar();
		}
	}

	virtual void SetItemEnabledState(UINT uID, bool fEnabled) override
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		if (pItem && pItem->SetEnabledState(fEnabled))
		{
			_RedrawToolBar();
		}
	}

	void SetItemsEnabledState(std::vector<UINT> ctrlsIds, BOOL fEnabled)
	{
		bool isChanged = false;
		for (UINT uId : ctrlsIds)
		{
			XETBItem* pItem = _GetItemFromCmdID(uId);
			if (pItem && pItem->SetEnabledState(fEnabled))
			{
				isChanged = true;
			}
		}
		if (isChanged)
		{
			_RedrawToolBar();
		}
	}

	virtual void SetItemVisibleState(UINT uID, bool visible) override
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		if (pItem)
		{
			pItem->SetVisibleState(visible);
			RecalculateItems();
			_RedrawToolBar();
		}
	}

	// Set bState member of item.
	virtual void SetItemFlags(UINT uID, uint8_t flags) override
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		if (pItem)
		{
			pItem->SetFlags(flags);
			_RedrawToolBar(); // Redraw entire toolbar when flags of item changes - exclude child windows.
		}
	}

	virtual uint8_t GetItemFlags(UINT uID) override
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		if (pItem)
		{
			return pItem->GetFlags();
		}
		return 0;
	}

	void SetCheckBoxState(UINT uID, int nCheck)
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		XETBItem_Button* pCB = dynamic_cast<XETBItem_Button*>(pItem);
		if (pCB)
		{
			pCB->SetCheckBoxState(nCheck);
		}
	}

	int GetCheckBoxState(UINT uID)
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		XETBItem_Button* pCB = dynamic_cast<XETBItem_Button*>(pItem);
		if (pCB)
		{
			return pCB->GetCheckBoxState();
		}
		XeASSERT(FALSE);
		return 0;
	}

	void SetLabelString(UINT uID, const wchar_t* pszText)
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		if (pItem && (pItem->m_itemType == XETBItemType::LABEL || pItem->m_itemType == XETBItemType::LABEL_BORDER))
		{
			pItem->m_sText = pszText;
			_RedrawToolBar();
		}
	}

	// Get item rect in screen coords.
	virtual CRect GetItemRect(UINT uID) override
	{
		CRect rcItem;
		XETBItem* pItem = _GetItemFromCmdID(uID);
		if (pItem)
		{
			rcItem = pItem->m_rItem;
			::ClientToScreen(Hwnd(), (LPPOINT)&rcItem);
			::ClientToScreen(Hwnd(), ((LPPOINT)&rcItem) + 1);
		}
		return rcItem;
	}

	// Get item rect in client coords.
	CRect GetItemRectClient(UINT uID)
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		if (pItem)
		{
			return pItem->m_rItem;
		}
		return CRect();
	}

	void SetItemRmargin(UINT uID, int cx)
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		if (pItem)
		{
			pItem->m_cxRmargin = cx;
			RecalculateItems();
			_RedrawToolBar();
		}
	}

	void SetTooltip(UINT uID, const wchar_t* szToolTip, int cxTooltip = 0)
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		XeASSERT(pItem);
		if (pItem)
		{
			pItem->m_sToolTip = szToolTip;
			pItem->m_cxTooltip = cxTooltip;
		}
	}

	void SetItemRefCalcText(UINT uID, const wchar_t* szTxt)
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		XeASSERT(pItem);
		if (pItem)
		{
			pItem->m_sRefCalcText = szTxt;
		}
	}

	CXeComboBoxEx* GetComboBox(UINT uID)
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		if (pItem && pItem->m_itemType == XETBItemType::COMBOBOX)
		{
			XETBItem_ComboBox* pCBitem = dynamic_cast<XETBItem_ComboBox*>(pItem);
			XeASSERT(pCBitem->m_combobox_object);
			if (pCBitem->m_combobox_object)
			{
				return pCBitem->m_combobox_object.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	void SetComboBoxList(UINT uID, const std::vector<ListBoxExItem>& list, int curSelIdx = -1)
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		XeASSERT(pItem && pItem->m_itemType == XETBItemType::COMBOBOX);
		XETBItem_ComboBox* pCBitem = dynamic_cast<XETBItem_ComboBox*>(pItem);
		if (pCBitem)
		{
			pCBitem->SetComboBoxList(list, curSelIdx);
		}
	}

	void SetDropListButtonList(UINT uID, const std::vector<ListBoxExItem>& list, int curSelIdx = -1)
	{
		CXeTB_DropListPopup* p = _GetDropListPtr(uID);
		if (p)
		{
			p->m_cmn->SetItemsList(list);
			p->m_cmn->OnSetCurSelMsg((WPARAM)curSelIdx, 0);
		}
	}

	void SetDropListButtonCurSel(UINT uID, int curSelIdx)
	{
		CXeTB_DropListPopup* p = _GetDropListPtr(uID);
		if (p)
		{
			p->m_cmn->OnSetCurSelMsg((WPARAM)curSelIdx, 0);
		}
	}

	virtual std::optional<std::wstring> GetDropListButtonCurSel(UINT uID) override
	{
		CXeTB_DropListPopup* p = _GetDropListPtr(uID);
		if (p)
		{
			return p->m_lastUserSelectedItem;
		}
		return std::nullopt;
	}

	virtual HWND GetItemHwnd(UINT uID) override
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		return pItem ? pItem->GetControlHwnd() : 0;
	}

	virtual HWND GetToolbarHwnd() override
	{
		return Hwnd();
	}

protected:
	// Send WM_COMMAND to parent frame if pItem is "command" type control.
	// Return true if WM_COMMAND sent else false.
	bool _SendCommand(const XETBItem* pItem)
	{
		WPARAM wParm = MAKELONG(pItem->m_uIDcommand, 0);
		LPARAM lParm = /*pItem->control_object ? (LPARAM)pItem->control_object->GetSafeHwnd() :*/ (LPARAM)GetSafeHwnd();
		if (pItem->m_itemType == XETBItemType::PNG_ICON_BUTTON
			|| pItem->m_itemType == XETBItemType::PNG_NRW_BUTTON
			|| pItem->m_itemType == XETBItemType::LABEL
			|| pItem->m_itemType == XETBItemType::LABEL_BORDER
			|| pItem->m_itemType == XETBItemType::DROP_LIST_BTN)
		{
			if (_OnSendCommand(wParm, lParm))
			{
				::SendMessage(::GetParent(Hwnd()), WM_COMMAND, wParm, lParm);
				return true;
			}
		}
		return false;
	}

	XETBItem* _GetItemFromCmdID(UINT uID)
	{
		for (auto& item : m_itemlist)
		{
			if (item->m_uIDcommand == uID)
			{
				return item.get();
			}
		}
		return nullptr;
	}

	std::wstring _GetComboBoxText(UINT uID)
	{
		XeASSERT(false);
		//CXeComboBoxEx* pCB = GetComboBox(uID);
		//if (pCB)
		//{
		//	return GetWindowTextStdW(pCB->GetSafeHwnd());
		//}
		return std::wstring();
	}

	// Returns index of item if point is within it's rect. -1 returned if no item at point.
	// If ppItem is valid - is set to address of item structure.
	int _FindItemFromPoint(CPoint point, const XETBItem** ppItem = 0) const
	{
		int idx = 0;
		for (const auto& item : m_itemlist)
		{
			if (item->m_rItem.PtInRect(point))
			{
				if (ppItem)
					*ppItem = item.get();
				return idx;
			}
			idx++;
		}
		return -1;
	}

	CXeTB_DropListPopup* _GetDropListPtr(UINT uID)
	{
		XETBItem* pItem = _GetItemFromCmdID(uID);
		XeASSERT(pItem && pItem->m_itemType == XETBItemType::DROP_LIST_BTN);
		XETBItem_DropButton* pDropBtn = dynamic_cast<XETBItem_DropButton*>(pItem);
		if (pDropBtn)
		{
			return pDropBtn->m_droplist_object.get();
		}
		return nullptr;
	}
#pragma endregion Manipulate_Controls

#pragma region UI_Drawing
protected:
	void _RedrawToolBar()
	{
		if (!m_isToolBarInMainFrame)
		{
			_RedrawDirectly();
		}
		else
		{
			// Redraw main frame parent window NC area (to redraw tool bar).
			::SendMessage(::GetParent(Hwnd()), WM_NCPAINT, 0, 0);
		}
	}

	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override
	{
		float cxClient = (rcClient.right - rcClient.left);
		float cyClient = (rcClient.bottom - rcClient.top);
		if (cxClient < 1 || cyClient < 1)
		{
			return;	// Nothing to paint.
		}
		_PaintInternal(this, rcClient);
	}

public:
	/*
	* Note - the _PaintInternal function is called both from 'this' class and from CXeD2DCustomFrameHlp.
	* When called from the frame delper - pRctx is the context of the frame helper - meaning that we should not use any
	* internal (of 'this' class) paint resources.
	*/
	virtual void _PaintInternal(CXeD2DRenderContext* pRctx, D2D1_RECT_F rcClient)
	{
		ID2D1RenderTarget* pRT = pRctx->m_pCurrentRT;
		XeASSERT(pRT);
		// Get L mouse button state
		bool isLbtnDown = ::GetAsyncKeyState(::GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON) & 0x8000;
		bool isAltKeyDown = (::GetAsyncKeyState(VK_MENU) & 0x8000);

		bool isEnabled = ::IsWindowEnabled(Hwnd());
		pRT->FillRectangle(rcClient, pRctx->GetBrush(CID::XTbBarBg));

		int idx = 0;
		for (const auto& item : m_itemlist)
		{
			if (item->m_isVisible)
			{
				bool isLbtnDownItem = isLbtnDown && m_nLbtnDownItemIdx == idx;
				bool isHotItem = idx == m_nHotItemIdx;
				item->_Paint(pRctx, D2D1_POINT_2F(rcClient.left, rcClient.top), isLbtnDownItem, isHotItem);
			}
			idx++;
		}
	}
#pragma endregion UI_Drawing

#pragma region Misc
public:
	virtual void OnChangedSettings(const ChangedSettings& chg_settings) override
	{
		bool isFontsChanged = m_xeUI->IsFontSettingsChanged(chg_settings);
		bool isColorsChanged = chg_settings.IsChanged(L"Colors", L"Color");
		if (isFontsChanged || isColorsChanged)
		{
			for (auto& item : m_itemlist)
			{
				item->OnChangedUIsettings(isFontsChanged, isColorsChanged);
			}
		}
		if (isFontsChanged)
		{
			RecalculateItems();
		}
		if (isColorsChanged)
		{
			_RedrawToolBar();
		}
	}

protected:
	// Called when processing WM_COMMAND sent from child controls.
	// Return true when SendMessage to parent should be done,
	// return false when default processing should be done.
	virtual bool _OnSendCommand(WPARAM wParam, LPARAM lParam)
	{
		XETBItem* pItem = _GetItemFromCmdID(LOWORD(wParam));
		return (pItem && pItem->m_itemType != XETBItemType::COMBOBOX) ? true : false;
	}

#pragma endregion Misc

#pragma region MessageHandlers
protected:
	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		if (uMsg == WMU_SHOW_TOP_LEVEL_MENU_ITEM)
		{
			int nIdx = (int)wParam;
			if (nIdx < m_itemlist.size())
			{
				_ShowTopLevelMenuItem(nIdx);
			}
		}
		//if ((uMsg >= BM_GETCHECK && uMsg <= BM_SETDONTCLICK)
		//	|| (uMsg >= BCM_GETIDEALSIZE && uMsg <= BCM_SETSHIELD))
		//{
		//	return _OnButtonControlMessage(uMsg, wParam, lParam);
		//}
		return CXeD2DWndBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender) override
	{
		const XETBItem* pItem = _GetItemFromCmdID(wID);
		if (pItem)
		{
			if (pItem->m_itemType == XETBItemType::DROP_LIST_BTN)
			{
				_ShowDropListButtonPopup(pItem);
			}
			else
			{
				_SendCommand(pItem);
			}
		}
		return 0;
	}

	virtual LRESULT _OnCancelMode() override
	{
		//m_isDown = m_isDropButtonDown = false;
		return CXeD2DWndBase::_OnCancelMode();
	}

	virtual LRESULT _OnNcHitTest(WPARAM wParam, LPARAM lParam) override
	{
		//if (m_style.IsGroupBox())
		//{
		//	return HTNOWHERE;
		//}
		return CXeD2DWndBase::_OnNcHitTest(wParam, lParam);
	}

	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint pt) override
	{
		if (DoOnLButtonDown(nFlags, pt))
		{
			_RedrawToolBar();
		}
		return 0;
	}

	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint pt) override
	{
		DoOnLButtonUp(nFlags, pt);
		_RedrawToolBar();
		return 0;
	}

	virtual LRESULT _OnMouseActivate(WPARAM wParam, LPARAM lParam) override
	{
		//if (m_style.IsGroupBox())
		//{
		//	return MA_NOACTIVATEANDEAT;
		//}
		return CXeD2DWndBase::_OnMouseActivate(wParam, lParam);
	}

	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{
		if (DoOnMouseMove(nFlags, point))
		{
			_RedrawToolBar();	// Redraw entire toolbar when 'hot' item changes - exclude child windows.
		}
		return CXeD2DWndBase::_OnMouseMove(nFlags, point);
	}

	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override
	{
		m_nHotItemIdx = -1;
		return CXeD2DWndBase::_OnMouseLeave(wparam, lparam);
	}

	virtual LRESULT _OnMouseHover(WPARAM wparam, LPARAM lparam) override
	{
		return CXeD2DWndBase::_OnMouseHover(wparam, lparam);

	}
#pragma endregion MessageHandlers

#pragma region MouseProcessing
public:
	bool DoOnLButtonDown(UINT nFlags, CPoint point)
	{
		m_ptLbtnDown = point;
		bool isRedraw = false;
		const XETBItem* pItem = 0;
		int nIdx = _FindItemFromPoint(point, &pItem);
		if (pItem && pItem->m_itemType != XETBItemType::SEPARATOR)
		{
			//TRACE("LBtn dn on: %d\n", nIdx);
			m_nLbtnDownItemIdx = nIdx;
			isRedraw = true;
		}
		else
		{
			//TRACE("LBtn dn on: %d\n", -1);
			m_nLbtnDownItemIdx = -1;
		}
		return isRedraw;
	}

	void DoOnLButtonUp(UINT nFlags, CPoint point)
	{
		if (std::abs(point.x - m_ptLbtnDown.x) > 5 || std::abs(point.y - m_ptLbtnDown.y) > 5)
		{
			return;	// Ignore button up event if to far away from where L button down.
		}
		const XETBItem* pItem = 0;
		int nIdx = _FindItemFromPoint(point, &pItem);
		//TRACE("LBtn up on: %d (LBtn dn idx: %d)\n", nIdx, m_nLbtnDownItemIdx);
		if (nIdx >= 0 && pItem && pItem->m_isEnabled && nIdx == m_nLbtnDownItemIdx)
		{
			if (pItem->m_itemType == XETBItemType::MENU_ITEM)
			{
				_ShowTopLevelMenuItem(nIdx);
			}
			else if (pItem->m_itemType == XETBItemType::SEPARATOR
				|| pItem->m_itemType == XETBItemType::PNG_BUTTON
				|| pItem->m_itemType == XETBItemType::STD_BUTTON
				|| pItem->m_itemType == XETBItemType::CHECKBOX
				|| pItem->m_itemType == XETBItemType::COMBOBOX)
			{
				nIdx = -1;	// nop
			}
			else if (pItem->m_itemType == XETBItemType::DROP_LIST_BTN)
			{
				_ShowDropListButtonPopup(pItem);
			}
			else
			{
				_SendCommand(pItem);
			}
		}
		m_nLbtnDownItemIdx = -1;
	}

	bool DoOnMouseMove(UINT nFlags, CPoint point)
	{
		const XETBItem* pItem = 0;
		int nHotItemIdx = _FindItemFromPoint(point, &pItem);
		if (pItem
			&& (pItem->m_itemType == XETBItemType::SEPARATOR || (pItem->m_itemType == XETBItemType::LABEL && pItem->m_uIDcommand == 0)
				|| pItem->m_itemType == XETBItemType::PNG_BUTTON || pItem->m_itemType == XETBItemType::STD_BUTTON))
		{
			nHotItemIdx = -1;
		}
		if (m_nHotItemIdx != nHotItemIdx)
		{
			m_nHotItemIdx = nHotItemIdx;

			// Redraw entire toolbar when 'hot' item changes - exclude child windows.
			return true;
		}
		return false;
	}
#pragma endregion MouseProcessing

#pragma region MenuHandling
public:
	bool ProcessAcceleratorKey(WPARAM vk_code)
	{
		if (m_menu)
		{
			size_t index = m_menu->GetTopLevelAccelaratorKeyIndex(vk_code);
			if (index < m_itemlist.size())
			{
				::PostMessage(Hwnd(), WMU_SHOW_TOP_LEVEL_MENU_ITEM, (WPARAM)index, 0);
				_RedrawDirectly();
				return true;
			}
		}
		return false;
	}

protected:
	// WMU_SHOW_TOP_LEVEL_MENU_ITEM message handler.
	LRESULT OnShowTopLevelMenuItem(WPARAM wParam, LPARAM lParam)
	{
		int nIdx = (int)wParam;
		if (nIdx < m_itemlist.size())
		{
			_ShowTopLevelMenuItem(nIdx);
		}
		return 0;
	}

	void _ShowTopLevelMenuItem(int nIdx)
	{
		XeASSERT(m_menu && nIdx < m_itemlist.size());
		if (m_menu && nIdx < m_itemlist.size())
		{
			const auto& item = m_itemlist[nIdx];
			CPoint ptMenu(item->m_rItem.left, item->m_rItem.bottom);
			::ClientToScreen(Hwnd(), &ptMenu);
			CRect rcWnd;
			::GetWindowRect(Hwnd(), &rcWnd);
			ptMenu.y = rcWnd.bottom;
			m_nCurrentlyShowingTopLevelMenuItem = nIdx;
			m_menu->SetTopLevelMenuNavigationCallback(
					[this](const TopLevelMenuNavigationParameters& params)
					{ return _GetTopLevelMenuNavigationItemIndex(params); });
			m_isShowingSubMenu = true;
			_RedrawToolBar();
			UINT uSelectedID = m_menu->ShowMenu(::GetParent(Hwnd()), ptMenu, item->m_uIDcommand);
			m_isShowingSubMenu = false;
			_RedrawToolBar();
			m_nCurrentlyShowingTopLevelMenuItem = -1;
			if (uSelectedID)
			{
				m_menu->SetTopLevelMenuNavigationCallback(nullptr);

				// Note - WM_COMMAND already sent for selected menu item.
				if (uSelectedID == ID__APP_EXIT)
				{
					::PostMessage(::GetParent(Hwnd()), WMU_NOTIFY_ID_APP_EXIT, 0, 0);
				}
			}
			else if (m_menu->m_cmn->m_nTopLevelMenuNavigationItemIndex >= 0	// Show another top level menu?
				&& m_menu->m_cmn->m_nTopLevelMenuNavigationItemIndex < m_itemlist.size())
			{
				int nMenuItemIndex = m_menu->m_cmn->m_nTopLevelMenuNavigationItemIndex;
				m_menu->m_cmn->m_nTopLevelMenuNavigationItemIndex = -1;
				PostMessage(Hwnd(), WMU_SHOW_TOP_LEVEL_MENU_ITEM, (WPARAM)nMenuItemIndex, 0);
				// Note - we post a message to show the other menu - to avoid recursive calls here.
			}
		}
	}

	int _GetTopLevelMenuNavigationItemIndex(const TopLevelMenuNavigationParameters& params)
	{
		if (params.m_isNavigationByMouse)
		{
			CPoint ptMouse(params.m_ptMouseScr);
			::ScreenToClient(Hwnd(), &ptMouse);
			const XETBItem* pItem = 0;
			int nIdx = _FindItemFromPoint(ptMouse, &pItem);
			if (pItem && pItem->m_itemType == XETBItemType::MENU_ITEM && nIdx != m_nCurrentlyShowingTopLevelMenuItem)
			{
				return nIdx;
			}
		}
		else
		{
			if (m_nCurrentlyShowingTopLevelMenuItem >= 0)
			{
				int& nCurIdx = m_nCurrentlyShowingTopLevelMenuItem;
				int nMax = m_menu_first_index + m_menu_item_count - 1;
				if (params.m_vk_key_code == VK_LEFT)
				{
					if (--nCurIdx < m_menu_first_index)
					{
						return nMax;
					}
					return nCurIdx;
				}
				if (params.m_vk_key_code == VK_RIGHT)
				{
					if (++nCurIdx > nMax)
					{
						return m_menu_first_index;
					}
					return nCurIdx;
				}
			}
		}
		return -1;
	}
#pragma endregion MenuHandling

#pragma region DropListHandling
protected:
	void _ShowDropListButtonPopup(const XETBItem* pItem)
	{
		CXeTB_DropListPopup* p = _GetDropListPtr(pItem->m_uIDcommand);
		if (p)
		{
			_OnShowDropListButtonPopup(pItem, p);
			int item_count = (int)p->m_cmn->OnGetCountMsg(0, 0);
			CRect rcParentCtrl(pItem->m_rItem);
			::ClientToScreen(Hwnd(), (LPPOINT)&rcParentCtrl);
			::ClientToScreen(Hwnd(), ((LPPOINT)&rcParentCtrl) + 1);
			CRect rcPopup = p->m_cmn->CalcPopupWindowSize(rcParentCtrl.left, rcParentCtrl.bottom,
					rcParentCtrl.Width(), 2.0, item_count);
			p->ShowPopup(Hwnd(), rcPopup, rcParentCtrl);
			if (p->m_nLastUserSelectedItemIdx >= 0)
			{
				if (!_OnDropListButton_SelectedItem(pItem, p, p->m_nLastUserSelectedItemIdx))
				{
					_SendCommand(pItem);
				}
			}
		}
	}

	virtual void _OnShowDropListButtonPopup(const XETBItem* pItem, CXeTB_DropListPopup* pDropListCtrl)
	{

	}

	//// Return true if selection has been processed and send command to parent is not needed.
	virtual bool _OnDropListButton_SelectedItem(const XETBItem* pItem, CXeTB_DropListPopup* pDropListCtrl, int selIdx)
	{
		return false;
	}
#pragma endregion DropListHandling

#pragma region Tooltips
protected:
	virtual LRESULT _OnNotify_NeedTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT) override
	{	// WM_NOTIFY - UDM_TOOLTIP_NEED_TT handler.
		XeASSERT(pNeedTT);
		if (pNeedTT && GetTooltip(pNeedTT/*, sti*/))
		{
			return 1;
		}
		return 0;
	}

public:
	bool GetTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT/*, SUPER_TOOLTIP_INFO& sti*/)
	{
		//sti.rgbBg = m_xeUI->GetColor(CID::TT_Bg);
		//sti.rgbText = m_xeUI->GetColor(CID::TT_Text);
		//sti.hWnd = GetSafeHwnd();
		pNeedTT->ti->hTWnd = GetSafeHwnd();

		CPoint pt(*pNeedTT->pt);
		const XETBItem* pItem = 0;
		bool isToolTip = false;
		_FindItemFromPoint(pt, &pItem);
		if (pItem && pt.y >= pItem->m_rItem.top && pt.y < pItem->m_rItem.bottom)
		{
			std::wstring tooltip;
			int cxTooltip = 0;
			if (!_OnGetTooltip(pItem, tooltip, cxTooltip))
			{
				if (pItem->m_sToolTip.size() > 0)
				{
					cxTooltip = pItem->m_cxTooltip;
					tooltip = pItem->m_sToolTip.c_str();
				}
			}
			if (tooltip.size() > 0)
			{
				//sti.nSizeX = cxTooltip;
				pNeedTT->ti->sTooltip = tooltip;
				//sti.m_sBody = tooltip.c_str();
				pNeedTT->ti->hTWnd = GetSafeHwnd();
				pNeedTT->ti->rectBounds = pItem->m_rItem;
				pNeedTT->ti->ptTipOffset.x = pItem->m_rItem.left;
				isToolTip = true;
			}
		}
		return isToolTip;
	}

protected:
	virtual bool _OnGetTooltip(const XETBItem* pItem, std::wstring& tooltip, int& cxTooltip)
	{
		return false;
	}
#pragma endregion Tooltips
};

