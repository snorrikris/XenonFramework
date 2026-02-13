module;

#include "os_minimal.h"
#include <string>
#include <vector>
#include "ColorIDs.h"	// CID enum

export module Xe.ThemeIF;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export struct CID_Color
{
	// CID color (from ColorIDs.json).
	CID m_CIDnumber;
	std::wstring m_CIDname;
	std::wstring m_comment;

	bool m_isChanged = false;	// Only used by ThemeEditor

	// CID color value (from Theme_X.json)
	std::wstring m_value;	// Is either label from palette or RGB string value.
	COLORREF m_rgb;			// The actual RGB color parsed from m_value.

	bool operator<(const CID_Color& rhs) const { return m_CIDnumber < rhs.m_CIDnumber; }
};

export struct PaletteColor
{
	std::wstring m_name;
	std::wstring m_def;
	COLORREF m_rgb;
	bool m_isValid;
	bool m_isChanged = false;	// Only used by ThemeEditor
};

export struct ThemeInfoJson
{
	std::wstring m_theme_name;
	bool m_isAppDefined = false;
	std::wstring m_based_on;			// Theme name that 'this' theme is based on.
	std::wstring m_json_pathname;
	std::string m_json_data;
};
/*
* Note - "based on" theme is used for future compatibility.
* User defined theme will have missing colors after some future LVS release.
* Those missing colors are retrieved from the "based on" theme.
*/

export class XeThemeIF
{
public:
	virtual std::wstring GetThemeName() const = 0;
	virtual std::wstring GetThemeBasedOnName() const = 0;
	virtual bool IsThemeAppDefined() const = 0;

	// Get the RGB value for a CID enum.
	virtual COLORREF GetColor(CID ID) const = 0;

	//virtual HBRUSH GetHBRUSH(CID uCID) = 0;
	//virtual CBrush* GetBrush(CID uCID) = 0;
	//virtual HPEN GetHPEN(CID uCID) = 0;
	//virtual CPen* GetPen(CID uCID) = 0;
	//virtual void DeletePensAndBrushes() = 0;

	virtual std::wstring LoadFromJson(const std::string& colorIDs_json,
		const std::string& theme_json, const std::vector<ThemeInfoJson>& all_theme_list) = 0;

	////////////////////////////////////////////////////////////////////////////////////////
	// All the functions below are used by theme editor only.

	virtual bool IsInEditMode() const = 0;
	virtual void SetEditMode(bool isEditmode) = 0;

	virtual std::wstring SaveThemeJson(const std::wstring& pathname, const std::wstring& theme_name) = 0;

	// Generate and save "ColorIDs.json" file.
	virtual std::wstring SaveColorIDsJson(const std::wstring& pathname) const = 0;

	// Generate and Save "ColorIDs.h" file.
	virtual bool SaveColorIDsIncludeFile(const std::wstring& pathname) const = 0;

	virtual bool IsChanged() const = 0;
	virtual bool IsPaletteChanged() const = 0;
	virtual bool IsColorIDsChanged() const = 0;
	virtual bool IsCID_list_Changed() const = 0;

	virtual const std::vector<CID_Color>& GetColorIDsConst() const = 0;

	virtual const CID_Color* FindColorIDConst(CID id) const = 0;
	virtual const CID_Color* FindColorIDbyNameConst(const std::wstring& name) const = 0;

	virtual bool SetColor(CID ID, const std::wstring& def_name) = 0;
	virtual bool SetColor(CID ID, COLORREF rgb) = 0;

	virtual const std::vector<PaletteColor>& GetPaletteConst() const = 0;

	virtual const PaletteColor* FindPaletteColorConst(const std::wstring& def_name) = 0;

	virtual bool SetPaletteItemColor(const std::wstring& def_name, const std::wstring& def_value) = 0;
	virtual bool SetPaletteItemColor(const std::wstring& def_name, COLORREF rgb) = 0;

	virtual bool AddNewPaletteItem(const std::wstring& def_label) = 0;
	virtual bool RenamePaletteItem(const std::wstring& old_def_name, const std::wstring& new_def_name) = 0;
	virtual bool DeletePaletteItem(const std::wstring& def_label) = 0;

	virtual void ResetTheme(const std::vector<CID_Color>& colorIDs, const std::vector<PaletteColor>& palette) = 0;

	virtual bool AddNewCIDname(const std::wstring& cid_name) = 0;
	virtual bool RenameCIDname(CID ID, const std::wstring& new_cid_name) = 0;
	virtual bool DeleteCID(CID ID) = 0;
	virtual bool SetCID_description(CID ID, const std::wstring& desc) = 0;
	virtual std::wstring GetCID_changes_report() = 0;
};

