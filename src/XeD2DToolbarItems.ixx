module;

#include "os_minimal.h"
#include <string>
#include <memory>
#include <optional>
#include <d2d1.h>
#include <dwrite.h>
#include <boost/algorithm/string.hpp>

export module Xe.D2DToolbarItems;

import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.D2DWndBase;
import Xe.Menu;
import Xe.ListBoxExCommon;
import Xe.D2DButton;
import Xe.ComboBoxEx;
import Xe.D2DRenderContext;
import Xe.PopupCtrl;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class XETBItemType
{
	PNG_ICON_BUTTON = 2,
	MENU_ITEM = 3,
	PNG_BUTTON = 4,
	STD_BUTTON = 7,
	CHECKBOX = 8,
	LABEL = 9,
	LABEL_BORDER = 10,
	PNG_NRW_BUTTON = 11,	// Narrow png image button
	COMBOBOX = 12,
	DROP_LIST_BTN = 13,
	SEPARATOR = 16
};

// stXETB_ITEM::m_flags =
export constexpr uint8_t XETB_ST_BG_RED = 0x80;		// BG color of item is red (signal upgrade error).
export constexpr uint8_t XETB_ST_BG_GREEN = 0x40;	// BG color of item is green (signal upgrade ready).
export constexpr uint8_t XETB_ST_GRAY = 0x20;		// Gray text in label type item.

export CSize GetToolBarItemTextSize(CXeUIcolorsIF* pUIcolors, XETBItemType type, const wchar_t* pTxt)
{
	CSize sizeTxt;
	if (pTxt)
	{
		sizeTxt = pUIcolors->GetTextSizeW(EXE_FONT::eUI_Font, pTxt);
	}
	switch (type)
	{
	case XETBItemType::LABEL:
	case XETBItemType::LABEL_BORDER:
		sizeTxt.cx += 8;
		break;
	case XETBItemType::STD_BUTTON:
		sizeTxt.cx += 18;
		break;
	case XETBItemType::CHECKBOX:
		sizeTxt.cx += 24;
		break;
	case XETBItemType::MENU_ITEM:
		break;	// No extra margin needed.
	case XETBItemType::COMBOBOX:
		sizeTxt.cx += 25;	// add space for drop down button.
		break;
	case XETBItemType::PNG_ICON_BUTTON:
	case XETBItemType::PNG_NRW_BUTTON:
	case XETBItemType::PNG_BUTTON:
	case XETBItemType::DROP_LIST_BTN:
	case XETBItemType::SEPARATOR:
		XeASSERT(FALSE);	// text size has no meaning for this type of control.
		break;
	}
	return sizeTxt;
}

export struct XETBItem
{
	CXeUIcolorsIF*				m_xeUI = nullptr;

	XETBItemType				m_itemType = XETBItemType::SEPARATOR;		// Type and style of item.

	UINT						m_uIDcommand = 0;	// Command ID of toolbar item.
								// When item is menu - this is index into menu.

	bool						m_isVisible = true;

	bool						m_isEnabled = true;	// Enabled state of the control.

	CRect						m_rItem;			// Toolbar item bounding rect (flyby button pos & size)

	std::wstring				m_sText;			// Label or Menu text

	int							m_cxRmargin = 0;	// Right hand side margin (pixels until next control)

	std::wstring				m_sToolTip;
	int							m_cxTooltip = 0;	// Default width used when 0.

	std::wstring				m_sRefCalcText;

	uint8_t						m_flags = 0;		// Special flags - see: XETBD2D_ST_xxxxx

	XETBItem()
	{
		XeASSERT(true);
	}
	virtual ~XETBItem()
	{
		XeASSERT(true);
	}

	XETBItem(CXeUIcolorsIF* pUIcolors, UINT uID, XETBItemType type, const wchar_t* szTooltip) : XETBItem()
	{
		m_xeUI = pUIcolors;
		m_uIDcommand = uID;
		m_itemType = type;
		if (szTooltip)
		{
			m_sToolTip = szTooltip;
		}
	}

	// Called by Toolbar when its window is being destroyed.
	virtual void OnDestroy() = 0;

	// Calculate item position, return next item x pos.
	virtual int CalculateItemRect(int x, int cyTopMargin, int cyTB) = 0;

