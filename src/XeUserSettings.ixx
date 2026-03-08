module;

/*
The CXeUserSettings class helps manage application settings and user settings.

Application default settings are stored in a json file that is loaded as a resource from 'this' .exe file.
Any changes the user makes to the default settings are stored in a json file at a specified location.
Note - 'this' instance is in "default settings" mode when Load json default settings file is called.
The debug version asserts when setting does not exist and getXXX or setXXX called.

User settings - such as window position and size - are stored in other json files.
Note 'this' instance is in "ad hoc settings" mode when ctor(name) or SetName is called.

The root folder of where all json setting files are stored is set on app startup (after logging initialized).
For LVS that is: C:\Users\{user}\AppData\Local\LogViewerStudio\UserSettings\
*/

#include "XeAssert.h"
#include "nlohmann/json.hpp"
#include <string>
#include <map>

export module Xe.UserSettings;

export import Xe.UserSetting;
export import Xe.UserSettings_Changed;

import Xe.FileHelpers;
import Xe.FileHelpersJson;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using json = nlohmann::json;

export class CXeUserSettings
{
	static std::wstring m_rootFolder;				// Root folder for all json setting files for 'this' app.

	std::wstring m_pathname;						// Full pathname to 'this' settings json file.

	bool m_create_settings_automatically = false;	// setXXX members create the setting if not exists.
	bool m_is_simple_settings = false;				// Only name, type and value are saved to json

	std::wstring m_settings_name;

	// All settings for 'this' instance, map is keyed by setting name.
	std::map<std::wstring, CXeUserSetting> m_settingsMap;

	std::wstring m_description_List;	// Description used in Settings UI (on lhs - in listbox)
	std::wstring m_description_UI;	// Description used in Settings UI (on rhs)

	std::vector<CXeUserSetting> m_settingsTmp;

	CXeUserSetting m_settingEmpty;

	auto _getConst(const std::wstring& s) const
	{
		auto it = m_settingsMap.find(s);
		if (!m_create_settings_automatically)
		{
			XeASSERT(it != m_settingsMap.cend());	// setting does not exist.
		}
		return it;
	}

	auto _get(const std::wstring& s, SettingType etype)
	{
		auto it = m_settingsMap.find(s);
		if (it == m_settingsMap.cend() && m_create_settings_automatically)
		{
			// Create setting if not exist.
			CXeUserSetting setting;
			if (is_valid(etype))
			{
				setting.m_typeEnum = etype;
				setting.m_name = s;
				m_settingsMap[setting.m_name] = setting;
				it = m_settingsMap.find(s);
			}
			else { XeASSERT(false); }	// type is invalid.
		}
		XeASSERT(it != m_settingsMap.cend());	// setting does not exist.
		return it;
	}

	CXeUserSetting& _set(const std::wstring& s, SettingType etype)
	{
		auto it = m_settingsMap.find(s);
		if (it == m_settingsMap.cend() && m_create_settings_automatically)
		{
			CXeUserSetting new_setting(etype, s, L"", 0, L"", L"", L"", 0);
			new_setting.m_is_simple_settings = true;
			AddSetting(new_setting);
			it = _get(s, etype);
		}
		return it != m_settingsMap.cend() ? it->second : m_settingEmpty;
	}

	void _processSettingsTmp(bool is_default_settings = false)
	{
		for (CXeUserSetting& setting : m_settingsTmp)
		{
			setting.m_is_simple_settings = m_is_simple_settings;
			bool isFound = m_settingsMap.find(setting.m_name) != m_settingsMap.end();
			if (is_default_settings)
			{
				if (isFound)
				{
					XeASSERT(false);	// Same setting name being set again!
				}
				m_settingsMap[setting.m_name] = setting;
			}
			else	// Apply user settings
			{
				if (isFound)
				{
					m_settingsMap[setting.m_name].m_value = setting.m_value;
					m_settingsMap[setting.m_name].m_value_is_default = false;
				}
				else
				{
					m_settingsMap[setting.m_name] = setting;
				}
			}
		}
		m_settingsTmp.clear();
	}

public:

