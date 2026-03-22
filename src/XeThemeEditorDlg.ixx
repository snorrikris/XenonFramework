module;

#include "os_minimal.h"
#include <functional>
#include <memory>
#include <algorithm>
#include <iterator>
#include <vector>
#include <string>
#include <optional>

#include "XeResource.h"

export module Xe.ThemeEditorDlg;

import Xe.BaseDlg;
import Xe.GridDataSource;
import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.ThemeIF;
import Xe.FileHelpers;
import Xe.FileDlgHelpers;
import Xe.RGB_HLS_Color;
import Xe.UserSettings;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export typedef std::function<void()> NotifyColorChangedCallbackFunc;
export typedef std::function<void(const std::wstring&)> ChangeThemeCallbackFunc;

enum class CID_SortBy { number, name, description, value, luminance, saturation, hue };
enum class Palette_SortBy { label, value, luminance, saturation, hue };

export class CXeThemeEditorDlg : public CXeBaseDlg
{
protected:
	XeThemeIF* m_theme = nullptr;

	std::vector<CID_Color> m_colorIDs_unchanged;
	std::vector<PaletteColor> m_palette_unchanged;

	bool m_edit_palette_only = false;
	BOOL m_isDebugOnlyControlsEnabled = FALSE;
	std::vector<UINT> m_cid_edit_buttons{ IDC_ADD_NEW_COLORID, IDC_DELETE_COLORID, IDC_SAVE_COLORIDS,
		IDC_RENAME_CID, IDC_EDIT_CID_DESCRIPTION };

	CXeListBoxEx*	m_colorIDsListBox = nullptr;
	CXeListBoxEx*	m_paletteListBox = nullptr;
	CXeComboBoxEx*	m_cid_paletteComboBox = nullptr;
	CXeComboBoxEx*	m_paletteDefAsComboBox = nullptr;

	CID_SortBy		m_sortBy_CID_LB = CID_SortBy::name;
	Palette_SortBy	m_sortBy_Palette_LB = Palette_SortBy::label;

	std::wstring	m_last_asked_theme_name;

	NotifyColorChangedCallbackFunc m_notifyColorChangedCallback = nullptr;
	ChangeThemeCallbackFunc m_changeThemeCallback = nullptr;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_THEME_EDITOR };
#endif

#pragma region Create
public:
	CXeThemeEditorDlg(CXeViewManagerIF* pVwMgr, HWND hParent,
			NotifyColorChangedCallbackFunc notifyColorChangedCallback,
			ChangeThemeCallbackFunc changeThemeCallback)
		: CXeBaseDlg(IDD_THEME_EDITOR, pVwMgr, hParent),
			m_notifyColorChangedCallback(notifyColorChangedCallback),
			m_changeThemeCallback(changeThemeCallback)
	{
		m_settingsName = L"ThemeEditorDlg";
		m_theme = m_xeUI->GetCurrentTheme();
		m_theme->SetEditMode(true);
		XeASSERT(m_notifyColorChangedCallback && m_changeThemeCallback);
	}
	~CXeThemeEditorDlg()
	{
		m_theme->SetEditMode(false);
	}

protected:
	virtual BOOL _OnInitDialog() override
	{
#ifdef _DEBUG
		m_isDebugOnlyControlsEnabled = TRUE;
#endif
		//if (m_isDebugOnlyControlsEnabled)
		//{
		//	GetDlgItem(IDC_CLRNUM_NAME)->SendMessage(EM_SETREADONLY, FALSE);
		//	GetDlgItem(IDC_CLRNUM_COMMENT)->SendMessage(EM_SETREADONLY, FALSE);
		//}

		std::vector<GlueCtrl> glue_controls;
		BOOL result = _InitDialog(glue_controls, CSize(0, 0));
		CXeD2DButton* pClrBtn = _GetButtonControl(IDC_COLOR_ID_DEF_COLOR);
		pClrBtn->SetColorButtonMode();
		CXeD2DButton* pClrBtn2 = _GetButtonControl(IDC_PALETTE_DEF_AS_COLOR);
		pClrBtn2->SetColorButtonMode();

		_UpdateData(FALSE);

		m_colorIDsListBox = _GetListBoxControl(IDC_LB_COLOR_ID);
		m_paletteListBox = _GetListBoxControl(IDC_LB_PALETTE);
		XeASSERT(m_colorIDsListBox && m_paletteListBox);
		const XeFontMetrics& fm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		m_colorIDsListBox->SetTabStops({ (UINT)fm.DialogUnitsXtoPixels(109), (UINT)fm.DialogUnitsXtoPixels(98) });
		m_paletteListBox->SetTabStops({ (UINT)fm.DialogUnitsXtoPixels(120) });

		m_cid_paletteComboBox = _GetComboBoxControl(IDC_CB_COLOR_ID_DEF_AS_LABEL);
		m_paletteDefAsComboBox = _GetComboBoxControl(IDC_CB_PALETTE_DEF_AS);
		XeASSERT(m_cid_paletteComboBox && m_paletteDefAsComboBox);

		std::vector<ListBoxExItem> list_CID_sortBy{ L"Unsorted", L"ColorId", L"Description",
			L"Value", L"Luminance", L"Saturation", L"Hue" };
		std::vector<ListBoxExItem> list_Palette_sortBy{ L"Palette label", L"Defined label",
			L"Luminance", L"Saturation", L"Hue" };
		CXeComboBoxEx* pCB = _GetComboBoxControl(IDC_CID_SORT_BY);
		pCB->SetItemsList(list_CID_sortBy);
		pCB->SetCurSel(1);
		pCB = _GetComboBoxControl(IDC_PALETTE_SORT_BY);
		pCB->SetItemsList(list_Palette_sortBy);
		pCB->SetCurSel(0);

		_SetTitle(m_theme->GetThemeName());
		_ResetUI();

		_ShowControls(m_cid_edit_buttons, (m_isDebugOnlyControlsEnabled ? SW_SHOW : SW_HIDE));
		_ShowHidePaletteEditControls();

		_MakeCancelChangesCheckPoint();

		m_colorIDsListBox->SetCurSel(0);
		OnLB_SelChange_ColorID();

		return TRUE;
	}
