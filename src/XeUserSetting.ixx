module;

// This module is child of Xe.UserSettings

#include "XeAssert.h"
#include <concepts>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

export module Xe.UserSetting;

import Xe.StringTools;
import Xe.Helpers;

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif

using json = nlohmann::json;

export enum class SettingType
{
	Invalid,
	Bool,
	I8,
	U8,
	I16,
	U16,
	I32,
	U32,
	I64,
	U64,
	Double,
	String,
	Binary,
	I32x2,		// value stored as 2 x comma separated values (position or size)
	I32x4,		// value stored as 4 x comma separated values (rect)
	Font,			// value stored as string - use get/setString to change value.
	MonospacedFont	// Note - the UI dialog box will supply the "range" value with
					// monospaced fonts found on 'this' system.
};

export std::wstring as_string(SettingType type)
{
	switch (type)
	{
	case SettingType::Bool   : return L"Bool"   ;
	case SettingType::I8     : return L"I8"     ;
	case SettingType::U8     : return L"U8"     ;
	case SettingType::I16    : return L"I16"    ;
	case SettingType::U16    : return L"U16"    ;
	case SettingType::I32    : return L"I32"    ;
	case SettingType::U32    : return L"U32"    ;
	case SettingType::I64    : return L"I64"    ;
	case SettingType::U64    : return L"U64"    ;
	case SettingType::Double : return L"Double" ;
	case SettingType::String : return L"String" ;
	case SettingType::Binary : return L"Binary" ;
	case SettingType::I32x2  : return L"I32x2"  ;
	case SettingType::I32x4  : return L"I32x4"  ;
	case SettingType::Font   : return L"Font"   ;
	case SettingType::MonospacedFont: return L"MonospacedFont"  ;
	}
	XeASSERT(false);
	return L"Invalid";
}

export SettingType from_string(const std::wstring& type)
{
	if      (wcscmp(type.c_str(), L"Bool"           ) == 0) return SettingType::Bool   ;
	else if (wcscmp(type.c_str(), L"I8"             ) == 0) return SettingType::I8     ;
	else if (wcscmp(type.c_str(), L"U8"             ) == 0) return SettingType::U8     ;
	else if (wcscmp(type.c_str(), L"I16"            ) == 0) return SettingType::I16    ;
	else if (wcscmp(type.c_str(), L"U16"            ) == 0) return SettingType::U16    ;
	else if (wcscmp(type.c_str(), L"I32"            ) == 0) return SettingType::I32    ;
	else if (wcscmp(type.c_str(), L"U32"            ) == 0) return SettingType::U32    ;
	else if (wcscmp(type.c_str(), L"I64"            ) == 0) return SettingType::I64    ;
	else if (wcscmp(type.c_str(), L"U64"            ) == 0) return SettingType::U64    ;
	else if (wcscmp(type.c_str(), L"Double"         ) == 0) return SettingType::Double ;
	else if (wcscmp(type.c_str(), L"String"         ) == 0) return SettingType::String ;
	else if (wcscmp(type.c_str(), L"Binary"         ) == 0) return SettingType::Binary ;
	else if (wcscmp(type.c_str(), L"I32x2"          ) == 0) return SettingType::I32x2  ;
	else if (wcscmp(type.c_str(), L"I32x4"          ) == 0) return SettingType::I32x4  ;
	else if (wcscmp(type.c_str(), L"Font"           ) == 0) return SettingType::Font   ;
	else if (wcscmp(type.c_str(), L"MonospacedFont" ) == 0) return SettingType::MonospacedFont;
	XeASSERT(false);
	return SettingType::Invalid;
}

export bool is_valid(SettingType type)
{
	switch (type)
	{
	case SettingType::Bool   :
	case SettingType::I8     :
	case SettingType::U8     :
	case SettingType::I16    :
	case SettingType::U16    :
	case SettingType::I32    :
	case SettingType::U32    :
	case SettingType::I64    :
	case SettingType::U64    :
	case SettingType::Double :
	case SettingType::String :
	case SettingType::Binary :
	case SettingType::I32x2  :
	case SettingType::I32x4  :
	case SettingType::Font   :
	case SettingType::MonospacedFont:
		return true;
	}
	XeASSERT(false);
	return false;
}

export struct I32x2
{
	int32_t a, b;

