module;

#include "os_minimal.h"
#include <Knownfolders.h>
#include <Shlobj.h>
#include <shobjidl.h> 
#include <atlbase.h>
#include <algorithm>
#include <chrono>
#include <cwctype>
#include <map>
#include "logging.h"
#include "XeAssert.h"
#pragma comment(lib, "shell32.lib")

export module Xe.FileHelpers;

export import Xe.StringTools;
import Xe.StringTools;

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif

VSRL::Logger& logger_filehelper() { return VSRL::s_pVSRL->GetInstance("FileHelpers"); }

#pragma region ErrorHelpers
// Get system error (:GetLastError()) description.
// Returns string from ::FormatMessage(...) - <crlf> are removed from string end.
export std::string GetSystemErrorDesc(DWORD dwSystemErrorCode)
{
	LPVOID lpMsgBuf = 0;
	::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dwSystemErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);

	std::string strMsg;
	if (lpMsgBuf)
	{
		strMsg = (char*)lpMsgBuf;
		::LocalFree(lpMsgBuf);
	}
	else
	{
		strMsg = "Unknown error code: " + std::to_string(dwSystemErrorCode);
	}

	xet::trim_trailing_crlf(strMsg);	// remove CRLF
	return strMsg;
}
export std::wstring GetSystemErrorDescW(DWORD dwSystemErrorCode)
{
	LPVOID lpMsgBuf = 0;
	::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dwSystemErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(wchar_t*)&lpMsgBuf, 0, NULL);

	std::wstring strMsg;
	if (lpMsgBuf)
	{
		strMsg = (wchar_t*)lpMsgBuf;
		::LocalFree(lpMsgBuf);
	}
	else
	{
		strMsg = L"Unknown error code: " + std::to_wstring(dwSystemErrorCode);
	}

	xet::trim_trailing_crlf(strMsg);	// remove CRLF
	return strMsg;
}

export std::string GetNTStatusErrorDesc(DWORD nt_status)
{
	HMODULE hNTDll = ::LoadLibraryW(L"NTDLL.DLL");

	LPVOID lpMsgBuf = 0;
	::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
			hNTDll, nt_status, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&lpMsgBuf, 0, NULL);
	::FreeLibrary(hNTDll);

	std::string strMsg;
	if (lpMsgBuf)
	{
		strMsg = (char*)lpMsgBuf;
		::LocalFree(lpMsgBuf);
	}
	else
	{
		strMsg = "Unknown error code: " + std::to_string(nt_status);
	}
	xet::trim_trailing_crlf(strMsg);	// remove CRLF

	return strMsg;
}
export std::wstring GetNTStatusErrorDescW(DWORD nt_status)
{
	HMODULE hNTDll = ::LoadLibraryW(L"NTDLL.DLL");

	LPVOID lpMsgBuf = 0;
	::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
			hNTDll, nt_status, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (wchar_t*)&lpMsgBuf, 0, NULL);
	::FreeLibrary(hNTDll);

	std::wstring strMsg;
	if (lpMsgBuf)
	{
		strMsg = (wchar_t*)lpMsgBuf;
		::LocalFree(lpMsgBuf);
	}
	else
	{
		strMsg = L"Unknown error code: " + std::to_wstring(nt_status);
	}
	xet::trim_trailing_crlf(strMsg);	// remove CRLF

	return strMsg;
}
#pragma endregion ErrorHelpers

#pragma region FileNameHelpers
std::tuple<std::string::size_type, bool> _FindLastSlash(const std::wstring& pathName)
{
	auto p1a = pathName.rfind(L'\\');
	bool p1a_valid = p1a != std::string::npos;
	auto p1b = pathName.rfind(L'/');
	bool p1b_valid = p1b != std::string::npos;
	if (p1a_valid && p1b_valid)
	{
		auto p1 = p1a > p1b ? p1a : p1b;
		return std::tuple<std::string::size_type, bool>(p1, true);
	}
	else if (p1a_valid)
	{
		return std::tuple<std::string::size_type, bool>(p1a, true);
	}
	else if (p1b_valid)
	{
		return std::tuple<std::string::size_type, bool>(p1b, true);
	}
	return std::tuple<std::string::size_type, bool>(std::string::npos, false);
}

std::tuple<std::string::size_type, bool> _FindLastDot(const std::wstring& pathName)
{
	auto p2 = pathName.rfind(L'.');
	bool p2_valid = p2 != std::string::npos;
	return std::tuple<std::string::size_type, bool>(p2, p2_valid);
}

export std::wstring GetFilenameWithoutExt(const std::wstring& pathName)
{
	auto [p1, p1_valid] = _FindLastSlash(pathName);
	auto [p2, p2_valid] = _FindLastDot(pathName);
	if (p1_valid && !p2_valid)
	{
		return pathName.substr(p1 + 1);
	}
	else if (!p1_valid && p2_valid)
	{
		return pathName.substr(0, p2);
	}
	else if (p1_valid && p2_valid)
	{
		return pathName.substr(p1 + 1, (p2 - p1) - 1);
	}
	return pathName;
}

export std::wstring GetDirectory(const std::wstring& pathName)
{
	auto [p1, p1_valid] = _FindLastSlash(pathName);
	return p1_valid ? pathName.substr(0, p1) : pathName;
}

export std::wstring GetFilenameWithExt(const std::wstring& pathName)
{
	auto [p1, p1_valid] = _FindLastSlash(pathName);
	return p1_valid ? pathName.substr(p1 + 1) : pathName;
}

