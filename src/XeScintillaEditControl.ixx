module;

#include "os_minimal.h"
#include <functional>
#include <memory>
#include <string>
#include "..\scintilla\include\Scintilla.h"
//#include "CustomWndMsgs.h"

export module Xe.ScintillaEditControl;

import Xe.ScrollBar;
import Xe.UIcolorsIF;
import Xe.D2DWndBase;
import Xe.Helpers;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef std::function<bool(UINT, UINT, UINT)> KeyDownFilterCallback;

// Helper class (enum) - state of H/V scroll bars visibility.
// Also helps calculate client area rects.
class EC_WndMode
{
public:
	CRect m_rcEditControl, m_rcHscr, m_rcVscr, m_rcCorner;

	int m_CYHSCROLL = 0, m_CXVSCROLL = 0;

	bool m_isSingleLineCtrl = false;

	bool m_hasBorder = false;

	int m_cyFont = 15;

protected:
	enum class ECWM_Enum { NoScrollBars, HScrollOnly, VScrollOnly, BothScrollBars };
	ECWM_Enum m_e = ECWM_Enum::NoScrollBars;

public:
	void Initialize(bool hasBorder, int cyFont, bool isSingleLineCtrl)
	{
		m_hasBorder = hasBorder;
		m_isSingleLineCtrl = isSingleLineCtrl;
		m_cyFont = cyFont;
	}
	bool HasHScrollBar() const
	{
		return m_e == ECWM_Enum::BothScrollBars || m_e == ECWM_Enum::HScrollOnly;
	}
	bool HasVScrollBar() const
	{
		return m_e == ECWM_Enum::BothScrollBars || m_e == ECWM_Enum::VScrollOnly;
	}
	bool HasBothScrollBars() const { return m_e == ECWM_Enum::BothScrollBars; }

	// Return true if state changed.
	bool SetHScrollBar(bool isHscrollBarNeeded)
	{
		if (m_isSingleLineCtrl)
		{
			isHscrollBarNeeded = false;
		}
		if (isHscrollBarNeeded)
		{
			if (HasHScrollBar())
			{
				return false;
			}
			m_e = m_e == ECWM_Enum::NoScrollBars ? ECWM_Enum::HScrollOnly : ECWM_Enum::BothScrollBars;
		}
		else
		{
			if (!HasHScrollBar())
			{
				return false;
			}
			m_e = m_e == ECWM_Enum::BothScrollBars ? ECWM_Enum::VScrollOnly : ECWM_Enum::NoScrollBars;
		}
		return true;
	}

	// Return true if state changed.
	bool SetVScrollBar(bool isVscrollBarNeeded)
	{
		if (m_isSingleLineCtrl)
		{
			isVscrollBarNeeded = false;
		}
		if (isVscrollBarNeeded)
		{
			if (HasVScrollBar())
			{
				return false;
			}
			m_e = m_e == ECWM_Enum::NoScrollBars ? ECWM_Enum::VScrollOnly : ECWM_Enum::BothScrollBars;
		}
		else
		{
			if (!HasVScrollBar())
			{
				return false;
			}
			m_e = m_e == ECWM_Enum::BothScrollBars ? ECWM_Enum::HScrollOnly : ECWM_Enum::NoScrollBars;
		}
		return true;
	}

