module;

#include "os_minimal.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <d2d1.h>
#include <dwrite.h>

export module Xe.ComboBoxEx;

import Xe.PopupCtrl;
import Xe.ListBoxExCommon;
import Xe.ScintillaEditControl;
import Xe.UIcolorsIF;
import Xe.D2DWndBase;
import Xe.Helpers;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class EFindUIcmd { eEscape, eShiftF3, eF3 };
typedef std::function<void(EFindUIcmd)> FindUIcmdCallback;

constexpr wchar_t XECOMBOBOXEXWND_CLASSNAME[] = L"XeComboBoxExWndClass";  // Window class name

constexpr UINT uIDC_EDIT = 1901;
constexpr UINT uIDC_LISTBOX = 1902;

export class CXeComboBoxEx : public CXeD2DCtrlBase
{
protected:
	bool m_hasFocus = false, m_isEnabled = true;

	std::unique_ptr<CXeScintillaEditControl> m_edit;
	std::unique_ptr<CXeListBoxExCommon> m_listbox;

	bool m_isStatic = false;

	float m_cxyBorder = 1.0f;

	float m_cxTextLmargin = 2.0f;
	int m_cyListbox = 200;

	bool m_hasCustomWidth = false;
	int m_cxListbox = 0;

	FindUIcmdCallback m_findUIcmdFunc = nullptr;

#pragma region Create
public:
	CXeComboBoxEx(CXeUIcolorsIF* pUIcolors) : CXeD2DCtrlBase(pUIcolors)
	{
		m_xeUI->RegisterWindowClass(XECOMBOBOXEXWND_CLASSNAME, D2DCtrl_WndProc);

		m_edit = std::make_unique<CXeScintillaEditControl>(m_xeUI);
		m_listbox = std::make_unique<CXeListBoxExCommon>(m_xeUI);
		m_listbox->Initialize(LBType::Combo,
			[this](WORD nfCode, int item_idx) { OnLBnotify(nfCode, item_idx); });
	}
	//virtual ~CXeComboBoxEx() {}

	BOOL Create(DWORD dwStyle, DWORD dwExStyle, const CRect& rect, HWND hWndParent, UINT nID,
		int cyListbox, DWORD dwLB_Style, const wchar_t* tooltip)
	{
		m_cyListbox = cyListbox;
		XeASSERT(hWndParent);
		if (!::IsWindow(hWndParent))
		{
			XeASSERT(FALSE);
			return FALSE;
		}
		HWND hWnd = CreateD2DCtrl(dwExStyle, XECOMBOBOXEXWND_CLASSNAME, nullptr, dwStyle,
				rect, hWndParent, nID, tooltip);
		DWORD dwLBstyle = m_style.WS().hasCBS_SORT() ? LBS_SORT : 0;
		m_isStatic = m_style.WS().hasCBS_DROPDOWNLIST();	// Selected item is a static control
		m_listbox->SetStyle(dwLBstyle, 0);
		// Note - OnSize has been called.
		if (!m_isStatic)
		{
			CRect rcEdit(rect);
			rcEdit.OffsetRect(-rcEdit.left, -rcEdit.top);	// Make top, left = 0,0
			rcEdit.right -= m_xeUI->GetValue(UIV::cxComboBoxButton);
			if (m_control_has_border)
			{
				rcEdit.DeflateRect(1, 1);
			}
			m_edit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, hWnd, rcEdit, uIDC_EDIT, nullptr);
			m_edit->SetComboBoxKeyDownFilterCallback([this](UINT nChar, UINT nRepCnt, UINT nFlags)
				{ return _EditCtrlKeyDownFilter(nChar, nRepCnt, nFlags); });
		}
		return hWnd != 0;
	}

	virtual LRESULT _OnNcDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam) override
	{
		if (m_edit->GetSafeHwnd() != NULL)
		{
			::DestroyWindow(m_edit->GetSafeHwnd());
		}
		return CXeD2DCtrlBase::_OnNcDestroy(hwnd, wParam, lParam);
	}

	void SetFindUIcmdCallback(FindUIcmdCallback findUIcmdFunc)
	{
		m_findUIcmdFunc = findUIcmdFunc;
	}