	virtual void MoveWindowToCalculatedPosition() = 0;

	// Returns 0 if item is not a "window" control.
	virtual HWND GetControlHwnd() = 0;

	int GetNextControlXpos() const { return m_isVisible ? m_rItem.right + m_cxRmargin : m_rItem.left; }

	// Note - derived class that has an actual window should override this member
	// to enable the window (and then call base class).
	virtual bool SetEnabledState(bool isEnabled)
	{
		bool isChanged = isEnabled != m_isEnabled;
		m_isEnabled = isEnabled;
		return isChanged;
	}

	// Note - derived class that has an actual window should override this member
	// to show/hide the window (and then call base class).
	virtual void SetVisibleState(bool isVisible)
	{
		m_isVisible = isVisible;
	}

	virtual PID GetBtnImageId() const { return PID::None; }
	virtual void SetBtnImageId(PID pId) const {}

	virtual void SetFlags(uint8_t flags) { m_flags = flags; }
	virtual uint8_t GetFlags() const { return m_flags; }

	virtual void SetTooltip(const wchar_t* szToolTip, int cxTooltip = 0)
	{
		m_sToolTip = szToolTip;
		m_cxTooltip = cxTooltip;
	}

	virtual void OnChangedUIsettings(bool isFontsChanged, bool isColorsChanged) = 0;

	virtual void _Paint(CXeD2DRenderContext* pRctx, const D2D1_POINT_2F& ptOfs, bool isLbtnDownItem, bool isHotItem) = 0;

	int _CalculateItemRectVcenter(int x, int cyTopMargin, int cyTB, int cxItem, int cyItem)
	{
		m_rItem.left = x;
		m_rItem.right = m_rItem.left + cxItem;
		m_rItem.top = (int)((double)(cyTopMargin + (cyTB - cyItem)) / 2 + 0.5);
		m_rItem.bottom = m_rItem.top + cyItem;
		return m_isVisible ? m_rItem.right + m_cxRmargin : x;
	}
};

export struct XETBItem_Menu : public XETBItem
{
	XETBItem_Menu(CXeUIcolorsIF* pUIcolors, HWND hWndParent, UINT uID, const wchar_t* szText, const wchar_t* szTooltip,
		int xPos, int cyTopMargin, int cyTB)
		: XETBItem(pUIcolors, uID, XETBItemType::MENU_ITEM, szTooltip)
	{
		m_cxRmargin = 2;
		m_sText = szText;
		CalculateItemRect(xPos, cyTopMargin, cyTB);
	}

	virtual void OnDestroy() override {}

	virtual int CalculateItemRect(int x, int cyTopMargin, int cyTB) override
	{
		int cxItem = 0, cyItem = cyTB;
		if (m_isVisible)
		{
			CSize sizeTxt = m_xeUI->GetTextSizeW(EXE_FONT::eUI_Font, m_sText.c_str());
			cxItem = sizeTxt.cx + 6;
			cyItem = sizeTxt.cy;
		}
		return _CalculateItemRectVcenter(x, cyTopMargin, cyTB, cxItem, cyItem);
	}

	virtual void MoveWindowToCalculatedPosition() override
	{
		// This control is not a child window.
	}
	virtual HWND GetControlHwnd() override
	{
		// This control is not a child window.
		return 0;
	}

	virtual void OnChangedUIsettings(bool isFontsChanged, bool isColorsChanged) override
	{
	}

	virtual void _Paint(CXeD2DRenderContext* pRctx, const D2D1_POINT_2F& ptOfs, bool isLbtnDownItem, bool isHotItem) override
	{
		D2D1_RECT_F rcItem = OffsetRectF(RectFfromRect(m_rItem), ptOfs.x, ptOfs.y);
		if (isHotItem && isLbtnDownItem)
		{
			rcItem = OffsetRectF(rcItem, 1.0f, 1.0f);
		}
		std::wstring s = boost::erase_all_copy(m_sText, L"&");
		pRctx->_CreateAndDrawTextLayout(pRctx->m_pCurrentRT, s, rcItem, CID::CtrlTxt,
				EXE_FONT::eUI_Font, DWRITE_TEXT_ALIGNMENT_CENTER);

		// TODO: Draw first letter with underscore when alt key down.

		//UINT uFmtDraw = DT_CENTER | DT_VCENTER | DT_SINGLELINE
		//		| (isAltKeyDown && !m_isShowingSubMenu ? 0 : DT_HIDEPREFIX);
		//::DrawTextExW(dcMem.m_hDC, item.sMenuText.data(), (int)item.sMenuText.size(), &rcTxt, uFmtDraw, NULL);

		if (isHotItem && m_isEnabled)
		{
			pRctx->m_pCurrentRT->DrawRectangle(rcItem, pRctx->GetBrush(CID::XTbHotBrd));
		}
	}
};

