/*************************************************************************
				Class Implementation : CUGEdit
**************************************************************************
	Source file : ugedit.cpp
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved
*************************************************************************/

#include <string>
#include <ctype.h>
#include <time.h>
#include "../os_minimal.h"
#include "..\scintilla\include\Scintilla.h"
#include "ugdefine.h"
#include "UGCelTyp.h"
#include "UGEdit.h"
#include "uggdinfo.h"
import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Name of property that is stored in the "window" handle by Windows.
// We store the original window proc pointer - when we "subclass" the control.
constexpr wchar_t XEPROP_ORIGWNDPROC[] = L"XePropOrigWndProc";
LRESULT CALLBACK EditCtrlSubclass_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

CUGEdit::CUGEdit() : m_continueCol(0), m_continueRow(0), m_continueFlag(0), m_cancel(0), m_lastKey(0) {}
CUGEdit::~CUGEdit() {}

bool CUGEdit::CreateUGEdit(const CRect& rect, HWND hParentWnd, UINT uID, bool isUseScintilla)
{
	XeASSERT(m_GI);

	m_isUseScintilla = isUseScintilla;

	HINSTANCE hInstance = m_GI->m_xeUI->GetInstanceHandle();

	if (m_isUseScintilla)
	{
		DWORD dwStyle = WS_CHILD | ES_AUTOHSCROLL/*WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL*/;
		m_hWnd = ::CreateWindowExW(0, L"Scintilla", nullptr, dwStyle,
			rect.left, rect.top, rect.Width(), rect.Height(),
			hParentWnd, (HMENU)(uint64_t)uID, hInstance, this);

		// Note - uncomment following line to enable DirectWrite. (select not painted correctly, more flickering)
		//::SendMessage(m_hWnd, SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE, 0);

		::SendMessage(m_hWnd, SCI_SETMARGINS, 0, 0);

		_UpdateScintillaColors();
		_UpdateScintillaFont();
	}
	else
	{
		DWORD dwStyle = WS_CHILD | ES_AUTOHSCROLL/*WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL*/;
		m_hWnd = ::CreateWindowExW(0, L"Edit", nullptr, dwStyle,
			rect.left, rect.top, rect.Width(), rect.Height(),
			hParentWnd, (HMENU)(uint64_t)uID, hInstance, this);
	}

	::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LPARAM)this);

	WNDPROC origWndProc = (WNDPROC)::SetWindowLongPtrW(m_hWnd, GWLP_WNDPROC, (DWORD_PTR)EditCtrlSubclass_WndProc);

	::SetPropW(m_hWnd, XEPROP_ORIGWNDPROC, origWndProc);

	return m_hWnd != 0;
}

#pragma region SetScintillaText
	LRESULT CUGEdit::_OnSetText(WPARAM wParam, LPARAM lParam)
	{
		const wchar_t* pWStr = (const wchar_t*)lParam;
		size_t len = wcslen(pWStr);
		std::wstring txtW(pWStr, len);
		std::string txtUTF8 = xet::toUTF8(txtW);
		_SetText(txtUTF8.c_str(), txtUTF8.size(), false);
		_UpdateScintillaColors();
		_UpdateScintillaFont();
		::RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_NOCHILDREN | RDW_UPDATENOW);
		return (TRUE);
	}

	void CUGEdit::_SetText(const void* pText, size_t text_len, bool isReadOnly)
	{
		::SendMessage(m_hWnd, SCI_CLEARALL, 0, 0);
		::SendMessage(m_hWnd, EM_EMPTYUNDOBUFFER, 0, 0);
		::SendMessage(m_hWnd, SCI_SETSAVEPOINT, 0, 0);

		::SendMessage(m_hWnd, SCI_CANCEL, 0, 0);
		::SendMessage(m_hWnd, SCI_SETUNDOCOLLECTION, 0, 0);

		::SendMessage(m_hWnd, SCI_ADDTEXT, text_len, reinterpret_cast<LPARAM>(pText));
		::SendMessage(m_hWnd, SCI_SETHSCROLLBAR, 1, 0);
		::SendMessage(m_hWnd, SCI_SETVSCROLLBAR, 1, 0);
		::SendMessage(m_hWnd, SCI_SETSCROLLWIDTHTRACKING, 1, 0);
		::SendMessage(m_hWnd, SCI_SETSCROLLWIDTH, 5, 0);

		::SendMessage(m_hWnd, SCI_SETUNDOCOLLECTION, 1, 0);
		::SendMessage(m_hWnd, EM_EMPTYUNDOBUFFER, 0, 0);
		::SendMessage(m_hWnd, SCI_SETSAVEPOINT, 0, 0);
		if (isReadOnly)
		{
			::SendMessage(m_hWnd, SCI_SETREADONLY, 1, 0);
		}
		::SendMessage(m_hWnd, SCI_GOTOPOS, 0, 0);
	}