#pragma endregion Create

	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender) override
	{
		if (wID == IDC_SAVE_CHANGES && wSource == BN_CLICKED)
		{
			OnBnClickedSaveChanges();
			return 0;
		}
		else if (wID == IDC_CANCEL_CHANGES && wSource == BN_CLICKED)
		{
			OnBnClickedCancelChanges();
			return 0;
		}
		else if (wID == IDC_CID_SORT_BY && wSource == CBN_SELENDOK)
		{
			OnCB_SelEndOk_ColorId_SortBy();
			return 0;
		}
		else if (wID == IDC_PALETTE_SORT_BY && wSource == CBN_SELENDOK)
		{
			OnCB_SelEndOk_Palette_SortBy();
			return 0;
		}
		else if (wID == IDC_CB_COLOR_ID_DEF_AS_LABEL && wSource == CBN_SELENDOK)
		{
			OnCB_SelEndOk_ColorId_DefLabel();
			return 0;
		}
		else if (wID == IDC_CB_PALETTE_DEF_AS && wSource == CBN_SELENDOK)
		{
			OnCB_SelEndOk_PaletteDefLabel();
			return 0;
		}
		else if (wID == IDC_LB_COLOR_ID && wSource == LBN_SELCHANGE)
		{
			OnLB_SelChange_ColorID();
			return 0;
		}
		else if (wID == IDC_LB_PALETTE && wSource == LBN_SELCHANGE)
		{
			OnLB_SelChanged_Palette();
			return 0;
		}
		else if (wID == IDC_COLOR_ID_DEF_COLOR && wSource == BN_CLICKED)
		{
			OnBnClicked_ColorId_Color();
			return 0;
		}
		else if (wID == IDC_EDIT_PALETTE_ONLY && wSource == BN_CLICKED)
		{
			OnBnClicked_EditPaletteOnly();
			return 0;
		}
		else if (wID == IDC_PALETTE_DEF_AS_COLOR && wSource == BN_CLICKED)
		{
			OnBnClicked_PaletteItem_Color();
			return 0;
		}
		else if (wID == IDC_ADD_NEW_PALETTE_ITEM && wSource == BN_CLICKED)
		{
			OnBnClicked_AddNewPaletteItem();
			return 0;
		}
		else if (wID == IDC_RENAME_PALETTE_LABEL && wSource == BN_CLICKED)
		{
			OnBnClicked_RenamePaletteItem();
			return 0;
		}
		else if (wID == IDC_DELETE_PALETTE_LABEL && wSource == BN_CLICKED)
		{
			OnBnClicked_DeletePaletteItem();
			return 0;
		}
		else if (wID == IDC_PALETTE_INVERT && wSource == BN_CLICKED)
		{
			OnBnClicked_InvertPaletteColors();
			return 0;
		}
		else if (wID == IDC_ADD_NEW_COLORID && wSource == BN_CLICKED)
		{
			OnBnClickedAddNewColorId();
			return 0;
		}
		else if (wID == IDC_DELETE_COLORID && wSource == BN_CLICKED)
		{
			OnBnClickedDeleteColorId();
			return 0;
		}
		else if (wID == IDC_SAVE_COLORIDS && wSource == BN_CLICKED)
		{
			OnBnClickedSaveColorIds();
			return 0;
		}
		else if (wID == IDC_RENAME_CID && wSource == BN_CLICKED)
		{
			OnBnClickedRenameCID();
			return 0;
		}
		else if (wID == IDC_EDIT_CID_DESCRIPTION && wSource == BN_CLICKED)
		{
			OnBnClickedEditCIDdescription();
			return 0;
		}
		return CXeBaseDlg::_OnWmCommand(wSource, wID, sender);
	}

	virtual void OnOK() override
	{
		CXeBaseDlg::OnOK();
	}

	virtual void OnCancel() override
	{
		bool canClose = !m_theme->IsChanged();
		if (m_theme->IsChanged())
		{
			if (ShowMessageBox(L"You have unsaved changes. Close anyway?", L"Theme editor", MB_ICONQUESTION | MB_YESNO)
				== IDYES)
			{
				canClose = true;
			}
		}
		if (canClose)
		{
			if (m_theme->IsChanged())
			{
				m_theme->ResetTheme(m_colorIDs_unchanged, m_palette_unchanged);
				m_notifyColorChangedCallback();
			}
			CXeBaseDlg::OnCancel();
		}
	}

protected:
#pragma region SortBy
	void _SetColorId_SortBy(CID_SortBy sortBy)
	{
		m_sortBy_CID_LB = sortBy;
		const CID_Color* pClrNum = _GetCurSelColorID();
		_ResetUI();
		int idx = _FindCIDindexInLB(pClrNum ? pClrNum->m_CIDnumber : CID::Invalid);
		m_colorIDsListBox->SetCurSel(idx);
		OnLB_SelChange_ColorID();
	}
	void OnCB_SelEndOk_ColorId_SortBy()
	{
		CXeComboBoxEx* pCB = _GetComboBoxControl(IDC_CID_SORT_BY);
		const ListBoxExItem* pCBitem = pCB->GetItemDataAtIndexConst(pCB->GetCurSel());
		if (pCBitem)
		{
			if (wcscmp(pCBitem->m_string.c_str(), L"Unsorted") == 0)
			{
				_SetColorId_SortBy(CID_SortBy::number);
			}
			else if (wcscmp(pCBitem->m_string.c_str(), L"ColorId") == 0)
			{
				_SetColorId_SortBy(CID_SortBy::name);
			}
			else if (wcscmp(pCBitem->m_string.c_str(), L"Description") == 0)
			{
				_SetColorId_SortBy(CID_SortBy::description);
			}
			else if (wcscmp(pCBitem->m_string.c_str(), L"Value") == 0)
			{
				_SetColorId_SortBy(CID_SortBy::value);
			}
			else if (wcscmp(pCBitem->m_string.c_str(), L"Luminance") == 0)
			{
				_SetColorId_SortBy(CID_SortBy::luminance);
			}
			else if (wcscmp(pCBitem->m_string.c_str(), L"Saturation") == 0)
			{
				_SetColorId_SortBy(CID_SortBy::saturation);
			}
			else if (wcscmp(pCBitem->m_string.c_str(), L"Hue") == 0)
			{
				_SetColorId_SortBy(CID_SortBy::hue);
			}
		}
	}

	void _SetPalette_SortBy(Palette_SortBy sortBy)
	{
		m_sortBy_Palette_LB = sortBy;
		const PaletteColor* pPalClr = _GetPaletteItem_at_LB_idx(m_paletteListBox->GetCurSel());
		const CID_Color* pClrID = _GetCurSelColorID();
		_ResetUI();
		if (m_edit_palette_only)
		{
			int idx = _FindPaletteItem_LBindex(pPalClr);
			m_paletteListBox->SetCurSel(idx);
			_SetSelectedPaletteItem_InUI();
		}
		else
		{
			int idx = _FindCIDindexInLB(pClrID ? pClrID->m_CIDnumber : CID::Invalid);
			m_colorIDsListBox->SetCurSel(idx);
			OnLB_SelChange_ColorID();
		}
	}

	void OnCB_SelEndOk_Palette_SortBy()
	{
		CXeComboBoxEx* pCB = _GetComboBoxControl(IDC_PALETTE_SORT_BY);
		const ListBoxExItem* pCBitem = pCB->GetItemDataAtIndexConst(pCB->GetCurSel());
		if (pCBitem)
		{
			if (wcscmp(pCBitem->m_string.c_str(), L"Palette label") == 0)
			{
				_SetPalette_SortBy(Palette_SortBy::label);
			}
			else if (wcscmp(pCBitem->m_string.c_str(), L"Defined label") == 0)
			{
				_SetPalette_SortBy(Palette_SortBy::value);
			}
			else if (wcscmp(pCBitem->m_string.c_str(), L"Luminance") == 0)
			{
				_SetPalette_SortBy(Palette_SortBy::luminance);
			}
			else if (wcscmp(pCBitem->m_string.c_str(), L"Saturation") == 0)
			{
				_SetPalette_SortBy(Palette_SortBy::saturation);
			}
			else if (wcscmp(pCBitem->m_string.c_str(), L"Hue") == 0)
			{
				_SetPalette_SortBy(Palette_SortBy::hue);
			}
		}
	}
