module;

#include "os_minimal.h"
#include <memory>
#include <string>
#include <vector>
#include "..\vsrollinglog\VSRL.h"
#include "XeResource.h"

// TODO: isolate what is needed for the app base - into base.rc file
//#include "..\LogViewerStudio\Resource.h"

export module Xe.D2DAppBase;

import Xe.UIcolors;
import Xe.FileHelpers;
import Xe.UserSettingsForUI;
import Xe.Helpers;

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export class CXeD2DAppBase
{
private:
	VSRL::Logger& logger() { return VSRL::s_pVSRL->GetInstance("CXeD2DAppBase"); }

protected:
	CXeUIcolors m_UIcolors;

	CXeUIcolorsIF* m_xeUI = nullptr;

	std::wstring m_appName;

	std::wstring m_logExt = L".log", m_logFilename, m_logFolder, m_settingsFolder;

	VSRL::LogLevel m_defaultLogLevel = VSRL::LogLevel::Debug;

public:
	CXeD2DAppBase(const std::wstring& app_name) noexcept
			: m_appName{app_name}
	{
		std::wstring strTemp = GetKnownFolder(L"%TEMP%");
		m_logFolder = strTemp + L"\\" + app_name;
		m_settingsFolder = GetCurrentUserAppDataFolder(app_name) + L"\\UserSettings";
		m_logFilename = app_name;
	}
	virtual ~CXeD2DAppBase() {}

public:
	int Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
	{
		_Initialize(hInstance);

		InitSettings();	// Derived class should add all it's settings for s_xeUIsettings and s_xeLastUsedUIsettings.

		// Load user modified settings.
		s_xeLastUsedUIsettings.Load();

		std::wstring lastUsedTheme = s_xeLastUsedUIsettings.GetString_or_Val(L"LastUsedTheme", L"Dark theme");
		if (wcscmp(lastUsedTheme.c_str(), L"Theme2") == 0) { lastUsedTheme = L"Dark theme"; }
		m_UIcolors.Create(m_appName, hInstance, lastUsedTheme);
		m_xeUI = &m_UIcolors;

		// Derived class will finish initializing and then create THE main window.
		if (!InitInstance(lpCmdLine, nCmdShow) || m_UIcolors.GetMainWindowHandle() == NULL)
		{
			_Exit();
			return -1;	// exit with error
		}

		//HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTWINBASICAPP));
		HACCEL hAccelTable = 0;

		// Main message loop: exit when WM_QUIT received.
		MSG msg;
		while (::GetMessageW(&msg, nullptr, 0, 0))
		{
			if (IsKeyboardMessage(msg.message))
			{
				if (m_UIcolors.FilterKeyboardMessage(msg))
				{
					continue;
				}
			}
			else if ((IsMouseMessage(msg.message) || msg.message == WM_MOUSELEAVE)
					&& m_UIcolors.IsScintillaEditControl(msg.hwnd))
			{
				HWND hECparentWnd = ::GetParent(msg.hwnd);
				if (hECparentWnd)
				{
					// Dispatch to parent of Scintilla edit control - to show/hide toolip when needed.
					MSG msgEC = msg;
					msgEC.hwnd = hECparentWnd;
					::DispatchMessageW(&msgEC);
				}
			}
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}

		_Exit();
		int exitCode = ExitInstance((int)msg.wParam); // The exit code given in the PostQuitMessage function.

		return exitCode;	// Exit application
	}

	// Derived class should add all it's settings for s_xeUIsettings and s_xeLastUsedUIsettings.
	virtual void InitSettings() {}

	// Derived class should finish initializing and then create THE main window.
	virtual bool InitInstance(LPWSTR lpCmdLine, int nCmdShow) = 0;

	virtual int ExitInstance(int exitCode)
	{
		return exitCode;
	}

protected:
	void _Initialize(HINSTANCE hInstance)
	{
		VSRL::VSRL::CreateTheOneAndOnlyInstance(m_logFolder, m_logFilename, m_logExt, m_defaultLogLevel);
		logger().debug("_Initialize");

		CXeUserSettings::SetRootFolder(m_settingsFolder);

		if (!s_xeUIsettings.AddSetting(L"GeneralSettings", IDR_DEFAULT_GENERAL_SETTINGS_JSON, JSON_FILE_TYPE))
		{
			XeASSERT(false);	// "GeneralSettings" MUST exist.
		}

		s_xeLastUsedUIsettings.SetName(L"LastUsedUIsettings");
		s_xeLastUsedUIsettings.CreateSettingsAutomatically();

		/* Initialize Intel IPP library */
		//ippInit();

		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

		HMODULE hmod = ::LoadLibraryW(L"Scintilla.dll");
		if (hmod == NULL)
		{
			::MessageBoxW(NULL,
				L"The Scintilla DLL could not be loaded.",
				L"Error loading Scintilla",
				MB_OK | MB_ICONERROR);
		}
	}

	void _Exit()
	{
		m_UIcolors.Destroy();

		CoUninitialize();

		VSRL::VSRL::DeleteTheOneAndOnlyInstance();
	}
};

