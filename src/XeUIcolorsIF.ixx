module;

#include "os_minimal.h"
#include <algorithm>
#include <string>
#include <functional>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>

export module Xe.UIcolorsIF;

export import Xe.UItypes;
import Xe.UserSettingsForUI;
import Xe.ThemeIF;
export import Xe.TooltipIF;
export import Xe.mfc_types;
//export import Xe.LogDefs;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma region CustomWindowMessages
// Sent to split window - recalculate windows layout and redraw.
export constexpr UINT WMU_REPOSITION_WINDOWS = (WM_USER + 800);

// Sent (or posted) to Mainframe.
// Mainframe will forward (send) the message to all (log) views, tab view and timeline view.
// after message has been forwarded Mainframe will redraw entire window.
// wParam = NOTIFY CODE
//          NFCHG_CODE_LOGFILE_COLORS_CHANGED -  Log File Colors have changed,
// lParam = SENDER CODE
//          NFCHG_SENDER_COLOR_SETTINGS - ColorSettings dialog.
export constexpr UINT WMU_NOTIFY_CHANGES = (WM_USER + 830);

export constexpr UINT NFCHG_CODE_LOGFILE_COLORS_CHANGED = 0x1001;
export constexpr UINT NFCHG_SENDER_COLOR_SETTINGS		= 0x2001;

export constexpr UINT NFCHG_CODE_SEL_THEME				= 0x1002;
export constexpr UINT NFCHG_SENDER_MAINFRAME			= 0x2002;

// Sent from CImageCtrl or CXeEdit to parent to notify got focus
// wParam = HWND of sender
// lParam = HWND of old focus window
export constexpr UINT WMU_NOTIFY_GOT_FOCUS = (WM_USER + 836);
// Sent from CImageCtrl or CXeEdit to parent to notify lost focus
// wParam = HWND of sender
// lParam = HWND of new focus window
export constexpr UINT WMU_NOTIFY_LOST_FOCUS = (WM_USER + 837);

// WMU_XE_COMMAND - Posted by main window to itself
// wParam - XE_CMD_xxx
// lParam - command parameter
export constexpr UINT WMU_XE_COMMAND = (WM_USER + 915);
// Show Open SSH dialog
// - "hot-key" (Shift+Ctrl + F4) detected for Open SSH dialog.
//export constexpr UINT XE_CMD_SHOW_OPEN_SSH_DLG		= 1001;
// - "hot-key" Ctrl+NumPad1...9 detected for Tab navigation
// - lparam = Tab view index 0...8
export constexpr UINT XE_CMD_NAVIGATE_TAB_1_9		= 1003;
// - "hot-key" Ctrl+PageUp detected for Tab navigation
export constexpr UINT XE_CMD_NAVIGATE_TAB_PREV		= 1004;
// - "hot-key" Ctrl+PageDown detected for Tab navigation
export constexpr UINT XE_CMD_NAVIGATE_TAB_NEXT		= 1005;
// Posted by main window to itself - "hot-key" (Ctrl + Tab) detected for Tabs list dialog.
export constexpr UINT XE_CMD_SHOW_TABS_LIST_DLG		= 1006;
// Posted by ViewsListWnd to main window - navigate to tab with data source id.
export constexpr UINT XE_CMD_NAVIGATE_TAB_ID		= 1007;
#pragma endregion CustomWindowMessages

// Various UI values (int)
export enum class UIV
{
	unknown = 0,
	s_cyGridTH_RowHeight,
	cxTooltipDefault,
	cxComboBoxButton,
	cxComboBoxButtonArrow,
	cySubMenuArrow,
	cxyThickBorder,
	cxyThinBorder,
	cyWindowCaption
};

export enum class XERESTYPE
{
	eBtnUpName = 0,
	eBtnDnName
};

export enum class ECURSOR
{
	eArrow = 0,
	eWaitArrow,
	eWait
};

export enum class CMTX
{
	Hot,
	Gray,
	Fg,
	FgNormal
};