export std::wstring GetFileExtension(const std::wstring& pathName)
{
	auto [p2, p2_valid] = _FindLastDot(pathName);
	return p2_valid ? pathName.substr(p2 + 1) : L"";
}

export std::wstring GetPathNameWithoutExtension(const std::wstring& pathName)
{
	auto [p2, p2_valid] = _FindLastDot(pathName);
	return p2_valid ? pathName.substr(0, p2) : pathName;
}

export bool IsNumericalFileExtension(const std::wstring& pathname)
{
	std::wstring fileExt = GetFileExtension(pathname);
	for (int i = 0; i < fileExt.size(); i++)
	{
		if (fileExt[i] < L'0' || fileExt[i] > L'9')
		{
			return false;
		}
	}
	return fileExt.size() ? true : false;
}

export bool IsFileExtension(const std::wstring& extension, const std::wstring& pathname)
{
	std::wstring fileExt = GetFileExtension(pathname);
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(),
		[](wchar_t c) { return std::towlower(c); });
	std::wstring extL = extension;
	std::transform(extL.begin(), extL.end(), extL.begin(),
		[](wchar_t c) { return std::towlower(c); });
	return extL == fileExt;
}

// Is file extension supported by GDI+ Bitmap load. 
export bool IsImageFileExtension(const std::wstring& pathname)
{
	std::wstring fileExt = GetFileExtension(pathname);
	return xet::compare_no_case(fileExt, L"jpg") == 0
		|| xet::compare_no_case(fileExt, L"png") == 0
		|| xet::compare_no_case(fileExt, L"gif") == 0
		|| xet::compare_no_case(fileExt, L"jpeg") == 0
		|| xet::compare_no_case(fileExt, L"tiff") == 0
		|| xet::compare_no_case(fileExt, L"jfif") == 0
		|| xet::compare_no_case(fileExt, L"bmp") == 0;
}

// Is file extension ".log" or ".log.xx" (where xx is a number).
export bool IsLogFileExtension(const std::wstring& pathname)
{
	std::wstring fileExt = GetFileExtension(pathname);
	if (xet::compare_no_case(fileExt, L"log") == 0)
	{
		return true;
	}
	if (IsNumericalFileExtension(pathname))
	{
		std::wstring pn = pathname.substr(0, pathname.size() - fileExt.size() - 1);
		fileExt = GetFileExtension(pn);
		return xet::compare_no_case(fileExt, L"log") == 0;
	}
	return false;
}

// If file extension ".log" or ".log.xx" (where xx is a number),
// then "log" or "log.xx" is returned - if extension is something else
// then that extension is returned or empty string if pathname has no extension.
export std::wstring GetLogFileExtension(const std::wstring& pathname)
{
	std::wstring fileExt = GetFileExtension(pathname);
	if (xet::compare_no_case(fileExt, L"log") == 0)
	{
		return fileExt;
	}
	if (IsNumericalFileExtension(pathname))
	{
		std::wstring pn = pathname.substr(0, pathname.size() - fileExt.size() - 1);
		std::wstring fileExt2 = GetFileExtension(pn);
		if (xet::compare_no_case(fileExt2, L"log") == 0)
		{
			std::wstring numExt = pathname.substr(pathname.size() - fileExt.size() - 4);
			return numExt;
		}
	}
	return L"";
}

export bool IsFile_LogFile(const std::wstring& pathname, const std::wstring& ext)
{
	auto parse = [&](const std::wstring& str, const std::wstring& fmt, auto& o)
		{
			std::wistringstream is{ str };
			//is.imbue(std::locale("en_US.utf-8"));
			is >> std::chrono::parse(fmt, o);
		};

	bool isLogFile = IsLogFileExtension(pathname);
	if (!isLogFile)
	{
		std::wstring fn = GetFilenameWithExt(pathname);

		// Detect if filename ends with "-YYYYmmdd" rolling linux log format.
		if (fn.size() > 8)
		{
			std::chrono::year_month_day ymd{};
			parse(fn.substr(fn.size() - 8), L"%Y%2m%2d", ymd);
			if (ymd.ok())
			{
				fn = fn.substr(0, fn.size() - 8);
				wchar_t last = fn[fn.size() - 1];
				if (last == L'-')
				{
					fn = fn.substr(0, fn.size() - 1);
				}
			}
		}

		// Is any of "known" Linux log files?
		if (fn == L"syslog" || fn == L"messages" || fn == L"debug" || fn == L"dmsg")
		{
			isLogFile = true;
		}
		else if (fn == L"lastlog" || fn == L"faillog")	// Linux binary files in the /var/log folder.
		{
			isLogFile = false;
		}
		else
		{	// We consider any filename extension that starts with "log" a log file.
			std::wstring ext = xet::to_lower(GetFileExtension(fn));
			if (ext.starts_with(L"log"))
			{
				isLogFile = true;
			}
		}
	}
	return isLogFile;
}
export bool IsFile_TextFile(const std::wstring& pathname, const std::wstring& ext)
{
	return xet::compare_no_case(ext, L"txt") == 0;
}
export bool IsFile_ImageFile(const std::wstring& pathname, const std::wstring& ext)
{
	return IsImageFileExtension(pathname);
}
export bool IsFile_ZipFile(const std::wstring& pathname, const std::wstring& ext)
{
	return xet::compare_no_case(ext, L"zip") == 0;
}
export bool IsFile_JsonFile(const std::wstring& pathname, const std::wstring& ext)
{
	return xet::compare_no_case(ext, L"json") == 0;
}

