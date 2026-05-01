module;

#include "os_minimal.h"
#include <string>

export module Xe.FileTimeX;

/* RESEARCH materals:
https://www.codeproject.com/Articles/144159/Time-Format-Conversion-Made-Easy
https://stackoverflow.com/questions/11081064/file-time-structure-in-vc/11081237#11081237
*/

// Make debug string macro
#ifdef _DEBUG
#define MKDBGSTR MakeDbgStr()
#else
#define MKDBGSTR
#endif
#ifdef _DEBUG
#define MKSPANDBGSTR MakeDbgStr()
#else
#define MKSPANDBGSTR
#endif

export constexpr UINT64 FT_MILLISECOND = ((UINT64)10000);
export constexpr UINT64 FT_SECOND = (1000 * FT_MILLISECOND);
export constexpr UINT64 FT_MINUTE = (60 * FT_SECOND);
export constexpr UINT64 FT_HOUR = (60 * FT_MINUTE);
export constexpr UINT64 FT_DAY = (24 * FT_HOUR);

export enum class DATEFMT
{
	ISOplus_mS,
	YMD_HMS,
	YMD_HMS_forBMfilename,
	YMD_HMSmS_forFilename,
	DMY_HMS,
	HMS,
	DMY,
	YMD
};

export enum class SPANFMT
{
	HMSmS,
	InMilliseconds
};

export enum class EROUNDTO
{
	NEAREST,
	UP,
	DOWN
};

#pragma pack(push)
#pragma pack(4)
export struct FILETIMESPAN
{
	INT64 m_n64Span;

	FILETIMESPAN()
	{
		m_n64Span = 0;
		MKSPANDBGSTR;
	}

	FILETIMESPAN(INT64 n64span)
	{
		m_n64Span = n64span;
		MKSPANDBGSTR;
	}

	FILETIMESPAN(const FILETIMESPAN& source)
	{
		m_n64Span = source.m_n64Span;
		MKSPANDBGSTR;
	}

	bool operator==(const FILETIMESPAN& ftB) const
	{
		return m_n64Span == ftB.m_n64Span;
	}

	bool operator!=(const FILETIMESPAN& ftB) const
	{
		return m_n64Span != ftB.m_n64Span;
	}

	bool operator>(const FILETIMESPAN& ftB) const
	{
		return m_n64Span > ftB.m_n64Span;
	}

	bool operator>=(const FILETIMESPAN& ftB) const
	{
		return m_n64Span >= ftB.m_n64Span;
	}

	bool operator<(const FILETIMESPAN& ftB) const
	{
		return m_n64Span < ftB.m_n64Span;
	}

	bool operator<=(const FILETIMESPAN& ftB) const
	{
		return m_n64Span <= ftB.m_n64Span;
	}

	FILETIMESPAN& operator=(const FILETIMESPAN& ftRHS)
	{
		m_n64Span = ftRHS.m_n64Span;
		MKSPANDBGSTR;
		return *this;
	}

	FILETIMESPAN operator+(FILETIMESPAN span) const
	{
		return FILETIMESPAN(m_n64Span + span.m_n64Span);
	}

	FILETIMESPAN operator-(FILETIMESPAN span) const
	{
		return FILETIMESPAN(m_n64Span - span.m_n64Span);
	}

	FILETIMESPAN& operator+=(FILETIMESPAN span)
	{
		m_n64Span += span.m_n64Span;
		MKSPANDBGSTR;
		return *this;
	}

	FILETIMESPAN& operator-=(FILETIMESPAN span)
	{
		m_n64Span -= span.m_n64Span;
		MKSPANDBGSTR;
		return *this;
	}

	FILETIMESPAN operator/(INT64 div) const
	{
		return FILETIMESPAN(m_n64Span / div);
	}

	FILETIMESPAN operator%(INT64 div) const
	{
		return FILETIMESPAN(m_n64Span % div);
	}

	FILETIMESPAN operator*(INT64 mul) const
	{
		return FILETIMESPAN(m_n64Span * mul);
	}

	FILETIMESPAN& operator/=(INT64 div)
	{
		m_n64Span /= div;
		MKSPANDBGSTR;
		return *this;
	}

	FILETIMESPAN& operator%=(INT64 div)
	{
		m_n64Span %= div;
		MKSPANDBGSTR;
		return *this;
	}

	FILETIMESPAN& operator*=(INT64 mul)
	{
		m_n64Span *= mul;
		MKSPANDBGSTR;
		return *this;
	}

	FILETIMESPAN operator-() const
	{
		return FILETIMESPAN(-m_n64Span);
	}

	FILETIMESPAN abs() const
	{
		if (m_n64Span < 0)
		{
			return FILETIMESPAN(-m_n64Span);
		}
		return FILETIMESPAN(m_n64Span);
	}

	double GetTotalDays() const
	{
		return (double)m_n64Span / FT_DAY;
	}

	double GetTotalHours() const
	{
		return (double)m_n64Span / FT_HOUR;
	}

	double GetTotalMinutes() const
	{
		return (double)m_n64Span / FT_MINUTE;
	}

	double GetTotalSeconds() const
	{
		return (double)m_n64Span / FT_SECOND;
	}

	double GetTotalMilliseconds() const
	{
		return (double)m_n64Span / FT_MILLISECOND;
	}

