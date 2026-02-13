module;

#include "os_minimal.h"
#include <memory>
#include <string>

export module Xe.UserSettingControl;

import Xe.UIcolorsIF;
export import Xe.UserSettingsForUI;
import Xe.D2DButton;
import Xe.ComboBoxEx;
import Xe.ScintillaEditControl;
import Xe.Helpers;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class XeCtrlType
{
	Invalid,
	CheckBox,
	ComboBox,
	EditControl
};

export class CXeUserSettingControl : public CXeUserSettingForUI
{
protected:
	std::unique_ptr<CXeD2DButton>				m_control_checkbox;
	std::unique_ptr<CXeComboBoxEx>				m_control_combobox;
	std::unique_ptr<CXeScintillaEditControl>	m_control_edit;

public:
	CXeUIcolorsIF*	m_xeUI = nullptr;
	XeCtrlType		m_control_type;
	HWND			m_hWndCtrl = 0;

	CXeUserSettingControl() = default;
	CXeUserSettingControl(CXeUserSettingControl& other) = delete;
	CXeUserSettingControl& operator=(const CXeUserSettingControl&) = delete;

	CXeUserSettingControl(CXeUserSettingControl&& other) noexcept
			: CXeUserSettingForUI(std::move(other)), m_control_type(other.m_control_type),
			m_control_checkbox(std::move(other.m_control_checkbox)), 
			m_control_combobox(std::move(other.m_control_combobox)),
			m_control_edit(std::move(other.m_control_edit)), m_xeUI(other.m_xeUI), m_hWndCtrl(other.m_hWndCtrl)
	{
	}

	CXeUserSettingControl(CXeUIcolorsIF* pUIcolors, const CXeUserSetting& setting)
			: CXeUserSettingForUI(setting), m_xeUI(pUIcolors)
	{
	}

	void CreateCheckBox(HWND hParent, CRect rc, const wchar_t* tooltip)
	{
		m_control_type = XeCtrlType::CheckBox;
		m_control_checkbox = std::make_unique<CXeD2DButton>(m_xeUI);
		m_control_checkbox->Create(m_display_name.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
				rc, hParent, m_ctrl_id, tooltip);
		m_hWndCtrl = m_control_checkbox->GetSafeHwnd();
		::SendMessageW(m_hWndCtrl, BM_SETCHECK, (WPARAM)getBool(), 0);
	}

	void InvertCheckboxTick()
	{
		int check = (int)::SendMessageW(m_hWndCtrl, BM_GETCHECK, 0, 0);
		::SendMessageW(m_hWndCtrl, BM_SETCHECK, (WPARAM)check, 0);
	}

	CXeComboBoxEx* GetComboBoxPtr()
	{
		CXeComboBoxEx* p = dynamic_cast<CXeComboBoxEx*>(m_control_combobox.get());
		XeASSERT(p && m_control_type == XeCtrlType::ComboBox);
		return p;
	}

	CXeScintillaEditControl* GetEditCtrlPtr()
	{
		CXeScintillaEditControl* p = dynamic_cast<CXeScintillaEditControl*>(m_control_edit.get());
		XeASSERT(p && m_control_type == XeCtrlType::EditControl);
		return p;
	}

	// Returns right edge of combobox.
	uint32_t CreateComboBox(HWND hParent, uint32_t x, uint32_t y, const std::vector<std::wstring>& strings,
			const wchar_t* tooltip)
	{
		x = AdjustXtoGrid(x);
		uint32_t cx_txt = 0;
		for (auto& string : strings)
		{
			CSize sizeLbl = m_xeUI->GetTextSizeUsingFontW(hParent, EXE_FONT::eUI_Font, string.c_str());
			if (cx_txt < (uint32_t)sizeLbl.cx) { cx_txt = (uint32_t)sizeLbl.cx; }
		}
		cx_txt = AdjustXtoGrid(cx_txt + 24);
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		CRect rc(x, y, x + cx_txt, y + tm.GetLineHeight() + 4);
		m_control_type = XeCtrlType::ComboBox;
		m_control_combobox = std::make_unique<CXeComboBoxEx>(m_xeUI);
		CXeComboBoxEx* pCB = GetComboBoxPtr();
		pCB->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP| WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST,
			0, rc, hParent, m_ctrl_id, 200, 0, tooltip);
		m_hWndCtrl = m_control_combobox->GetSafeHwnd();
		for (auto& string : strings)
		{
			pCB->AddString(string.c_str());
		}
		pCB->SetCurSel(pCB->FindStringExact(-1, getString().c_str()));	// Set cur sel to "value".
		return (uint32_t)rc.right;
	}

	// Returns right edge of combobox.
	uint32_t CreateEditControl(HWND hParent, uint32_t x, uint32_t y, const wchar_t* tooltip)
	{
		x = AdjustXtoGrid(x);
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		std::vector<std::wstring> strings = getRangeStrings();
		uint32_t cx_txt = 0;
		for (auto& string : strings)
		{
			CSize sizeLbl = m_xeUI->GetTextSizeUsingFontW(hParent, EXE_FONT::eUI_Font, string.c_str());
			cx_txt = std::max((uint32_t)sizeLbl.cx, cx_txt);
		}
		if (cx_txt)
		{
			cx_txt += 2 * tm.m_aveCharWidth;
		}
		else
		{
			cx_txt = 20 * tm.m_aveCharWidth;
		}
		cx_txt = AdjustXtoGrid(cx_txt);
		CRect rc(x, y, x + cx_txt, y + (uint32_t)((double)tm.GetHeight() * 1.5 + 0.5));
		m_control_type = XeCtrlType::EditControl;
		m_control_edit = std::make_unique<CXeScintillaEditControl>(m_xeUI);
		CXeScintillaEditControl* pEC = GetEditCtrlPtr();
		pEC->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP| WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
			hParent, rc, m_ctrl_id, tooltip);
		m_hWndCtrl = m_control_edit->GetSafeHwnd();
		pEC->SetTextSuppressEnChangeNotify(m_value);
		return (uint32_t)rc.right;
	}

	void DestroyControl()
	{
		XeASSERT(m_hWndCtrl);
		::DestroyWindow(m_hWndCtrl);	// Destroy control window.
	}

	// Returns true if new value is valid, else false (is out of range)
	bool ReadSettingValueFromControl()
	{
		XeASSERT(m_hWndCtrl);
		if (!m_hWndCtrl) { return false; }
		switch (m_control_type)
		{
		case XeCtrlType::CheckBox:
			setBool((int)::SendMessageW(m_hWndCtrl, BM_GETCHECK, 0, 0));
			return true;

		case XeCtrlType::ComboBox: {
			CXeComboBoxEx* pCB = GetComboBoxPtr();
			int nCurSel = pCB->GetCurSel();
			std::wstring val;
			if (nCurSel >= 0)
			{
				val = GetComboBoxLBtext(pCB->GetSafeHwnd(), nCurSel);
			}
			setString(val);
			} return true;

		case XeCtrlType::EditControl: {
			CXeScintillaEditControl* pEC = GetEditCtrlPtr();
			return ValidateAndStoreValue(GetWindowTextStdW(pEC->GetSafeHwnd()));
			} break;
		}
		return false;
	}

	uint32_t AdjustXtoGrid(uint32_t x)
	{
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		uint32_t x_grid = (uint32_t)(tm.m_aveCharWidth * 2);
		uint32_t mod_x = x % x_grid;
		if (mod_x)
		{
			return (x - mod_x) + x_grid;
		}
		return x;
	}
};