#pragma endregion SortBy

#pragma region Edit_CID_colors
	void OnCB_SelEndOk_ColorId_DefLabel()
	{
		const ListBoxExItem* pCBitem = m_cid_paletteComboBox->GetCurSelItemDataConst();
		if (pCBitem
			&& m_theme->SetColor(_GetCurrentlySelectedColorID(), pCBitem->m_string))
		{
			_OnSelectedCIDvalueChanged();
		}
	}
	void OnBnClicked_ColorId_Color()
	{
		CXeD2DButton* pClrBtn = _GetButtonControl(IDC_COLOR_ID_DEF_COLOR);
		if (m_theme->SetColor(_GetCurrentlySelectedColorID(), pClrBtn->GetColor()))
		{
			_OnSelectedCIDvalueChanged();
		}
		bool picker_layout_mode = pClrBtn->GetColorPickerLayoutMode();
		CXeD2DButton* pClrBtn2 = _GetButtonControl(IDC_PALETTE_DEF_AS_COLOR);
		pClrBtn2->SetColorPickerLayoutMode(picker_layout_mode);
	}
	void OnLB_SelChanged_Palette()
	{
		const PaletteColor* pPaletteItem = _GetPaletteItem_at_LB_idx(m_paletteListBox->GetCurSel());
		if (!pPaletteItem)
		{
			return;
		}
		if (m_edit_palette_only)
		{
			_SetPaletteItem_InUI(pPaletteItem);
		}
		else
		{
			const CID_Color* pClrNum = _GetCurSelColorID();
			if (pClrNum
				&& m_theme->SetColor(pClrNum->m_CIDnumber, pPaletteItem->m_name))
			{
				_SetPaletteItem_InUI(pPaletteItem);
				_OnSelectedCIDvalueChanged();
			}
		}
	}
	void _OnSelectedCIDvalueChanged()
	{
		_UpdateCurrentlySelectedColorIdItem_InListBox();
		OnLB_SelChange_ColorID();
		_OnChange();
	}
	void _UpdateCurrentlySelectedColorIdItem_InListBox()
	{
		int nCurSel = m_colorIDsListBox->GetCurSel();
		const CID_Color* pColorId = _GetCurSelColorID();
		if (nCurSel >= 0 && pColorId)
		{
			ListBoxExItem lb_item = _MakeColorID_LB_item(*pColorId);
			m_colorIDsListBox->SetItemDataAtIndex(nCurSel, lb_item);
			m_colorIDsListBox->RedrawWindow();
		}
	}
#pragma endregion Edit_CID_colors

#pragma region SaveOrCancelChanges
	void OnBnClickedCancelChanges()
	{
		m_theme->ResetTheme(m_colorIDs_unchanged, m_palette_unchanged);
		_ResetUI(true);
		_OnChange();
	}

	bool _AskThemeName()
	{
		std::vector<std::wstring> theme_names = m_xeUI->GetThemeNames();
		bool isThemeNameOk = false;
		do
		{
			std::optional<std::wstring> str = m_pVwMgr->AskStringDlg(Hwnd(), L"Theme name", L"");
			if (str.has_value())
			{
				std::wstring name = str.value();
				if (std::any_of(theme_names.cbegin(), theme_names.cend(),
					[&](const std::wstring& tn) { return tn == name; }))
				{
					ShowMessageBox(L"Name already used.", L"Error", MB_ICONERROR | MB_OK);
				}
				else if (!IsValidFilename(name))
				{
					ShowMessageBox(L"Invalid characters in name.", L"Error", MB_ICONERROR | MB_OK);
				}
				else
				{
					m_last_asked_theme_name = name;
					isThemeNameOk = true;
				}
			}
			else
			{
				return false;
			}
		} while (!isThemeNameOk);
		return true;
	}

	void OnBnClickedSaveChanges()
	{
		std::wstring theme_name = m_theme->GetThemeName();
		if (m_theme->IsThemeAppDefined())
		{
			if (m_last_asked_theme_name.size() == 0)
			{
				_AskThemeName();
			}
			theme_name = m_last_asked_theme_name;
		}
		std::wstring user_data_folder = GetCurrentUserAppDataFolder(m_xeUI->GetAppName());
		std::wstring json_pathname = user_data_folder + L"\\Theme_" + theme_name + L".json";
		std::wstring error_str = m_theme->SaveThemeJson(json_pathname, theme_name);
		if (error_str.size())
		{
			ShowMessageBox(error_str.c_str(), L"Failed to save theme data.", MB_ICONERROR | MB_OK);
			return;
		}
		s_xeLastUsedUIsettings.Set(L"LastUsedTheme", theme_name);

		// Change theme for the app - to 'this' theme.
		m_changeThemeCallback(theme_name);

		// Set current UI font to all controls - because theme has changed.
		_ApplyFontChangeToControls();

		// Apply new theme to all 'our' theme editor controls.
		m_theme = m_xeUI->GetCurrentTheme();
		m_theme->SetEditMode(true);
		std::wstring new_theme_name = m_theme->GetThemeName();
		XeASSERT(theme_name == new_theme_name);
		_SetTitle(theme_name);
		_ResetUI(true);
		_MakeCancelChangesCheckPoint();
	}
