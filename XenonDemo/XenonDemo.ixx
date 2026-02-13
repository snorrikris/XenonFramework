module;

#include "XeResource.h"

//#include "framework.h"
#include "os_minimal.h"
#include <memory>
#include "..\vsrollinglog\VSRL.h"

export module Demo.app;

import Demo.MainWnd;
import Xe.UserSettingsForUI;
import Xe.D2DAppBase;
//import Xe.UIcolorsIF;

export class CXenonDemoApp : public CXeD2DAppBase
{
private:
    VSRL::Logger& logger() { return VSRL::s_pVSRL->GetInstance("CXenonDemoApp"); }

protected:
    std::unique_ptr<CDemoMainWnd> m_mainWindow;

public:
    CXenonDemoApp() noexcept : CXeD2DAppBase(L"XenonDemoApp")
    {
    }

public:
    virtual void InitSettings() override
    {
        //std::vector<std::tuple<std::wstring, int>> ui_settings;
        //ui_settings.push_back(std::tuple<std::wstring, int>(L"LogFileGridSettings", IDR_DEFAULT_LOG_GRID_SETTINGS_JSON));
        //ui_settings.push_back(std::tuple<std::wstring, int>(L"LogFilesProcessingSettings", IDR_DEFAULT_LOG_FILES_PROCESSING_SETTINGS_JSON));
        //ui_settings.push_back(std::tuple<std::wstring, int>(L"RollingLogFilesSettings", IDR_DEFAULT_ROLLING_LOG_FILES_SETTINGS_JSON));
        //ui_settings.push_back(std::tuple<std::wstring, int>(L"TimelineSettings", IDR_DEFAULT_TIMELINE_SETTINGS_JSON));
        //ui_settings.push_back(std::tuple<std::wstring, int>(L"FilesListsSettings", IDR_DEFAULT_FILES_LISTS_SETTINGS_JSON));
        //ui_settings.push_back(std::tuple<std::wstring, int>(L"SoftwareUpgradeSettings", IDR_DEFAULT_SOFTWARE_UPGRADE_SETTINGS_JSON));
        //s_xeUIsettings.AddSettings(ui_settings, JSON_FILE_TYPE);

        //// Initialize last used UI settings
        //// Show tooltip about Ctrl+H for log message.
        //s_xeLastUsedUIsettings.AddSetting(CXeUserSetting::MakeBool(L"SHOW_LOGMSG_TT_HINT", true));
        //// Use Intel IPP regex in find when = 1, std::regex when = 0
        //s_xeLastUsedUIsettings.AddSetting(CXeUserSetting::MakeBool(L"USE_FAST_REGEX_IN_FIND", true));
    }