	void SetClientRect(const CRect& rcClient)
	{
		if (m_CYHSCROLL == 0)
		{
			m_CYHSCROLL = ::GetSystemMetrics(SM_CYHSCROLL);
			m_CXVSCROLL = ::GetSystemMetrics(SM_CXVSCROLL);
		}
		m_rcEditControl = rcClient;
		m_rcEditControl.OffsetRect(-m_rcEditControl.left, -m_rcEditControl.top);
		if (m_hasBorder)
		{
			m_rcEditControl.DeflateRect(1, 1);
		}
		m_rcHscr = m_rcVscr = m_rcCorner = m_rcEditControl;
		bool hasBothSBs = HasBothScrollBars();
		m_rcCorner.left = hasBothSBs ? m_rcCorner.right - m_CXVSCROLL : m_rcCorner.right;
		m_rcCorner.top = hasBothSBs ? m_rcCorner.bottom - m_CYHSCROLL : m_rcCorner.bottom;
		if (HasHScrollBar())
		{
			m_rcHscr.top = m_rcHscr.bottom - m_CYHSCROLL;
			if (hasBothSBs)
			{
				m_rcHscr.right -= m_CXVSCROLL;
			}
			m_rcEditControl.bottom -= m_rcHscr.Height();
		}
		else
		{
			m_rcHscr.top = m_rcHscr.bottom;
		}
		if (HasVScrollBar())
		{
			m_rcVscr.left = m_rcVscr.right - m_CXVSCROLL;
			if (hasBothSBs)
			{
				m_rcVscr.bottom -= m_CYHSCROLL;
			}
			m_rcEditControl.right -= m_rcVscr.Width();
		}
		else
		{
			m_rcVscr.left = m_rcVscr.right;
		}
		if (m_isSingleLineCtrl)
		{
			XeASSERT(m_e == ECWM_Enum::NoScrollBars);
			int cy = m_rcEditControl.Height();
			if (m_cyFont < cy)
			{
				int yOfs = (cy - m_cyFont) / 2;
				m_rcEditControl.top += yOfs;
				m_rcEditControl.bottom = m_rcEditControl.top + m_cyFont;
			}
		}
	}
};

constexpr wchar_t XESCINTILLAEDITCONTROLWND_CLASSNAME[] = L"XeScintillaEditControlWndClass";  // Window class name

export class CXeScintillaEditControl : public CXeD2DCtrlBase
{
protected:
	HWND m_hSciEdWnd = 0;
	std::unique_ptr<CXeScrollBar> m_hScr, m_vScr;

	EC_WndMode m_mode;

	clock_t m_lastDblClickTicks = 0;

	KeyDownFilterCallback m_comboBoxKeydownFilterFunc = nullptr;

	bool m_isSuppressEnChangeNotify = false;

	XeWindowStyle m_editStyle;

#pragma region Create
public:
	CXeScintillaEditControl(CXeUIcolorsIF* pUIcolors) : CXeD2DCtrlBase(pUIcolors)
	{
		if (!m_xeUI->RegisterWindowClass(XESCINTILLAEDITCONTROLWND_CLASSNAME, D2DCtrl_WndProc))
		{
			XeASSERT(FALSE);
		}
	}

	bool Create(DWORD dwEditControlStyle, HWND hParent, const CRect& rcCtrl, UINT uID, const wchar_t* tooltip)
	{
		DWORD dwScrollStyle = dwEditControlStyle & (WS_VSCROLL | WS_HSCROLL);
		dwEditControlStyle = dwEditControlStyle & ~(WS_VSCROLL | WS_HSCROLL);
		// Create parent window of edit control and the scroll bars.
		HWND hWnd = CreateD2DCtrl(0, XESCINTILLAEDITCONTROLWND_CLASSNAME, nullptr,
				dwEditControlStyle, rcCtrl, hParent, uID, tooltip);
		if (hWnd)
		{
			ShowWindow(SW_SHOW);

			CRect rc0;
			m_hScr = std::make_unique<CXeScrollBar>(m_xeUI);
			m_hScr->Create(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SBS_HORZ, rc0, hWnd, 2001, nullptr);
			m_vScr = std::make_unique<CXeScrollBar>(m_xeUI);
			m_vScr->Create(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SBS_VERT, rc0, hWnd, 2002, nullptr);

			XeWindowStyleValue ws(dwEditControlStyle, 0);
			const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
			m_mode.Initialize(m_control_has_border, tm.GetHeight(), !ws.isES_MultiLine());
			m_mode.SetClientRect(rcCtrl);

			const CRect& rcEC = m_mode.m_rcEditControl;
			HWND hWndParent = GetSafeHwnd();
			dwEditControlStyle |= WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
			dwEditControlStyle = dwEditControlStyle & ~WS_BORDER;
			m_hSciEdWnd = ::CreateWindowExW(0, L"Scintilla", L"EditCtrlWnd", dwEditControlStyle,
					rcEC.left, rcEC.top, rcEC.Width(), rcEC.Height(),
					hWndParent, 0, m_xeUI->GetInstanceHandle(), 0);
			m_editStyle.Set(m_hSciEdWnd, dwEditControlStyle, 0, m_xeUI);

			m_xeUI->RegisterScintillaKeyboardFilterCallback(m_hSciEdWnd,
					[this](const MSG& msg) { return _OnKeyboardFilter(msg); });

			// Note - uncomment following line to enable DirectWrite. (select not painted correctly, more flickering)
			//::SendMessage(m_hSciEdWnd, SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE, 0);

			::SendMessage(m_hSciEdWnd, SCI_SETMARGINS, 0, 0);

			UpdateColors();
			UpdateFont();

			::ShowWindow(m_hSciEdWnd, SW_SHOW);
		}
		return hWnd != 0;
	}