#pragma endregion SaveOrCancelChanges

#pragma region Edit_CID_list
	bool _IsValidLabelName(const std::wstring& label)
	{
		if (label.size() == 0) { return false; }
		std::wstring inv(L" \\/<>|\":?*'°-+,.!#$%&()={}[]´~@;^\t\r\n");
		wchar_t c1 = label.at(0);
		bool firstChrValid = (c1 >= L'A' && c1 <= L'Z') || (c1 >= L'a' && c1 <= L'z');
		if (!firstChrValid || std::any_of(inv.cbegin(), inv.cend(),
			[&](wchar_t c) { return label.find(c) != std::string::npos; }))
		{
			return false;
		}
		return true;
	}

	void OnBnClickedAddNewColorId()
	{
		std::optional<std::wstring> str = m_pVwMgr->AskStringDlg(Hwnd(), L"CID name", L"");
		if (!str.has_value())
		{
			return;
		}
		std::wstring cid_name = str.value();
		if (cid_name.size() == 0 || m_theme->FindColorIDbyNameConst(cid_name))
		{
			ShowMessageBox(L"CID name already in use.", L"Add new Color ID error.", MB_ICONERROR | MB_OK);
			return;
		}
		if (!_IsValidLabelName(cid_name))
		{
			ShowMessageBox(L"CID name contains invalid characters.", L"Add new Color ID error.", MB_ICONERROR | MB_OK);
			return;
		}
		if (!m_theme->AddNewCIDname(cid_name))
		{
			ShowMessageBox(L"Add new CID name failed.", L"Add new Color ID error.", MB_ICONERROR | MB_OK);
			return;
		}
		const CID_Color* pClr = m_theme->FindColorIDbyNameConst(cid_name);
		XeASSERT(pClr);
		if (pClr)
		{
			_ResetUI();
			int idx = _FindColorID_LB_item_index(*pClr);
			m_colorIDsListBox->SetCurSel(idx);
			OnLB_SelChange_ColorID();
		}
	}

	void OnBnClickedRenameCID()
	{
		const CID_Color* pClr = _GetCurSelColorID();
		if (!pClr) { return; }
		std::wstring old_name = pClr->m_CIDname;
		std::optional<std::wstring> str = m_pVwMgr->AskStringDlg(Hwnd(), L"Rename " + old_name, old_name);
		if (!str.has_value() && str.value() == old_name) { return; }
		if (m_theme->RenameCIDname(pClr->m_CIDnumber, str.value()))
		{
			_ResetUI(true);
		}
		else
		{
			ShowMessageBox(L"Rename CID name failed.", L"Theme editor", MB_ICONERROR | MB_OK);
		}
	}

	void OnBnClickedEditCIDdescription()
	{
		const CID_Color* pClr = _GetCurSelColorID();
		if (!pClr) { return; }
		std::wstring comment = pClr->m_comment;
		std::optional<std::wstring> str = m_pVwMgr->AskStringDlg(Hwnd(), pClr->m_CIDname + L" description", comment);
		if (!str.has_value() || str.value() == comment) { return; }
		if (m_theme->SetCID_description(pClr->m_CIDnumber, str.value()))
		{
			_ResetUI(true);
		}
		else
		{
			ShowMessageBox(L"Edit CID description failed.", L"Theme editor", MB_ICONERROR | MB_OK);
		}
	}

	void OnBnClickedDeleteColorId()
	{
		const CID_Color* pClr = _GetCurSelColorID();
		if (!pClr) { return; }
		std::wstring question = L"Delete '" + pClr->m_CIDname + L"'. Are you sure?";
		if (ShowMessageBox(question.c_str(), L"Theme editor", MB_ICONQUESTION | MB_YESNO) != IDYES) { return; }
		if (m_theme->DeleteCID(pClr->m_CIDnumber))
		{
			_ResetUI(true);
		}
		else
		{
			ShowMessageBox(L"Delete CID failed.", L"Theme editor.", MB_ICONERROR | MB_OK);
		}
	}

	void OnBnClickedSaveColorIds()
	{
		std::wstring lastUsedFolder = s_xeLastUsedUIsettings.GetString_or_Val(L"LastUsedColorIDsFolder", L"");
		//CFolderPickerDialog folderDlg(lastUsedFolder.c_str(), 0, CWnd::FromHandle(Hwnd()), 0);
		//folderDlg.m_ofn.lpstrTitle = _T("Select folder containing ColorIDs.json and ColorIDs.h files");
		//if (folderDlg.DoModal() != IDOK)
		//{
		//	return;
		//}
		//std::wstring folder = folderDlg.GetPathName().GetString();
		std::wstring folder = ShowFolderPickerDlg(lastUsedFolder);
		if (!folder.size())
		{
			return;
		}

		std::wstring msg;
		std::wstring json_error = m_theme->SaveColorIDsJson(folder + L"\\ColorIDs.json");
		if (json_error.size())
		{
			msg = L"Save ColorIDs.json failed:\r\n" + json_error + L"\r\n";
		}
		if (!m_theme->SaveColorIDsIncludeFile(folder + L"\\ColorIDs.h"))
		{
			msg += L"Save ColorIDs.h failed:\r\n" + GetSystemErrorDescW(GetLastError()) + L"\r\n";
		}
		std::wstring report_pathname = folder + L"\\ColorIDs_changes_report.txt";
		bool isReadOk;
		std::string report = ReadTextFile(report_pathname, isReadOk);
		report += FILETIMEX::LocalNow().ToStr(DATEFMT::YMD_HMS) + "\n";
		report += xet::to_astr(m_theme->GetCID_changes_report());
		if (!WriteTextFile(report_pathname, report))
		{
			msg += L"Save changes report failed:\r\n" + GetSystemErrorDescW(GetLastError());
		}
		if (msg.size() == 0)
		{
			msg = L"Changes report has been saved as:\r\n" + report_pathname;
			ShowMessageBox(msg.c_str(), L"Save ColorIDs", MB_OK);
			s_xeLastUsedUIsettings.Set(L"LastUsedColorIDsFolder", folder);
			_ResetUI(true);
		}
		else
		{
			ShowMessageBox(msg.c_str(), L"Save ColorIDs", MB_ICONERROR | MB_OK);
		}
	}
	
	int _FindColorID_LB_item_index(const CID_Color& cid_item)
	{
		const std::vector<ListBoxExItem>& items = m_colorIDsListBox->GetItemsList();
		int idx = 0;
		for (const ListBoxExItem& item : items)
		{
			if (item.m_item_data.GetColorID() == cid_item.m_CIDnumber)
			{
				return idx;
			}
			++idx;
		}
		return -1;
	}
