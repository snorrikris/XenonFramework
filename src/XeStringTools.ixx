module;

#include "os_minimal.h"
#include <vector>
#include <string>
#include <algorithm> 
#include <cctype>
#include <cwctype>
#include <locale>
#include <cwchar>

export module Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export class xet
{
public:
    // Source (for string trim): https://stackoverflow.com/questions/216823/how-to-trim-an-stdstring

    // trim from start (in place)
    static inline void ltrim(std::string& s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));
    }

    // trim from end (in place)
    static inline void rtrim(std::string& s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    }

    // trim from both ends (in place)
    static inline void trim(std::string& s)
    {
        rtrim(s);
        ltrim(s);
    }

    // trim from start (copying)
    static inline std::string ltrim_copy(std::string s)
    {
        ltrim(s);
        return s;
    }

    // trim from end (copying)
    static inline std::string rtrim_copy(std::string s)
    {
        rtrim(s);
        return s;
    }

    // trim from both ends (copying)
    static inline std::string trim_copy(std::string s)
    {
        trim(s);
        return s;
    }

    static std::string to_lower(const std::string& src)
    {
        std::string dest(src);
        std::transform(dest.begin(), dest.end(), dest.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });
        return dest;
    }

    static std::string to_upper(const std::string& src)
    {
        std::string dest(src);
        std::transform(dest.begin(), dest.end(), dest.begin(),
            [](unsigned char c) { return (char)std::toupper(c); });
        return dest;
    }

    static void find_replace(std::string& s, char fnd, char rplc)
    {
        std::replace(s.begin(), s.end(), fnd, rplc);
    }

    // trim from start (in place)
    static inline void ltrim(std::wstring& s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            [](wchar_t ch) { return !std::iswspace(ch); }));
    }

    // trim from end (in place)
    static inline void rtrim(std::wstring& s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(),
            [](wchar_t ch) { return !std::iswspace(ch); }).base(), s.end());
    }

    // trim from both ends (in place)
    static inline void trim(std::wstring& s)
    {
        rtrim(s);
        ltrim(s);
    }

    // trim from start (copying)
    static inline std::wstring ltrim_copy(std::wstring s)
    {
        ltrim(s);
        return s;
    }

    // trim from end (copying)
    static inline std::wstring rtrim_copy(std::wstring s)
    {
        rtrim(s);
        return s;
    }

    // trim from both ends (copying)
    static inline std::wstring trim_copy(std::wstring s)
    {
        trim(s);
        return s;
    }

    static std::wstring to_lower(const std::wstring& src)
    {
        std::wstring dest(src);
        std::transform(dest.begin(), dest.end(), dest.begin(),
            [](wchar_t c) { return std::towlower(c); });
        return dest;
    }

    static std::wstring to_upper(const std::wstring& src)
    {
        std::wstring dest(src);
        std::transform(dest.begin(), dest.end(), dest.begin(),
            [](wchar_t c) { return std::towupper(c); });
        return dest;
    }

    static void find_replace(std::wstring& s, wchar_t fnd, wchar_t rplc)
    {
        std::replace(s.begin(), s.end(), fnd, rplc);
    }

    static uint32_t GetTrimmedTrailingCRLF_length(const char* pStr, size_t str_length)
    {
        if (str_length == 0) { return 0; }
        const char* pEnd = pStr + str_length - 1;
        while (pEnd > pStr)
        {
            if (!(*pEnd == 10 || *pEnd == 13))
            {
                break;
            }
            --pEnd;
        }
        return (uint32_t)(pEnd - pStr) + 1;
    }

    static uint32_t GetTrimmedTrailingCRLF_length(const wchar_t* pStr, size_t str_length)
    {
        if (str_length == 0) { return 0; }
        const wchar_t* pEnd = pStr + str_length - 1;
        while (pEnd > pStr)
        {
            if (!(*pEnd == 10 || *pEnd == 13))
            {
                break;
            }
            --pEnd;
        }
        return (uint32_t)(pEnd - pStr) + 1;
    }

    static void trim_trailing_crlf(std::string& s)
    {
        uint32_t len = GetTrimmedTrailingCRLF_length(s.c_str(), s.size());
        if (len < (uint32_t)s.size())
        {
            s.resize(len);
        }
    }

    static void trim_trailing_crlf(std::wstring& s)
    {
        uint32_t len = GetTrimmedTrailingCRLF_length(s.c_str(), s.size());
        if (len < (uint32_t)s.size())
        {
            s.resize(len);
        }
    }

    static std::string toUTF8(const std::string& str)
    {
        int size = ::MultiByteToWideChar(CP_ACP, MB_COMPOSITE, str.c_str(), (int)str.length(),
            nullptr, 0);
        std::wstring utf16_str(size, L'\0');
        ::MultiByteToWideChar(CP_ACP, MB_COMPOSITE, str.c_str(), (int)str.length(),
            &utf16_str[0], size);

        int utf8_size = ::WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(), (int)utf16_str.length(),
            nullptr, 0, nullptr, nullptr);
        std::string utf8_str(utf8_size, '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(), (int)utf16_str.length(),
            &utf8_str[0], utf8_size, nullptr, nullptr);
        return utf8_str;
    }

    static std::string toUTF8(const std::wstring& wstr)
    {
        int utf8_size = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(),
            nullptr, 0, nullptr, nullptr);
        std::string utf8_str(utf8_size, '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(),
            &utf8_str[0], utf8_size, nullptr, nullptr);
        return utf8_str;
    }

    static std::string fromUTF8(const std::string& utf8_str)
    {
        int size = ::MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, utf8_str.c_str(),
            (int)utf8_str.length(), nullptr, 0);
        std::wstring utf16_str(size, L'\0');
        ::MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, utf8_str.c_str(), (int)utf8_str.length(),
            &utf16_str[0], size);

        int acp_size = ::WideCharToMultiByte(CP_ACP, 0, utf16_str.c_str(), (int)utf16_str.length(),
            nullptr, 0, nullptr, nullptr);
        std::string acp_str(acp_size, '\0');
        ::WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, utf16_str.c_str(), (int)utf16_str.length(),
            &acp_str[0], acp_size, nullptr, nullptr);
        return acp_str;
    }

    static std::wstring fromUTF8toWStr(const std::string& string)
    {
        if (string.empty())
        {
            return std::wstring();
        }

        int len = ::MultiByteToWideChar(CP_UTF8, 0, string.c_str(), (int)string.size(), nullptr, 0);
        if (len <= 0)
        {
            return std::wstring();
        }

        std::wstring result(len, 0);
        ::MultiByteToWideChar(CP_UTF8, 0, string.c_str(), (int)string.size(), result.data(), len);
        return result;
    }

    static std::string to_astr(const std::wstring& wstr)
    {
        char default_char = '_';	// Default char (in case UNICODE char doesn't translate to system code page).
        int buf_len = ::WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wstr.c_str(), (DWORD)wstr.size(),
            nullptr, 0, &default_char, NULL);
        std::string str(buf_len, '\0');
        int str_len = ::WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wstr.c_str(), (DWORD)wstr.size(),
            str.data(), (DWORD)buf_len, &default_char, NULL);
        if (str_len == 0)
        {
            return std::string();
        }
        str.resize(str_len);
        return str;
    }

    static std::wstring to_wstr(const std::string& str)
    {
        int len = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), (DWORD)str.size(), nullptr, 0);
        std::wstring wstr(len, L'\0');
        int str_len = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), (DWORD)str.size(), wstr.data(), (DWORD)len);
        if (str_len == 0)
        {
            return std::wstring();
        }
        wstr.resize(str_len);
        return wstr;
    }

    static std::wstring to_wstr(const char* pStr, size_t length)
    {
        int len = ::MultiByteToWideChar(CP_ACP, 0, pStr, (DWORD)length, nullptr, 0);
        std::wstring wstr(len, L'\0');
        int str_len = ::MultiByteToWideChar(CP_ACP, 0, pStr, (DWORD)length, wstr.data(), (DWORD)len);
        if (str_len == 0)
        {
            return std::wstring();
        }
        wstr.resize(str_len);
        return wstr;
    }

    static int compare_no_case(const std::wstring& strA, const std::wstring& strB)
    {
        std::wstring strAl = xet::to_lower(strA);
        std::wstring strBl = xet::to_lower(strB);
        return strAl.compare(strBl);
    }
    static int compare_no_case(const std::string& strA, const std::string& strB)
    {
        return std::lexicographical_compare(
            std::begin(strA), std::end(strA),
            std::begin(strB), std::end(strB),
            [](const char& char1, const char& char2)
            {
                return std::tolower(char1) < std::tolower(char2);
            }
        );
    }

    static bool is_equal_no_case(const std::wstring& strA, const std::wstring& strB)
    {
        if (strA.size() != strB.size())
        {
            return false;
        }
        auto it1 = strA.cbegin();
        auto it2 = strB.cbegin();
        for (; it1 != strA.cend() || it2 != strB.cend(); ++it1, ++it2)
        {
            if (std::towlower(*it1) != std::towlower(*it2))
            {
                return false;
            }
        }
        return true;
    }

    // Return true if strA contains strB (case insensetive compare)
    static bool is_contains_no_case(const std::wstring& strA, const std::wstring& strB)
    {
        size_t a_len = strA.size(), b_len = strB.size();
        for (auto outerIt = strA.cbegin(); outerIt != strA.cend(); ++outerIt, --a_len)
        {
            if (a_len < b_len)
            {
                return false;
            }

            auto substrIt = strB.cbegin();
            for (auto innerIt = outerIt; innerIt != strA.cend() && substrIt != strB.cend(); ++innerIt, ++substrIt)
            {
                if (std::towlower(*innerIt) != std::towlower(*substrIt))
                {
                    break;
                }
            }
            if (substrIt == strB.cend())	// Substring match?
            {
                return true;
            }
        }
        return false;
    }

    static bool is_starts_with_no_case(const std::wstring& strA, const std::wstring& strB)
    {
        if (strA.size() < strB.size())
        {
            return false;
        }
        for (auto itA = strA.cbegin(), itB = strB.cbegin(); itA != strA.cend() && itB != strB.cend(); ++itA, ++itB)
        {
            if (std::towlower(*itA) != std::towlower(*itB))
            {
                return false;
            }
        }
        return true;
    }

    static bool is_ends_with_no_case(const std::wstring& strA, const std::wstring& strB)
    {
        size_t a_len = strA.size(), b_len = strB.size();
        if (a_len < b_len)
        {
            return false;
        }
        auto itA = strA.cbegin() + (a_len - b_len);
        for (auto itB = strB.cbegin(); itA != strA.cend() && itB != strB.cend(); ++itA, ++itB)
        {
            if (std::towlower(*itA) != std::towlower(*itB))
            {
                return false;
            }
        }
        return true;
    }
};

