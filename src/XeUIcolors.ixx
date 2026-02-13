module;

#include "os_minimal.h"
#include <string>
#include <memory>
//#include "..\LogViewerStudio\resource.h"
#include <algorithm>
#include <optional>
#include <functional>
#include <tchar.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include "logging.h"
#include "nlohmann/json.hpp"
#include "XeResource.h"

export module Xe.UIcolors;

import Xe.UIcolorsIF;
import Xe.Theme;
import Xe.UserSettings;
import Xe.UserSettingsForUI;
import Xe.Helpers;
import Xe.FileHelpers;
import Xe.Menu;
import Xe.PPTooltip;
import Xe.D2DWndBase;
//import Xe.LogDefs;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dxguid")

using json = nlohmann::json;

// Convert font point size to DIPs conversion factor .
constexpr float FONT_PT_TO_DIPS = 1.3333333333f;

struct XeD2DFont
{
	XeFontMetrics	m_tm;
	IDWriteTextFormat* m_pFont = nullptr;
};

class XeDWFontMap
{
	std::map<EXE_FONT, XeD2DFont> m_map;

public:
	XeDWFontMap() = default;
	~XeDWFontMap()
	{
		XeASSERT(m_map.size() == 0);	// Call clear() before dtor.
		_release_fonts();
	}

	void InitializeMap()
	{
		XeASSERT(m_map.size() == 0);	// Call when UIColors created.
		for (size_t i = 0; i < static_cast<size_t>(EXE_FONT::eLastFont); ++i)
		{
			m_map[(EXE_FONT)i] = XeD2DFont();
		}
	}

	void ReleaseFonts()
	{
		_release_fonts();
	}

	void SetFont(EXE_FONT key, IDWriteTextFormat* pTF)
	{
		if (m_map.contains(key))
		{
			XeD2DFont& tm = m_map.at(key);
			XeASSERT(tm.m_pFont == nullptr);	// Font should not exist.
			tm.m_pFont = pTF;
		}
		else
		{
			XeASSERT(false);	// font struct should exist.
		}
	}

	bool HasFont(EXE_FONT key) const
	{
		if (m_map.contains(key))
		{
			return m_map.at(key).m_pFont != nullptr;
		}
		return false;
	}

	IDWriteTextFormat* GetFont(EXE_FONT key) const
	{
		const XeD2DFont* pTM = _GetFontInfoStructConst(key);
		XeASSERT(pTM && pTM->m_pFont);
		return pTM->m_pFont;
	}

	const XeFontMetrics& GetFontInfoConst(EXE_FONT key) const
	{
		const XeD2DFont* pTM = _GetFontInfoStructConst(key);
		XeASSERT(pTM);
		return pTM->m_tm;
	}

	void SetFontInfo(EXE_FONT key, const XeFontMetrics& tm)
	{
		XeD2DFont* pTM = _GetFontInfoStruct(key);
		XeASSERT(pTM);
		pTM->m_tm = tm;
	}

	void Clear()
	{
		_release_fonts();
		m_map.clear();
	}

protected:
	void _release_fonts()
	{
		for (auto& [eFont, tm] : m_map)
		{
			if (tm.m_pFont)
			{
				tm.m_pFont->Release();
				tm.m_pFont = nullptr;
			}
		}
	}

	XeD2DFont* _GetFontInfoStruct(EXE_FONT key)
	{
		if (m_map.contains(key))
		{
			return &(m_map.at(key));
		}
		XeASSERT(false);	// struct should exist.

		// Return default font if requested font not found in map.
		if (key != EXE_FONT::eUI_Font)
		{
			return _GetFontInfoStruct(EXE_FONT::eUI_Font);
		}
		XeASSERT(false);
		return nullptr;
	}
	const XeD2DFont* _GetFontInfoStructConst(EXE_FONT key) const
	{
		if (m_map.contains(key))
		{
			return &(m_map.at(key));
		}
		XeASSERT(false);	// struct should exist.

		// Return default font if requested font not found in map.
		if (key != EXE_FONT::eUI_Font)
		{
			return _GetFontInfoStructConst(EXE_FONT::eUI_Font);
		}
		XeASSERT(false);
		return nullptr;
	}
};

struct XeD2DWndInfo
{
	CXeD2DWndBase* m_pWndObj = nullptr;
	std::wstring	m_className;
	std::wstring	m_parentClassName;
};
class XeHwnd2ObjectMap
{
	std::map<HWND, XeD2DWndInfo> m_map;

	// We keep the last hWnd removed - because it can happen that a window can receive messages
	// after it has been removed (had a case of WM_IME_NOTIFY after WM_NCDESTROY for ViewsListWnd).
	// When Get is called for a window not in the map - we just silently return null instead of ASSERT
	// when the handle is the last removed window.
	HWND m_last_hWnd_removed = 0;

public:
	XeHwnd2ObjectMap() = default;
	//~XeHwnd2ObjectMap() {}

	void Set(HWND hWnd, XeD2DWndInfo nfo)
	{
		if (m_map.contains(hWnd))
		{
			XeASSERT(false);	// Set called twice - why?
		}
		m_map[hWnd] = nfo;
	}

	bool HasHwnd(HWND hWnd) const
	{
		return m_map.contains(hWnd);
	}

	inline const XeD2DWndInfo* Get(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return &(m_map.at(hWnd));
		}
		if (hWnd != m_last_hWnd_removed)
		{
			XeASSERT(false);
		}
		return nullptr;
	}

	inline const XeD2DWndInfo* GetIfExists(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return &(m_map.at(hWnd));
		}
		return nullptr;
	}

	void Remove(HWND hWnd)
	{
		if (m_map.contains(hWnd))
		{
			m_map.erase(hWnd);
		}
		m_last_hWnd_removed = hWnd;
	}
};

class XeHwnd2CallbackMap
{
	std::map<HWND, DialogClosedCallbackFunc> m_map;

public:
	XeHwnd2CallbackMap() = default;
	//~XeHwnd2CallbackMap() {}

	void Set(HWND hWnd, DialogClosedCallbackFunc callback)
	{
		if (m_map.contains(hWnd))
		{
			XeASSERT(false);	// Set called twice - why?
		}
		m_map[hWnd] = callback;
	}

	bool HasHwnd(HWND hWnd) const
	{
		return m_map.contains(hWnd);
	}

	inline DialogClosedCallbackFunc Get(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return m_map.at(hWnd);
		}
		XeASSERT(false);
		return nullptr;
	}

	inline DialogClosedCallbackFunc GetIfExists(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return m_map.at(hWnd);
		}
		return nullptr;
	}

	void Remove(HWND hWnd)
	{
		if (m_map.contains(hWnd))
		{
			m_map.erase(hWnd);
		}
	}
};

class XeHwnd2KeyboardFilterMap
{
	std::map<HWND, FilterKeyboardMessageCallbackFunc> m_map;

public:
	XeHwnd2KeyboardFilterMap() = default;
	//~XeHwnd2KeyboardFilterMap() {}

	void Set(HWND hWnd, FilterKeyboardMessageCallbackFunc filter_callback)
	{
		if (m_map.contains(hWnd))
		{
			XeASSERT(false);	// Set called twice - why?
		}
		m_map[hWnd] = filter_callback;
	}

	bool HasHwnd(HWND hWnd) const
	{
		return m_map.contains(hWnd);
	}

	inline FilterKeyboardMessageCallbackFunc Get(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return m_map.at(hWnd);
		}
		XeASSERT(false);
		return nullptr;
	}

	inline FilterKeyboardMessageCallbackFunc GetIfExists(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return m_map.at(hWnd);
		}
		return nullptr;
	}

	void Remove(HWND hWnd)
	{
		if (m_map.contains(hWnd))
		{
			m_map.erase(hWnd);
		}
	}
};

class XeTooltipsMap
{
protected:
	std::map<HWND, std::unique_ptr<CPPToolTip>> m_tooltips;

public:
	CXeTooltipIF* Create(CXeUIcolorsIF* pUIcolors, const std::wstring& nameForLogging, HWND hWndParent)
	{
		XeASSERT(m_tooltips.find(hWndParent) == m_tooltips.end());
		std::unique_ptr<CPPToolTip> tt = std::make_unique<CPPToolTip>(pUIcolors);
		XeASSERT(tt.get());
		tt->Create(nameForLogging.c_str(), hWndParent);
		m_tooltips[hWndParent] = std::move(tt);
		return Find(hWndParent);
	}

	void Destroy(HWND hWndParent)
	{
		CPPToolTip* pTT = Find(hWndParent);
		if (pTT->GetSafeHwnd())
		{
			pTT->DestroyWindow();
		}
	}

	void HideTooltip(HWND hWndParent)
	{
		CPPToolTip* pTT = Find(hWndParent);
		if (pTT->GetSafeHwnd())
		{
			pTT->HideTooltip();
		}
	}

	void HideOtherTooltips(HWND hWndTooltip)
	{
		for (auto& [hWndParent, tt] : m_tooltips)
		{
			HWND hWndTT = tt->GetSafeHwnd();
			if (hWndTT && hWndTT != hWndTooltip)
			{
				tt->HideTooltip();
			}
		}
	}

	CPPToolTip* Find(HWND hWndParent)
	{
		auto it = m_tooltips.find(hWndParent);
		if (it != m_tooltips.end())
		{
			return it->second.get();
		}
		return nullptr;
	}
};

// Define function pointer for DWM dll functions.
//typedef HRESULT (WINAPI * LPFNDLL_DwmEnableComposition)( UINT uCompositionAction );
//LPFNDLL_DwmEnableComposition pFN 
//	= (LPFNDLL_DwmEnableComposition)::GetProcAddress( hDWMdll, "DwmEnableComposition" );
typedef HRESULT(WINAPI* LPFNDLL_DwmSetWindowAttribute)(__in HWND hwnd, __in DWORD dwAttribute, __in LPCVOID pvAttribute, __in DWORD cbAttribute);

typedef std::function<int(CXeUIcolorsIF* pUIcolors, HWND hParent, const wchar_t* pMsg, const wchar_t* pTitle, UINT uType, CRect* pRectCell)> ShowMessageBoxCallbackFunc;