	I32x2() = default;
	I32x2(int32_t n, int32_t m) : a(n), b(m) { }
	bool operator==(const I32x2& rhs) { return a == rhs.a && b == rhs.b; }
};
export struct I32x4
{
	int32_t a, b, c, d;

	I32x4() = default;
	I32x4(int32_t n, int32_t m, int32_t o, int32_t k) : a(n), b(m), c(o), d(k) { }
	bool operator==(const I32x4& rhs) { return a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d; }
};

export struct CXeUserSetting
{
	SettingType  m_typeEnum;				// Data type for 'this' setting
	std::wstring m_name;					// Unique setting name (not visible in UI)
	std::wstring m_display_name;			// UI control title
	uint32_t	 m_display_position = 0;	// Order in group in UI
	std::wstring m_description;				// Used as tooltip in UI

	std::wstring m_group_name;				// Unique group name (is GroupBox in UI)
	std::wstring m_group_title;				// GroupBox title in UI
	uint32_t	 m_group_order = 0;			// Sort order for groups

	std::wstring m_range;					// comma separated values (only when 'this' is I32 or String)

	std::wstring m_value;					// Value for 'this' setting.

	bool m_value_is_default = false;		// When true 'this' value is the default value for the setting.

	std::wstring m_default_value;			// Default Value for 'this' setting. Set when load from json.

	bool m_is_simple_settings = false;		// Only name, type and value are saved to json


	CXeUserSetting() = default;
	CXeUserSetting(SettingType type, const std::wstring& name, const std::wstring& display_name, 
		uint32_t display_position, const std::wstring& description, const std::wstring& group_name,
		const std::wstring& group_title, uint32_t group_order)
		: m_typeEnum(type), m_name(name), m_display_name(display_name),
		m_display_position(display_position), m_description(description), m_group_name(group_name),
		m_group_title(group_title), m_group_order(group_order), m_value_is_default(true)
	{
	}

	static CXeUserSetting MakeBool(const std::wstring& name, bool value)
	{
		CXeUserSetting set1(SettingType::Bool, name, L"", 0, L"", L"", L"", 0);
		set1.m_value = value ? L"1" : L"0";
		set1.m_is_simple_settings = true;
		return set1;
	}

	bool operator==(const CXeUserSetting& rhs) const
	{
		return m_typeEnum == rhs.m_typeEnum && m_name == rhs.m_name && m_display_name == rhs.m_display_name
			&& m_display_position == rhs.m_display_position && m_description == rhs.m_description
			&& m_range == rhs.m_range && m_value == rhs.m_value && m_group_name == rhs.m_group_name
			&& m_group_title == rhs.m_group_title && m_group_order == rhs.m_group_order
			&& m_value_is_default == rhs.m_value_is_default
			&& m_is_simple_settings == rhs.m_is_simple_settings;
	}

	// Used by sort.
	bool operator<(const CXeUserSetting& rhs) const
	{
		return m_group_order < rhs.m_group_order 
			|| (!(rhs.m_group_order < m_group_order) && m_display_position < rhs.m_display_position);
	}

	inline bool isFontSetting() const
	{
		return m_typeEnum == SettingType::Font || m_typeEnum == SettingType::MonospacedFont;
	}

	inline bool isNumberSetting() const
	{
		return m_typeEnum == SettingType::I8 || m_typeEnum == SettingType::U8
			|| m_typeEnum == SettingType::I16 || m_typeEnum == SettingType::U16
			|| m_typeEnum == SettingType::I32 || m_typeEnum == SettingType::U32
			|| m_typeEnum == SettingType::I64 || m_typeEnum == SettingType::U64
			|| m_typeEnum == SettingType::Double;
	}

#pragma region getters
	bool		getBool() const				{ return _getBool(m_value); }
	uint8_t		getU8() const				{ return _getU8(m_value); }
	int8_t		getI8() const				{ return _getI8(m_value); }
	uint16_t	getU16() const				{ return _getU16(m_value); }
	int16_t		getI16() const				{ return _getI16(m_value); }
	uint32_t	getU32() const				{ return _getU32(m_value); }
	int32_t		getI32() const				{ return _getI32(m_value); }
	uint64_t	getU64() const				{ return _getU64(m_value); }
	int64_t		getI64() const				{ return _getI64(m_value); }
	double		getDouble() const			{ return _getDouble(m_value); }
	std::wstring getString() const			{ return _getString(m_value); }
	// Get binary data into pData, the size of pData is data_len.
	// Return true if successfully parsed the m_value into binary of size data_len.
	bool		getBinary(uint8_t* pData, size_t data_len) const { return _getBinary(m_value, pData, data_len); }
	I32x2		getI32x2() const			{ return _getI32x2(m_value); }
	I32x4		getI32x4() const			{ return _getI32x4(m_value); }