	CXeUserSettings() = default;
	CXeUserSettings(const std::wstring& settings_name)
	{
		XeASSERT(m_rootFolder.size());	// Call SetRootFolder before calling this ctor.
		m_settings_name = settings_name;
		m_pathname = m_rootFolder + settings_name + L".json";
		m_create_settings_automatically = true;
		m_is_simple_settings = true;
		Load();
	}
	//virtual ~CXeUserSettings() = default;

	// Set root folder where all json settings files are to be stored.
	// Note - if the last folder in the path does not exist - it is created.
	static bool SetRootFolder(const std::wstring& path)
	{
		if (!CreateDirectoryIfNotExists(path)) { XeASSERT(false); return false; }
		m_rootFolder = path;
		if (path[path.size() - 1] != L'\\') { m_rootFolder += L'\\'; }
		return true;
	}

	// Set name of settings for 'this' instance.
	void SetName(const std::wstring& settings_name)
	{
		XeASSERT(m_rootFolder.size());	// Call SetRootFolder before calling here.
		m_settings_name = settings_name;
		m_pathname = m_rootFolder + settings_name + L".json";
	}

	std::wstring GetName() const { return m_settings_name; }

	std::wstring GetJsonPathname() const { return m_pathname; }

	std::wstring GetSettingsName() const { return m_settings_name; }

	std::vector<CXeUserSetting> GetSortedSettings() const
	{
		std::vector<CXeUserSetting> sorted_settings;
		for (auto const& [name, settings] : m_settingsMap)
		{
			if (settings.m_display_position < 100000)
			{
				sorted_settings.push_back(settings);
			}
		}
		std::sort(sorted_settings.begin(), sorted_settings.end());
		return sorted_settings;
	}

	void CreateSettingsAutomatically()
	{
		m_create_settings_automatically = true;
		m_is_simple_settings = true;
	}

	std::tuple<std::wstring, std::wstring> GetDescription() const
	{
		return std::tuple<std::wstring, std::wstring>(m_description_List, m_description_UI);
	}

	void SetDescription(const std::wstring & descriptionList, const std::wstring& descriptionUI)
	{
		m_description_List = descriptionList;
		m_description_UI = descriptionUI;
	}

	bool AddSetting(const CXeUserSetting& setting)
	{
		auto it = m_settingsMap.find(setting.m_name);
		if (it != m_settingsMap.cend()) { return false; }
		if (!is_valid(setting.m_typeEnum)) { return false; }
		m_settingsMap[setting.m_name] = setting;
		return true;
	}

	bool Exists(const std::wstring& s) const
	{
		return m_settingsMap.find(s) != m_settingsMap.cend();
	}

	const CXeUserSetting& Get(const std::wstring& s) const
	{
		auto it = m_settingsMap.find(s);
		if (it == m_settingsMap.cend())	// setting does not exist?
		{
			//XeASSERT(false);
			return m_settingEmpty;
		}
		return it->second;
	}

	// Get existing setting value or provided value if setting does not exist.
	bool		GetBool_or_Val(const std::wstring& s, bool val) const		{ if (Exists(s)) { return Get(s).getBool(); }	return val; }
	uint8_t		GetU8_or_Val(const std::wstring& s, uint8_t val) const		{ if (Exists(s)) { return Get(s).getU8(); }		return val; }
	int8_t		GetI8_or_Val(const std::wstring& s, int8_t val) const		{ if (Exists(s)) { return Get(s).getI8(); }		return val; }
	uint16_t	GetU16_or_Val(const std::wstring& s, uint16_t val) const	{ if (Exists(s)) { return Get(s).getU16(); }	return val; }
	int16_t		GetI16_or_Val(const std::wstring& s, int16_t val) const		{ if (Exists(s)) { return Get(s).getI16(); }	return val; }
	uint32_t	GetU32_or_Val(const std::wstring& s, uint32_t val) const	{ if (Exists(s)) { return Get(s).getU32(); }	return val; }
	int32_t		GetI32_or_Val(const std::wstring& s, int32_t val) const		{ if (Exists(s)) { return Get(s).getI32(); }	return val; }
	uint64_t	GetU64_or_Val(const std::wstring& s, uint64_t val) const	{ if (Exists(s)) { return Get(s).getU64(); }	return val; }
	int64_t		GetI64_or_Val(const std::wstring& s, int64_t val) const		{ if (Exists(s)) { return Get(s).getI64(); }	return val; }
	double		GetDouble_or_Val(const std::wstring& s, double val) const	{ if (Exists(s)) { return Get(s).getDouble(); } return val; }
	std::wstring GetString_or_Val(const std::wstring& s, std::wstring val) const { if (Exists(s)) { return Get(s).getString(); } return val; }
	I32x2 GetI32x2_or_Val(const std::wstring& s, I32x2 val) const			{ if (Exists(s)) { return Get(s).getI32x2(); }	return val; }
	I32x4 GetI32x4_or_Val(const std::wstring& s, I32x4 val) const			{ if (Exists(s)) { return Get(s).getI32x4(); }	return val; }

