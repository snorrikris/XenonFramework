module;

// Source: https://www.codeproject.com/Articles/5270027/A-Modern-Direct2D-Color-Picker-for-Plain-Win32

#include "os_minimal.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <d2d1.h>
#include <dwrite.h>


export module Xe.ColorPicker;

import Xe.PopupCtrlBase;
import Xe.PopupCtrl;
import Xe.UIcolorsIF;
import Xe.D2DWndBase;
import Xe.StringTools;
import Xe.Helpers;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class ClrPickCtrlNotify
{
	None = 0,
	SelColor = 0x3101,
	ModeChange = 0x3102,
	LayoutChange = 0x3103,
	OkBtn = 0x3104,
	CancelBtn = 0x3105

	// Remember to update CPN_mmm - if the enum is changed.
};

// Notification codes (in NMHDR.code) sent to parent.
export constexpr UINT CPN_FIRST =        (UINT)ClrPickCtrlNotify::SelColor;		// Color picker first notify code.
export constexpr UINT CPN_SELCOLOR =     (UINT)ClrPickCtrlNotify::SelColor;		// Selected color has changed.
export constexpr UINT CPN_MODECHANGE =   (UINT)ClrPickCtrlNotify::ModeChange;	// Mode changed (RGB or HSL).
export constexpr UINT CPN_LAYOUTCHANGE = (UINT)ClrPickCtrlNotify::LayoutChange;	// Layout changed (Simple or Full).
export constexpr UINT CPN_OKBUTTON =     (UINT)ClrPickCtrlNotify::OkBtn;		// User clicked the OK button (in dropdown).
export constexpr UINT CPN_CANCELBUTTON = (UINT)ClrPickCtrlNotify::CancelBtn;	// User clicked the Cancel button (in dropdown).
export constexpr UINT CPN_LAST =         (UINT)ClrPickCtrlNotify::CancelBtn;	// Color picker last notify code.

export struct NMXECOLORPICKER
{
	NMHDR	hdr;
	int		notifyCode;	// See ClrPickCtrlNotify
	DWORD	rgbColor;
};

export enum class ColorPickerMode { Simple, Full };

class RCT
{
public:
	enum RCT_Enum
	{
		RedLabel, GreenLabel, BlueLabel, HueLabel, SatLabel, LumLabel,
		RedValue, GreenValue, BlueValue, HueValue, SatValue, LumValue,
		ColorWheel, HueSlider, SatSlider, LumSlider,
		Mode1Btn, Mode2Btn, PicrBtn, OkBtn, CancelBtn, MoreBtn, None
	};

protected:
	RCT_Enum m_e = RCT_Enum::None;

public:
	RCT() = default;
	/* implicit */ constexpr RCT(RCT_Enum e) : m_e(e) {}
	constexpr operator RCT_Enum() const { return m_e; }	// Allows comparisons with Enum constants.
	explicit operator bool() const = delete;			// Needed to prevent if(c).

	bool IsHueOrRedSlider() const { return m_e == RCT_Enum::HueSlider; }
	bool IsSatOrGreenSlider() const { return m_e == RCT_Enum::SatSlider; }
	bool IsLumOrBlueSlider() const { return m_e == RCT_Enum::LumSlider; }
	bool IsAnySlider() const { return m_e == RCT_Enum::HueSlider || m_e == RCT_Enum::SatSlider || m_e == RCT_Enum::LumSlider; }
	bool IsRedValue() const { return m_e == RCT_Enum::RedLabel || m_e == RCT_Enum::RedValue; }
	bool IsGreenValue() const { return m_e == RCT_Enum::GreenLabel || m_e == RCT_Enum::GreenValue; }
	bool IsBlueValue() const { return m_e == RCT_Enum::BlueLabel || m_e == RCT_Enum::BlueValue; }
	bool IsHueValue() const { return m_e == RCT_Enum::HueLabel || m_e == RCT_Enum::HueValue; }
	bool IsSatValue() const { return m_e == RCT_Enum::SatLabel || m_e == RCT_Enum::SatValue; }
	bool IsLumValue() const { return m_e == RCT_Enum::LumLabel || m_e == RCT_Enum::LumValue; }
	bool IsAnyValue() const { return m_e == RCT_Enum::RedValue || m_e == RCT_Enum::GreenValue || m_e == RCT_Enum::BlueValue
			|| m_e == RCT_Enum::HueValue || m_e == RCT_Enum::SatValue || m_e == RCT_Enum::LumValue; }
	bool IsAnyButton() const { return m_e == RCT_Enum::Mode1Btn || m_e == RCT_Enum::Mode2Btn || m_e == RCT_Enum::PicrBtn
			|| m_e == RCT_Enum::OkBtn || m_e == RCT_Enum::CancelBtn ||  m_e == RCT_Enum::MoreBtn; }

	bool CanHover() const { return IsAnySlider() || IsAnyValue() || IsAnyButton(); }

	VT ToVT() const
	{
		switch (m_e)
		{
		case RCT::RedValue: return VT::Red;
		case RCT::GreenValue: return VT::Green;
		case RCT::BlueValue: return VT::Blue;
		case RCT::HueValue: return VT::Hue;
		case RCT::SatValue: return VT::Sat;
		case RCT::LumValue: return VT::Lum;
		}
		return VT::None;
	}
};

#pragma region EditValues
class XeColPartEdit
{
	VT m_editing = VT::None;
	XeD2D1_COLOR_F m_color, m_original_color;
	int m_cur_digit = 0;
	bool m_isBlinkOn = false;
	wchar_t m_digits[4];
	WORD m_max;

	void _SetValue(WORD value)
	{
		m_max = m_editing == VT::Hue ? 359 : 255;
		std::memset(m_digits, 0, 4);
		swprintf_s(m_digits, 4, L"%u", value);
		if (m_digits[1] == 0) { m_digits[1] = L' '; m_digits[2] = L' '; m_digits[3] = 0; }
		if (m_digits[2] == 0) { m_digits[2] = L' ';  m_digits[3] = 0; }
	}

	WORD _as_value() const
	{
		return _HasDigits() ? (WORD)xen::stoi(_as_string()) : 0;
	}

	std::wstring _as_string() const { return std::wstring(m_digits); }

	bool _HasDigits() const
	{
		bool hasDigits = false;
		for (int i = 0; i < 3; ++i)
		{
			hasDigits |= m_digits[i] >= L'0' && m_digits[i] <= L'9';
		}
		return hasDigits;
	}

	VT _GetNext(VT vt)
	{
		switch (vt)
		{
		case VT::Red:			return VT::Green;
		case VT::Green:			return VT::Blue;
		case VT::Blue:			return VT::None;
		case VT::Hue:			return VT::Sat;
		case VT::Sat:			return VT::Lum;
		case VT::Lum:			return VT::None;
		}
		return VT::None;
	}
	VT _GetPrev(VT vt)
	{
		switch (vt)
		{
		case VT::Red:			return VT::None;
		case VT::Green:			return VT::Red;
		case VT::Blue:			return VT::Green;
		case VT::Hue:			return VT::None;
		case VT::Sat:			return VT::Hue;
		case VT::Lum:			return VT::Sat;
		}
		return VT::None;
	}

	bool _UpdateColor()
	{
		if (!IsError())
		{
			m_color.SetUsingDisplayValue(m_editing, _as_value());
			return true;
		}
		return false;
	}