protected:
	// WM_SIZE message handler.
	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{
		LRESULT lResult = CXeD2DCtrlBase::_OnSize(hWnd, wParam, lParam);
		UINT nType = (UINT)wParam;
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		XeASSERT(::IsWindow(Hwnd()));
		XeASSERT(!m_style.WS().hasCBS_UnsupportedStyle());

		if (m_edit->GetSafeHwnd())
		{
			int cxButton = m_xeUI->GetValue(UIV::cxComboBoxButton);
			bool hasBorder = m_control_has_border;
			int x = hasBorder ? 1 : 0, y = x;
			cx = cx > cxButton ? cx - cxButton : 0;
			cx = cx > 2 && hasBorder ? cx - 2 : cx;
			cy = cy > 2 && hasBorder ? cy - 2 : cy;
			m_edit->SetWindowPos(nullptr, x, y, cx, cy, SWP_NOCOPYBITS | SWP_NOZORDER);
		}
		return lResult;
	}

	// WM_ENABLE message handler.
	virtual LRESULT _OnEnableWindow(bool isEnabled) override
	{
		XeASSERT(::IsWindow(Hwnd()));

		m_isEnabled = (bool)isEnabled;
		if (m_edit->GetSafeHwnd())
		{
			m_edit->EnableWindow(isEnabled);
		}
		_RedrawDirectly();
		return 0;
	}

	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WM_GETTEXT:				return OnGetText(wParam, lParam);
		case WM_GETTEXTLENGTH:			return OnGetTextLen(wParam, lParam);
		case CB_GETEDITSEL:				return OnGetEditSelMsg(wParam, lParam);
		case CB_SETEDITSEL:				return OnSetEditSelMsg(wParam, lParam);
		case CB_LIMITTEXT:				return OnLimitTextMsg(wParam, lParam);
		case CB_ADDSTRING:				return OnAddStringMsg(wParam, lParam);
		case CB_INSERTSTRING:			return OnInsertStringMsg(wParam, lParam);
		case CB_DELETESTRING:			return OnDeleteStringMsg(wParam, lParam);
		case CB_DIR:					return OnNotSupportedMsg(wParam, lParam);
		case CB_GETCOUNT:				return OnGetCountMsg(wParam, lParam);
		case CB_GETCURSEL:				return OnGetCurSelMsg(wParam, lParam);
		case CB_GETLBTEXT:				return OnGetLBtextMsg(wParam, lParam);
		case CB_GETLBTEXTLEN:			return OnGetLBtextLenMsg(wParam, lParam);
		case CB_RESETCONTENT:			return OnResetContentMsg(wParam, lParam);
		case CB_SELECTSTRING:			return OnNotSupportedMsg(wParam, lParam);
		case CB_SETCURSEL:				return OnSetCurSelMsg(wParam, lParam);
		case CB_SHOWDROPDOWN:			return OnShowDropDownMsg(wParam, lParam);
		case CB_GETITEMDATA:			return OnGetItemDataMsg(wParam, lParam);
		case CB_SETITEMDATA:			return OnSetItemDataMsg(wParam, lParam);
		case CB_GETDROPPEDCONTROLRECT:	return OnGetDroppedControlRectMsg(wParam, lParam);
		case CB_SETITEMHEIGHT:			return OnSetItemHeightMsg(wParam, lParam);
		case CB_GETITEMHEIGHT:			return OnNotSupportedMsg(wParam, lParam);
		case CB_SETEXTENDEDUI:			return OnNotSupportedMsg(wParam, lParam);
		case CB_GETEXTENDEDUI:			return OnNotSupportedMsg(wParam, lParam);
		case CB_GETDROPPEDSTATE:		return OnGetDroppedStateMsg(wParam, lParam);
		case CB_FINDSTRING:				return OnNotSupportedMsg(wParam, lParam);
		case CB_FINDSTRINGEXACT:		return OnFindStringExactMsg(wParam, lParam);
		case CB_SETLOCALE:				return OnNotSupportedMsg(wParam, lParam);
		case CB_GETLOCALE:				return OnNotSupportedMsg(wParam, lParam);
		case CB_GETTOPINDEX:			return OnNotSupportedMsg(wParam, lParam);
		case CB_SETTOPINDEX:			return OnNotSupportedMsg(wParam, lParam);
		case CB_GETHORIZONTALEXTENT:	return OnNotSupportedMsg(wParam, lParam);
		case CB_SETHORIZONTALEXTENT:	return OnNotSupportedMsg(wParam, lParam);
		case CB_GETDROPPEDWIDTH:		return OnGetDroppedWidthMsg(wParam, lParam);
		case CB_SETDROPPEDWIDTH:		return OnSetDroppedWidthMsg(wParam, lParam);
		case CB_INITSTORAGE:			return OnNotSupportedMsg(wParam, lParam);
		case CB_GETCOMBOBOXINFO:		return OnGetComboBoxInfoMsg(wParam, lParam);
		}
		return CXeD2DCtrlBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender) override
	{
		if (wID == uIDC_EDIT && wSource == EN_CHANGE)
		{
			OnEnChange();
		}
		return CXeD2DCtrlBase::_OnWmCommand(wSource, wID, sender);
	}
#pragma endregion Create