export struct XETBItem_Separator : public XETBItem
{
	XETBItem_Separator(CXeUIcolorsIF* pUIcolors, int xPos, int cyTopMargin, int cyTB)
		: XETBItem(pUIcolors, 0, XETBItemType::SEPARATOR, nullptr)
	{
		m_cxRmargin = 4;
		CalculateItemRect(xPos, cyTopMargin, cyTB);
	}

	virtual void OnDestroy() override {}

	virtual int CalculateItemRect(int x, int cyTopMargin, int cyTB) override
	{
		int cxItem = m_isVisible ? 6 : 0, cyItem = cyTB;
		return _CalculateItemRectVcenter(x, cyTopMargin, cyTB, cxItem, cyItem);
	}

	virtual void MoveWindowToCalculatedPosition() override
	{
		// This control is not a child window.
	}
	virtual HWND GetControlHwnd() override
	{
		// This control is not a child window.
		return 0;
	}

	virtual void OnChangedUIsettings(bool isFontsChanged, bool isColorsChanged) override
	{
	}

	virtual void _Paint(CXeD2DRenderContext* pRctx, const D2D1_POINT_2F& ptOfs, bool isLbtnDownItem, bool isHotItem) override
	{
		D2D1_RECT_F rcItem = OffsetRectF(RectFfromRect(m_rItem), ptOfs.x, ptOfs.y);
		float x = rcItem.left + (WidthOf(rcItem) / 2.0f) - 1.0f, cy = 18.0f;
		float y = rcItem.top + 2 + ((HeightOf(rcItem) - cy) / 2.0f);
		pRctx->m_pCurrentRT->DrawLine(D2D1_POINT_2F(x, y), D2D1_POINT_2F(x, y + cy), pRctx->GetBrush(CID::XTbHotBrd), 2.0);
	}
};

export struct XETBItem_Label : public XETBItem
{
	XETBItem_Label(CXeUIcolorsIF* pUIcolors, HWND hWndParent, UINT uID, const wchar_t* szText, const wchar_t* szTooltip, 
			bool isBorder, int xPos, int cyTopMargin, int cyTB)
			: XETBItem(pUIcolors, uID, (isBorder ? XETBItemType::LABEL_BORDER : XETBItemType::LABEL), szTooltip)
	{
		m_cxRmargin = 4;
		m_sText = szText;
		CalculateItemRect(xPos, cyTopMargin, cyTB);
	}

	virtual void OnDestroy() override {}

	virtual int CalculateItemRect(int x, int cyTopMargin, int cyTB) override
	{
		int cxItem = 0, cyItem = cyTB;
		if (m_isVisible)
		{
			CSize sizeTxt = m_xeUI->GetTextSizeW(EXE_FONT::eUI_Font, m_sText.c_str());
			cxItem = sizeTxt.cx + 8 + (m_itemType == XETBItemType::LABEL_BORDER ? 6 : 0);
			cyItem = sizeTxt.cy + (m_itemType == XETBItemType::LABEL_BORDER ? 6 : 0);
		}
		return _CalculateItemRectVcenter(x, cyTopMargin, cyTB, cxItem, cyItem);
	}

	virtual void MoveWindowToCalculatedPosition() override
	{
		// This control is not a child window.
	}
	virtual HWND GetControlHwnd() override
	{
		// This control is not a child window.
		return 0;
	}

	virtual void OnChangedUIsettings(bool isFontsChanged, bool isColorsChanged) override
	{
	}

