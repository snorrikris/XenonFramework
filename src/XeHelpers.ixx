module;

#include "os_minimal.h"
#include <memory>
#include <map>
#include <random>
//#include "ipp.h"
#include "logging.h"

#include <bcrypt.h>

export module Xe.Helpers;

export import Xe.FileVersionInfo;
export import Xe.FileHelpers;
import Xe.mfc_types;
//import Xe.LogDefs;
import Xe.DefData;
import Xe.StringTools;
import Xe.FileTimeX;

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif

#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "bcrypt.lib")

export class xen
{
public:
	static int stoi(const std::wstring& s)
	{
		try
		{
			int n = std::stoi(s);
			return n;
		}
		catch (const std::exception&)
		{
			return 0;
		}
	}
	static long stol(const std::wstring& s)
	{
		try
		{
			long n = std::stol(s);
			return n;
		}
		catch (const std::exception&)
		{
			return 0;
		}
	}
	static long long stoll(const std::wstring& s)
	{
		try
		{
			long long n = std::stoll(s);
			return n;
		}
		catch (const std::exception&)
		{
			return 0;
		}
	}
	static unsigned long stoul(const std::wstring& s)
	{
		try
		{
			unsigned long n = std::stoul(s);
			return n;
		}
		catch (const std::exception&)
		{
			return 0;
		}
	}
	static unsigned long long stoull(const std::wstring& s)
	{
		try
		{
			unsigned long long n = std::stoull(s);
			return n;
		}
		catch (const std::exception&)
		{
			return 0;
		}
	}
};

export void SortAndMakeDataSourceIdsUnique(std::vector<dsid_t>& ids)
{
	// Sort data source id's followed by unique, to remove all duplicates.
	std::sort(ids.begin(), ids.end());
	auto last = std::unique(ids.begin(), ids.end());
	ids.erase(last, ids.end());
}

export struct DToffsetInfo
{
	dsid_t dwDataSourceId;
	std::wstring strFN;
	FILETIMEX dtCurRow;

	DToffsetInfo() = default;
	DToffsetInfo(dsid_t dsId, const std::wstring& fn, FILETIMEX dt)
		: dwDataSourceId(dsId), strFN(fn), dtCurRow(dt) {}
};

export void TrimTrailingCRLF(std::string& str)
{
	const char* pStr = str.c_str();
	size_t n = str.size();
	size_t new_len = n;
	if (n == 0) { return; }
	n--;
	while (n > 0 && (pStr[n] == 13 || pStr[n] == 10))
	{
		--new_len;
		--n;
	}
	str.resize(new_len);
}

// Find and fix line endings, LF to CRLF, LFCR to CRLF.
export std::string FixLineEndings(const char* szInput)
{
	if (szInput == nullptr)
	{
		return std::string();
	}
	size_t lenInp = strlen(szInput);
	std::unique_ptr<char[]> buffer(new char[lenInp * 2]);
	char* pcOut = buffer.get();
	const char* pcIn = szInput;
	while (*pcIn)
	{
		if (*pcIn == '\r' || *pcIn == '\n')
		{
			if (*pcIn == '\r')
			{
				if (*(pcIn + 1) == '\n')	// Normal CRLF?
				{
					*pcOut++ = *pcIn++;
					*pcOut++ = *pcIn++;
				}
				else
				{	// CR only
					*pcOut++ = *pcIn++;
					*pcOut++ = '\n';
				}
			}
			else
			{	// LF only or reversed LFCR
				if (*(pcIn + 1) == '\r')	// LFCR?
				{
					*pcOut++ = '\r';
					*pcOut++ = '\n';
					pcIn += 2;
				}
				else
				{	// LF only
					*pcOut++ = '\r';
					*pcOut++ = *pcIn++;
				}
			}
		}
		else
		{
			*pcOut++ = *pcIn++;
		}
	}
	*pcOut = 0;
	return std::string(buffer.get());
}
export std::wstring FixLineEndingsW(const wchar_t* szInput)
{
	if (szInput == nullptr)
	{
		return std::wstring();
	}
	size_t lenInp = wcslen(szInput);
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[lenInp * 2]);
	wchar_t* pcOut = buffer.get();
	const wchar_t* pcIn = szInput;
	while (*pcIn)
	{
		if (*pcIn == L'\r' || *pcIn == L'\n')
		{
			if (*pcIn == L'\r')
			{
				if (*(pcIn + 1) == L'\n')	// Normal CRLF?
				{
					*pcOut++ = *pcIn++;
					*pcOut++ = *pcIn++;
				}
				else
				{	// CR only
					*pcOut++ = *pcIn++;
					*pcOut++ = L'\n';
				}
			}
			else
			{	// LF only or reversed LFCR
				if (*(pcIn + 1) == L'\r')	// LFCR?
				{
					*pcOut++ = L'\r';
					*pcOut++ = L'\n';
					pcIn += 2;
				}
				else
				{	// LF only
					*pcOut++ = L'\r';
					*pcOut++ = *pcIn++;
				}
			}
		}
		else
		{
			*pcOut++ = *pcIn++;
		}
	}
	*pcOut = 0;
	return std::wstring(buffer.get());
}

export std::string GetRGB(COLORREF rgb)
{
	BYTE r, g, b;
	b = (BYTE)(rgb >> 16);
	g = (rgb & 0xFF00) >> 8;
	r = rgb & 0xFF;
	std::unique_ptr<char[]> buffer(new char[16]);
	sprintf_s(buffer.get(), 16, "#%0.2X%0.2X%0.2X", r, g, b);
	return std::string(buffer.get());
}

export int GetValueInValidRange(int newValue, int minValue, int maxValue)
{
	return (newValue < minValue) ? minValue : (newValue > maxValue) ? maxValue : newValue;
}