	bool _MoveEditPart(bool isMoveNext)
	{
		bool isChanged = _UpdateColor();
		m_editing = isMoveNext ? _GetNext(m_editing) : _GetPrev(m_editing);
		m_cur_digit = isMoveNext ? 0 : 2;
		m_isBlinkOn = false;
		_SetValue((WORD)m_color.GetDisplayValue(m_editing));
		return isChanged;
	}

	bool _BackSpace()
	{
		if (m_cur_digit <= 0 || m_cur_digit > 2)
		{
			return false;
		}
		m_digits[m_cur_digit - 1] = m_digits[m_cur_digit];
		for (int i = m_cur_digit; i < 3; ++i)
		{
			m_digits[i] = m_digits[i + 1];
		}
		m_digits[2] = L' ';
		--m_cur_digit;
		_UpdateColor();
		return true;
	}

	bool _Delete()
	{
		if (m_cur_digit < 0 || m_cur_digit > 2)
		{
			return false;
		}
		if (m_cur_digit < 2)
		{
			m_digits[m_cur_digit] = m_digits[m_cur_digit + 1];
			for (int i = m_cur_digit + 1; i < 3; ++i)
			{
				m_digits[i] = m_digits[i + 1];
			}
		}
		m_digits[2] = L' ';
		_UpdateColor();
		return true;
	}

public:
	void Set(VT editing, XeD2D1_COLOR_F color)
	{
		m_editing = editing;
		m_color = m_original_color = color;
		m_cur_digit = 0;
		m_isBlinkOn = false;
		_SetValue((WORD)m_color.GetDisplayValue(m_editing));
	}

	XeD2D1_COLOR_F GetColor() const { return m_color; }

	XeD2D1_COLOR_F GetOriginalColor() const { return m_original_color; }

	VT part() const { return m_editing; }

	RCT getRCT() const
	{
		switch (m_editing)
		{
		case VT::Red:			return RCT::RedValue;
		case VT::Green:			return RCT::GreenValue;
		case VT::Blue:			return RCT::BlueValue;
		case VT::Hue:			return RCT::HueValue;
		case VT::Sat:			return RCT::SatValue;
		case VT::Lum:			return RCT::LumValue;
		}
		return RCT::None;
	}

	void Blink() { m_isBlinkOn = !m_isBlinkOn; }

	std::wstring GetText(VT vt, const XeD2D1_COLOR_F& color)
	{
		if (m_editing == VT::None)
		{
			return color.GetDisplayString(vt);
		}
		if (vt == m_editing)
		{
			std::wstring s(m_digits);
			if (m_isBlinkOn && m_cur_digit >= 0 && m_cur_digit < s.size())
			{
				s[m_cur_digit] = L'#';
			}
			return s;
		}
		return m_color.GetDisplayString(vt);
	}

	bool IsError() const
	{
		return m_editing != VT::None && (!_HasDigits() || _as_value() > m_max);
	}

	// Return true if color value has (maybe) changed.
	bool OnKeyDown(WPARAM ww)
	{
		if (ww == VK_LEFT)
		{
			if (m_cur_digit > 0)
			{
				--m_cur_digit;
				return false;
			}
			return _MoveEditPart(false);
		}
		else if (ww == VK_RIGHT)
		{
			if (m_cur_digit < 2)
			{
				++m_cur_digit;
				return false;
			}
			return _MoveEditPart(true);
		}
		else if (ww == VK_TAB)
		{
			return _MoveEditPart(true);
		}
		else if (ww == VK_BACK)
		{
			return _BackSpace();
		}
		else if (ww == VK_DELETE)
		{
			return _Delete();
		}
		return false;
	}

	// Return true if color value has (maybe) changed.
	bool OnChar(WPARAM ww)
	{
		if (ww >= '0' && ww <= '9' && m_cur_digit < 3)
		{
			m_digits[m_cur_digit] = (wchar_t)ww;
			if (++m_cur_digit >= 3)
			{
				if (IsError())
				{
					m_cur_digit = 2;
				}
				else
				{
					_MoveEditPart(true);
				}
			}
			return true;
		}
		else if (ww == ',' || ww == '.')
		{
			return _MoveEditPart(true);
		}
		return false;
	}
};
#pragma endregion EditValues

constexpr UINT BLINK_TIMERID = 1003;
constexpr UINT BLINK_TIME = 500;			// mS - repeat interval.

typedef std::function<void(ClrPickCtrlNotify nf_code)> ClrPickCtrlNotifyCallbackFunc;

constexpr wchar_t XECOLORPICKERWND_CLASSNAME[] = L"XeColorPickerWndClass";  // Window class name

export class CXeColorPicker : public CXePopupCtrlBase, public CXeD2DCtrlBase
{
#pragma region Data
	enum class UImode { RGB, HSL };

public:
	bool m_isPopupCtrl = false;

	// Parent window sets the callback (used when color picker is just a normal control - i.e. not a popup).
	ClrPickCtrlNotifyCallbackFunc m_nfCallback = nullptr;

	bool m_isMoreOrLessButtonVisible = true;

	ColorPickerMode m_layout = ColorPickerMode::Simple;
	UINT m_notifyChangeWndSizeMsg = 0;
	UImode m_uiMode = UImode::RGB;

	float ScaleDPI = 96.0f;

	XeD2D1_COLOR_F SELCOL;
	D2D1_COLOR_F BEFORE = { 0,0,0,1.0f };

	XeColPartEdit m_editing;

	bool m_isOkBtnClicked = false;
	bool m_isCancelBtnClicked = false;

protected:
	////////////////////////////////////////////////////////////////////////////////////////
	// Mouse processing data
	RCT m_isAdjusting;	// User is adjusting slider, picking color or color wheel.
	RCT m_hovering;		// Mouse is hovering over a control.
	int m_idxHoverColorSquare = -1;	// Index of color square where mouse is hovering.

	////////////////////////////////////////////////////////////////////////////////////////
	// Paint data
	bool m_isDimensionsCalculated = false;
	float marginL, marginT, wheelCXY, labelCX, valueCX, moreBtnCX;
	float sliderCX, spcCX, txtCY, sliderCY, smallBtnCY, pickerBtnCX;
	float m_clr_squareW, m_clr_squareH;

	D2D1_RECT_F ColorWheelRect = {};
	D2D1_ROUNDED_RECT SelC = {}, BeforeC = {};
	float m_L = 0.5f;

	struct PT
	{
		D2D1_RECT_F r = {};
		D2D1_COLOR_F c;
	};
	std::vector<PT> map_squares;

	Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> m_wheelBitmap;
#pragma endregion Data

#pragma region Create
public:
	CXeColorPicker(CXeUIcolorsIF* pUIcolors, ColorPickerMode mode)
			: CXeD2DCtrlBase(pUIcolors), m_layout(mode)
	{
		m_xeUI->RegisterWindowClass(XECOLORPICKERWND_CLASSNAME, D2DCtrl_WndProc);

		SELCOL.SetFromRGB(0, false);
		BEFORE = SELCOL;
	}
	//virtual ~CXeColorPicker() {}

	bool Create(DWORD dwStyle, const CRect& rect, HWND hParentWnd,
			UINT nID, const wchar_t* tooltip)
	{
		DWORD dwExStyle = 0;
		HWND hWnd = CreateD2DCtrl(dwExStyle, XECOLORPICKERWND_CLASSNAME, nullptr, dwStyle,
				rect, hParentWnd, nID, tooltip);
		return hWnd != 0;
	}
#pragma endregion Create

#pragma region CXePopupCtrlBase_impl
public:
	virtual HWND GetHwnd() const override { return Hwnd(); }
	virtual HWND GetVscrollHwnd() const override { return 0; }
	virtual HWND GetHscrollHwnd() const override { return 0; }