export bool IsValidFilename(const std::wstring& fn)
{
	std::wstring inv(L"\\/<>|\":?*");
	return !std::any_of(inv.cbegin(), inv.cend(), [&](wchar_t c) { return fn.find(c) != std::string::npos; });
}

export void RemovePathnamesWithExtension(std::vector<std::wstring>& pathnames, const std::wstring& ext)
{
	for (auto it = pathnames.begin(); it != pathnames.end(); )
	{
		std::wstring fileExt = GetFileExtension(*it);
		if (xet::compare_no_case(fileExt, ext) == 0)
		{
			it = pathnames.erase(it);
		}
		else
		{
			++it;
		}
	}
}

export void RemoveTrailingBackslash(std::wstring& str)
{
	if (str.size() > 0)
	{
		wchar_t last = str[str.size() - 1];
		if (last == L'\\' || last == L'/')
		{
			str.resize(str.size() - 1);
		}
	}
}
#pragma endregion FileNameHelpers

#pragma region KnownFolders
export KNOWNFOLDERID ParseKnownFolder(const std::wstring& strKF)
{
	std::wstring str = xet::to_upper(strKF);
	if (wcscmp(str.c_str(), L"%USERPROFILE%") == 0)
	{
		return FOLDERID_Profile;
	}
	if (wcscmp(str.c_str(), L"%PROGRAMDATA%") == 0 || wcscmp(str.c_str(), L"%ALLUSERSPROFILE%") == 0)
	{
		return FOLDERID_ProgramData;
	}
	if (wcscmp(str.c_str(), L"%PROGRAMFILES%") == 0)
	{
		return FOLDERID_ProgramFiles;
	}
	if (wcscmp(str.c_str(), L"%PUBLIC%") == 0)
	{
		return FOLDERID_Public;
	}
	if (wcscmp(str.c_str(), L"%WINDIR%") == 0)
	{
		return FOLDERID_Windows;
	}
	if (wcscmp(str.c_str(), L"%LOCALAPPDATA%") == 0)
	{
		return FOLDERID_LocalAppData;
	}
	KNOWNFOLDERID nf = { 0 };
	return nf;
}

export std::wstring GetKnownFolder(const std::wstring& strKFalias)
{
	constexpr size_t buf_len = MAX_PATH + 1;
	wchar_t buffer[buf_len];
	if (wcscmp(xet::to_upper(strKFalias).c_str(), L"%TEMP%") == 0)
	{
		GetTempPathW(buf_len, buffer);
		std::wstring str(buffer);
		RemoveTrailingBackslash(str);
		return str;
	}
	else
	{
		KNOWNFOLDERID refKFId = ParseKnownFolder(strKFalias);
		if (refKFId.Data1 != 0 && refKFId.Data2 != 0 && refKFId.Data3 != 0 && refKFId.Data4 != 0)
		{
			std::wstring strKF;
			PWSTR pwstrKF = 0;
			HRESULT hr = SHGetKnownFolderPath(refKFId, 0, NULL, &pwstrKF);
			if (hr == S_OK)
			{
				strKF = pwstrKF;
				CoTaskMemFree(pwstrKF);
				RemoveTrailingBackslash(strKF);
				return strKF;
			}
		}
	}
	return strKFalias;
}

export std::wstring GetKnownFolder(KNOWNFOLDERID refKFId)
{
	if (refKFId.Data1 != 0 && refKFId.Data2 != 0 && refKFId.Data3 != 0 && refKFId.Data4 != 0)
	{
		PWSTR pwstrKF = 0;
		HRESULT hr = SHGetKnownFolderPath(refKFId, 0, NULL, &pwstrKF);
		if (hr == S_OK)
		{
			std::wstring strKF = pwstrKF;
			CoTaskMemFree(pwstrKF);
			RemoveTrailingBackslash(strKF);
			return strKF;
		}
	}
	return std::wstring();
}

export std::wstring ParseKnownFolderPath(const std::wstring& str)
{
	if (str.size() > 2 && str[0] == L'%')
	{
		std::string::size_type n = str.find(L'%', 1);
		if (n != std::string::npos)
		{
			std::wstring strKFalias = str.substr(0, n + 1);
			return GetKnownFolder(strKFalias) + str.substr(n + 1);
		}
	}
	return str;
}

// Try to parse "known folder" from supplied pathname.
// E.g. if pathname = "C:\ProgramData\Dummy.log"
// Return value is: "%programdata%\Dummy.log"
// If no "known folder found - pathname is returned unchanged.
export std::wstring ReverseParseKnownFolderInPathname(const std::wstring& pathname)
{
	std::vector<std::wstring> knf = { L"%temp%", L"%userprofile%", L"%programdata%",
		L"%programfiles%", L"%public%", L"%windir%", L"%localappdata%" };
	std::wstring pn_lower = xet::to_lower(pathname);
	for (const std::wstring& alias : knf)
	{
		std::wstring kf = xet::to_lower(GetKnownFolder(alias));
		if (pn_lower.find(kf) == 0)
		{
			return alias + pathname.substr(kf.size());
		}
	}
	return pathname;
}
#pragma endregion KnownFolders

// Returns: "C:\Windows" on most systems.
export std::wstring GetWindowsDirectoryStd()
{
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[MAX_PATH]);
	UINT len = GetWindowsDirectoryW(buffer.get(), MAX_PATH);
	return std::wstring(std::wstring_view(buffer.get(), len));
}