export std::wstring LoadResourceString(uint32_t uID)
{
	const wchar_t* pStr = nullptr;
	size_t str_len = ::LoadStringW(GetModuleHandle(0), uID, reinterpret_cast<LPWSTR>(&pStr), 0);
	if (str_len == 0 || pStr == nullptr)
	{
		return std::wstring();
	}
	return std::wstring { pStr, str_len };
}

bool get_hex_digit(char c, uint32_t& dw)
{
	if (c >= '0' && c <= '9')
		dw = (uint32_t)(c - 0x30);
	else if (c >= 'a' && c <= 'f')
		dw = (uint32_t)((c - 'a') + 10);
	else if (c >= 'A' && c <= 'F')
		dw = (uint32_t)((c - 'A') + 10);
	else
		return false;
	return true;
}

// Read HEX value from string.
// szHex = '0xA2345678' or '0XF2345678' or 'ABCD1234'.
// Note - hex digit count is variable from 1 to 8.
export uint32_t read_hex(const char* szHex)
{
	uint32_t dwResult = 0, dwDigit;
	if (szHex[0] == '0' && (szHex[1] == 'x' || szHex[1] == 'X'))
	{
		szHex += 2;
	}
	else if (szHex[0] == '#')
	{
		szHex++;
	}
	while (*szHex && get_hex_digit(*szHex, dwDigit))
	{
		dwResult <<= 4;
		dwResult += dwDigit;
		szHex++;
	}
	return dwResult;
}