    virtual bool InitInstance(LPWSTR lpCmdLine, int nCmdShow) override
    {
        logger().debug("InitInstance");

        //std::wstring verType = s_xeUIsettings[L"SoftwareUpgradeSettings"].Get(L"UpgradeToVersionType").getString();
        //bool useReleaseVersionUrl = wcscmp(verType.c_str(), L"Released Versions") == 0;
        //if (!useReleaseVersionUrl)
        //{
        //    // Set logging level to debug if the user setting is not set to "Released Versions".
        //    VSRL::s_pVSRL->SetLogLevel(VSRL::LogLevel::Debug);
        //}

        //MessageBox(NULL, "Attach debugger", "Pause", MB_OK);

        m_mainWindow = std::make_unique<CDemoMainWnd>(&m_UIcolors);
        XeASSERT(m_mainWindow.get());
        if (!m_mainWindow.get())
        {
            return false;
        }
        // create main frame window
        if (!m_mainWindow->Create(IDR_MAINFRAME, L"Xenon Framework Demo App", CRect()))
        {
            XeASSERT(false);
            return false;
        }

        return true;
    }
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    CXenonDemoApp theApp;
    return theApp.Run(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

//#define MAX_LOADSTRING 100
//
//// Global Variables:
//HINSTANCE hInst;                                // current instance
//WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
//WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
//
//// Forward declarations of functions included in this code module:
//ATOM                MyRegisterClass(HINSTANCE hInstance);
//BOOL                InitInstance(HINSTANCE, int);
//LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
//INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
//
//int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
//                     _In_opt_ HINSTANCE hPrevInstance,
//                     _In_ LPWSTR    lpCmdLine,
//                     _In_ int       nCmdShow)
//{
//    UNREFERENCED_PARAMETER(hPrevInstance);
//    UNREFERENCED_PARAMETER(lpCmdLine);
//
//    // TODO: Place code here.
//
//    // Initialize global strings
//    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
//    LoadStringW(hInstance, IDC_XENONDEMO, szWindowClass, MAX_LOADSTRING);
//    MyRegisterClass(hInstance);
//
//    // Perform application initialization:
//    if (!InitInstance (hInstance, nCmdShow))
//    {
//        return FALSE;
//    }
//
//    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_XENONDEMO));
//
//    MSG msg;
//
//    // Main message loop:
//    while (GetMessage(&msg, nullptr, 0, 0))
//    {
//        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
//        {
//            TranslateMessage(&msg);
//            DispatchMessage(&msg);
//        }
//    }
//
//    return (int) msg.wParam;
//}
//
//
//
////
////  FUNCTION: MyRegisterClass()
////
////  PURPOSE: Registers the window class.
////
//ATOM MyRegisterClass(HINSTANCE hInstance)
//{
//    WNDCLASSEXW wcex;
//
//    wcex.cbSize = sizeof(WNDCLASSEX);
//
//    wcex.style          = CS_HREDRAW | CS_VREDRAW;
//    wcex.lpfnWndProc    = WndProc;
//    wcex.cbClsExtra     = 0;
//    wcex.cbWndExtra     = 0;
//    wcex.hInstance      = hInstance;
//    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XENONDEMO));
//    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
//    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
//    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_XENONDEMO);
//    wcex.lpszClassName  = szWindowClass;
//    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
//
//    return RegisterClassExW(&wcex);
//}
//
////
////   FUNCTION: InitInstance(HINSTANCE, int)
////
////   PURPOSE: Saves instance handle and creates main window
////
////   COMMENTS:
////
////        In this function, we save the instance handle in a global variable and
////        create and display the main program window.
////
//BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
//{
//   hInst = hInstance; // Store instance handle in our global variable
//
//   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
//      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
//
//   if (!hWnd)
//   {
//      return FALSE;
//   }
//
//   ShowWindow(hWnd, nCmdShow);
//   UpdateWindow(hWnd);
//
//   return TRUE;
//}
//
////
////  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
////
////  PURPOSE: Processes messages for the main window.
////
////  WM_COMMAND  - process the application menu
////  WM_PAINT    - Paint the main window
////  WM_DESTROY  - post a quit message and return
////
////
//LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//    switch (message)
//    {
//    case WM_COMMAND:
//        {
//            int wmId = LOWORD(wParam);
//            // Parse the menu selections:
//            switch (wmId)
//            {
//            case IDM_ABOUT:
//                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
//                break;
//            case IDM_EXIT:
//                DestroyWindow(hWnd);
//                break;
//            default:
//                return DefWindowProc(hWnd, message, wParam, lParam);
//            }
//        }
//        break;
//    case WM_PAINT:
//        {
//            PAINTSTRUCT ps;
//            HDC hdc = BeginPaint(hWnd, &ps);
//            // TODO: Add any drawing code that uses hdc here...
//            EndPaint(hWnd, &ps);
//        }
//        break;
//    case WM_DESTROY:
//        PostQuitMessage(0);
//        break;
//    default:
//        return DefWindowProc(hWnd, message, wParam, lParam);
//    }
//    return 0;
//}
//
//// Message handler for about box.
//INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
//{
//    UNREFERENCED_PARAMETER(lParam);
//    switch (message)
//    {
//    case WM_INITDIALOG:
//        return (INT_PTR)TRUE;
//
//    case WM_COMMAND:
//        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
//        {
//            EndDialog(hDlg, LOWORD(wParam));
//            return (INT_PTR)TRUE;
//        }
//        break;
//    }
//    return (INT_PTR)FALSE;
//}