	bool		get_default_Bool() const	{ return _getBool(m_default_value); }
	uint8_t		get_default_U8() const		{ return _getU8(m_default_value); }
	int8_t		get_default_I8() const		{ return _getI8(m_default_value); }
	uint16_t	get_default_U16() const		{ return _getU16(m_default_value); }
	int16_t		get_default_I16() const		{ return _getI16(m_default_value); }
	uint32_t	get_default_U32() const		{ return _getU32(m_default_value); }
	int32_t		get_default_I32() const		{ return _getI32(m_default_value); }
	uint64_t	get_default_U64() const		{ return _getU64(m_default_value); }
	int64_t		get_default_I64() const		{ return _getI64(m_default_value); }
	double		get_default_Double() const	{ return _getDouble(m_default_value); }
	std::wstring get_default_String() const	{ return _getString(m_default_value); }
	// Get binary data into pData, the size of pData is data_len.
	// Return true if successfully parsed the m_value into binary of size data_len.
	bool		get_default_Binary(uint8_t* pData, size_t data_len) const { return _getBinary(m_default_value, pData, data_len); }
	I32x2		get_default_I32x2() const	{ return _getI32x2(m_default_value); }
	I32x4		get_default_I32x4() const	{ return _getI32x4(m_default_value); }

protected:
	bool _getBool(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::Bool) { XeASSERT(false); return false; }
		return xen::stoi(val.c_str()) != 0;
	}
	uint8_t _getU8(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::U8) { XeASSERT(false); return 0; }
		return (uint8_t)xen::stoul(val.c_str());
	}
	int8_t _getI8(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::I8) { XeASSERT(false); return 0; }
		return (int8_t)xen::stoi(val.c_str());
	}
	uint16_t _getU16(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::U16) { XeASSERT(false); return 0; }
		return (uint16_t)xen::stoul(val.c_str());
	}
	int16_t _getI16(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::I16) { XeASSERT(false); return 0; }
		return (int16_t)xen::stoi(val.c_str());
	}
	uint32_t _getU32(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::U32) { XeASSERT(false); return 0; }
		return xen::stoul(val.c_str());
	}
	int32_t _getI32(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::I32) { XeASSERT(false); return 0; }
		return xen::stol(val.c_str());
	}
	uint64_t _getU64(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::U64) { XeASSERT(false); return 0; }
		return xen::stoull(val.c_str());
	}
	int64_t _getI64(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::I64) { XeASSERT(false); return 0; }
		return xen::stoll(val.c_str());
	}
	double _getDouble(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::Double) { XeASSERT(false); return 0; }
		return std::wcstod(val.c_str(), nullptr);
	}
	std::wstring _getString(const std::wstring& val) const
	{
		//XeASSERT(m_typeEnum == SettingType::String || isFontSetting());
		return val;
	}
	// Get binary data into pData, the size of pData is data_len.
	// Return true if successfully parsed the m_value into binary of size data_len.
	bool _getBinary(const std::wstring& val, void* pData, size_t data_len) const
	{
		if (m_typeEnum != SettingType::Binary) { XeASSERT(false); return false; }
		size_t strLen = data_len * 3;
		XeASSERT(strLen == val.size());
		if (strLen != val.size()) { return false; }
		wchar_t h, l;
		uint8_t b;
		uint8_t* pD = (uint8_t*)pData;
		for (size_t i = 0; i < strLen; i += 3)
		{
			h = val[i];
			l = val[i + 1];
			h = (h >= L'0' && h <= L'9') ? h - L'0' : (h >= L'a' && h <= L'f') ? h - L'a' + 10 : L'x';
			l = (l >= L'0' && l <= L'9') ? l - L'0' : (l >= L'a' && l <= L'f') ? l - L'a' + 10 : L'x';
			if (val[i + 2] != L' ' || h == L'x' || l == L'x')
			{
				XeASSERT(false);
				return false;
			}
			b = (uint8_t)((h << 4) + l);
			*pD++ = b;
		}
		return true;
	}
	I32x2 _getI32x2(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::I32x2) { XeASSERT(false); return I32x2(); }
		bool isValid;
		std::vector<int32_t> csv = _numericSplitCSV<int32_t>(isValid, val);
		XeASSERT(csv.size() == 2);
		if (isValid && csv.size() >= 2)
		{
			return I32x2(csv[0], csv[1]);
		}
		return I32x2(-1, -1);
	}
	I32x4 _getI32x4(const std::wstring& val) const
	{
		if (m_typeEnum != SettingType::I32x4) { XeASSERT(false); return I32x4(); }
		bool isValid;
		std::vector<int32_t> csv = _numericSplitCSV<int32_t>(isValid, val);
		XeASSERT(csv.size() == 4);
		if (isValid && csv.size() >= 4)
		{
			return I32x4(csv[0], csv[1], csv[2], csv[3]);
		}
		return I32x4(-1, -1, -1, -1);
	}
