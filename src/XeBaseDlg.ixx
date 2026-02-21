module;

#include "os_minimal.h"
#include <dwmapi.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "XeResource.h"

export module Xe.BaseDlg;

import Xe.D2DWndBase;
import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.UserSettings;
import Xe.Grid;
import Xe.GridDataSource;
import Xe.GridTableIF;
import Xe.ViewManagerIF;
import Xe.DialogTemplateEx;
import Xe.D2DButton;
import Xe.D2DStatic;
import Xe.ComboBoxEx;
import Xe.ListBoxEx;
import Xe.ScintillaEditControl;
import Xe.ScrollBar;
import Xe.TreeCtrlEx;
import Xe.ProgressCtrl;
import Xe.DurationCtrl;
import Xe.CalendarCtrl;
import Xe.ColorPicker;
import Xe.FileTimeX;

#pragma comment (lib, "Dwmapi")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma region Helpers
enum class ECtrlType
{
	Invalid,
	CheckBox,
	RadioButton,
	GroupBox,
	StdButton,
	OwDrButton,
	StaticCtrl,
	StaticLabel,		// has IDC_STATIC is.
	StaticIcon,
	DataGrid,
	SysDateTimePick32, 	// Date/time picker
	msctls_updown32,	// SpinButtons
	ComboBox,
	ListBox,
	EditCtrl,
	EditControlMultiline,
	ScrollBar,
	ProgressCtrl,
	TreeCtrl,
	ColorButton
};

ECtrlType GetControlType(HWND hwndChild)
{
	int nIDC = ::GetWindowLong(hwndChild, GWL_ID);
	int nStyle = ::GetWindowLong(hwndChild, GWL_STYLE);
	int nExStyle = ::GetWindowLong(hwndChild, GWL_EXSTYLE);

	std::string strClassName = GetWindowClassName(hwndChild);

	HWND hWndParent = ::GetParent(hwndChild);

	if (strClassName == "Button" || strClassName == "XeD2DButtonWndClass")
	{
		int nBtnType = nStyle & 0x0000000F;
		if (nBtnType == BS_CHECKBOX
			|| nBtnType == BS_AUTOCHECKBOX
			|| nBtnType == BS_3STATE
			|| nBtnType == BS_AUTO3STATE)
		{	// Button is checkbox
			return ECtrlType::CheckBox;
		}
		else if (nBtnType == BS_RADIOBUTTON
			|| nBtnType == BS_AUTORADIOBUTTON)
		{	// Button is radio button
			return ECtrlType::RadioButton;
		}
		else if (nBtnType == BS_GROUPBOX)
		{	// Button is group box
			return ECtrlType::GroupBox;
		}
		else
		{	// Normal push button
			if (nBtnType != BS_OWNERDRAW)	// All buttons that are NOT owner draw
			{
				return ECtrlType::StdButton;
			}
			return ECtrlType::OwDrButton;
		}
	}
	else if (strClassName == "Static" || strClassName == "XeD2DStaticWndClass")
	{
		int ss_style = nStyle & SS_TYPEMASK;
		bool isIDC_STATIC = nIDC == IDC_STATIC;
		bool isIconBitmap = ss_style == SS_BITMAP || ss_style == SS_ICON || ss_style == SS_ENHMETAFILE;
		if (!(isIDC_STATIC || isIconBitmap))	// not IDC_STATIC, not Icon/bitmap ?
		{
			return ECtrlType::StaticCtrl;
		}
		else if (isIconBitmap)
		{
			return ECtrlType::StaticIcon;
		}
		else if (isIDC_STATIC)
		{
			return ECtrlType::StaticLabel;
		}
	}
	else if (strClassName == "SysDateTimePick32") 	// Date/time picker
	{
		return ECtrlType::SysDateTimePick32;
	}
	//else if (strClassName == "msctls_updown32") 		// SpinButtons
	//{
	//	//pThis->m_sc_UpDownControls.AddNew(nIDC, hwndChild);
	//	pThis->m_static_label.push_back(hwndChild);
	//}
	else if (strClassName == "ComboBox" || strClassName == "XeComboBoxExWndClass") 			// Combo box
	{
		return ECtrlType::ComboBox;
	}
	else if (strClassName == "ListBox" || strClassName == "XeListBoxExWndClass")				// List box
	{
		return ECtrlType::ListBox;
	}
	else if (strClassName == "Edit" || strClassName == "XeScintillaEditControlWndClass"
		|| strClassName == "Scintilla")
	{
		if (nStyle & ES_MULTILINE)
		{
			return ECtrlType::EditControlMultiline;
		}
		return ECtrlType::EditCtrl;
	}
	else if (strClassName == "msctls_progress32")
	{
		return ECtrlType::ProgressCtrl;
	}
	else if (strClassName == "SysTreeView32")
	{
		return ECtrlType::TreeCtrl;
	}
	else if (strClassName == "MfcColorButton")
	{
		return ECtrlType::ColorButton;
	}
	else if (strClassName == "CUGCtrl_WNDCLASS")
	{
		return ECtrlType::DataGrid;
	}
	else
	{
		//XeASSERT(FALSE);	// Unknown control type - need to add support?
		return ECtrlType::Invalid;
	}
	return ECtrlType::Invalid;
}

CRect GetControlRectInParentCoordinates(HWND hWnd)
{
	XeASSERT(hWnd);
	if (!(hWnd)) { return CRect(); }

	CRect rect;
	::GetWindowRect(hWnd, &rect);
	HWND hWndParent = ::GetParent(hWnd);
	::ScreenToClient(hWndParent, (LPPOINT)&rect);
	::ScreenToClient(hWndParent, ((LPPOINT)&rect) + 1);
	return rect;
}

struct WndZorder
{
	int zOrder = 0;
	ECtrlType ctrlType;
	CRect rcCtrl;	// window rect in parent coordinates.
	HWND hWnd = 0;
	UINT uCtrlID = 0;

	WndZorder(HWND wnd, UINT id, int zord, ECtrlType tp, const CRect& rc)
		: hWnd(wnd), uCtrlID(id), zOrder(zord), ctrlType(tp), rcCtrl(rc) {
	}
};

std::vector<WndZorder> GetChildControlsInZorder(HWND hParentWnd)
{
	std::vector<WndZorder> wnds;
	HWND hWnd = ::GetTopWindow(hParentWnd);
	int zOrder = 1;
	while (hWnd != 0)
	{
		UINT id = ::GetDlgCtrlID(hWnd);
		wnds.push_back(WndZorder(hWnd, id, zOrder++, GetControlType(hWnd),
			GetControlRectInParentCoordinates(hWnd)));

		hWnd = ::GetNextWindow(hWnd, GW_HWNDNEXT);
	}
	return wnds;
}
#pragma endregion Helpers