export void CopyTextToClipboard(const std::string& strToCopy)
{
	::OpenClipboard(NULL);
	::EmptyClipboard();

	size_t len = (strToCopy.size() + 1) * sizeof(char);

	HGLOBAL hglobal = GlobalAlloc(GMEM_ZEROINIT, len);
	char* tempStr = (char*)::GlobalLock(hglobal);
	std::memcpy(tempStr, strToCopy.c_str(), strToCopy.size());
	tempStr[strToCopy.size()] = 0;

	::GlobalUnlock(hglobal);
	::SetClipboardData(CF_TEXT, hglobal);

	::CloseClipboard();
}

export void CopyTextToClipboardW(const std::wstring& strToCopy)
{
	::OpenClipboard(NULL);
	::EmptyClipboard();

	size_t str_mem_len = strToCopy.size() * sizeof(wchar_t);

	HGLOBAL hglobal = GlobalAlloc(GMEM_ZEROINIT, str_mem_len + sizeof(wchar_t));
	wchar_t* tempStr = (wchar_t*)::GlobalLock(hglobal);
	std::memcpy(tempStr, strToCopy.c_str(), str_mem_len);
	tempStr[str_mem_len] = 0;

	::GlobalUnlock(hglobal);
	::SetClipboardData(CF_UNICODETEXT, hglobal);

	::CloseClipboard();
}

bool _Get_CF_TEXT(std::string& txt)
{
	HGLOBAL hg = ::GetClipboardData(CF_TEXT);
	if (hg != NULL)
	{
		LPSTR data = (LPSTR)::GlobalLock(hg);
		unsigned long size = (long)::GlobalSize(hg);

		txt = std::string_view(data, size);

		::GlobalUnlock(hg);
	}
	return hg != NULL;
}

bool _Get_CF_UNICODETEXT(std::wstring& txt)
{
	HGLOBAL hg = ::GetClipboardData(CF_UNICODETEXT);
	if (hg != NULL)
	{
		const wchar_t* data = (const wchar_t*)::GlobalLock(hg);
		if (data)
		{
			txt = data;
		}
		::GlobalUnlock(hg);
	}
	return hg != NULL;
}

export std::string CopyTextFromClipBoard()
{
	std::string txt;
	if (::OpenClipboard(NULL))
	{
		if (!_Get_CF_TEXT(txt))
		{
			std::wstring txtW;
			if (_Get_CF_UNICODETEXT(txtW))
			{
				txt = xet::to_astr(txtW);
			}
		}
		::CloseClipboard();
	}
	return txt;
}

export std::wstring CopyTextFromClipBoardW()
{
	std::wstring txt;
	if (::OpenClipboard(NULL))
	{
		if (!_Get_CF_UNICODETEXT(txt))
		{
			std::string txtA;
			if (_Get_CF_TEXT(txtA))
			{
				txt = xet::to_wstr(txtA);
			}
		}
		::CloseClipboard();
	}
	return txt;
}

// Returns double value for display - used for 'disk space' display.
// E.g. 780610306048 returns 727 and cMx = 'G'  (727GBytes).
// cMx is one of 'K', 'M', 'G' or 'T'.
export double GetKMGTvalue(double dblVal, wchar_t& cMx)
{
	cMx = L'K';
	if ((dblVal /= 1024) > 1000)
	{
		cMx = L'M';
		if ((dblVal /= 1024) > 1000)
		{
			cMx = L'G';
			if ((dblVal /= 1024) > 1000)
			{
				cMx = L'T';
				dblVal /= 1024;
			}
		}
	}
	return dblVal;
}

export std::wstring GetNumberWithThousandsSeparator(int n, wchar_t cSep)
{
	std::wstring s = std::to_wstring(n);
	int sLen = (int)s.size();
	bool isNeg = s[0] == L'-';
	int numDigits = isNeg ? sLen - 1 : sLen;
	int numSepsNeeded = numDigits / 3;
	if (numSepsNeeded > 0 && numDigits % 3 == 0) { numSepsNeeded--; }
	int newLen = sLen + numSepsNeeded;
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[newLen + 1]);
	wchar_t* pS = buffer.get();
	*(pS + newLen) = 0;
	int count = 0, sIdx = sLen - 1;
	for (wchar_t* pIter = pS + newLen - 1; pIter >= pS; pIter--)
	{
		if (count == 3)
		{
			*pIter = cSep;
			count = 0;
		}
		else
		{
			*pIter = s[sIdx--];
			count++;
		}
	}
	return std::wstring(buffer.get());
}

// Returns a new GUID as string.
export std::wstring GetGuid()
{
	std::string strGuid;
	UUID uuID;
	UuidCreate(&uuID);
	char* pszID = nullptr;
	UuidToStringA(&uuID, (RPC_CSTR*)&pszID);
	if (pszID != nullptr)
	{
		strGuid = pszID;
		RpcStringFreeA((RPC_CSTR*)&pszID);
	}
	return xet::to_wstr(strGuid);
}

// Returns version number of current process (exe), build number not included. E.g. "1.0.2".
export std::wstring GetCurrentInstanceVersion()
{
	CFileVersionInfo exeVersion;
	exeVersion.Create(GetThisExePathname().c_str());
	std::wstring strVersion = exeVersion.m_strFileVersion;
	size_t len = strVersion.size();
	if (len >= 10)
	{
		strVersion.resize(len - 5);	// Remove 4 digit build number.
	}
	return strVersion;	// E.g. "1.0.2".
}

DWORD ParseVersionNumber(const std::wstring& strVersion)
{
	size_t len = strVersion.size();
	wchar_t* szVer = new wchar_t[len + 2];
	wcscpy_s(szVer, len + 2, strVersion.c_str());
	szVer[len] = '.';
	szVer[len + 1] = 0;

	wchar_t* szV = szVer;
	DWORD dwVersion = 0;
	wchar_t* pBeg = szV;
	while (*szV != 0)
	{
		if (*szV == '.')
		{
			*szV = 0;
			dwVersion += (DWORD)xen::stoi(pBeg);
			dwVersion <<= 8;
			pBeg = szV + 1;
		}
		szV++;
	}

	delete[] szVer;
	return dwVersion;
}