export bool DirectoryExists(const std::wstring& strDirectoryPath, std::string* pErrMsg = nullptr)
{
	std::string errMsg;
	if (!pErrMsg) { pErrMsg = &errMsg; }
	DWORD dwAttrib = GetFileAttributesW(strDirectoryPath.c_str());
	bool isExistingDirectory = (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
	DWORD dwError = ERROR_SUCCESS;
	if (dwAttrib == INVALID_FILE_ATTRIBUTES)
	{
		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_FILE_NOT_FOUND)	// Some other error (that we need to report)?
		{
			*pErrMsg = "DirectoryExists call for '" + xet::to_astr(strDirectoryPath) + "' failed.\n"
					"Error: " + std::to_string(dwError) + " - " + GetSystemErrorDesc(dwError);
			logger_filehelper().error(*pErrMsg);
		}
	}
	else if (!isExistingDirectory)
	{
		*pErrMsg = "DirectoryExists call for '" + xet::to_astr(strDirectoryPath) + "' failed.\n"
				"File exists but is not a directory. File attributes=" + std::to_string(dwAttrib);
		logger_filehelper().error(*pErrMsg);
	}
	// Call this function with path to directory (pathname to a file is an error when the file exists).
	XeASSERT((dwAttrib == INVALID_FILE_ATTRIBUTES && dwError == ERROR_FILE_NOT_FOUND) || isExistingDirectory);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && isExistingDirectory);
}

export bool CreateDirectoryIfNotExists(const std::wstring& strDir, std::string* pErrMsg = nullptr)
{
	std::string errMsg;
	if (!pErrMsg) { pErrMsg = &errMsg; }
	bool isDirectoryExists = DirectoryExists(strDir, pErrMsg);
	if (pErrMsg->size())
	{
		return false;
	}
	if (!isDirectoryExists)
	{
		if (CreateDirectoryW(strDir.c_str(), NULL) == 0)
		{
			DWORD dwError = ::GetLastError();
			*pErrMsg = "Create directory: " + xet::to_astr(strDir) + " failed.\n"
					"Error: " + std::to_string(dwError) + " - " + GetSystemErrorDesc(dwError);
			logger_filehelper().error(*pErrMsg);
			return false;
		}
	}
	return true;
}

export bool FileExists(const wchar_t* strPathName)
{
	DWORD dwAttrib = GetFileAttributesW(strPathName);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

export bool FileExists(std::wstring strPathName)
{
	return FileExists(strPathName.c_str());
}

export uint64_t GetFileSizeU64(const std::wstring& pathname)
{
	uint64_t file_size = 0;
	HANDLE hFile = CreateFileW(pathname.c_str(), GENERIC_READ,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER li;
		if (GetFileSizeEx(hFile, &li))
		{
			file_size = li.QuadPart;
		}
		CloseHandle(hFile);
	}
	return file_size;
}

export bool ReadFilePart(const std::wstring& pathname, uint64_t offset, size_t len2read, char* pDest)
{
	bool result = false;
	HANDLE hFile = CreateFileW(pathname.c_str(), GENERIC_READ,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER li = {0};
		li.QuadPart = offset;
		if (SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN))
		{
			DWORD dwBytesRead = 0;
			if (ReadFile(hFile, pDest, (DWORD)len2read, &dwBytesRead, 0))
			{
				result = true;
			}
		}
		CloseHandle(hFile);
	}
	return result;
}

export bool AppendFile(const std::wstring& pathname, uint64_t offset, void* pData, size_t len2write)
{
	bool result = false;
	HANDLE hFile = CreateFileW(pathname.c_str(), GENERIC_READ | GENERIC_WRITE,
		0 /* file share none */, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER li = { 0 };
		li.QuadPart = offset;
		if (SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN))
		{
			DWORD dwBytesWritten = 0;
			if (WriteFile(hFile, pData, (DWORD)len2write, &dwBytesWritten, 0))
			{
				result = true;
			}
		}
		CloseHandle(hFile);
	}
	return result;
}

export std::wstring GetCurrentUserAppDataFolder(const std::wstring& app_name)
{
	std::wstring f(GetKnownFolder(FOLDERID_LocalAppData));
	f += L"\\" + app_name;
	CreateDirectoryIfNotExists(f);
	return f;
}

export std::wstring GetLocalAppDataFolder()
{
	return GetKnownFolder(FOLDERID_LocalAppData);
}

// If {pathname}.{first_ext} exists - rename to {pathname}.{first_ext+1}
// Rename log file {pathname} to {pathname}.{first_ext}
export bool RenameRollingLogFiles(const std::wstring& pathname, int first_ext)
{
	int ext = first_ext, last_ext = -1;
	while (true)
	{
		std::wstring pn = pathname + L"." + std::to_wstring(ext);
		if (FileExists(pn))
		{
			last_ext = ext;
		}
		else
		{
			break;
		}
		++ext;
	}
	while (last_ext >= first_ext)
	{
		std::wstring pn = pathname + L"." + std::to_wstring(last_ext);
		std::wstring new_pn = pathname + L"." + std::to_wstring(last_ext + 1);
		if (!MoveFileW(pn.c_str(), new_pn.c_str()))
		{
			std::string syserr = GetSystemErrorDesc(GetLastError());
			std::string error_string = "RenameRollingLogFiles failed for "
					+ xet::to_astr(pathname) + " - Error: " + syserr;
			logger_filehelper().error(error_string.c_str());
			return false;
		}
		--last_ext;
	}
	std::wstring new_pn = pathname + L"." + std::to_wstring(first_ext);
	if (!MoveFileW(pathname.c_str(), new_pn.c_str()))
	{
		std::string syserr = GetSystemErrorDesc(GetLastError());
		std::string error_string = "RenameRollingLogFiles failed for "
				+ xet::to_astr(pathname) + " - Error: " + syserr;
		logger_filehelper().error(error_string.c_str());
		return false;
	}
	return true;
}