	template <class T> void Set(const std::wstring& s, T val); // No body - specializations only
	template <> void Set<bool>(const std::wstring& s, bool val)					{ _set(s, SettingType::Bool).setBool(val);		Save(); }
	template <> void Set<int8_t>(const std::wstring& s, int8_t val)				{ _set(s, SettingType::I8).setI8(val);			Save(); }
	template <> void Set<uint8_t>(const std::wstring& s, uint8_t val)			{ _set(s, SettingType::U8).setU8(val);			Save(); }
	template <> void Set<int16_t>(const std::wstring& s, int16_t val)			{ _set(s, SettingType::I16).setI16(val);		Save(); }
	template <> void Set<uint16_t>(const std::wstring& s, uint16_t val)			{ _set(s, SettingType::U16).setU16(val);		Save(); }
	template <> void Set<int32_t>(const std::wstring& s, int32_t val)			{ _set(s, SettingType::I32).setI32(val);		Save(); }
	template <> void Set<uint32_t>(const std::wstring& s, uint32_t val)			{ _set(s, SettingType::U32).setU32(val);		Save(); }
	template <> void Set<int64_t>(const std::wstring& s, int64_t val)			{ _set(s, SettingType::I64).setI64(val);		Save(); }
	template <> void Set<uint64_t>(const std::wstring& s, uint64_t val)			{ _set(s, SettingType::U64).setU64(val);		Save(); }
	template <> void Set<double>(const std::wstring& s, double val)				{ _set(s, SettingType::Double).setDouble(val);	Save(); }
	template <> void Set<std::wstring>(const std::wstring& s, std::wstring val)	{ _set(s, SettingType::String).setString(val);	Save(); }
	template <> void Set<I32x2>(const std::wstring& s, I32x2 val)				{ _set(s, SettingType::I32x2).setI32x2(val);	Save(); }
	template <> void Set<I32x4>(const std::wstring& s, I32x4 val)				{ _set(s, SettingType::I32x4 ).setI32x4(val);	Save(); }

	void SetBinary(const std::wstring& s, const void* pData, size_t data_len)
	{
		_set(s, SettingType::Binary).setBinary(pData, data_len);
		Save();
	}

	CXeUserSetting getSetting(const std::wstring& s) const
	{
		auto it = _getConst(s);
		return it != m_settingsMap.cend() ? it->second : CXeUserSetting();
	}

	// Returns true if value has changed
	bool setValueFromOtherNoSave(const CXeUserSetting& other)
	{
		auto it = _get(other.m_name, other.m_typeEnum);
		if (it != m_settingsMap.end())
		{
			if (it->second.m_value != other.m_value)
			{
				it->second.m_value = other.m_value;
				it->second.m_value_is_default = false;
				return true;
			}
		}
		return false;
	}

	std::vector<ChangedSetting> SaveChangedSettings(const CXeUserSettings& other)
	{
		std::vector<ChangedSetting> chg_settings;
		XeASSERT(m_settings_name == other.m_settings_name);
		bool is_changed = false;
		for (auto const& [setting_name, setting] : other.m_settingsMap)
		{
			if (setValueFromOtherNoSave(setting))
			{
				is_changed = true;
				ChangedSetting chg_setting(m_settings_name, setting_name);
				chg_settings.push_back(chg_setting);
			}
		}

		if (is_changed)
		{
			Save();
		}
		return chg_settings;
	}