export bool IsNewerVersion(const std::wstring& strCurrentVersion, const std::wstring& strNewVersion)
{
	// Compare version numbers - format: "Major.Minor.SubMinor" e.g. "1.0.2".
	DWORD dwCurVersion = ParseVersionNumber(strCurrentVersion), dwNewVersion = ParseVersionNumber(strNewVersion);
	return dwNewVersion > dwCurVersion;
}

BOOL RunProgram(const std::wstring& app, std::wstring cmdLineParams, PROCESS_INFORMATION* pProcessInfo)
{
	STARTUPINFOW startupInfo = {};
	startupInfo.cb = sizeof(STARTUPINFOW);
	startupInfo.dwX = (DWORD)CW_USEDEFAULT;
	startupInfo.dwY = (DWORD)CW_USEDEFAULT;
	startupInfo.dwXSize = (DWORD)CW_USEDEFAULT;
	startupInfo.dwYSize = (DWORD)CW_USEDEFAULT;
	BOOL result = CreateProcessW(app.c_str(), cmdLineParams.data(), NULL, NULL, FALSE,
			NORMAL_PRIORITY_CLASS, NULL, NULL, &startupInfo, pProcessInfo);
	return result;
}

export BOOL RunAnotherInstance(std::wstring cmdLineParams)
{
	wchar_t szExeFN[MAX_PATH];
	GetModuleFileNameW(NULL, szExeFN, MAX_PATH);
	PROCESS_INFORMATION processInfo = {};
	return RunProgram(szExeFN, cmdLineParams, &processInfo);
}

export bool IsRunningAsAdministrator()
{
	bool isAdmin = false;
	HANDLE hToken = NULL;
	if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		TOKEN_ELEVATION te = { 0 };
		DWORD dwReturnLength = 0;
		if (::GetTokenInformation(hToken, TokenElevation, &te, sizeof(te), &dwReturnLength))
		{
			_ASSERT(dwReturnLength == sizeof(te));
			isAdmin = te.TokenIsElevated;
		}
		::CloseHandle(hToken);
	}
	return isAdmin;
}

#pragma warning (disable:4996)
export std::wstring PrepareTooltipText(const std::wstring& text, 
	const std::wstring& txtIns1 = std::wstring(), int nIns1pos = 0,
	const std::wstring& txtIns2 = std::wstring(), int nIns2pos = 0)
{
	size_t symbolCount = 0;	// Count all symbols to be replaced in source text.
	size_t pos = text.find_first_of(L"<>");
	while (pos != std::string::npos)
	{
		++symbolCount;
		pos = text.find_first_of(L"<>", ++pos);
	}

	// Allocate string big enough for source text + 3 times symbols found + length of insert strings + 0 terminator.
	size_t newLen = text.size() + (symbolCount * 3) + txtIns1.size() + txtIns2.size();
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[newLen + 1]);
	wchar_t* pOut = buffer.get();
	const wchar_t* pMax = pOut + newLen;
	const wchar_t* pIn = text.c_str();
	const wchar_t* pIns1 = txtIns1.c_str();
	const wchar_t* pIns2 = txtIns2.c_str();
	wchar_t c;
	for (int i = 0; i < (int)text.size(); i++)
	{
		if (i == nIns1pos && txtIns1.size())
		{
			while (*pIns1)
			{
				*pOut++ = *pIns1++;
				if (pOut >= pMax) { break; }
			}
		}
		if (i == nIns2pos && txtIns2.size())
		{
			while (*pIns2)
			{
				*pOut++ = *pIns2++;
				if (pOut >= pMax) { break; }
			}
		}
		if (pOut >= pMax) { break; }
		c = *pIn++;
		if (c == L'<' || c == L'>')
		{
			*pOut++ = L'&';
			*pOut++ = c == L'<' ? L'l' : L'g';
			*pOut++ = L't';
			*pOut++ = L';';
		}
		else
		{
			*pOut++ = c;
		}
	}
	*pOut = 0;
	std::wstring strOut = buffer.get();
	return strOut;
}
#pragma warning (default:4996)

export inline bool IsMouseMessage(UINT message)
{
	return (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
			|| (message >= WM_NCMOUSEMOVE && message <= WM_NCXBUTTONDBLCLK);
}

export inline bool IsKeyboardMessage(UINT message)
{
	return (message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP
			|| message == WM_SYSKEYUP || message == WM_CHAR);
}

export uint32_t GetRandomU32()
{
	std::random_device seed;
	std::mt19937 gen{ seed() }; // seed the generator
	std::uniform_int_distribution<> dist{ 1001, 2147483647 }; // set min and max
	return (uint32_t)dist(gen); // generate number
}

export std::string GetWindowClassName(HWND hWnd)
{
	int max_len = 256;
	std::unique_ptr<char[]> buffer(new char[max_len]);
	int nLen = ::GetClassNameA(hWnd, buffer.get(), max_len);
	return std::string(std::string_view(buffer.get(), nLen));
}

export std::wstring GetWindowClassNameW(HWND hWnd)
{
	int max_len = 256;
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[max_len]);
	int nLen = ::GetClassNameW(hWnd, buffer.get(), max_len);
	return std::wstring(std::wstring_view(buffer.get(), nLen));
}

export std::string GetWindowTextStd(HWND hWnd)
{
	int txtLen = GetWindowTextLengthA(hWnd);
	if (txtLen)
	{
		std::unique_ptr<char[]> buffer(new char[txtLen + 1]);
		GetWindowTextA(hWnd, buffer.get(), txtLen + 1);
		return std::string(std::string_view(buffer.get(), txtLen));
	}
	return std::string();
}

export std::wstring GetWindowTextStdW(HWND hWnd)
{
	int txtLen = GetWindowTextLengthW(hWnd);
	if (txtLen)
	{
		std::unique_ptr<wchar_t[]> buffer(new wchar_t[txtLen + 1]);
		GetWindowTextW(hWnd, buffer.get(), txtLen + 1);
		return std::wstring(std::wstring_view(buffer.get(), txtLen));
	}
	return std::wstring();
}

