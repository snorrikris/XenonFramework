module;

#include "os_minimal.h"
#include <string>
#include <winver.h>

export module Xe.FileVersionInfo;

/********************************************************************
*
* Copyright (C) 1999-2000 Sven Wiegand
* Copyright (C) 2000-2001 ToolsCenter
*
* This file is free software; you can redistribute it and/or
* modify, but leave the headers intact and do not remove any
* copyrights from the source.
*
* If you have further questions, suggestions or bug fixes, visit
* our homepage
*
*    http://www.ToolsCenter.org
*
********************************************************************/

#pragma comment(lib, "version.lib")

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif

export class CFileVersionInfo
{
public:
	VS_FIXEDFILEINFO m_FileInfo;

	std::wstring m_strCompanyName;
	std::wstring m_strFileDescription;
	std::wstring m_strFileVersion;
	std::wstring m_strInternalName;
	std::wstring m_strLegalCopyright;
	std::wstring m_strOriginalFileName;
	std::wstring m_strProductName;
	std::wstring m_strProductVersion;
	std::wstring m_strComments;
	std::wstring m_strLegalTrademarks;
	std::wstring m_strPrivateBuild;
	std::wstring m_strSpecialBuild;

	CFileVersionInfo()
	{
		ZeroMemory(&m_FileInfo, sizeof(m_FileInfo));
	}
	//virtual ~CFileVersionInfo();

public:
	BOOL Create(const wchar_t* lpszFileName)
	{
		DWORD	dwHandle;
		DWORD	dwFileVersionInfoSize = ::GetFileVersionInfoSizeW((wchar_t*)lpszFileName, &dwHandle);
		if (!dwFileVersionInfoSize)
			return FALSE;

		LPVOID	lpData = (LPVOID)new BYTE[dwFileVersionInfoSize];
		if (!lpData)
			return FALSE;

		try
		{
			if (!::GetFileVersionInfoW((wchar_t*)lpszFileName, dwHandle, dwFileVersionInfoSize, lpData))
				throw FALSE;

			// catch default information
			LPVOID	lpInfo;
			UINT		unInfoLen;
			if (::VerQueryValueW(lpData, L"\\", &lpInfo, &unInfoLen))
			{
				XeASSERT(unInfoLen == sizeof(m_FileInfo));
				if (unInfoLen == sizeof(m_FileInfo))
					memcpy(&m_FileInfo, lpInfo, unInfoLen);
			}

			// find best matching language and codepage
			::VerQueryValueW(lpData, L"\\VarFileInfo\\Translation", &lpInfo, &unInfoLen);

			DWORD	dwLangCode = 0;
			if (!GetTranslationId(lpInfo, unInfoLen, GetUserDefaultLangID(), dwLangCode, FALSE))
			{
				if (!GetTranslationId(lpInfo, unInfoLen, GetUserDefaultLangID(), dwLangCode, TRUE))
				{
					if (!GetTranslationId(lpInfo, unInfoLen, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), dwLangCode, TRUE))
					{
						if (!GetTranslationId(lpInfo, unInfoLen, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), dwLangCode, TRUE))
							// use the first one we can get
							dwLangCode = *((DWORD*)lpInfo);
					}
				}
			}


			constexpr size_t buf_len = 64;
			wchar_t buffer[buf_len];
			swprintf(buffer, buf_len, L"\\StringFileInfo\\%04X%04X\\",
					dwLangCode & 0x0000FFFF, (dwLangCode & 0xFFFF0000) >> 16);
			std::wstring sub_block(buffer);
			//CString	strSubBlock;
			//strSubBlock.Format(L"\\StringFileInfo\\%04X%04X\\", dwLangCode & 0x0000FFFF, (dwLangCode & 0xFFFF0000) >> 16);