	void SetComboBoxKeyDownFilterCallback(KeyDownFilterCallback keyDownFilterFunc)
	{
		m_comboBoxKeydownFilterFunc = keyDownFilterFunc;
	}

	virtual void OnChangedUIsettings(bool isFontsChanged, bool isColorsChanged) override
	{
		if (isFontsChanged)
		{
			UpdateFont();
		}
		if (isColorsChanged)
		{
			UpdateColors();
		}
		_RedrawDirectly();
	}

protected:
	// Filter WM_KEYDOWN or WM_SYSKEYDOWN or WM_KEYUP or WM_SYSKEYUP or WM_CHAR
	// Called from within the message loop (in the main app or modal dialog message loop).
	bool _OnKeyboardFilter(const MSG& msg)
	{
		//bool isSingleLineEditControl = !m_style.isES_MultiLine();
		bool isSingleLineEditControl = !m_editStyle.WS().isES_MultiLine();
		CXeShiftCtrlAltKeyHelper sca;
		UINT uKey = (UINT)msg.wParam;
		if (isSingleLineEditControl)
		{
			if (m_comboBoxKeydownFilterFunc && m_comboBoxKeydownFilterFunc(uKey, 0, 0))
			{
				return true;	// Suppress key (already handled by ComboBox parent control).
			}
			if (sca.IsNoneDown()
					&& (msg.message == WM_KEYDOWN || msg.message == WM_CHAR) && uKey == VK_RETURN)
			{
				return true;	// Suppress key.
			}
			if (msg.message == WM_KEYDOWN
					&& (uKey == VK_UP || uKey == VK_DOWN || uKey == VK_PRIOR || uKey == VK_NEXT))
			{
				return true;	// Suppress key.
			}
		}
		return false;	// Process message normally
	}

	virtual LRESULT _OnDestroy() override
	{
		m_xeUI->UnRegisterScintillaKeyboardFilterCallback(m_hSciEdWnd);
		return 0;
	}

	// WM_SIZE message handler.
	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{
		UINT nType = (UINT)wParam;
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);

		CRect rc_cli(0, 0, cx, cy);
		m_mode.SetClientRect(rc_cli);
		if (m_hScr)
		{
			m_hScr->MoveWindow(m_mode.m_rcHscr, true);
		}
		if (m_vScr)
		{
			m_vScr->MoveWindow(m_mode.m_rcVscr, true);
		}
		if (m_hSciEdWnd)
		{
			rc_cli = m_mode.m_rcEditControl;
			::MoveWindow(m_hSciEdWnd, rc_cli.left, rc_cli.top, rc_cli.Width(), rc_cli.Height(), true);
		}
		return CXeD2DCtrlBase::_OnSize(hWnd, wParam, lParam);
	}

	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WM_GETTEXT:
			return _OnGetText(wParam, lParam);
		case WM_GETTEXTLENGTH:
			return _OnGetTextLength();
		case WMU_SCI_SETSCROLLINFO:
			return _OnSciSetScrollInfo(wParam, lParam);
		case WMU_SCI_GETSCROLLINFO:
			return _OnSciGetScrollInfo(wParam, lParam);
		}
		return CXeD2DCtrlBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender) override
	{
		if (sender == m_hSciEdWnd)
		{
			if (wSource == SCEN_SETFOCUS)	// Scintilla got focus.
			{
				::SendMessage(::GetParent(Hwnd()), WM_COMMAND,
						MAKEWPARAM(GetDlgCtrlID(), EN_SETFOCUS), reinterpret_cast<LPARAM>(Hwnd()));
			}
			else if (wSource == SCEN_KILLFOCUS)	// Scintilla lost focus.
			{
				::SendMessage(::GetParent(Hwnd()), WM_COMMAND,
						MAKEWPARAM(GetDlgCtrlID(), EN_KILLFOCUS), reinterpret_cast<LPARAM>(Hwnd()));
			}
		}
		return CXeD2DCtrlBase::_OnWmCommand(wSource, wID, sender);
	}

	virtual LRESULT _OnNotify(WPARAM wParam, LPARAM lParam) override
	{
		NMHDR* pNMhdr = reinterpret_cast<NMHDR*>(lParam);
		if (pNMhdr->code == SCN_SAVEPOINTLEFT)	// Text has been modified.
		{
			::SendMessage(::GetParent(Hwnd()), WM_COMMAND,
					MAKEWPARAM(GetDlgCtrlID(), EN_CHANGE), reinterpret_cast<LPARAM>(Hwnd()));
		}
		return CXeD2DCtrlBase::_OnNotify(wParam, lParam);
	}
