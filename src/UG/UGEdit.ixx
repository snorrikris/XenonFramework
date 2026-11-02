module;

/*************************************************************************
				Class Implementation : CUGEdit
**************************************************************************
	Source file : ugedit.cpp
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved
*************************************************************************/
/*************************************************************************
				Class Declaration : CUGEdit
**************************************************************************
	Source file : UGEdit.cpp
	Header file : UGEdit.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		The Ultimate grid uses the CUGEdit class
		as the default edit control.  An instance
		of this class is created every time an instance
		of the Ultimate Grid is created.

		It is possible to specify new edit control
		for the Ultimate Grid to use, you can do this
		either with the use of the OnEditStart notification
		or SetNewEditClass.  Each of these methods has
		its benefits and please refer to the documentation
		for more information.

	Key features
		-This control automatically expands to the
		 right as text is entered, once the control
		 reaches the right side of the grid then it
		 expands downward until it reaches the bottom.
		 Once it reaches the bottom then it will start
		 scrolling text as it is entered.
		-When editing first starts the control also
		 automatically expands to fit the inital text.
*************************************************************************/

#include <string>
#include <ctype.h>
#include <time.h>
#include "../os_minimal.h"
#include "..\scintilla\include\Scintilla.h"
#include "ugdefine.h"

export module Xe.UGEdit;