	virtual CSize GetWindowPreferredSize() override
	{
		return CSize(_CalculateUI());
	}

	virtual void AdjustPopupWindowFinalSize(
			CRect& rcScreenPos, const CRect& rcParentCtrlScreenPos) override {}

	virtual DropWndOffset GetDropWndOffsetPreference() const override { return DropWndOffset::AboveParentCtrl; }

	// Called by XePopupParent when processing the NC_CREATE message.
	virtual void PopupDropDown(HWND hWnd, DWORD dwStyle, DWORD dwExStyle) override
	{
		SetHwnd(hWnd);
		m_style.Set(hWnd, dwStyle, dwExStyle, m_xeUI);
		m_control_has_border = true;
	}

	// Return true if 'this' is the first instance of the 'control'.
	// The derived control should return true - unless 'this' instance is a 'sub-control' window (e.g. sub-menu).
	virtual bool IsFirstInstance() const override { return true; }

	// Return true when user has made any kind of choice in the control that should close the popup window.
	virtual bool IsPopupEndCondition() override
	{
		return m_isOkBtnClicked || m_isCancelBtnClicked;
	}

	// Called by XePopupParent before popup window created (to tell the control that it's in a popup window).
	virtual void PopupOpening() override
	{
		m_isPopupCtrl = true;
	}

	// Called by XePopupParent after popup window destroyed.
	virtual void PopupClosed() override
	{
		SetHwnd(0);
		m_isPopupCtrl = false;
	}

	virtual LRESULT ProcessPopupWndMessage(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam) override
	{
		if (hwnd == Hwnd() && (uiMsg == WM_LBUTTONDOWN || uiMsg == WM_RBUTTONDOWN))
		{
			CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			CRect rcClient;
			::GetClientRect(hwnd, &rcClient);
			if (!rcClient.PtInRect(pt))
			{
				m_isCancelBtnClicked = true;
				_NotifyParent(ClrPickCtrlNotify::CancelBtn);
				return 1;
			}
		}
		return CXeD2DCtrlBase::_OnMessageD2DBase(hwnd, uiMsg, wParam, lParam);
	}
#pragma endregion CXePopupCtrlBase_impl

#pragma region helpers
public:
	CSize GetControlSize() { return _CalculateUI(); }

protected:
	RCT _PtInRect(const float& x, const float& y)
	{
		int key = m_rr.PtInRect(x, y);
		if (key >= 0)
		{
			RCT rct((RCT::RCT_Enum)key);
			if (!m_isPopupCtrl && (rct == RCT::OkBtn || rct == RCT::CancelBtn))
			{
				rct = RCT::None;
			}
			return rct;
		}
		return RCT::None;
	}

	bool _SetSelectedColorFromPointInColorWheel(const float& x, const float& y, bool isRedraw = true)
	{
		auto col = _GetColorFromPointInColorWheel(x, y);
		if (col.has_value())
		{
			// Keep L
			SELCOL.SetColorAndL(col.value(), m_L);
			if (isRedraw)
			{
				_RedrawDirectly();
			}
			_NotifySelColor();
			return true;
		}
		return false;
	}
	std::optional<XeD2D1_COLOR_F> _GetColorFromPointInColorWheel(const float& x, const float& y)
	{
		float wheelRadius = wheelCXY / 2.0f;
		float center_y = marginT + wheelRadius, center_x = marginL + wheelRadius;
		float x_inCircle = x - center_x, y_inCircle = y - center_y;
		// atan returns a result in radians. Convert to degrees with degrees=180*radians/pi.
		float radians = std::atan2(y_inCircle, x_inCircle);
		// degrees is 0 to -180° for upper half of circle, 0 to 180° for lower half. 0° is on the right side.
		float degrees = 180.0f * radians / (float)std::numbers::pi;
		// We need 0 to 360°. 0° is on the right side, 90° is on top, 180° is on left side and 270° is on the bottom.
		float corr_deg = degrees < 0.0f ? -degrees : 360.0f - degrees;
		float radius = std::sqrt((x_inCircle * x_inCircle) + (y_inCircle * y_inCircle));
		//TRACE("radians=%.2f, degrees=%.2f, corr_deg=%.2f, radius=%.2f\n", radians, degrees, corr_deg, radius);
		if (radius > wheelRadius)
		{
			return std::nullopt;
		}
		float S = (float)radius / wheelRadius;	// Convert S to [0,1]
		float H = (corr_deg * 6) / 360.0f;		// Convert H to [0,6]
		XeD2D1_COLOR_F col;
		col.SetHSL(H, S, 0.5f);
		return col;
	}

	bool SetColorFromPointInSquaresMap(const float& x, const float& y, bool isRedraw = true)
	{
		PT* pPT = _IsPointInSquaresMap(x, y);
		if (pPT)
		{
			SELCOL = pPT->c;
			if (isRedraw)
			{
				_RedrawDirectly();
			}
			_NotifySelColor();
			return true;
		}
		return false;
	}

	PT* _IsPointInSquaresMap(const float& x, const float& y, int* pIndex = nullptr)
	{
		int idx = 0;
		for (auto& v : map_squares)
		{
			if (x >= v.r.left && x <= v.r.right && y >= v.r.top && y <= v.r.bottom)
			{
				if (pIndex) { *pIndex = idx; }
				return &v;
			}
			++idx;
		}
		if (pIndex) { *pIndex = -1; }
		return nullptr;
	}

	void _CalculateSelColorForSlider(float y)
	{
		const D2D1_RECT_F& LRect = m_rr.Get((int)RCT::LumSlider).rect;
		float he = LRect.bottom - LRect.top;
		if (y < LRect.top) { y = LRect.top; }
		if (y > LRect.bottom) { y = LRect.bottom; }
		y -= LRect.top;
		float L2 = (1.0f - (y / he));
		switch (m_isAdjusting)
		{
		case RCT::LumSlider:
			if (m_uiMode == UImode::RGB)
			{
				SELCOL.b = L2;
			}
			else
			{
				SELCOL.SetL(L2);
				m_L = L2;
			}
			break;

		case RCT::SatSlider:
			if (m_uiMode == UImode::RGB)
			{
				SELCOL.g = L2;
			}
			else
			{
				SELCOL.SetS(L2);
			}
			break;

		case RCT::HueSlider:
			if (m_uiMode == UImode::RGB)
			{
				SELCOL.r = L2;
			}
			else
			{
				SELCOL.SetH(L2 * 6);
			}
			break;
		}
		_NotifySelColor();
	}

	std::vector<int> _GetLabelKeys() const
	{
		return { (int)RCT::RedLabel, (int)RCT::RedValue, (int)RCT::GreenLabel, (int)RCT::GreenValue,
				(int)RCT::BlueLabel, (int)RCT::BlueValue, (int)RCT::HueLabel, (int)RCT::HueValue,
				(int)RCT::SatLabel, (int)RCT::SatValue, (int)RCT::LumLabel, (int)RCT::LumValue };
	}