export struct XeFontMetrics
{
	XeFontMetrics() = default;
	XeFontMetrics(const std::wstring& fontname, int fontpointsize,
			DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL)
			: m_fontName(fontname), m_fontPointSize(fontpointsize), m_style(style), m_weight(weight), m_stretch(stretch) {}

	CSize m_sizeDatetime;	// size of "0000-00-00 00:00:00,000"

	// All values in pixels.
	int m_aveCharWidth = 0;	// Average width of font characters.
	int m_height = 0;		// Total height of font "box".
	int m_ascent = 0;		// Height of "box" above baseline.
	int m_descent = 0;		// Height of "box" below baseline.
	int m_intLeading = 0;	// Height of "box" above a typical english capital letter, e.g. 'H'.
	int m_extLeading = 0;	// Recommended additional white space to add between lines (can be 0 or negative).

	std::wstring        m_fontName;
	int                 m_fontPointSize = 0;	// User requested font point size.
	DWRITE_FONT_STYLE   m_style   = DWRITE_FONT_STYLE_NORMAL;
	DWRITE_FONT_WEIGHT  m_weight  = DWRITE_FONT_WEIGHT_NORMAL;
	DWRITE_FONT_STRETCH m_stretch = DWRITE_FONT_STRETCH_NORMAL;

	// All values in "design" units.
	DWRITE_FONT_METRICS m_dfm{ 0 };

	int GetHeight() const { return m_height; }
	int GetInternalLeading() const { return m_intLeading; }
	int GetLineHeight() const { return GetHeight() + GetInternalLeading(); }

	int DialogUnitsXtoPixels(int dlu_x) const
	{
		// 4 x dlu = 1 average character width
		float dlux_mul = (float)m_aveCharWidth / 4.0f;
		return (int)(((float)dlu_x * dlux_mul) + 0.5);
	}
	int DialogUnitsYtoPixels(int dlu_y) const
	{
		// 8 y dlu = 1 character height
		float dluy_mul = (float)m_height / 8.0f;
		return (int)(((float)dlu_y * dluy_mul) + 0.5);
	}
};

export struct UIScaleFactor
{
	double m_cxScaleFactor = 1.0, m_cyScaleFactor = 1.0;
	
	UIScaleFactor() = default;
	UIScaleFactor(double cxScaleFactor, double m_cyScaleFactor)
		: m_cxScaleFactor(cxScaleFactor), m_cyScaleFactor(m_cyScaleFactor) {}

	void ScaleRect(CRect& rect) const
	{
		int cx = (int)((double)rect.Width() * m_cxScaleFactor + 0.5);
		int cy = (int)((double)rect.Height() * m_cyScaleFactor + 0.5);
		rect.left = (int)((double)rect.left * m_cxScaleFactor + 0.5);
		rect.right = rect.left + cx;
		rect.top = (int)((double)rect.top * m_cyScaleFactor + 0.5);
		rect.bottom = rect.top + cy;
	}

	void ScalePoint(CPoint& pt) const
	{
		pt.x = (int)((double)pt.x * m_cxScaleFactor + 0.5);
		pt.y = (int)((double)pt.y * m_cyScaleFactor + 0.5);
	}

	int ScaleX(int x) const { return (int)((double)x * m_cxScaleFactor + 0.5); }
	int ScaleY(int y) const { return (int)((double)y * m_cyScaleFactor + 0.5); }
};

export typedef std::function<void()> DialogClosedCallbackFunc;

//class CXeD2DWndBase;
// Note to self - forward declaring a class does not work with modules (unless the class is later defined in the same module).
// You will get an error like this: error C2664: 'void CXeUIcolorsIF::SetHwndD2DBasePtr(HWND,CXeD2DWndBase *,const wchar_t *)': cannot convert argument 2 from 'CXeD2DWndBase *' to 'CXeD2DWndBase *'

export class CXeUIcolorsIF
{
public:
	virtual HINSTANCE GetInstanceHandle() = 0;

	// Get application name (as used in settings, folder names and such).
	virtual const std::wstring& GetAppName() const = 0;

	// Note - wndProc is usually D2DCtrl_WndProc (never? = ::DefWindowProc).
	virtual bool RegisterWindowClass(const std::wstring& class_name, WNDPROC wndProc,
		UINT style = CS_DBLCLKS, HICON hIcon = nullptr, HBRUSH hBgBrush = nullptr) = 0;