	// Load default settings from resource and load user (modified) settings.
	bool Load(int idr_resource_file, int json_file_type)
	{
		XeASSERT(m_pathname.size() > 0);	// Call SetSettingsFilePathname first.
		m_settingsTmp.clear();
		m_settingsMap.clear();

		uint32_t size = 0;
		const char* data = NULL;
		if (LoadFileInResource(idr_resource_file, json_file_type, size, data))
		{
			XeASSERT(data && size);
			std::string json_str(data, size);
			auto j2 = json::parse(json_str);
			j2.at("UserSettings").get_to(m_settingsTmp);
			_processSettingsTmp(true);
			std::string utf8;
			if (j2.find("DescriptionList") != j2.end())
			{
				j2.at("DescriptionList").get_to(utf8);
				this->m_description_List = xet::fromUTF8toWStr(utf8);
			}
			if (j2.find("DescriptionUI") != j2.end())
			{
				j2.at("DescriptionUI").get_to(utf8);
				this->m_description_UI = xet::fromUTF8toWStr(utf8);
			}

			return Load();
		}
		return false;
	}

	// Load default settings from json file and load user (modified) settings.
	bool Load(const std::wstring& pathname)
	{
		XeASSERT(m_pathname.size() > 0);	// Call SetSettingsFilePathname first.
		m_settingsTmp.clear();
		m_settingsMap.clear();

		bool isOk = LoadJsonDataFromFile(pathname, [this](json& j)
			{
				j.at("UserSettings").get_to(this->m_settingsTmp);
				std::string utf8;
				if (j.find("DescriptionList") != j.end())
				{
					j.at("DescriptionList").get_to(utf8);
					this->m_description_List = xet::fromUTF8toWStr(utf8);
				}
				if (j.find("DescriptionUI") != j.end())
				{
					j.at("DescriptionUI").get_to(utf8);
					this->m_description_UI = xet::fromUTF8toWStr(utf8);
				}
			});
		if (isOk)
		{
			_processSettingsTmp(true);

			return Load();
		}
		return false;
	}

	// Load user modified settings.
	bool Load()
	{
		bool isOk = true;
		if (FileExists(m_pathname))
		{
			isOk = LoadJsonDataFromFile(m_pathname, [this](json& j)
				{
					j.at("UserSettings").get_to(this->m_settingsTmp);
					std::string utf8;
					if (j.find("DescriptionList") != j.end())
					{
						j.at("DescriptionList").get_to(utf8);
						this->m_description_List = xet::fromUTF8toWStr(utf8);
					}
					if (j.find("DescriptionUI") != j.end()) 
					{
						j.at("DescriptionUI").get_to(utf8);
						this->m_description_UI = xet::fromUTF8toWStr(utf8);
					}
				});
			_processSettingsTmp();
		}
		return isOk;
	}

	// Save user modified settings.
	void Save()
	{
		m_settingsTmp.clear();
		for (auto const& [name, value] : m_settingsMap)
		{
			if (!value.m_value_is_default)
			{
				m_settingsTmp.push_back(value);
			}
		}

		SaveJsonDataToFile(m_pathname, [this](json& j)
			{
				if (m_description_List.size())
				{
					j = json{ {"UserSettings", this->m_settingsTmp},
					{"DescriptionList", xet::toUTF8(this->m_description_List)},
					{"DescriptionUI", xet::toUTF8(this->m_description_UI)} };
				}
				else
				{
					j = json{ {"UserSettings", this->m_settingsTmp} };
				}
			});
	}
};

std::wstring CXeUserSettings::m_rootFolder;

/////////////////////////////////////////////////////////////////////////////
// Static (global) settings to store last used settings for UI
// s_xeLastUsedUIsettings is initialized in LogViewerStudio.cpp at startup.
// Note - when adding new settings remember that the setting MUST exist
// before it is used. LVSState::CreateState is a good place to add settings.

export CXeUserSettings s_xeLastUsedUIsettings;	// static class instance

/////////////////////////////////////////////////////////////////////////////

