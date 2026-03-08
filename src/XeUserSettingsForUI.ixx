module;

#include "XeAssert.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

export module Xe.UserSettingsForUI;

export import Xe.UserSettings;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef std::function<void(const ChangedSettings&)> NotifyChangedSettingsCallback;

export class CXeUserSettingForUI : public CXeUserSetting
{
public:
	CXeUserSettingForUI() = default;
	CXeUserSettingForUI(const CXeUserSetting& other) : CXeUserSetting(other) {}
	std::wstring	m_parent_settings_name;
	uint32_t	m_ctrl_id;
};

export class CXeUserSettingsForUI
{
	std::vector<std::wstring> m_user_settings_names;
	std::vector<CXeUserSettings> m_user_settings_list;
	std::map<std::wstring, size_t> m_user_settings_map;

	NotifyChangedSettingsCallback m_notifyCallback = nullptr;

	CXeUserSettings m_settingsEmpty;

public:
	CXeUserSettingsForUI() = default;

	// Used for unit tests
	CXeUserSettingsForUI(const std::vector<std::wstring>& user_setting_names)
		: m_user_settings_names(user_setting_names) {}

	// Used for unit tests
	void SetUserSettingsList(const std::vector<std::wstring>& user_setting_names)
	{
		m_user_settings_names = user_setting_names;
	}

	void SetNotifyCallback(NotifyChangedSettingsCallback notify_callback)
	{
		m_notifyCallback = notify_callback;
	}

	// Used for unit tests
	bool LoadSettings()
	{
		m_user_settings_list.clear();
		m_user_settings_map.clear();
		for (const std::wstring& settings_name : m_user_settings_names)
		{
			CXeUserSettings settings;
			settings.SetName(settings_name);
			settings.Load();
			m_user_settings_list.push_back(settings);
			m_user_settings_map[settings_name] = m_user_settings_list.size() - 1;
		}
		return true;
	}

	bool AddSetting(const std::wstring& setting_name, int resource_id, int json_file_type)
	{
		CXeUserSettings settings;
		settings.SetName(setting_name);
		bool isOk = settings.Load(resource_id, json_file_type);
		m_user_settings_names.push_back(setting_name);
		m_user_settings_list.push_back(settings);
		m_user_settings_map[setting_name] = m_user_settings_list.size() - 1;
		return isOk;
	}

	// Load default settings from resource and load user (modified) settings.
	// Note - XeApp calls here from InitInstance - to load settings json files from 
	//        resources ('this'.exe file).
	bool AddSettings(const std::vector<std::tuple<std::wstring, int>>& user_setting_names_and_idr,
			int json_file_type)
	{
		bool isOk = true;
		for (auto [ setting_name, resource_id ] : user_setting_names_and_idr)
		{
			CXeUserSettings settings;
			settings.SetName(setting_name);
			if (!settings.Load(resource_id, json_file_type))
			{
				isOk = false;
			}
			m_user_settings_names.push_back(setting_name);
			m_user_settings_list.push_back(settings);
			m_user_settings_map[setting_name] = m_user_settings_list.size() - 1;
		}
		return isOk;
	}

	bool Exists(const std::wstring& settings_name) const
	{
		auto it_map = m_user_settings_map.find(settings_name);
		return it_map != m_user_settings_map.end();
	}

	const CXeUserSettings* GetSettingsFromNameConst(const std::wstring& settings_name)
	{
		return _getSettingsFromName(settings_name);
	}
	CXeUserSettings* GetSettingsFromName(const std::wstring& settings_name)
	{
		return _getSettingsFromName(settings_name);
	}

	const CXeUserSettings& operator[](const std::wstring settings_name) const
	{
		return _getSettingsFromNameOrEmptyConst(settings_name);
	}

	CXeUserSettings& operator[](const std::wstring settings_name)
	{
		return _getSettingsFromNameOrEmpty(settings_name);
	}

	const std::vector<CXeUserSettings>& GetAllSettings() const { return m_user_settings_list; }

	// Note - notify callback is called if any setting has changed.
	ChangedSettings SaveChangedSettings(const CXeUserSettingsForUI& other)
	{
		ChangedSettings all_chg_settings;

		for (const CXeUserSettings& other_settings : other.m_user_settings_list)
		{
			CXeUserSettings* pSettingsInThis = _getSettingsFromName(other_settings.GetName());
			XeASSERT(pSettingsInThis);
			if (pSettingsInThis)
			{
				std::vector<ChangedSetting> chg_settings = pSettingsInThis->SaveChangedSettings(other_settings);
				all_chg_settings.AddChangedSettings(chg_settings);
			}
		}

		if (all_chg_settings.AnyChanged() && m_notifyCallback)
		{
			m_notifyCallback(all_chg_settings);
		}
		return all_chg_settings;
	}

	// Call the callback defined by SetNotifyCallback.
	void TriggerNotifyCallback(const ChangedSettings& chg_settings)
	{
		if (m_notifyCallback)
		{
			m_notifyCallback(chg_settings);
		}
	}

protected:
	CXeUserSettings* _getSettingsFromName(const std::wstring& settings_name)
	{
		auto it_map = m_user_settings_map.find(settings_name);
		if (it_map != m_user_settings_map.end())
		{
			return &(m_user_settings_list.at(it_map->second));
		}
		return nullptr;
	}

	const CXeUserSettings& _getSettingsFromNameOrEmptyConst(const std::wstring& settings_name) const
	{
		auto it_map = m_user_settings_map.find(settings_name);
		if (it_map != m_user_settings_map.end())
		{
			return m_user_settings_list.at(it_map->second);
		}
		XeASSERT(false);	// Settings should exist.
		return m_settingsEmpty;
	}
	CXeUserSettings& _getSettingsFromNameOrEmpty(const std::wstring& settings_name)
	{
		auto it_map = m_user_settings_map.find(settings_name);
		if (it_map != m_user_settings_map.end())
		{
			return m_user_settings_list.at(it_map->second);
		}
		XeASSERT(false);	// Settings should exist.
		return m_settingsEmpty;
	}
};

/////////////////////////////////////////////////////////////////////////////
// The one and only CXeUserSettingsForUI object.

export CXeUserSettingsForUI s_xeUIsettings;	// static class instance

/////////////////////////////////////////////////////////////////////////////

