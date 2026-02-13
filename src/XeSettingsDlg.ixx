module;

#include "os_minimal.h"
#include <vector>
#include <memory>
#include <string>
#include <cstring>
#include "XeResource.h"

export module Xe.SettingsDlg;

import Xe.BaseDlg;
import Xe.UserSettingControl;
import Xe.D2DButton;
import Xe.UIcolorsIF;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

constexpr uint32_t _first_ctrl_id = 3000;
constexpr uint32_t _last_ctrl_id = 4000;

BOOL CALLBACK EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD fontType, LPVOID lp);

export class CXeSettingsDlg : public CXeBaseDlg
{
protected:
	CXeUserSettingsForUI				m_all_settings_for_ui_copy;

	std::vector<std::wstring>			m_settings_names;

	CXeUserSettings* m_pCurSelSettings = nullptr;

	std::vector<CXeUserSettingControl>	m_current_sel_settings;

	std::wstring m_cur_group_name;

	std::vector<std::unique_ptr<CXeD2DButton>> m_groupboxes;
	std::vector<std::unique_ptr<CXeD2DStatic>> m_labels;

	// Listbox position - used when resizing dialog
	int m_nLB_Xpos;
	int m_nLB_Ypos;
	int m_nLB_Rmargin;
	int m_nLB_Bmargin;
	int m_nGridWidth, m_cyLB;
	int m_nBtnWidth, m_nBtnHeight, m_nBtnYpos;
	int m_nBtnBmargin, m_nBtnOkRmargin, m_nBtnCancelRmargin;

	std::vector<std::wstring> m_monospaced_fonts, m_all_fonts;

	uint32_t m_cur_grp_right_edge = 0, m_rightmost_edge = 0;

	bool m_currentSettingsHaveChanged = false;

public:
	// Set to name of first setting "page" to show - or leave blank (=> show topmost "page").
	std::wstring m_initial_setting_name;

	CXeSettingsDlg(CXeViewManagerIF* pVwMgr, CRect* pRectCell, HWND hParent = NULL)
		: CXeBaseDlg(IDD_SETTINGS, pVwMgr, hParent, pRectCell)
	{
	}
	//~CXeSettingsDlg() {}

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTINGS };
#endif

	//afx_msg void OnSize(UINT nType, int cx, int cy)
	//{
	//	CXeBaseDlg::OnSize(nType, cx, cy);
	//}