#pragma region Painting
protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		D2D1_RECT_F rcClient = rc;
		if (m_control_has_border)
		{
			pRT->DrawRectangle(rc, GetBrush(CID::CtrlTxtDis));

			// NOTE - client area includes border - meaning all painting must be offset by the border size.
			rcClient = DeflateRectF(rc, m_cxyBorder);
		}
	
		bool isWindowEnabled = IsWindowEnabled();

		D2D1_RECT_F rcBtn(rcClient);
		float cxButton = (float)m_xeUI->GetValue(UIV::cxComboBoxButton);
		rcBtn.left = rcBtn.right - cxButton;
		CID cidBtnFg = isWindowEnabled ? CID::CtrlTxt : CID::CtrlTxtDis;
		_DrawComboBoxButton(pRT, rcBtn, cidBtnFg, CID::CtrlBg);

		if (m_isStatic)
		{
			D2D1_RECT_F rcStatic(rcClient);
			rcStatic.right = rcStatic.right - cxButton;
			pRT->FillRectangle(rcStatic, GetBrush(CID::CtrlBg));
		}

		UINT curSel = (UINT)m_listbox->OnGetCurSelMsg(0, 0);
		UINT txtLen = (UINT)m_listbox->OnGetTextLenMsg(curSel, 0);
		if (m_isStatic && curSel != LB_ERR && txtLen != LB_ERR)
		{
			ListBoxItemData itemData;
			itemData.m_i64 = m_listbox->OnGetItemDataMsg(curSel, 0);
			std::wstring text(txtLen, L' ');
			m_listbox->OnGetTextMsg(curSel, (LPARAM)&text[0]);

			D2D1_RECT_F rcText(rcClient);
			rcText.left += m_cxTextLmargin;
			rcText.right -= cxButton;
			rcText.top += 2;
			rcText.bottom -= 2;

			if (m_hasFocus)
			{
				pRT->FillRectangle(rcText, GetBrush(CID::CtrlCurItemBg));
				_DrawFocusRect(pRT, rcText);
			}

			DWORD rgbText = m_xeUI->GetColor(isWindowEnabled ? CID::CtrlTxt : CID::CtrlTxtDis);
			if (itemData.m_hasColor && !itemData.m_isColorSquare && isWindowEnabled)
			{
				rgbText = itemData.m_rgbColor;
			}
			_CreateAndDrawTextLayout(pRT, text, rcText, rgbText);
		}
	}
#pragma endregion Painting

#pragma region Focus
protected:
	// WM_SETFOCUS message handler.
	virtual LRESULT _OnSetFocus(HWND hOldWnd) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_hasFocus = true;
		_RedrawDirectly();
		if (m_edit->GetSafeHwnd())
		{
			m_edit->SetFocus();
			m_edit->SetSel(0, -1);
		}
		return 0;
	}

	// WM_KILLFOCUS message handler.
	virtual LRESULT _OnKillFocus(HWND hNewWnd) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_hasFocus = false;
		_RedrawDirectly();
		return 0;
	}

	// Q83302: HOWTO: Use the WM_GETDLGCODE Message
	// https://jeffpar.github.io/kbarchive/kb/083/Q83302/
	// Q104637: HOWTO: Trap Arrow Keys in an Edit Control of a Dialog Box
	// https://jeffpar.github.io/kbarchive/kb/104/Q104637/
	// WM_GETDLGCODE message handler.
	virtual LRESULT _OnGetDlgCode(WPARAM wParam, LPARAM lParam) override
	{
		XeASSERT(::IsWindow(Hwnd()));
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

#pragma region Mouse
	virtual LRESULT _OnNcHitTest(WPARAM wParam, LPARAM lParam) override
	{
		CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		CRect rcC;
		GetClientRect(&rcC);
		ScreenToClient(&point);
		if (rcC.PtInRect(point))
		{
			return HTCLIENT;
		}
		return HTBORDER;
	}

	// WM_LBUTTONDOWN message handler.
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint point) override
	{
		SetFocus();
		CRect rcButton;
		GetClientRect(&rcButton);
		if (!m_isStatic)	// Click anywhere in a static control to initiate the popup.
		{
			rcButton.left = rcButton.right - m_xeUI->GetValue(UIV::cxComboBoxButton);
		}
		if (rcButton.PtInRect(point))
		{
			ShowPopupListBox();
		}
		return 0;
	}

	// WM_MOUSEWHEEL message handler.
	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint pt) override
	{
		if (m_isStatic)
		{
			XeASSERT(::IsWindow(Hwnd()));
			short xPos = (short)pt.x;
			short yPos = (short)pt.y;

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
			if (wSBcode != 0xFFFF)
			{
				size_t num_items = m_listbox->OnGetCountMsg(0, 0);
				size_t curSel = m_listbox->OnGetCurSelMsg(0, 0);
				do
				{
					if (wSBcode == SB_LINEUP) { --curSel; } else { ++curSel; }
					if (curSel < num_items)
					{
						m_listbox->OnSetCurSelMsg(curSel, 0);
						_NotifyParent(CBN_SELENDOK);
					}
				} while ((zDelta -= WHEEL_DELTA) >= WHEEL_DELTA);
				_RedrawDirectly();
			}
		}
		return 0;	// Message was processed.
	}
#pragma endregion Mouse