#pragma endregion SetScintillaText

#pragma region SetScintillaFontAndColors
	void CUGEdit::_UpdateScintillaColors()
	{
		const COLORREF bg = m_GI->m_xeUI->GetColor(CID::GrdCellDefBg);
		const COLORREF fg = m_GI->m_xeUI->GetColor(CID::GrdCellDefTxt);
		const COLORREF sel_bg = m_GI->m_xeUI->GetColor(CID::GrdSelTxtBg);
		const COLORREF sel_fg = m_GI->m_xeUI->GetColor(CID::GrdSelTxtFg);
		::SendMessage(m_hWnd, SCI_STYLESETFORE, STYLE_DEFAULT, fg);
		::SendMessage(m_hWnd, SCI_STYLESETBACK, STYLE_DEFAULT, bg);
		::SendMessage(m_hWnd, SCI_SETCARETFORE, fg, 0);
		::SendMessage(m_hWnd, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_TEXT, sel_fg);
		::SendMessage(m_hWnd, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK, sel_bg);
		::SendMessage(m_hWnd, SCI_STYLECLEARALL, 0, 0);	// Copies global style to all others
	}

	void CUGEdit::_UpdateScintillaFont()
	{
		const XeFontMetrics& tm = m_GI->m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		_SetScintillaFont(tm.m_fontName.c_str(), tm.m_fontPointSize);
	}

	void CUGEdit::_SetScintillaFont(const wchar_t* pFontName, int size)
	{
		std::string fontname = xet::to_astr(pFontName);
		::SendMessage(m_hWnd, SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<LPARAM>(fontname.c_str()));
		::SendMessage(m_hWnd, SCI_STYLESETSIZE, STYLE_DEFAULT, size);
		::SendMessage(m_hWnd, SCI_STYLECLEARALL, 0, 0);	// Copies global style to all others
	}
#pragma endregion SetScintillaFontAndColors