	std::vector<int> _GetButtonKeys() const
	{
		if (m_isMoreOrLessButtonVisible)
		{
			if (m_isPopupCtrl)
			{
				return { (int)RCT::Mode1Btn, (int)RCT::Mode2Btn, (int)RCT::PicrBtn,
						(int)RCT::OkBtn, (int)RCT::CancelBtn, (int)RCT::MoreBtn };
			}
			else
			{	// No OK/Cancel buttons when control is not popup.
				return { (int)RCT::Mode1Btn, (int)RCT::Mode2Btn, (int)RCT::PicrBtn, (int)RCT::MoreBtn };
			}
		}
		else
		{
			if (m_isPopupCtrl)
			{
				return { (int)RCT::Mode1Btn, (int)RCT::Mode2Btn, (int)RCT::PicrBtn,
						(int)RCT::OkBtn, (int)RCT::CancelBtn };
			}
			else
			{	// No OK/Cancel buttons when control is not popup.
				return { (int)RCT::Mode1Btn, (int)RCT::Mode2Btn, (int)RCT::PicrBtn };
			}
		}
	}

	void _NotifySelColor()
	{
		_NotifyParent(ClrPickCtrlNotify::SelColor);
	}

	void _NotifyParent(ClrPickCtrlNotify nf_code)
	{
		HWND hWndParent = ::GetParent(Hwnd());
		if (hWndParent)
		{
			NMHDR nf{ 0 };
			nf.hwndFrom = Hwnd();
			nf.code = (UINT)nf_code;	// CPN_mmm codes.
			nf.idFrom = GetDlgCtrlID();
			::SendMessageW(hWndParent, WM_NOTIFY, nf.code, reinterpret_cast<LPARAM>(&nf));
		}
		if (m_nfCallback)
		{
			m_nfCallback(nf_code);
		}
	}

	void _ChangeLayout()
	{
		HWND hWndParent = ::GetParent(Hwnd());
		if (hWndParent)
		{
			CRect rcCtrl;
			::GetWindowRect(Hwnd(), &rcCtrl);
			::ScreenToClient(hWndParent, (LPPOINT)&rcCtrl);
			::ScreenToClient(hWndParent, ((LPPOINT)&rcCtrl) + 1);
			CSize newSize = GetWindowPreferredSize();
			::MoveWindow(Hwnd(), rcCtrl.left, rcCtrl.top, newSize.cx, newSize.cy, FALSE);
			_RedrawDirectly();
		}
	}
#pragma endregion helpers

#pragma region CalculateUI
	void _CalculateDimensions()
	{
		D2D1_SIZE_F szF{};
		m_xeUI->D2D_GetTextSize(std::wstring(L"GREEN"), EXE_FONT::eUI_Font, szF);
		labelCX = szF.width * 1.3f;
		txtCY = szF.height;

		m_xeUI->D2D_GetTextSize(std::wstring(L"000"), EXE_FONT::eUI_Font, szF);
		valueCX = szF.width * 1.1f;

		m_xeUI->D2D_GetTextSize(std::wstring(L"More>"), EXE_FONT::eUI_Font, szF);
		moreBtnCX = szF.width * 1.1f;
		smallBtnCY = szF.height * 1.2f;

		m_xeUI->D2D_GetTextSize(std::wstring(L"{+}"), EXE_FONT::eUI_Font, szF);
		pickerBtnCX = szF.width * 1.3f;

		m_xeUI->D2D_GetTextSize(std::wstring(L"HH"), EXE_FONT::eUI_Font, szF);
		sliderCX = szF.width;
		spcCX = sliderCX * 0.5f;

		marginL = marginT = sliderCX * 0.5f;
		if (txtCY <= 23.0f)
		{
			m_clr_squareW = 17;
			m_clr_squareH = 25;
			wheelCXY = 220;
		}
		else
		{
			m_clr_squareW = 18;
			m_clr_squareH = 26;
			wheelCXY = 230;
		}

		m_isDimensionsCalculated = true;
	}

	SIZE _CalculateUI()	// Return size needed for control.
	{
		_ResetRects();
		bool isSimple = m_layout == ColorPickerMode::Simple;

		if (!m_isDimensionsCalculated)
		{
			_CalculateDimensions();
		}
		// Color wheel and color squares rect.
		ColorWheelRect = SetR(marginL, marginT, wheelCXY, wheelCXY);

		SIZE sCtrl;
		if (isSimple)
		{
			float yML = marginT + wheelCXY + (txtCY / 2);;
			if (m_isPopupCtrl)
			{
				// more button is below color wheel in "simple" mode when popup.
				float xML = marginL + ((wheelCXY - moreBtnCX) / 2);
				m_rr.Set((int)RCT::MoreBtn, xML, yML, moreBtnCX, smallBtnCY, 1.0f, L"More>");
			}
			else
			{
				// Selected color rects and more button are below color wheel in "simple" mode when not popup.
				float cxSpc = (wheelCXY - (3 * moreBtnCX));
				float x = marginL, cx = moreBtnCX;
				SelC = SetRR(x, yML, cx, smallBtnCY);			// Select color rect.
				BeforeC = SetRR(x + cx, yML, cx, smallBtnCY);	// Before color rect.
				x += (2 * cx) + cxSpc;
				m_rr.Set((int)RCT::MoreBtn, x, yML, cx, smallBtnCY, 1.0f, L"More>");
			}

			sCtrl.cx = (int)(ColorWheelRect.right + marginL);
			sCtrl.cy = (int)(m_rr.Get((int)RCT::MoreBtn).rect.bottom + marginT);
		}
		else
		{
			// Sliders are to the right of the colorwheel.
			float sliderCY = wheelCXY - txtCY - smallBtnCY - txtCY;
			float x = marginL + wheelCXY + spcCX, y = marginT + txtCY, cx = sliderCX, cy = sliderCY, cx2;
			auto HRect = m_rr.Set((int)RCT::HueSlider, x, y, cx, cy);	// Hue or Red slider rect.
			x = HRect.rect.right + spcCX;
			auto SRect = m_rr.Set((int)RCT::SatSlider, x, y, cx, cy);	// Saturation or Green slider rect.
			x = SRect.rect.right + spcCX;
			auto LRect = m_rr.Set((int)RCT::LumSlider, x, y, cx, cy);	// Lightness or Blue slider rect.

			// Mode buttons are below sliders.
			x = marginL + wheelCXY + spcCX;
			float modeBtnsY = marginT + txtCY + sliderCY + (txtCY * 0.5f);
			cx = ((LRect.rect.right - HRect.rect.left) - spcCX) / 2;
			bool isEnabled = m_uiMode == UImode::RGB;
			m_rr.Set((int)RCT::Mode1Btn, x, modeBtnsY, cx, smallBtnCY, 2.0f, L"HSL", isEnabled);	// HSL mode button rect.
			x += cx + spcCX;
			m_rr.Set((int)RCT::Mode2Btn, x, modeBtnsY, cx, smallBtnCY, 2.0f, L"RGB", !isEnabled);	// RGB mode button rect.

			// RGB / HSL Labels and values rects are to the right of sliders.
			x = marginL + wheelCXY + (3 * spcCX) + (3 * sliderCX) + spcCX;
			y = marginT, cx = labelCX, cx2 = valueCX, cy = txtCY;
			float sliderTotalCY = txtCY + sliderCY;
			float labelSpaceV = 6 * txtCY;
			float incCY = ((sliderTotalCY -labelSpaceV) / 6);
			m_rr.Set((int)RCT::RedLabel, x, y, cx, cy, 1.0f, L"RED");		// Red label rect
			m_rr.Set((int)RCT::RedValue, x + cx, y, cx2, cy, 1.0f,			// Red value rect
					m_editing.GetText(VT::Red, SELCOL));
			y += txtCY + incCY;
			m_rr.Set((int)RCT::GreenLabel, x, y, cx, cy, 1.0f, L"GREEN");	// Green label rect
			m_rr.Set((int)RCT::GreenValue, x + cx, y, cx2, cy, 1.0f,		// Green value rect
					m_editing.GetText(VT::Green, SELCOL));
			y += txtCY + incCY;
			m_rr.Set((int)RCT::BlueLabel, x, y, cx, cy, 1.0f, L"BLUE");		// Blue label rect
			m_rr.Set((int)RCT::BlueValue, x + cx, y, cx2, cy, 1.0f,			// Blue value rect
					m_editing.GetText(VT::Blue, SELCOL));
			y += txtCY + (incCY * 2);
			m_rr.Set((int)RCT::HueLabel, x, y, cx, cy, 1.0f, L"HUE");		// Hue label rect
			m_rr.Set((int)RCT::HueValue, x + cx, y, cx2, cy, 1.0f,			// Hue value rect
					m_editing.GetText(VT::Hue, SELCOL));
			y += txtCY + incCY;
			m_rr.Set((int)RCT::SatLabel, x, y, cx, cy, 1.0f, L"SAT");		// Saturation label rect
			m_rr.Set((int)RCT::SatValue, x + cx, y, cx2, cy, 1.0f,			// Saturation value rect
					m_editing.GetText(VT::Sat, SELCOL));
			y += txtCY + incCY;
			m_rr.Set((int)RCT::LumLabel, x, y, cx, cy, 1.0f, L"LIGHT");		// Luminance label rect
			m_rr.Set((int)RCT::LumValue, x + cx, y, cx2, cy, 1.0f,			// Luminance value rect
					m_editing.GetText(VT::Lum, SELCOL));
			m_rr.SetFont(_GetLabelKeys(), CID::CtrlTxt, EXE_FONT::eUI_Font, DWRITE_TEXT_ALIGNMENT_LEADING);

			// Color picker button rect - to the right of mode buttons, align X to labels.
			x = m_rr.Get((int)RCT::RedLabel).rect.left;
			m_rr.Set((int)RCT::PicrBtn, x, modeBtnsY, pickerBtnCX, smallBtnCY, 2.0f, L"{+}");

			// less button to the right of picker button in "full" mode.
			x += pickerBtnCX + spcCX;
			cx = labelCX + valueCX - pickerBtnCX - spcCX;
			m_rr.Set((int)RCT::MoreBtn, x, modeBtnsY, cx, smallBtnCY, 1.0f, L"<Less");

			// Ok and Cancel buttons are below color wheel.
			x = marginL, y = marginT + wheelCXY + txtCY;
			cx = (wheelCXY - spcCX) / 2, cy = smallBtnCY;
			m_rr.Set((int)RCT::OkBtn, x, y, cx, cy, 1.0f, L"Ok");			// OK button rect.
			x += cx + spcCX;
			m_rr.Set((int)RCT::CancelBtn, x, y, cx, cy, 1.0f, L"Cancel");	// Cancel button rect.

			// Color rects are below sliders and labels and to the right of Ok,Cancel buttons.
			x = marginL + wheelCXY + spcCX;
			y = m_rr.Get((int)RCT::OkBtn).rect.top;
			cx = (m_rr.Get((int)RCT::RedValue).rect.right - m_rr.Get((int)RCT::Mode1Btn).rect.left) / 2;
			SelC = SetRR(x, y, cx, cy);			// Select color rect.
			BeforeC = SetRR(x + cx, y, cx, cy);	// Before color rect.

			sCtrl.cx = (int)(BeforeC.rect.right + marginL);
			sCtrl.cy = (int)(BeforeC.rect.bottom + marginT);
		}
		return sCtrl;
	}