// Delete rolling log files that have extension greater than last_ext
// Delete all log files {pathname}.{last_ext + n}
export void DeleteRollingLogFilesGreaterThan(const std::wstring& pathname, int last_ext)
{
	int ext = last_ext + 1;
	while (true)
	{
		std::wstring pn = pathname + L"." + std::to_wstring(ext);
		if (FileExists(pn))
		{
			DeleteFileW(pn.c_str());
		}
		else
		{
			break;
		}
		++ext;
	}
}

// Open and close and existing file for read/write with share=delete/read/write.
// Return true if open succeeded. Return false if file not found or write permissions not available.
export bool HasWritePermissionsToFile(const wchar_t* szPathName)
{
	bool hasWritePermissions = false;
	HANDLE hFile = CreateFileW(szPathName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		hasWritePermissions = true;	// Open existing file for read/write succeeded.
		CloseHandle(hFile);
	}
	return hasWritePermissions;
}

// Open and close and existing file for read/write with share=none.
// Return true if open succeeded. Return false if file not found or write permissions not available.
export bool HasExclusivePermissionsToFile(const wchar_t* szPathName)
{
	bool hasWritePermissions = false;
	HANDLE hFile = CreateFileW(szPathName, GENERIC_READ | GENERIC_WRITE,
		0 /* FILE_SHARE_none */, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		hasWritePermissions = true;	// Open existing file for read/write succeeded.
		CloseHandle(hFile);
	}
	return hasWritePermissions;
}

// Create file for read/write and then delete it.
// Return true if successfully created and deleted the file.
// Return false if permission not granted to create file or file already exists.
export bool CanCreateFile(const wchar_t* szPathName)
{
	bool canCreateFile = false;
	// Note - this call will fail if file already exists.
	HANDLE hTestFile = CreateFileW(szPathName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hTestFile != INVALID_HANDLE_VALUE)
	{
		canCreateFile = true;	// Create file succeeded.
		CloseHandle(hTestFile);
		DeleteFileW(szPathName);
	}
	return canCreateFile;
}

// Delete files specified by strPath and strSearch.
// E.g. strPath = "C:\Temp" and strSearch = "*.txt" delete all .txt files in C:\Temp
// Returns true if no error (or no files found) else false.
export bool DeleteFiles(const std::wstring& strPath, const std::wstring& strSearch)
{
	bool bResult = true;
	WIN32_FIND_DATAW findFileData;
	std::wstring strSearchPath = strPath + L"\\" + strSearch;
	HANDLE hFind = FindFirstFileW(strSearchPath.c_str(), &findFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				std::wstring strDelFilePath = strPath + L"\\" + findFileData.cFileName;
				if (!DeleteFileW(strDelFilePath.c_str()))
				{
					bResult = false;
				}
			}
		} while (FindNextFileW(hFind, &findFileData));
	}
	return bResult;
}

export bool Delete_File(const std::wstring& pathname)
{
	return DeleteFileW(pathname.c_str()) ? true : false;
}

export std::string GetSmallTextFile(const std::wstring& strPathName, bool* pIsSuccess = nullptr)
{
	std::string strResult;
	HANDLE hTxtFile = CreateFileW(strPathName.c_str(), GENERIC_READ,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hTxtFile == INVALID_HANDLE_VALUE)
	{
		if (pIsSuccess) { *pIsSuccess = false; }
	}
	else
	{
		LARGE_INTEGER nFileSize;
		GetFileSizeEx(hTxtFile, &nFileSize);

		DWORD uFileSize = nFileSize.LowPart;
		std::unique_ptr<char[]> buffer(new char[uFileSize + 1]);

		DWORD dwBytesRead;
		if (ReadFile(hTxtFile, buffer.get(), uFileSize, &dwBytesRead, 0))
		{
			std::string_view v(buffer.get(), uFileSize);
			strResult = v;
			if (pIsSuccess) { *pIsSuccess = true; }
		}
		else
		{
			if (pIsSuccess) { *pIsSuccess = false; }
		}
		CloseHandle(hTxtFile);
	}
	return strResult;
}

export std::wstring GetSmallTextFileW(const std::wstring& strPathName, bool* pIsSuccess = nullptr)
{
	std::wstring strResult;
	HANDLE hTxtFile = CreateFileW(strPathName.c_str(), GENERIC_READ,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hTxtFile == INVALID_HANDLE_VALUE)
	{
		if (pIsSuccess) { *pIsSuccess = false; }
	}
	else
	{
		LARGE_INTEGER nFileSize;
		GetFileSizeEx(hTxtFile, &nFileSize);

		DWORD uFileSize = nFileSize.LowPart;
		size_t num_chars = uFileSize >> 1;
		std::unique_ptr<wchar_t[]> buffer(new wchar_t[num_chars + 1]);

		DWORD dwBytesRead;
		if (ReadFile(hTxtFile, buffer.get(), uFileSize, &dwBytesRead, 0))
		{
			std::wstring_view v(buffer.get(), num_chars);
			strResult = v;
			if (pIsSuccess) { *pIsSuccess = true; }
		}
		else
		{
			if (pIsSuccess) { *pIsSuccess = false; }
		}
		CloseHandle(hTxtFile);
	}
	return strResult;
}