constexpr UINT c_uDialogWantsClickOutsideMouseDownMessage = (WM_USER + 0x7000);

constexpr wchar_t XEDIALOGWND_CLASSNAME[] = L"XeDialogWndClass";  // Window class name

export class CXeBaseDlg : public CXeD2DWndBase
{
#pragma region class_data
protected:
	const wchar_t* m_lpszTemplateName = nullptr;     // name or MAKEINTRESOURCE
	HWND m_hDlgParentWnd;

	bool m_isModelessDialog = false;

	INT_PTR m_dlg_result = 0;
	bool m_isDialogEnded = false;	// true after EndManualModeDialog called.

	CXeViewManagerIF* m_pVwMgr = nullptr;

	CRect m_rectCell;
	bool m_isRepositionDlgToRectCell = false;

	std::wstring m_settingsName;			// Set by derived class (before OnInitDialog is called).
	bool m_isSaveRecallDlgPos = false;	// true if dlg should be positioned to last saved position.

	bool m_isSizing = false, m_isMoving = false;

	int m_nDialogMinX = 0, m_nDialogMinY = 0;

	bool m_isDialogWantsClickOutsideMouseDownToCancel = false;

	// All controls - note - every control is derived from CXeD2DCtrlBase.
	std::vector<std::unique_ptr<CXeD2DButton>>				m_buttons;
	std::vector<std::unique_ptr<CXeD2DStatic>>				m_statics;
	std::vector<std::unique_ptr<CXeListBoxEx>>				m_listboxes;
	std::vector<std::unique_ptr<CXeComboBoxEx>>				m_comboboxes;
	std::vector<std::unique_ptr<CXeScrollBar>>				m_scrollbars;
	std::vector<std::unique_ptr<CXeScintillaEditControl>>	m_editcontrols;
	std::vector<std::unique_ptr<CXeTreeCtrlEx>>				m_treecontrols;
	std::vector<std::unique_ptr<CXeProgressCtrl>>			m_progresscontrols;
	std::vector<std::unique_ptr<CXeDurationCtrl>>			m_durationcontrols;
	std::vector<std::unique_ptr<CXeCalendarCtrl>>			m_calendarcontrols;
	std::vector<std::unique_ptr<CXeColorPicker>>			m_colorpickers;
	std::vector<std::unique_ptr<CUGCtrl>>					m_datagrids;
	// Remember to update _GetAllControls() when new control type added.

	XeGlueControls m_glueControls;
#pragma endregion class_data

#pragma region ctor
public:
	CXeBaseDlg(UINT nIDTemplate, CXeViewManagerIF* pVwMgr, HWND hParentWnd, CRect* pRectCell = 0)
			: CXeD2DWndBase(pVwMgr->GetUIcolors()), m_pVwMgr(pVwMgr), m_hDlgParentWnd(hParentWnd)
	{
		m_lpszTemplateName = MAKEINTRESOURCE(nIDTemplate);
		_Init(pRectCell);
	}

	CXeBaseDlg(const wchar_t* lpszTemplateName, CXeViewManagerIF* pVwMgr, HWND hParentWnd, CRect* pRectCell = 0)
			: CXeD2DWndBase(pVwMgr->GetUIcolors()),
			m_pVwMgr(pVwMgr), m_lpszTemplateName(lpszTemplateName), m_hDlgParentWnd(hParentWnd)
	{
		_Init(pRectCell);
	}

	virtual ~CXeBaseDlg() {}

protected:
	void _Init(CRect* pRectCell)
	{
		XeASSERT(m_pVwMgr && m_xeUI && m_hDlgParentWnd);
		if (m_hDlgParentWnd == ::GetDesktopWindow())
		{
			XeASSERT(FALSE);	// Desktop windows can't be a parent to a dialog.
			m_hDlgParentWnd = NULL;
		}
		if (pRectCell)
		{
			m_isRepositionDlgToRectCell = true;
			m_rectCell = *pRectCell;
		}
	}
#pragma endregion ctor

#pragma region Create
public:
	// Callback - needed when dialog is mode-less (non modal dialogs)
	// Dialog creator needs to supply this callback pointer.
	// Called after 'this' window destroyed - creator should delete 'this'.
	bool CreateModelessDialog(DialogClosedCallbackFunc onCloseCallback)
	{
		m_isModelessDialog = true;
		if (_CreateDialog())
		{
			m_xeUI->AddDialogHandle(Hwnd(), onCloseCallback);
			return true;
		}
		return false;
	}