std::tuple<bool, LRESULT> CUGEdit::_OnMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	XeASSERT(m_GI);
	switch (message)
	{
	case WM_SETTEXT:
		if (m_isUseScintilla)
		{
			_OnSetText(wParam, lParam);
			return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.
		}
		break;

	case WM_SETFOCUS:
		::SendMessageW(m_hWnd, EM_SETSEL, 0, -1);

		m_lastKey = 0;
		m_cancel = FALSE;
		m_continueFlag = FALSE;
		break;

	case WM_KILLFOCUS: {
		HWND hNewWnd = (HWND)wParam;
		if (m_GI->m_bCancelMode != FALSE)
		{
			if (hNewWnd != NULL)
			{
				if (hNewWnd != m_GI->m_ctrlWnd && ::GetParent(hNewWnd) != m_GI->m_ctrlWnd)
				{
					m_cancel = TRUE;
				}
			}
			else
			{
				m_cancel = TRUE;
			}
		}

		if (hNewWnd != m_GI->m_gridWnd)
		{
			m_GI->OnKillFocusNewWnd(UG_GRID, hNewWnd);
		}

		if (hNewWnd != NULL)
		{
			if (hNewWnd == m_GI->m_gridWnd)
			{
				::SetFocus(m_GI->m_gridWnd);
			}
		}

		std::wstring str = GetWindowTextStdW(m_hWnd);

		if (m_GI->EditCtrlFinished(str.c_str(), m_cancel, m_continueFlag, m_continueCol, m_continueRow) == FALSE)
		{
			m_GI->GotoCell(m_GI->m_editCol, m_GI->m_editRow);
			::SetCapture(m_hWnd);
			::ReleaseCapture();
			::SetFocus(m_hWnd);
		}
		} break;

	case WM_KEYDOWN:
		m_lastKey = (UINT)wParam;

		switch (wParam)
		{
		case VK_TAB:
			if (GetKeyState(VK_SHIFT) < 0)
			{
				m_continueCol = m_GI->m_editCol - 1;
				m_continueRow = m_GI->m_editRow;
			}
			else
			{
				m_continueCol = m_GI->m_editCol + 1;
				m_continueRow = m_GI->m_editRow;
			}

			m_continueFlag = TRUE;
			::SetFocus(m_GI->m_gridWnd);
			return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.

		case VK_RETURN:
			if (GetKeyState(VK_CONTROL) < 0 &&
				m_GI->m_editCell.GetCellTypeEx() & UGCT_NORMALMULTILINE)
			{	// in mulitiline cells allow CTRL-RETURN to advance to a new line
				return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.
			}

			m_continueCol = m_GI->m_editCol;
			m_continueRow = m_GI->m_editRow + 1;
			m_continueFlag = TRUE;
			::SetFocus(m_GI->m_gridWnd);
			return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.

		case VK_ESCAPE:
			m_cancel = TRUE;
			m_continueFlag = FALSE;
			::SetFocus(m_GI->m_gridWnd);
			return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.

		case VK_UP:
			if (m_GI->m_editCell.GetCellTypeEx() & UGCT_NORMALMULTILINE)
			{	// in mulitiline cells allow UP key to advance to next line above
				break;
			}
			m_continueCol = m_GI->m_editCol;
			m_continueRow = m_GI->m_editRow - 1;
			m_continueFlag = TRUE;
			::SetFocus(m_GI->m_gridWnd);
			return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.

		case VK_DOWN:
			if (m_GI->m_editCell.GetCellTypeEx() & UGCT_NORMALMULTILINE)
			{	// in mulitiline cells allow UP key to advance to next line below
				break;
			}
			m_continueCol = m_GI->m_editCol;
			m_continueRow = m_GI->m_editRow + 1;
			m_continueFlag = TRUE;
			::SetFocus(m_GI->m_gridWnd);
			return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.

		case VK_DELETE: {
			// The OnEditVerify should also called when user enters
			// the DELETE key.  ASCII representation of the DEL key is decimal 127
			UINT nChar = 127;
			int col = m_GI->GetCurrentCol();
			long row = m_GI->GetCurrentRow();

			// First the datasource has a look ...
			if (m_GI->m_defDataSource->OnEditVerify(col, row, m_hWnd, &nChar) == FALSE
					|| m_GI->OnEditVerify(col, row, m_hWnd, &nChar) == FALSE)
			{
				return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.
			}
			} break;
		}	// endof: switch (wParam)
		break;

	case WM_CHAR: {
		UINT nChar = (UINT)wParam;
		m_lastKey = nChar;

		if (!(nChar == VK_TAB || nChar == VK_RETURN || nChar == VK_ESCAPE))
		{
			// Allow OnEditVerify a look at the char... First the datasource has a look ... then the ctrl class
			int col = m_GI->GetCurrentCol();
			long row = m_GI->GetCurrentRow();
			if (m_GI->m_defDataSource->OnEditVerify(col, row, m_hWnd, &nChar) == FALSE
					|| m_GI->OnEditVerify(col, row, m_hWnd, &nChar) == FALSE)
			{
				return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.
			}
		}
		} break;

	case WM_LBUTTONDBLCLK:
		m_lastDblClickTicks = clock();
		break;

	case WM_LBUTTONDOWN: {
		clock_t dblClickTicks = GetDoubleClickTime() * CLOCKS_PER_SEC / 1000;
		clock_t nowTicks = clock();
		clock_t ticksSinceLastDblClick = nowTicks - m_lastDblClickTicks;
		if (ticksSinceLastDblClick < dblClickTicks) // Is Triple click detected?
		{
			::SendMessageW(m_hWnd, EM_SETSEL, 0, -1);
			m_lastDblClickTicks = 0;
		}
		} break;
	}
	return std::make_tuple(true, 0);	// Allow the edit control to process the message.
}

LRESULT CALLBACK EditCtrlSubclass_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC origWndProc = (WNDPROC)::GetPropW(hwnd, XEPROP_ORIGWNDPROC);

	CUGEdit* pThis = (CUGEdit*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
	XeASSERT(origWndProc != nullptr && pThis != nullptr);

	if (!(origWndProc && pThis))
	{
		return ::DefWindowProc(hwnd, msg, wParam, lParam);
	}
	if (msg == WM_NCDESTROY)
	{
		// Restore original window proc pointer and delete the special property.
		::SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<INT_PTR>(origWndProc));
		::RemovePropW(hwnd, XEPROP_ORIGWNDPROC);
	}

	auto [isProcessByEditCtrl, lResult] = pThis->_OnMessage(hwnd, msg, wParam, lParam);

	if (isProcessByEditCtrl)
	{
		lResult = ::CallWindowProcW(origWndProc, hwnd, msg, wParam, lParam);
	}
	return lResult;
}