#pragma endregion getters

#pragma region setters
public:
	void setBool(bool f)
	{
		XeASSERT(m_typeEnum == SettingType::Bool);
		m_value = f ? L"1" : L"0";
		m_value_is_default = false;
	}
	void setU8(uint8_t u)
	{
		XeASSERT(m_typeEnum == SettingType::U8);
		m_value = std::to_wstring(u);
		m_value_is_default = false;
	}
	void setI8(int8_t i)
	{
		XeASSERT(m_typeEnum == SettingType::I8);
		m_value = std::to_wstring(i);
		m_value_is_default = false;
	}
	void setU16(uint16_t u)
	{
		XeASSERT(m_typeEnum == SettingType::U16);
		m_value = std::to_wstring(u);
		m_value_is_default = false;
	}
	void setI16(int16_t i)
	{
		XeASSERT(m_typeEnum == SettingType::I16);
		m_value = std::to_wstring(i);
		m_value_is_default = false;
	}
	void setU32(uint32_t u)
	{
		XeASSERT(m_typeEnum == SettingType::U32);
		m_value = std::to_wstring(u);
		m_value_is_default = false;
	}
	void setI32(int32_t i)
	{
		XeASSERT(m_typeEnum == SettingType::I32);
		m_value = std::to_wstring(i);
		m_value_is_default = false;
	}
	void setU64(uint64_t u)
	{
		XeASSERT(m_typeEnum == SettingType::U64);
		m_value = std::to_wstring(u);
		m_value_is_default = false;
	}
	void setI64(int64_t i)
	{
		XeASSERT(m_typeEnum == SettingType::I64);
		m_value = std::to_wstring(i);
		m_value_is_default = false;
	}
	void setDouble(double d)
	{
		XeASSERT(m_typeEnum == SettingType::Double);
		std::wostringstream streamObj;
		streamObj << std::setprecision(17) << d;
		m_value = streamObj.str();
		m_value_is_default = false;
	}
	void setString(const std::wstring& s)
	{
		XeASSERT(m_typeEnum == SettingType::String || isFontSetting());
		m_value = s;
		m_value_is_default = false;
	}
	void setBinary(const void* pData, size_t data_len)
	{
#pragma warning(disable:4996)
		XeASSERT(m_typeEnum == SettingType::Binary);
		size_t strLen = data_len * 3 + 1;
		std::unique_ptr<wchar_t[]> buffer(new wchar_t[strLen]);
		wchar_t* pC = buffer.get();
		const uint8_t* pD = (const uint8_t*)pData;
		for (size_t i = 0; i < data_len; i++, pC += 3)
		{
			swprintf(pC, 4, L"%02x ", pD[i]);
		}
		*pC = 0;
		std::wstring str(buffer.get());
		m_value = str;
		m_value_is_default = false;
#pragma warning(default:4996)
	}
	void setI32x2(const I32x2& ix2)
	{
		XeASSERT(m_typeEnum == SettingType::I32x2);
		m_value = std::to_wstring(ix2.a) + L"," + std::to_wstring(ix2.b);
		m_value_is_default = false;
	}
	void setI32x4(const I32x4& ix4)
	{
		XeASSERT(m_typeEnum == SettingType::I32x4);
		m_value = std::to_wstring(ix4.a) + L"," + std::to_wstring(ix4.b)
			+ L"," + std::to_wstring(ix4.c) + L"," + std::to_wstring(ix4.d);
		m_value_is_default = false;
	}