	// component days in span
	INT64 GetDays() const
	{
		INT64 n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS;
		GetComponents(n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS);
		return n_Sign * n_Days;
	}

	// component hours in span (-23 to 23)
	int GetHours() const
	{
		INT64 n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS;
		GetComponents(n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS);
		return (int)(n_Sign * n_HH);
	}

	// component minutes in span (-59 to 59)
	int GetMinutes() const
	{
		INT64 n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS;
		GetComponents(n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS);
		return (int)(n_Sign * n_MM);
	}

	// component seconds in span (-59 to 59)
	int GetSeconds() const
	{
		INT64 n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS;
		GetComponents(n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS);
		return (int)(n_Sign * n_SS);
	}

	// component milli seconds in span (-999 to 999)
	int GetMilliSeconds() const
	{
		INT64 n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS;
		GetComponents(n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS);
		return (int)(n_Sign * n_mS);
	}

	static FILETIMESPAN FromDays(int days) { FILETIMESPAN s; s.Set(days, 0, 0, 0, 0); return s; }
	static FILETIMESPAN FromHours(int hours) { FILETIMESPAN s; s.Set(0, hours, 0, 0, 0); return s; }
	static FILETIMESPAN FromMinutes(int minutes) { FILETIMESPAN s; s.Set(0, 0, minutes, 0, 0); return s; }
	static FILETIMESPAN FromSeconds(int seconds) { FILETIMESPAN s; s.Set(0, 0, 0, seconds, 0); return s; }
	static FILETIMESPAN FromMilliSeconds(int mS) { FILETIMESPAN s; s.Set(0, 0, 0, 0, mS); return s; }

	// Get components (as positive values and sign as a separate component)
	// Return true if days < 99999
	bool GetComponents(INT64& n_Sign, INT64& n_Days, INT64& n_HH, INT64& n_MM, INT64& n_SS, INT64& n_mS) const
	{
		INT64 n_Offset = m_n64Span;
		n_Sign = (n_Offset < 0) ? -1 : 1;
		n_Offset *= n_Sign;
		n_Days = n_Offset / FT_DAY;
		n_Offset %= FT_DAY;
		n_HH = n_Offset / FT_HOUR;
		n_Offset %= FT_HOUR;
		n_MM = n_Offset / FT_MINUTE;
		n_Offset %= FT_MINUTE;
		n_SS = n_Offset / FT_SECOND;
		n_Offset %= FT_SECOND;
		n_mS = n_Offset / FT_MILLISECOND;
		return (n_Days < 99999 && n_HH < 24 && n_MM < 60 && n_SS < 60 && n_mS < 1000);
	}

	void Set(INT64 lDays, int nHours, int nMins, int nSecs, int nMilliSeconds)
	{
		m_n64Span = (lDays * FT_DAY) + (nHours * FT_HOUR) + (nMins * FT_MINUTE) + (nSecs * FT_SECOND) + (nMilliSeconds * FT_MILLISECOND);
		MKSPANDBGSTR;
	}