	// Set 'this' pointer of the CXeD2DWndBase object that created the window.
	// Note - we call here from within the window class procedure (of the registered class)
	// when the WM_NCCREATE message is being handled - the lpCreateParams of the CREATESTRUCT
	// is the 'this' pointer (provided by the creator in the call to CreateWindowExW).
	// We need to keep a global map of the pointers so the window class procedure can call
	// OnMessage function in the correct object.
	virtual void SetHwndD2DBasePtr(HWND hWnd, void* pWndObj, const wchar_t* class_name) = 0;
	virtual void RemoveHwndD2DBasePtr(HWND hWnd) = 0;

	// Get 'this' pointer of the CXeD2DWndBase object that created the window.
	// Note - debug version ASSERTs.
	virtual void* GetHwndD2DBasePtr(HWND hWnd) = 0;

	// Get 'this' pointer of the CXeD2DWndBase object that created the window.
	// Note - debug version does not ASSERT if hWnd not found in map.
	virtual void* GetHwndD2DBasePtrIfExists(HWND hWnd) = 0;

	// Add dialog box window handle to map (needed for dialog management).
	// Modeless dialog boxes should supply a callback function
	// (that deletes 'this' after dialog window destroyed).
	// Note - the handle is removed from the map after the window has been destroyed.
	virtual void AddDialogHandle(HWND hDlgWnd, DialogClosedCallbackFunc dlgClosedCallback) = 0;

	/// <summary>
	/// Set handle to main window (CXeMainFrameBase class handles this).
	/// </summary>
	virtual void SetMainWindowHandle(HWND hMainWnd) = 0;
	virtual HWND GetMainWindowHandle() const = 0;

	virtual void RegisterScintillaKeyboardFilterCallback(HWND hWndScintilla,
			FilterKeyboardMessageCallbackFunc filter_callback) = 0;
	virtual void UnRegisterScintillaKeyboardFilterCallback(HWND hWndScintilla) = 0;

	virtual bool IsScintillaEditControl(HWND hWnd) const = 0;

	// Called from within THE message loop (and the modal dialog message loop).
	virtual bool FilterKeyboardMessage(const MSG& msg) = 0;

	// Set application wide keyboard shortcuts handler.
	virtual void SetGlobalKeyboardFilterCallback(FilterKeyboardMessageCallbackFunc filter_callback) = 0;

	virtual void ReloadThemeJsonFiles() = 0;

	virtual bool SetTheme(const std::wstring& theme_name) = 0;

	virtual std::vector<std::wstring> GetThemeNames() const = 0;

	virtual XeThemeIF* GetCurrentTheme() = 0;

	bool IsFontSettingsChanged(const ChangedSettings& chg_settings) const
	{
		return IsGridFontSettingsChanged(chg_settings)
			|| chg_settings.IsAnyChanged(L"GeneralSettings", { L"UI_FontName", L"UI_FontPointSize" });
	}

	bool IsGridFontSettingsChanged(const ChangedSettings& chg_settings) const
	{
		return chg_settings.IsAnyChanged(L"LogFileGridSettings",
			{ L"GridFontName", L"GridFontPointSize", L"GridRowPitch" });
	}

	virtual void OnChangedSettings(const ChangedSettings& chg_settings) = 0;

	virtual int GetValue(UIV val) const = 0;

	virtual HICON GetAppIconHandle() = 0;

	//virtual HBRUSH GetHBRUSH(CID uCID) = 0;

	//virtual CBrush* GetBrush(CID uCID) = 0;

	//virtual HPEN GetHPEN(CID uCID) = 0;

	//virtual CPen* GetPen(CID uCID) = 0;

	// Get color for CID identifier.
	virtual COLORREF GetColor(CID uCID) const = 0;
	virtual D2D1_COLOR_F GetColorF(CID uCID) const = 0;

	//virtual ColorMatrix* GetColorMatrix(CMTX cmtx) const = 0;