export std::wstring GetComboBoxLBtext(HWND hWnd, int idx)
{
	if (idx < 0) { return std::wstring(); }
	LRESULT len = SendMessage(hWnd, CB_GETLBTEXTLEN, (WPARAM)idx, 0);
	if (len != CB_ERR)
	{
		std::unique_ptr<wchar_t[]> buffer(new wchar_t[len + 1]);
		LRESULT res = SendMessage(hWnd, CB_GETLBTEXT, (WPARAM)idx, (LPARAM)buffer.get());
		if (res != CB_ERR)
		{
			return std::wstring(std::wstring_view(buffer.get(), len));
		}
	}
	return std::wstring();
}

export std::string GetMenuStringStd(HMENU hMenu, UINT uId, bool isByPosition)
{
	MENUITEMINFOA nfo = { 0 };
	nfo.cbSize = sizeof(MENUITEMINFOA);
	nfo.fMask = MIIM_TYPE;
	nfo.fType = MFT_STRING;
	if (GetMenuItemInfoA(hMenu, uId, isByPosition, &nfo))
	{
		std::unique_ptr<char[]> buffer(new char[nfo.cch + 1]);
		nfo.dwTypeData = buffer.get();
		++nfo.cch;
		if (GetMenuItemInfoA(hMenu, uId, isByPosition, &nfo))
		{
			return std::string(std::string_view(buffer.get(), nfo.cch));
		}
	}
	return std::string();
}

export std::vector<HWND> GetChildWindowsInZorder(HWND hParentWnd)
{
	XeASSERT(hParentWnd);
	std::vector<HWND> children;
	HWND hWndChild = ::GetWindow(hParentWnd, GW_CHILD);	// get first child
	while (hWndChild)
	{
		children.push_back(hWndChild);
		hWndChild = ::GetWindow(hWndChild, GW_HWNDNEXT); // get next child control
	}
	return children;
}

export bool IsWindowVisibleAndEnabledAndHasTabStop(HWND hWnd)
{
	if (::IsWindowEnabled(hWnd) && ::IsWindowVisible(hWnd))
	{
		DWORD dwStyle = ::GetWindowLong(hWnd, GWL_STYLE);
		if (dwStyle & WS_TABSTOP)
		{
			return true;
		}
	}
	return false;
}

bool _SetFocusIfWindowIsDlgChildAndVisibleAndEnabledAndHasTabStop(HWND hParentDlg, HWND hWndCtrl)
{
	if (hWndCtrl)
	{
		HWND hParent = ::GetParent(hWndCtrl);
		if (hParent == hParentDlg	// Is direct child of dialog window?
			&& IsWindowVisibleAndEnabledAndHasTabStop(hWndCtrl))
		{
			::SetFocus(hWndCtrl);
			return true;	// don't process the keyboard message.
		}
	}
	return false;
}

// Find next (or prev) control from hWndStart and set focus to it.
// hParentDlg is handle to the dialog window.
// hWndStart is handle to the control that currently has focus (can be the dialog itself).
// isReverseDirection is true if setting focus to previous control is wanted, false when next control.
// Note - the control that receives the focus must be a direct child of hParentDlg
// and be enabled and visible and have the WS_TABSTOP style.
// Return true if a (control) window was found and focus was set to it.
export bool DoTabToNextCtrl(bool isReverseDirection, HWND hParentDlg, HWND hWndStart)
{
	HWND hWnd = hWndStart;
	HWND hParent = ::GetParent(hWnd);
	if (hParentDlg == hWnd)	// Is message for the dialog window itself?
	{
		hWnd = ::GetWindow(hWnd, GW_CHILD);	// Get first child of dialog window.
		if (IsWindowVisibleAndEnabledAndHasTabStop(hWnd))
		{
			::SetFocus(hWnd);
			return true;	// don't process the keyboard message.
		}
	}
	else if (hParent != hParentDlg)	// NOT a direct child of dialog window?
	{
		do	// Search up the tree - until a direct child of dialog window is found.
		{
			hWnd = ::GetParent(hWnd);	// Get parent window.
			hParent = ::GetParent(hWnd);
		} while (hWnd && hParent != hParentDlg);	// Find direct child of dialog window.
	}
	UINT findDirection = isReverseDirection ? GW_HWNDPREV : GW_HWNDNEXT;
	while (hWnd)
	{
		hWnd = ::GetWindow(hWnd, findDirection);
		if (_SetFocusIfWindowIsDlgChildAndVisibleAndEnabledAndHasTabStop(hParentDlg, hWnd))
		{
			return true;	// Control to take focus was found.
		}
	}
	// No control was found that can take focus.
	// Find first or last control that can take focus.
	std::vector<HWND> children = GetChildWindowsInZorder(hParentDlg);
	if (isReverseDirection)
	{
		for (auto it = children.rbegin(); it != children.rend(); ++it)
		{
			if (_SetFocusIfWindowIsDlgChildAndVisibleAndEnabledAndHasTabStop(hParentDlg, *it))
			{
				return true;	// Control to take focus was found.
			}
		}
	}
	else
	{
		for (HWND hWndCtrl : children)
		{
			if (_SetFocusIfWindowIsDlgChildAndVisibleAndEnabledAndHasTabStop(hParentDlg, hWndCtrl))
			{
				return true;	// Control to take focus was found.
			}
		}
	}
	return false;	// No control was found that can take focus.
}