	// Parse time offset - format expected is +/-HH:MM:SS,mS -e.g. '-1:23,405'
	bool ParseTimeOffset(const char* szTimeOffset)
	{
		// Format:
		// -HH:MM:SS,MLS
		//           1111
		// 01234567890123

		char szTO[20] = { 0 };
		size_t len = strlen(szTimeOffset);
		if (len > 15 || len < 1)
		{
			return false;
		}
		char first = szTimeOffset[0];
		const char* szBeg = (first == '-' || first == '+') ? szTimeOffset + 1 : szTimeOffset;
		strcpy_s(szTO, sizeof(szTO), szBeg);
		bool bNegative = first == '-' ? true : false;
		char* p_mS = 0, * p_SS = 0, * p_MM = 0, * p_HH = 0, * pc = szTO + strlen(szTO) - 1, ** ppc;
		while (pc > szTO)
		{
			char c = *pc;
			if (c == ',')
			{
				p_mS = pc + 1;
				*pc = 0;
			}
			else if (c == ':')
			{
				ppc = p_SS == 0 ? &p_SS : p_MM == 0 ? &p_MM : p_HH == 0 ? &p_HH : nullptr;
				if (ppc == nullptr)
				{
					return false;	// invalid format.
				}
				*ppc = pc + 1;
				*pc = 0;
			}
			else if (c < '0' || c > '9')
			{
				return false;	// Invalid format.
			}
			pc--;
		}
		ppc = p_SS == 0 ? &p_SS : p_MM == 0 ? &p_MM : p_HH == 0 ? &p_HH : nullptr;
		if (ppc)
		{
			*ppc = szTO;
		}
		INT64 n_mS = p_mS != nullptr ? atoi(p_mS) * FT_MILLISECOND : 0;
		INT64 n_SS = p_SS != nullptr ? atoi(p_SS) * FT_SECOND : 0;
		INT64 n_MM = p_MM != nullptr ? atoi(p_MM) * FT_MINUTE : 0;
		INT64 n_HH = p_HH != nullptr ? atoi(p_HH) * FT_HOUR : 0;
		INT64 n_Sign = bNegative ? -1 : 1;
		INT64 n_Offset = (n_HH + n_MM + n_SS + n_mS) * n_Sign;
		m_n64Span = n_Offset;
		MKSPANDBGSTR;
		return true;
	}
	bool ParseTimeOffsetW(const wchar_t* szTimeOffset)
	{
		// Format:
		// -HH:MM:SS,MLS
		//           1111
		// 01234567890123

		wchar_t szTO[20] = { 0 };
		size_t len = wcslen(szTimeOffset);
		if (len > 15 || len < 1)
		{
			return false;
		}
		wchar_t first = szTimeOffset[0];
		const wchar_t* szBeg = (first == L'-' || first == L'+') ? szTimeOffset + 1 : szTimeOffset;
		wcscpy_s(szTO, sizeof(szTO) / sizeof(wchar_t), szBeg);
		bool bNegative = first == '-' ? true : false;
		wchar_t* p_mS = 0, * p_SS = 0, * p_MM = 0, * p_HH = 0, * pc = szTO + wcslen(szTO) - 1, ** ppc;
		while (pc > szTO)
		{
			wchar_t c = *pc;
			if (c == L',')
			{
				p_mS = pc + 1;
				*pc = 0;
			}
			else if (c == L':')
			{
				ppc = p_SS == 0 ? &p_SS : p_MM == 0 ? &p_MM : p_HH == 0 ? &p_HH : nullptr;
				if (ppc == nullptr)
				{
					return false;	// invalid format.
				}
				*ppc = pc + 1;
				*pc = 0;
			}
			else if (c < L'0' || c > L'9')
			{
				return false;	// Invalid format.
			}
			pc--;
		}
		ppc = p_SS == 0 ? &p_SS : p_MM == 0 ? &p_MM : p_HH == 0 ? &p_HH : nullptr;
		*ppc = szTO;
		INT64 n_mS{ 0 }, n_SS{ 0 }, n_MM{ 0 }, n_HH{ 0 };
		try
		{
			n_mS = p_mS != nullptr ? std::stoi(p_mS) * FT_MILLISECOND : 0;
			n_SS = p_SS != nullptr ? std::stoi(p_SS) * FT_SECOND : 0;
			n_MM = p_MM != nullptr ? std::stoi(p_MM) * FT_MINUTE : 0;
			n_HH = p_HH != nullptr ? std::stoi(p_HH) * FT_HOUR : 0;
		}
		catch (const std::exception&)
		{
			XeASSERT(false);
		}
		INT64 n_Sign = bNegative ? -1 : 1;
		INT64 n_Offset = (n_HH + n_MM + n_SS + n_mS) * n_Sign;
		m_n64Span = n_Offset;
		MKSPANDBGSTR;
		return true;
	}

	bool IsZero() const
	{
		return m_n64Span == 0;
	}

	void SetZero()
	{
		m_n64Span = 0;
		MKSPANDBGSTR;
	}

	void Set(INT64 n64span)
	{
		m_n64Span = n64span;
		MKSPANDBGSTR;
	}

	bool IsNegative() const
	{
		return m_n64Span < 0;
	}

	INT64 GetAs_mS() const
	{
		return m_n64Span / FT_MILLISECOND;
	}

	void SetFrom_mS(INT64 mS)
	{
		m_n64Span = mS * FT_MILLISECOND;
		MKSPANDBGSTR;
	}

	std::string ToStr(SPANFMT fmt) const
	{
		std::string strDT;
		switch (fmt)
		{
		case SPANFMT::HMSmS:
		{
			INT64 n_Offset = m_n64Span;
			INT64 n_Sign = (n_Offset < 0) ? -1 : 1;
			n_Offset *= n_Sign;
			INT64 n_HH = n_Offset / FT_HOUR;
			n_Offset %= FT_HOUR;
			INT64 n_MM = n_Offset / FT_MINUTE;
			n_Offset %= FT_MINUTE;
			INT64 n_SS = n_Offset / FT_SECOND;
			n_Offset %= FT_SECOND;
			INT64 n_mS = n_Offset / FT_MILLISECOND;
			char sign = n_Sign < 0 ? '-' : ' ';
			constexpr size_t buf_len = 30;
			char buffer[buf_len];
			sprintf_s(buffer, buf_len, "%c%d:%02d:%02d,%03d", sign, (int)n_HH, (int)n_MM, (int)n_SS, (int)n_mS);
			strDT = buffer;
		}
		break;
		case SPANFMT::InMilliseconds:
		{
			INT64 n_mS = m_n64Span / FT_MILLISECOND;
			strDT = std::to_string(n_mS);
		}
		break;
		default:
			_ASSERT(FALSE);
			break;
		}
		return strDT;
	}
	std::wstring ToStrW(SPANFMT fmt) const
	{
		std::wstring strDT;
		switch (fmt)
		{
		case SPANFMT::HMSmS:
		{
			INT64 n_Offset = m_n64Span;
			INT64 n_Sign = (n_Offset < 0) ? -1 : 1;
			n_Offset *= n_Sign;
			INT64 n_HH = n_Offset / FT_HOUR;
			n_Offset %= FT_HOUR;
			INT64 n_MM = n_Offset / FT_MINUTE;
			n_Offset %= FT_MINUTE;
			INT64 n_SS = n_Offset / FT_SECOND;
			n_Offset %= FT_SECOND;
			INT64 n_mS = n_Offset / FT_MILLISECOND;
			wchar_t sign = n_Sign < 0 ? L'-' : L' ';
			constexpr size_t buf_len = 30;
			wchar_t buffer[buf_len];
			swprintf(buffer, buf_len, L"%c%d:%02d:%02d,%03d", sign, (int)n_HH, (int)n_MM, (int)n_SS, (int)n_mS);
			strDT = buffer;
		}
		break;
		case SPANFMT::InMilliseconds:
		{
			INT64 n_mS = m_n64Span / FT_MILLISECOND;
			strDT = std::to_wstring(n_mS);
		}
		break;
		default:
			_ASSERT(FALSE);
			break;
		}
		return strDT;
	}

#ifdef _DEBUG
	char m_szDateTime[24] = "DAYSd HH:MM:SS,mmm";

