module;

#include "os_minimal.h"
#include <memory>
#include <string>
#include <vector>

export module Xe.ListBoxEx;

import Xe.UIcolorsIF;
import Xe.ScrollBar;
import Xe.ListBoxExCommon;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

constexpr wchar_t XELISTBOXEXWND_CLASSNAME[] = L"XeListBoxExWndClass";  // Window class name

export class CXeListBoxEx : public CXeListBoxExCommon
{
#pragma region Create
public:
	CXeListBoxEx(CXeUIcolorsIF* pUIcolors) : CXeListBoxExCommon(pUIcolors)
	{
		m_xeUI->RegisterWindowClass(XELISTBOXEXWND_CLASSNAME, D2DCtrl_WndProc);
	}

	BOOL Create(DWORD dwStyle, DWORD dwExStyle, const CRect& rect, HWND hParentWnd, UINT nID)
	{
		dwStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		PreCreateWindow();
		if (!::IsWindow(hParentWnd))
		{
			XeASSERT(FALSE);
			return FALSE;
		}
		dwStyle &= (~(WS_VSCROLL | WS_HSCROLL));
		HWND hWnd = CreateD2DWindow(dwExStyle, XELISTBOXEXWND_CLASSNAME, nullptr, dwStyle,
				rect, hParentWnd, nID);
		return hWnd != 0;
	}
#pragma endregion Create

#pragma region CustomListBoxSupport
public:
	// Set tab stops as pixel width.
	void SetTabStops(const std::vector<uint32_t>& tab_stops)
	{
		m_tab_stops = tab_stops;
	}
#pragma endregion CustomListBoxSupport

#pragma region CListBox_MFCfunctions
public:
int GetCount() const
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), LB_GETCOUNT, 0, 0); }
int GetCurSel() const
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), LB_GETCURSEL, 0, 0); }
int SetCurSel(int nSelect)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), LB_SETCURSEL, nSelect, 0); }
int AddString(LPCTSTR lpszString)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), LB_ADDSTRING, 0, (LPARAM)lpszString); }
int DeleteString(UINT nIndex)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), LB_DELETESTRING, nIndex, 0);}
int InsertString(int nIndex, LPCTSTR lpszString)
	{ XeASSERT(::IsWindow(Hwnd())); return (int)::SendMessage(Hwnd(), LB_INSERTSTRING, nIndex, (LPARAM)lpszString); }
void ResetContent()
	{ XeASSERT(::IsWindow(Hwnd())); ::SendMessage(Hwnd(), LB_RESETCONTENT, 0, 0); }
void SetItemData(UINT nIndex, DWORD_PTR dwData)
	{ XeASSERT(::IsWindow(Hwnd())); ::SendMessage(Hwnd(), LB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)dwData); }
#pragma endregion CListBox_MFCfunctions
};