#pragma endregion Edit_CID_list

#pragma region PopulateComboAndListBoxes
	ListBoxExItem _MakeColorID_LB_item(const CID_Color& item)
	{
		std::wstring s(item.m_CIDname + L"\t" + item.m_value + L"\t" + item.m_comment);
		ListBoxExItem lb_item(s);
		lb_item.m_item_data.SetColorID(item.m_CIDnumber);
		lb_item.m_item_data.m_isColorSquare = 1;
		lb_item.m_item_data.m_isChanged = item.m_isChanged;
		return lb_item;
	}

	bool _CompareCIDitems(const CID_Color& itA, const CID_Color& itB) const
	{
		CRGB_HLS_Color ccA(itA.m_rgb), ccB(itB.m_rgb);
		switch (m_sortBy_CID_LB)
		{
		case CID_SortBy::number:
			return itA.m_CIDnumber < itB.m_CIDnumber;
		case CID_SortBy::name:
			return itA.m_CIDname < itB.m_CIDname;
		case CID_SortBy::description:
			return itA.m_comment < itB.m_comment;
		case CID_SortBy::value:
			return itA.m_value < itB.m_value;
		case CID_SortBy::luminance:
			return ccA.GetLuminance() < ccB.GetLuminance();
		case CID_SortBy::saturation:
			return ccA.GetSaturation() < ccB.GetSaturation();
		case CID_SortBy::hue:
			return ccA.GetHue() < ccB.GetHue();
		}
		return false;
	}

	void _LoadColorID_LBcontents(bool isKeepSelected = false)
	{
		std::vector<CID_Color> clrIDs = m_theme->GetColorIDsConst(); /* copy */
		std::sort(clrIDs.begin(), clrIDs.end(),
			[this](const CID_Color& itA, const CID_Color& itB)
			{ return _CompareCIDitems(itA, itB); });
		std::vector<ListBoxExItem> list;
		for (const CID_Color& item : clrIDs)
		{
			if (item.m_CIDname == L"unused" || item.m_CIDname == L"Invalid") { continue; }
			list.push_back(_MakeColorID_LB_item(item));
		}
		m_colorIDsListBox->SetItemsList2(list, isKeepSelected);
	}

	ListBoxExItem _MakePalette_LB_Item(const PaletteColor& clrDef)
	{
		std::wstring s(clrDef.m_name + L"\t" + clrDef.m_def);
		ListBoxExItem lb_item(s);
		lb_item.m_item_data.SetColor(clrDef.m_rgb);
		lb_item.m_item_data.m_isColorSquare = 1;
		lb_item.m_item_data.m_isChanged = clrDef.m_isChanged;
		return lb_item;
	}

	ListBoxExItem _MakePalette_CB_Item(const PaletteColor& clrDef)
	{
		ListBoxExItem lb_item(clrDef.m_name);
		lb_item.m_item_data.SetColor(clrDef.m_rgb);
		lb_item.m_item_data.m_isColorSquare = 1;
		return lb_item;
	}

	bool _ComparePaletteItems(const PaletteColor& itA, const PaletteColor& itB) const
	{
		CRGB_HLS_Color ccA(itA.m_rgb), ccB(itB.m_rgb);
		switch (m_sortBy_Palette_LB)
		{
		case Palette_SortBy::label:
			return itA.m_name < itB.m_name;
		case Palette_SortBy::value:
			return itA.m_def < itB.m_def;
		case Palette_SortBy::luminance:
			return ccA.GetLuminance() < ccB.GetLuminance();
		case Palette_SortBy::saturation:
			return ccA.GetSaturation() < ccB.GetSaturation();
		case Palette_SortBy::hue:
			return ccA.GetHue() < ccB.GetHue();
		}
		return false;
	}

	void _LoadPaletteLBcontents(bool isKeepSelected = false)
	{
		m_cid_paletteComboBox->ResetContent();
		std::vector<PaletteColor> clrDefs = m_theme->GetPaletteConst(); /* copy */
		std::sort(clrDefs.begin(), clrDefs.end(),
				[this](const PaletteColor& itA, const PaletteColor& itB)
				{ return _ComparePaletteItems(itA, itB); });
		std::vector<ListBoxExItem> list_LB, list_CB;
		for (const PaletteColor& clrDef : clrDefs)
		{
			list_LB.push_back(_MakePalette_LB_Item(clrDef));
			list_CB.push_back(_MakePalette_CB_Item(clrDef));
		}
		m_paletteListBox->SetItemsList2(list_LB, isKeepSelected);
		std::sort(list_CB.begin(), list_CB.end(),
				[](const ListBoxExItem& itA, const ListBoxExItem& itB)
				{ return itA.m_string < itB.m_string; });
		m_cid_paletteComboBox->SetItemsList(list_CB);
	}
#pragma endregion PopulateComboAndListBoxes