export bool WriteAllBytes(const wchar_t* szPathname, const void* pData, size_t length)
{
	HANDLE hFile = CreateFileW(szPathname, GENERIC_WRITE,
		0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwBytesWritten = 0;
		if (WriteFile(hFile, pData, (DWORD)length, &dwBytesWritten, 0))
		{
			CloseHandle(hFile);
			return true;
		}
	}
	return false;
}

export bool WriteTextFile(const std::wstring& pathname, const std::string& text)
{
	return WriteAllBytes(pathname.c_str(), text.c_str(), text.size());
}

export std::string ReadTextFile(const std::wstring& pathname, bool& isSuccess)
{
	return GetSmallTextFile(pathname, &isSuccess);
}

// See: https://stackoverflow.com/questions/2933295/embed-text-file-in-a-resource-in-a-native-windows-application
export bool LoadFileInResource(int name, int type, uint32_t& size, const char*& data)
{
	HMODULE handle = ::GetModuleHandle(NULL);
	HRSRC rc = ::FindResource(handle, MAKEINTRESOURCE(name),
		MAKEINTRESOURCE(type));
	XeASSERT(rc != 0);
	if (!rc)
	{
		size = 0;
		data = nullptr;
		return false;
	}
	HGLOBAL rcData = ::LoadResource(handle, rc);
	size = ::SizeofResource(handle, rc);
	data = static_cast<const char*>(::LockResource(rcData));
	return size > 0;
}

// Return zip file pathname from strPath or strPath unchanged if not zip file path. 
export std::wstring GetSourceZipPathname(const std::wstring& strPath)
{
	if (strPath.find(L"[ZIP] ") == 0)
	{
		size_t idx = strPath.find(L".zip", 6);
		return idx > 0 ? strPath.substr(6, (idx - 2)) : strPath.substr(6);
	}
	return strPath;
}

/*
--------------------------------------------------------------------------------
Description:
  Creates the actual 'lnk' file (assumes COM has been initialized).

Parameters:
  pszTargetfile    - File name of the link's target, must be a non-empty
					 string.

  pszTargetargs    - Command line arguments passed to link's target, may
					 be an empty string.

  pszLinkfile      - File name of the actual link file, must be a non-empty
					 string.

  pszDescription   - Description of the linked item. If this is an empty
					 string the description is not set.

  iShowmode        - ShowWindow() constant for the link's target. Use one of:
					   1 (SW_SHOWNORMAL) = Normal window.
					   3 (SW_SHOWMAXIMIZED) = Maximized.
					   7 (SW_SHOWMINNOACTIVE) = Minimized.
					 If this is zero the showmode is not set.

  pszCurdir        - Working directory of the active link. If this is
					 an empty string the directory is not set.

  pszIconfile      - File name of the icon file used for the link.
					 If this is an empty string the icon is not set.

  iIconindex       - Index of the icon in the icon file. If this is
					 < 0 the icon is not set.

Returns:
  HRESULT value >= 0 for success, < 0 for failure.
--------------------------------------------------------------------------------
*/
HRESULT CreateShortCut(const wchar_t* pszTargetfile, const wchar_t* pszTargetargs,
	const wchar_t* pszLinkfile, const wchar_t* pszDescription,
	int iShowmode, const wchar_t* pszCurdir,
	const wchar_t* pszIconfile, int iIconindex)
{
	HRESULT       hRes;                  /* Returned COM result code */
	IShellLinkW* pShellLink;            /* IShellLink object pointer */
	IPersistFile* pPersistFile;          /* IPersistFile object pointer */

	hRes = E_INVALIDARG;
	if (
		(pszTargetfile != NULL) && (wcslen(pszTargetfile) > 0) &&
		(pszTargetargs != NULL) &&
		(pszLinkfile != NULL) && (wcslen(pszLinkfile) > 0) &&
		(pszDescription != NULL) &&
		(iShowmode >= 0) &&
		(pszCurdir != NULL) &&
		(pszIconfile != NULL) &&
		(iIconindex >= 0)
		)
	{
		hRes = CoCreateInstance(CLSID_ShellLink,     /* pre-defined CLSID of the IShellLink object */
			NULL,                 /* pointer to parent interface if part of aggregate */
			CLSCTX_INPROC_SERVER, /* caller and called code are in same process */
			IID_IShellLink,      /* pre-defined interface of the IShellLink object */
			(LPVOID*)&pShellLink);         /* Returns a pointer to the IShellLink object */
		if (SUCCEEDED(hRes))
		{
			/* Set the fields in the IShellLink object */
			hRes = pShellLink->SetPath(pszTargetfile);
			hRes = pShellLink->SetArguments(pszTargetargs);
			if (wcslen(pszDescription) > 0)
			{
				hRes = pShellLink->SetDescription(pszDescription);
			}
			if (iShowmode > 0)
			{
				hRes = pShellLink->SetShowCmd(iShowmode);
			}
			if (wcslen(pszCurdir) > 0)
			{
				hRes = pShellLink->SetWorkingDirectory(pszCurdir);
			}
			if (wcslen(pszIconfile) > 0 && iIconindex >= 0)
			{
				hRes = pShellLink->SetIconLocation(pszIconfile, iIconindex);
			}

			/* Use the IPersistFile object to save the shell link */
			hRes = pShellLink->QueryInterface(
				IID_IPersistFile, /* pre-defined interface of the IPersistFile object */
				(LPVOID*)&pPersistFile);    /* returns a pointer to the IPersistFile object */
			if (SUCCEEDED(hRes))
			{
				hRes = pPersistFile->Save(pszLinkfile, TRUE);
				pPersistFile->Release();
			}
			pShellLink->Release();
		}
	}
	return (hRes);
}