import std;
import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.StringTools;
import Xe.UGGridInfoIF;
import Xe.UGCelTyp;
import Xe.ScintillaEditControl;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export class CUGEdit
{
public:
	CUGGridInfoIF* m_GI = nullptr;

protected:
	// based on user action destination cell needs to be set 
	int m_continueCol = 0;
	long m_continueRow = 0;
	BOOL m_cancel = FALSE;
	BOOL m_continueFlag = FALSE;

	// member variable that keeps track of the last pressed key
	UINT m_lastKey = 0;

	clock_t m_lastDblClickTicks = 0;

	std::unique_ptr<CXeScintillaEditControl> m_scintilla;

public:
	CUGEdit() {}
	~CUGEdit() {}

	bool CreateUGEdit(const CRect& rect, HWND hParentWnd, UINT uID/*, bool isUseScintilla*/)
	{
		XeASSERT(m_GI);

		m_scintilla = std::make_unique<CXeScintillaEditControl>(m_GI->GetXeUI());
		m_scintilla->Create(WS_CHILD | ES_AUTOHSCROLL, hParentWnd, rect, uID, L"");
		m_scintilla->SetProcessScintillWndMessageCallback(
				[this](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
				{ return _OnMessage(hwnd, msg, wParam, lParam); });
		return m_scintilla->GetSafeHwnd() != 0;
	}

	HWND GetEditControlWindowHandle() const
	{
		return m_scintilla->GetSafeHwnd();
	}

public:
	std::tuple<bool, LRESULT> _OnMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		XeASSERT(m_GI);
		switch (message)
		{
		case WM_SETFOCUS:
			::SendMessageW(hWnd, EM_SETSEL, 0, -1);

			m_lastKey = 0;
			m_cancel = FALSE;
			m_continueFlag = FALSE;
			break;

		case WM_KILLFOCUS: {
			HWND hNewWnd = (HWND)wParam;
			if (m_GI->CancelMode() != FALSE)
			{
				if (hNewWnd != NULL)
				{
					if (hNewWnd != m_GI->CtrlWnd() && ::GetParent(hNewWnd) != m_GI->CtrlWnd())
					{
						m_cancel = TRUE;
					}
				}
				else
				{
					m_cancel = TRUE;
				}
			}

			if (hNewWnd != m_GI->GridWnd())
			{
				m_GI->OnKillFocusNewWnd(UG_GRID, hNewWnd);
			}

			if (hNewWnd != NULL)
			{
				if (hNewWnd == m_GI->GridWnd())
				{
					::SetFocus(m_GI->GridWnd());
				}
			}

			std::wstring str = GetWindowTextStdW(hWnd);

			if (m_GI->EditCtrlFinished(str.c_str(), m_cancel, m_continueFlag, m_continueCol, m_continueRow) == FALSE)
			{
				m_GI->GotoCell(m_GI->EditCol(), m_GI->EditRow());
				::SetCapture(hWnd);
				::ReleaseCapture();
				::SetFocus(hWnd);
			}
		} break;

		case WM_KEYDOWN:
			m_lastKey = (UINT)wParam;

			switch (wParam)
			{
			case VK_TAB:
				if (GetKeyState(VK_SHIFT) < 0)
				{
					m_continueCol = m_GI->EditCol() - 1;
					m_continueRow = m_GI->EditRow();
				}
				else
				{
					m_continueCol = m_GI->EditCol() + 1;
					m_continueRow = m_GI->EditRow();
				}

				m_continueFlag = TRUE;
				::SetFocus(m_GI->GridWnd());
				return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.

			case VK_RETURN:
				if (GetKeyState(VK_CONTROL) < 0 &&
					m_GI->EditCell().GetCellTypeEx() & UGCT_NORMALMULTILINE)
				{	// in mulitiline cells allow CTRL-RETURN to advance to a new line
					return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.
				}

				m_continueCol = m_GI->EditCol();
				m_continueRow = m_GI->EditRow() + 1;
				m_continueFlag = TRUE;
				::SetFocus(m_GI->GridWnd());
				return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.

			case VK_ESCAPE:
				m_cancel = TRUE;
				m_continueFlag = FALSE;
				::SetFocus(m_GI->GridWnd());
				return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.

			case VK_UP:
				if (m_GI->EditCell().GetCellTypeEx() & UGCT_NORMALMULTILINE)
				{	// in mulitiline cells allow UP key to advance to next line above
					break;
				}
				m_continueCol = m_GI->EditCol();
				m_continueRow = m_GI->EditRow() - 1;
				m_continueFlag = TRUE;
				::SetFocus(m_GI->GridWnd());
				return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.

			case VK_DOWN:
				if (m_GI->EditCell().GetCellTypeEx() & UGCT_NORMALMULTILINE)
				{	// in mulitiline cells allow UP key to advance to next line below
					break;
				}
				m_continueCol = m_GI->EditCol();
				m_continueRow = m_GI->EditRow() + 1;
				m_continueFlag = TRUE;
				::SetFocus(m_GI->GridWnd());
				return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.

			case VK_DELETE: {
				// The OnEditVerify should also called when user enters
				// the DELETE key.  ASCII representation of the DEL key is decimal 127
				UINT nChar = 127;
				int col = m_GI->GetCurrentCol();
				long row = m_GI->GetCurrentRow();

				// First the datasource has a look ...
				if (m_GI->DefDataSource()->OnEditVerify(col, row, hWnd, &nChar) == FALSE
						|| m_GI->OnEditVerify(col, row, hWnd, &nChar) == FALSE)
				{
					return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.
				}
			} break;
			case VK_PRIOR:
			case VK_NEXT:
				return std::make_tuple(false, 0);	// Don't allow the edit control to process the message.
			}	// endof: switch (wParam)
			break; // endof: case WM_KEYDOWN

		case WM_CHAR: {
			UINT nChar = (UINT)wParam;
			m_lastKey = nChar;

			if (!(nChar == VK_TAB || nChar == VK_RETURN || nChar == VK_ESCAPE))
			{
				// Allow OnEditVerify a look at the char... First the datasource has a look ... then the ctrl class
				int col = m_GI->GetCurrentCol();
				long row = m_GI->GetCurrentRow();
				if (m_GI->DefDataSource()->OnEditVerify(col, row, hWnd, &nChar) == FALSE
						|| m_GI->OnEditVerify(col, row, hWnd, &nChar) == FALSE)
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
				::SendMessageW(hWnd, EM_SETSEL, 0, -1);
				m_lastDblClickTicks = 0;
			}
		} break;
		}
		return std::make_tuple(true, 0);	// Allow the edit control to process the message.
	}
};