	//virtual CXeGdiPng* GetPngImage(PID uPID) const = 0;
	virtual CSize GetPngImageSize(PID uPID) = 0;

	//virtual CXeGdiPng* GetFileIconPng(FileIcon icn) const = 0;

	//inline PID GetFileIconPID(FileIcon icn) const
	//{
	//	XeASSERT(icn != FileIcon::None);
	//	switch (icn)
	//	{
	//	case FileIcon::None:	return PID::AppAbout;			// INVALID (no icon available)
	//	case FileIcon::Server:	return PID::FileIconServer;
	//	case FileIcon::App:		return PID::FileIconApp;
	//	case FileIcon::Console:	return PID::FileIconConsole;
	//	case FileIcon::User:	return PID::FileIconUser;
	//	case FileIcon::Service:	return PID::FileIconService;
	//	case FileIcon::Device:	return PID::FileIconDevice;
	//	}
	//	return PID::AppAbout;			// INVALID (no icon available)
	//}

	//virtual CFont* GetFont(EXE_FONT eFont) const = 0;
	//virtual HFONT GetFontHandle(EXE_FONT eFont) const = 0;

	virtual const XeFontMetrics& GetFontMetric(EXE_FONT eFont) const = 0;

	//virtual const LOGFONTW& GetLogFontStruct(EXE_FONT eFont) const = 0;

	virtual UIScaleFactor GetUIScaleFactor() const = 0;

	// Get text size using font and desktop window DC.
	//virtual CSize GetTextSize(EXE_FONT eFont, const char* szText) const = 0;
	virtual CSize GetTextSizeW(EXE_FONT eFont, const wchar_t* szText) = 0;
	//virtual CSize GetTextSize(EXE_FONT eFont, const char* szText, size_t length) const = 0;
	virtual CSize GetTextSizeW(EXE_FONT eFont, const wchar_t* szText, size_t length) = 0;

	// Get text size using font and window DC.
	// if pWnd is null then desktop window is used.
	//virtual CSize GetTextSizeUsingFont(HWND hWnd, EXE_FONT eFontconst, const char* szText) const = 0;
	virtual CSize GetTextSizeUsingFontW(HWND hWnd, EXE_FONT eFontconst, const wchar_t* szText) = 0;

	// Get text widths using font and window DC.
	// if pWnd is null then desktop window is used.
	//virtual std::vector<uint32_t> GetTextWidthsUsingFont(const std::vector<std::string> texts,
	//	EXE_FONT eFontconst = EXE_FONT::eUI_Font, HWND hWnd = nullptr) const = 0;
	virtual std::vector<uint32_t> GetTextWidthsUsingFontW(const std::vector<std::wstring> texts,
			EXE_FONT eFontconst = EXE_FONT::eUI_Font, HWND hWnd = nullptr) = 0;

	// Get max. text width of supplied strings using font and window DC.
	// if pWnd is null then desktop window is used.
	//virtual uint32_t GetMaxTextWidthUsingFont(const std::vector<std::string> texts,
	//	EXE_FONT eFontconst = EXE_FONT::eUI_Font, HWND hWnd = nullptr) const = 0;
	virtual uint32_t GetMaxTextWidthUsingFontW(const std::vector<std::wstring> texts,
			EXE_FONT eFontconst = EXE_FONT::eUI_Font, HWND hWnd = nullptr) = 0;

	//virtual CSize GetMultilineTextExtent(const char* szTxt,
	//		EXE_FONT eFontconst = EXE_FONT::eUI_Font, HWND hWnd = nullptr) const = 0;
	virtual CSize GetMultilineTextExtentW(const wchar_t* szTxt,
			EXE_FONT eFontconst = EXE_FONT::eUI_Font, HWND hWnd = nullptr) = 0;

	//virtual int GetUI_FontGridRowHeight() const = 0;

	// Get string to append to PNG resource names - used by XePngButton class.
	//virtual CString GetThemePngAppend() const = 0;

	virtual MonospacedCharSize GetLogGridCharSize() const = 0;

	// Get background color for log level
	//virtual COLORREF GetLogLevelColor(LOGLEVEL logLevel) const = 0;

	//virtual CBrush* GetLogLevelBrush(LOGLEVEL logLevel) = 0;