			// catch string table
			if (VerQueryValueW(lpData, (sub_block + L"CompanyName").c_str(), &lpInfo, &unInfoLen))
				m_strCompanyName = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"FileDescription").c_str(), &lpInfo, &unInfoLen))
				m_strFileDescription = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"FileVersion").c_str(), &lpInfo, &unInfoLen))
				m_strFileVersion = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"InternalName").c_str(), &lpInfo, &unInfoLen))
				m_strInternalName = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"LegalCopyright").c_str(), &lpInfo, &unInfoLen))
				m_strLegalCopyright = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"OriginalFileName").c_str(), &lpInfo, &unInfoLen))
				m_strOriginalFileName = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"ProductName").c_str(), &lpInfo, &unInfoLen))
				m_strProductName = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"ProductVersion").c_str(), &lpInfo, &unInfoLen))
				m_strProductVersion = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"Comments").c_str(), &lpInfo, &unInfoLen))
				m_strComments = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"LegalTrademarks").c_str(), &lpInfo, &unInfoLen))
				m_strLegalTrademarks = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"PrivateBuild").c_str(), &lpInfo, &unInfoLen))
				m_strPrivateBuild = (LPCTSTR)lpInfo;
			if (VerQueryValueW(lpData, (sub_block + L"SpecialBuild").c_str(), &lpInfo, &unInfoLen))
				m_strSpecialBuild = (LPCTSTR)lpInfo;

			delete[] lpData;
		}
		catch (BOOL)
		{
			delete[] lpData;
			return FALSE;
		}

		return TRUE;
	}

	// attribute operations
public:
	WORD GetFileVersion(int nIndex) const
	{
		if (nIndex == 0)
			return (WORD)(m_FileInfo.dwFileVersionLS & 0x0000FFFF);
		else if (nIndex == 1)
			return (WORD)((m_FileInfo.dwFileVersionLS & 0xFFFF0000) >> 16);
		else if (nIndex == 2)
			return (WORD)(m_FileInfo.dwFileVersionMS & 0x0000FFFF);
		else if (nIndex == 3)
			return (WORD)((m_FileInfo.dwFileVersionMS & 0xFFFF0000) >> 16);
		else
			return 0;
	}

	WORD GetProductVersion(int nIndex) const
	{
		if (nIndex == 0)
			return (WORD)(m_FileInfo.dwProductVersionLS & 0x0000FFFF);
		else if (nIndex == 1)
			return (WORD)((m_FileInfo.dwProductVersionLS & 0xFFFF0000) >> 16);
		else if (nIndex == 2)
			return (WORD)(m_FileInfo.dwProductVersionMS & 0x0000FFFF);
		else if (nIndex == 3)
			return (WORD)((m_FileInfo.dwProductVersionMS & 0xFFFF0000) >> 16);
		else
			return 0;
	}

	DWORD GetFileFlagsMask() const
	{
		return m_FileInfo.dwFileFlagsMask;
	}

	DWORD GetFileFlags() const
	{
		return m_FileInfo.dwFileFlags;
	}

	DWORD GetFileOs() const
	{
		return m_FileInfo.dwFileOS;
	}

	DWORD GetFileType() const
	{
		return m_FileInfo.dwFileType;
	}

	DWORD GetFileSubtype() const
	{
		return m_FileInfo.dwFileSubtype;
	}

	FILETIME GetFileDate() const
	{
		FILETIME	ft;
		ft.dwLowDateTime = m_FileInfo.dwFileDateLS;
		ft.dwHighDateTime = m_FileInfo.dwFileDateMS;
		return ft;
	}

protected:
	BOOL GetTranslationId(LPVOID lpData, UINT unBlockSize, WORD wLangId, DWORD& dwId, BOOL bPrimaryEnough = FALSE)
	{
		for (LPWORD lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData) + unBlockSize; lpwData += 2)
		{
			if (*lpwData == wLangId)
			{
				dwId = *((DWORD*)lpwData);
				return TRUE;
			}
		}

		if (!bPrimaryEnough)
			return FALSE;

		for (LPWORD lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData) + unBlockSize; lpwData += 2)
		{
			if (((*lpwData) & 0x00FF) == (wLangId & 0x00FF))
			{
				dwId = *((DWORD*)lpwData);
				return TRUE;
			}
		}

		return FALSE;
	}
};