export bool CreateShortCutToThisApp(bool isForAllUsers, const wchar_t* shortcutFilename,
	const wchar_t* description, bool inStartMenu, bool onDesktop)
{
	std::wstring strLinkFN = shortcutFilename;
	std::wstring strDesc = description;
	_ASSERT(wcscmp(GetFileExtension(strLinkFN).c_str(), L"lnk") == 0);
	wchar_t szExeFN[MAX_PATH];
	GetModuleFileNameW(NULL, szExeFN, sizeof(szExeFN));
	std::wstring strStartMenuPath, strDesktopPath;
	if (isForAllUsers)
	{
		strStartMenuPath = GetKnownFolder(FOLDERID_CommonStartMenu);
		strDesktopPath = GetKnownFolder(FOLDERID_PublicDesktop);
	}
	else
	{
		strStartMenuPath = GetKnownFolder(FOLDERID_StartMenu);
		strDesktopPath = GetKnownFolder(FOLDERID_Desktop);
	}
	if ((inStartMenu && strStartMenuPath.empty()) || (onDesktop && strDesktopPath.empty()))
	{
		return false;
	}
	bool retval = false;
	std::wstring strStartMenuLinkFN = strStartMenuPath + L"\\" + strLinkFN;
	if (inStartMenu && !FileExists(strStartMenuLinkFN))
	{
		HRESULT result = CreateShortCut(szExeFN, L"", strStartMenuLinkFN.c_str(), strDesc.c_str(),
			SW_SHOWNORMAL, L"", szExeFN, 0);
		retval = SUCCEEDED(result);
	}
	std::wstring strDesktopLinkFN = strDesktopPath + L"\\" + strLinkFN;
	if (onDesktop && !FileExists(strDesktopLinkFN))
	{
		HRESULT result = CreateShortCut(szExeFN, L"", strDesktopLinkFN.c_str(), strDesc.c_str(),
			SW_SHOWNORMAL, L"", szExeFN, 0);
		retval |= SUCCEEDED(result);
	}
	return retval;
}

// strFolder should not include a trailing backslash e.g. "C:\temp"
// strSearch should be the search condition e.g. "*.txt"
// returns full pathnames to every file found.
export std::vector<std::wstring> GetFileListFromFolder(
	const std::wstring& strFolder, const std::wstring& strSearch, bool includeSubFolders)
{
	std::vector<std::wstring> files_found;
	std::wstring strFF = strFolder + L"\\" + strSearch;
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(strFF.c_str(), &ffd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (ffd.cFileName[0] == '.')
			{
				continue;
			}
			std::wstring strFile = strFolder + L"\\" + std::wstring(ffd.cFileName);
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && includeSubFolders)
			{
				std::vector<std::wstring> sub_dir
					= GetFileListFromFolder(strFile, strSearch, includeSubFolders);
				files_found.insert(files_found.end(), sub_dir.begin(), sub_dir.end());
			}
			else
			{
				files_found.push_back(strFile);
			}
		} while (FindNextFileW(hFind, &ffd) != 0);
	}
	return files_found;
}

export std::wstring GetPathToThisExe()
{
	wchar_t szExeFN[MAX_PATH];
	GetModuleFileNameW(NULL, szExeFN, sizeof(szExeFN));
	return GetDirectory(szExeFN);
}

export std::wstring GetThisExePathname()
{
	wchar_t szExeFN[MAX_PATH];
	GetModuleFileNameW(NULL, szExeFN, sizeof(szExeFN));
	return std::wstring(szExeFN);
}

//export std::vector<std::wstring> ShowOpenFileDlg(const std::wstring& lastUsedDir)
//{
//	std::vector<std::wstring> files;
//	CComPtr<IFileOpenDialog> fod = 0;
//	//IFileOpenDialog* pFileOpen;
//
//	// Create the FileOpenDialog object.
//	HRESULT hr = fod.CoCreateInstance(__uuidof(FileOpenDialog));
//	//HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
//	//	IID_IFileOpenDialog, fod/*reinterpret_cast<void**>(&pFileOpen)*/);
//
//	if (SUCCEEDED(hr))
//	{
//		if (lastUsedDir.size())
//		{
//			IShellItem* pCurFolder = nullptr;
//			hr = SHCreateItemFromParsingName(lastUsedDir.c_str(), NULL, IID_PPV_ARGS(&pCurFolder));
//			if (SUCCEEDED(hr))
//			{
//				fod->SetFolder(pCurFolder);
//				pCurFolder->Release();
//			}
//		}
//
//		FILEOPENDIALOGOPTIONS fos;
//		fod->GetOptions(&fos);
//		fos |= FOS_ALLOWMULTISELECT;
//		fod->SetOptions(fos);
//
//		// Show the Open dialog box.
//		//hr = pFileOpen->Show(NULL);
//		hr = fod->Show(NULL);
//
//		// Get the file name from the dialog box.
//		if (SUCCEEDED(hr))
//		{
//			//IShellItem* pItem;
//			//hr = fod->GetResult(&pItem);
//
//			IShellItemArray* pItems = nullptr;
//			hr = fod->GetResults(&pItems);
//
//			if (SUCCEEDED(hr))
//			{
//				DWORD cItems = 0;
//				hr = pItems->GetCount(&cItems); // Get the number of items
//
//				if (SUCCEEDED(hr))
//				{
//					for (DWORD i = 0; i < cItems; i++)
//					{
//						IShellItem* pSI = nullptr;
//						hr = pItems->GetItemAt(i, &pSI); // Get the item at index i
//
//						if (SUCCEEDED(hr) && pSI)
//						{
//							// Use the IShellItem (pSI) here
//							// For example, to get the path:
//							// PWSTR pszPath = nullptr;
//							// if (SUCCEEDED(pSI->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
//							// {
//							//     // Use pszPath
//							//     CoTaskMemFree(pszPath);
//							// }
//							PWSTR pszFilePath;
//							hr = pSI->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
//
//							// Display the file name to the user.
//							if (SUCCEEDED(hr))
//							{
//								files.push_back(pszFilePath);
//								//MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
//								//CoTaskMemFree(pszFilePath);
//							}
//							//pItem->Release();
//
//							pSI->Release(); // Release the IShellItem
//						}
//					}
//				}
//				pItems->Release(); // Release the IShellItemArray
//			}
//		}
//		//pFileOpen->Release();
//	}
//	return files;
//}