protected:
	virtual BOOL _OnInitDialog() override
	{
		// call base class - dialog box is repositioned if needed.
		BOOL result = _InitDialog({});

		// save grid position - needed for resize calculations
		CRect rect;
		GetClientRect(rect);
		int nDlgRight = rect.right;
		int nDlgBottom = rect.bottom;

		CXeListBoxEx* pSettingsList = _GetListBoxControl(IDC_SETTINGS_LIST);
		pSettingsList->GetWindowRect(rect);
		ScreenToClient(rect);
		m_nLB_Xpos = rect.left;
		m_nLB_Ypos = rect.top;
		m_nLB_Rmargin = nDlgRight - rect.right;
		m_nLB_Bmargin = nDlgBottom - rect.bottom;
		m_cyLB = rect.Height();

		rect = GetControlRect(IDOK);
		m_nBtnYpos = rect.top;
		m_nBtnWidth = rect.Width();
		m_nBtnHeight = rect.Height();
		m_nBtnOkRmargin = nDlgRight - rect.right;

		rect = GetControlRect(IDCANCEL);
		m_nBtnCancelRmargin = nDlgRight - rect.right;
		m_nBtnBmargin = nDlgBottom - rect.bottom;

		m_all_settings_for_ui_copy = s_xeUIsettings;
		m_all_settings_for_ui_copy.SetNotifyCallback(nullptr);

		const std::vector<CXeUserSettings>& all_settings = m_all_settings_for_ui_copy.GetAllSettings();
		for (auto& settings : all_settings)
		{
			if (m_initial_setting_name.size() == 0)
			{
				m_initial_setting_name = settings.GetSettingsName();
			}
			m_settings_names.push_back(settings.GetSettingsName());
			auto [description_List, description_UI] = settings.GetDescription();
			pSettingsList->AddString(description_List.c_str());
		}
		XeASSERT(m_initial_setting_name.size()); // No settings!?!

		_AdjustListBoxWidth();

		// Note - we need a list of fonts in case settings contain font setting.
		HDC hDC = ::GetWindowDC(Hwnd());

		::EnumFontFamiliesW(hDC, (LPCTSTR)NULL,	// Enumerate fonts found on 'this' system.
			(FONTENUMPROC)EnumFamCallBack, (LPARAM)this);

		::ReleaseDC(Hwnd(), hDC);

		int sel_idx = _FindSettingsName(m_initial_setting_name);
		XeASSERT(sel_idx >= 0);
		pSettingsList->SetCurSel(sel_idx);
		SetSelectedSettings(m_initial_setting_name);

		EnableSaveAndApplyButtons(FALSE);

		return result;
	}

	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender) override
	{
		if (wID >= _first_ctrl_id && wID <= _last_ctrl_id)
		{
			OnCommandRange(wID);
		}
		else if (wID == IDC_APPLY_SETTINGS && wSource == BN_CLICKED)
		{
			OnBnClickedApply();
			return 0;
		}
		else if (wID == IDC_SETTINGS_LIST && wSource == LBN_SELCHANGE)
		{
			OnSelchangeSettingsList();
			return 0;
		}
		else if (wID >= _first_ctrl_id && wID <= _last_ctrl_id && wSource == EN_CHANGE)
		{
			OnEditChangeRange(wID);
			return 0;
		}
		else if (wID >= _first_ctrl_id && wID <= _last_ctrl_id && wSource == CBN_SELENDOK)
		{
			OnComboBoxSelEndOkRange(wID);
			return 0;
		}
		return CXeBaseDlg::_OnWmCommand(wSource, wID, sender);
	}

	void _AdjustListBoxWidth()
	{
		CXeListBoxEx* pSettingsList = _GetListBoxControl(IDC_SETTINGS_LIST);
		int cxLongestStr = m_xeUI->GetMaxTextWidthUsingFontW(m_settings_names, EXE_FONT::eUI_Font, Hwnd());
		if (cxLongestStr)
		{
			m_nGridWidth = cxLongestStr + _GetAvgCharCX(2.0);
			pSettingsList->SetWindowPos(nullptr, 0, 0, m_nGridWidth, m_cyLB,
				SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
		}
	}

public:
	BOOL EnumFontsCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD fontType)
	{
		if (fontType & TRUETYPE_FONTTYPE)
		{
			std::wstring fontname(lplf->lfFaceName);

			// u32_Flags128 = DWORD[4] = 4 * 32 bit = 128 bit
			//DWORD* u32_Flags128 = ((NEWTEXTMETRICEX*)lpntm)->ntmFontSig.fsUsb;

			//if (u32_Flags128[13 / 32] & (1 << (13 % 32)))
			//{
			//	// the font contains arabic characters (bit 13)
			//	return TRUE;
			//}
			//if (u32_Flags128[38 / 32] & (1 << (38 % 32)))
			//{
			//	// the font contains mathematical symbols (bit 38)
			//	return TRUE;
			//}
			//if (u32_Flags128[70 / 32] & (1 << (70 % 32)))
			//{
			//	// the font contains tibetan characters (bit 70)
			//	return TRUE;
			//}

			if (lplf->lfPitchAndFamily & FIXED_PITCH)
			{
				m_monospaced_fonts.push_back(fontname);
			}
			m_all_fonts.push_back(fontname);
		}
		return TRUE;

		//UNREFERENCED_PARAMETER(lpntm);
	}