#pragma region Helpers
	CID _GetCurrentlySelectedColorID() const
	{
		int nCurSel = m_colorIDsListBox->GetCurSel();
		if (nCurSel >= 0)
		{
			const ListBoxExItem* pLB_Item = m_colorIDsListBox->GetItemDataAtIndexConst(nCurSel);
			if (pLB_Item)
			{
				return pLB_Item->m_item_data.GetColorID();
			}
		}
		return CID::Invalid;
	}

	const CID_Color* _GetCurSelColorID() const
	{
		int nCurSel = m_colorIDsListBox->GetCurSel();
		if (nCurSel >= 0)
		{
			const ListBoxExItem* pLB_Item = m_colorIDsListBox->GetItemDataAtIndexConst(nCurSel);
			if (pLB_Item)
			{
				return m_theme->FindColorIDConst(pLB_Item->m_item_data.GetColorID());
			}
		}
		return nullptr;
	}

	int _FindCIDindexInLB(CID cid)
	{
		const std::vector<ListBoxExItem>& lb_items = m_colorIDsListBox->GetItemsList();
		int idx = 0;
		for (const ListBoxExItem& item : lb_items)
		{
			if (item.m_item_data.GetColorID() == cid)
			{
				return idx;
			}
			++idx;
		}
		return -1;
	}

	std::wstring _GetPaletteItemNameFromLBitemString(const std::wstring& str) const
	{
		auto pos = str.find(L'\t');
		std::wstring pal_item_name = pos != std::string::npos ? str.substr(0, pos) : str;
		return pal_item_name;
	}

	int _FindPaletteItem_LBindex(const PaletteColor* pPalClr) const
	{
		if (!pPalClr) { return -1; }
		const std::vector<ListBoxExItem>& lb_items = m_paletteListBox->GetItemsList();
		int idx = 0;
		for (const ListBoxExItem& item : lb_items)
		{
			std::wstring pal_item_name = _GetPaletteItemNameFromLBitemString(item.m_string);
			if (pal_item_name == pPalClr->m_name)
			{
				return idx;
			}
			++idx;
		}
		return - 1;
	}

	const PaletteColor* _GetPaletteItem_at_LB_idx(int idx) const
	{
		const ListBoxExItem* pLBitem = m_paletteListBox->GetItemDataAtIndexConst(idx);
		if (!pLBitem) { return nullptr; }
		std::wstring pal_item_name = _GetPaletteItemNameFromLBitemString(pLBitem->m_string);
		const PaletteColor* pPalClr = m_theme->FindPaletteColorConst(pal_item_name);
		return pPalClr;
	}

	void OnLB_SelChange_ColorID()
	{
		const CID_Color* pClrNum = _GetCurSelColorID();
		if (pClrNum)
		{
			_SetControlString(IDC_CLRNUM_NAME, pClrNum->m_CIDname.c_str());
			if (m_isDebugOnlyControlsEnabled)
			{
				_SetControlString(IDC_CLRNUM_ID, std::to_wstring((int)pClrNum->m_CIDnumber).c_str());
			}
			_SetControlString(IDC_CLRNUM_COMMENT, pClrNum->m_comment.c_str());

			const PaletteColor* pPalClr = m_theme->FindPaletteColorConst(pClrNum->m_value);
			m_paletteListBox->SetCurSel(_FindPaletteItem_LBindex(pPalClr));
			CXeD2DButton* pClrBtn = _GetButtonControl(IDC_COLOR_ID_DEF_COLOR);
			pClrBtn->SetColor(pClrNum->m_rgb);

			if (pPalClr)
			{
				int palette_item_idx = m_cid_paletteComboBox->FindStringExact(0, pPalClr->m_name.c_str());
				m_cid_paletteComboBox->SetCurSel(palette_item_idx);
				_SetControlString(IDC_DEFINED_AS, L"");
			}
			else
			{
				m_cid_paletteComboBox->SetCurSel(-1);
				_SetControlString(IDC_DEFINED_AS, pClrNum->m_value);
			}
		}
		m_colorIDsListBox->RedrawWindow();
	}

	void _SetTitle(const std::wstring& title)
	{
		std::wstring title_text = L"Theme editor - " + title;
		SetWindowText(title_text.c_str());
	}

	void _ResetUI(bool isKeepSelected = false)
	{
		const CID_Color* pClrID = _GetCurSelColorID();
		int idx_CID_LB = pClrID ? _FindCIDindexInLB(pClrID->m_CIDnumber) : -1;
		_LoadColorID_LBcontents(isKeepSelected);
		if (isKeepSelected && pClrID && idx_CID_LB >= 0)
		{
			idx_CID_LB = _FindCIDindexInLB(pClrID->m_CIDnumber);
			m_colorIDsListBox->SetCurSel(idx_CID_LB);
		}
		_LoadPaletteLBcontents(isKeepSelected);
		OnLB_SelChange_ColorID();
		_SetSaveCancelButtonsEnabledState();
		_EnableControls({ IDC_SAVE_COLORIDS }, (m_theme->IsCID_list_Changed() ? TRUE : FALSE));
	}

	void _MakeCancelChangesCheckPoint()
	{
		m_colorIDs_unchanged = m_theme->GetColorIDsConst();
		m_palette_unchanged = m_theme->GetPaletteConst();
	}

	void _OnChange()
	{
		_SetSaveCancelButtonsEnabledState();
		m_notifyColorChangedCallback();
	}

	void _SetSaveCancelButtonsEnabledState()
	{
		bool isChanged = m_theme->IsPaletteChanged() || m_theme->IsColorIDsChanged();
		_EnableControls({ IDC_CANCEL_CHANGES, IDC_SAVE_CHANGES }, isChanged);
	}

#pragma endregion Helpers