	void _ResetRects()
	{
		ColorWheelRect = {};
		SelC = BeforeC = {};
		m_rr.ResetRects({ (int)RCT::RedLabel, (int)RCT::RedValue, (int)RCT::GreenLabel, (int)RCT::GreenValue,
				(int)RCT::BlueLabel, (int)RCT::BlueValue, (int)RCT::HueLabel, (int)RCT::HueValue,
				(int)RCT::SatLabel, (int)RCT::SatValue, (int)RCT::LumLabel, (int)RCT::LumValue,
				(int)RCT::HueSlider, (int)RCT::SatSlider, (int)RCT::LumSlider, (int)RCT::PicrBtn,
				(int)RCT::Mode1Btn, (int)RCT::Mode2Btn, (int)RCT::CancelBtn, (int)RCT::OkBtn, (int)RCT::MoreBtn });
	}
#pragma endregion CalculateUI

#pragma region UI_Drawing
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		bool isSimple = m_layout == ColorPickerMode::Simple;
		_CalculateUI();

		pRT->Clear(m_xeUI->GetColorF(CID::DialogBg));
		D2D1_ROUNDED_RECT rcR(rc, 1, 1);
		pRT->DrawRoundedRectangle(rcR, GetBrush(CID::CtrlBorder));

		if (m_uiMode == UImode::RGB)
		{
			_DrawColorSquares(pRT);
		}
		else
		{
			_DrawColorWheel(pRT);
		}

		if (isSimple)
		{
			if (!m_isPopupCtrl)
			{
				// Selected and Before Color rects
				_DrawColorRect(pRT, SELCOL, SelC, L"");
				_DrawColorRect(pRT, BEFORE, BeforeC, L"");
			}

			if (m_isMoreOrLessButtonVisible)
			{
				_DrawButton(pRT, (int)RCT::MoreBtn);
			}
		}
		else
		{
			_DrawSliders(pRT);

			_DrawButtons(pRT, _GetButtonKeys());

			// Red, Green, Blue, Hue, Saturation and Lightness values
			_DrawTexts(pRT, _GetLabelKeys());

			// Selected and Before Color rects
			_DrawColorRect(pRT, SELCOL, SelC, L"Selected");
			_DrawColorRect(pRT, BEFORE, BeforeC, L"Before");
		}
		RCT active = m_editing.part() != VT::None ? m_editing.getRCT() : m_hovering;
		if (active != RCT::None)
		{
			const XeD2Rect& r = m_rr.Get((int)active);
			if (r.m_isEnabled)
			{
				_DrawRoundedRectangle(pRT, r, CID::CtrlBtnDefBorder);
			}
		}
		if (m_editing.IsError())
		{
			_DrawRoundedRectangle(pRT, (int)m_editing.getRCT(), CID::MrgLogNoDataBg);
		}