	// Show "this" as timespan in the debugger.
	// m_szDateTime is used in the NatvisFile.natvis visualizer.
	void MakeDbgStr()
	{
		INT64 n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS;
		bool isValid = GetComponents(n_Sign, n_Days, n_HH, n_MM, n_SS, n_mS);
		char sign = n_Sign < 0 ? '-' : ' ';
		if (isValid)
		{
			if (n_Days == 0)
			{
				sprintf_s(m_szDateTime, sizeof(m_szDateTime), "%c%d:%02d:%02d.%03d",
					sign, (int)n_HH, (int)n_MM, (int)n_SS, (int)n_mS);
			}
			else
			{
				sprintf_s(m_szDateTime, sizeof(m_szDateTime), "%d(d) %c%d:%02d:%02d.%03d",
					(int)n_Days, sign, (int)n_HH, (int)n_MM, (int)n_SS, (int)n_mS);
			}
		}
		else
		{
			strcpy_s(m_szDateTime, sizeof(m_szDateTime), "INVALID SPAN");
		}
	}
#endif
};

// Parse date/time - format MUST be 'YYYY MM DD HH MM SS MLS'.
// char between the numbers is not checked (can be anything).
export struct SYSTEMTIMEX : public SYSTEMTIME
{
	inline bool TryParseISO8601format(const char* pS, size_t sourceLen)
	{
		if (sourceLen < 23)
		{
			return false;
		}
		// Format:
		// YYYY MM DD HH MM SS MLS
		//           11111111112222
		// 012345678901234567890123
		int16_t d[17];
		d[0] = pS[0] - '0'; d[1] = pS[1] - '0'; d[2] = pS[2] - '0'; d[3] = pS[3] - '0';
		d[4] = pS[5] - '0'; d[5] = pS[6] - '0';
		d[6] = pS[8] - '0'; d[7] = pS[9] - '0';
		d[8] = pS[11] - '0'; d[9] = pS[12] - '0';
		d[10] = pS[14] - '0'; d[11] = pS[15] - '0';
		d[12] = pS[17] - '0'; d[13] = pS[18] - '0';
		d[14] = pS[20] - '0'; d[15] = pS[21] - '0'; d[16] = pS[22] - '0';
		for (size_t i = 0; i < 17; ++i)
		{
			if (d[i] < 0 || d[i] > 9) { return false; }
		}
		wYear = d[0] * 1000 + d[1] * 100 + d[2] * 10 + d[3];
		wMonth = d[4] * 10 + d[5];
		wDay = d[6] * 10 + d[7];
		wHour = d[8] * 10 + d[9];
		wMinute = d[10] * 10 + d[11];
		wSecond = d[12] * 10 + d[13];
		wMilliseconds = d[14] * 100 + d[15] * 10 + d[16];
		return true;
	}

	inline bool TryParseISO8601format(const wchar_t* pS, size_t sourceLen)
	{
		if (sourceLen < 23)
		{
			return false;
		}
		// Format:
		// YYYY MM DD HH MM SS MLS
		//           11111111112222
		// 012345678901234567890123
		int16_t d[17];
		d[0] = pS[0] - '0'; d[1] = pS[1] - '0'; d[2] = pS[2] - '0'; d[3] = pS[3] - '0';
		d[4] = pS[5] - '0'; d[5] = pS[6] - '0';
		d[6] = pS[8] - '0'; d[7] = pS[9] - '0';
		d[8] = pS[11] - '0'; d[9] = pS[12] - '0';
		d[10] = pS[14] - '0'; d[11] = pS[15] - '0';
		d[12] = pS[17] - '0'; d[13] = pS[18] - '0';
		d[14] = pS[20] - '0'; d[15] = pS[21] - '0'; d[16] = pS[22] - '0';
		for (size_t i = 0; i < 17; ++i)
		{
			if (d[i] < 0 || d[i] > 9) { return false; }
		}
		wYear = d[0] * 1000 + d[1] * 100 + d[2] * 10 + d[3];
		wMonth = d[4] * 10 + d[5];
		wDay = d[6] * 10 + d[7];
		wHour = d[8] * 10 + d[9];
		wMinute = d[10] * 10 + d[11];
		wSecond = d[12] * 10 + d[13];
		wMilliseconds = d[14] * 100 + d[15] * 10 + d[16];
		return true;
	}

