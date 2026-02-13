module;

#include <string>
#include <vector>

export module Xe.UserSettings_Changed;

export struct ChangedSetting
{
	std::wstring		m_settings_name;
	std::wstring		m_setting_name;

	ChangedSetting() = default;
	ChangedSetting(const std::wstring& settings_name, const std::wstring& setting_name)
		: m_settings_name(settings_name), m_setting_name(setting_name) {}
	ChangedSetting(const ChangedSetting& other)
		: m_settings_name(other.m_settings_name), m_setting_name(other.m_setting_name) {}
};

export struct ChangedSettings
{
	std::vector<ChangedSetting> m_chg_settings;

	bool AnyChanged() { return m_chg_settings.size(); }

	void AddChangedSettings(const std::vector<ChangedSetting>& chg_settings)
	{
		if (chg_settings.size())
		{
			m_chg_settings
				.insert(m_chg_settings.end(), chg_settings.begin(), chg_settings.end());
		}
	}

	std::vector<ChangedSetting> GetChanged(const std::wstring& settings_name) const
	{
		std::vector<ChangedSetting> chg;
		for (const ChangedSetting& item : m_chg_settings)
		{
			if (item.m_settings_name == settings_name)
			{
				chg.push_back(item);
			}
		}
		return chg;
	}

	bool IsChanged(const std::wstring& settings_name, const std::wstring& setting_name) const
	{
		for (const ChangedSetting& item : m_chg_settings)
		{
			if (item.m_settings_name == settings_name && item.m_setting_name == setting_name)
			{
				return true;
			}
		}
		return false;
	}

	bool IsAnyChanged(const std::wstring& settings_name, const std::vector<std::wstring>& setting_names) const
	{
		for (const ChangedSetting& item : m_chg_settings)
		{
			if (item.m_settings_name == settings_name)
			{
				for (const std::wstring& setting_name : setting_names)
				{
					if (item.m_setting_name == setting_name)
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool IsAnyChanged(const std::wstring& settings_name) const
	{
		for (const ChangedSetting& item : m_chg_settings)
		{
			if (item.m_settings_name == settings_name)
			{
				return true;
			}
		}
		return false;
	}
};