export class CXeUIcolors : public CXeUIcolorsIF
{
private:
	VSRL::Logger& logger() { return VSRL::s_pVSRL->GetInstance("CXeUIcolors"); }

protected:
	HINSTANCE m_hInstance = 0;

	std::wstring m_appName;

	XeHwnd2ObjectMap m_hwndMap;

	XeHwnd2CallbackMap m_hwndMapDialog;

	XeHwnd2KeyboardFilterMap m_hwndScintillaEditControls_MapkeyboardFilter;

	FilterKeyboardMessageCallbackFunc m_global_keyboard_filter_callback = nullptr;

	HWND m_hMainWnd = 0;

	/////////////////////////////////////////////////////////////////////////////
	// Theme data
	XeTheme m_theme;
	std::vector<ThemeInfoJson> m_theme_list;

	/////////////////////////////////////////////////////////////////////////////
	// Font data

	int m_monospacedFontSize = 14, /*m_gridRowPitch = 6,*/ m_uiFontSize = 14;
	//std::wstring m_strUI_FontName;
	//int m_uiFontGridRowHeight = 20;

	int m_cxTooltipDefaultUnscaled = 212;

	double m_cxScaleFactor = 1.0, m_cyScaleFactor = 1.0;

	int s_cyGridTH_RowHeight = 0;

	int m_cxDefaultTooltip = 212;

	MonospacedCharSize m_monospacedCharSize;

	/////////////////////////////////////////////////////////////////////////////
	// Other data

	XeTooltipsMap m_tooltips;

	// Dialog that wants to know if user clicked L button down outside it's window.
	HWND m_hWndDialogWantsClickOutsideMouseDown = nullptr;
	UINT m_uDialogWantsClickOutsideMouseDownMessage = 0;

	ECURSOR m_eAppCursor = ECURSOR::eArrow;

#pragma region Direct2D_data
	Microsoft::WRL::ComPtr<IWICImagingFactory> m_d2d_IWICFactory;

	Microsoft::WRL::ComPtr<ID2D1Factory> m_d2d_fa;
	Microsoft::WRL::ComPtr<IDWriteFactory> m_d2d_writeFactory;

	XeDWFontMap m_d2d_fonts;
	int m_logPixelsY = 1;	// Logical pixels/inch in Y - of the desktop monitor (UI screen).

	std::map<PID, Microsoft::WRL::ComPtr<IWICFormatConverter>> m_d2d_pid_map_img;
	std::map<PID, CSize> m_d2d_pid_map_size;

	Microsoft::WRL::ComPtr<ID2D1StrokeStyle> m_strokeStyleFocus;
#pragma endregion Direct2D_data

#pragma region Create
public:
	void Create(const std::wstring& app_name, HINSTANCE hInstance, const std::wstring& theme_name)
	{
		logger().debug("Create");
		XeASSERT(hInstance);
		m_hInstance = hInstance;
		m_appName = app_name;

		//m_monospacedCharSize.cySingleLineRowHeight = 0/*20*/;
		m_monospacedCharSize.cyMultiLineTextHeight = 0/*14*/;
		m_monospacedCharSize.cxGridCharWidth = 8;

		_ReadAllThemeFiles();
		_InitializeThemeData(theme_name);

		_InitializeDirect2D();

		m_d2d_fonts.InitializeMap();
		_CreateFonts();
	}

	void Destroy()
	{
		_DeleteFonts();

		_UninitializeDirect2D();
	}
#pragma endregion Create

#pragma region System
	virtual HINSTANCE GetInstanceHandle() override { return m_hInstance; }

	virtual const std::wstring& GetAppName() const override { return m_appName; }

	virtual bool RegisterWindowClass(const std::wstring& class_name, WNDPROC wndProc,
		UINT style = CS_DBLCLKS, HICON hIcon = nullptr, HBRUSH hBgBrush = nullptr) override
	{
		WNDCLASS wndcls;

		// 'Our' class already registered?
		if (!(::GetClassInfo(m_hInstance, class_name.c_str(), &wndcls)))
		{	// Otherwise we need to register a new class
			wndcls.style = style;
			wndcls.lpfnWndProc = wndProc;
			wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
			wndcls.hInstance = m_hInstance;
			wndcls.hIcon = hIcon;
			wndcls.hCursor = ::LoadCursor(NULL, IDC_ARROW);
			wndcls.hbrBackground = hBgBrush;
			wndcls.lpszMenuName = NULL;
			wndcls.lpszClassName = class_name.c_str();

			if (!::RegisterClass(&wndcls))
			{
				//TRACE(_T("Register window class %s failed\n"), class_name.c_str());
				XeASSERT(FALSE);
				logger().error("Register window class %s failed.", xet::to_astr(class_name).c_str());
				return false;
			}
		}
		return true;
	}

	virtual void* GetHwndD2DBasePtr(HWND hWnd) override
	{
		const XeD2DWndInfo* pNfo = m_hwndMap.Get(hWnd);
		if (!::IsWindow(hWnd))
		{
			XeASSERT(false);
			return nullptr;
		}
		return pNfo ? pNfo->m_pWndObj : nullptr;
	}

	virtual void* GetHwndD2DBasePtrIfExists(HWND hWnd) override
	{
		const XeD2DWndInfo* pNfo = m_hwndMap.GetIfExists(hWnd);
		return pNfo ? pNfo->m_pWndObj : nullptr;
	}

	virtual void SetHwndD2DBasePtr(HWND hWnd, void* pWndObj, const wchar_t* class_name) override
	{
		HWND hParentWnd = ::GetParent(hWnd);
		std::wstring parent_class_name;
		parent_class_name.resize(256, L' ');
		int len = ::GetClassNameW(hParentWnd, parent_class_name.data(), 255);
		parent_class_name.resize(len);
		XeD2DWndInfo nfo;
		nfo.m_parentClassName = parent_class_name;
		nfo.m_className = class_name ? class_name : L"";
		nfo.m_pWndObj = (CXeD2DWndBase*)pWndObj;
		m_hwndMap.Set(hWnd, nfo);
	}
	const XeD2DWndInfo* GetHwndD2DBaseNfo(HWND hWnd)
	{
		const XeD2DWndInfo* pNfo = m_hwndMap.Get(hWnd);
		return pNfo;
	}
	virtual void RemoveHwndD2DBasePtr(HWND hWnd) override
	{
		m_hwndMap.Remove(hWnd);
		DialogClosedCallbackFunc callback = m_hwndMapDialog.GetIfExists(hWnd);
		if (callback)
		{
			callback();
		}
		m_hwndMapDialog.Remove(hWnd);
	}

	virtual void AddDialogHandle(HWND hDlgWnd, DialogClosedCallbackFunc dlgClosedCallback) override
	{
		m_hwndMapDialog.Set(hDlgWnd, dlgClosedCallback);
	}

	virtual void SetMainWindowHandle(HWND hMainWnd) override
	{
		m_hMainWnd = hMainWnd;
	}

	virtual HWND GetMainWindowHandle() const override
	{
		XeASSERT(m_hMainWnd);
		return m_hMainWnd;
	}

	virtual void RegisterScintillaKeyboardFilterCallback(HWND hWndScintilla,
		FilterKeyboardMessageCallbackFunc filter_callback) override
	{
		m_hwndScintillaEditControls_MapkeyboardFilter.Set(hWndScintilla, filter_callback);
	}

	virtual void UnRegisterScintillaKeyboardFilterCallback(HWND hWndScintilla) override
	{
		m_hwndScintillaEditControls_MapkeyboardFilter.Remove(hWndScintilla);
	}

	virtual bool IsScintillaEditControl(HWND hWnd) const override
	{
		return m_hwndScintillaEditControls_MapkeyboardFilter.HasHwnd(hWnd);
	}

	// This method is called from within the message loop when any keyboard message.
	virtual bool FilterKeyboardMessage(const MSG& msg) override
	{
		// Find parent dialog window (of the window whose message this is).
		HWND hParentDlg = _FindParentDialogWindow(msg.hwnd);
		if (hParentDlg)
		{
			// Hande ESCAPE, RETURN and TAB keys.
			if (_ProcessDialogKeyboardMessage(hParentDlg, msg))
			{
				return true;	// Message was processed and should not be dispatched.
			}
		}

		if (m_global_keyboard_filter_callback && m_global_keyboard_filter_callback(msg))
		{
			return true;
		}

		// Keyboard filter callbacks (used by Scintilla EC - to "handle" single line edit control).
		FilterKeyboardMessageCallbackFunc filter_callback = m_hwndScintillaEditControls_MapkeyboardFilter.GetIfExists(msg.hwnd);
		if (filter_callback)
		{
			if (::IsWindow(msg.hwnd))
			{
				return filter_callback(msg);
			}
		}
		return false;	// Process message normally.
	}

	virtual void SetGlobalKeyboardFilterCallback(FilterKeyboardMessageCallbackFunc filter_callback) override
	{
		m_global_keyboard_filter_callback = filter_callback;
	}

protected:
	// Find parent dialog window (of the window whose window handle this is).
	// Returns window handle of the parent dialog
	// or NULL when hWnd does not belong to a control inside a dialog box window.
	HWND _FindParentDialogWindow(HWND hWnd)
	{
		HWND hParentDlg = hWnd;
		if (m_hwndMapDialog.HasHwnd(hParentDlg))	// Is hWnd itself the dialog window?
		{
			return hParentDlg;
		}
		while (hParentDlg)
		{
			hParentDlg = ::GetParent(hParentDlg);
			if (m_hwndMapDialog.HasHwnd(hParentDlg))
			{
				return hParentDlg;
			}
		}
		return NULL;
	}