	inline bool TryParseISO8601_OFFSET_DATE_TIME_HHCMM2format(const char* pS, size_t sourceLen,
			FILETIMESPAN& timezone)
	{
		if (sourceLen < 32)
		{
			return false;
		}
		// Format:                    +/- timezone
		// YYYY MM DD HH MM SS NANOSE+HH MM
		//           11111111112222222222333
		// 012345678901234567890123456789012
		int16_t d[32];
		bool isTzPositive = false;
		for (size_t i = 0; i < 32; ++i)
		{
			int16_t& n = d[i];
			n = pS[i] - '0';
			if (i == 26)
			{
				isTzPositive = pS[i] == '+';
				continue;
			}
			if (!(i == 4 || i == 7 || i == 10 || i == 13 || i == 16 || i == 19 || i == 29)
					&& (n < 0 || n > 9))
			{
				return false;
			}
		}
		wYear = d[0] * 1000 + d[1] * 100 + d[2] * 10 + d[3];
		wMonth = d[5] * 10 + d[6];
		wDay = d[8] * 10 + d[9];
		wHour = d[11] * 10 + d[12];
		wMinute = d[14] * 10 + d[15];
		wSecond = d[17] * 10 + d[18];
		wMilliseconds = d[20] * 100 + d[21] * 10 + d[22];

		int tzMul = pS[26] == '-' ? -1 : 1;
		int tzHours = (d[27] * 10 + d[28]);
		int tzMinutes = (d[30] * 10 + d[31]);
		timezone.Set(0, tzHours, tzMinutes, 0, 0);
		timezone *= tzMul;

		// Note - the last three digits of nanoseconds are ignored because SYSTEMTIME only supports milli seconds.
		return true;
	}

	inline bool TryParseISO8601_OFFSET_DATE_TIME_HHCMM3format(const char* pS, size_t sourceLen,
			FILETIMESPAN& timezone)
	{
		if (sourceLen < 32)
		{
			return false;
		}
		// Format:                     +/- timezone
		// YYYY MM DD HH MM SS NANOSEC+HH MM
		// 2025-09-23T07:38:03.8224727-07:00
		// 000000000011111111112222222222333
		// 012345678901234567890123456789012
		int16_t d[33];	// array for individual digits.
		bool isTzPositive = false;
		for (size_t i = 0; i < 33; ++i)
		{
			int16_t& n = d[i];
			n = pS[i] - '0';
			if (i == 27)
			{
				isTzPositive = pS[i] == '+';
				continue;
			}
			if (!(i == 4 || i == 7 || i == 10 || i == 13 || i == 16 || i == 19 || i == 30)
					&& (n < 0 || n > 9))
			{
				return false;
			}
		}
		wYear = d[0] * 1000 + d[1] * 100 + d[2] * 10 + d[3];
		wMonth = d[5] * 10 + d[6];
		wDay = d[8] * 10 + d[9];
		wHour = d[11] * 10 + d[12];
		wMinute = d[14] * 10 + d[15];
		wSecond = d[17] * 10 + d[18];
		wMilliseconds = d[20] * 100 + d[21] * 10 + d[22];

		int tzMul = pS[27] == '-' ? -1 : 1;
		int tzHours = (d[28] * 10 + d[29]);
		int tzMinutes = (d[31] * 10 + d[32]);
		timezone.Set(0, tzHours, tzMinutes, 0, 0);
		timezone *= tzMul;

		// Note - the last three digits of nanoseconds are ignored because SYSTEMTIME only supports milli seconds.
		return true;
	}

	inline bool IsLeapYear() const
	{
		return (wYear % 400 == 0 || wYear % 100 != 0) && (wYear % 4 == 0);
	}

	inline bool Is30dayMonth() const
	{
		return wMonth == 4 || wMonth == 6 || wMonth == 9 || wMonth == 11;
	}

	inline WORD GetMaxDayForMonth() const
	{
		return Is30dayMonth() ? 30 : wMonth == 2 ? IsLeapYear() ? 29 : 28 : 31;
	}

	inline bool IsDayValidForMonth() const
	{
		return wDay <= GetMaxDayForMonth();
	}

	inline bool IsValidYear() const { return wYear >= 1601 && wYear <= 3000; }
	inline bool IsValidMonth() const { return wMonth >= 1 && wMonth <= 12; }
	inline bool IsValidDay() const { return wDay >= 1 && IsDayValidForMonth(); }
	inline bool IsValidHour() const { return wHour <= 23; }
	inline bool IsValidMinute() const { return wMinute <= 59; }
	inline bool IsValidSecond() const { return wSecond <= 59; }
	inline bool IsValidMilliSecond() const { return wMilliseconds <= 999; }
};

export struct FILETIMEX
{
	union
	{
		UINT64 m_u64;
		struct
		{
			DWORD dwLow;
			DWORD dwHigh;
		};
	};

	FILETIMEX()
	{
		m_u64 = 0;
		MKDBGSTR;
	}

	FILETIMEX(UINT64 u64FT)
	{
		m_u64 = u64FT;
		MKDBGSTR;
	}

	FILETIMEX(DWORD low, DWORD high)
	{
		dwLow = low;
		dwHigh = high;
		MKDBGSTR;
	}

	FILETIMEX(const FILETIMEX& source)
	{
		m_u64 = source.m_u64;
		MKDBGSTR;
	}

	FILETIMEX(SYSTEMTIME& source)
	{
		::SystemTimeToFileTime(&source, (FILETIME*)this);
		MKDBGSTR;
	}

	bool operator==(const FILETIMEX& ftB) const
	{
		return m_u64 == ftB.m_u64;
	}

	bool operator!=(const FILETIMEX& ftB) const
	{
		return m_u64 != ftB.m_u64;
	}