#pragma endregion setters

	bool hasRange() const { return m_range.size() > 0; }
	std::vector<int8_t>      getRangeI8(bool &isValid)     const { return _numericSplitCSV<int8_t>(isValid, m_range); }
	std::vector<uint8_t>     getRangeU8(bool &isValid)     const { return _numericSplitCSV<uint8_t>(isValid, m_range); }
	std::vector<int16_t>     getRangeI16(bool &isValid)    const { return _numericSplitCSV<int16_t>(isValid, m_range); }
	std::vector<uint16_t>    getRangeU16(bool &isValid)    const { return _numericSplitCSV<uint16_t>(isValid, m_range); }
	std::vector<int32_t>     getRangeI32(bool &isValid)    const { return _numericSplitCSV<int32_t>(isValid, m_range); }
	std::vector<uint32_t>    getRangeU32(bool &isValid)    const { return _numericSplitCSV<uint32_t>(isValid, m_range); }
	std::vector<int64_t>     getRangeI64(bool &isValid)    const { return _numericSplitCSV<int64_t>(isValid, m_range); }
	std::vector<uint64_t>    getRangeU64(bool &isValid)    const { return _numericSplitCSV<uint64_t>(isValid, m_range); }
	std::vector<double>      getRangeDouble(bool &isValid) const { return _numericSplitCSV<double>(isValid, m_range); }
	std::vector<std::wstring> getRangeStrings()            const { return _StringSplitCSV(m_range); }

	// Convert val into numeric value, if range is present (only lowest to highest range allowed
	// i.e. two values in the range) the val number is compared against the allowed range,
	// if the val number is between the range then it is stored and true returned, if outside of 
	// the range then false is returned (and not stored).
	// If the value type of 'this' is string then val is simply stored and true returned.
	// If no range provided in 'this' and value type is numeric then val is stored and true returned.
	bool ValidateAndStoreValue(const std::wstring& val)
	{
		bool isValid = true;
		switch (m_typeEnum)
		{
		case SettingType::I8: {
			int8_t v = _convAndValidate<int8_t>(isValid, val, m_range);
			if (isValid) { setI8(v); }
			} break;
		case SettingType::U8: {
			uint8_t v = _convAndValidate<uint8_t>(isValid, val, m_range);
			if (isValid) { setU8(v); }
			} break;
		case SettingType::I16: {
			int16_t v = _convAndValidate<int16_t>(isValid, val, m_range);
			if (isValid) { setI16(v); }
			} break;
		case SettingType::U16: {
			uint16_t v = _convAndValidate<uint16_t>(isValid, val, m_range);
			if (isValid) { setU16(v); }
			} break;
		case SettingType::I32: {
			int32_t v = _convAndValidate<int32_t>(isValid, val, m_range);
			if (isValid) { setI32(v); }
			} break;
		case SettingType::U32: {
			uint32_t v = _convAndValidate<uint32_t>(isValid, val, m_range);
			if (isValid) { setU32(v); }
			} break;
		case SettingType::I64: {
			int64_t v = _convAndValidate<int64_t>(isValid, val, m_range);
			if (isValid) { setI64(v); }
			} break;
		case SettingType::U64: {
			uint64_t v = _convAndValidate<uint64_t>(isValid, val, m_range);
			if (isValid) { setU64(v); }
			} break;
		case SettingType::Double: {
			double v = _convAndValidate<double>(isValid, val, m_range);
			if (isValid) { setDouble(v); }
			} break;
		case SettingType::String:
			setString(val);
		default:
			XeASSERT(false);	// Invalid value type for this function.
			return false;
		}
		return isValid;
	}

	static std::vector<std::wstring> _StringSplitCSV(const std::wstring& v)
	{
		std::vector<std::wstring> csv;
		std::wstring split;
		std::wistringstream ss(v);
		while (std::getline(ss, split, L','))
		{
			xet::trim(split);
			csv.push_back(split.c_str());
		}
		return csv;
	}

	template <typename T>
	requires std::is_arithmetic_v<T>
	static std::vector<T> _numericSplitCSV(bool& isValid, const std::wstring& csv_str)
	{
		std::vector<T> csv;
		for (const std::wstring& s : _StringSplitCSV(csv_str))
		{
			T v = _convert<T>(isValid, s);
			if (isValid)
			{
				csv.push_back(v);
			}
			else
			{
				XeASSERT(false);	// A value was outside range of type
				break;
			}
		}
		return csv;
	}

	template <typename T>
	requires std::is_arithmetic_v<T>
	static T _convAndValidate(bool& isValid, const std::wstring& val, const std::wstring& range)
	{
		T v = _convert<T>(isValid, val);
		if (isValid && range.size() > 0)
		{
			std::vector<T> r = _numericSplitCSV<T>(isValid, range);
			XeASSERT(r.size() == 2);	// Range can only be lowest, highest.
			if (r.size() != 2 || v < r[0] || v > r[1])
			{
				isValid = false;
			}
		}
		return v;
	}

	template <typename T>
	requires std::is_arithmetic_v<T>
	static T _convert(bool& isValid, const std::wstring& val)
	{
		if constexpr (std::is_integral_v<T>)
		{
			// Note - limits checking for 64 bit doesn't really do anything - but the stoll/stoull
			// functions throw an exception if val is out of range (for 64 bit).
			if constexpr (std::is_signed_v<T>)
			{
				try
				{
					int64_t i = std::stoll(val.c_str());
					isValid = i >= std::numeric_limits<T>::min() && i <= std::numeric_limits<T>::max();
					return (T)i;
				}
				catch (...)
				{
					isValid = false;
					return T();
				}
			}
			else // is unsigned
			{
				try
				{
					uint64_t u = std::stoull(val.c_str());
					isValid = u >= std::numeric_limits<T>::min() && u <= std::numeric_limits<T>::max();
					return (T)u;
				}
				catch (...)
				{
					isValid = false;
					return T();
				}
			}
		}
		else if constexpr (std::is_same_v<double, T>)
		{
			try
			{
				isValid = true;	// Can't check for limits
				return std::stof(val.c_str());
			}
			catch (...)
			{
				isValid = false;
				return T();
			}
		}
		else
		{
			//static_assert(false, "Type not supported here");
			isValid = false;
			return T();
		}
	}
};