export class CNumbersDirs
{
	std::vector<int> m_numbers_dirs;

public:
	// Find all directories under {path} whose names are composed using numbers only.
	// path = path to directory, e.g. 'C:\temp\mydir' - note: no trailing '\'.
	// return true if no error - m_numbers_dirs contains all directories found - sorted.
	bool ListDirs(const std::wstring& path)
	{
		m_numbers_dirs.clear();


		WIN32_FIND_DATAW ffd;
		std::wstring srch_path = path + L"\\*.*";
		HANDLE hFind = FindFirstFileW(srch_path.c_str(), &ffd);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			return false;
		}
		do
		{
			if (ffd.cFileName[0] == L'.')
			{
				continue;
			}
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				std::wstring fn(ffd.cFileName);
				if (is_digits(fn))
				{
					int n = std::stoi(fn);
					m_numbers_dirs.push_back(n);
				}
			}
		} while (FindNextFileW(hFind, &ffd) != 0);
		std::sort(m_numbers_dirs.begin(), m_numbers_dirs.end());
		return true;
	}

	// Get next unused number - 1 is returned if m_numbers_dirs is empty.
	int GetNextNumber() const
	{
		int cntr = 1;
		for (int n : m_numbers_dirs)
		{
			if (n != cntr)
			{
				break;
			}
			++cntr;
		}
		return cntr;
	}

	static bool is_digits(const std::wstring& str)
	{
		return str.find_first_not_of(L"0123456789") == std::string::npos;
	}
};

export struct CUSTOMERRCODE
{
	CUSTOMERRCODE() {}
	CUSTOMERRCODE(uint32_t dwErrCode) { dwError = dwErrCode; }
	union
	{
		uint32_t dwError;
		struct // first line = bit0
		{
			uint16_t uCode;
			uint16_t uFacility : 12;
			uint16_t uReserved : 1;
			uint16_t uCustomer : 1;	// Customer (custom) flag.
			uint16_t uSeverity : 2;	//      severity code
									//          00 - Success
									//          01 - Informational
									//          10 - Warning
									//          11 - Error
		};
	};
};

// Facility code for libzip zip library. Low 16 bits of dwError is code from libzip.
export uint16_t LIBZIP_FACILITYCODE = 3501;

// Create error code that includes the facility code. See: Winerror.h
export uint32_t MakeErrorCode(uint16_t uFacilityCode, uint16_t uErrorCode)
{
	CUSTOMERRCODE error;
	error.uCode = uErrorCode;
	error.uFacility = uFacilityCode;
	error.uCustomer = 1;
	error.uReserved = 0;
	error.uSeverity = 3;	// Error
	return error.dwError;
}

// FNV-1a hash calculator
export const uint64_t hash_64_fnv1a(const void* key, const uint64_t len)
{
	const char* data = (char*)key;
	uint64_t hash = 0xcbf29ce484222325;
	uint64_t prime = 0x100000001b3;

	for (int i = 0; i < len; ++i) {
		uint8_t value = data[i];
		hash = hash ^ value;
		hash *= prime;
	}

	return hash;
}

export class CWString_U32_bimap
{
	// Note - this class was created because boost::bimap doesn't compile with VS 17.1.3
protected:
	std::map<std::wstring, uint32_t>	m_left_map;
	std::map<uint32_t, std::wstring>	m_right_map;
	uint32_t							m_next_id = 1;
	std::wstring						m_dummy;

public:
	// Add (or find existing) string, return string ID
	uint32_t AddString(const wchar_t* str)
	{
		return AddString(std::wstring(str));
	}

	uint32_t AddString(const std::wstring& wstr)
	{
		auto it = m_left_map.find(wstr);
		if (it == m_left_map.end())
		{
			m_left_map[wstr] = m_next_id;
			m_right_map[m_next_id] = wstr;
			return m_next_id++;
		}
		return it->second;
	}

	//const std::wstring& GetString(uint32_t string_id)
	//{
	//	auto it = m_right_map.find(string_id);
	//	if (it != m_right_map.end())
	//	{
	//		return it->second;
	//	}
	//	return m_dummy;
	//}

	const std::wstring* GetString(uint32_t string_id)
	{
		auto it = m_right_map.find(string_id);
		if (it != m_right_map.end())
		{
			return &(it->second);
		}
		return nullptr;
	}
};