#pragma region Keyboard
	// WM_KEYDOWN message handler.
	virtual LRESULT _OnKeyDown(WPARAM wParam, LPARAM lParam) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		UINT nChar = (UINT)wParam;
		UINT nRepCnt = (UINT)lParam & 0xFFFF;
		UINT nFlags = (UINT)(lParam >> 16) & 0xFFFF;
		if (_EditCtrlKeyDownFilter(nChar, nRepCnt, nFlags))
		{
			return 0;
		}
		if (m_isStatic)
		{
			size_t num_items = m_listbox->OnGetCountMsg(0, 0);
			size_t curSel = m_listbox->OnGetCurSelMsg(0, 0);
			size_t newIdx = 0xFFFFFFFF, page = 10;
			if (num_items > 0 && curSel < num_items)
			{
				switch (wParam)
				{
				case VK_PRIOR:
					newIdx = curSel < page ? 0 : curSel - page;
					break;

				case VK_NEXT:
					newIdx = curSel + page > num_items - 1 ? num_items - 1 : curSel + page;
					break;

				case VK_HOME:
					newIdx = 0;
					break;

				case VK_END:
					newIdx = num_items - 1;
					break;

				case VK_LEFT:
				case VK_UP:
					newIdx = curSel - 1;
					break;

				case VK_RIGHT:
				case VK_DOWN:
					newIdx = curSel + 1;
					break;
				}
				if (newIdx < num_items)
				{
					m_listbox->OnSetCurSelMsg(newIdx, 0);
					_RedrawDirectly();
					_NotifyParent(CBN_SELENDOK);
					return 0;
				}
			}
		}
		return 0;
	}

	// WM_KEYUP message handler.
	virtual LRESULT _OnKeyUp(WPARAM wParam, LPARAM lParam) override
	{
		return 0;
	}

	bool _EditCtrlKeyDownFilter(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		CXeShiftCtrlAltKeyHelper sca;
		if (m_findUIcmdFunc)
		{
			if (sca.IsNoneDown() && nChar == VK_F3)
			{
				m_findUIcmdFunc(EFindUIcmd::eF3);
				return true;
			}
			else if (sca.IsOnlyShiftDown() && nChar == VK_F3)
			{
				m_findUIcmdFunc(EFindUIcmd::eShiftF3);
				return true;
			}
			else if (sca.IsNoneDown() && nChar == VK_ESCAPE)
			{
				m_findUIcmdFunc(EFindUIcmd::eEscape);
				return true;
			}
			else if (sca.IsNoneDown() && nChar == VK_RETURN)
			{
				m_findUIcmdFunc(EFindUIcmd::eF3);
				return true;
			}
		}
		if (sca.IsNoneDown() && (nChar == VK_UP || nChar == VK_DOWN))
		{
			size_t count = OnGetCountMsg(0, 0);
			size_t curIdx = OnGetCurSelMsg(0, 0), newIdx = LB_ERR;
			if (curIdx == LB_ERR)
			{
				newIdx = 0;
			}
			else if (nChar == VK_UP && curIdx > 0)
			{
				newIdx = curIdx - 1;
			}
			else if (nChar == VK_DOWN)
			{
				newIdx = curIdx + 1;
			}
			if (newIdx != LB_ERR && newIdx < count)
			{
				OnSetCurSelMsg(newIdx, 0);
			}
			return true;
		}
		return false;
	}
#pragma endregion Keyboard

#pragma region Helpers
	bool _NotifyParent(UINT nfCode)
	{
		XeASSERT(::IsWindow(Hwnd()));
		HWND hWndParent = ::GetParent(Hwnd());
		if (hWndParent)
		{
			WPARAM wParam = MAKELONG(::GetDlgCtrlID(Hwnd()), nfCode);
			::SendMessage(hWndParent, WM_COMMAND, wParam, (LPARAM)Hwnd());
			return true;
		}
		return false;
	}
#pragma endregion Helpers

#pragma region CustomComboBoxSupport
public:
	void SetItemsList(const std::vector<ListBoxExItem>& list)
	{
		m_listbox->SetItemsList(list);
	}

	const ListBoxExItem* GetItemDataAtIndexConst(int idx) const
	{
		return m_listbox->GetItemDataAtIndexConst(idx);
	}

	const ListBoxExItem* GetCurSelItemDataConst() const
	{
		size_t curSel = m_listbox->OnGetCurSelMsg(0, 0);
		return m_listbox->GetItemDataAtIndexConst(curSel == 0xFFFFFFFF ? -1 : (int)curSel);
	}
#pragma endregion CustomComboBoxSupport