#pragma endregion Create

#pragma region MFC_Functions_Replacements
public:
	void SetSel(int nStartChar, int nEndChar, BOOL bNoScroll = FALSE)
	{
		XeASSERT(::IsWindow(m_hSciEdWnd));
		::SendMessageW(m_hSciEdWnd, EM_SETSEL, (WPARAM)nStartChar, (LPARAM)nEndChar);
	}
	BOOL GetModify() const
	{
		XeASSERT(::IsWindow(m_hSciEdWnd));
		return (BOOL)::SendMessage(m_hSciEdWnd, SCI_GETMODIFY, 0, 0);
	}
	void SetModify(BOOL bModified)
	{
		XeASSERT(::IsWindow(m_hSciEdWnd));
		::SendMessage(Hwnd(), SCI_SETSAVEPOINT, bModified, 0);
	}
#pragma endregion MFC_Functions_Replacements

#pragma region SetText
public:
	void SetTextSuppressEnChangeNotify(const std::wstring& txt)
	{
		m_isSuppressEnChangeNotify = true;
		_OnSetText(0, (LPARAM)txt.c_str());
		m_isSuppressEnChangeNotify = false;
	}

protected:
	LRESULT _OnGetText(WPARAM wParam, LPARAM lParam)
	{
		return ::SendMessage(m_hSciEdWnd, WM_GETTEXT, wParam, lParam);
	}

	LRESULT _OnGetTextLength()
	{
		return ::SendMessage(m_hSciEdWnd, WM_GETTEXTLENGTH, 0, 0);
	}

	virtual LRESULT _OnSetText(WPARAM wParam, LPARAM lParam) override
	{
		const wchar_t* pWStr = (const wchar_t*)lParam;
		size_t len = wcslen(pWStr);
		std::wstring txtW(pWStr, len);
		std::string txtUTF8 = xet::toUTF8(txtW);
		SetText(txtUTF8.c_str(), txtUTF8.size(), false);
		_RedrawDirectly();
		return (TRUE);
	}

public:
	void SetText(const void* pText, size_t text_len, bool isReadOnly)
	{
		::SendMessage(m_hSciEdWnd, SCI_CLEARALL, 0, 0);
		::SendMessage(m_hSciEdWnd, EM_EMPTYUNDOBUFFER, 0, 0);
		::SendMessage(m_hSciEdWnd, SCI_SETSAVEPOINT, 0, 0);

		::SendMessage(m_hSciEdWnd, SCI_CANCEL, 0, 0);
		::SendMessage(m_hSciEdWnd, SCI_SETUNDOCOLLECTION, 0, 0);

		::SendMessage(m_hSciEdWnd, SCI_ADDTEXT, text_len, reinterpret_cast<LPARAM>(pText));
		::SendMessage(m_hSciEdWnd, SCI_SETHSCROLLBAR, 1, 0);
		::SendMessage(m_hSciEdWnd, SCI_SETVSCROLLBAR, 1, 0);
		::SendMessage(m_hSciEdWnd, SCI_SETSCROLLWIDTHTRACKING, 1, 0);
		::SendMessage(m_hSciEdWnd, SCI_SETSCROLLWIDTH, 5, 0);

		::SendMessage(m_hSciEdWnd, SCI_SETUNDOCOLLECTION, 1, 0);
		::SendMessage(m_hSciEdWnd, EM_EMPTYUNDOBUFFER, 0, 0);
		::SendMessage(m_hSciEdWnd, SCI_SETSAVEPOINT, 0, 0);
		if (isReadOnly || m_editStyle.WS().isES_readOnly())
		{
			::SendMessage(m_hSciEdWnd, SCI_SETREADONLY, 1, 0);
		}
		::SendMessage(m_hSciEdWnd, SCI_SETSAVEPOINT, 0, 0);	// Set text as "unmodified".
		::SendMessage(m_hSciEdWnd, SCI_GOTOPOS, 0, 0);
	}