	// return true if message was processed and should not be dispatched.
	bool _ProcessDialogKeyboardMessage(HWND hParentDlg, const MSG& msg)
	{
		bool isEditCtrl = m_hwndScintillaEditControls_MapkeyboardFilter.HasHwnd(msg.hwnd);
		bool wantReturn = false;
		if (isEditCtrl)
		{
			DWORD dwECStyle = ::GetWindowLongW(msg.hwnd, GWL_STYLE);
			wantReturn = dwECStyle & ES_WANTRETURN;
		}
		CXeShiftCtrlAltKeyHelper sca;
		if (msg.message == WM_KEYDOWN)
		{
			if (sca.IsNoneDown() && msg.wParam == VK_ESCAPE)
			{
				::SendMessage(hParentDlg, WM_COMMAND, IDCANCEL, 0);
				return true;
			}
			else if (sca.IsNoneDown() && msg.wParam == VK_RETURN && !(isEditCtrl && wantReturn))
			{
				::SendMessage(hParentDlg, WM_COMMAND, IDOK, 0);
				return true;
			}
			else if ((sca.IsNoneDown() || sca.IsOnlyShiftDown()) && msg.wParam == VK_TAB)
			{
				DoTabToNextCtrl(sca.IsOnlyShiftDown(), hParentDlg, msg.hwnd);
				return true;	// Don't process this message any further.
			}
		}
		else if (msg.message == WM_KEYUP)
		{
			if ((sca.IsNoneDown() || sca.IsOnlyShiftDown()) && msg.wParam == VK_TAB)
			{
				return true;	// Ignore message.
			}
		}
		return false;
	}
#pragma endregion System

#pragma region Fonts
	virtual const XeFontMetrics& GetFontMetric(EXE_FONT eFont) const override
	{
		return m_d2d_fonts.GetFontInfoConst(eFont);
	}

	virtual UIScaleFactor GetUIScaleFactor() const override
	{
		return UIScaleFactor(m_cxScaleFactor, m_cyScaleFactor);
	}

	virtual CSize GetTextSizeW(EXE_FONT eFont, const wchar_t* szText) override
	{
		return GetTextSizeW(eFont, szText, std::wcslen(szText));
	}
	virtual CSize GetTextSizeW(EXE_FONT eFont, const wchar_t* szText, size_t length) override
	{
		CSize size1;

		HRESULT hr = S_OK;
		IDWriteTextFormat* pUIfont = D2D_GetFont(eFont);
		Microsoft::WRL::ComPtr<IDWriteTextLayout> pTextLayout;
		float floatDim = 2500.0f;
		// Create a text layout 
		hr = m_d2d_writeFactory->CreateTextLayout(szText, (UINT32)length,
				pUIfont, floatDim, floatDim, &pTextLayout);
		XeASSERT(hr == S_OK);
		if (SUCCEEDED(hr))
		{
			// Get text size  
			DWRITE_TEXT_METRICS textMetrics;
			hr = pTextLayout->GetMetrics(&textMetrics);
			D2D1_SIZE_F size = D2D1::SizeF((float)ceil(textMetrics.widthIncludingTrailingWhitespace),
				(float)ceil(textMetrics.height));
			size1.cx = (int)(size.width + 0.5f);
			size1.cy = (int)(size.height + 0.5f);
		}

		return size1;
	}

	virtual CSize GetTextSizeUsingFontW(HWND hWnd, EXE_FONT eFontconst, const wchar_t* szText) override
	{
		return GetTextSizeW(eFontconst, szText);
	}

	// Get text widths using font and window DC.
	// if pWnd is null then desktop window is used.
	virtual std::vector<uint32_t> GetTextWidthsUsingFontW(const std::vector<std::wstring> texts,
		EXE_FONT eFontconst = EXE_FONT::eUI_Font, HWND hWnd = nullptr) override
	{
		std::vector<uint32_t> cx_of_text;
		for (const std::wstring& txt : texts)
		{
			CSize sizeText = GetTextSizeW(eFontconst, txt.c_str(), (int)txt.size());
			cx_of_text.push_back(sizeText.cx);
		}
		return cx_of_text;
	}

	// Get max. text width of supplied strings using font.
	virtual uint32_t GetMaxTextWidthUsingFontW(const std::vector<std::wstring> texts,
		EXE_FONT eFontconst = EXE_FONT::eUI_Font, HWND hWnd = nullptr) override
	{
		std::vector<uint32_t> ln_cx = GetTextWidthsUsingFontW(texts, eFontconst, hWnd);
		auto it_max = std::max_element(ln_cx.begin(), ln_cx.end());
		return it_max != ln_cx.end() ? *it_max : 0;
	}

	virtual CSize GetMultilineTextExtentW(const wchar_t* szTxt,
			EXE_FONT eFontconst = EXE_FONT::eUI_Font, HWND hWnd = nullptr) override
	{
		CSize sizeTotal;

		size_t txtLen = wcslen(szTxt);
		const wchar_t* pEndOfData = szTxt + txtLen;

		int index = -1, brkLen;
		wchar_t cr = L'\r', lf = L'\n';
		const wchar_t* pSrc = szTxt;

		while (pSrc < pEndOfData)
		{
			brkLen = 0;
			int len = (int)(pEndOfData - pSrc);

			index = 0;
			const wchar_t* pLF = pSrc;
			while (pLF < pEndOfData)	// Find LF
			{
				if (*pLF == cr)
				{
					break;
				}
				++index;
				++pLF;
			}

			if (index >= 0)										// LF found?
			{
				brkLen = 1;
				if (index > 0 && *(pSrc + index - 1) == cr)		// LF preceeded by CR?
				{
					index--;
					brkLen = 2;
				}
			}

			CSize sizeOfLine = GetTextSizeW(eFontconst, pSrc, (index >= 0 ? index : len));
			sizeTotal.cy += sizeOfLine.cy;
			if (sizeOfLine.cx > sizeTotal.cx)
			{
				sizeTotal.cx = sizeOfLine.cx;
			}

			pSrc = (index >= 0) ? pSrc + index + brkLen : pEndOfData;
		}
		return sizeTotal;
	}

	//virtual int GetUI_FontGridRowHeight() const override { return m_uiFontGridRowHeight; }

	virtual MonospacedCharSize GetLogGridCharSize() const override
	{
		return m_monospacedCharSize;
	}

	virtual CXeTooltipIF* CreateTooltip(const std::wstring& nameForLogging, HWND hWndParent) override
	{
		return m_tooltips.Create(this, nameForLogging, hWndParent);
	}

	virtual void DestroyTooltip(HWND hWndParent) override
	{
		return m_tooltips.Destroy(hWndParent);
	}

	virtual void HideTooltip(HWND hWndParent)
	{
		return m_tooltips.HideTooltip(hWndParent);
	}

	virtual void HideOtherTooltips(HWND hWndTooltip) override
	{
		m_tooltips.HideOtherTooltips(hWndTooltip);
	}

	virtual int GetTooltipDefaultWidth() const override { return m_cxDefaultTooltip; }

protected:
	void _DeleteFonts()
	{
		m_d2d_fonts.ReleaseFonts();
	}

	bool _LogIfErrorAndAssert(HRESULT hr, const std::string& fn)
	{
		XeASSERT(hr == S_OK);
		if (!SUCCEEDED(hr))
		{
			logger().error("%s failed. Error code: 0x%08X", fn.c_str(), hr);
		}
		return SUCCEEDED(hr);
	}