#pragma region ListBox
	void ShowPopupListBox()
	{
		CRect rcListbox;
		GetClientRect(&rcListbox);
		CRect rcCombobox(rcListbox);
		rcListbox.top = rcListbox.bottom;
		rcListbox.bottom = rcListbox.top + m_cyListbox;
		if (m_hasCustomWidth)
		{
			rcListbox.right = m_cxListbox;
		}
		ClientToScreen(rcListbox);
		ClientToScreen(rcCombobox);
		CXePopupCtrl popup(m_xeUI, m_listbox.get(), XeShowPopup::FadeIn80);
		popup.ShowPopup(Hwnd(), rcListbox, rcCombobox);
	}

	void OnLBnotify(WORD nfCode, int item_idx)
	{
		if (nfCode == LBN_SELCHANGE)
		{
			_NotifyParent(CBN_SELCHANGE);
		}
		else if (nfCode == LBN_DBLCLK)
		{
			_NotifyParent(CBN_DBLCLK);
		}
		else if (nfCode == CBN_DROPDOWN || nfCode == CBN_CLOSEUP)
		{
			_NotifyParent(nfCode);
			if (nfCode == CBN_CLOSEUP && m_listbox->m_isSelEndOk)
			{
				_NotifyParent(CBN_SELENDOK);
			}
		}
		else if (nfCode == CBN_SELENDCANCEL)
		{
			_NotifyParent(CBN_SELENDCANCEL);
		}
		_RedrawDirectly();
	}
#pragma endregion ListBox

#pragma region CComboBox_MFCfunctions
public:
int GetCount() const
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_GETCOUNT, 0, 0); }
int GetCurSel() const
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_GETCURSEL, 0, 0); }
int SetCurSel(int nSelect)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_SETCURSEL, nSelect, 0); }
DWORD GetEditSel() const
	{ XeASSERT(::IsWindow(Hwnd())); return DWORD(::SendMessage(Hwnd(), CB_GETEDITSEL, 0, 0)); }
BOOL LimitText(int nMaxChars)
	{ XeASSERT(::IsWindow(Hwnd())); return (BOOL)::SendMessage(Hwnd(), CB_LIMITTEXT, nMaxChars, 0); }
BOOL SetEditSel(_In_ int nStartChar, _In_ int nEndChar)
	{ XeASSERT(::IsWindow(Hwnd())); return (BOOL)::SendMessage(Hwnd(), CB_SETEDITSEL, 0, MAKELONG(nStartChar, nEndChar)); }
DWORD_PTR GetItemData(int nIndex) const
	{ XeASSERT(::IsWindow(Hwnd())); return ::SendMessage(Hwnd(), CB_GETITEMDATA, nIndex, 0); }
int SetItemData(int nIndex, DWORD_PTR dwItemData)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_SETITEMDATA, nIndex, (LPARAM)dwItemData); }
void* GetItemDataPtr(int nIndex) const
	{ XeASSERT(::IsWindow(Hwnd())); return (LPVOID)GetItemData(nIndex); }
int SetItemDataPtr(int nIndex, void* pData)
	{ XeASSERT(::IsWindow(Hwnd())); return SetItemData(nIndex, (DWORD_PTR)(LPVOID)pData); }
int GetLBText(/*_In_*/ int nIndex, /*_Pre_notnull_ _Post_z_*/ LPTSTR lpszText) const
{ 
	XeASSERT(::IsWindow(Hwnd())); 
	return (int)::SendMessage(Hwnd(), CB_GETLBTEXT, nIndex, (LPARAM)lpszText); 
}
int GetLBTextLen(int nIndex) const
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_GETLBTEXTLEN, nIndex, 0); }
void ShowDropDown(BOOL bShowIt)
	{ XeASSERT(::IsWindow(Hwnd())); ::SendMessage(Hwnd(), CB_SHOWDROPDOWN, bShowIt, 0); }
int AddString(LPCTSTR lpszString)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_ADDSTRING, 0, (LPARAM)lpszString); }
int DeleteString(UINT nIndex)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_DELETESTRING, nIndex, 0);}
int InsertString(_In_ int nIndex, _In_z_ LPCTSTR lpszString)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_INSERTSTRING, nIndex, (LPARAM)lpszString); }
void ResetContent()
	{ XeASSERT(::IsWindow(Hwnd())); ::SendMessage(Hwnd(), CB_RESETCONTENT, 0, 0); }
int FindString(/*_In_*/ int nStartAfter, /*_In_z_*/ LPCTSTR lpszString) const
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_FINDSTRING, nStartAfter,
		(LPARAM)lpszString); }
int SelectString(int nStartAfter, LPCTSTR lpszString)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_SELECTSTRING,
		nStartAfter, (LPARAM)lpszString); }
void Clear()
	{ XeASSERT(::IsWindow(Hwnd())); ::SendMessage(Hwnd(), WM_CLEAR, 0, 0); }
void Copy()
	{ XeASSERT(::IsWindow(Hwnd())); ::SendMessage(Hwnd(), WM_COPY, 0, 0); }
void Cut()
	{ XeASSERT(::IsWindow(Hwnd())); ::SendMessage(Hwnd(), WM_CUT, 0, 0); }