	//inline CID GetLogLevelColorId(LOGLEVEL logLevel) const
	//{
	//	switch (logLevel)
	//	{
	//	case LOGLEVEL::eWARN:
	//		return CID::WarnBg;
	//	case LOGLEVEL::eERROR:
	//		return CID::ErrorBg;
	//	case LOGLEVEL::eDEBUG:
	//		return CID::DebugBg;
	//	case LOGLEVEL::eFATAL:
	//	case LOGLEVEL::eCRIT:
	//	case LOGLEVEL::eALERT:
	//	case LOGLEVEL::eEMERG:
	//		return CID::FatalBg;
	//	case LOGLEVEL::eNOTICE:
	//		return CID::InfoBg;
	//	default:
	//		return CID::InfoBg;
	//	}
	//}

	[[nodiscard]] virtual CXeTooltipIF* CreateTooltip(const std::wstring& nameForLogging, HWND hWndParent) = 0;
	virtual void DestroyTooltip(HWND hWndParent) = 0;
	virtual void HideTooltip(HWND hWndParent) = 0;
	virtual void HideOtherTooltips(HWND hWndTooltip) = 0;

	//virtual void ApplyTooltipFontAndSize(LOGFONTW& lf) const = 0;

	virtual int GetTooltipDefaultWidth() const = 0;

	virtual HCURSOR GetAppCursor(bool isGrid = false) const = 0;

	virtual void SetAppCursor(ECURSOR eCursor) = 0;

	bool IsChangeAppCursorMsg(UINT nHitTest, UINT message) const
	{
		if ((nHitTest >= HTLEFT && nHitTest <= HTBORDER) || nHitTest == HTGROWBOX)
		{
			return false;
		}
		return true;
	}

	//virtual BOOL OnSetCursorHelper(CWnd* pCallingWnd, CWnd* pWndInMsg, UINT nHitTest, UINT message) = 0;
	virtual BOOL OnSetCursorHelper(HWND hCallingWnd, HWND hWndInMsg, UINT nHitTest, UINT message) = 0;

	//virtual CPen* GetPngBtnFocusPen() const = 0;	// Shared 'focus' outline pen for PngButton

	//virtual CXeGdiPng* GetCachedPngBtnImg(CString& strPngName) const = 0;

	//virtual const LogGridPaintObj& GetLogGridPaintObj() const = 0;

	virtual void SetDialogWantsClickOutsideMouseDown(HWND hWndDlg, UINT message) = 0;

	// Note - this is an ugly hack to show scrollbar menu
	// (because XeMenu depends on XeScrollbar (through XeListBox).
	virtual void ShowScrollbarMenu(HWND hParentWnd, bool isHorizontalSB, CPoint ptScreen) = 0;

#pragma region Direct2D
	virtual ID2D1Factory* D2D_GetFactory() const = 0;
	virtual IDWriteFactory* D2D_GetWriteFactory() const = 0;
	virtual IWICImagingFactory* D2D_GetWICFactory() const = 0;
	virtual HRESULT D2D_CreateDCRenderTarget(_COM_Outptr_ ID2D1DCRenderTarget** dcRenderTarget) const = 0;
	virtual IDWriteTextFormat* D2D_GetFont(EXE_FONT font) = 0;
	virtual HRESULT D2D_GetTextSize(const std::wstring& txt, EXE_FONT font, D2D1_SIZE_F& size) = 0;

	virtual IWICFormatConverter* D2D_GetImage(PID pId, CSize* pSize = nullptr) = 0;

	virtual ID2D1StrokeStyle* D2D_GetFocusStrokeStyle() = 0;
#pragma endregion Direct2D
};

export class CXeWaitCursor
{
protected:
	CXeUIcolorsIF* m_xeUI;

public:
	CXeWaitCursor(CXeUIcolorsIF* pUIcolors) : m_xeUI(pUIcolors)
	{
		m_xeUI->SetAppCursor(ECURSOR::eWait);
	}
	~CXeWaitCursor()
	{
		m_xeUI->SetAppCursor(ECURSOR::eArrow);
	}
};