#pragma region EditPalette
	void OnCB_SelEndOk_PaletteDefLabel()
	{
		XeASSERT(m_edit_palette_only);
		if (!m_edit_palette_only) { return; }
		const PaletteColor* pPalClr = _GetPaletteItem_at_LB_idx(m_paletteListBox->GetCurSel());
		if (pPalClr)
		{
			const ListBoxExItem* pCBitem = m_paletteDefAsComboBox->GetCurSelItemDataConst();
			const PaletteColor* pSelPalClr
				= pCBitem ? m_theme->FindPaletteColorConst(pCBitem->m_string) : nullptr;
			if (pSelPalClr)
			{
				const PaletteColor* pSelPalClr_2lvl
					= m_theme->FindPaletteColorConst(pSelPalClr->m_def);
				if (pSelPalClr_2lvl)
				{
					ShowMessageBox(L"Selected Label has no color.", L"Theme editor", MB_ICONERROR | MB_OK);
					_SetPaletteItem_InUI(pPalClr);
					return;	// Two levels of indirection not allowed.
				}
				m_theme->SetPaletteItemColor(pPalClr->m_name, pSelPalClr->m_name);
				_LoadPaletteLBcontents(true);
				_SetPaletteItem_InUI(pPalClr);
				m_paletteListBox->SetCurSel(_FindPaletteItem_LBindex(pPalClr));
				_OnChange();
			}
		}
	}

	void OnBnClicked_PaletteItem_Color()
	{
		XeASSERT(m_edit_palette_only);
		if (!m_edit_palette_only) { return; }
		CXeD2DButton* pClrBtn2 = _GetButtonControl(IDC_PALETTE_DEF_AS_COLOR);
		const PaletteColor* pPalClr = _GetPaletteItem_at_LB_idx(m_paletteListBox->GetCurSel());
		if (pPalClr)
		{
			m_theme->SetPaletteItemColor(pPalClr->m_name, pClrBtn2->GetColor());
			_LoadPaletteLBcontents(true);
			m_paletteListBox->SetCurSel(_FindPaletteItem_LBindex(pPalClr));
			_SetSelectedPaletteItem_InUI();
			_OnChange();
		}
		bool picker_layout_mode = pClrBtn2->GetColorPickerLayoutMode();
		CXeD2DButton* pClrBtn = _GetButtonControl(IDC_COLOR_ID_DEF_COLOR);
		pClrBtn->SetColorPickerLayoutMode(picker_layout_mode);
	}

	void OnBnClicked_AddNewPaletteItem()
	{
		std::optional<std::wstring> str = m_pVwMgr->AskStringDlg(Hwnd(), L"Palette color label name", L"");
		if (!str.has_value())
		{
			return;
		}
		std::wstring label_name = str.value();
		if (label_name.size() == 0 || m_theme->FindPaletteColorConst(label_name))
		{
			ShowMessageBox(L"Label name already in use.", L"Theme editor", MB_ICONERROR | MB_OK);
			return;
		}
		if (!_IsValidLabelName(label_name))
		{
			ShowMessageBox(L"Label name contains invalid characters.", L"Theme editor", MB_ICONERROR | MB_OK);
			return;
		}
		if (!m_theme->AddNewPaletteItem(label_name))
		{
			ShowMessageBox(L"Add new Palette label name failed", L"Theme editor", MB_ICONERROR | MB_OK);
			return;
		}
		_LoadPaletteLBcontents(false);
		const PaletteColor* pPaletteClr = m_theme->FindPaletteColorConst(label_name);
		XeASSERT(pPaletteClr);
		m_paletteListBox->SetCurSel(_FindPaletteItem_LBindex(pPaletteClr));
		_SetPaletteItem_InUI(pPaletteClr);
		_OnChange();
	}

	void OnBnClicked_RenamePaletteItem()
	{
		XeASSERT(m_edit_palette_only);
		const PaletteColor* pPalClr = _GetPaletteItem_at_LB_idx(m_paletteListBox->GetCurSel());
		if (!pPalClr) { return; }
		std::wstring name = pPalClr->m_name;
		std::optional<std::wstring> str = m_pVwMgr->AskStringDlg(Hwnd(), L"Rename " + name, name);
		if (!str.has_value() || str.value() == name) { return; }
		if (m_theme->RenamePaletteItem(pPalClr->m_name, str.value()))
		{
			_LoadPaletteLBcontents(true);
			m_paletteListBox->SetCurSel(_FindPaletteItem_LBindex(pPalClr));
			_SetSelectedPaletteItem_InUI();
			_LoadColorID_LBcontents();
			_OnChange();
		}
		else
		{
			ShowMessageBox(L"Rename Palette label name failed.", L"Theme editor", MB_ICONERROR | MB_OK);
		}
	}

	void OnBnClicked_DeletePaletteItem()
	{
		XeASSERT(m_edit_palette_only);
		const PaletteColor* pPalClr = _GetPaletteItem_at_LB_idx(m_paletteListBox->GetCurSel());
		if (!pPalClr) { return; }
		if (m_theme->DeletePaletteItem(pPalClr->m_name))
		{
			_LoadPaletteLBcontents(true);
			_SetSelectedPaletteItem_InUI();
			_OnChange();
		}
	}

	void OnBnClicked_EditPaletteOnly()
	{
		m_edit_palette_only = !m_edit_palette_only;
		_SetEditMode();
	}

	void OnBnClicked_InvertPaletteColors()
	{
		const std::vector<PaletteColor>& clrDefs = m_theme->GetPaletteConst(); /* copy */
		for (const PaletteColor& clrDef : clrDefs)
		{
			if (clrDef.m_def.starts_with(L"RGB("))
			{
				CRGB_HLS_Color cc(clrDef.m_rgb);
				cc.SetLuminance(1 - cc.GetLuminance());
				m_theme->SetPaletteItemColor(clrDef.m_name, cc);
			}
		}
		_LoadPaletteLBcontents(true);
		OnLB_SelChanged_Palette();
		_OnChange();
	}

	void _SetSelectedPaletteItem_InUI()
	{
		_SetPaletteItem_InUI(_GetPaletteItem_at_LB_idx(m_paletteListBox->GetCurSel()));
	}

	void _SetPaletteItem_InUI(const PaletteColor* pPalClr)
	{
		CXeD2DButton* pClrBtn2 = _GetButtonControl(IDC_PALETTE_DEF_AS_COLOR);
		if (pPalClr)
		{
			std::vector<ListBoxExItem> list_CB;
			pClrBtn2->SetColor(pPalClr->m_rgb);
			m_paletteDefAsComboBox->SetCurSel(-1);
			m_paletteDefAsComboBox->Clear();
			const std::vector<PaletteColor>& clr_defs = m_theme->GetPaletteConst();
			for (const PaletteColor& clr_def : clr_defs)
			{
				if (clr_def.m_name != pPalClr->m_name)
				{
					list_CB.push_back(_MakePalette_CB_Item(clr_def));
				}
			}
			std::sort(list_CB.begin(), list_CB.end(),
					[](const ListBoxExItem& itA, const ListBoxExItem& itB)
					{ return itA.m_string < itB.m_string; });
			m_paletteDefAsComboBox->SetItemsList(list_CB);
			m_paletteDefAsComboBox->SetCurSel(
					m_paletteDefAsComboBox->FindStringExact(0, pPalClr->m_def.c_str()));
			_SetControlString(IDC_CLRDEF_DEFINED_AS, pPalClr->m_def);
			_SetControlString(IDC_PALETTE_ITEM, pPalClr->m_name);

			const std::vector<CID_Color>& cids = m_theme->GetColorIDsConst();
			int count = (int)std::count_if(cids.begin(), cids.end(),
					[&](const CID_Color& cid) { return cid.m_value == pPalClr->m_name; });
			_EnableControls({ IDC_DELETE_PALETTE_LABEL }, count == 0);
		}
		else
		{
			pClrBtn2->SetColor(0);
			m_paletteDefAsComboBox->SetCurSel(-1);
			m_paletteDefAsComboBox->Clear();
			_SetControlString(IDC_CLRDEF_DEFINED_AS, L"");
			_SetControlString(IDC_PALETTE_ITEM, L"");
			_EnableControls({ IDC_DELETE_PALETTE_LABEL }, false);
		}
	}

	void _SetEditMode()
	{
		_SetControlString(IDC_EDIT_PALETTE_ONLY,
			m_edit_palette_only ? L"Normal edit" : L"Edit Palette");

		BOOL fEn = m_edit_palette_only ? FALSE : TRUE;

		_EnableControls({ IDC_LB_COLOR_ID, IDC_CLRNUM_NAME, IDC_CLRNUM_COMMENT, IDC_CID_SORT_BY,
			IDC_CB_COLOR_ID_DEF_AS_LABEL, IDC_COLOR_ID_DEF_COLOR }, (m_edit_palette_only ? FALSE : TRUE));
		if (m_isDebugOnlyControlsEnabled)
		{
			_EnableControls(m_cid_edit_buttons, (m_edit_palette_only ? FALSE : TRUE));
			_EnableControls({ IDC_SAVE_COLORIDS }, (m_theme->IsCID_list_Changed() ? TRUE : FALSE));
		}
		_ShowHidePaletteEditControls();
		if (m_edit_palette_only)
		{
			_SetSelectedPaletteItem_InUI();
			m_colorIDsListBox->SetCurSel(-1);
			m_cid_paletteComboBox->SetCurSel(-1);
			_SetControlString(IDC_CLRNUM_NAME, L"");
			_SetControlString(IDC_CLRNUM_COMMENT, L"");
		}
		else
		{
			m_colorIDsListBox->SetCurSel(0);
			OnLB_SelChange_ColorID();
			_SetSaveCancelButtonsEnabledState();
		}
	}

	void _ShowHidePaletteEditControls()
	{
		_ShowControls({ IDC_CB_PALETTE_DEF_AS, IDC_PALETTE_DEF_AS_COLOR, IDC_PALETTE_ITEM, IDC_STATIC1,
			IDC_STATIC2, IDC_ADD_NEW_PALETTE_ITEM, IDC_RENAME_PALETTE_LABEL,IDC_DELETE_PALETTE_LABEL,
			IDC_CLRDEF_DEFINED_AS, IDC_PALETTE_INVERT },
			(m_edit_palette_only ? SW_SHOW : SW_HIDE));
	}