void Paste()
	{ XeASSERT(::IsWindow(Hwnd())); ::SendMessage(Hwnd(), WM_PASTE, 0, 0); }
int SetItemHeight(int nIndex, UINT cyItemHeight)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_SETITEMHEIGHT, nIndex, MAKELONG(cyItemHeight, 0)); }
int GetItemHeight(int nIndex) const
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_GETITEMHEIGHT, nIndex, 0L); }
int FindStringExact(int nIndexStart, LPCTSTR lpszFind) const
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_FINDSTRINGEXACT, nIndexStart, (LPARAM)lpszFind); }
int SetExtendedUI(BOOL bExtended )
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_SETEXTENDEDUI, bExtended, 0L); }
BOOL GetExtendedUI() const
	{ XeASSERT(::IsWindow(Hwnd())); return (BOOL)::SendMessage(Hwnd(), CB_GETEXTENDEDUI, 0, 0L); }
void GetDroppedControlRect(LPRECT lprect) const
	{ XeASSERT(::IsWindow(Hwnd())); ::SendMessage(Hwnd(), CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)lprect); }
BOOL GetDroppedState() const
	{ XeASSERT(::IsWindow(Hwnd())); return (BOOL)::SendMessage(Hwnd(), CB_GETDROPPEDSTATE, 0, 0L); }
LCID GetLocale() const
	{ XeASSERT(::IsWindow(Hwnd())); return (LCID)::SendMessage(Hwnd(), CB_GETLOCALE, 0, 0); }
LCID SetLocale(LCID nNewLocale)
	{ XeASSERT(::IsWindow(Hwnd())); return (LCID)::SendMessage(Hwnd(), CB_SETLOCALE, (WPARAM)nNewLocale, 0); }
int GetTopIndex() const
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_GETTOPINDEX, 0, 0); }
int SetTopIndex(int nIndex)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_SETTOPINDEX, nIndex, 0); }
void SetHorizontalExtent(UINT nExtent)
	{ XeASSERT(::IsWindow(Hwnd())); ::SendMessage(Hwnd(), CB_SETHORIZONTALEXTENT, nExtent, 0); }
UINT GetHorizontalExtent() const
	{ XeASSERT(::IsWindow(Hwnd())); return (UINT)::SendMessage(Hwnd(), CB_GETHORIZONTALEXTENT, 0, 0); }
int SetDroppedWidth(UINT nWidth)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_SETDROPPEDWIDTH, nWidth, 0); }
int GetDroppedWidth() const
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), CB_GETDROPPEDWIDTH, 0, 0); }
#pragma endregion CComboBox_MFCfunctions