export uint32_t read_hex_RGB(const char* szHex)
{
	uint32_t dw = read_hex(szHex);
	uint8_t r, g, b;
	r = (uint8_t)(dw >> 16);
	g = (dw & 0xFF00) >> 8;
	b = dw & 0xFF;
	uint32_t rgb = (b << 16) + (g << 8) + r;
	return rgb;
}

export std::string get_RGB_as_string(uint32_t rgb)
{
	uint8_t r, g, b;
	b = (uint8_t)(rgb >> 16);
	g = (rgb & 0xFF00) >> 8;
	r = rgb & 0xFF;
	constexpr size_t buf_len = 20;
	char buffer[buf_len];
	sprintf_s(buffer, buf_len, "#%0.2X%0.2X%0.2X", r, g, b);
	return std::string(buffer);
}

export int wformat(wchar_t* buffer, size_t buf_len, const wchar_t* fmt, ...)
{
	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	int result = std::vswprintf(buffer, buf_len, fmt, arg_ptr);
	va_end(arg_ptr);
	return result;
}

export std::wstring get_RGB_as_wstring(uint8_t r, uint8_t g, uint8_t b)
{
	constexpr size_t buf_len = 20;
	wchar_t buffer[buf_len];
	wformat(buffer, buf_len, L"#%0.2X%0.2X%0.2X", r, g, b);
	return std::wstring(buffer);
}

export std::wstring get_RGB_as_wstring(uint32_t rgb)
{
	uint8_t r, g, b;
	b = (uint8_t)(rgb >> 16);
	g = (rgb & 0xFF00) >> 8;
	r = rgb & 0xFF;
	return get_RGB_as_wstring(r, g, b);
}