	virtual void _Paint(CXeD2DRenderContext* pRctx, const D2D1_POINT_2F& ptOfs, bool isLbtnDownItem, bool isHotItem) override
	{
		D2D1_RECT_F rcItem = OffsetRectF(RectFfromRect(m_rItem), ptOfs.x, ptOfs.y);
		if (m_itemType == XETBItemType::LABEL_BORDER)
		{
			rcItem.bottom -= 1.0f;
			CID brd_clr = isHotItem && m_isEnabled ? CID::XTbHotBrd : CID::CtrlTxt;
			pRctx->m_pCurrentRT->DrawRectangle(rcItem, pRctx->GetBrush(brd_clr));
		}
		//if (bState & XETB_ST_GRAY)
		//{
		//	rgbTmp = dcMem.SetTextColor(rgbGrayTextColor);
		//} 
		//pRctx->_DrawText3(pRctx->m_pCurrentRT, m_sText, rcItem);
		//if (item.bState & XETB_ST_GRAY)
		//{
		//	dcMem.SetTextColor(rgbTmp);
		//}
		CID fg = m_flags & XETB_ST_GRAY ? CID::XTbGrayTxt : CID::CtrlTxt;
		pRctx->_CreateAndDrawTextLayout(pRctx->m_pCurrentRT, m_sText, rcItem,
				fg, EXE_FONT::eUI_Font, DWRITE_TEXT_ALIGNMENT_CENTER);
	}
};

export struct XETBItem_Button : public XETBItem
{
	std::unique_ptr<CXeD2DButton> m_button_object;

	XETBItem_Button(CXeUIcolorsIF* pUIcolors, HWND hWndParent, XETBItemType type, UINT uID, const wchar_t* szText, 
			const wchar_t* szTooltip, int xPos, int cyTopMargin, int cyTB)
		: XETBItem(pUIcolors, uID, type, szTooltip)
	{
		XeASSERT(type == XETBItemType::STD_BUTTON || type == XETBItemType::CHECKBOX);
		m_cxRmargin = 4;
		//m_bState = 0;
		m_sText = szText;
		CalculateItemRect(xPos, cyTopMargin, cyTB);
		m_button_object = std::make_unique<CXeD2DButton>(m_xeUI);
		DWORD dwStyle = type == XETBItemType::STD_BUTTON ? BS_PUSHBUTTON : BS_AUTOCHECKBOX;
		m_button_object->CreateTBmode(szText, WS_CHILD | WS_VISIBLE | dwStyle, m_rItem, hWndParent,
				m_uIDcommand, szTooltip);
	}

	XETBItem_Button(CXeUIcolorsIF* pUIcolors, HWND hWndParent, XETBItemType type, UINT uID, PID uPngImgId, 
			const wchar_t* szTooltip, int xPos, int cyTopMargin, int cyTB)
			: XETBItem(pUIcolors, uID, type, szTooltip)
	{
		XeASSERT(type == XETBItemType::PNG_NRW_BUTTON || type == XETBItemType::PNG_ICON_BUTTON);
		m_cxRmargin = 4;
		CalculateItemRect(xPos, cyTopMargin, cyTB);
		m_button_object = std::make_unique<CXeD2DButton>(m_xeUI);
		m_button_object->CreateTBmode(L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_BITMAP,
				m_rItem, hWndParent, m_uIDcommand, szTooltip);
		::SendMessage(m_button_object->GetSafeHwnd(), BM_SETIMAGE, 0, (LPARAM)uPngImgId);
	}

	virtual void OnDestroy() override
	{
		if (m_button_object->GetSafeHwnd())
		{
			::DestroyWindow(m_button_object->GetSafeHwnd());
		}
	}

	virtual int CalculateItemRect(int x, int cyTopMargin, int cyTB) override
	{
		int cxItem = 0, cyItem = cyTB;
		if (m_isVisible && m_itemType == XETBItemType::STD_BUTTON || m_itemType == XETBItemType::CHECKBOX)
		{
			CSize sizeTxt = m_xeUI->GetTextSizeW(EXE_FONT::eUI_Font, m_sText.c_str());
			cxItem = sizeTxt.cx + (m_itemType == XETBItemType::STD_BUTTON ? 18 : 24);
			cyItem = sizeTxt.cy;
		}
		else if (m_isVisible)
		{
			cxItem = m_itemType == XETBItemType::PNG_NRW_BUTTON ? 20 : 20 + 4;
			cyItem = m_itemType == XETBItemType::PNG_NRW_BUTTON ? 24 : 20 + 4;
		}
		return _CalculateItemRectVcenter(x, cyTopMargin, cyTB, cxItem, cyItem);
	}