#pragma endregion EditPalette
};

//BEGIN_MESSAGE_MAP(CXeThemeEditorDlg, CXeBaseDlg)
//	ON_CBN_SELENDOK(IDC_CID_SORT_BY, &CXeThemeEditorDlg::OnCB_SelEndOk_ColorId_SortBy)
//	ON_CBN_SELENDOK(IDC_PALETTE_SORT_BY, &CXeThemeEditorDlg::OnCB_SelEndOk_Palette_SortBy)
//	ON_LBN_SELCHANGE(IDC_LB_COLOR_ID, &CXeThemeEditorDlg::OnLB_SelChange_ColorID)
//	ON_CBN_SELENDOK(IDC_CB_COLOR_ID_DEF_AS_LABEL, &CXeThemeEditorDlg::OnCB_SelEndOk_ColorId_DefLabel)
//	ON_BN_CLICKED(IDC_COLOR_ID_DEF_COLOR, &CXeThemeEditorDlg::OnBnClicked_ColorId_Color)
//	ON_LBN_SELCHANGE(IDC_LB_PALETTE, &CXeThemeEditorDlg::OnLB_SelChanged_Palette)
//	ON_BN_CLICKED(IDC_CANCEL_CHANGES, &CXeThemeEditorDlg::OnBnClickedCancelChanges)
//	ON_BN_CLICKED(IDC_SAVE_CHANGES, &CXeThemeEditorDlg::OnBnClickedSaveChanges)
//	ON_BN_CLICKED(IDC_EDIT_PALETTE_ONLY, &CXeThemeEditorDlg::OnBnClicked_EditPaletteOnly)
//	ON_CBN_SELENDOK(IDC_CB_PALETTE_DEF_AS, &CXeThemeEditorDlg::OnCB_SelEndOk_PaletteDefLabel)
//	ON_BN_CLICKED(IDC_PALETTE_DEF_AS_COLOR, &CXeThemeEditorDlg::OnBnClicked_PaletteItem_Color)
//	ON_BN_CLICKED(IDC_ADD_NEW_PALETTE_ITEM, &CXeThemeEditorDlg::OnBnClicked_AddNewPaletteItem)
//	ON_BN_CLICKED(IDC_RENAME_PALETTE_LABEL, &CXeThemeEditorDlg::OnBnClicked_RenamePaletteItem)
//	ON_BN_CLICKED(IDC_DELETE_PALETTE_LABEL, &CXeThemeEditorDlg::OnBnClicked_DeletePaletteItem)
//	ON_BN_CLICKED(IDC_PALETTE_INVERT, &CXeThemeEditorDlg::OnBnClicked_InvertPaletteColors)
//	ON_BN_CLICKED(IDC_ADD_NEW_COLORID, &CXeThemeEditorDlg::OnBnClickedAddNewColorId)
//	ON_BN_CLICKED(IDC_DELETE_COLORID, &CXeThemeEditorDlg::OnBnClickedDeleteColorId)
//	ON_BN_CLICKED(IDC_SAVE_COLORIDS, &CXeThemeEditorDlg::OnBnClickedSaveColorIds)
//	ON_BN_CLICKED(IDC_RENAME_CID, &CXeThemeEditorDlg::OnBnClickedRenameCID)
//	ON_BN_CLICKED(IDC_EDIT_CID_DESCRIPTION, &CXeThemeEditorDlg::OnBnClickedEditCIDdescription)
//END_MESSAGE_MAP()