	bool operator>(const FILETIMEX& ftB) const
	{
		return m_u64 > ftB.m_u64;
	}

	bool operator>=(const FILETIMEX& ftB) const
	{
		return m_u64 >= ftB.m_u64;
	}

	bool operator<(const FILETIMEX& ftB) const
	{
		return m_u64 < ftB.m_u64;
	}

	bool operator<=(const FILETIMEX& ftB) const
	{
		return m_u64 <= ftB.m_u64;
	}

	FILETIMEX& operator=(const FILETIMEX& ftRHS)
	{
		m_u64 = ftRHS.m_u64;
		MKDBGSTR;
		return *this;
	}

	FILETIMEX& operator=(const SYSTEMTIME& stRHS)
	{
		SystemTimeToFileTime(&stRHS, (FILETIME*)this);
		MKDBGSTR;
		return *this;
	}

	// FILETIMEX math
	FILETIMEX operator+(FILETIMESPAN span) const
	{
		_ASSERT(!IsZeroOrMax());
		return FILETIMEX(m_u64 + span.m_n64Span);
	}

	FILETIMEX operator-(FILETIMESPAN span) const
	{
		_ASSERT(!IsZeroOrMax());
		return FILETIMEX(m_u64 - span.m_n64Span);
	}

	FILETIMEX& operator+=(FILETIMESPAN span)
	{
		_ASSERT(!IsZeroOrMax());
		m_u64 = m_u64 + span.m_n64Span;
		MKDBGSTR;
		return *this;
	}

	FILETIMEX& operator-=(FILETIMESPAN span)
	{
		_ASSERT(!IsZeroOrMax());
		m_u64 = m_u64 - span.m_n64Span;
		MKDBGSTR;
		return *this;
	}

	// FILETIMESPAN math (e.g. FILETIMESPAN s = ft1 - ft2;
	FILETIMESPAN operator-(const FILETIMEX& ft2) const
	{
		_ASSERT(!(IsZeroOrMax() || ft2.IsZeroOrMax()));
		if (IsZeroOrMax() || ft2.IsZeroOrMax())
		{
			return FILETIMESPAN();
		}
		return FILETIMESPAN(m_u64 - ft2.m_u64);
	}

	// Parse date/time - format MUST be 'YYYY MM DD HH MM SS MLS'.
	// char between the numbers is not checked (can be anything).
	bool TryParseISO8601format(const char* pS, size_t sourceLen)
	{
		SYSTEMTIMEX st;
		bool isOk = st.TryParseISO8601format(pS, sourceLen)
			&& SystemTimeToFileTime(&st, (FILETIME*)this);
		MKDBGSTR;
		return isOk;
	}

	// Parse date/time - format MUST be 'YYYY MM DD HH MM SS MLS'.
	// char between the numbers is not checked (can be anything).
	bool TryParseISO8601format(const wchar_t* pS, size_t sourceLen)
	{
		SYSTEMTIMEX st;
		bool isOk = st.TryParseISO8601format(pS, sourceLen)
			&& SystemTimeToFileTime(&st, (FILETIME*)this);
		MKDBGSTR;
		return isOk;
	}

	bool TryParseISO8601_OFFSET_DATE_TIME_HHCMM2format(const char* pS, size_t sourceLen)
	{
		SYSTEMTIMEX st;
		FILETIMESPAN tz;
		FILETIMEX ft;
		bool isOk = st.TryParseISO8601_OFFSET_DATE_TIME_HHCMM2format(pS, sourceLen, tz)
			&& SystemTimeToFileTime(&st, (FILETIME*)&ft);
		ft += tz;
		this->Set(ft.m_u64);
		MKDBGSTR;
		return isOk;
	}

	bool TryParseISO8601_OFFSET_DATE_TIME_HHCMM3format(const char* pS, size_t sourceLen)
	{
		SYSTEMTIMEX st;
		FILETIMESPAN tz;
		FILETIMEX ft;
		bool isOk = st.TryParseISO8601_OFFSET_DATE_TIME_HHCMM3format(pS, sourceLen, tz)
			&& SystemTimeToFileTime(&st, (FILETIME*)&ft);
		ft += tz;
		this->Set(ft.m_u64);
		MKDBGSTR;
		return isOk;
	}

	bool ParseDateTime(const std::string& src)
	{
		SYSTEMTIMEX st;
		bool isOk = st.TryParseISO8601format(src.c_str(), src.size())
			&& SystemTimeToFileTime(&st, (FILETIME*)this);
		MKDBGSTR;
		return isOk;
	}

	inline bool IsZero() const { return m_u64 == 0; }

	void SetZero()
	{
		m_u64 = 0;
		MKDBGSTR;
	}

	inline bool IsMax() const { return m_u64 == 0xFFFFFFFFFFFFFFFF; }

	void SetMax()
	{
		m_u64 = 0xFFFFFFFFFFFFFFFF;
		MKDBGSTR;
	}

	inline bool IsZeroOrMax() const { return IsZero() || IsMax(); }

	inline bool IsValid() const { return !IsZeroOrMax(); }

	void Set(UINT64 ft)
	{
		m_u64 = ft;
		MKDBGSTR;
	}