	virtual void MoveWindowToCalculatedPosition() override
	{
		if (m_button_object->GetSafeHwnd())
		{
			::MoveWindow(m_button_object->GetSafeHwnd(), m_rItem.left, m_rItem.top, m_rItem.Width(), m_rItem.Height(), TRUE);
		}
	}
	virtual HWND GetControlHwnd() override
	{
		return m_button_object->GetSafeHwnd();
	}

	void SetCheckBoxState(int nCheck)
	{
		if (m_button_object.get())
		{
			::SendMessage(m_button_object->GetSafeHwnd(), BM_SETCHECK, (WPARAM)nCheck, 0);
		}
	}

	int GetCheckBoxState()
	{
		if (m_button_object.get())
		{
			return (int)::SendMessage(m_button_object->GetSafeHwnd(), BM_GETCHECK, 0, 0);
		}
		XeASSERT(FALSE);
		return 0;
	}

	virtual PID GetBtnImageId() const override
	{
		if (m_button_object.get())
		{
			return (PID)::SendMessage(m_button_object->GetSafeHwnd(), BM_GETIMAGE, 0, 0);
		}
		return PID::None;
	}

	virtual void SetBtnImageId(PID pId) const
	{
		if (m_button_object.get())
		{
			::SendMessage(m_button_object->GetSafeHwnd(), BM_SETIMAGE, 0, (LPARAM)pId);
		}
	}

	virtual bool SetEnabledState(bool isEnabled) override
	{
		if (m_button_object.get())
		{
			::EnableWindow(m_button_object->GetSafeHwnd(), isEnabled);
		}
		return XETBItem::SetEnabledState(isEnabled);
	}

	virtual void SetVisibleState(bool isVisible) override
	{
		if (m_button_object.get())
		{
			::ShowWindow(m_button_object->GetSafeHwnd(), isVisible ? SW_SHOW : SW_HIDE);
		}
		XETBItem::SetVisibleState(isVisible);
	}

	virtual void SetFlags(uint8_t flags) override
	{
		XETBItem::SetFlags(flags);
		if (m_button_object.get())
		{
			CID bg = (flags & XETB_ST_BG_GREEN) ? CID::LogFilterHdrBg : (flags & XETB_ST_BG_RED) ? CID::ErrorBg : CID::Invalid;
			m_button_object->SetButtonBgColor(bg);
		}
	}

	virtual void SetTooltip(const wchar_t* szToolTip, int cxTooltip = 0) override
	{
		XETBItem::SetTooltip(szToolTip, cxTooltip);
		if (m_button_object.get())
		{
			m_button_object->SetTooltipString(szToolTip);
		}
	}

	virtual void OnChangedUIsettings(bool isFontsChanged, bool isColorsChanged) override
	{
		m_button_object->OnChangedUIsettings(isFontsChanged, isColorsChanged);
	}

	virtual void _Paint(CXeD2DRenderContext* pRctx, const D2D1_POINT_2F& ptOfs, bool isLbtnDownItem, bool isHotItem) override
	{
		D2D1_RECT_F rcItem = OffsetRectF(RectFfromRect(m_rItem), ptOfs.x, ptOfs.y);
		if (m_button_object.get())
		{
			m_button_object->_PaintInternal(pRctx, rcItem, m_isEnabled, isLbtnDownItem, isHotItem);
		}
	}
};

export class CXeTB_DropListPopup : public CXePopupCtrl //, public CWnd
{
public:
	std::unique_ptr<CXeListBoxExCommon> m_cmn;

	int m_nLastUserSelectedItemIdx = -1;
	std::wstring m_lastUserSelectedItem;