export UINT GetMouseMsgFlags()
{
	UINT uFlags = 0;
	uFlags |= (::GetAsyncKeyState(VK_CONTROL) & 0x8000) ? MK_CONTROL : 0;
	uFlags |= (::GetAsyncKeyState(VK_SHIFT) & 0x8000) ? MK_SHIFT : 0;
	uFlags |= (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? MK_LBUTTON : 0;
	uFlags |= (::GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? MK_MBUTTON : 0;
	uFlags |= (::GetAsyncKeyState(VK_MBUTTON) & 0x8000) ? MK_RBUTTON : 0;
	return uFlags;
}

export class CXeShiftCtrlAltKeyHelper
{
public:
	bool m_isCtrlKeyDown, m_isShiftKeyDown, m_isMenuKeyDown;

	CXeShiftCtrlAltKeyHelper()
	{
		m_isCtrlKeyDown =  (::GetKeyState(VK_CONTROL) & 0x8000) ? true : false;
		m_isShiftKeyDown = (::GetKeyState(VK_SHIFT)   & 0x8000) ? true : false;
		m_isMenuKeyDown  = (::GetKeyState(VK_MENU)    & 0x8000) ? true : false;
	}

	bool IsNoneDown() const { return !m_isCtrlKeyDown && !m_isShiftKeyDown && !m_isMenuKeyDown; }
	bool IsOnlyShiftDown() const { return !m_isCtrlKeyDown && m_isShiftKeyDown && !m_isMenuKeyDown; }
	bool IsOnlyCtrlDown() const { return m_isCtrlKeyDown && !m_isShiftKeyDown && !m_isMenuKeyDown; }
	bool IsOnlyAltDown() const { return !m_isCtrlKeyDown && !m_isShiftKeyDown && m_isMenuKeyDown; }
	bool IsOnlyShiftCtrlDown() const { return m_isCtrlKeyDown && m_isShiftKeyDown && !m_isMenuKeyDown; }
	bool IsOnlyShiftAltDown() const { return !m_isCtrlKeyDown && m_isShiftKeyDown && m_isMenuKeyDown; }
	bool IsOnlyCtrlAltDown() const { return m_isCtrlKeyDown && !m_isShiftKeyDown && m_isMenuKeyDown; }
	bool IsShiftCtrlAltDown() const { return m_isCtrlKeyDown && m_isShiftKeyDown && m_isMenuKeyDown; }
	bool IsShiftOrCtrlOrAltDown() const { return m_isCtrlKeyDown || m_isShiftKeyDown || m_isMenuKeyDown; }
};

inline uint8_t _ConvHexChar(char c, bool& isValid)
{
	uint8_t u = 0xFF;
	if (c >= '0' && c <= '9')
	{
		u = c - '0';
	}
	else if (c >= 'a' && c <= 'f')
	{
		u = c - 'a' + 10;
	}
	else if (c >= 'A' && c <= 'F')
	{
		u = c - 'A' + 10;
	}
	isValid = u != 0xFF;
	return isValid ? u : 0;
}

export class CXeBuffer
{
public:
	std::unique_ptr<uint8_t[]>	m_buffer;
	size_t						m_buffer_size = 0;

	CXeBuffer() = default;
	CXeBuffer(const void* pData, size_t data_len)
	{
		_Alloc(data_len);
		std::memcpy(m_buffer.get(), pData, data_len);
	}
	CXeBuffer(size_t data_len)
	{
		_Alloc(data_len, true);
	}

	// Input string is assumed to contain ONLY hex characters '0'...'9', 'a'...'f', 'A'...'F'.
	// If pIsValid parameter is supplied - it is set to true if all characters of the string could be parsed,
	// it is set to false if any character from the string is not a valid hex character.
	static CXeBuffer FromHexString(const std::wstring& ws, bool* pIsValid = nullptr)
	{
		bool isV, isHiOk, isLoOk;
		if (!pIsValid)
		{
			pIsValid = &isV;
		}
		std::string s = xet::to_astr(ws);
		CXeBuffer b(s.size() / 2);
		const char* pC = s.c_str();
		uint8_t* pD = b.get();
		for (size_t i = 0; i < b.size(); ++i)
		{
			uint8_t u = (_ConvHexChar(*pC++, isHiOk) << 4) + _ConvHexChar(*pC++, isLoOk);
			*pIsValid = isHiOk && isLoOk;
			if (!*pIsValid)
			{
				break;
			}
			*pD++ = u;
		}
		return b;
	}

	void Alloc(size_t data_len)
	{
		_Alloc(data_len, true);
	}

	void Copy(const void* pData, size_t data_len)
	{
		XeASSERT(data_len <= m_buffer_size);
		if (data_len <= m_buffer_size)
		{
			std::memcpy(m_buffer.get(), pData, data_len);
		}
	}

	void AllocAndCopy(const void* pData, size_t data_len)
	{
		_Alloc(data_len);
		std::memcpy(m_buffer.get(), pData, data_len);
	}

	uint8_t* get() { return m_buffer.get(); }

	size_t size() { return m_buffer_size; }

	// Return buffer contents as Hex string.
	std::wstring ToHexString()
	{
		std::string h(m_buffer_size * 2, '\0');
		char* pH = &h[0];
		const uint8_t* pT = m_buffer.get();
		for (size_t i = 0; i < m_buffer_size; ++i)
		{
			sprintf_s(pH + i * 2, h.size() - i * 2 + 1, "%02X", *(pT + i));
		}
		return xet::to_wstr(h);
	}

protected:
	void _Alloc(size_t len, bool isSetZero = false)
	{
		m_buffer.reset();
		m_buffer_size = len;
		m_buffer = std::make_unique<uint8_t[]>(m_buffer_size);
		if (isSetZero)
		{
			std::memset(m_buffer.get(), 0, m_buffer_size);
		}
	}
};

export CXeBuffer GetResource(const wchar_t* name, const wchar_t* type)
{
	DWORD dwSize = 0;
	HRSRC hResource = 0;
	void* pResourceData = nullptr;
	HGLOBAL hgmemRes = 0;
	void* pBuffer = nullptr;

	if ((hResource = ::FindResourceW(0, name, type))
		&& (dwSize = ::SizeofResource(0, hResource))
		&& (pResourceData = ::LockResource(::LoadResource(0, hResource)))
		&& (hgmemRes = ::GlobalAlloc(GMEM_MOVEABLE, dwSize))
		&& (pBuffer = ::GlobalLock(hgmemRes)))
	{
		CXeBuffer xeBuffer(pResourceData, dwSize);
		if (pBuffer)
			::GlobalUnlock(hgmemRes);
		if (hgmemRes)
			::GlobalFree(hgmemRes);
		return xeBuffer;
	}
	else
	{
		XeASSERT(false);
		std::string str = "Load resource '" + xet::to_astr(name) + "' failed.\n";
		XeTRACE(str.c_str());
	}
	return CXeBuffer(0, 0);
}

export class CXeMenuItemInfo
{
public:
	WORD		m_wOptFlags = 0;
	WORD		m_wID = 0;
	bool		m_hasID = false;
	std::wstring	m_text;
	bool		m_hasString = false;

	inline bool IsPopUp() const { return m_wOptFlags & MF_POPUP; }
	inline bool IsChecked() const { return m_wOptFlags & MF_CHECKED; }
	inline bool IsEnabled() const { return !(m_wOptFlags & MF_DISABLED); }

	inline bool IsSeparator() const { return m_wOptFlags == 0 && m_hasID && m_wID == 0 && !m_hasString; }

	inline bool IsLastItemInSubMenu() const { return m_wOptFlags & 0x0080; }

	void Reset()
	{
		m_wOptFlags = m_wID = 0;
		m_text.clear();
		m_hasID = m_hasString = false;
	}

	// Returns menu item length in pData buffer or 0 if not enough data to parse or other error.
	size_t ParseMenuItem(const uint8_t* pData, size_t data_len)
	{
		Reset();
		if (data_len < 4) { return 0; }
		size_t item_len = 2;
		m_wOptFlags = *((WORD*)pData);
		if (!IsPopUp())
		{
			m_wID = *((WORD*)(pData + 2));
			m_hasID = true;
			item_len += 2;
		}
		if (data_len < 6) { return item_len; }
		LPCWCH pwDataEnd = (LPCWCH)(pData + data_len);
		LPCWCH pwStr = IsPopUp() ? (LPCWCH)(pData + 2) : (LPCWCH)(pData + 4);
		LPCWCH pwStrEnd = pwStr;
		while (pwStrEnd < pwDataEnd && *pwStrEnd != 0)
		{
			++pwStrEnd;
		}
		size_t w_str_len = pwStrEnd - pwStr;
		if (w_str_len)
		{
			m_text = std::wstring_view(pwStr, w_str_len);
			m_hasString = true;
		}
		return item_len + (w_str_len * 2) + 2 /* ZERO string terminator */;
	}
};


// Scoped lock for file container (or any other class that implements AcquireLock and ReleaseLock).
export template<class T>
 class CIF_lock
{
protected:
	T* m_ptr;
public:
	CIF_lock(T* pIF) : m_ptr(pIF) { _ASSERT(m_ptr);  m_ptr->AcquireLock(); }
	~CIF_lock() { m_ptr->ReleaseLock(); }
};

#pragma pack(push)
#pragma pack(1)
struct TextAndLength
{
	uint16_t m_len;
	uint8_t m_text[512];
	uint8_t m_padding[14];

	void SetText(const std::wstring& s)
	{
		std::string su8 = xet::toUTF8(s);
		m_len = su8.size() > 512 ? 512 : (uint16_t)su8.size();
		std::memcpy(m_text, su8.c_str(), m_len);
	}

	std::wstring GetText()
	{
		std::wstring ws;
		if (m_len > 0 && m_len < 512)
		{
			std::string s((char*)m_text, (size_t)m_len);
			ws = xet::fromUTF8toWStr(s);
		}
		return ws;
	}
};
static_assert(sizeof(TextAndLength) == 528);
#pragma pack(pop)

#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)

#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)

export class CXeCrypto
{
protected:
	VSRL::Logger& logger() { return VSRL::s_pVSRL->GetInstance("CXeCrypto"); }

	bool m_isInitialized = false;
	BCRYPT_ALG_HANDLE m_hAesAlg = NULL;
	BCRYPT_KEY_HANDLE m_hKey = NULL;

	CXeBuffer m_buf_Key_in, m_buf_iv_in;

	CXeBuffer m_buf_key, m_buf_iv;
	CXeBuffer m_buf_data;

public:
	CXeCrypto(const uint8_t* pKey, size_t key_size, const uint8_t* pIV, size_t size_iv)
	{
		m_buf_Key_in.AllocAndCopy(pKey, key_size);
		m_buf_iv_in.AllocAndCopy(pIV, size_iv);
	}
	~CXeCrypto()
	{
		if (m_hAesAlg)
		{
			BCryptCloseAlgorithmProvider(m_hAesAlg, 0);
		}
		if (m_hKey)
		{
			BCryptDestroyKey(m_hKey);
		}
	}

	// Encrypted data will be in data buffer efter encryption.
	bool Encrypt(const void* pData, size_t data_len)
	{
		NTSTATUS status = STATUS_UNSUCCESSFUL;
		if (!m_isInitialized)
		{
			if (!_Initialize())
			{
				return false;
			}
		}

		// Refresh the IV
		m_buf_iv.Copy(m_buf_iv_in.get(), m_buf_iv_in.size());

		// Get the output buffer size.
		DWORD cbCipherText = 0;
		if (!NT_SUCCESS(status = BCryptEncrypt(m_hKey, (uint8_t*)pData, (ULONG)data_len, NULL, m_buf_iv.get(), (ULONG)m_buf_iv.size(),
			NULL, 0, &cbCipherText, BCRYPT_BLOCK_PADDING)))
		{
			logger().error("Encrypt failed (get buffer size needed). Error: 0x%08X - %s", status, GetNTStatusErrorDesc(status).c_str());
			return false;
		}

		m_buf_data.Alloc(cbCipherText);

		// Use the key to encrypt the plaintext buffer.
		// For block sized messages, block padding will add an extra block.
		ULONG cbResult = 0;
		if (!NT_SUCCESS(status = BCryptEncrypt(m_hKey, (uint8_t*)pData, (ULONG)data_len, NULL, m_buf_iv.get(), (ULONG)m_buf_iv.size(),
			m_buf_data.get(), (ULONG)m_buf_data.size(), &cbResult, BCRYPT_BLOCK_PADDING)))
		{
			logger().error("Encrypt failed. Error: 0x%08X - %s", status, GetNTStatusErrorDesc(status).c_str());
			return false;
		}
		return true;
	}

	// Decrypted data will be in data buffer efter decryption.
	bool Decrypt(const void* pData, size_t data_len)
	{
		NTSTATUS status = STATUS_UNSUCCESSFUL;
		if (!m_isInitialized)
		{
			if (!_Initialize())
			{
				return false;
			}
		}

		// Refresh the IV
		m_buf_iv.Copy(m_buf_iv_in.get(), m_buf_iv_in.size());

		// Get the output buffer size.
		DWORD cbPlainText = 0;
		if (!NT_SUCCESS(status = BCryptDecrypt(m_hKey, (uint8_t*)pData, (ULONG)data_len, NULL, m_buf_iv.get(), (ULONG)m_buf_iv.size(),
			NULL, 0, &cbPlainText, BCRYPT_BLOCK_PADDING)))
		{
			logger().error("Decrypt failed (get buffer size needed). Error: 0x%08X - %s", status, GetNTStatusErrorDesc(status).c_str());
			return false;
		}

		m_buf_data.Alloc(cbPlainText);

		// Use the key to encrypt the plaintext buffer.
		// For block sized messages, block padding will add an extra block.
		ULONG cbResult = 0;
		if (!NT_SUCCESS(status = BCryptDecrypt(m_hKey, (uint8_t*)pData, (ULONG)data_len, NULL, m_buf_iv.get(), (ULONG)m_buf_iv.size(),
			m_buf_data.get(), (ULONG)m_buf_data.size(), &cbResult, BCRYPT_BLOCK_PADDING)))
		{
			logger().error("Decrypt failed. Error: 0x%08X - %s", status, GetNTStatusErrorDesc(status).c_str());
			return false;
		}
		return true;
	}

	bool GenerateRandomBlock(uint8_t* pData, uint32_t data_len)
	{
		NTSTATUS status = STATUS_UNSUCCESSFUL;
		if (!NT_SUCCESS(status = BCryptGenRandom(0, pData, data_len, BCRYPT_USE_SYSTEM_PREFERRED_RNG)))
		{
			logger().error("GenerateRandomBlock failed. Error: 0x%08X - %s", status, GetNTStatusErrorDesc(status).c_str());
			return false;
		}
		return true;
	}

	uint8_t* GetDataBuffer() { return m_buf_data.get(); }
	size_t GetDataSize() { return m_buf_data.size(); }

	std::wstring EncryptTextField(const std::wstring& text)
	{
		TextAndLength tl;
		GenerateRandomBlock((uint8_t*)&tl, sizeof(TextAndLength));
		tl.SetText(text);
		if (Encrypt(&tl, sizeof(TextAndLength)))
		{
			return m_buf_data.ToHexString();
		}
		return L"";
	}

	std::wstring DecryptTextField(const std::wstring& encrypted_text_and_len)
	{
		CXeBuffer b = CXeBuffer::FromHexString(encrypted_text_and_len);
		if (Decrypt(b.get(), b.size()))
		{
			XeASSERT(GetDataSize() >= sizeof(TextAndLength));
			if (GetDataSize() >= sizeof(TextAndLength))
			{
				TextAndLength* pTL = (TextAndLength*)GetDataBuffer();
				return pTL->GetText();
			}
		}
		return L"";
	}

protected:
	bool _Initialize()
	{
		NTSTATUS status = STATUS_UNSUCCESSFUL;
		if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&m_hAesAlg, BCRYPT_AES_ALGORITHM, NULL, 0)))
		{
			logger().error("Get crypto provider failed. Error: 0x%08X - %s", status, GetNTStatusErrorDesc(status).c_str());
			return false;
		}

		// Calculate the size of the buffer to hold the KeyObject.
		DWORD cbKeyObject = 0, cbData = 0;
		if (!NT_SUCCESS(status = BCryptGetProperty(m_hAesAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbKeyObject,
			sizeof(DWORD), &cbData, 0)))
		{
			logger().error("Get crypto key size failed. Error: 0x%08X - %s", status, GetNTStatusErrorDesc(status).c_str());
			return false;
		}

		// Allocate the key object on the heap.
		m_buf_key.Alloc(cbKeyObject);

		// Calculate the block length for the IV.
		DWORD cbBlockLen = 0;
		if (!NT_SUCCESS(status = BCryptGetProperty(m_hAesAlg, BCRYPT_BLOCK_LENGTH, (PBYTE)&cbBlockLen,
			sizeof(DWORD), &cbData, 0)))
		{
			logger().error("Get crypto IV size failed. Error: 0x%08X - %s", status, GetNTStatusErrorDesc(status).c_str());
			return false;
		}

		// Determine whether the cbBlockLen is not longer than the IV length.
		if (cbBlockLen > m_buf_iv_in.size())
		{
			logger().error("Crypto IV size calculation failed.");
			return false;
		}

		// Allocate a buffer for the IV. The buffer is consumed during the encrypt/decrypt process.
		m_buf_iv.Alloc(cbBlockLen);
		m_buf_iv.Copy(m_buf_iv_in.get(), m_buf_iv_in.size());

		if (!NT_SUCCESS(status = BCryptSetProperty(m_hAesAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_CBC,
			sizeof(BCRYPT_CHAIN_MODE_CBC), 0)))
		{
			logger().error("Set crypto mode failed. Error: 0x%08X - %s", status, GetNTStatusErrorDesc(status).c_str());
			return false;
		}

		// Generate the key from supplied input key bytes.
		if (!NT_SUCCESS(status = BCryptGenerateSymmetricKey(m_hAesAlg, &m_hKey, m_buf_key.get(), (ULONG)m_buf_key.size(),
			(PBYTE)m_buf_Key_in.get(), (ULONG)m_buf_Key_in.size(), 0)))
		{
			logger().error("Generate crypto key failed. Error: 0x%08X - %s", status, GetNTStatusErrorDesc(status).c_str());
			return false;
		}
		m_isInitialized = true;
		return true;
	}
};

