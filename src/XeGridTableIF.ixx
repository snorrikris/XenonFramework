module;

#include "XeAssert.h"
#include <stdint.h>
#include <string>
#include <vector>

export module Xe.GridTableIF;

import Xe.UserSettings;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export class CGridTblInfo
{
protected:
	std::vector<std::wstring>	m_colNames;
	std::wstring				m_errColName;
	std::vector<uint32_t>		m_defColWidth;
	uint32_t					m_defSHcolWidth = 0;

	std::vector<uint32_t>		m_colWidth;
	uint32_t					m_SHcolWidth = 0;

public:
	void Initialize(const std::vector<std::wstring>& colNames, const std::vector<uint32_t>& defColWidths,
		uint32_t defSHwidth)
	{
		XeASSERT(colNames.size() > 0 && colNames.size() == defColWidths.size());
		m_colNames = colNames;
		m_colWidth = m_defColWidth = defColWidths;
		m_SHcolWidth = m_defSHcolWidth = defSHwidth;
		m_errColName = L"unknown";
	}
	bool IsInitialized() const { return m_colNames.size() > 0; }
	int32_t GetNumColumns() const { return (int32_t)m_colNames.size(); }
	uint32_t GetDefColWidth(int nColumnIndex) const
	{
		if (nColumnIndex == -1)
		{
			return m_defSHcolWidth;
		}
		else if (nColumnIndex >= 0 && nColumnIndex < m_colNames.size())
		{
			return m_defColWidth[nColumnIndex];
		}
		return 0;
	}
	bool GetColumnIndex(const wchar_t* szColName, int& nColumnIndex) const
	{
		std::wstring colName(szColName);
		for (size_t i = 0; i < m_colNames.size(); i++)
		{
			if (m_colNames[i] == colName)
			{
				nColumnIndex = (int)i;
				return true;
			}
		}
		return false;
	}
	const wchar_t* GetColumnName(int nColumnIndex) const
	{
		return GetColName(nColumnIndex).c_str();
	}
	const std::wstring& GetColName(int nColumnIndex) const
	{
		if (!_IsValidColIdx(nColumnIndex))
		{
			XeASSERT(false);
			return m_errColName;
		}
		return m_colNames[nColumnIndex];
	}

	int GetColWidth(int col)
	{
		if (col == -1) { return m_SHcolWidth; }
		return _IsValidColIdx(col) ? m_colWidth[col] : 0;
	}
	void SetColWidth(int col, int nWidth)
	{
		if (col == -1) { m_SHcolWidth = nWidth; }
		else if (_IsValidColIdx(col)) { m_colWidth[col] = nWidth; }
	}

	int GetSHwidth() { return m_SHcolWidth; }
	void SetSHwidth(int width) { m_SHcolWidth = width; }

	void ResetColWidths()
	{
		m_colWidth = m_defColWidth;
		m_SHcolWidth = m_defSHcolWidth;
	}

	void LoadColWidthsUserSettings(const std::wstring& settingsFilename, const std::wstring& prefix)
	{
		CXeUserSettings settings(settingsFilename);
		std::wstring s = prefix + L".SHwidth";
		m_SHcolWidth = settings.GetU32_or_Val(s, m_defSHcolWidth);
		for (size_t i = 0; i < m_colNames.size(); ++i)
		{
			std::wstring s = prefix + L"." + m_colNames[i];
			m_colWidth[i] = settings.GetU32_or_Val(s, m_defColWidth[i]);
		}
	}

	void SaveColWidthsUserSettings(const std::wstring& settingsFilename, const std::wstring& prefix)
	{
		CXeUserSettings settings(settingsFilename);
		std::wstring s = prefix + L".SHwidth";
		settings.Set(s, m_SHcolWidth);
		for (size_t i = 0; i < m_colNames.size(); ++i)
		{
			std::wstring s = prefix + L"." + m_colNames[i];
			settings.Set(s, m_colWidth[i]);
		}
	}

protected:
	bool _IsValidColIdx(int col) const { return col >= 0 || col < m_colNames.size(); }
};