#pragma endregion SetText

#pragma region SetFontAndColors
public:
	void UpdateColors()
	{
		const COLORREF bg = m_xeUI->GetColor(CID::GrdCellDefBg);
		const COLORREF fg = m_xeUI->GetColor(CID::GrdCellDefTxt);
		const COLORREF sel_bg = m_xeUI->GetColor(CID::GrdSelTxtBg);
		const COLORREF sel_fg = m_xeUI->GetColor(CID::GrdSelTxtFg);
		::SendMessage(m_hSciEdWnd, SCI_STYLESETFORE, STYLE_DEFAULT, fg);
		::SendMessage(m_hSciEdWnd, SCI_STYLESETBACK, STYLE_DEFAULT, bg);
		::SendMessage(m_hSciEdWnd, SCI_SETCARETFORE, fg, 0);
		::SendMessage(m_hSciEdWnd, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_TEXT, sel_fg);
		::SendMessage(m_hSciEdWnd, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK, sel_bg);
		::SendMessage(m_hSciEdWnd, SCI_STYLECLEARALL, 0, 0);	// Copies global style to all others
	}

	void UpdateFont()
	{
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		SetSciFont(tm.m_fontName.c_str(), tm.m_fontPointSize);
	}

	void SetSciFont(const wchar_t* pFontName, int size)
	{
		std::string fontname = xet::to_astr(pFontName);
		::SendMessage(m_hSciEdWnd, SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<LPARAM>(fontname.c_str()));
		::SendMessage(m_hSciEdWnd, SCI_STYLESETSIZE, STYLE_DEFAULT, size);
		::SendMessage(m_hSciEdWnd, SCI_STYLECLEARALL, 0, 0);	// Copies global style to all others
	}
#pragma endregion SetFontAndColors