// Source: MFC C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\atlmfc\src\mfc\wingdix.cpp
// CDC::GetHalftoneBrush()
HBRUSH CreateHalftoneBrush()
{
	HBRUSH hBr = NULL;
	WORD grayPattern[8];
	for (int i = 0; i < 8; i++)
	{
		grayPattern[i] = (WORD)(0x5555 << (i & 1));
	}
	HBITMAP grayBitmap = ::CreateBitmap(8, 8, 1, 1, grayPattern);
	if (grayBitmap != NULL)
	{
		hBr = ::CreatePatternBrush(grayBitmap);
		::DeleteObject(grayBitmap);
	}
	return hBr;
}

export void DrawSplitBarTracker(HWND hWnd, const RECT& rcTracker)
{
	//ASSERT(!rcTracker.IsRectEmpty());
	//ASSERT((pWnd->GetStyle() & WS_CLIPCHILDREN) == 0);	// pat-blt without clip children on

	//CDC* pDC = pWnd->GetDC();
	//CBrush* pBrush = CDC::GetHalftoneBrush();
	HDC hDC = ::GetWindowDC(hWnd);
	HBRUSH hHalfToneBr = CreateHalftoneBrush();
	if (hHalfToneBr != NULL)
	{
		HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDC, hHalfToneBr);
		// invert the brush pattern
		int cx = rcTracker.right - rcTracker.left, cy = rcTracker.bottom - rcTracker.top;
		::PatBlt(hDC, rcTracker.left, rcTracker.top, cx, cy, PATINVERT);
		if (hOldBrush != NULL)
		{
			::SelectObject(hDC, hOldBrush);
		}
		::DeleteObject(hHalfToneBr);
	}
	//pWnd->ReleaseDC(pDC);
	::ReleaseDC(hWnd, hDC);
}