// Get a list of files that was returned from a call to CFileDialog.
// The buffer contains one or more zero terminated strings.
// If user selected only one file the buffer contains only one string - full pathname to file.
// If user selected more than one file - the first string in the buffer is the path (no \),
// the remaining strings are the filenames (no path).
export std::vector<std::wstring> ParseOPENFILENAMEstrings(const wchar_t* pBuffer, size_t buf_len)
{
	const wchar_t* pBufEnd = pBuffer + buf_len;
	const wchar_t* p = pBuffer;
	std::vector<std::wstring> files;
	do
	{
		files.push_back(std::wstring(p));
		while ((p < pBufEnd) && (*p)) { ++p; }	// Find next zero
		++p;	// point to next string
	} while (p < pBufEnd && *p != 0);
	// one (or zero) files - strings[0] has full pathname.
	if (files.size() > 1)
	{
		// When more than one file selected by user - the first string is the path, the rest are filenames.
		std::wstring path = files[0] + L"\\";
		files.erase(files.begin());
		for (std::wstring& file : files)
		{
			file = path + file;
		}
	}
	return files;
}

export std::vector<std::wstring> strings_to_wstr(const std::vector<std::string>& strings)
{
	std::vector<std::wstring> wstrings;
	for (const std::string& s : strings)
	{
		wstrings.push_back(xet::to_wstr(s));
	}
	return wstrings;
}

export std::vector<std::string> split(const std::string& s, const std::string& delimiter)
{
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
	{
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}

export std::vector<std::wstring> splitW(const std::wstring& s, const std::wstring& delimiter)
{
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::wstring token;
	std::vector<std::wstring> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
	{
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}

// Split string on LF. Returns positions array. Each element is position in string of line beginning.
// If string contains no LF - an array of size 1 is returned, element [0] = 0.
// Optionally trailing empty lines are trimmed from returned array.
export std::vector<uint32_t> split_lf(const std::string& s, bool isTrimTrailingEmptyLines)
{
	size_t pos_start = 0, pos_end;
	std::vector<uint32_t> res;

	while ((pos_end = s.find('\n', pos_start)) != std::string::npos)
	{
		res.push_back((uint32_t)pos_start);
		pos_start = pos_end + 1;
	}

	if (pos_start < s.size())
	{
		res.push_back((uint32_t)pos_start);
	}

	if (isTrimTrailingEmptyLines)
	{
		while (res.size() > 1)
		{
			uint32_t pos = res.back();
			if (s[pos] != '\n')
			{
				break;
			}
			res.resize(res.size() - 1);
		}
	}
	return res;
}
export std::vector<uint32_t> split_lf(const std::wstring& s, bool isTrimTrailingEmptyLines)
{
	size_t pos_start = 0, pos_end;
	std::vector<uint32_t> res;

	while ((pos_end = s.find(L'\n', pos_start)) != std::string::npos)
	{
		res.push_back((uint32_t)pos_start);
		pos_start = pos_end + 1;
	}

	if (pos_start < s.size())
	{
		res.push_back((uint32_t)pos_start);
	}

	if (isTrimTrailingEmptyLines)
	{
		while (res.size() > 1)
		{
			uint32_t pos = res.back();
			if (s[pos] != L'\n')
			{
				break;
			}
			res.resize(res.size() - 1);
		}
	}
	return res;
}

/// <summary>
/// Remove '&' from string, return position of '&' or -1 if not found.
/// </summary>
/// <param name="s"></param>
/// <returns></returns>
export int PrepareMenuTextForDraw(std::wstring& s)
{
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == L'&')
		{
			size_t j = i + 1;
			if (j < s.size() && s[j] == L'&')
			{
				i = j;
				continue;
			}
			s.erase(i, 1);
			return (int)i;
		}
	}
	return -1;
}

export template <class TS, class SV>
	requires (std::same_as<TS, std::string> && std::same_as<SV, std::string_view>)
		|| (std::same_as<TS, std::wstring> && std::same_as<SV, std::wstring_view>)
class CXeDistinctStrings
{
protected:
	std::vector<TS> m_distinct_strings;

public:
	void Clear() { m_distinct_strings.clear(); }

	void AddString(const TS& s) { AddString(SV(s)); }
	void AddString(const SV& s)
	{
		// Insert string into ordered list. Keep it ordered at each insertion.
		auto it = std::lower_bound(m_distinct_strings.begin(), m_distinct_strings.end(), s,
			[](const TS& lhs, const SV& _s)
			{ return lhs < _s; });
		if (it != m_distinct_strings.end())
		{
			if (*it == s)
			{
				return;
			}
		}
		m_distinct_strings.insert(it, TS(s));
	}

	const std::vector<TS>& GetAllStringsConst() const { return m_distinct_strings; }
};