#pragma region ScrollBars
protected:
	LRESULT _OnSciSetScrollInfo(WPARAM wParam, LPARAM lParam)
	{
		LPSCROLLINFO lpsi = (LPSCROLLINFO)lParam;
		int nBar = LOWORD(wParam);
		BOOL bRedraw = HIWORD(wParam);
		_AdjustScrollBars(nBar, lpsi);
		//XeTRACE("nBar: %d, fMask: %u, nMin: %d, nMax: %d, nPage: %u, nPos: %d, nTrackPos: %d\n",
		//	nBar, lpsi->fMask, lpsi->nMin, lpsi->nMax, lpsi->nPage, lpsi->nPos, lpsi->nTrackPos);
		if (nBar == 0)
		{
			m_hScr->SetScrollInfo(lpsi, bRedraw);
		}
		else
		{
			m_vScr->SetScrollInfo(lpsi, bRedraw);
		}
		return 0;
	}

	void _AdjustScrollBars(int nBar, LPSCROLLINFO lpsi)
	{
		int range = (lpsi->nMax - lpsi->nMin) + 1;
		bool isSBNeeded = range > (int)lpsi->nPage;
		bool isChanged = nBar == 0 ? m_mode.SetHScrollBar(isSBNeeded) : m_mode.SetVScrollBar(isSBNeeded);
		if (isChanged)
		{
			CRect rc_cli = _GetClientRect();
			m_mode.SetClientRect(rc_cli);
			if (m_mode.HasHScrollBar())
			{
				m_hScr->MoveWindow(m_mode.m_rcHscr, true);
				m_hScr->ShowWindow(SW_SHOW);
			}
			else
			{
				m_hScr->ShowWindow(SW_HIDE);
			}
			if (m_mode.HasVScrollBar())
			{
				m_vScr->MoveWindow(m_mode.m_rcVscr, true);
				m_vScr->ShowWindow(SW_SHOW);
			}
			else
			{
				m_vScr->ShowWindow(SW_HIDE);
			}
			rc_cli = m_mode.m_rcEditControl;
			::MoveWindow(m_hSciEdWnd, rc_cli.left, rc_cli.top, rc_cli.Width(), rc_cli.Height(), true);
		}
	}

	LRESULT _OnSciGetScrollInfo(WPARAM wParam, LPARAM lParam)
	{
		LPSCROLLINFO lpsi = (LPSCROLLINFO)lParam;
		int nBar = LOWORD(wParam);
		if (nBar == 0)
		{
			m_hScr->GetScrollInfo(lpsi);
		}
		else
		{
			m_vScr->GetScrollInfo(lpsi);
		}
		return 1;
	}

	//LRESULT _OnSci_SB_H(WPARAM wParam, LPARAM lParam)
	virtual LRESULT _OnHScroll(WPARAM wParam, LPARAM lParam) override
	{
		::SendMessage(m_hSciEdWnd, WM_HSCROLL, wParam, 0);
		return 0;
	}

	//LRESULT _OnSci_SB_V(WPARAM wParam, LPARAM lParam)
	virtual LRESULT _OnVScroll(WPARAM wParam, LPARAM lParam) override
	{
		::SendMessage(m_hSciEdWnd, WM_VSCROLL, wParam, 0);
		return 0;
	}
#pragma endregion ScrollBars

	virtual LRESULT _OnSetFocus(HWND hOldWnd) override
	{
		//TRACE("CXeScintillaEditControl control got focus\n");
		::SetFocus(m_hSciEdWnd);

		::SendMessage(::GetParent(Hwnd()), WMU_NOTIFY_GOT_FOCUS, (WPARAM)GetSafeHwnd(), (LPARAM)hOldWnd);
		return 0;
	}

	virtual LRESULT _OnKillFocus(HWND hNewWnd) override
	{
		//TRACE("CXeScintillaEditControl control lost focus\n");

		::SendMessage(::GetParent(Hwnd()), WMU_NOTIFY_LOST_FOCUS, (WPARAM)GetSafeHwnd(), (LPARAM)hNewWnd);
		return 0;
	}

	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{
		if (m_mode.m_rcEditControl.PtInRect(point))
		{
			return 0;
		}
		return CXeD2DCtrlBase::_OnMouseMove(nFlags, point);
	}

	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override
	{
		CPoint point;
		::GetCursorPos(&point);
		::ScreenToClient(Hwnd(), &point);
		if (m_mode.m_rcEditControl.PtInRect(point))
		{
			return 0;
		}
		return CXeD2DCtrlBase::_OnMouseLeave(wparam, lparam);
	}

#pragma region Paint
protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		// Fill empty area above and below the edit control window.
		CRect rcClient = _GetClientRect();
		const CRect& rcEC = m_mode.m_rcEditControl;
		CRect rcTop = rcClient, rcBtm = rcClient;
		rcTop.bottom = rcEC.top;
		if (rcTop.Height())
		{
			pRT->FillRectangle(RectFfromRect(rcTop), GetBrush(CID::GrdCellDefBg));
		}
		rcBtm.top = rcEC.bottom;
		if (rcBtm.Height())
		{
			pRT->FillRectangle(RectFfromRect(rcBtm), GetBrush(CID::GrdCellDefBg));
		}

		if (m_mode.HasBothScrollBars())
		{
			pRT->FillRectangle(RectFfromRect(m_mode.m_rcCorner), GetBrush(CID::CtrlBg));
		}

		if (m_control_has_border)
		{
			rc.left += 0.5f;
			rc.right -= 0.5f;
			rc.top += 0.5f;
			rc.bottom -= 0.5f;
			pRT->DrawRectangle(rc, GetBrush(CID::CtrlBorder));
		}
	}
#pragma endregion Paint
};