		if (::GetFocus() == Hwnd())
		{
			_DrawFocusRect(pRT, DeflateRectF(rc, 2.5f));
		}
	}

	void _DrawColorSquares(ID2D1RenderTarget* pRT)
	{
		if (map_squares.empty())
		{
			_CreateSquaresMap();
		}
		auto br = GetD2SolidBrush(pRT, { 1.0f,0,0,1.0f });
		int idx = 0;
		for (auto& v : map_squares)
		{
			br->SetColor(v.c);
			pRT->FillRectangle(v.r, br.Get());
			if (idx == m_idxHoverColorSquare)
			{
				pRT->DrawRectangle(v.r, GetBrush(CID::CtrlBtnDefBorder), 1.0f);
			}
			++idx;
		}
	}

	void _CreateSquaresMap()
	{
		map_squares.reserve(117);
		int X = 13;
		int Y = 9;

		std::vector<float> Lclr{ 25,51,76,102,127,153,178,204,223 };
		std::vector<float> Lbw{ 0,32,64,96,128,160,192,224,255 };
		float H, S, L;
		for (int x = 0; x < X; x++)
		{
			for (int y = 0; y < Y; y++)
			{
				PT pt;
				D2D1_RECT_F r = {};
				r.left = m_clr_squareW * x + marginL;
				r.right = r.left + m_clr_squareW - 3;
				r.top = m_clr_squareH * y + marginT;
				r.bottom = r.top + m_clr_squareH - 3;
				pt.r = r;

				if (x < 12) // Colored square?
				{
					H = ((float)(x * 30) * 6.0f) / 360.0f;
					S = 1.0f;
					L = Lclr[y] / 255;
				}
				else	// B/W square
				{
					H = 0;
					S = 0;
					L = Lbw[y] / 255;
				}
				XeD2D1_COLOR_F col;
				col.SetHSL(H, S, L);
				pt.c = col;
				map_squares.push_back(pt);
			}
		}
	}

	void _DrawColorWheel(ID2D1RenderTarget* pRT)
	{
		if (!m_wheelBitmap)
		{
			_CreateWheelBitmap(pRT);
		}
		if (m_wheelBitmap)
		{
			Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
			HRESULT hr = m_wheelBitmap->GetBitmap(&bitmap);
			if (SUCCEEDED(hr))
			{
				D2D1_RECT_F destRect{ marginL, marginT, marginL + wheelCXY, marginT + wheelCXY };
				pRT->DrawBitmap(bitmap.Get(), destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
			}
		}
	}

	HRESULT _CreateWheelBitmap(ID2D1RenderTarget* pRT)
	{
		HRESULT hr = pRT->CreateCompatibleRenderTarget(D2D1::SizeF(wheelCXY, wheelCXY), &m_wheelBitmap);
		if (SUCCEEDED(hr))
		{
			ID2D1SolidColorBrush* pBrush = NULL;
			hr = m_wheelBitmap->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF(0.93f, 0.94f, 0.96f, 1.0f)),
				&pBrush);
			if (SUCCEEDED(hr))
			{
				m_wheelBitmap->BeginDraw();

				float Radius = wheelCXY / 2.0f;
				D2D1_POINT_2F Center = { wheelCXY / 2.0f, wheelCXY / 2.0f };
				float L = 0.5f;
				float Resolution = 0.1f;
				XeD2D1_COLOR_F col;

				for (float hue = 0; hue < 360.0f; hue += Resolution)
				{
					for (int sat = 0; sat < (int)Radius; sat++)
					{
						float S = (float)sat / Radius;	// Convert S to [0,1]
						float H = (hue * 6) / 360.0f;	// Convert H to [0,6]
						col.SetHSL(H,S,L);

						// in 360 deg, 2pi
						// in   hue deg, ? 
						float hue2 = (2 * 3.1415f * hue) / 360.0f;
						float xPoint = (float)sat;
						float nX = xPoint * cos(hue2);
						float nY = -xPoint * sin(hue2);

						D2D1_POINT_2F p2 = { Center.x + nX, Center.y + nY };
						D2D1_RECT_F rc{ p2.x, p2.y, p2.x + 1, p2.y + 1 };
						pBrush->SetColor(col);
						m_wheelBitmap->FillRectangle(rc, pBrush);
						//TRACE("angle=%.2f, radius=%d, x=%.2f, y=%.2f\n", hue, sat, p2.x, p2.y);
					}
				}
				m_wheelBitmap->EndDraw();

				pBrush->Release();
			}
		}
		return hr;
	}

	void _DrawSlider(ID2D1RenderTarget* pRT, const std::vector<D2D1_GRADIENT_STOP>& gradientStops,
		const D2D1_RECT_F rect, const wchar_t* txt, size_t txt_len)
	{
		Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> lbr = 0;
		Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> pGradientStops = NULL;

		// Create the ID2D1GradientStopCollection from a previously
		// declared array of D2D1_GRADIENT_STOP structs.
		pRT->CreateGradientStopCollection(gradientStops.data(), (UINT32)gradientStops.size(),
			D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStops);
		pRT->CreateLinearGradientBrush(
			D2D1::LinearGradientBrushProperties(D2D1::Point2F(rect.left, rect.top),
				D2D1::Point2F(rect.right, rect.bottom)), pGradientStops.Get(), lbr.GetAddressOf());
		pRT->FillRectangle(rect, lbr.Get());

		IDWriteTextFormat* Text = m_xeUI->D2D_GetFont(EXE_FONT::eUI_Font);
		Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		auto r2 = rect;
		r2.top = marginT;
		r2.bottom = r2.top + txtCY;
		_CreateAndDrawTextLayout(pRT, txt, r2, CID::CtrlTxt);
	}

	void _DrawSliderSelLine(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rect, float multiplier)
	{
		float he = rect.bottom - rect.top, d = 3;
		float h2 = he * multiplier;
		h2 += rect.top;
		ID2D1SolidColorBrush* txt_brush = GetBrush(CID::CtrlTxt);
		pRT->DrawLine({ rect.left - d,h2 - d }, { rect.left,h2 }, txt_brush, 1);
		pRT->DrawLine({ rect.left,h2 }, { rect.left - d,h2 + d }, txt_brush, 1);
	}

	void _DrawSliders(ID2D1RenderTarget* pRT)
	{
		std::vector<D2D1_GRADIENT_STOP> gradientStops;
		gradientStops.resize(2);

		// Saturation or Green slider (gradient bar)
		if (m_uiMode == UImode::HSL)
		{
			XeD2D1_COLOR_F col(SELCOL);
			col.SetS(0);
			gradientStops[1].color = { col.r,col.g,col.b,1.0f };
			gradientStops[0].position = 0.0f;

			col = SELCOL;
			col.SetS(1.0f);
			gradientStops[0].color = { col.r,col.g,col.b,1.0f };
			gradientStops[1].position = 1.0f;
			_DrawSlider(pRT, gradientStops, m_rr.Get((int)RCT::SatSlider).rect, L"S", 1);
		}
		else
		{
			gradientStops[0].color = { 0.0,0.0,0,1.0f };
			gradientStops[0].position = 1.0;
			gradientStops[1].color = { 0.0,1.0f,0,1.0f };
			gradientStops[1].position = 0.0f;
			_DrawSlider(pRT, gradientStops, m_rr.Get((int)RCT::SatSlider).rect, L"G", 1);
		}

		// Luminance or Blue slider (gradient bar)
		if (m_uiMode == UImode::HSL)
		{
			XeD2D1_COLOR_F col(SELCOL);
			col.SetL(m_L);
			gradientStops[1].color = { col.r,col.g,col.b,1.0f };

			col = SELCOL;
			col.SetL(1.0f);
			gradientStops[0].color = { col.r,col.g,col.b,1.0f };
			_DrawSlider(pRT, gradientStops, m_rr.Get((int)RCT::LumSlider).rect, L"L", 1);
		}
		else
		{
			gradientStops[0].color = { 0.0,0.0,0,1.0f };
			gradientStops[0].position = 1.0;
			gradientStops[1].color = { 0.0,0.0f,1.0f,1.0f };
			gradientStops[1].position = 0.0f;
			_DrawSlider(pRT, gradientStops, m_rr.Get((int)RCT::LumSlider).rect, L"B", 1);
		}

		// Hue or Red slider (gradient bar)
		if (m_uiMode == UImode::HSL)
		{
			// Create the linear brush for hue
			std::vector<D2D1_GRADIENT_STOP>  gst(360);
			XeD2D1_COLOR_F col;
			for (int i = 0; i < 360; i++)
			{
				float H = ((360 - i) / 360.0f) * 6.0f;
				col.SetHSL(H, 1, m_L);
				gst[i].position = i / 360.0f;
				gst[i].color = col;
			}
			_DrawSlider(pRT, gst, m_rr.Get((int)RCT::HueSlider).rect, L"H", 1);
		}
		else
		{
			gradientStops[0].color = { 0.0,0.0,0,1.0f };
			gradientStops[0].position = 1.0;
			gradientStops[1].color = { 1.0,0,0,1.0f };
			gradientStops[1].position = 0.0f;
			_DrawSlider(pRT, gradientStops, m_rr.Get((int)RCT::HueSlider).rect, L"R", 1);
		}

		_DrawSliderSelLine(pRT, m_rr.Get((int)RCT::LumSlider).rect,
				(m_uiMode == UImode::RGB ? (1.0f - SELCOL.b) : (1.0f - SELCOL.GetL())));
		_DrawSliderSelLine(pRT, m_rr.Get((int)RCT::SatSlider).rect,
				(m_uiMode == UImode::RGB ? (1.0f - SELCOL.g) : (1.0f - SELCOL.GetS())));
		_DrawSliderSelLine(pRT, m_rr.Get((int)RCT::HueSlider).rect,
				(m_uiMode == UImode::RGB ? (1.0f - SELCOL.r) : (1.0f - (SELCOL.GetH() / 6.0f))));
	}

	void _DrawColorRect(ID2D1RenderTarget* pRT, const D2D1_COLOR_F& clr,
		const D2D1_ROUNDED_RECT& rect, const std::wstring& txt)
	{
		auto b2 = GetD2SolidBrush(pRT, clr);
		pRT->FillRoundedRectangle(rect, b2.Get());

		if (txt.size())
		{
			D2D1_ROUNDED_RECT t1_rect = rect;
			t1_rect.rect.top -= txtCY;
			t1_rect.rect.bottom = t1_rect.rect.top + txtCY;
			_DrawText2(pRT, txt, t1_rect, CID::CtrlTxt, EXE_FONT::eUI_Font);
		}
	}