// How a control is "glued" in the dialog.
export enum class Glue
{
	Bottom,				// Keep control in constant distance from dialog bottom edge.
	Right,				// Keep control in constant distance from dialog right edge.
	BottomRight,		// Keep control in constant distance from dialog bottom, right edge.
	HsizeRight,			// Size control horizontally such that right edge is constant distance from dialog right edge.
	VsizeBottom,		// Size control vertically such that bottom edge is constant distance from dialog bottom edge.
	HVsizeBottomRight	// Size control such that bottom, right edge is constant distance from dialog bottom, right edge.
};

export struct GlueCtrl
{
	UINT m_IDC = 0;
	Glue m_glue;

	int m_cxRight;	// control right edge distance from dialog right edge.
	int m_cyBottom;	// control bottom edge distance from dialog bottom edge.

	GlueCtrl() = default;
	GlueCtrl(UINT idc, Glue glue) : m_IDC(idc), m_glue(glue) {}
};

// Helper - manage controls size and position in a dialog box.
export class XeGlueControls
{
	HWND m_hWndOwner = 0;
	std::vector<GlueCtrl> m_glue_controls;

public:
	void InitializeGlueControls(HWND hWnd, const std::vector<GlueCtrl>& glue_controls)
	{
		m_hWndOwner = hWnd;
		m_glue_controls = glue_controls;
		CRect rectCli, rect;
		XeASSERT(m_hWndOwner);
		::GetClientRect(m_hWndOwner, rectCli);

		for (GlueCtrl& gk : m_glue_controls)
		{
			HWND hCtrl = ::GetDlgItem(m_hWndOwner, gk.m_IDC);
			XeASSERT(hCtrl);
			if (hCtrl)
			{
				::GetWindowRect(hCtrl, rect);
				::ScreenToClient(m_hWndOwner, (LPPOINT)&rect);
				::ScreenToClient(m_hWndOwner, ((LPPOINT)&rect) + 1);
				// control right edge distance from dialog right edge.
				gk.m_cxRight = rectCli.right - rect.right;
				// control bottom edge distance from dialog bottom edge.
				gk.m_cyBottom = rectCli.bottom - rect.bottom;
			}
		}
	}