#pragma region ComboBox_message_handlers
protected:
	// WM_SETTEXT message handler.
	virtual LRESULT _OnSetText(WPARAM wParam, LPARAM lParam) override
	{
		if (!m_isStatic)
		{
			LPCTSTR pStr = (LPCTSTR)lParam;
			m_edit->SetTextSuppressEnChangeNotify(pStr);
			return TRUE;	// TRUE is the correct return value for this message.
		}
		return CB_ERR;	// No edit control
	}

	// WM_GETTEXT
	LRESULT OnGetText(WPARAM wParam, LPARAM lParam)
	{
		if (m_isStatic)
		{
			size_t curSel = m_listbox->OnGetCurSelMsg(0, 0);
			size_t textLen = m_listbox->OnGetTextLenMsg(curSel, 0);
			if (textLen != LB_ERR && textLen < wParam)
			{
				LRESULT result = m_listbox->OnGetTextMsg(curSel, lParam);
				return result == LB_ERR ? 0 : result;
			}
			return 0;
		}
		return m_edit->SendMessage(WM_GETTEXT, wParam, lParam);
	}

	// WM_GETTEXTLENGTH
	LRESULT OnGetTextLen(WPARAM wParam, LPARAM lParam)
	{
		if (m_isStatic)
		{
			size_t curSel = m_listbox->OnGetCurSelMsg(0, 0);
			size_t textLen = m_listbox->OnGetTextLenMsg(curSel, 0);
			return textLen != LB_ERR ? textLen : 0;
		}
		return m_edit->SendMessage(WM_GETTEXTLENGTH, wParam, lParam);
	}

	LRESULT OnResetContentMsg(WPARAM wParam, LPARAM lParam)
	{
		m_listbox->OnResetContentMsg(wParam, lParam);
		if (!m_isStatic)
		{
			std::wstring txt;
			::SendMessageW(m_edit->GetSafeHwnd(), WM_SETTEXT, 0, (LPARAM)txt.c_str());
		}
		return CB_OKAY;
	}

	LRESULT OnGetCurSelMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->OnGetCurSelMsg(wParam, lParam);
	}

	/* An application sends a CB_SETCURSEL message to select a string in the list of a
	combo box. If necessary, the list scrolls the string into view. The text in the edit
	control of the combo box changes to reflect the new selection, and any previous
	selection in the list is removed.

	Parameters:
	wParam=the zero-based index of the string to select. If this parameter is -1, any
	current selection in the list is removed and the edit control is cleared.

	lParam=This parameter is not used.

	Return value - If the message is successful, the return value is the index of the
	item selected. If wParam is greater than the number of items in the list or if
	wParam is -1, the return value is CB_ERR and the selection is cleared. */
	LRESULT OnSetCurSelMsg(WPARAM wParam, LPARAM lParam)
	{
		LRESULT result = m_listbox->OnSetCurSelMsg(wParam, lParam);
		if (!m_isStatic)
		{
			if (wParam >= 0)
			{
				UINT txtLen = (UINT)m_listbox->OnGetTextLenMsg(wParam, 0);
				if (txtLen != LB_ERR)
				{
					std::wstring text(txtLen, L' ');
					m_listbox->OnGetTextMsg(wParam, (LPARAM)&text[0]);
					::SendMessageW(m_edit->GetSafeHwnd(), WM_SETTEXT, 0, (LPARAM)text.c_str());
				}
			}
			else
			{
				std::wstring text;
				::SendMessageW(m_edit->GetSafeHwnd(), WM_SETTEXT, 0, (LPARAM)text.c_str());
			}
		}
		_RedrawDirectly();
		return result;
	}

	/* Adds a string to the list box of a combo box. If the combo box does not have the
	CBS_SORT style, the string is added to the end of the list. Otherwise, the string is
	inserted into the list, and the list is sorted.

	Parameters:
	wParam=This parameter is not used.

	lParam=An LPCTSTR pointer to the null-terminated string to be added. If you create the
	combo box with an owner-drawn style but without the CBS_HASSTRINGS style, the value of
	the lParam parameter is stored as item data rather than the string it would otherwise
	point to. The item data can be retrieved or modified by sending the CB_GETITEMDATA or
	CB_SETITEMDATA message.

	Return value - The return value is the zero-based index to the string in the list box
	of the combo box. If an error occurs, the return value is CB_ERR. If insufficient space
	is available to store the new string, it is CB_ERRSPACE. */
	LRESULT OnAddStringMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->OnAddStringMsg(wParam, lParam);
	}

	// CB_DELETESTRING, 
	LRESULT OnDeleteStringMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->OnDeleteStringMsg(wParam, lParam);
	}

	LRESULT OnInsertStringMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->OnInsertStringMsg(wParam, lParam);
	}

	/* An application sends a CB_SETITEMDATA message to set the value associated with the
	specified item in a combo box.

	Parameters:
	wParam=Specifies the item's zero-based index.

	lParam=Specifies the new value to be associated with the item.

	Return value - If an error occurs, the return value is CB_ERR. */
	LRESULT OnSetItemDataMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->OnSetItemDataMsg(wParam, lParam);
	}

	// CB_GETITEMDATA, 
	LRESULT OnGetItemDataMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->OnGetItemDataMsg(wParam, lParam);
	}

	/* An application sends a CB_SETITEMHEIGHT message to set the height of list items or
	the selection field in a combo box.

	Parameters:
	wParam=Specifies the component of the combo box for which to set the height.
	This parameter must be 1 to set the height of the selection field. It must be zero to set
	the height of list items, unless the combo box has the CBS_OWNERDRAWVARIABLE style.
	In that case, the wParam parameter is the zero-based index of a specific list item.

	lParam=Specifies the height, in pixels, of the combo box component identified by wParam.

	Return value - If the index or height is invalid, the return value is CB_ERR. */
	LRESULT OnSetItemHeightMsg(WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}

	/* CB_GETEDITSEL message
	Gets the starting and ending character positions of the current selection in the edit
	control of a combo box.

	Parameters:
	wParam=A pointer to a DWORD value that receives the starting position of the selection.
	This parameter can be NULL.

	lParam=A pointer to a DWORD value that receives the ending position of the selection.
	This parameter can be NULL.

	Return value - zero-based DWORD value with the starting position of the selection in
	the LOWORD and with the ending position of the first character after the last selected
	character in the HIWORD. */
	LRESULT OnGetEditSelMsg(WPARAM wParam, LPARAM lParam)
	{
		if (m_isStatic) { return 0; }
		return m_edit->SendMessage(EM_GETSEL, wParam, lParam);
	}

	/* An application sends a CB_SETEDITSEL message to select characters in the edit control
	of a combo box.

	Parameters
	wParam=This parameter is not used.

	lParam [in] = The LOWORD of lParam specifies the starting position. If the LOWORD is -1,
	the selection, if any, is removed.
	The HIWORD of lParam specifies the ending position. If the HIWORD is -1, all text from
	the starting position to the last character in the edit control is selected.

	Return value - If the message succeeds, the return value is TRUE. If the message is sent
	to a combo box with the CBS_DROPDOWNLIST style, it is CB_ERR. */
	LRESULT OnSetEditSelMsg(WPARAM wParam, LPARAM lParam)
	{
		if (m_isStatic) { return CB_ERR; }
		m_edit->SetSel(LOWORD(lParam), HIWORD(lParam));
		return TRUE;
	}

	LRESULT OnLimitTextMsg(WPARAM wParam, LPARAM lParam)
	{
		if (m_isStatic) { return 0; }
		m_edit->SendMessage(EM_LIMITTEXT, wParam, lParam);
		return TRUE;
	}

	// CB_GETCOUNT
	LRESULT OnGetCountMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->OnGetCountMsg(wParam, lParam);
	}
	// CB_GETLBTEXT
	LRESULT OnGetLBtextMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->OnGetTextMsg(wParam, lParam);
	}
	// CB_GETLBTEXTLEN
	LRESULT OnGetLBtextLenMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->OnGetTextLenMsg(wParam, lParam);
	}
	// WM_COMMAND
	void OnEnChange()
	{
		HWND this_hWnd = GetSafeHwnd();
		::SendMessage(::GetParent(this_hWnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(),
				CBN_EDITCHANGE), (LPARAM)this_hWnd);
	}
	// CB_SHOWDROPDOWN
	LRESULT OnShowDropDownMsg(WPARAM wParam, LPARAM lParam)
	{
		bool isPopupShown = m_listbox->GetHwnd() != 0;
		if (wParam)
		{
			if (!isPopupShown)
			{
				ShowPopupListBox();
			}
		}
		else
		{
			if (isPopupShown)
			{
				m_listbox->m_isSelEndCancel = true;	// Hide popup
			}
		}
		return TRUE;
	}
	// CB_GETDROPPEDSTATE
	LRESULT OnGetDroppedStateMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->GetHwnd() ? TRUE : FALSE;
	}

	// CB_FINDSTRINGEXACT
	LRESULT OnFindStringExactMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_listbox->OnFindStringExactMsg(wParam, lParam);
	}

	/* CB_GETCOMBOBOXINFO message
	Gets information about the specified combo box.

	Parameters
	wParam=This parameter is not used.
	lParam [out] = A pointer to a COMBOBOXINFO structure that receives the information.

	Return value - If the function succeeds, the return value is nonzero.
	If the function fails, the return value is zero. */
	LRESULT OnGetComboBoxInfoMsg(WPARAM wParam, LPARAM lParam)
	{
		COMBOBOXINFO* pCBI = (COMBOBOXINFO*)lParam;
		if (pCBI->cbSize != sizeof(COMBOBOXINFO)) { return 0; }
		CRect rcClient;
		GetClientRect(&rcClient);
		CRect rcEdit(rcClient);
		int cxButton = m_xeUI->GetValue(UIV::cxComboBoxButton);
		rcEdit.right -= cxButton;
		CRect rcButton(rcClient);
		rcButton.left = rcButton.right - cxButton;
		// A RECT structure that specifies the coordinates of the edit box.
		pCBI->rcItem = rcEdit;

		// A RECT structure that specifies the coordinates of the button that contains the drop-down arrow.
		pCBI->rcButton = rcButton;

		pCBI->stateButton = 0;
		pCBI->hwndCombo = Hwnd();
		pCBI->hwndItem = m_edit->GetSafeHwnd();	// A handle to the edit box.
		pCBI->hwndList = 0;	// A handle to the drop - down list.
		return TRUE;
	}

	/* CB_GETDROPPEDCONTROLRECT message
	An application sends a CB_GETDROPPEDCONTROLRECT message to retrieve the screen coordinates
	of a combo box in its dropped-down state.

	Parameters
	wParam=This parameter is not used.
	lParam=A pointer to the RECT structure that receives the coordinates of the combo box in
	its dropped-down state.

	Return value - If the message succeeds, the return value is nonzero.
	If the message fails, the return value is zero.*/
	LRESULT OnGetDroppedControlRectMsg(WPARAM wParam, LPARAM lParam)
	{
		CRect rcW;
		GetWindowRect(&rcW);
		rcW.bottom -= m_cyListbox;
		RECT* pRect = (RECT*)lParam;
		*pRect = rcW;
		return 0;
	}

	// CB_GETDROPPEDWIDTH
	LRESULT OnGetDroppedWidthMsg(WPARAM wParam, LPARAM lParam)
	{
		if (m_hasCustomWidth) return m_cxListbox;
		CRect rcW;
		GetWindowRect(&rcW);
		return rcW.Width();
	}

	// CB_SETDROPPEDWIDTH
	LRESULT OnSetDroppedWidthMsg(WPARAM wParam, LPARAM lParam)
	{
		m_hasCustomWidth = true;
		m_cxListbox = (int)wParam;
		return m_cxListbox;
	}

	LRESULT OnNotSupportedMsg(WPARAM wParam, LPARAM lParam)
	{
		XeASSERT(FALSE);	// Not supported in this implementation of a combobox.
		return 0;
	}
#pragma endregion ComboBox_message_handlers
};
