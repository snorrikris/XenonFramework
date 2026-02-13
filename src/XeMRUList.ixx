module;

#include "os_minimal.h"
#include <string>
#include <list>
#include <tchar.h>
#include "nlohmann/json.hpp"

export module Xe.MRUList;

import Xe.FileHelpersJson;
import Xe.StringTools;

using json = nlohmann::json;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export class CMRUList
{
	size_t m_maxItems = 30;	// ID_MRU_ITEM1...30 limits the max number

	std::list<std::wstring> m_items;

	std::wstring m_pathname;
	std::wstring m_original;

public:
	void SetMaxSize(size_t num_items) { m_maxItems = num_items; }

	void LoadFromFile(const std::wstring& pathname)
	{
		m_pathname = pathname;
		m_items.clear();
		std::list<std::string> items;
		LoadJsonDataFromFile(m_pathname, [&](json& j)
			{ j.at("OpenFileMRUlist").get_to(items); });
		for (const std::string& item : items)
		{
			m_items.push_back(xet::fromUTF8toWStr(item));
		}
	}

	void Add(const std::wstring& item)
	{
		_Add(item);
		_Save();
	}

	void Add(const std::vector<std::wstring>& items)
	{
		for (auto it = items.rbegin(); it != items.rend(); ++it)
		{
			_Add(*it);
		}
		_Save();
	}

	std::vector<std::wstring> GetMRU_MenuItems() const
	{
		std::vector<std::wstring> list;
		std::wstring item;
		auto it = m_items.begin();
		for (int iMRU = 0; iMRU < m_items.size(); iMRU++)
		{
			item = *it++;
			// double up any '&' characters so they are not underlined
			LPCTSTR lpszSrc = item.c_str();
			std::wstring strTemp(item.size() * 2, L'\0');
			LPTSTR lpszDest = strTemp.data();
			while (*lpszSrc != 0)
			{
				if (*lpszSrc == '&')
					*lpszDest++ = '&';
				if (_istlead(*lpszSrc))
					*lpszDest++ = *lpszSrc++;
				*lpszDest++ = *lpszSrc++;
			}
			*lpszDest++ = 0;
			size_t len = lpszDest - strTemp.data();
			strTemp.resize(len);

			// insert mnemonic + the file name
			int nItem = iMRU + 1;

			// number &1 thru &9, then 1&0, then 11 thru ...
			std::wstring prefix;
			if (nItem > 10)
			{
				prefix = std::to_wstring(nItem) + L" ";
			}
			else if (nItem == 10)
			{
				prefix = L"1&0";
			}
			else
			{
				prefix = L"&" + std::to_wstring(nItem) + L" ";
			}

			std::wstring ts = prefix + strTemp;
			list.push_back(ts);
		}
		return list;
	}

	std::wstring GetItemAt(size_t idx)
	{
		if (idx < m_items.size())
		{
			auto it = m_items.begin();
			std::advance(it, idx);
			return *it;
		}
		return std::wstring();
	}

protected:
	void _Add(const std::wstring& item)
	{
		for (auto it = m_items.begin(); it != m_items.end(); )
		{
			if (*it == item)
			{
				it = m_items.erase(it);
			}
			else
			{
				++it;
			}
		}
		m_items.push_front(item);
		while (m_items.size() > m_maxItems)
		{
			m_items.pop_back();
		}
	}

	void _Save()
	{
		if (m_pathname.size())
		{
			// Save to file
			std::list<std::string> items;
			for (const std::wstring& item : m_items)
			{
				items.push_back(xet::toUTF8(item));
			}
			SaveJsonDataToFile(m_pathname, [items](json& j)
				{ j = json{ {"OpenFileMRUlist", items} }; });
		}
	}
};