#pragma endregion UI_Drawing

#pragma region message_processing
	// WM_KEYDOWN and WM_SYSKEYDOWN
	virtual LRESULT _OnKeyDown(WPARAM ww, LPARAM ll) override
	{
		bool IsEditingColorValues = m_editing.part() != VT::None;
		if (ww == VK_RETURN)
		{
			if (IsEditingColorValues)
			{
				SELCOL = m_editing.IsError() ? m_editing.GetOriginalColor() : m_editing.GetColor();
				m_editing.Set(VT::None, SELCOL);
				_RedrawDirectly();
				_NotifySelColor();
			}
			else
			{
				m_isOkBtnClicked = true;
				_NotifyParent(ClrPickCtrlNotify::OkBtn);
			}
		}
		if (ww == VK_ESCAPE)
		{
			if (IsEditingColorValues)
			{
				SELCOL = m_editing.GetOriginalColor();
				m_editing.Set(VT::None, SELCOL);
				_NotifySelColor();
			}
			else
			{
				m_isCancelBtnClicked = true;
				_NotifyParent(ClrPickCtrlNotify::CancelBtn);
			}
		}
		if (IsEditingColorValues)
		{
			if (m_editing.OnKeyDown(ww) && !m_editing.IsError())
			{
				SELCOL = m_editing.GetColor();
				_NotifySelColor();
			}
			_RedrawDirectly();
		}
		return 0;
	}

	virtual LRESULT _OnChar(WPARAM ww, LPARAM ll)
	{
		if (m_editing.part() != VT::None)
		{
			if (m_editing.OnChar(ww) && !m_editing.IsError())
			{
				SELCOL = m_editing.GetColor();
				_NotifySelColor();
			}
			_RedrawDirectly();
		}
		return 0;
	}

	// WM_MOUSEWHEEL
	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint point) override
	{
		::ScreenToClient(Hwnd(), &point);
		if (m_editing.part() != VT::None)
		{
			return 0;
		}
		Nudge nudge = zDelta > 0 ? Nudge::Inc : Nudge::Dec;

		RCT inrc = _PtInRect((float)point.x, (float)point.y);
		bool isHueSlider = m_uiMode == UImode::HSL && inrc.IsHueOrRedSlider();
		bool isRedSlider = m_uiMode == UImode::RGB && inrc.IsHueOrRedSlider();
		bool isSatSlider = m_uiMode == UImode::HSL && inrc.IsSatOrGreenSlider();
		bool isGreenSlider = m_uiMode == UImode::RGB && inrc.IsSatOrGreenSlider();
		bool isLumSlider = m_uiMode == UImode::HSL && inrc.IsLumOrBlueSlider();
		bool isBlueSlider = m_uiMode == UImode::RGB && inrc.IsLumOrBlueSlider();
		if (isHueSlider || inrc.IsHueValue())			{ SELCOL.NudgeVT(nudge, VT::Hue); _NotifySelColor(); }
		else if (isRedSlider || inrc.IsRedValue())		{ SELCOL.NudgeVT(nudge, VT::Red); _NotifySelColor(); }
		else if (isSatSlider || inrc.IsSatValue())		{ SELCOL.NudgeVT(nudge, VT::Sat); _NotifySelColor(); }
		else if (isGreenSlider || inrc.IsGreenValue())	{ SELCOL.NudgeVT(nudge, VT::Green); _NotifySelColor(); }
		else if (isLumSlider || inrc.IsLumValue())		{ SELCOL.NudgeVT(nudge, VT::Lum); m_L = SELCOL.GetL(); _NotifySelColor(); }
		else if (isBlueSlider || inrc.IsBlueValue())	{ SELCOL.NudgeVT(nudge, VT::Blue); _NotifySelColor(); }
		_RedrawDirectly();
		return 0;
	}

	// WM_LBUTTONDOWN
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint point)
	{
		bool isSimple = m_layout == ColorPickerMode::Simple;

		RCT inrc = _PtInRect((float)point.x, (float)point.y);

		// If user is editing time parts and clicks anywhere in the control (except the value boxes)?
		if (m_editing.part() != VT::None && !inrc.IsAnyValue())
		{
			m_editing.Set(VT::None, SELCOL);	// Cancel edit time parts.
			_RedrawDirectly();
			_NotifySelColor();
		}

		switch (inrc)
		{
		case RCT::PicrBtn:
			m_isAdjusting = RCT::PicrBtn;
			SetCursor(LoadCursor(0, IDC_CROSS));
			if (!m_isPopupCtrl)
			{
				::SetCapture(Hwnd());
			}
			return 1;
		case RCT::Mode1Btn:
			m_uiMode = UImode::HSL;
			_RedrawDirectly();
			_NotifyParent(ClrPickCtrlNotify::ModeChange);
			return 1;
		case RCT::Mode2Btn:
			m_uiMode = UImode::RGB;
			_RedrawDirectly();
			_NotifyParent(ClrPickCtrlNotify::ModeChange);
			return 1;
		case RCT::CancelBtn:
			m_isCancelBtnClicked = true;
			_NotifyParent(ClrPickCtrlNotify::CancelBtn);
			return 1;
		case RCT::OkBtn:
			m_isOkBtnClicked = true;
			_NotifyParent(ClrPickCtrlNotify::OkBtn);
			return 1;
		case RCT::MoreBtn:
			if (m_isMoreOrLessButtonVisible)
			{
				// Notify parent that "More>>" (or "Less<<") button was clicked => expand the UI
				m_layout = isSimple ? ColorPickerMode::Full : ColorPickerMode::Simple;
				m_uiMode = UImode::RGB;
				m_hovering = RCT::None;
				if (m_isPopupCtrl)
				{
					::PostMessage(Hwnd(), m_notifyChangeWndSizeMsg, 0, 0);
				}
				else
				{
					_ChangeLayout();
				}
				_NotifyParent(ClrPickCtrlNotify::LayoutChange);
			}
			return 1;
		default:
			break;
		}

		if (inrc.IsAnySlider())
		{
			m_isAdjusting = inrc;
			_CalculateSelColorForSlider((float)point.y);
			_RedrawDirectly();
			return 1;
		}

		if (inrc.IsAnyValue())
		{
			m_editing.Set(inrc.ToVT(), SELCOL);
			::SetTimer(Hwnd(), BLINK_TIMERID, BLINK_TIME, 0);
			_RedrawDirectly();
			_NotifySelColor();
			return 1;
		}

		if (m_uiMode == UImode::HSL)
		{
			if (_SetSelectedColorFromPointInColorWheel((float)point.x, (float)point.y))
			{
				m_isAdjusting = RCT::ColorWheel;
			}
		}
		else	// RGB mode
		{
			if (SetColorFromPointInSquaresMap((float)point.x, (float)point.y))
			{
				if (isSimple)
				{
					// When "simple" - single click is select color.
					m_isOkBtnClicked = true;
					_NotifyParent(ClrPickCtrlNotify::OkBtn);
				}
				return 1;
			}
		}
		return 0;
	}

	// WM_LBUTTONUP
	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint point) override
	{
		if (m_isAdjusting == RCT::PicrBtn && !m_isPopupCtrl)
		{
			::ReleaseCapture();
		}
		m_isAdjusting = RCT::None;
		SetCursor(LoadCursor(0, IDC_ARROW));
		return 0;
	}

	// WM_LBUTTONDBLCLK
	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint point) override
	{
		if (m_uiMode == UImode::HSL)
		{
			if (_SetSelectedColorFromPointInColorWheel((float)point.x, (float)point.y))
			{
				m_isOkBtnClicked = true;
				_NotifyParent(ClrPickCtrlNotify::OkBtn);
			}
		}
		else	// RGB mode
		{
			if (SetColorFromPointInSquaresMap((float)point.x, (float)point.y))
			{
				m_isOkBtnClicked = true;
				_NotifyParent(ClrPickCtrlNotify::OkBtn);
			}
		}
		return 0;
	}

	// WM_MOUSEMOVE
	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{
		bool LeftClick = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

		if (LeftClick)
		{
			if (m_isAdjusting == RCT::PicrBtn)
			{
				POINT pt = { 0 };
				GetCursorPos(&pt);

				//  Get the device context of the desktop and from it get the color 
				//  of the pixel at the current mouse pointer position

				SetCursor(LoadCursor(0, IDC_CROSS));
				auto hDC = ::GetDCEx(NULL, NULL, 0);
				float fd = ScaleDPI / 96.0f;
				DWORD cr = GetPixel(hDC, (int)(pt.x * fd), (int)(pt.y * fd));
				ReleaseDC(0, hDC);
				SELCOL.SetFromRGB(cr, false);
				_RedrawDirectly();
				_NotifySelColor();
				return CXeD2DCtrlBase::_OnMouseMove(nFlags, point);
			}
			else if (m_isAdjusting.IsAnySlider())
			{
				_CalculateSelColorForSlider((float)point.y);
				_RedrawDirectly();
				return CXeD2DCtrlBase::_OnMouseMove(nFlags, point);
			}
			else if (m_isAdjusting == RCT::ColorWheel)
			{
				_SetSelectedColorFromPointInColorWheel((float)point.x, (float)point.y);
			}
		}
		else
		{
			RCT inrc = _PtInRect((float)point.x, (float)point.y);
			int idxHoverColorSquare = -1;
			if (inrc == RCT::None && m_uiMode != UImode::HSL)
			{
				_IsPointInSquaresMap((float)point.x, (float)point.y, &idxHoverColorSquare);
			}
			if (!m_isMoreOrLessButtonVisible && inrc == RCT::MoreBtn)
			{
				inrc = RCT::None;
			}
			if (inrc != RCT::None && !inrc.CanHover())
			{
				inrc = RCT::None;
			}
			bool isRedrawNeeded = false;
			if (inrc != m_hovering)
			{
				m_hovering = inrc;
				isRedrawNeeded = true;
			}
			if (m_idxHoverColorSquare != idxHoverColorSquare)
			{
				m_idxHoverColorSquare = idxHoverColorSquare;
				isRedrawNeeded = true;
			}
			if (isRedrawNeeded)
			{
				_RedrawDirectly();
			}
		}
		return CXeD2DCtrlBase::_OnMouseMove(nFlags, point);
	}

	// WM_TIMER message handler.
	virtual LRESULT _OnTimer(WPARAM ww, LPARAM ll) override
	{
		UINT_PTR nIDEvent = ww;
		XeASSERT(::IsWindow(Hwnd()));
		if (nIDEvent == BLINK_TIMERID)
		{
			if (m_editing.part() != VT::None)
			{
				m_editing.Blink();
				_RedrawDirectly();
			}
			else
			{
				::KillTimer(Hwnd(), BLINK_TIMERID);
			}
			return 0;
		}
		return CXeD2DCtrlBase::_OnTimer(ww, ll);
	}