export void to_json(json& j, const CXeUserSetting& item)
{
	if (item.m_is_simple_settings)
	{
		j = json{ {"name", xet::toUTF8(item.m_name)},
			{"type", xet::toUTF8(as_string(item.m_typeEnum))},
			{"value", xet::toUTF8(item.m_value)} };
	}
	else
	{
		j = json{ {"description", xet::toUTF8(item.m_description)},
			{"name", xet::toUTF8(item.m_name)}, 
			{"type", xet::toUTF8(as_string(item.m_typeEnum))},
			{"value", xet::toUTF8(item.m_value)},
			{"value_is_default", item.m_value_is_default},
			{"display_name", xet::toUTF8(item.m_display_name)},
			{"display_position", item.m_display_position},
			{"range", xet::toUTF8(item.m_range)},
			{"group_name", xet::toUTF8(item.m_group_name)},
			{"group_title", xet::toUTF8(item.m_group_title)},
			{"group_order", item.m_group_order} };
	}
}

export void from_json(const json& j, CXeUserSetting& item)
{
	std::string s;
	if (j.find("description") != j.end())
	{
		j.at("description").get_to(s);
		item.m_description = xet::fromUTF8toWStr(s);
	}
	j.at("name").get_to(s);
	item.m_name = xet::fromUTF8toWStr(s);
	std::string type;
	j.at("type").get_to(type);
	item.m_typeEnum = from_string(xet::fromUTF8toWStr(type));
	j.at("value").get_to(s);
	item.m_value = xet::fromUTF8toWStr(s);
	if (j.find("value_is_default") != j.end()) { j.at("value_is_default").get_to(item.m_value_is_default); }
	if (j.find("display_name") != j.end())
	{
		j.at("display_name").get_to(s);
		item.m_display_name = xet::fromUTF8toWStr(s);
	}
	if (j.find("display_position") != j.end()) { j.at("display_position").get_to(item.m_display_position); }
	if (j.find("range") != j.end())
	{
		j.at("range").get_to(s);
		item.m_range = xet::fromUTF8toWStr(s);
	}
	if (j.find("group_name") != j.end())
	{
		j.at("group_name").get_to(s);
		item.m_group_name = xet::fromUTF8toWStr(s);
	}
	if (j.find("group_title") != j.end())
	{
		j.at("group_title").get_to(s);
		item.m_group_title = xet::fromUTF8toWStr(s);
	}
	if (j.find("group_order") != j.end()) { j.at("group_order").get_to(item.m_group_order); }
	if (item.m_value_is_default)
	{
		item.m_default_value = item.m_value;	// Save the default value for this setting.
	}
}