	/// <summary>
	/// Manual mode dialog box.
	/// Based on: Raymond Chen articles on Dialog Manager
	/// https://devblogs.microsoft.com/oldnewthing/20050329-00/?p=36043
	/// </summary>
	/// <returns>result from end dialog box</returns>
	INT_PTR DoModal()
	{
		if (!_CreateDialog())
		{
			::DestroyWindow(Hwnd());
			return 0;
		}

		m_xeUI->AddDialogHandle(Hwnd(), nullptr);
		::EnableWindow(m_hDlgParentWnd, FALSE);
		MSG msg;
		msg.message = WM_NULL; // anything that isn't WM_QUIT
		while (!m_isDialogEnded && ::GetMessageW(&msg, NULL, 0, 0))
		{
			if (IsKeyboardMessage(msg.message))
			{
				if (m_xeUI->FilterKeyboardMessage(msg))
				{
					continue;
				}
			}
			else if ((IsMouseMessage(msg.message) || msg.message == WM_MOUSELEAVE)
					&& m_xeUI->IsScintillaEditControl(msg.hwnd))
			{
				HWND hECparentWnd = ::GetParent(msg.hwnd);
				if (hECparentWnd)
				{
					// Dispatch to parent of Scintilla edit control - to show/hide toolip when needed.
					MSG msgEC = msg;
					msgEC.hwnd = hECparentWnd;
					::DispatchMessageW(&msgEC);
				}
			}
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT)
		{
			::PostQuitMessage((int)msg.wParam);
		}
		::EnableWindow(m_hDlgParentWnd, TRUE);
		::DestroyWindow(Hwnd());
		return m_dlg_result;
	}

protected:
	virtual LRESULT _OnCreate(CREATESTRUCTW* pCS) override
	{
		// Disable DWM (Desktop Window Manager) rendering of NC window area.
		DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
		::DwmSetWindowAttribute(GetSafeHwnd(), DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
		return 0;
	}

	// Derived class overrides this - and then calls InitDialog.
	virtual BOOL _OnInitDialog()
	{
		return _InitDialog({});
	}

	virtual CXeGridDataSource* _GetDataGrid_DataSource(UINT uID)
	{
		XeASSERT(false);	// Derived class MUST override this to supply data grid data source.
		return nullptr;
	}

	// Derived classes call this from within their own OnInitDialog member.
	// If pRectCell was valid in ctor call - the dialog is repositioned now.
	// m_formHelper->Initialize(...) is called here.
	//    Note - derived classes can call m_formHelper->Initialize(...) before
	//           calling this member. (to enumerate child controls.)
	// m_formHelper->AutoSubclassControls() is called here.
	BOOL _InitDialog(const std::vector<GlueCtrl>& glue_controls,
		CSize sizeDefault = CSize(0, 0)/*,
		CXeGridDataSource* pDS_grid1 = nullptr, CXeGridDataSource* pDS_grid2 = nullptr*/)
	{
		if (m_settingsName.size() && m_isSaveRecallDlgPos)
		{
			CXeUserSettings settings(m_settingsName);
			int32_t nLastXpos, nLastYpos;
			int32_t nMainWndCheckValue, nCalcCheckValue = GetMainWndCheckValue();
			nLastXpos = settings.GetI32_or_Val(L"DlgXpos", -1);
			nLastYpos = settings.GetI32_or_Val(L"DlgYpos", -1);
			nMainWndCheckValue = settings.GetI32_or_Val(L"DlgPosCheckValue", 0);
			if (nLastXpos >= 0 && nLastYpos >= 0 && nMainWndCheckValue == nCalcCheckValue)
			{
				::SetWindowPos(Hwnd(), ::GetParent(Hwnd()), nLastXpos, nLastYpos, 0, 0,
					SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE);
			}
			else if (m_isRepositionDlgToRectCell)	// cell rect valid?
			{
				::SetWindowPos(Hwnd(), ::GetParent(Hwnd()), m_rectCell.left, m_rectCell.top, 0, 0,
					SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE);
			}
		}
		else if (m_isRepositionDlgToRectCell)	// dialog is used as a PopUp dialog in grid on a cell
		{
			_SetDlgPosToGridCell(m_rectCell);
		}

		_SetGroupBoxesZorder();

		m_glueControls.InitializeGlueControls(Hwnd(), glue_controls);

		int32_t nLastHeight = 0, nLastWidth = 0;
		if (m_style.WS().hasThickBorder() && m_settingsName.size())
		{
			CXeUserSettings settings(m_settingsName);
			nLastHeight = settings.GetI32_or_Val(L"DlgHeight", sizeDefault.cy);
			nLastWidth = settings.GetI32_or_Val(L"DlgWidth", sizeDefault.cx);
		}

		_AutoAddTooltips();

		//if (pDS_grid1)
		//{
		//	m_pgrid = std::make_unique<CUGCtrl>(pDS_grid1,
		//		pDS_grid1->GetDataGridSettingsName().c_str());
		//	m_pgrid->AttachGrid(CWnd::FromHandle(Hwnd()), IDC_GRID);
		//	m_pgrid->SetSH_Width(16);
		//	m_pgrid->SetWindowPos(0, 0, 0, 0, 0,
		//		SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
		//}
		//if (pDS_grid2)
		//{
		//	m_pgrid2 = std::make_unique<CUGCtrl>(pDS_grid2,
		//		pDS_grid2->GetDataGridSettingsName().c_str());
		//	m_pgrid2->AttachGrid(CWnd::FromHandle(Hwnd()), IDC_GRID2);
		//	m_pgrid2->SetSH_Width(16);
		//	m_pgrid2->SetWindowPos(0, 0, 0, 0, 0,
		//		SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
		//}

		if (m_isDialogWantsClickOutsideMouseDownToCancel)
		{
			// We want a posted message from XeUIcolors when user L clicked outside 'this' window.
			m_xeUI->SetDialogWantsClickOutsideMouseDown(Hwnd(), c_uDialogWantsClickOutsideMouseDownMessage);
		}

		return TRUE;
	}

	void _SetDlgPosToGridCell(CRect& rectCell, int nDlgWidth = 0, int nDlgHeight = 0)
	{	// call from OnInitDialog()
		HMONITOR hMonitor;
		hMonitor = ::MonitorFromRect(rectCell, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		::GetMonitorInfoW(hMonitor, &mi);
		CRect rectDlg, rectMon = mi.rcWork;

		::GetWindowRect(Hwnd(), rectDlg);
		rectDlg.MoveToXY(rectCell.left, rectCell.bottom);

		if (!nDlgWidth)	// Dialog width not specified - use current
			nDlgWidth = rectDlg.Width();
		else
			rectDlg.right = rectDlg.left + nDlgWidth;
		if (!nDlgHeight)	// Dialog height not specified - use current
			nDlgHeight = rectDlg.Height();
		else
			rectDlg.bottom = rectDlg.top + nDlgHeight;

		if (rectDlg.bottom > rectMon.bottom)
		{
			rectDlg.OffsetRect(0, -(rectDlg.Height() + rectCell.Height()));
			if (rectDlg.top < rectMon.top)
			{
				rectDlg.OffsetRect(0, (rectMon.top - rectDlg.top));
			}
		}

		if (rectDlg.right > rectMon.right)
			rectDlg.OffsetRect(-(rectDlg.right - rectMon.right), 0);

		if (rectDlg.left < rectMon.left)
			rectDlg.OffsetRect(rectMon.left - rectDlg.left, 0);

		::SetWindowPos(Hwnd(), ::GetParent(Hwnd()), rectDlg.left, rectDlg.top, nDlgWidth, nDlgHeight,
			SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW);
	}

	// Call this AFTER Initialize to auto generate tooltips for controls.
	// All child windows are enumerated, if ID != IDC_STATIC and string exists in
	//   resources: Add tooltip for that child window control.
	void _AutoAddTooltips()
	{
		//m_xtooltip->SetBorder(m_xeUI->GetColor(CID::TT_BgBorder));

		//SUPER_TOOLTIP_INFO sti;
		//sti.rgbBg = m_xeUI->GetColor(CID::TT_Bg);
		//sti.rgbText = m_xeUI->GetColor(CID::TT_Text);

		// TODO: FIXME - load string from resources using winapi
		//CString str;
		//for (UINT idc : m_idOfChildren)
		//{
		//	CWnd* pChildWnd = m_pWndOwner->GetDlgItem(idc);
		//	if (pChildWnd && str.LoadString(idc))
		//	{
		//		//sti.hWnd = pChildWnd->GetSafeHwnd();
		//		//sti.m_sBody = str;
		//		//sti.nSizeX = m_xeUI->GetValue(UIV::cxTooltipDefault);
		//		//m_xtooltip->AddTool(&sti);
		//		PPTOOLTIP_INFO ti;
		//		ti.hTWnd = pChildWnd->GetSafeHwnd();
		//		ti.sTooltip = str.GetString();
		//		m_xtooltip->AddTooltipTool(ti);
		//	}
		//}
	}

	void _SetGroupBoxesZorder()
	{
		std::vector<WndZorder> zordWnds = GetChildControlsInZorder(Hwnd());

		std::vector<WndZorder> grpboxes, others;
		for (const WndZorder& ctrl : zordWnds)
		{
			if (ctrl.ctrlType == ECtrlType::GroupBox)
			{
				grpboxes.push_back(ctrl);
			}
			else
			{
				others.push_back(ctrl);
			}
		}
		for (const WndZorder& gb : grpboxes)
		{
			CRect rcTest;
			std::vector<WndZorder> overlapping;
			for (const WndZorder& ctrl : others)
			{
				if (rcTest.IntersectRect(gb.rcCtrl, ctrl.rcCtrl))
				{
					overlapping.push_back(ctrl);
				}
			}
			if (overlapping.size())
			{
				// Set Z-order of group box to be after (below) last overlapping control.
				::SetWindowPos(gb.hWnd, overlapping.back().hWnd, 0, 0, 0, 0,
					SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
			}
		}
	}

#pragma endregion Create

#pragma region CreateDialogAndControls
protected:
	bool _CreateDialog()
	{
		XeASSERT(m_lpszTemplateName != nullptr && m_hDlgParentWnd);

		HINSTANCE hInst = m_xeUI->GetInstanceHandle();
		CXeDialogTemplateEx dlg_template;
		if (!dlg_template.LoadTemplateFromResources(hInst, m_lpszTemplateName))
		{
			XeASSERT(FALSE);
		}
		XeASSERT(dlg_template.m_hdr.windowClass.size() == 0);	// Only default dialog class supported (that we then ignore).

		// We use 'our' UI font to convert from dialog units to pixels.
		const XeFontMetrics& otm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);

		// Dialog client area size.
		int dlg_cx = otm.DialogUnitsXtoPixels(dlg_template.m_hdr.s1.cx);
		int dlg_cy = otm.DialogUnitsYtoPixels(dlg_template.m_hdr.s1.cy);

		DWORD style = dlg_template.m_hdr.s1.style & 0xFFFF0000;	// Remove all DS_styles.

		style |= (WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

		// Add caption and barders to size of window.
		XeWindowStyleValue xe_style(style, 0);
		int cyCaption = xe_style.hasCaption() ? m_xeUI->GetValue(UIV::cyWindowCaption) : 0;
		int cxyBorder = m_xeUI->GetValue(xe_style.hasThickBorder() ? UIV::cxyThickBorder : UIV::cxyThinBorder);

		dlg_cy += (cyCaption + (2 * cxyBorder));
		dlg_cx += (2 * cxyBorder);

		m_xeUI->RegisterWindowClass(XEDIALOGWND_CLASSNAME, D2DCtrl_WndProc);

		if (m_hDlgParentWnd)
		{
			// Only top-level windows can be owners, we have to take the putative hwndParent
			// (which might be a child window) and walk up the window hierarchy until we find a top-level window.
			m_hDlgParentWnd = ::GetAncestor(m_hDlgParentWnd, GA_ROOT);
		}

		int x = 0, y = 0;
		CRect rcParent;
		if (::GetWindowRect(m_hDlgParentWnd, &rcParent))
		{
			x = rcParent.left + ((rcParent.Width() - dlg_cx) / 2);
			y = rcParent.top + ((rcParent.Height() - dlg_cy) / 2);
		}

		HWND hWnd = CreateD2DWindow(0, XEDIALOGWND_CLASSNAME, dlg_template.m_hdr.title.c_str(), style,
				CRect(x, y, x + dlg_cx, y + dlg_cy), m_hDlgParentWnd, 0, true);
		if (dlg_template.m_hdr.title.size())
		{
			::SetWindowTextW(hWnd, dlg_template.m_hdr.title.c_str());
		}

		for (const CXeDlgTemplateItem& item : dlg_template.m_items)
		{
			CPoint pt(otm.DialogUnitsXtoPixels(item.s.x), otm.DialogUnitsYtoPixels(item.s.y));
			short cy = item.s.cy;
			if (item.class_ordinal == 0x0083) // List box?
			{
				// Listbox CY size is bigger than it should be (for some unknown reason).
				cy = cy > 10 ? cy - 10 : cy;
			}
			CSize sz(otm.DialogUnitsXtoPixels(item.s.cx), otm.DialogUnitsYtoPixels(cy));
			CRect rc(pt, sz);
			_CreateControl(item, rc);
		}

		if (!_OnInitDialog())
		{
			return false;
		}

		::ShowWindow(hWnd, SW_SHOW);

		DoTabToNextCtrl(false, hWnd, hWnd);	// Set focus to first control.

		return hWnd != 0;
	}

	void _CreateControl(const CXeDlgTemplateItem& item, const CRect& rcCtrl)
	{
		if (item.class_ordinal)
		{
			switch (item.class_ordinal)
			{
			case 0x0080: // Button
				_CreateButton(item.s.id, item.s.style, item.s.exStyle, rcCtrl, item.title);
				break;
			case 0x0081: // Edit
				_CreateEditControl(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
				break;
			case 0x0082: // Static
				if (item.resource_ordinal != 0)
				{
					// Icon (or bitmap)
					_CreateStatic(item.s.id, item.s.style, item.s.exStyle, rcCtrl, item.resource_ordinal);
				}
				else
				{
					// Static text
					_CreateStatic(item.s.id, item.s.style, item.s.exStyle, rcCtrl, item.title);
				}
				break;
			case 0x0083: // List box
				_CreateListbox(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
				break;
			case 0x0084: // Scroll bar
				_CreateScrollBar(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
				break;
			case 0x0085: // Combo box
				_CreateCombobox(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
				break;
			}
		}
		else if (item.windowClass.size() > 0)
		{
			if (item.windowClass == L"msctls_progress32")
			{
				_CreateProgressControl(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
			}
			else if (item.windowClass == L"SysTreeView32")
			{
				_CreateTreeControl(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
			}
			else if (item.windowClass == L"MfcColorButton")
			{
				_CreateColorButton(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
			}
			else if (item.windowClass == L"XeDurationWndClass")
			{
				_CreateDurationControl(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
			}
			else if (item.windowClass == L"XeCalendarWndClass")
			{
				_CreateCalendarControl(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
			}
			else if (item.windowClass == L"XeColorPickerWndClass")
			{
				_CreateColorPickerControl(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
			}
			else if (item.windowClass == L"CUGCtrl_WNDCLASS")
			{
				_CreateGrid(item.s.id, item.s.style, item.s.exStyle, rcCtrl);
			}
			else
			{
				//XeASSERT(FALSE);	// Unknown control type - need to add support?
				//return ECtrlType::Invalid;
				// TODO: support Progress control and other control types needed for LVS.
				XeASSERT(FALSE);	// Not supported (yet).
			}
		}
	}

	void _CreateButton(UINT uID, DWORD style, DWORD exStyle, const CRect& rc, const std::wstring& title)
	{
		auto btn = std::make_unique<CXeD2DButton>(m_xeUI);
		btn->Create(title.c_str(), style, rc, Hwnd(), uID, nullptr);
		m_buttons.push_back(std::move(btn));
	}

	void _CreateColorButton(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		auto btn = std::make_unique<CXeD2DButton>(m_xeUI);
		btn->CreateColorButton(style, rc, Hwnd(), uID, nullptr);
		m_buttons.push_back(std::move(btn));
	}

	void _CreateStatic(UINT uID, DWORD style, DWORD exStyle, const CRect& rc, const std::wstring& title)
	{
		auto stc = std::make_unique<CXeD2DStatic>(m_xeUI);
		stc->Create(title.c_str(), style, exStyle, rc, Hwnd(), uID, nullptr);
		m_statics.push_back(std::move(stc));
	}

	void _CreateStatic(UINT uID, DWORD style, DWORD exStyle, const CRect& rc, UINT resourceId)
	{
		auto stc = std::make_unique<CXeD2DStatic>(m_xeUI);
		stc->Create(resourceId, style, exStyle, rc, Hwnd(), uID, nullptr);
		m_statics.push_back(std::move(stc));
	}

	void _CreateListbox(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		auto lb = std::make_unique<CXeListBoxEx>(m_xeUI);
		lb->Create(style, exStyle, rc, Hwnd(), uID);
		m_listboxes.push_back(std::move(lb));
	}

	void _CreateScrollBar(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		auto sb = std::make_unique<CXeScrollBar>(m_xeUI);
		sb->Create(style, rc, Hwnd(), uID, nullptr);
		m_scrollbars.push_back(std::move(sb));
	}

	void _CreateCombobox(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		// rc is including listbox height - we need height of (scaled) ComboBox here.
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		CRect rcCB(rc);
		rcCB.bottom = rcCB.top + tm.GetLineHeight();
		auto cb = std::make_unique<CXeComboBoxEx>(m_xeUI);
		cb->Create(style, exStyle, rcCB, Hwnd(), uID, rc.Height(), 0, nullptr);
		m_comboboxes.push_back(std::move(cb));
	}

	void _CreateEditControl(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		auto ec = std::make_unique<CXeScintillaEditControl>(m_xeUI);
		ec->Create(style, Hwnd(), rc, uID, nullptr);
		//ec->UpdateFont();
		//ec->UpdateColors();
		//ec->SetTextSuppressEnChangeNotify(L"");
		m_editcontrols.push_back(std::move(ec));
	}

	void _CreateTreeControl(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		auto tc = std::make_unique<CXeTreeCtrlEx>(m_xeUI);
		tc->Create(style, exStyle, rc, Hwnd(), uID);
		m_treecontrols.push_back(std::move(tc));
	}

	void _CreateProgressControl(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		auto pc = std::make_unique<CXeProgressCtrl>(m_xeUI);
		pc->Create(style, exStyle, rc, Hwnd(), uID, nullptr);
		m_progresscontrols.push_back(std::move(pc));
	}

	// return new rect for control of sizeWanted, y pos adjusted to vertical center of rc.
	[[nodiscard]] CRect _AdjustControlSize(const CRect& rc, const CSize& sizeWanted)
	{
		if (rc.Height() > sizeWanted.cy)
		{
			int y = rc.top + ((rc.Height() - sizeWanted.cy) / 2);
			return CRect(rc.left, y, rc.left + sizeWanted.cx, y + sizeWanted.cy);
		}
		else
		{
			int y = rc.top - ((sizeWanted.cy - rc.Height()) / 2);
			return CRect(rc.left, y, rc.left + sizeWanted.cx, y + sizeWanted.cy);
		}
	}

	void _CreateDurationControl(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		auto pc = std::make_unique<CXeDurationCtrl>(m_xeUI, DurCtrlMode::DurationSM2);
		CRect rcWanted = _AdjustControlSize(rc, pc->GetControlSize());
		pc->Create(style, rcWanted, Hwnd(), uID, nullptr);
		m_durationcontrols.push_back(std::move(pc));
	}

	void _CreateCalendarControl(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		auto pc = std::make_unique<CXeCalendarCtrl>(m_xeUI, CalCtrlMode::SimpleDateTimeSM2);
		CRect rcWanted = _AdjustControlSize(rc, pc->GetControlSize());
		pc->Create(style, rcWanted, Hwnd(), uID, nullptr);
		m_calendarcontrols.push_back(std::move(pc));
	}

	void _CreateColorPickerControl(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		auto pc = std::make_unique<CXeColorPicker>(m_xeUI, ColorPickerMode::Simple);
		pc->Create(style, rc, Hwnd(), uID, nullptr);
		m_colorpickers.push_back(std::move(pc));
	}

	void _CreateGrid(UINT uID, DWORD style, DWORD exStyle, const CRect& rc)
	{
		CXeGridDataSource* pDS = _GetDataGrid_DataSource(uID);
		if (!pDS)
		{
			XeASSERT(false);	// Need data source for data grid.
			return;
		}
		auto dg = std::make_unique<CUGCtrl>(pDS, pDS->GetDataGridSettingsName().c_str());
		if (!dg->CreateGrid(style, exStyle, rc, Hwnd(), uID))
		{
			XeASSERT(FALSE);	// Create grid failed.
		}
		dg->SetSH_Width(16);
		m_datagrids.push_back(std::move(dg));
	}
#pragma endregion CreateDialogAndControls

#pragma region DialogManagement
protected:
	virtual void OnOK()
	{
		// TODO: DoDataExchange
		EndDialog(IDOK);
	}

	virtual void OnCancel()
	{
		// Need to check if any date/time control has an open month calendar control and close it.
		//std::vector<UINT> dtCtrlIDs = m_formHelper->GetAllControlIDs(ECtrlType::SysDateTimePick32);
		//for (UINT id : dtCtrlIDs)
		//{
		//	XeASSERT(FALSE);
		//	//CDateTimeCtrl* pCtrl1 = (CDateTimeCtrl*)GetDlgItem(id);
		//	//if (pCtrl1)
		//	//{
		//	//	CMonthCalCtrl* pMCC1 = pCtrl1->GetMonthCalCtrl();
		//	//	if (pMCC1)
		//	//	{
		//	//		pMCC1->DestroyWindow();
		//	//	}
		//	//}
		//}
		EndDialog(IDCANCEL);
	}

	void EndDialog(int nResult)
	{
		XeASSERT(::IsWindow(Hwnd()));

		m_dlg_result = nResult;
		m_isDialogEnded = true;
		if (m_isModelessDialog)
		{
			::DestroyWindow(Hwnd());
		}
	}
#pragma endregion DialogManagement

#pragma region Helpers
protected:
	void _ApplyFontChangeToControls()
	{
		XeASSERT(false);	// FIXME
		//m_formHelper->ApplyFontChangeToControls();
	}

	void SetDlgSize(int32_t cx, int32_t cy)
	{
		WINDOWPLACEMENT wplc;
		wplc.length = sizeof(WINDOWPLACEMENT);
		if (::GetWindowPlacement(Hwnd(), &wplc))
		{
			CRect rcNormalSize = wplc.rcNormalPosition;
			int32_t nHeight = rcNormalSize.Height();
			int32_t nWidth = rcNormalSize.Width();
			if (nHeight != cy || nWidth != cx)
			{
				wplc.rcNormalPosition.right = wplc.rcNormalPosition.left + cx;
				wplc.rcNormalPosition.bottom = wplc.rcNormalPosition.top + cy;
				::SetWindowPlacement(Hwnd(), &wplc);
			}
		}
	}

	void SetDlgClientSize(int32_t cx, int32_t cy)
	{
		CRect rcWnd, rcCli;
		::GetWindowRect(Hwnd(), &rcWnd);
		::GetClientRect(Hwnd(), &rcCli);
		SetDlgSize(cx + (rcWnd.Width() - rcCli.Width()), cy + (rcWnd.Height() - rcCli.Height()));
	}

	// W/H of dlg is saved as "DlgWidth"/"DlgHeight" entries.
	void SaveUserPreferredDlgSize()
	{
		WINDOWPLACEMENT wplc;
		wplc.length = sizeof(WINDOWPLACEMENT);
		if (::GetWindowPlacement(Hwnd(), &wplc) && m_settingsName.size())
		{
			CXeUserSettings settings(m_settingsName);
			CRect rcNormalSize = wplc.rcNormalPosition;
			int32_t nHeight = rcNormalSize.Height();
			int32_t nWidth = rcNormalSize.Width();
			settings.Set(L"DlgHeight", nHeight);
			settings.Set(L"DlgWidth", nWidth);
		}
	}

	void SaveUserPreferredDlgPos()
	{
		WINDOWPLACEMENT wplc;
		wplc.length = sizeof(WINDOWPLACEMENT);
		if (::GetWindowPlacement(Hwnd(), &wplc) && m_settingsName.size())
		{
			CXeUserSettings settings(m_settingsName);
			CRect rcNormalSize = wplc.rcNormalPosition;
			int32_t x_pos = rcNormalSize.left;
			int32_t y_pos = rcNormalSize.top;
			settings.Set(L"DlgXpos", x_pos);
			settings.Set(L"DlgYpos", y_pos);
			int32_t nCalcCheckValue = GetMainWndCheckValue();
			settings.Set(L"DlgPosCheckValue", nCalcCheckValue);
		}
	}

	const GlueCtrl* GetGlueCtrlStruct(UINT uIDC)
	{
		return m_glueControls.GetGlueCtrlStruct(uIDC);
	}

	void _SetControlString(int nIDC, const std::wstring& value)
	{
		HWND hWndCtrl = GetDlgItem(nIDC);
		XeASSERT(hWndCtrl);
		::SetWindowText(hWndCtrl, value.c_str());
	}

	std::wstring _GetControlString(int nIDC)
	{
		HWND hWndCtrl = GetDlgItem(nIDC);
		XeASSERT(hWndCtrl);
		return GetWindowTextStdW(hWndCtrl);
	}

	/// <summary>
	/// Get control rect in dialog client coordinates.
	/// </summary>
	CRect GetControlRect(UINT id)
	{
		HWND hCtrlWnd = GetDlgItem(id);
		CRect rc;
		if (hCtrlWnd)
		{
			::GetWindowRect(hCtrlWnd, rc);
			::ScreenToClient(Hwnd(), (LPPOINT)&rc);
			::ScreenToClient(Hwnd(), ((LPPOINT)&rc) + 1);
		}
		return rc;
	}

	// Get summed value of main window rect (used for sanity check).
	int32_t GetMainWndCheckValue()
	{
		CRect rcWnd;
		::GetWindowRect(m_xeUI->GetMainWindowHandle(), &rcWnd);
		int32_t uCalcCheckValue = rcWnd.left + rcWnd.right + rcWnd.top + rcWnd.bottom;
		return uCalcCheckValue;
	}

	int ShowMessageBox(const wchar_t* pMsg, const wchar_t* pTitle = nullptr,
		UINT uType = MB_OK, CRect* pRectCell = nullptr)
	{
		return m_pVwMgr->ShowMessageBox(Hwnd(), pMsg, pTitle, uType, pRectCell);
	}

	void _EnableControls(const std::vector<UINT>& ids, BOOL fEnable)
	{
		for (UINT id : ids)
		{
			HWND hCtrlWnd = GetDlgItem(id);
			if (hCtrlWnd)
			{
				::EnableWindow(hCtrlWnd, fEnable);
			}
		}
	}

	void _ShowControls(const std::vector<UINT>& ids, int sw_cmd)
	{
		for (UINT id : ids)
		{
			HWND hCtrlWnd = GetDlgItem(id);
			if (hCtrlWnd)
			{
				::ShowWindow(hCtrlWnd, sw_cmd);
			}
		}
	}

	void _MoveControl(int nID, const CRect& rc, BOOL isRepaint = TRUE)
	{
		HWND hWndCtrl = GetDlgItem(nID);
		XeASSERT(hWndCtrl);
		::MoveWindow(hWndCtrl, rc.left, rc.top, rc.Width(), rc.Height(), isRepaint);
	}

	LRESULT _SendControlMessage(UINT uCtrlId, UINT message, WPARAM wParam, LPARAM lParam)
	{
		HWND hWndCtrl = GetDlgItem(uCtrlId);
		return ::SendMessageW(hWndCtrl, message, wParam, lParam);
	}

	int _GetCheckBoxState(UINT uID)
	{
		return (int)_SendControlMessage(uID, BM_GETCHECK, 0, 0);
	}

	void _SetCheckBoxState(UINT uID, int state)
	{
		_SendControlMessage(uID, BM_SETCHECK, (WPARAM)state, 0);
	}

	void _InvertCheckBoxState(UINT uID)
	{
		int state = _GetCheckBoxState(uID);
		_SetCheckBoxState(uID, state == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED);
	}

	std::wstring _GetComboBoxText(UINT uID)
	{
		HWND hCtrlWnd = GetDlgItem(uID);
		if (hCtrlWnd)
		{
			return GetWindowTextStdW(hCtrlWnd);
		}
		return std::wstring();
	}

	CXeD2DButton* _GetButtonControl(UINT uID)
	{
		for (auto& btn : m_buttons)
		{
			if (btn->GetDlgCtrlID() == uID)
			{
				return btn.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CXeD2DStatic* _GetStaticControl(UINT uID)
	{
		for (auto& ctrl : m_statics)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CXeListBoxEx* _GetListBoxControl(UINT uID)
	{
		for (auto& ctrl : m_listboxes)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CXeComboBoxEx* _GetComboBoxControl(UINT uID)
	{
		for (auto& ctrl : m_comboboxes)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CXeScrollBar* _GetScrollBarControl(UINT uID)
	{
		for (auto& ctrl : m_scrollbars)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CXeScintillaEditControl* _GetEditControl(UINT uID)
	{
		for (auto& ctrl : m_editcontrols)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CXeTreeCtrlEx* _GetTreeControl(UINT uID)
	{
		for (auto& ctrl : m_treecontrols)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CXeProgressCtrl* _GetProgressControl(UINT uID)
	{
		for (auto& ctrl : m_progresscontrols)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CXeDurationCtrl* _GetDurationControl(UINT uID)
	{
		for (auto& ctrl : m_durationcontrols)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CXeCalendarCtrl* _GetCalendarControl(UINT uID)
	{
		for (auto& ctrl : m_calendarcontrols)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CXeColorPicker* _GetColorPickerControl(UINT uID)
	{
		for (auto& ctrl : m_colorpickers)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	CUGCtrl* _GetDataGridControl(UINT uID)
	{
		for (auto& ctrl : m_datagrids)
		{
			if (ctrl->GetDlgCtrlID() == uID)
			{
				return ctrl.get();
			}
		}
		XeASSERT(FALSE);
		return nullptr;
	}

	//void _AddTooltip(UINT uIDC, const std::wstring& strTip)
	//{
	//	//SUPER_TOOLTIP_INFO sti;
	//	//sti.rgbBg = m_xeUI->GetColor(CID::TT_Bg);
	//	//sti.rgbText = m_xeUI->GetColor(CID::TT_Text);
	//	//sti.hWnd = m_pWndOwner->GetDlgItem(uIDC)->GetSafeHwnd();
	//	//sti.m_sBody = strTip;
	//	//sti.nSizeX = m_xeUI->GetValue(UIV::cxTooltipDefault);
	//	//m_xtooltip->AddTool(&sti);
	//	PPTOOLTIP_INFO ti;
	//	ti.hTWnd = ::GetDlgItem(Hwnd(), uIDC);
	//	ti.sTooltip = strTip.c_str();
	//	m_xtooltip->AddTooltipTool(ti);
	//}

	std::vector<CXeD2DCtrlBase*> _GetAllControls() const
	{
		std::vector<CXeD2DCtrlBase*> controls;
		for (auto& ctrl : m_buttons)			{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_statics)			{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_listboxes)			{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_comboboxes)			{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_scrollbars)			{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_editcontrols)		{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_treecontrols)		{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_progresscontrols)	{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_durationcontrols)	{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_calendarcontrols)	{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_colorpickers)		{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		for (auto& ctrl : m_datagrids)			{ if (ctrl->GetSafeHwnd()) { controls.push_back(ctrl.get()); } }
		return controls;
	}

	void _SetTooltipForControl(UINT uID, const std::wstring& tooltip)
	{
		CXeD2DButton* pBtn = _GetButtonControl(uID);
		XeASSERT(pBtn);
		if (pBtn)
		{
			pBtn->SetTooltip(tooltip);
		}
	}
#pragma endregion Helpers

#pragma region DataExchange
protected:
	/// <summary>
	/// Perform data exchange with dialog controls (similar to - but the same as MFC).
	/// <para>isSave = TRUE when getting data from controls into local variables.</para>
	/// </summary>
	void _UpdateData(BOOL isSave)
	{
		_DoDataExchange(isSave);
	}

	/// <summary>
	/// Derived class overrides this to perform data exchange with controls.
	/// </summary>
	virtual void _DoDataExchange(BOOL isSave)
	{
	}

	void _DDX_TEXT(BOOL isSave, int uID, std::wstring& str)
	{
		if (isSave)
		{
			str = _GetControlString(uID);
		}
		else
		{
			_SetControlString(uID, str);
		}
	}

	// simple text operations
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, BYTE& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, short& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, int& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, UINT& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, long& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, DWORD& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, LONGLONG& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, ULONGLONG& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, CString& value);
	//void AFXAPI DDX_Text(_Inout_ CDataExchange* pDX, _In_ int nIDC, _Out_writes_z_(nMaxLen) LPTSTR value, _In_ int nMaxLen);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, float& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, double& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, COleCurrency& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, COleDateTime& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, GUID& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, DECIMAL& value);
	//void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, FILETIME& value);

	// special control types
	void _DDX_Check(BOOL isSave, int nIDC, int& value)
	{
		// From MFC source: C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\atlmfc\src\mfc\dlgdata.cpp
		HWND hWndCtrl = GetDlgItem(nIDC);
		if (isSave)
		{
			value = (int)::SendMessage(hWndCtrl, BM_GETCHECK, 0, 0L);
			XeASSERT(value >= 0 && value <= 2);
		}
		else
		{
			if (value < 0 || value > 2)
			{
				//TRACE(traceAppMsg, 0, "Warning: dialog data checkbox value (%d) out of range.\n",
				//	value);
				value = 0;  // default to off
			}
			::SendMessage(hWndCtrl, BM_SETCHECK, (WPARAM)value, 0L);
		}
	}
	//void AFXAPI DDX_Radio(CDataExchange* pDX, int nIDC, int& value);
	//void AFXAPI DDX_LBString(CDataExchange* pDX, int nIDC, CString& value);
	//void AFXAPI DDX_CBString(CDataExchange* pDX, int nIDC, CString& value);
	//void AFXAPI DDX_LBIndex(CDataExchange* pDX, int nIDC, int& index);
	//void AFXAPI DDX_CBIndex(CDataExchange* pDX, int nIDC, int& index);
	//void AFXAPI DDX_LBStringExact(CDataExchange* pDX, int nIDC, CString& value);
	//void AFXAPI DDX_CBStringExact(CDataExchange* pDX, int nIDC, CString& value);
	//void AFXAPI DDX_Scroll(CDataExchange* pDX, int nIDC, int& value);
	//void AFXAPI DDX_Slider(CDataExchange* pDX, int nIDC, int& value);

	//void AFXAPI DDX_IPAddress(CDataExchange* pDX, int nIDC, DWORD& value);

	//void AFXAPI DDX_MonthCalCtrl(CDataExchange* pDX, int nIDC, CTime& value);
	//void AFXAPI DDX_MonthCalCtrl(CDataExchange* pDX, int nIDC, COleDateTime& value);
	//void AFXAPI DDX_MonthCalCtrl(CDataExchange* pDX, int nIDC, FILETIME& value);
	//void AFXAPI DDX_DateTimeCtrl(CDataExchange* pDX, int nIDC, CString& value);
	//void AFXAPI DDX_DateTimeCtrl(CDataExchange* pDX, int nIDC, CTime& value);
	//void AFXAPI DDX_DateTimeCtrl(CDataExchange* pDX, int nIDC, COleDateTime& value);
	//void AFXAPI DDX_DateTimeCtrl(CDataExchange* pDX, int nIDC, FILETIME& value);

	void _DDX_Duration(BOOL isSave, int nIDC, FILETIMESPAN& value)
	{
		CXeDurationCtrl* pDur = _GetDurationControl((UINT)nIDC);
		if (!pDur)
		{
			XeASSERT(false);
			return;
		}
		if (isSave)
		{
			value = pDur->GetDuration();
		}
		else
		{
			pDur->SetDuration(value);
		}
	}

	void _DDX_DateTime(BOOL isSave, int nIDC, FILETIMEX& value)
	{
		CXeCalendarCtrl* pDT = _GetCalendarControl((UINT)nIDC);
		if (!pDT)
		{
			XeASSERT(false);
			return;
		}
		if (isSave)
		{
			value = pDT->m_fromDT;
		}
		else
		{
			pDT->SetDateTimes(value, FILETIMEX());
		}
	}

	void _DDX_Color(BOOL isSave, int nIDC, DWORD& value)
	{
		CXeD2DButton* pBtn = _GetButtonControl((UINT)nIDC);
		if (pBtn && pBtn->IsColorButtonMode())
		{
			if (isSave)
			{
				value = pBtn->GetColor();
			}
			else
			{
				pBtn->SetColor(value);
			}
			return;
		}
		CXeColorPicker* pCP = _GetColorPickerControl((UINT)nIDC);
		if (!pCP)
		{
			XeASSERT(false);
			return;
		}
		if (isSave)
		{
			value = pCP->SELCOL.ToRGB(false);
		}
		else
		{
			pCP->SELCOL.SetFromRGB(value, false);
			pCP->RedrawDirectly();
		}
	}
#pragma endregion DataExchange

#pragma region MessageHandlers
protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		pRT->FillRectangle(rc, GetBrush(CID::DialogBg));
	}

	virtual LRESULT _OnSysCommand(WPARAM wParam, LPARAM lParam) override
	{
		if (wParam == SC_CLOSE && !m_isModelessDialog)
		{
			EndDialog(IDCANCEL);
		}
		return CXeD2DWndBase::_OnSysCommand(wParam, lParam);
	}

	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam) override
	{
		HWND hWnd = (HWND)wParam;
		UINT nHitTest = LOWORD(lParam);
		UINT message = HIWORD(lParam);
		return m_xeUI->OnSetCursorHelper(Hwnd(), hWnd, nHitTest, message);
	}

	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender) override
	{
		if (wID == IDOK)
		{
			OnOK();
		}
		else if (wID == IDCANCEL)
		{
			OnCancel();
		}
		return 0;
	}

	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		case WM_NCUAHDRAWCAPTION:
		case WM_NCUAHDRAWFRAME:
			return 1; // 0;	// Eat message.

		case WM_SIZING:
			m_isSizing = true;
			break;

		case WM_MOVING:
			m_isMoving = true;
			break;

		case c_uDialogWantsClickOutsideMouseDownMessage:
			return OnDialogWantsClickOutsideMouseDownMessage();
		}
		return CXeD2DWndBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}
#pragma endregion MessageHandlers

#pragma region CustomWindowFramesupport
protected:
	virtual LRESULT _OnGetMinMaxInfo(WPARAM wParam, LPARAM lParam)
	{
		MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
		if (m_nDialogMinX)
		{
			XeASSERT(m_nDialogMinY);	// Both min x and Y should be set.
			pMMI->ptMinTrackSize.x = m_nDialogMinX;
			pMMI->ptMinTrackSize.y = m_nDialogMinY;
		}

		return CXeD2DWndBase::_OnGetMinMaxInfo(wParam, lParam);
	}

	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{
		UINT nType = (UINT)wParam;
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);

		if (nType == SIZE_RESTORED)
		{
			m_glueControls.MoveGlueControlsOnSize(cx, cy);
		}
		return CXeD2DWndBase::_OnSize(hWnd, wParam, lParam);
	}

	virtual LRESULT _OnExitSizeMove() override
	{
		if (m_isSizing)
		{
			SaveUserPreferredDlgSize();
		}
		if (m_isMoving && m_isSaveRecallDlgPos)
		{
			SaveUserPreferredDlgPos();
		}
		m_isSizing = false;
		m_isMoving = false;
		return 0;
	}

	// We get a posted message from XeUIcolors when user L clicked outside 'this' window.
	LRESULT OnDialogWantsClickOutsideMouseDownMessage()
	{
		OnCancel();
		return 0;
	}
#pragma endregion CustomWindowFramesupport
};