#pragma endregion message_processing
};

export class CXeColorPickerDropDown
{
protected:
	CXeUIcolorsIF* m_xeUI = nullptr;

	COLORREF m_selectedColor;
	bool m_hasUserSelectedColor = false;
	ColorPickerMode m_pkr_mode = ColorPickerMode::Simple;

#pragma region Create
public:
	CXeColorPickerDropDown(CXeUIcolorsIF* pUIcolors, COLORREF selectedColor)
		: m_xeUI(pUIcolors), m_selectedColor(selectedColor)
	{
	}
	//virtual ~CXeColorPickerDropDown() {}
#pragma endregion Create

#pragma region ShowDropDown
public:
	void ShowDropDown(HWND hParent, const CRect& rcParentCtrlScreenPos,
		ColorPickerMode mode = ColorPickerMode::Simple)
	{
		m_pkr_mode = mode;
		CXeColorPicker color_picker(m_xeUI, mode);
		color_picker.SELCOL.SetFromRGB(m_selectedColor, false);
		color_picker.BEFORE = color_picker.SELCOL;
		color_picker.m_notifyChangeWndSizeMsg = NF_POPUP_WND_SIZE_CHANGE;
		CSize sCtrl = color_picker.GetControlSize();
		int scrX = rcParentCtrlScreenPos.left, scrY = rcParentCtrlScreenPos.bottom;
		CRect rPicker(scrX, scrY, scrX + sCtrl.cx, scrY + sCtrl.cy);
		CXePopupCtrl popup(m_xeUI, &color_picker, XeShowPopup::FadeIn80);
		popup.ShowPopup(hParent, rPicker, rcParentCtrlScreenPos);
		if (color_picker.m_isOkBtnClicked)
		{
			m_selectedColor = color_picker.SELCOL.ToRGB(false);
			m_hasUserSelectedColor = true;
			m_pkr_mode = color_picker.m_layout;
		}
	}

	COLORREF GetSelectedColor() const { return m_selectedColor; }
	bool HasUserSelectedColor() const { return m_hasUserSelectedColor; }

	ColorPickerMode GetColorPickerLayoutMode() const { return m_pkr_mode; }
#pragma endregion ShowDropDown
};