	const GlueCtrl* GetGlueCtrlStruct(UINT uIDC)
	{
		for (GlueCtrl& gk : m_glue_controls)
		{
			if (gk.m_IDC == uIDC)
			{
				return &gk;
			}
		}
		return nullptr;
	}

	// cx = dialog client area width, cy = dialog client area height.
	void MoveGlueControlsOnSize(int cx, int cy)
	{
		CRect rect;
		for (const GlueCtrl& gk : m_glue_controls)
		{
			HWND hCtrl = ::GetDlgItem(m_hWndOwner, gk.m_IDC);
			if (!hCtrl) { continue; }
			::GetWindowRect(hCtrl, rect);
			::ScreenToClient(m_hWndOwner, (LPPOINT)&rect);
			::ScreenToClient(m_hWndOwner, ((LPPOINT)&rect) + 1);

			int y_btm = cy - gk.m_cyBottom;	// control new bottom edge.
			int x_rth = cx - gk.m_cxRight;	// control new right edge.
			switch (gk.m_glue)
			{
				// Keep control in constant distance from dialog bottom edge.
			case Glue::Bottom:
				rect.OffsetRect(0, y_btm - rect.bottom);
				break;

				// Keep control in constant distance from dialog right edge.
			case Glue::Right:
				rect.OffsetRect(x_rth - rect.right, 0);
				break;

				// Keep control in constant distance from dialog bottom, right edge.
			case Glue::BottomRight:
				rect.OffsetRect(x_rth - rect.right, y_btm - rect.bottom);
				break;

				// Size control horizontally such that right edge is constant distance from dialog right edge.
			case Glue::HsizeRight:
				rect.right = x_rth;
				break;

				// Size control vertically such that bottom edge is constant distance from dialog bottom edge.
			case Glue::VsizeBottom:
				rect.bottom = y_btm;
				break;

				// Size control such that bottom, right edge is constant distance from dialog bottom, right edge.
			case Glue::HVsizeBottomRight:
				rect.bottom = y_btm;
				rect.right = x_rth;
				break;
			}
			::SetWindowPos(hCtrl, nullptr, rect.left, rect.top, rect.Width(), rect.Height(),
				SWP_NOZORDER | SWP_NOCOPYBITS);
		}
	}
};