	bool _CreateFonts()
	{
		_DeleteFonts();
		m_cxScaleFactor = m_cyScaleFactor = 1.0;

		// Handle deprecated font size settings - use new font point size unless user has selected font pixel size.
		//const CXeUserSetting& settingGridFontPointSize = s_xeUIsettings[L"LogFileGridSettings"].Get(L"GridFontPointSize");
		//m_monospacedFontSize = xen::stoi(settingGridFontPointSize.getString().c_str());
		//if (settingGridFontPointSize.m_value_is_default)
		//{
		//	const CXeUserSetting& settingGridFontSize = s_xeUIsettings[L"LogFileGridSettings"].Get(L"GridFontSize");
		//	if (!settingGridFontSize.m_value_is_default)	// Deprecated "GridFontSize" is not default value?
		//	{
		//		int fontSizePixels = xen::stoi(settingGridFontSize.getString().c_str());
		//		m_monospacedFontSize = (int)(((float)fontSizePixels / 1.3333f) + 0.5f);
		//	}
		//}
		const CXeUserSetting& settingUIFontPointSize = s_xeUIsettings[L"GeneralSettings"].Get(L"UI_FontPointSize");
		m_uiFontSize = xen::stoi(settingUIFontPointSize.getString().c_str());
		if (settingUIFontPointSize.m_value_is_default)
		{
			const CXeUserSetting& settingUIFontSize = s_xeUIsettings[L"GeneralSettings"].Get(L"UI_FontSize");
			if (!settingUIFontSize.m_value_is_default)	// Deprecated "UI_FontSize" is not default value?
			{
				int fontSizePixels = xen::stoi(settingUIFontSize.getString().c_str());
				m_uiFontSize = (int)(((float)fontSizePixels / 1.3333f) + 0.5f);
			}
		}
		m_monospacedFontSize = m_uiFontSize;

		std::wstring monospacedFontName = s_xeUIsettings[L"GeneralSettings"].Get(L"UI_MonospacedFontName").getString();
		//std::wstring monospacedFontName = s_xeUIsettings[L"LogFileGridSettings"].Get(L"GridFontName").getString();
		//std::wstring gridRowPitch = s_xeUIsettings[L"LogFileGridSettings"].Get(L"GridRowPitch").getString();
		//m_gridRowPitch = xen::stoi(gridRowPitch.c_str());

		std::wstring strUI_FontName = s_xeUIsettings[L"GeneralSettings"].Get(L"UI_FontName").getString();

		m_d2d_fonts.SetFontInfo(EXE_FONT::eMonospacedFont, XeFontMetrics(monospacedFontName, m_monospacedFontSize));

		m_d2d_fonts.SetFontInfo(EXE_FONT::eUI_Font, XeFontMetrics(strUI_FontName, m_uiFontSize));

		XeFontMetrics tmUIfontBold(strUI_FontName, m_uiFontSize, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_WEIGHT_BOLD);
		m_d2d_fonts.SetFontInfo(EXE_FONT::eUI_FontBold, tmUIfontBold);

		double dbl_size = m_uiFontSize;
		XeFontMetrics tmUIfontMedBig(strUI_FontName, (int)(dbl_size * 1.25 + 0.5),
				DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_WEIGHT_BOLD);// eMediumBig is 25% bigger font.
		m_d2d_fonts.SetFontInfo(EXE_FONT::eMediumBig, tmUIfontMedBig);

		XeFontMetrics tmUIfontTabTitle(strUI_FontName, (int)(dbl_size * 1.43 + 0.5),
				DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_WEIGHT_BOLD);// 43% bigger size than uiFontSize - default is 14 - resulting in 20
		m_d2d_fonts.SetFontInfo(EXE_FONT::eTabListTitleFont, tmUIfontTabTitle);

		XeFontMetrics tmUIfontBig(strUI_FontName, (int)(dbl_size * 2.85 + 0.5),
				DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_WEIGHT_BOLD);// 285% bigger size than uiFontSize - default is 14 - resulting in 40
		m_d2d_fonts.SetFontInfo(EXE_FONT::eBigMsgFont, tmUIfontBig);

		// Get metrics for all fonts.
		HDC hDC = ::CreateCompatibleDC(0);
		m_logPixelsY = ::GetDeviceCaps(hDC, LOGPIXELSY);
		XeASSERT(m_logPixelsY > 1);
		::DeleteDC(hDC);

		// Honestly stolen from: https://chromium.googlesource.com/chromium/src/+/51ba3e6902ecd7284d3be51db648127d1be2187f/ui/gfx/platform_font_win.cc
		Microsoft::WRL::ComPtr<IDWriteFontCollection> font_collection;
		HRESULT hrFC = m_d2d_writeFactory->GetSystemFontCollection(font_collection.GetAddressOf());
		if (!_LogIfErrorAndAssert(hrFC, "GetSystemFontCollection"))
		{
			return false;
		}
		std::wstring dt(L"0000-00-00 00:00:00,000");
		std::wstring ab(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
		for (size_t i = 0; i < static_cast<size_t>(EXE_FONT::eLastFont); ++i)
		{
			EXE_FONT eFont = static_cast<EXE_FONT>(i);

			XeFontMetrics tm = m_d2d_fonts.GetFontInfoConst(eFont);
			std::string fontnameA = xet::toUTF8(tm.m_fontName);

			float fsDIPs = (float)tm.m_fontPointSize * FONT_PT_TO_DIPS;	// font size is in points - D2D wants DIPs.

			// Create font object (note - is managed by font map class).
			IDWriteTextFormat* pTextFmt = nullptr;
			HRESULT hr = m_d2d_writeFactory->CreateTextFormat(tm.m_fontName.c_str(), 0, tm.m_weight, tm.m_style,
					tm.m_stretch, fsDIPs, L"", &pTextFmt);
			if (_LogIfErrorAndAssert(hr, std::string("CreateTextFormat ") + fontnameA))
			{
				m_d2d_fonts.SetFont(eFont, pTextFmt);
			}

			D2D1_SIZE_F sizeF, sizeF_ab;
			HRESULT hr1 = D2D_GetTextSize(dt, eFont, sizeF);
			HRESULT hr2 = D2D_GetTextSize(ab, eFont, sizeF_ab);
			XeASSERT(hr1 == S_OK && hr2 == S_OK);
			if (_LogIfErrorAndAssert(hr1, "GetTextSize " + fontnameA) && SUCCEEDED(hr2))
			{
				tm.m_aveCharWidth = (int)(sizeF_ab.width / (float)ab.size() + 0.5f);
				tm.m_height = (int)(sizeF.height + 0.5f);
				tm.m_sizeDatetime.cx = (int)(sizeF.width + 0.5f), tm.m_sizeDatetime.cy = (int)(sizeF.height + 0.5f);

				// Calculate some guessed values - in case get font metrics fails.
				tm.m_descent = (int)(sizeF.height / 6.0f + 0.5f);
				tm.m_ascent = tm.m_height - tm.m_descent;
				tm.m_intLeading = (int)(sizeF.height / 6.0f + 0.5f);
				tm.m_extLeading = 0;
			}

			if (eFont == EXE_FONT::eUI_Font)
			{
				double cyUIfont = tm.GetHeight();
				//m_uiFontGridRowHeight = tm.GetHeight() + tm.GetInternalLeading();

				// Calculate Data Grid row sizes.
				s_cyGridTH_RowHeight = (int)(cyUIfont * 1.125);//tm.GetHeight() + m_gridRowPitch;

				double scaleFactor = (double)cyUIfont / 18.0;
				if (scaleFactor > 1.0)
				{
					m_cxScaleFactor = m_cyScaleFactor = scaleFactor;
				}
				m_cxDefaultTooltip = (int)(212.0 * m_cxScaleFactor + 0.5);
			}
			else if (eFont == EXE_FONT::eMonospacedFont)
			{
				// Set fall-back values in case calculate metrics fails.
				//m_monospacedCharSize.cySingleLineRowHeight = tm.GetHeight() + m_gridRowPitch;
				m_monospacedCharSize.cyMultiLineTextHeight = tm.GetHeight();
				//m_monospacedCharSize.cyMultiLineTextTopMargin = (int)((double)tm.GetHeight() / 8.0 + 0.5);
				//m_monospacedCharSize.cyMultiLineTextBottomMargin = (int)((double)tm.GetHeight() / 8.0 + 0.5);
				m_monospacedCharSize.cxGridCharWidth = tm.m_aveCharWidth;
			}

			if (SUCCEEDED(hrFC))
			{
				_CalculateFontMetrics(eFont, fsDIPs, tm, font_collection.Get());
			}
			m_d2d_fonts.SetFontInfo(eFont, tm);
		}
		return true;
	}

	void _CalculateFontMetrics(EXE_FONT eFont, const float& fsDIPs, XeFontMetrics& tm, IDWriteFontCollection* pCollection)
	{
		std::string fontnameA = xet::toUTF8(tm.m_fontName);
		BOOL exists = FALSE;
		uint32_t index = 0;
		HRESULT hr = pCollection->FindFamilyName(tm.m_fontName.c_str(), &index, &exists);
		if (!_LogIfErrorAndAssert(hr, "FindFamilyName " + fontnameA))
		{
			return;
		}
		Microsoft::WRL::ComPtr<IDWriteFontFamily> font_family;
		if (index != UINT_MAX && exists)
		{
			hr = pCollection->GetFontFamily(index, font_family.GetAddressOf());
		}
		else
		{
			// If we fail to find a matching font, then fallback to the first font in
			// the list. This is what skia does as well.
			hr = pCollection->GetFontFamily(0, font_family.GetAddressOf());
		}
		if (!_LogIfErrorAndAssert(hr, "GetFontFamily " + fontnameA))
		{
			return;
		}
		Microsoft::WRL::ComPtr<IDWriteFont> dwrite_font;
		hr = font_family->GetFirstMatchingFont(tm.m_weight, tm.m_stretch, tm.m_style, dwrite_font.GetAddressOf());
		if (!_LogIfErrorAndAssert(hr, "GetFirstMatchingFont " + fontnameA))
		{
			return;
		}

		dwrite_font->GetMetrics(&tm.m_dfm);

		tm.m_ascent = (int)(tm.m_dfm.ascent * fsDIPs / tm.m_dfm.designUnitsPerEm + 0.5f);
		tm.m_descent = (int)(tm.m_dfm.descent * fsDIPs / tm.m_dfm.designUnitsPerEm + 0.5f);
		int capHeightPixels = (int)(tm.m_dfm.capHeight * fsDIPs / tm.m_dfm.designUnitsPerEm + 0.5f);
		tm.m_intLeading = tm.m_ascent - capHeightPixels;
		tm.m_extLeading = (int)(tm.m_dfm.lineGap * fsDIPs / tm.m_dfm.designUnitsPerEm + 0.5f);

		if (eFont == EXE_FONT::eMonospacedFont)
		{
			Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace;
			hr = dwrite_font->CreateFontFace(fontFace.GetAddressOf());
			if (_LogIfErrorAndAssert(hr, "CreateFontFace"))
			{
				uint32_t codepoints[2]{ wchar_t{L'M'}, wchar_t{L'y'} };
				uint16_t glyphIdc[2]{ 0, 0 };

				hr = fontFace->GetGlyphIndicesW(codepoints, 2, glyphIdc);
				if (_LogIfErrorAndAssert(hr, "GetGlyphIndicesW"))
				{
					BOOL useGdiNatural = TRUE;
					// When set to FALSE, the metrics are the same as the metrics of GDI aliased text.
					// When set to TRUE, the metrics are the same as the metrics of text measured by GDI
					// using a font created with CLEARTYPE_NATURAL_QUALITY.

					DWRITE_GLYPH_METRICS glyph_meterics[2]{ 0, 0 };
					hr = fontFace->GetGdiCompatibleGlyphMetrics(fsDIPs, 1.0f, nullptr, useGdiNatural, glyphIdc, 2, glyph_meterics);
					if (_LogIfErrorAndAssert(hr, "GetGdiCompatibleGlyphMetrics"))
					{
						float cyHeight = (float)glyph_meterics[0].advanceHeight * 1.075f;
						float cy = cyHeight * fsDIPs / (float)tm.m_dfm.designUnitsPerEm;
						//m_monospacedCharSize.cySingleLineRowHeight = (int)(cy + 0.5) + m_gridRowPitch;
						m_monospacedCharSize.cyMultiLineTextHeight = (int)(cy + 0.5);
						//m_monospacedCharSize.cyMultiLineTextTopMargin = (int)(cy / 8.0 + 0.5);
						//m_monospacedCharSize.cyMultiLineTextBottomMargin = (int)(cy / 8.0 + 0.5);
						m_monospacedCharSize.cxGridCharWidth
								= (int)((float)glyph_meterics[0].advanceWidth * fsDIPs / tm.m_dfm.designUnitsPerEm + 0.5f);
					}
				}
			}
		}
	}
#pragma endregion Fonts

#pragma region Cursor
	virtual HCURSOR GetAppCursor(bool isGrid = false) const override
	{
		switch (m_eAppCursor)
		{
		case ECURSOR::eArrow:
			if (isGrid)
			{
				return LoadCursor(NULL, IDC_IBEAM);
			}
			return LoadCursor(NULL, IDC_ARROW);
		case ECURSOR::eWaitArrow:
			return LoadCursor(NULL, IDC_APPSTARTING);
		case ECURSOR::eWait:
			return LoadCursor(NULL, IDC_WAIT);
		}
		return LoadCursor(NULL, IDC_ARROW);
	}

	virtual void SetAppCursor(ECURSOR eCursor) override
	{
		m_eAppCursor = eCursor;
		::SetCursor(GetAppCursor(false));
	}

	//virtual BOOL OnSetCursorHelper(CWnd* pCallingWnd, CWnd* pWndInMsg, UINT nHitTest, UINT message) override
	//{
	//	return OnSetCursorHelper(pCallingWnd->GetSafeHwnd(), pWndInMsg->GetSafeHwnd(), nHitTest, message);
	//}
	
	virtual BOOL OnSetCursorHelper(HWND hCallingWnd, HWND hWndInMsg, UINT nHitTest, UINT message) override
	{
		if ((nHitTest >= HTLEFT && nHitTest <= HTBORDER) || nHitTest == HTGROWBOX)
		{
			::DefWindowProc(hCallingWnd, WM_SETCURSOR,
				(WPARAM)hWndInMsg, MAKELPARAM(nHitTest, message));
		}
		else
		{
			// Post WM_LBUTTONDOWN message to a dialog that wants to know if user L clicked outside it's window.
			if (m_hWndDialogWantsClickOutsideMouseDown)
			{
				HWND hWndDlg = m_hWndDialogWantsClickOutsideMouseDown;
				if (hWndDlg && ::IsWindow(hWndDlg))
				{
					if (m_hWndDialogWantsClickOutsideMouseDown != hCallingWnd
						&& (message == WM_LBUTTONDOWN || message == WM_MBUTTONDOWN || message == WM_RBUTTONDOWN))
					{
						::PostMessage(hWndDlg, m_uDialogWantsClickOutsideMouseDownMessage, 0, 0);
					}
				}
				else
				{
					m_hWndDialogWantsClickOutsideMouseDown = nullptr;
				}
			}
			//TRACE("OnSetCursorHelper - %d\n", m_eAppCursor);
			SetCursor(GetAppCursor());
		}
		return TRUE;
	}
#pragma endregion Cursor

#pragma region Theme_support
public:
	virtual XeThemeIF* GetCurrentTheme() override { return &m_theme; }

	virtual std::vector<std::wstring> GetThemeNames() const
	{
		std::vector<std::wstring> names;
		for (const ThemeInfoJson& theme_info : m_theme_list)
		{
			names.push_back(theme_info.m_theme_name);
		}
		return names;
	}

	virtual void ReloadThemeJsonFiles() override
	{
		_ReadAllThemeFiles();
	}

	virtual bool SetTheme(const std::wstring& theme_name) override
	{
		if (std::find_if(m_theme_list.cbegin(), m_theme_list.cend(),
			[&](const ThemeInfoJson& theme_info) { return theme_info.m_theme_name == theme_name; })
			== m_theme_list.cend())
		{
			_ReadAllThemeFiles();
		}
		const std::string* pThemeJsonData = _GetThemeJsonDataPtr(theme_name);
		if (!pThemeJsonData)
		{
			XeASSERT(FALSE);
			return false;
		}

		XeTheme theme_test;
		std::string colorIDs_json = _LoadColorIDsJson();
		std::wstring load_theme_report = theme_test.LoadFromJson(colorIDs_json, *pThemeJsonData, m_theme_list);
		if (load_theme_report.size() > 0)
		{
			XeASSERT(FALSE);
			logger().error("Load theme data failed: %s", xet::to_astr(load_theme_report).c_str());
			return false;
		}

		//_DeleteThemeData();
		_InitializeThemeData(theme_name);
		return true;
	}

	// Get color for CID identifier.
	virtual COLORREF GetColor(CID uCID) const override { return m_theme.GetColor(uCID); }
	virtual D2D1_COLOR_F GetColorF(CID uCID) const override
	{
		XeD2D1_COLOR_F clr;
		clr.SetFromRGB(GetColor(uCID), false);
		return clr;
	}
	//virtual HBRUSH GetHBRUSH(CID uCID) override { return m_theme.GetHBRUSH(uCID); }
	//virtual CBrush* GetBrush(CID uCID) override { return m_theme.GetBrush(uCID); }
	//virtual HPEN GetHPEN(CID uCID) override { return m_theme.GetHPEN(uCID); }
	//virtual CPen* GetPen(CID uCID) override { return m_theme.GetPen(uCID); }

	// Get background color for log level
	//virtual COLORREF GetLogLevelColor(LOGLEVEL logLevel) const override
	//{
	//	return GetColor(GetLogLevelColorId(logLevel));
	//}

	//virtual CBrush* GetLogLevelBrush(LOGLEVEL logLevel) override
	//{
	//	return GetBrush(GetLogLevelColorId(logLevel));
	//}

	//virtual CXeGdiPng* GetPngImage(PID uPID) const override
	//{
	//	XeASSERT(uPID < PID::PID_MAX);
	//	if (uPID < PID::PID_MAX)
	//	{
	//		XeASSERT(m_parrPngs[(UINT)uPID] != nullptr);
	//		return m_parrPngs[(UINT)uPID];
	//	}
	//	return m_parrPngs[0];
	//}

	virtual CSize GetPngImageSize(PID uPID) override
	{
		//CXeGdiPng* pPng = GetPngImage(uPID);
		//if (pPng)
		//{
		//	return pPng->GetSize();
		//}
		CSize sizeImg;
		IWICFormatConverter* pImg = D2D_GetImage(uPID, &sizeImg);
		if (pImg)
		{
			return sizeImg;
		}
		return CSize();
	}

	//virtual CXeGdiPng* GetFileIconPng(FileIcon icn) const override
	//{
	//	return m_parrPngs[(UINT)GetFileIconPID(icn)];
	//}

	//virtual CPen* GetPngBtnFocusPen() const override { return m_ppenDots.get(); }	// Shared 'focus' outline pen for PngButton
	//virtual CPen* GetPngBtnFocusPen() const override
	//{
	//	return CPen::FromHandle(m_pFocusDotsPen);
	//}	// Shared 'focus' outline pen for PngButton

	//virtual CXeGdiPng* GetCachedPngBtnImg(CString& strPngName) const override
	//{
	//	return m_pPngCache->AddPngToCache(strPngName);
	//}

	// Get string to append to PNG resource names - used by XePngButton class.
	//virtual CString GetThemePngAppend() const override
	//{
	//	return m_uThemeNumber == 1 ? CString("") : CString("_2");
	//}

	//virtual const LogGridPaintObj& GetLogGridPaintObj() const override
	//{
	//	return m_gridPaintObj;
	//}

protected:
	void _InitializeThemeData(const std::wstring& theme_name)
	{
		const std::string* pThemeJsonData = _GetThemeJsonDataPtr(theme_name);
		if (!pThemeJsonData)
		{
			if (m_theme_list.size() >= 2)
			{
				pThemeJsonData = &(m_theme_list[1].m_json_data);	// Default "fallback" theme is "Dark theme".
			}
			else
			{
				XeASSERT(FALSE);
				return;
			}
		} 
		std::string colorIDs_json = _LoadColorIDsJson();
		std::wstring load_theme_report = m_theme.LoadFromJson(colorIDs_json, *pThemeJsonData, m_theme_list);
		if (load_theme_report.size() > 0)
		{
			XeASSERT(FALSE);
			logger().error("Load theme data failed: %s", xet::to_astr(load_theme_report).c_str());
		}
		bool isLightTheme = (wcscmp(theme_name.c_str(), L"Light theme") == 0)
				|| (wcscmp(m_theme.GetThemeBasedOnName().c_str(), L"Light theme") == 0);
		//m_uThemeNumber = isLightTheme ? 1 : 2;

		//m_pTheme1PngArray = new CXeGdiPng * [(UINT)PID::PID_MAX]{ nullptr };
		//m_pTheme2PngArray = new CXeGdiPng * [(UINT)PID::PID_MAX]{ nullptr };
		//_LoadThemeImages();
		//XeASSERT(m_uThemeNumber == 1 || m_uThemeNumber == 2);
		//m_parrPngs = m_uThemeNumber == 1 ? m_pTheme1PngArray : m_pTheme2PngArray;

		//m_pPngCache = new CXeGdiPngCache;

		// Create 'dots' pen for PngButton focus rect.
		//if (m_pFocusDotsPen)
		//{
		//	::DeleteObject(m_pFocusDotsPen);
		//}
		//LOGBRUSH lb;
		//lb.lbColor = RGB(255, 0, 0);// GetColor(CID::CtrlFocusDots);
		//lb.lbStyle = BS_SOLID;
		//lb.lbHatch = 0;
		//m_pFocusDotsPen = ExtCreatePen(PS_COSMETIC | PS_DASHDOT, 1, &lb, 0, NULL);
		//m_ppenDots = std::make_unique<CPen>(PS_COSMETIC | PS_DOT, 1, &LogBrush);

		//_InitGridPaintObj();
	}

	//void _DeleteThemeData()
	//{
	//	for (UINT i = 0; i < (UINT)PID::PID_MAX; i++)
	//	{
	//		delete m_pTheme1PngArray[i];
	//		delete m_pTheme2PngArray[i];
	//	}
	//	delete m_pTheme1PngArray;
	//	delete m_pTheme2PngArray;

	//	//delete m_pPngCache;
	//	//m_pPngCache = 0;

	//	//if (m_pFocusDotsPen)
	//	//{
	//	//	::DeleteObject(m_pFocusDotsPen);
	//	//}
	//}

	std::string _LoadColorIDsJson()
	{
		uint32_t ids_size = 0;
		const char* ids_data = nullptr;
		LoadFileInResource(IDR_COLOR_IDS_JSON, JSON_FILE_TYPE, ids_size, ids_data);
		std::string colorIDs_json(ids_data, ids_size);
		return colorIDs_json;
	}

	void _ReadAllThemeFiles()
	{
		m_theme_list.clear();
		uint32_t ids_size = 0, th1_size = 0, th2_size = 0;
		const char* ids_data = NULL, * th1_data = nullptr, * th2_data = nullptr;
		bool isOk1 = LoadFileInResource(IDR_COLOR_IDS_JSON, JSON_FILE_TYPE, ids_size, ids_data);
		bool isOk2 = LoadFileInResource(IDR_THEME1_JSON, JSON_FILE_TYPE, th1_size, th1_data);
		bool isOk3 = LoadFileInResource(IDR_THEME2_JSON, JSON_FILE_TYPE, th2_size, th2_data);
		if (!(isOk1 && isOk2 && isOk3))
		{
			logger().error("Failed to load One or more theme resources from exe file");
		}

		std::string colorIDs_json(ids_data, ids_size), theme1_json(th1_data, th1_size), theme2_json(th2_data, th2_size);
		_ReadThemeInfos({ theme1_json, theme2_json });
		_ReadAllUserThemeInfos();
	}

	const std::string* _GetThemeJsonDataPtr(const std::wstring& theme_name)
	{
		for (const ThemeInfoJson& theme_info : m_theme_list)
		{
			if (theme_name == theme_info.m_theme_name)
			{
				return &theme_info.m_json_data;
			}
		}
		return nullptr;
	}

	void _ReadThemeInfos(const std::vector<std::string>& themes_json)
	{
		for (const std::string& theme_json : themes_json)
		{
			auto theme_info = XeTheme::ReadThemeInfo(theme_json, L"");
			if (theme_info.has_value())
			{
				m_theme_list.push_back(theme_info.value());
			}
			else
			{
				XeASSERT(FALSE);
			}
		}
	}

	void _ReadAllUserThemeInfos()
	{
		std::vector<std::wstring> files;
		std::wstring user_data_folder = GetCurrentUserAppDataFolder(m_appName);
		std::wstring json_search = user_data_folder + L"Theme_*.json";
		WIN32_FIND_DATAW ffd;
		HANDLE hFind = ::FindFirstFileW(json_search.c_str(), &ffd);
		if (INVALID_HANDLE_VALUE != hFind)
		{
			do
			{
				std::wstring pathname = user_data_folder + ffd.cFileName;
				files.push_back(pathname);
			} while (::FindNextFileW(hFind, &ffd) != 0);
			::FindClose(hFind);
		}
		for (const std::wstring& pathname : files)
		{
			std::string file_data = GetSmallTextFile(pathname);
			auto theme_info = XeTheme::ReadThemeInfo(file_data, pathname);
			if (theme_info.has_value())
			{
				m_theme_list.push_back(theme_info.value());
			}
			else
			{
				XeASSERT(FALSE);
			}
		}
	}

	//void _InitGridPaintObj()
	//{
	//	m_gridPaintObj.m_pCellBorderPen = GetPen(CID::GrdExcelBrd);
	//	m_gridPaintObj.m_pDefBg = GetBrush(CID::GrdCellDefBg);
	//	m_gridPaintObj.m_pSelCellBg = GetBrush(CID::GrdHdrCurCell);
	//	m_gridPaintObj.m_rgbDefTxt = GetColor(CID::GrdCellDefTxt);
	//	m_gridPaintObj.m_pDefTxtPen = GetPen(CID::GrdCellDefTxt);
	//	m_gridPaintObj.m_pDefTxtBrush = GetBrush(CID::GrdCellDefTxt);
	//	m_gridPaintObj.m_pDToffsetBg = GetBrush(CID::DateOffsetBg);
	//	m_gridPaintObj.m_pMoreDataBg = GetBrush(CID::LogEvBg);
	//	m_gridPaintObj.m_pNoDataBg = GetBrush(CID::MrgLogNoDataBg);
	//	m_gridPaintObj.m_pHilite1FilterBg = GetBrush(CID::HiliteFilterBg);
	//	m_gridPaintObj.m_pHilite2FilterBg = GetBrush(CID::Hilite2FilterBg);
	//	m_gridPaintObj.m_pHilite3FilterBg = GetBrush(CID::Hilite3FilterBg);
	//	m_gridPaintObj.m_pTimeGapBrush = GetBrush(CID::WarnBg);
	//	m_gridPaintObj.m_pTimeJumpBrush = GetBrush(CID::ErrorBg);
	//	m_gridPaintObj.m_pAboveFilteredBrush = GetBrush(CID::LogFilterHdrBg);
	//	m_gridPaintObj.m_pBlankBg = GetBrush(CID::GrdBlankBG);

	//	m_gridPaintObj.m_pPngMultilineExpBtn = GetPngImage(PID::MultilineExpBtn);
	//	m_gridPaintObj.m_pPngMultilineClpsBtn = GetPngImage(PID::MultilineClpsBtn);
	//	m_gridPaintObj.m_pPngSimpleBookmark = GetPngImage(PID::BookmarkTogglePng);
	//	m_gridPaintObj.m_pPngCommentBookmark = GetPngImage(PID::BookmarkToggleWithCommentPng);
	//	m_gridPaintObj.m_pSH_DefBg = GetBrush(CID::GrdHdrDefBg);
	//	m_gridPaintObj.m_pSH_CurRowBg = GetBrush(CID::GrdHdrCurCell);
	//	m_gridPaintObj.m_pSH_CurRowInActPaneBg = GetBrush(CID::GrdHdrCurCellInActPane);
	//	m_gridPaintObj.m_rgbSH_DefTxt = GetColor(CID::GrdSHDefTxt);
	//	m_gridPaintObj.m_rgbSH_CurRowTxt = GetColor(CID::GrdSHCurTxt);
	//	m_gridPaintObj.m_pThreeDLightPen = GetPen(CID::GrdHdrBrd);
	//	m_gridPaintObj.m_pThreeDDarkPen = GetPen(CID::GrdHdrBrd);
	//}

	std::wstring _GetPIDresourceName(PID pid)
	{
		switch (pid)
		{
		case PID::LogFilterPng:                  return L"LOG_FILTER_PNG";
		case PID::TimelineCenter:                return L"TL_CENTER_THEME1";
		case PID::TimelineZoomIn:                return L"TL_ZOOMIN_THEME1";
		case PID::TimelineZoomOut:               return L"TL_ZOOMOUT_THEME1";
		case PID::unused5:                       return L"LOG_FILTER_PNG";
		case PID::eRequested:                    return L"IDB_eRequested";
		case PID::eQueued:                       return L"IDB_eQueued";
		case PID::eOpening:                      return L"IDB_eOpening";
		case PID::eUnzipping:                    return L"IDB_eUnzipping";
		case PID::eParsing:                      return L"IDB_eParsing";
		case PID::eMerging:                      return L"IDB_eMerging";
		case PID::eIndexing:                     return L"IDB_eIndexing";
		case PID::eDone:                         return L"IDB_eDone";
		case PID::eDoneAlreadyOpen:              return L"IDB_eDoneAlreadyOpen";
		case PID::eErrorFileNotFound:            return L"IDB_eErrorFileNotFound";
		case PID::eErrorNetworkPathInvalid:      return L"IDB_eErrorNetworkPathInvalid";
		case PID::eErrorAccessDenied:            return L"IDB_eErrorAccessDenied";
		case PID::eErrorOther:                   return L"IDB_eErrorOther";
		case PID::eErrorUnsupportedFile:         return L"IDB_eErrorOther";
		case PID::Cancelled:                     return L"IDB_eCancelled";
		case PID::eErrorOutOfMemory:             return L"IDB_eErrorOther";
		case PID::BookmarkTogglePng:             return L"BOOKMARK_TOGGLE_THEME1";
		case PID::BookmarkPrevPng:               return L"BOOKMARK_PREV_THEME1";
		case PID::BookmarkNextPng:               return L"BOOKMARK_NEXT_THEME1";
		case PID::BookmarksClearPng:             return L"BOOKMARKS_CLEAR_THEME1";
		case PID::BookmarkToggleWithCommentPng:  return L"BOOKMARK_TOGGLE_WITH_COMMENT_THEME1";
		case PID::MergePng:                      return L"TB_PNG_MERGE";
		case PID::MultilineExpBtn:               return L"MULTILINE_EXP_BTN";
		case PID::MultilineClpsBtn:              return L"MULTILINE_CLPS_BTN";
		case PID::unused30:                      return L"LOG_FILTER_PNG";
		case PID::BookmarkCurrentPng:            return L"BOOKMARK_CURRENT_THEME1";
		case PID::BookmarkCurrentWithCommentPng: return L"BOOKMARK_CURRENT_WITH_COMMENT_THEME1";
		case PID::TimeJumpWarningPng:            return L"TIME_JUMP_WARNING_THEME1";
		case PID::RebuildMergeNeededPng:         return L"REBUILD_MERGE_NEEDED_THEME1";
		case PID::eIMGUI_RESET_PNG:              return L"IMGUI_RESET_THEME1";
		case PID::eIMGUI_ZOOM_IN_PNG:            return L"IMGUI_ZOOM_IN_THEME1";
		case PID::eIMGUI_ZOOM_OUT_PNG:           return L"IMGUI_ZOOM_OUT_THEME1";
		case PID::eIMGUI_ROTATE_LEFT_PNG:        return L"IMGUI_ROTATE_LEFT_THEME1";
		case PID::eIMGUI_ROTATE_RIGHT_PNG:       return L"IMGUI_ROTATE_RIGHT_THEME1";
		case PID::LogFileError:                  return L"LOG_FILE_ERROR_THEME1";
		case PID::FileOpen:                      return L"TB_PNG_FILE_OPEN";
		case PID::AppAbout:                      return L"TB_PNG_ABOUT";
		case PID::TimeOffset:                    return L"TB_TIME_OFFSET";
		case PID::Find:                          return L"TB_FIND";
		case PID::TOGGLE_BOOKMARK:               return L"BOOKMARK_TOGGLE_THEME1";
		case PID::TOGGLE_BOOKMARK_COMMENT:       return L"BOOKMARK_TOGGLE_WITH_COMMENT_THEME1";
		case PID::PREV_BOOKMARK:                 return L"BOOKMARK_PREV_THEME1";
		case PID::NEXT_BOOKMARK:                 return L"BOOKMARK_NEXT_THEME1";
		case PID::CLEAR_BOOKMARKS:               return L"BOOKMARKS_CLEAR_THEME1";
		case PID::FILTER_OFF:                    return L"TB_FILTER_OFF";
		case PID::FILTER_ON:                     return L"TB_FILTER_ON";
		case PID::FILTER_CLEAR_ALL:              return L"TB_FILTER_CLEAR_ALL";
		case PID::FILTER_QUICK_SET:              return L"TB_FILTER_QUICK_SET";
		case PID::FILTER_QUICK_CLEAR:            return L"TB_FILTER_QUICK_CLEAR";
		case PID::MultilineExpBtn20x20:          return L"MULTILINE_EXP_BTN20x20";
		case PID::MultilineClpsBtn20x20:         return L"MULTILINE_CLPS_BTN20x20";
		case PID::FindPrev:                      return L"TB_FIND_PREV";
		case PID::FindNext:                      return L"TB_FIND_NEXT";
		case PID::FindAll:                       return L"TB_FIND_ALL";
		case PID::FindSave:                      return L"TB_FIND_SAVE";
		case PID::FindRemove:                    return L"TB_FIND_REMOVE";
		case PID::FindClose:                     return L"TB_FIND_CLOSE";
		case PID::AddToHiliteList:               return L"TB_ADD_TO_HILITE_LIST";
		case PID::EditHiliteList:                return L"TB_EDIT_HILITE_LIST";
		case PID::eRm_Connecting:                return L"IDB_eRm_Connecting";
		case PID::eRm_Reading:                   return L"IDB_eRm_Reading";
		case PID::eErrorRmConnect:               return L"IDB_eErrorRmConnect";
		case PID::eErrorRmReadFailed:            return L"IDB_eErrorRmReadFailed";
		case PID::eErrorRmWriteLocalFailed:      return L"IDB_eErrorRmWriteLocalFailed";
		case PID::StopFileRefresh:               return L"TB_STOP_FILE_REFRESH";
		case PID::StartFileRefresh:              return L"TB_START_FILE_REFRESH";
		case PID::eRmConnected:                  return L"IDB_eRmConnected";
		case PID::FileIconServer:                return L"FILEICON_SERVER_IMG";
		case PID::FileIconApp:                   return L"FILEICON_APP_IMG";
		case PID::FileIconConsole:               return L"FILEICON_CONSOLE_IMG";
		case PID::FileIconUser:                  return L"FILEICON_USER_IMG";
		case PID::FileIconService:               return L"FILEICON_SERVICE_IMG";
		case PID::FilterDeactivate:              return L"TB_FILTER_DEACTIVATE";
		case PID::FilterActivate:                return L"TB_FILTER_ACTIVATE";
		case PID::FontSize:                      return L"TB_FONT_SIZE";
		case PID::CWF_BTN_MIN_DN :               return L"CWF_BTN_MIN_DN";
		case PID::CWF_BTN_MIN_HOT:               return L"CWF_BTN_MIN_HOT";
		case PID::CWF_BTN_MIN_UP :               return L"CWF_BTN_MIN_UP";
		case PID::CWF_BTN_MAX_DN :               return L"CWF_BTN_MAX_DN";
		case PID::CWF_BTN_MAX_HOT:               return L"CWF_BTN_MAX_HOT";
		case PID::CWF_BTN_MAX_UP :               return L"CWF_BTN_MAX_UP";
		case PID::CWF_BTN_RST_DN :               return L"CWF_BTN_RST_DN";
		case PID::CWF_BTN_RST_HOT:               return L"CWF_BTN_RST_HOT";
		case PID::CWF_BTN_RST_UP :               return L"CWF_BTN_RST_UP";
		case PID::CWF_BTN_CLS_DN :               return L"CWF_BTN_CLS_DN";
		case PID::CWF_BTN_CLS_HOT:               return L"CWF_BTN_CLS_HOT";
		case PID::CWF_BTN_CLS_UP :               return L"CWF_BTN_CLS_UP";
		case PID::EL_Operational:                return L"IDB_EL_OPERATIONAL";
		case PID::EL_Admin      :                return L"IDB_EL_ADMIN";
		case PID::EL_Analytic   :                return L"IDB_EL_ANALYTIC";
		case PID::EL_Debug      :                return L"IDB_EL_DEBUG";
		case PID::FileIconDevice:				 return L"FILEICON_DEVICE_IMG";
		case PID::FilterRecall:				     return L"TB_FILTER_RECALL";
		}
		XeASSERT(false);	// PID with no PNG resource - fix it!
		return L"LOG_FILTER_PNG";
	}

	//bool _LoadThemeImages()
	//{
	//	// Load PNG images
	//	bool isOk = true;
	//	std::string err = "Failed to load Theme image(s):\n";
	//	m_parrPngs = m_pTheme1PngArray;
	//	std::vector<PID> pids = GetAllPIDs();
	//	for (PID pid : pids)
	//	{
	//		_LoadThemePngImage(pid, _GetPIDresourceName(pid));
	//	}

	//	m_parrPngs = m_pTheme2PngArray;
	//	for (PID pid : pids)
	//	{
	//		std::wstring res_name = _GetPIDresourceName(pid);
	//		if (!_LoadThemePngImage(pid, res_name))
	//		{
	//			isOk = false;
	//			err += xet::to_astr(res_name) + "\n";
	//		}
	//	}
	//	if (!isOk)
	//	{
	//		XeASSERT(false);
	//		logger().error(err);
	//	}
	//	return isOk;
	//}

	//bool _LoadThemePngImage(PID uPID, const std::wstring& resourceName)
	//{
	//	XeASSERT(uPID < PID::PID_MAX);
	//	XeASSERT(m_parrPngs[(UINT)uPID] == nullptr);
	//	m_parrPngs[(UINT)uPID] = new CXeGdiPng();
	//	return m_parrPngs[(UINT)uPID]->LoadPNG(resourceName.c_str());
	//}

#pragma endregion Theme_support

#pragma region Misc
	virtual void SetDialogWantsClickOutsideMouseDown(HWND hWndDlg, UINT message) override
	{
		m_hWndDialogWantsClickOutsideMouseDown = hWndDlg;
		m_uDialogWantsClickOutsideMouseDownMessage = message;
	}

	virtual void ShowScrollbarMenu(HWND hParentWnd, bool isHorizontalSB, CPoint ptScreen) override
	{
		std::vector<ListBoxExItem> list{ ListBoxExItem(XSB_IDM_SCRHERE, L"Scroll Here") };
		if (isHorizontalSB)
		{
			list.push_back(ListBoxExItem(XSB_IDM_SCR_TL, L"Left Edge", IsSeparator::yes));
			list.push_back(ListBoxExItem(XSB_IDM_SCR_BR, L"Right Edge"));
			list.push_back(ListBoxExItem(XSB_IDM_SCR_PG_UL, L"Page Left", IsSeparator::yes));
			list.push_back(ListBoxExItem(XSB_IDM_SCR_PG_DR, L"Page Right"));
			list.push_back(ListBoxExItem(XSB_IDM_SCR_UL, L"Scroll Left", IsSeparator::yes));
			list.push_back(ListBoxExItem(XSB_IDM_SCR_DR, L"Scroll Right"));
		}
		else
		{
			list.push_back(ListBoxExItem(XSB_IDM_SCR_TL, L"Top", IsSeparator::yes));
			list.push_back(ListBoxExItem(XSB_IDM_SCR_BR, L"Bottom"));
			list.push_back(ListBoxExItem(XSB_IDM_SCR_PG_UL, L"Page Up", IsSeparator::yes));
			list.push_back(ListBoxExItem(XSB_IDM_SCR_PG_DR, L"Page Down"));
			list.push_back(ListBoxExItem(XSB_IDM_SCR_UL, L"Scroll Up", IsSeparator::yes));
			list.push_back(ListBoxExItem(XSB_IDM_SCR_DR, L"Scroll Down"));
		}
		CXeMenu menu(this, list);
		menu.ShowMenu(hParentWnd, ptScreen, 0);
	}

	virtual void OnChangedSettings(const ChangedSettings& chg_settings) override
	{
		if (IsFontSettingsChanged(chg_settings))
		{
			_CreateFonts();
		}
		else if (chg_settings.IsChanged(L"Colors", L"Color"))
		{
			//m_theme.DeletePensAndBrushes();
			//_InitGridPaintObj();
		}
	}

	virtual int GetValue(UIV val) const override
	{
		switch (val)
		{
		case UIV::s_cyGridTH_RowHeight:
			return s_cyGridTH_RowHeight;
		case UIV::cxTooltipDefault:
			return (int)((double)m_cxTooltipDefaultUnscaled * m_cxScaleFactor + 0.5);
		case UIV::cxComboBoxButton:
			return (int)((double)18 * m_cxScaleFactor + 0.5);
		case UIV::cxComboBoxButtonArrow: {
			int cxTri = (int)((double)8 * m_cxScaleFactor + 0.5);
			if ((cxTri & 1)) { ++cxTri; }			// Must be even number
			return cxTri; }
		case UIV::cySubMenuArrow: {
			int cxTri = (int)((double)6 * m_cxScaleFactor + 0.5);
			if ((cxTri & 1)) { ++cxTri; }			// Must be even number
			return cxTri; }
		case UIV::cxyThickBorder:
			return 6;
		case UIV::cxyThinBorder:
			return 3;
		case UIV::cyWindowCaption:
			return GetFontMetric(EXE_FONT::eUI_FontBold).GetHeight();
		}
		XeASSERT(FALSE);
		return 0;
	}

	virtual HICON GetAppIconHandle() override
	{
		XeASSERT(m_hMainWnd);
		HICON hIconWnd = (HICON)::SendMessage(m_hMainWnd, WM_GETICON, ICON_SMALL, 0);
		if (!hIconWnd)
			hIconWnd = (HICON)::SendMessage(m_hMainWnd, WM_GETICON, ICON_SMALL2, 0);
		if (!hIconWnd)
			hIconWnd = (HICON)::SendMessage(m_hMainWnd, WM_GETICON, ICON_BIG, 0);
		if (!hIconWnd)
			hIconWnd = (HICON)GetClassLongPtr(m_hMainWnd, GCLP_HICONSM);
		if (!hIconWnd)
			hIconWnd = (HICON)GetClassLongPtr(m_hMainWnd, GCLP_HICON);
		//if (!hIconWnd && !m_bThisIsMainWnd)
		//{	// Use Icon of parent window if possible.
		//	HWND hParentWnd = ::GetParent(m_hWnd);
		//	hIconWnd = (HICON)GetClassLongPtr(hParentWnd, GCLP_HICONSM);
		//}
#pragma warning (disable:4302)
		if (!hIconWnd)
			hIconWnd = ::LoadIcon(0, MAKEINTRESOURCE(IDI_APPLICATION));
#pragma warning (default:4302)
		return hIconWnd;
	}

	//virtual ColorMatrix* GetColorMatrix(CMTX cmtx) const override
	//{
	//	switch (cmtx)
	//	{
	//	case CMTX::Hot:
	//		return &s_clrmtxHot;
	//	case CMTX::Gray:
	//		return &s_clrmtxGray;
	//	case CMTX::Fg:
	//		return &s_clrmtxFg;
	//	case CMTX::FgNormal:
	//		return &s_clrmtxFgNormal;
	//	}
	//	XeASSERT(FALSE);
	//	return &s_clrmtxFgNormal;
	//}
#pragma endregion Misc

#pragma region Direct2D
/*
Idea - support SVG files - see: https://stackoverflow.com/questions/75917247/convert-svg-files-to-bitmap-using-direct2d-in-mfc

*/
protected:
	void _InitializeDirect2D()
	{
		HRESULT hr;
		// If you set the options.debugLevel to D2D1_DEBUG_LEVEL_NONE, the debug layer is not enabled.
#if _DEBUG
		D2D1_FACTORY_OPTIONS options;
		options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, m_d2d_fa.GetAddressOf());
#else
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2d_fa.GetAddressOf());
#endif
		XeASSERT(hr == S_OK);
		//D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_MULTI_THREADED, &m_d2d_fa);
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&m_d2d_writeFactory);
		XeASSERT(hr == S_OK);

		// Create WIC factory
		hr = ::CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IWICImagingFactory), reinterpret_cast<void**>(m_d2d_IWICFactory.GetAddressOf()));
		//hr = ::CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_d2d_IWICFactory));
		XeASSERT(hr == S_OK);
	}

	void _UninitializeDirect2D()
	{
		m_d2d_pid_map_img.clear();
		m_d2d_fonts.Clear();
		m_d2d_IWICFactory.Reset();
		m_d2d_writeFactory.Reset();
		m_d2d_fa.Reset();
	}

	D2D1_RENDER_TARGET_PROPERTIES _CreateRenderTargetProperties() const
	{
		// Create a pixel format and initialize its format and alphaMode fields.
		D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);

		D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
		props.pixelFormat = pixelFormat;
		props.dpiX = 96;
		props.dpiY = 96;
		return props;
	}