	CXeTB_DropListPopup(CXeUIcolorsIF* pUIcolors) : CXePopupCtrl(pUIcolors, XeShowPopup::FadeIn80)
	{
		m_cmn = std::make_unique<CXeListBoxExCommon>(m_xeUI);
		m_cmn->Initialize(LBType::DropDown,
			[this](WORD nfCode, int item_idx) { OnLBnotify(nfCode, item_idx); });
		//m_cmn.Initialize(pUIcolors, LBType::DropDown,
		//	[this](WORD nfCode, int item_idx) { OnLBnotify(nfCode, item_idx); });
		SetPopupPointer(m_cmn.get());
	}

protected:
	void OnLBnotify(WORD nfCode, int item_idx)
	{
		if (nfCode == LBN_SELCHANGE)
		{
		}
		else if (nfCode == LBN_DBLCLK)
		{
		}
		else if (nfCode == CBN_DROPDOWN || nfCode == CBN_CLOSEUP)
		{
			m_nLastUserSelectedItemIdx = -1;
			m_lastUserSelectedItem.clear();
			if (nfCode == CBN_CLOSEUP && m_cmn->m_isSelEndOk)
			{
				LRESULT curSel = m_cmn->OnGetCurSelMsg(0, 0);
				const ListBoxExItem* pItem = m_cmn->GetItemDataAtIndexConst((int)curSel);
				if (pItem)
				{
					m_nLastUserSelectedItemIdx = (int)curSel;
					m_lastUserSelectedItem = pItem->m_string;
				}
			}
		}
		else if (nfCode == CBN_SELENDCANCEL)
		{
			m_nLastUserSelectedItemIdx = -1;
			m_lastUserSelectedItem.clear();
		}
	}
};

export struct XETBItem_DropButton : public XETBItem
{
	std::unique_ptr<CXeD2DButton> m_button_object;
	std::unique_ptr<CXeTB_DropListPopup> m_droplist_object;

	XETBItem_DropButton(CXeUIcolorsIF* pUIcolors, HWND hWndParent, UINT uID, PID uPngImgId,
		const wchar_t* szTooltip, int xPos, int cyTopMargin, int cyTB)
		: XETBItem(pUIcolors, uID, XETBItemType::DROP_LIST_BTN, szTooltip)
	{
		m_cxRmargin = 2;
		CalculateItemRect(xPos, cyTopMargin, cyTB);
		m_button_object = std::make_unique<CXeD2DButton>(m_xeUI);
		m_droplist_object = std::make_unique<CXeTB_DropListPopup>(m_xeUI);
		m_button_object->CreateTBmode(L"", WS_CHILD | WS_VISIBLE | BS_SPLITBUTTON, m_rItem, hWndParent,
				m_uIDcommand, szTooltip);
		::SendMessage(m_button_object->GetSafeHwnd(), BM_SETIMAGE, D2D_BS_IGNORE_SPLIT_BS, (LPARAM)uPngImgId);
	}

	//~XETBItem_DropButton()
	//{
	//	if (m_button_object.get())
	//	{
	//		m_button_object.reset();
	//	}
	//}

	virtual void OnDestroy() override
	{
		if (m_button_object->GetSafeHwnd())
		{
			::DestroyWindow(m_button_object->GetSafeHwnd());
		}
		if (m_droplist_object->GetPopupHwnd())
		{
			::DestroyWindow(m_droplist_object->GetPopupHwnd());
		}
	}

	virtual int CalculateItemRect(int x, int cyTopMargin, int cyTB) override
	{
		int cxItem = m_isVisible ? 20 + 4 + m_xeUI->GetValue(UIV::cxComboBoxButton) : 0;
		int cyItem = 20 + 4;
		return _CalculateItemRectVcenter(x, cyTopMargin, cyTB, cxItem, cyItem);
	}

	virtual void MoveWindowToCalculatedPosition() override
	{
		if (m_button_object->GetSafeHwnd())
		{
			::MoveWindow(m_button_object->GetSafeHwnd(), m_rItem.left, m_rItem.top, m_rItem.Width(), m_rItem.Height(), TRUE);
		}
	}
	virtual HWND GetControlHwnd() override
	{
		return m_button_object->GetSafeHwnd();
	}

	virtual PID GetBtnImageId() const override
	{
		if (m_button_object.get())
		{
			return (PID)::SendMessage(m_button_object->GetSafeHwnd(), BM_GETIMAGE, 0, 0);
		}
		return PID::None;
	}
	
	virtual void SetBtnImageId(PID pId) const
	{
		if (m_button_object.get())
		{
			::SendMessage(m_button_object->GetSafeHwnd(), BM_SETIMAGE, 0, (LPARAM)pId);
		}
	}