	// Update 'this' date/time if ftOther smaller than 'this'.
	// Note - If 'this' is zero and ftOther is not zero then set 'this' to ftOther.
	void UpdateIfSmaller(const FILETIMEX& ftOther)
	{
		if (!ftOther.IsZero())
		{
			if (IsZero() || ftOther < *this)
			{
				m_u64 = ftOther.m_u64;
				MKDBGSTR;
			}
		}
	}

	// Update 'this' date/time if ftOther bigger than 'this'.
	// Note - If 'this' is zero and ftOther is not zero then set 'this' to ftOther.
	void UpdateIfBigger(const FILETIMEX& ftOther)
	{
		if (!ftOther.IsZero())
		{
			if (IsZero() || ftOther > *this)
			{
				m_u64 = ftOther.m_u64;
				MKDBGSTR;
			}
		}
	}

	void SetUTCnow() noexcept
	{
		SYSTEMTIME st = { 0 };
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, (FILETIME*)this);
		MKDBGSTR;
	}

	static FILETIMEX UTCnow()
	{
		FILETIMEX utcNow;
		utcNow.SetUTCnow();
		return utcNow;
	}

	void SetLocalNow()
	{
		SYSTEMTIME st = { 0 };
		GetLocalTime(&st);
		SystemTimeToFileTime(&st, (FILETIME*)this);
		MKDBGSTR;
	}

	static FILETIMEX LocalNow()
	{
		FILETIMEX localNow;
		localNow.SetLocalNow();
		return localNow;
	}

	void RoundToInterval(UINT64 uInterval, EROUNDTO eRoundTo)
	{
		UINT64 uRemainder = m_u64 % uInterval;
		if (eRoundTo == EROUNDTO::NEAREST)
		{
			eRoundTo = (uRemainder < (uInterval / 2)) ? EROUNDTO::DOWN : EROUNDTO::UP;
		}
		switch (eRoundTo)
		{
		case EROUNDTO::DOWN:
			m_u64 -= uRemainder;
			break;
		case EROUNDTO::UP:
			m_u64 = m_u64 - uRemainder + uInterval;
			break;
		case EROUNDTO::NEAREST:
		default:
			_ASSERT(FALSE);
			break;
		}
		MKDBGSTR;
	}

	SYSTEMTIME ToSystemTime() const
	{
		SYSTEMTIME st;
		FileTimeToSystemTime((FILETIME*)this, &st);
		return st;
	}

	bool FromSystemTime(const SYSTEMTIME& st)
	{
		if (::SystemTimeToFileTime(&st, (FILETIME*)this))
		{
			MKDBGSTR;
			return true;
		}
		return false;
	}

	bool IsMidnight() const
	{
		if (IsZeroOrMax())
		{
			return false;
		}
		SYSTEMTIME st;
		FileTimeToSystemTime((FILETIME*)this, &st);
		return (st.wHour == 0 && st.wMinute == 0 && st.wSecond == 0);
	}

	std::string ToStr(DATEFMT fmt) const
	{
		std::string strDT;
		if (!(IsZero() || IsMax()))
		{
			constexpr size_t buf_len = 50;
			char buffer[buf_len]{ 0 };
			SYSTEMTIME st = { 0 };
			FileTimeToSystemTime((FILETIME*)this, &st);
			switch (fmt)
			{
			case DATEFMT::ISOplus_mS:
				sprintf_s(buffer, buf_len, "%04u-%02u-%02u %02u:%02u:%02u,%03u",
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
				break;
			case DATEFMT::YMD_HMS:
				sprintf_s(buffer, buf_len, "%04u-%02u-%02u %02u:%02u:%02u",
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
				break;
			case DATEFMT::YMD_HMS_forBMfilename:
				sprintf_s(buffer, buf_len, "%04u%02u%02u_%02u%02u%02u",
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
				break;
			case DATEFMT::YMD_HMSmS_forFilename:
				sprintf_s(buffer, buf_len, "%04u%02u%02u_%02u%02u%02u_%03u",
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
				break;
			case DATEFMT::DMY_HMS:
				sprintf_s(buffer, buf_len, "%02u-%02u-%04u %02u:%02u:%02u",
					st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
				break;
			case DATEFMT::HMS:
				sprintf_s(buffer, buf_len, "%02u:%02u:%02u", st.wHour, st.wMinute, st.wSecond);
				break;
			case DATEFMT::DMY:
				sprintf_s(buffer, buf_len, "%02u-%02u-%04u", st.wDay, st.wMonth, st.wYear);
				break;
			case DATEFMT::YMD:
				sprintf_s(buffer, buf_len, "%04u-%02u-%02u", st.wYear, st.wMonth, st.wDay);
				break;
			}
			strDT = buffer;
		}
		return strDT;
	}

	std::wstring ToStrW(DATEFMT fmt) const
	{
		std::wstring strDT;
		if (!(IsZero() || IsMax()))
		{
			constexpr size_t buf_len = 50;
			wchar_t buffer[buf_len]{ 0 };
			SYSTEMTIME st = { 0 };
			FileTimeToSystemTime((FILETIME*)this, &st);
			switch (fmt)
			{
			case DATEFMT::ISOplus_mS:
				swprintf_s(buffer, buf_len, L"%04u-%02u-%02u %02u:%02u:%02u,%03u",
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
				break;
			case DATEFMT::YMD_HMS:
				swprintf_s(buffer, buf_len, L"%04u-%02u-%02u %02u:%02u:%02u",
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
				break;
			case DATEFMT::YMD_HMS_forBMfilename:
				swprintf_s(buffer, buf_len, L"%04u%02u%02u_%02u%02u%02u",
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
				break;
			case DATEFMT::YMD_HMSmS_forFilename:
				swprintf_s(buffer, buf_len, L"%04u%02u%02u_%02u%02u%02u_%03u",
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
				break;
			case DATEFMT::DMY_HMS:
				swprintf_s(buffer, buf_len, L"%02u-%02u-%04u %02u:%02u:%02u",
					st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
				break;
			case DATEFMT::HMS:
				swprintf_s(buffer, buf_len, L"%02u:%02u:%02u", st.wHour, st.wMinute, st.wSecond);
				break;
			case DATEFMT::DMY:
				swprintf_s(buffer, buf_len, L"%02u-%02u-%04u", st.wDay, st.wMonth, st.wYear);
				break;
			case DATEFMT::YMD:
				swprintf_s(buffer, buf_len, L"%04u-%02u-%02u", st.wYear, st.wMonth, st.wDay);
				break;
			}
			strDT = buffer;
		}
		return strDT;
	}

#ifdef _DEBUG
	char m_szDateTime[24] = "YYYY-MM-DD HH:MM:SS,mmm";

	// Show "this" as date/time in the debugger.
	// m_szDateTime is used in the NatvisFile.natvis visualizer.
	void MakeDbgStr()
	{
		if (IsZero())
		{
			strcpy_s(m_szDateTime, sizeof(m_szDateTime), "ZERO");
		}
		else if (IsMax())
		{
			strcpy_s(m_szDateTime, sizeof(m_szDateTime), "MAX");
		}
		else
		{
			SYSTEMTIME st = { 0 };
			FileTimeToSystemTime((FILETIME*)this, &st);
			bool isValid = st.wYear < 10000 && st.wMonth < 13 && st.wDay < 32
				&& st.wHour < 24 && st.wMinute < 60 && st.wSecond < 60 && st.wMilliseconds < 1000;
			if (isValid)
			{
				sprintf_s(m_szDateTime, sizeof(m_szDateTime), "%04u-%02u-%02u %02u:%02u:%02u.%03u",
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
			}
			else
			{
				strcpy_s(m_szDateTime, sizeof(m_szDateTime), "INVALID date/time");
			}
		}
	}
#endif
};

export struct FILETIMEX_RANGE
{
	FILETIMEX m_ftFrom, m_ftTo;
	FILETIMEX_RANGE() = default;
	FILETIMEX_RANGE(FILETIMEX from, FILETIMEX to) : m_ftFrom(from), m_ftTo(to) {}
	bool operator==(const FILETIMEX_RANGE& rhs) const
	{
		return m_ftFrom == rhs.m_ftFrom && m_ftTo == rhs.m_ftTo;
	}
	bool operator!=(const FILETIMEX_RANGE& rhs) const { return !operator==(rhs); }
	FILETIMEX_RANGE& operator=(const FILETIMEX_RANGE& rhs)
	{
		m_ftFrom = rhs.m_ftFrom;
		m_ftTo = rhs.m_ftTo;
		return *this;
	}
	inline bool IsWithinRange(const FILETIMEX& ft) const
	{
		return !((!m_ftFrom.IsZero() && ft < m_ftFrom) || (!m_ftTo.IsZero() && ft > m_ftTo));
	}
	bool IsValid() const { return m_ftFrom.IsValid() || m_ftTo.IsValid(); }
	void Set(FILETIMEX from, FILETIMEX to)
	{
		m_ftFrom = from;
		m_ftTo = to;
	}
	std::string to_json_string() const
	{
		return "'" + m_ftFrom.ToStr(DATEFMT::ISOplus_mS)
			+ "','" + m_ftTo.ToStr(DATEFMT::ISOplus_mS) + "'";
	}
	void set_from_json_string(const std::string& s)
	{
		m_ftFrom.SetZero();
		m_ftTo.SetZero();
		if (s.size() == 0 || s[0] != '\'') { return; }
		std::string::size_type fp = s.find('\'', 1);
		if (fp == std::string::npos) { return; }
		std::string s1 = s.substr(1, fp - 1);
		m_ftFrom.ParseDateTime(s1);
		if (s.size() < fp + 3 || s[fp + 1] != ',' || s[fp + 2] != '\'') { return; }
		std::string::size_type fp1 = fp + 3;
		fp = s.find('\'', fp1);
		if (fp == std::string::npos) { return; }
		std::string s2 = s.substr(fp1, fp - fp1);
		m_ftTo.ParseDateTime(s2);
	}
	std::string as_string() const
	{
		return "From: " + m_ftFrom.ToStr(DATEFMT::ISOplus_mS)
			+ ", To: " + m_ftTo.ToStr(DATEFMT::ISOplus_mS);
	}
};
#pragma pack( pop )

// Performance critical!
#ifdef _DEBUG
static_assert(sizeof(FILETIMEX) == 32, "FILETIMEX size incorrect");
static_assert(sizeof(FILETIMESPAN) == 32, "FILETIMESPAN size incorrect");
#else
static_assert(sizeof(FILETIMEX) == 8, "FILETIMEX size incorrect");
static_assert(sizeof(FILETIMESPAN) == 8, "FILETIMESPAN size incorrect");
#endif // DEBUG