public:
	virtual ID2D1Factory* D2D_GetFactory() const override
	{
		return m_d2d_fa.Get();
	}
	virtual IDWriteFactory* D2D_GetWriteFactory() const override
	{
		return m_d2d_writeFactory.Get();
	}
	virtual IWICImagingFactory* D2D_GetWICFactory() const override
	{
		return m_d2d_IWICFactory.Get();
	}
	virtual HRESULT D2D_CreateDCRenderTarget(_COM_Outptr_ ID2D1DCRenderTarget** dcRenderTarget) const override
	{
		D2D1_RENDER_TARGET_PROPERTIES props = _CreateRenderTargetProperties();
		HRESULT hr = m_d2d_fa->CreateDCRenderTarget(&props, dcRenderTarget);
		XeASSERT(hr == S_OK);
		return hr;
	}

	virtual IDWriteTextFormat* D2D_GetFont(EXE_FONT font) override
	{
		if (!m_d2d_fonts.HasFont(font))
		{
			XeASSERT(false);	// All fonts should have been created at this point.
			return m_d2d_fonts.GetFont(EXE_FONT::eUI_Font);
		}
		return m_d2d_fonts.GetFont(font);
	}

	virtual HRESULT D2D_GetTextSize(const std::wstring& txt, EXE_FONT font, D2D1_SIZE_F& size) override
	{
		HRESULT hr = S_OK;
		IDWriteTextFormat* pUIfont = D2D_GetFont(font);
		Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
		float cxDim = 10000.0f, cyDim = 1000.f;
		// Create a text layout
		if (font != EXE_FONT::eMonospacedFont)
		{
			hr = m_d2d_writeFactory->CreateTextLayout(txt.c_str(), (UINT32)txt.size(),
					pUIfont, cxDim, cyDim, textLayout.GetAddressOf());
		}
		else
		{
			float pixelsPerDip = 1.0f;
			hr = m_d2d_writeFactory->CreateGdiCompatibleTextLayout(txt.c_str(), (UINT32)txt.size(),
					pUIfont, cxDim, cyDim, pixelsPerDip, nullptr, TRUE, textLayout.GetAddressOf());
		}
		XeASSERT(hr == S_OK);
		if (SUCCEEDED(hr))
		{
			// Get text size  
			DWRITE_TEXT_METRICS textMetrics;
			hr = textLayout->GetMetrics(&textMetrics);
			size = D2D1::SizeF((float)ceil(textMetrics.widthIncludingTrailingWhitespace),
					(float)ceil(textMetrics.height));
		}
		return hr;
	}

	virtual IWICFormatConverter* D2D_GetImage(PID pid, CSize* pSize = nullptr) override
	{
		if (m_d2d_pid_map_img.contains(pid))
		{
			if (pSize)
			{
				XeASSERT(m_d2d_pid_map_size.contains(pid));
				if (m_d2d_pid_map_size.contains(pid))
				{
					*pSize = m_d2d_pid_map_size.at(pid);
				}
			}
			return m_d2d_pid_map_img.at(pid).Get();
		}
		m_d2d_pid_map_img[pid] = Microsoft::WRL::ComPtr<IWICFormatConverter>();
		auto it = m_d2d_pid_map_img.find(pid);
		XeASSERT(it != m_d2d_pid_map_img.end());
		if (it != m_d2d_pid_map_img.end())
		{
			std::wstring res_name = _GetPIDresourceName(pid);
			std::wstring t1 = L"_THEME1";
			if (((wcscmp(m_theme.GetThemeBasedOnName().c_str(), L"Dark theme") == 0)
				|| (wcscmp(m_theme.GetThemeName().c_str(), L"Dark theme") == 0))
					&& res_name.ends_with(t1) /*.size() > t1.size()*/)
			{
				res_name[res_name.size() - 1] = L'2';
				//if (std::wstring_view{ res_name.data() + res_name.size() - t1.size(), t1.size() } == t1)
				//{
				//	res_name[res_name.size() - 1] = L'2';
				//}
			}
			Microsoft::WRL::ComPtr<IWICStream> stream;
			Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
			Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
			DWORD dwSize = 0;
			HRSRC hResource = 0;
			void* pResourceData = 0;

			if ((hResource = ::FindResourceW(0, res_name.c_str(), L"PNG"))
				&& (dwSize = ::SizeofResource(0, hResource))
				&& (pResourceData = ::LockResource(::LoadResource(0, hResource))))
			{
				HRESULT hr = m_d2d_IWICFactory->CreateStream(stream.GetAddressOf());
				XeASSERT(hr == S_OK);
				if (SUCCEEDED(hr))
				{
					// Initialize the stream with the memory pointer and size.
					hr = stream->InitializeFromMemory(reinterpret_cast<BYTE*>(pResourceData), dwSize);
					XeASSERT(hr == S_OK);
				}
				if (SUCCEEDED(hr))
				{
					// Create a decoder
					hr = m_d2d_IWICFactory->CreateDecoderFromStream(stream.Get(), NULL,
							WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
					XeASSERT(hr == S_OK);

					// Retrieve the first frame of the image from the decoder
					if (SUCCEEDED(hr))
					{
						hr = decoder->GetFrame(0, frame.GetAddressOf());
						XeASSERT(hr == S_OK);
						if (SUCCEEDED(hr))
						{
							UINT width, height;
							hr = frame->GetSize(&width, &height);
							XeASSERT(hr == S_OK);
							CSize sizeImg(width, height);
							m_d2d_pid_map_size[pid] = sizeImg;
							if (pSize)
							{
								*pSize = sizeImg;
							}
						}
					}

					//Step 3: Format convert the frame to 32bppPBGRA
					if (SUCCEEDED(hr))
					{
						hr = m_d2d_IWICFactory->CreateFormatConverter(it->second.GetAddressOf());
						XeASSERT(hr == S_OK);
					}
					if (SUCCEEDED(hr))
					{
						hr = it->second->Initialize(
							frame.Get(),					 // Input bitmap to convert
							GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format
							WICBitmapDitherTypeNone,         // Specified dither pattern
							NULL,                            // Specify a particular palette 
							0.f,                             // Alpha threshold
							WICBitmapPaletteTypeCustom       // Palette translation type
						);
						XeASSERT(hr == S_OK);
						if (SUCCEEDED(hr))
						{
							return it->second.Get();
						}
					}
				}
			}
		}
		XeASSERT(false);
		return nullptr;
	}

	virtual ID2D1StrokeStyle* D2D_GetFocusStrokeStyle() override
	{
		if (!m_strokeStyleFocus.Get())
		{
			// Create stroke style for focus rect.
			D2D_GetFactory()->CreateStrokeStyle(
				D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
					D2D1_LINE_JOIN_MITER, 0.5f, D2D1_DASH_STYLE_DASH, 0.0f),
				nullptr, 0, &m_strokeStyleFocus);
		}
		return m_strokeStyleFocus.Get();
	}
#pragma endregion Direct2D
};