protected:
	void EnableSaveAndApplyButtons(BOOL enable_state)
	{
		_EnableControls({ IDOK, IDC_APPLY_SETTINGS }, enable_state);
	}

	void OnSelchangeSettingsList()
	{
		CXeListBoxEx* pLB = _GetListBoxControl(IDC_SETTINGS_LIST);
		int nCurSel = pLB->GetCurSel();
		if (nCurSel >= 0)
		{
			if (!_ReadSettingsValuesFromControls())
			{
				return;
			}

			XeASSERT(nCurSel < m_settings_names.size());
			if (nCurSel < m_settings_names.size())
			{
				m_initial_setting_name = m_settings_names[(size_t)nCurSel];
				SetSelectedSettings(m_initial_setting_name);
			}
		}
	}

	bool _ReadSettingsValuesFromControls()
	{
		if (!m_pCurSelSettings) { return true; }
		bool isValidSettings = true;
		for (CXeUserSettingControl& item : m_current_sel_settings)
		{
			if (item.ReadSettingValueFromControl())
			{
				m_pCurSelSettings->setValueFromOtherNoSave(item);
			}
			else
			{
				std::wstring msg = L"Value for '" + item.m_display_name + L"' is out of range.";
				ShowMessageBox(msg.c_str());
				isValidSettings = false;
			}
		}
		return isValidSettings;
	}

	// Remove all settings controls and create settings controls for selected settings.
	void SetSelectedSettings(const std::wstring& settings_name)
	{
		_ResetAndClear();
		m_pCurSelSettings = m_all_settings_for_ui_copy.GetSettingsFromName(settings_name);
		if (!m_pCurSelSettings) { XeASSERT(FALSE); return; }

		auto [description_List, description_UI] = m_pCurSelSettings->GetDescription();
		std::vector<CXeUserSetting> sorted_settings = m_pCurSelSettings->GetSortedSettings();
		uint32_t next_ctrl_id = _first_ctrl_id, next_ctrl_y = m_nLB_Ypos;
		uint32_t x_beg = m_nLB_Xpos + m_nGridWidth + _GetAvgCharCX(2.0);

		// Create a label at the top for settings description.
		if (description_UI.size())
		{
			CRect rcLabel = _CalculateLabel(description_UI, x_beg, next_ctrl_y, true);
			_CreateLabel(rcLabel, description_UI, nullptr, true);
			next_ctrl_y += _GetTxtCY(2.0);
		}

		for (const CXeUserSetting& setting : sorted_settings)
		{
			uint32_t x = x_beg;

			// Create new groupbox (and resize the last one)
			if (m_cur_group_name != setting.m_group_name)
			{
				next_ctrl_y = _ResizeLastGroupbox(next_ctrl_y);

				next_ctrl_y = _CreateGroupbox(x, next_ctrl_y, setting);

				m_cur_grp_right_edge = 0;
			}
			x += m_cur_group_name.size() ? _GetAvgCharCX(3.0) : 0;	// left margin inside the groupbox (if any).

			next_ctrl_y = _AddControl(x, next_ctrl_y, setting, next_ctrl_id++);
		}
		_ResizeLastGroupbox(next_ctrl_y);

		CRect rect;
		GetClientRect(rect);
		rect.right -= _GetAvgCharCX(2.0);
		int cy_inc = m_nBtnYpos < (int)next_ctrl_y ? (int)next_ctrl_y - m_nBtnYpos : 0;
		int cx_inc = (int)m_rightmost_edge > rect.right ? (int)m_rightmost_edge - rect.right : 0;
		if (cy_inc || cx_inc)
		{
			_IncreaseWindowSize(cx_inc, cy_inc);
		}
		RedrawWindow();
	}

	void _IncreaseWindowSize(int cx_inc, int cy_inc)
	{
		CRect rect = GetControlRect(IDC_SETTINGS_LIST);
		rect.bottom += cy_inc;
		_MoveControl(IDC_SETTINGS_LIST, rect);

		std::vector<UINT> ids({ IDOK, IDC_APPLY_SETTINGS, IDCANCEL });
		for (UINT id : ids)
		{
			rect = GetControlRect(id);
			rect.OffsetRect(cx_inc, cy_inc);
			_MoveControl(id, rect);

			if (id == IDOK)
			{
				m_nBtnYpos = rect.top;
			}
		}

		GetWindowRect(rect);
		rect.right += cx_inc;
		rect.bottom += cy_inc;
		MoveWindow(rect);
	}

	void _ResetAndClear()
	{
		m_rightmost_edge = 0;
		for (auto& item : m_current_sel_settings)
		{
			item.DestroyControl();
		}
		m_current_sel_settings.clear();
		for (auto& grpbox : m_groupboxes)
		{
			::DestroyWindow(grpbox->GetSafeHwnd());
		}
		m_groupboxes.clear();
		for (auto& label : m_labels)
		{
			::DestroyWindow(label->GetSafeHwnd());
		}
		m_labels.clear();
		m_cur_group_name = L"";
		XeASSERT(m_xtooltip);
		//m_xtooltip->RemoveAllTools();
		m_xtooltip->HideTooltip();
	}

	int _GetTxtCY(double multiplier)
	{
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		return (int)((double)tm.GetHeight() * multiplier + 0.5);
	}

	int _GetAvgCharCX(double multiplier)
	{
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		return (int)((double)tm.m_aveCharWidth * multiplier + 0.5);
	}

	uint32_t _AddControl(uint32_t x, uint32_t y,
		const CXeUserSetting& setting, uint32_t ctrl_id)
	{
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		uint32_t ctrl_h = _GetTxtCY(2.0);

		CXeUserSettingControl ui_setting(m_xeUI, setting);
		ui_setting.m_parent_settings_name = m_pCurSelSettings->GetSettingsName();
		ui_setting.m_ctrl_id = ctrl_id;

		const wchar_t* tooltip = ui_setting.m_description.size() ? ui_setting.m_description.c_str() : nullptr;
		SettingType val_type = setting.m_typeEnum;
		if (val_type == SettingType::Bool)
		{
			CSize sizeCB = m_xeUI->GetTextSizeUsingFontW(Hwnd(), EXE_FONT::eUI_Font, setting.m_display_name.c_str());
			CRect rcCB(x, y, x + sizeCB.cx + _GetAvgCharCX(4.0), y + _GetTxtCY(1.2));
			ui_setting.CreateCheckBox(Hwnd(), rcCB, tooltip);
			if (m_cur_grp_right_edge < (uint32_t)rcCB.right) { m_cur_grp_right_edge = (uint32_t)rcCB.right; }
			ctrl_h = _GetTxtCY(1.7);
			m_rightmost_edge = std::max((uint32_t)rcCB.right, m_rightmost_edge);
		}
		else if (val_type == SettingType::String || setting.isFontSetting() || setting.isNumberSetting())
		{
			CRect rcLabel = _CalculateLabel(setting.m_display_name, x, y);
			_CreateLabel(rcLabel, setting.m_display_name, tooltip);
			x = rcLabel.right;

			uint32_t right_edge = 0;
			if (val_type == SettingType::String)
			{
				if (ui_setting.m_range.size())	// has range? meaning we need a combobox
				{
					right_edge = ui_setting.CreateComboBox(Hwnd(), x, y, ui_setting.getRangeStrings(), tooltip);
				}
				else
				{
					right_edge = ui_setting.CreateEditControl(Hwnd(), x, y, tooltip);
				}
			}
			else if (setting.isFontSetting())	// it's font setting
			{
				right_edge = ui_setting.CreateComboBox(Hwnd(), x, y, 
					val_type == SettingType::MonospacedFont ? m_monospaced_fonts : m_all_fonts, tooltip);
			}
			else if (setting.isNumberSetting())
			{
				right_edge = ui_setting.CreateEditControl(Hwnd(), x, y, tooltip);
			}
			if (m_cur_grp_right_edge < right_edge) { m_cur_grp_right_edge = right_edge; }
			m_rightmost_edge = std::max(right_edge, m_rightmost_edge);
		}
		else
		{
			XeASSERT(FALSE);	// value type not supported in UI as a control.
		}

		//if (ui_setting.m_hWndCtrl && ui_setting.m_description.size())	// Need tooltip?
		//{
		//	//CXeTooltipIF* tt_ctrl = m_formHelper->GetTooltipCtrl();
		//	CXeTooltipIF* tt_ctrl = m_xtooltip;
		//	PPTOOLTIP_INFO ttNfo(ui_setting.m_hWndCtrl, ui_setting.m_description);
		//	tt_ctrl->AddTooltipTool(ttNfo);
		//	//SUPER_TOOLTIP_INFO sti;
		//	//sti.rgbBg = m_xeUI->GetColor(CID::TT_Bg);
		//	//sti.rgbText = m_xeUI->GetColor(CID::TT_Text);
		//	//sti.hWnd = ui_setting.m_hWndCtrl;
		//	//sti.m_sBody = ui_setting.m_description.c_str();
		//	//tt_ctrl.AddTool(&sti);
		//}

		m_current_sel_settings.push_back(std::move(ui_setting));

		return y + ctrl_h;
	}

	uint32_t _CreateGroupbox(uint32_t x, uint32_t next_ctrl_y, const CXeUserSetting& setting)
	{
		if (m_groupboxes.size())
		{
			next_ctrl_y += _GetTxtCY(0.5);
		}
		m_cur_group_name = setting.m_group_name;
		std::unique_ptr<CXeD2DButton> grpbox = std::make_unique<CXeD2DButton>(m_xeUI);
		CRect rcGrpBox(x, next_ctrl_y, x + _GetAvgCharCX(25.0), next_ctrl_y + _GetTxtCY(2.0));
		grpbox->Create(setting.m_group_title.c_str(), WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
				rcGrpBox, GetSafeHwnd(), IDC_STATIC, nullptr);
		m_groupboxes.push_back(std::move(grpbox));
		next_ctrl_y += _GetTxtCY(1.2);
		m_rightmost_edge = std::max((uint32_t)rcGrpBox.right, m_rightmost_edge);
		return next_ctrl_y;
	}

	uint32_t _ResizeLastGroupbox(uint32_t next_ctrl_y)
	{
		size_t num_grpboxes = m_groupboxes.size();
		if (!num_grpboxes) { return next_ctrl_y; }

		// Resize last groupbox to bottom of last control
		CXeD2DButton* pLastGrpBox = m_groupboxes[num_grpboxes - 1].get();
		HWND hWndGB = pLastGrpBox->GetSafeHwnd();
		CRect rectGB, rectCtrl;
		::GetWindowRect(hWndGB, &rectGB);
		ScreenToClient(rectGB);

		size_t num_ctrls = m_current_sel_settings.size();
		::GetWindowRect(m_current_sel_settings[num_ctrls - 1].m_hWndCtrl, &rectCtrl);
		ScreenToClient(rectCtrl);
		rectGB.right = m_cur_grp_right_edge + _GetAvgCharCX(2.0);
		rectGB.bottom = rectCtrl.bottom + _GetTxtCY(0.7);
		::MoveWindow(hWndGB, rectGB.left, rectGB.top, rectGB.Width(), rectGB.Height(), true);
		next_ctrl_y += _GetTxtCY(1.2);
		m_rightmost_edge = std::max((uint32_t)rectGB.right, m_rightmost_edge);

		if (m_current_sel_settings.size())	// Any control created?
		{
			// Set group box Z order to be after (below) last control (inside the group box).
			HWND hWndLastCtrl = m_current_sel_settings.back().m_hWndCtrl;
			::SetWindowPos(hWndGB, hWndLastCtrl, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		}

		return next_ctrl_y;
	}

	CRect _CalculateLabel(const std::wstring& text, uint32_t x, uint32_t y, bool isBold = false)
	{
		CSize sizeLbl = m_xeUI->GetTextSizeUsingFontW(Hwnd(),
			isBold ? EXE_FONT::eUI_FontBold : EXE_FONT::eUI_Font, text.c_str());
		CRect rcLabel(x, y + 3, x + sizeLbl.cx + 5, y + 3 + _GetTxtCY(1.2));
		return rcLabel;
	}

	void _CreateLabel(const CRect rcLabel, const std::wstring& text, const wchar_t* tooltip, bool isBold = false)
	{
		std::unique_ptr<CXeD2DStatic> label = std::make_unique<CXeD2DStatic>(m_xeUI);
		label->Create(L"", WS_CHILD | WS_VISIBLE, 0, rcLabel, Hwnd(), IDC_STATIC, tooltip);
		label->SetFont(isBold ? EXE_FONT::eUI_FontBold : EXE_FONT::eUI_Font);
		label->SetWindowTextW(text.c_str());
		m_labels.push_back(std::move(label));
	}

	void OnCommandRange(UINT uID)
	{
		CXeUserSettingControl* pCtrl = _GetControlFromCtrlID(uID);
		if (!pCtrl) { return; }

		if (pCtrl->m_control_type == XeCtrlType::CheckBox)
		{
			pCtrl->InvertCheckboxTick();
		}
		m_currentSettingsHaveChanged = true;
		EnableSaveAndApplyButtons(TRUE);
	}

	void OnEditChangeRange(UINT uID)
	{
		CXeUserSettingControl* pCtrl = _GetControlFromCtrlID(uID);
		if (!pCtrl) { return; }

		m_currentSettingsHaveChanged = true;
		EnableSaveAndApplyButtons(TRUE);
	}

	void OnComboBoxSelEndOkRange(UINT uID)
	{
		CXeUserSettingControl* pCtrl = _GetControlFromCtrlID(uID);
		if (!pCtrl) { return; }

		m_currentSettingsHaveChanged = true;
		EnableSaveAndApplyButtons(TRUE);
	}

	CXeUserSettingControl* _GetControlFromCtrlID(uint32_t ctrl_id)
	{
		for (CXeUserSettingControl& item : m_current_sel_settings)
		{
			if (item.m_ctrl_id == ctrl_id)
			{
				return &item;
			}
		}
		return nullptr;
	}

	virtual void OnOK() override
	{
		OnBnClickedApply();

		CXeBaseDlg::OnOK();
	}

	virtual void OnCancel() override
	{
		CXeBaseDlg::OnCancel();
	}

	void OnBnClickedApply()
	{
		if (!_ReadSettingsValuesFromControls())
		{
			return;
		}

		ChangedSettings chg_settings = s_xeUIsettings.SaveChangedSettings(m_all_settings_for_ui_copy);
		m_currentSettingsHaveChanged = false;
		EnableSaveAndApplyButtons(FALSE);

		if (m_xeUI->IsFontSettingsChanged(chg_settings))
		{
			_ApplyFontChangeToControls();
			//GetDlgItem(IDC_SETTINGS_LIST)->SetFont(m_xeUI->GetFont(EXE_FONT::eUI_Font));
			_AdjustListBoxWidth();

			SetSelectedSettings(m_initial_setting_name);
		}
	}

	int _FindSettingsName(const std::wstring& settings_name)
	{
		for (int i = 0; i < m_settings_names.size(); ++i)
		{
			if (m_settings_names[i] == settings_name)
			{
				return i;
			}
		}
		return - 1;
	}

	CXeUserSettingControl* _FindControlFromPoint(const CPoint& pt, CRect& ctrl_rect)
	{
		for (CXeUserSettingControl& item : m_current_sel_settings)
		{
			::GetWindowRect(item.m_hWndCtrl, &ctrl_rect);
			ScreenToClient(ctrl_rect);
			if (ctrl_rect.PtInRect(pt))
			{
				return &item;
			}
		}
		return nullptr;
	}
};

// Reflect the callback to CXeSettingsDlg
BOOL CALLBACK EnumFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD fontType, LPVOID lp)
{
	CXeSettingsDlg* pThis = (CXeSettingsDlg*)lp;
	if (pThis)
	{
		return pThis->EnumFontsCallBack(lplf, lpntm, fontType);
	}
	return FALSE;
}