	virtual bool SetEnabledState(bool isEnabled) override
	{
		if (m_button_object.get())
		{
			::EnableWindow(m_button_object->GetSafeHwnd(), isEnabled);
		}
		return XETBItem::SetEnabledState(isEnabled);
	}

	virtual void OnChangedUIsettings(bool isFontsChanged, bool isColorsChanged) override
	{
		m_button_object->OnChangedUIsettings(isFontsChanged, isColorsChanged);
	}

	virtual void _Paint(CXeD2DRenderContext* pRctx, const D2D1_POINT_2F& ptOfs, bool isLbtnDownItem, bool isHotItem) override
	{
		D2D1_RECT_F rcItem = OffsetRectF(RectFfromRect(m_rItem), ptOfs.x, ptOfs.y);
		if (m_button_object.get())
		{
			m_button_object->_PaintInternal(pRctx, rcItem, m_isEnabled, isLbtnDownItem, isHotItem);
		}
	}
};

export struct XETBItem_ComboBox : public XETBItem
{
	std::unique_ptr<CXeComboBoxEx> m_combobox_object;
	CSize m_combobox_size;
	int m_cyComboBoxDropDown = 0;

	XETBItem_ComboBox(CXeUIcolorsIF* pUIcolors, HWND hWndParent, DWORD dwStyle, UINT uID, const wchar_t* szTooltip,
			CSize sizeItem, int cyDropDown, int xPos, int cyTopMargin, int cyTB)
			: XETBItem(pUIcolors, uID, XETBItemType::COMBOBOX, szTooltip)
	{
		m_cxRmargin = 2;
		m_combobox_size = sizeItem;
		//m_bState = 0;
		m_cyComboBoxDropDown = cyDropDown;
		CalculateItemRect(xPos, cyTopMargin, cyTB);
		m_combobox_object = std::make_unique<CXeComboBoxEx>(m_xeUI);
		m_combobox_object->Create(WS_CHILD | WS_VISIBLE | dwStyle, 0, m_rItem, hWndParent, m_uIDcommand,
				m_cyComboBoxDropDown, 0, szTooltip);
	}

	virtual void OnDestroy() override
	{
		if (m_combobox_object->GetSafeHwnd())
		{
			::DestroyWindow(m_combobox_object->GetSafeHwnd());
		}
	}

	virtual int CalculateItemRect(int x, int cyTopMargin, int cyTB) override
	{
		int cxItem = m_isVisible ? m_combobox_size.cx : 0, cyItem = cyTB;
		return _CalculateItemRectVcenter(x, cyTopMargin, cyTB, cxItem, cyItem);
	}

	virtual void MoveWindowToCalculatedPosition() override
	{
		if (m_combobox_object->GetSafeHwnd())
		{
			::MoveWindow(m_combobox_object->GetSafeHwnd(), m_rItem.left, m_rItem.top, m_rItem.Width(), m_rItem.Height(), TRUE);
		}
	}
	virtual HWND GetControlHwnd() override
	{
		return m_combobox_object->GetSafeHwnd();
	}

	virtual bool SetEnabledState(bool isEnabled) override
	{
		if (m_combobox_object.get())
		{
			::EnableWindow(m_combobox_object->GetSafeHwnd(), isEnabled);
		}
		return XETBItem::SetEnabledState(isEnabled);
	}

	virtual void OnChangedUIsettings(bool isFontsChanged, bool isColorsChanged) override
	{
		m_combobox_object->OnChangedUIsettings(isFontsChanged, isColorsChanged);
	}

	virtual void _Paint(CXeD2DRenderContext* pRctx, const D2D1_POINT_2F& ptOfs, bool isLbtnDownItem, bool isHotItem) override
	{
		//D2D1_RECT_F rcItem = OffsetRectF(RectFfromRect(m_rItem), ptOfs.x, ptOfs.y);
		// No painting needed.
	}

	void SetComboBoxList(const std::vector<ListBoxExItem>& list, int curSelIdx = -1)
	{
		if (m_combobox_object.get())
		{
			m_combobox_object->SetItemsList(list);
			m_combobox_object->SetCurSel(curSelIdx);
		}
	}
};

