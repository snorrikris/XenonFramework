module;

#include "os_minimal.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <d2d1.h>
#include <dwrite.h>

export module Xe.DurationCtrl;

import Xe.PopupCtrlBase;
import Xe.PopupCtrl;
import Xe.UIcolorsIF;
import Xe.D2DWndBase;
import Xe.StringTools;
import Xe.Helpers;
import Xe.FileTimeX;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Notification codes (in NMHDR.code) sent to parent.
export constexpr UINT DCN_DURCHANGED = 0x3201;	// Duration has changed.

constexpr UINT LBTN_DOWN_TIMERID = 1001;
constexpr UINT LBTN_DOWN_TIME = 200;		// mS - time to first auto repeat.
constexpr UINT LBTN_REPT_TIMERID = 1002;
constexpr UINT LBTN_REPT_TIME = 100;		// mS - repeat interval.
constexpr UINT BLINK_TIMERID = 1003;
constexpr UINT BLINK_TIME = 500;			// mS - repeat interval.

export class DurCtrlMode
{
public:
	enum DCM_Enum
	{
		DurationSM,	// Duration with Ok and Cancel buttons - normal UI font
		DurationSM2	// Duration - normal UI font
	};

protected:
	DCM_Enum m_e = DCM_Enum::DurationSM;

public:
	DurCtrlMode() = default;
	/* implicit */ constexpr DurCtrlMode(DCM_Enum e) : m_e(e) {}

	// Allows comparisons with Enum constants.
	constexpr operator DCM_Enum() const { return m_e; }

	// Needed to prevent if(c).
	explicit operator bool() const = delete;

	bool HasOkAndCancelButtons() const
	{
		return m_e == DCM_Enum::DurationSM;
	}

	EXE_FONT GetFontForMode() const
	{
		switch (m_e)
		{
		case DCM_Enum::DurationSM:
		case DCM_Enum::DurationSM2:
			return EXE_FONT::eUI_Font;
		}
		XeASSERT(false);	// Unknown mode.
		return EXE_FONT::eUI_Font;
	}
};

enum class RCT {
	None, Days, Hours, Minutes, Seconds, MilliSecs, DaysDec, DaysInc,
	HoursDec, HoursInc, MinutesDec, MinutesInc, SecondsDec, SecondsInc, MilliSecsDec, MilliSecsInc,
	OkBtn, CancelBtn
};

struct PartBuffer
{
	RCT m_part = RCT::None;
	int m_num_digits = 0;	// Number of digits - including the sign (if used).
	int m_min, m_max, m_step;
	bool m_hasSign, m_isNegative;
	wchar_t m_digits[5];

	PartBuffer(RCT part) : m_part(part)
	{
		if (part == RCT::Days)
		{
			m_num_digits = 4, m_min = -999, m_max = 999, m_step = 1;
			m_hasSign = true, m_isNegative = false;
		}
		else if (part == RCT::Hours)
		{
			m_num_digits = 2, m_min = 0, m_max = 23, m_step = 1;
			m_hasSign = false, m_isNegative = false;
		}
		else if (part == RCT::MilliSecs)
		{
			m_num_digits = 3, m_min = 0, m_max = 999, m_step = 50;
			m_hasSign = false, m_isNegative = false;
		}
		else
		{
			m_num_digits = 2, m_min = 0, m_max = 59, m_step = 1;
			m_hasSign = false, m_isNegative = false;
		}
	}

	void set(int value, bool isNegative = false)
	{
		XeASSERT(value >= 0 && value >= m_min && value <= m_max && (!isNegative || (isNegative && m_hasSign)));
		m_isNegative = m_hasSign ? isNegative : false;
		switch (m_part)
		{
		case RCT::Days:
			if (isNegative)
			{
				swprintf_s(m_digits, 5, L"-%03u", value);
			}
			else
			{
				swprintf_s(m_digits, 5, L"+%03u", value);
			}
			break;
		case RCT::MilliSecs:
			swprintf_s(m_digits, 5, L"%03u", value);
			break;
		default:
			swprintf_s(m_digits, 5, L"%02u", value);
			break;
		}
	}

	void NudgeValue(Nudge nudge)
	{
		int w = as_value();
		if (nudge == Nudge::Dec)
		{
			w = w == m_min ? m_max : w - m_step;
		}
		else
		{
			w = w == m_max ? m_min : w + m_step;
		}
		w = w < m_min ? m_min : w;
		w = w > m_max ? m_max : w;
		int value = std::abs(w);
		bool isNegative = w < 0;
		set(value, isNegative);
	}

	bool IsValid() const
	{
		int val = as_value();
		return val >= m_min && val <= m_max;
	}

	std::wstring as_string() const { return std::wstring(m_digits); }

	int as_value() const
	{
		return xen::stoi(as_string());
	}
};

class PartEdit
{
protected:
	FILETIMESPAN m_duration;
	std::vector<PartBuffer> m_buffers;
	RCT m_editing = RCT::None;
	int m_cur_digit = 0;
	bool m_isBlinkOn = false;

public:
	PartEdit()
	{
		m_buffers.push_back(PartBuffer(RCT::Days));
		m_buffers.push_back(PartBuffer(RCT::Hours));
		m_buffers.push_back(PartBuffer(RCT::Minutes));
		m_buffers.push_back(PartBuffer(RCT::Seconds));
		m_buffers.push_back(PartBuffer(RCT::MilliSecs));
	}

protected:
	RCT _GetRealPart(RCT part)
	{
		switch (part)
		{
		case RCT::DaysDec:
		case RCT::DaysInc:
			return RCT::Days;
		case RCT::HoursDec:
		case RCT::HoursInc:
			return RCT::Hours;
		case RCT::MinutesDec:
		case RCT::MinutesInc:
			return RCT::Minutes;
		case RCT::SecondsDec:
		case RCT::SecondsInc:
			return RCT::Seconds;
		case RCT::MilliSecsDec:
		case RCT::MilliSecsInc:
			return RCT::MilliSecs;
		}
		return part;
	}

	PartBuffer* _GetBuffer(RCT part)
	{
		part = _GetRealPart(part);
		auto it = std::find_if(m_buffers.begin(), m_buffers.end(),
			[part](PartBuffer& buf) { return buf.m_part == part; });
		return it != m_buffers.end() ? &(*it) : nullptr;
	}

	RCT _GetNext(RCT rct)
	{
		switch (rct)
		{
		case RCT::Days:			return RCT::Hours;
		case RCT::Hours:		return RCT::Minutes;
		case RCT::Minutes:		return RCT::Seconds;
		case RCT::Seconds:		return RCT::MilliSecs;
		case RCT::MilliSecs:	return RCT::None;
		}
		return RCT::None;
	}
	RCT _GetPrev(RCT rct)
	{
		switch (rct)
		{
		case RCT::Days:			return RCT::None;
		case RCT::Hours:		return RCT::Days;
		case RCT::Minutes:		return RCT::Hours;
		case RCT::Seconds:		return RCT::Minutes;
		case RCT::MilliSecs:	return RCT::Seconds;
		}
		return RCT::None;
	}

	void _MoveEditPart(bool isMoveNext)
	{
		m_editing = isMoveNext ? _GetNext(m_editing) : _GetPrev(m_editing);
		PartBuffer* pBuf = _GetBuffer(m_editing);
		if (pBuf)
		{
			m_cur_digit = isMoveNext ? 0 : pBuf->m_num_digits - 1;
			m_isBlinkOn = false;
		}
	}

public:
	FILETIMESPAN GetDuration() const
	{
		if (AnyErrors())
		{
			return m_duration;
		}
		FILETIMESPAN dur;
		dur.Set(m_buffers[0].as_value(), m_buffers[1].as_value(), m_buffers[2].as_value(),
			m_buffers[3].as_value(), m_buffers[4].as_value());
		if (!dur.IsNegative() && m_buffers[0].m_digits[0] == L'-')
		{
			dur.Set(dur.m_n64Span * -1);
		}
		return dur;
	}

	void Set(const FILETIMESPAN& dur)
	{
		m_duration = dur;
		m_editing = RCT::None;
		m_cur_digit = 0;
		m_isBlinkOn = false;

		FILETIMESPAN d2 = dur.abs();
		m_buffers[0].set((int)d2.GetDays(), dur.IsNegative());
		m_buffers[1].set(d2.GetHours());
		m_buffers[2].set(d2.GetMinutes());
		m_buffers[3].set(d2.GetSeconds());
		m_buffers[4].set(d2.GetMilliSeconds());
	}

	RCT GetEditingPart() const { return m_editing; }

	// Return true if editing part is not None.
	bool ToggleEditingPart(RCT part)
	{
		m_editing = part == m_editing ? RCT::None : part;
		return m_editing != RCT::None;
	}

	void CancelEditing()
	{
		m_editing = RCT::None;
		m_cur_digit = 0;
		m_isBlinkOn = false;
	}

	void Blink() { m_isBlinkOn = !m_isBlinkOn; }

	std::wstring GetText(RCT rct)
	{
		PartBuffer* pBuf = _GetBuffer(rct);
		if (pBuf)
		{
			if (m_editing == RCT::None)
			{
				return pBuf->as_string();
			}

			std::wstring s(pBuf->m_digits);
			if (m_editing == rct && m_isBlinkOn && m_cur_digit >= 0 && m_cur_digit < s.size())
			{
				s[m_cur_digit] = L'#';
			}
			return s;
		}
		XeASSERT(false);
		return std::wstring();
	}

	std::vector<RCT> GetErrors() const
	{
		std::vector<RCT> errors;
		for (const PartBuffer& part : m_buffers)
		{
			if (!part.IsValid())
			{
				errors.push_back(part.m_part);
			}
		}
		return errors;
	}

	bool AnyErrors() const
	{
		std::vector<RCT> errors = GetErrors();
		return errors.size() > 0;
	}

	// Return true if date/time value has (maybe) changed.
	bool OnKeyDown(WPARAM ww)
	{
		PartBuffer* pBuf = _GetBuffer(m_editing);
		if (!pBuf)
		{
			return false;
		}
		if (ww == VK_LEFT || ww == VK_BACK)
		{
			if (m_cur_digit > 0)
			{
				--m_cur_digit;
				return false;
			}
			_MoveEditPart(false);
		}
		else if (ww == VK_RIGHT)
		{
			if (m_cur_digit < (pBuf->m_num_digits - 1))
			{
				++m_cur_digit;
				return false;
			}
			_MoveEditPart(true);
		}
		else if (ww == VK_TAB)
		{
			_MoveEditPart(true);
		}
		return false;
	}

	// Return true if date/time value has (maybe) changed.
	bool OnChar(WPARAM ww)
	{
		PartBuffer* pBuf = _GetBuffer(m_editing);
		XeASSERT(pBuf);
		if (!pBuf) { return false; }
		if (ww == '-' || ww == '+')
		{
			if (m_editing == RCT::Days && m_cur_digit == 0)	// + or - only allowed in first digit for days.
			{
				pBuf->m_digits[m_cur_digit++] = (wchar_t)ww;
				return true;
			}
		}
		else if (ww >= '0' && ww <= '9' && m_cur_digit < 5)
		{
			pBuf->m_digits[m_cur_digit] = (wchar_t)ww;
			if (++m_cur_digit >= pBuf->m_num_digits)
			{
				_MoveEditPart(true);
			}
			return true;
		}
		else if (ww == ' ' || ww == '/' || ww == ',' || ww == '.')
		{
			_MoveEditPart(true);
		}
		return false;
	}

	void NudgeValue(Nudge nudge, RCT part)
	{
		PartBuffer* pBuf = _GetBuffer(part);
		XeASSERT(pBuf);
		if (pBuf)
		{
			pBuf->NudgeValue(nudge);
		}
	}
};

typedef std::function<void()> DurationCtrlNotifyCallbackFunc;

constexpr wchar_t XEDURATIONWND_CLASSNAME[] = L"XeDurationWndClass";  // Window class name

export class CXeDurationCtrl : public CXePopupCtrlBase, public CXeD2DCtrlBase
{
#pragma region class_data
protected:
	bool m_isPopupCtrl = false;	// true when 'this' is a popup control.

	DurCtrlMode m_mode = DurCtrlMode::DurationSM;

	bool m_isReadOnly = false;

	CSize m_sizeCtrl;

	PartEdit m_rctEditing;

	RCT m_rctHover = RCT::None;

	RCT m_rctRepeat = RCT::None;

	bool m_isEnabled = true;

public:
	bool m_isOkBtnClicked = false, m_isCancelBtnClicked = false;

	// Parent window sets the callback (used when calendar control is just a normal control - i.e. not a popup).
	DurationCtrlNotifyCallbackFunc m_nfCallback = nullptr;
#pragma endregion class_data

#pragma region Create
public:
	CXeDurationCtrl(CXeUIcolorsIF* pUIcolors, DurCtrlMode mode)
		: CXeD2DCtrlBase(pUIcolors), m_mode(mode)
	{
		m_xeUI->RegisterWindowClass(XEDURATIONWND_CLASSNAME, D2DCtrl_WndProc);

		m_rctEditing.Set(FILETIMESPAN());

		m_sizeCtrl = _CalculateUI();
	}
	//virtual ~CXeDurationCtrl() {}

	bool Create(DWORD dwStyle, const CRect& rect, HWND hParentWnd,
			UINT nID, const wchar_t* tooltip)
	{
		DWORD dwExStyle = 0;
		HWND hWnd = CreateD2DCtrl(dwExStyle, XEDURATIONWND_CLASSNAME, nullptr, dwStyle,
				rect, hParentWnd, nID, tooltip);
		return hWnd != 0;
	}

	void SetReadOnly() { m_isReadOnly = true; }
#pragma endregion Create

#pragma region CXePopupCtrlBase_impl
public:
	virtual HWND GetHwnd() const override { return Hwnd(); }
	virtual HWND GetVscrollHwnd() const override { return 0; }
	virtual HWND GetHscrollHwnd() const override { return 0; }

	virtual CSize GetWindowPreferredSize() override
	{
		return GetControlSize();
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
				return 1;
			}
		}
		return CXeD2DCtrlBase::_OnMessageD2DBase(hwnd, uiMsg, wParam, lParam);
	}
#pragma endregion CXePopupCtrlBase_impl

#pragma region helpers
public:
	CSize GetControlSize() { return m_sizeCtrl; }

	FILETIMESPAN GetDuration() const { return m_rctEditing.GetDuration(); }

	void SetDuration(const FILETIMESPAN& dur)
	{
		m_rctEditing.Set(dur);
		m_rctHover = m_rctRepeat = RCT::None;
		if (::IsWindow(Hwnd()))
		{
			_RedrawDirectly();
		}
	}

protected:
	bool _CanEdit() const { return !m_isReadOnly && m_isEnabled; }

	RCT _PtInRect(const float& x, const float& y)
	{
		int key = m_rr.PtInRect(x, y);
		if (key < 0)
		{
			return RCT::None;
		}
		RCT rct = (RCT)key;
		return rct;
	}

	bool IsAnyTimePartBox(RCT rct) const
	{
		return rct == RCT::Days
				|| rct == RCT::Hours || rct == RCT::Minutes || rct == RCT::Seconds || rct == RCT::MilliSecs;
	}

	bool IsAnyTimePartDecBox(RCT rct) const
	{
		return rct == RCT::DaysDec || rct == RCT::HoursDec
				|| rct == RCT::MinutesDec || rct == RCT::SecondsDec || rct == RCT::MilliSecsDec;
	}

	bool IsAnyTimePartIncBox(RCT rct) const
	{
		return rct == RCT::DaysInc || rct == RCT::HoursInc
				|| rct == RCT::MinutesInc || rct == RCT::SecondsInc || rct == RCT::MilliSecsInc;
	}

	void _NotifyParent()
	{
		HWND hWndParent = ::GetParent(Hwnd());
		if (hWndParent)
		{
			NMHDR nf{ 0 };
			nf.hwndFrom = Hwnd();
			nf.code = DCN_DURCHANGED;
			nf.idFrom = GetDlgCtrlID();
			// Note - caller needs to call GetDuration.
			::SendMessageW(hWndParent, WM_NOTIFY, nf.code, reinterpret_cast<LPARAM>(&nf));
		}
		if (m_nfCallback)
		{
			m_nfCallback();
		}
	}
#pragma endregion helpers

#pragma region CalculateUI
protected:
	CSize _CalculateUI()	// Return size needed for control.
	{
		D2D1_SIZE_F szF{};
		EXE_FONT mode_font = m_mode.GetFontForMode();

		m_xeUI->D2D_GetTextSize(std::wstring(L"W"), EXE_FONT::eUI_Font, szF);
		float mul = 0.5f;
		float marginL = szF.width * mul, spaceV = marginL * 0.5f, wndCX = 0;
		float marginT = marginL;
		float x = marginL, y = marginT;

		m_xeUI->D2D_GetTextSize(std::wstring(L"Cancel"), EXE_FONT::eUI_Font, szF);
		float okBtnCX = szF.width * 1.4f;
		float okBtnCY = szF.height * 1.2f;

		// Calculate timeBox time boxes
		m_xeUI->D2D_GetTextSize(std::wstring(L"-000"), mode_font, szF);
		float daysCX = szF.width * 1.2f;
		m_xeUI->D2D_GetTextSize(std::wstring(L"00"), mode_font, szF);
		float hhCX = szF.width * 1.2f;
		float hhCY = szF.height;
		m_xeUI->D2D_GetTextSize(std::wstring(L"000"), mode_font, szF);
		float msCX = szF.width * 1.2f;
		float cx_dot = hhCY * 0.3f;
		float cy_inc = hhCY * 0.3f, cx_inc = hhCX * 0.8f;
		float cy_spc = hhCY * 0.05f;
		x = marginL * 0.5f;
		float x_ofs = (daysCX - cx_inc) / 2;
		m_rr.Set((int)RCT::Days, x, y, daysCX, hhCY);
		m_rr.Set((int)RCT::DaysInc, x + x_ofs, y - cy_inc - cy_spc, cx_inc, cy_inc);
		m_rr.Set((int)RCT::DaysDec, x + x_ofs, y + hhCY + cy_spc, cx_inc, cy_inc);
		x += daysCX + cx_dot;
		x_ofs = (hhCX - cx_inc) / 2;
		m_rr.Set((int)RCT::Hours, x, y, hhCX, hhCY);
		m_rr.Set((int)RCT::HoursInc, x + x_ofs, y - cy_inc - cy_spc, cx_inc, cy_inc);
		m_rr.Set((int)RCT::HoursDec, x + x_ofs, y + hhCY + cy_spc, cx_inc, cy_inc);
		x += hhCX + cx_dot;
		m_rr.Set((int)RCT::Minutes, x, y, hhCX, hhCY);
		m_rr.Set((int)RCT::MinutesInc, x + x_ofs, y - cy_inc - cy_spc, cx_inc, cy_inc);
		m_rr.Set((int)RCT::MinutesDec, x + x_ofs, y + hhCY + cy_spc, cx_inc, cy_inc);
		x += hhCX + cx_dot;
		m_rr.Set((int)RCT::Seconds, x, y, hhCX, hhCY);
		m_rr.Set((int)RCT::SecondsInc, x + x_ofs, y - cy_inc - cy_spc, cx_inc, cy_inc);
		m_rr.Set((int)RCT::SecondsDec, x + x_ofs, y + hhCY + cy_spc, cx_inc, cy_inc);
		x += hhCX + cx_dot;
		D2D1_ROUNDED_RECT ms_dec_Btn = m_rr.Set((int)RCT::MilliSecs, x, y, msCX, hhCY);
		x_ofs = (msCX - cx_inc) / 2;
		D2D1_ROUNDED_RECT msIncBox
				= m_rr.Set((int)RCT::MilliSecsInc, x + x_ofs, y - cy_inc - cy_spc, cx_inc, cy_inc);
		D2D1_ROUNDED_RECT msDecBox
				= m_rr.Set((int)RCT::MilliSecsDec, x + x_ofs, y + hhCY + cy_spc, cx_inc, cy_inc);

		wndCX = x + msCX + (marginL * 0.5f);	// Window width when small duration

		// Ok and Cancel buttons
		XeASSERT(wndCX > 0);
		if (m_mode.HasOkAndCancelButtons())
		{
			float cxOkCancelButtons = (2 * okBtnCX) + marginL;
			x = (wndCX - cxOkCancelButtons) / 2;	// small duration
			y = msDecBox.rect.bottom + spaceV;
			m_rr.Set((int)RCT::OkBtn, x, y, okBtnCX, okBtnCY);
			x += okBtnCX + marginL;
			D2D1_ROUNDED_RECT cancelBtn = m_rr.Set((int)RCT::CancelBtn, x, y, okBtnCX, okBtnCY);

			float wndCY = cancelBtn.rect.bottom + marginT;
			return CSize((int)(wndCX + 0.5), (int)(wndCY + 0.5));
		}

		// When no Ok and Cancel buttons - window height is element DEC button bottom + margin.
		float wndCY = ms_dec_Btn.rect.bottom + marginT + 1.0f;
		return CSize((int)(wndCX + 0.5), (int)(wndCY + 0.5));
	}
#pragma endregion CalculateUI

#pragma region UI_Drawing
protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		pRT->Clear(m_xeUI->GetColorF(_CanEdit() ? CID::DialogBg : CID::CtrlBgDis));
		D2D1_ROUNDED_RECT rcR(rc, 1, 1);
		pRT->DrawRoundedRectangle(rcR, GetBrush(CID::CtrlBorder));

		_DrawTimeBox(pRT);

		// OK and CANCEL buttons
		if (m_mode.HasOkAndCancelButtons())
		{
			_DrawButton(pRT, (int)RCT::OkBtn, std::wstring(L"Ok"), m_rctHover == RCT::OkBtn);
			_DrawButton(pRT, (int)RCT::CancelBtn, std::wstring(L"Cancel"), m_rctHover == RCT::CancelBtn);
		}

		if (::GetFocus() == Hwnd())
		{
			_DrawFocusRect(pRT, DeflateRectF(rc, 2.5f));
		}
	}

	virtual void _CreatePaintResources(ID2D1RenderTarget* pRT) override
	{
		CXeD2DCtrlBase::_CreatePaintResources(pRT);
	}

	void _DrawTimeBox(ID2D1RenderTarget* pRT)
	{
		EXE_FONT eFont = m_mode.GetFontForMode();

		if (m_rctEditing.GetEditingPart() != RCT::None)
		{
			_FillRoundedRectangle(pRT, (int)m_rctEditing.GetEditingPart(), CID::CtrlBg);
		}
		CID fgColor = _CanEdit() ? CID::CtrlTxt : CID::CtrlTxtDis;
		_DrawText(pRT, m_rctEditing.GetText(RCT::Days), (int)RCT::Days, fgColor, eFont);

		_DrawTextBetweenBoxes(pRT, std::wstring(L"."), (int)RCT::Days, (int)RCT::Hours, fgColor);

		_DrawText(pRT, m_rctEditing.GetText(RCT::Hours), (int)RCT::Hours, fgColor, eFont);

		_DrawTextBetweenBoxes(pRT, std::wstring(L":"), (int)RCT::Hours, (int)RCT::Minutes, fgColor);

		_DrawText(pRT, m_rctEditing.GetText(RCT::Minutes), (int)RCT::Minutes, fgColor, eFont);

		_DrawTextBetweenBoxes(pRT, std::wstring(L":"), (int)RCT::Minutes, (int)RCT::Seconds, fgColor);

		_DrawText(pRT, m_rctEditing.GetText(RCT::Seconds), (int)RCT::Seconds, fgColor, eFont);

		_DrawTextBetweenBoxes(pRT, std::wstring(L","), (int)RCT::Seconds, (int)RCT::MilliSecs, fgColor);

		_DrawText(pRT, m_rctEditing.GetText(RCT::MilliSecs), (int)RCT::MilliSecs, fgColor, eFont);

		if (IsAnyTimePartBox(m_rctHover))
		{
			_DrawRoundedRectangle(pRT, (int)m_rctHover, CID::CtrlBtnDefBorder);
		}

		if (m_rctEditing.GetEditingPart() != RCT::None)
		{
			_DrawRoundedRectangle(pRT, (int)m_rctEditing.GetEditingPart(), CID::CtrlBtnDefBorder);
		}

		std::vector<RCT> errors = m_rctEditing.GetErrors();
		for (RCT err_rct : errors)
		{
			_DrawRoundedRectangle(pRT, (int)err_rct, CID::MrgLogNoDataBg);
		}

		bool isHoverDecBox = IsAnyTimePartDecBox(m_rctHover);
		if (isHoverDecBox || IsAnyTimePartIncBox(m_rctHover))
		{
			_DrawTriangle(pRT, (int)m_rctHover, isHoverDecBox, CID::CtrlBtnDefBorder);
		}
	}
#pragma endregion UI_Drawing

#pragma region message_processing
	// WM_KEYDOWN and WM_SYSKEYDOWN
	virtual LRESULT _OnKeyDown(WPARAM ww, LPARAM ll) override
	{
		if (ww == VK_RETURN)
		{
			m_isOkBtnClicked = true;
			_NotifyParent();
		}
		if (ww == VK_ESCAPE)
		{
			m_isCancelBtnClicked = true;
			_NotifyParent();
		}
		if (m_rctEditing.GetEditingPart() != RCT::None)
		{
			if (m_rctEditing.OnKeyDown(ww) && !m_rctEditing.AnyErrors())
			{
				_NotifyParent();
			}
			_RedrawDirectly();
		}
		return 0;
	}

	virtual LRESULT _OnChar(WPARAM ww, LPARAM ll)
	{
		if (m_rctEditing.GetEditingPart() != RCT::None)
		{
			if (m_rctEditing.OnChar(ww) && !m_rctEditing.AnyErrors())
			{
				_NotifyParent();
			}
			_RedrawDirectly();
		}
		return 0;
	}

	// WM_MOUSEMOVE
	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{
		RCT inrc = _PtInRect((float)point.x, (float)point.y);
		if (_CanEdit() && inrc != m_rctHover)
		{
			m_rctHover = inrc;
			_RedrawDirectly();
		}
		return CXeD2DCtrlBase::_OnMouseMove(nFlags, point);
	}

	// WM_MOUSELEAVE message handler.
	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override
	{
		m_rctHover = RCT::None;
		return CXeD2DCtrlBase::_OnMouseLeave(wparam, lparam);
	}

	// WM_MOUSEWHEEL
	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint point) override
	{
		::ScreenToClient(Hwnd(), &point);
		Nudge nudge = zDelta > 0 ? Nudge::Inc : Nudge::Dec;

		if (_CanEdit() && m_rctEditing.GetEditingPart() == RCT::None)
		{
			RCT inrc = _PtInRect((float)point.x, (float)point.y);
			if (IsAnyTimePartBox(inrc))
			{
				m_rctEditing.NudgeValue(nudge, inrc);
				_RedrawDirectly();
				_NotifyParent();
			}
		}
		return 0;
	}

	// WM_LBUTTONDOWN
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint point)
	{
		if (!m_isPopupCtrl && m_style.WS().hasTabStop())
		{
			::SetFocus(Hwnd());	// Grab focus on Ldown when 'this' ctrl has TAB_STOP.
		}

		RCT inrc = _PtInRect((float)point.x, (float)point.y);

		// Is user editing time parts and clicks anywhere in the control (except the time parts boxes)?
		if (m_rctEditing.GetEditingPart() != RCT::None && !IsAnyTimePartBox(inrc))
		{
			m_rctEditing.CancelEditing();	// Cancel edit time parts.
			_RedrawDirectly();
		}

		bool isTimeDec = IsAnyTimePartDecBox(inrc);
		bool isTimeInc = IsAnyTimePartIncBox(inrc);
		if (_CanEdit() && (isTimeDec || isTimeInc))
		{
			m_rctEditing.NudgeValue(isTimeDec ? Nudge::Dec : Nudge::Inc, inrc);
			_RedrawDirectly();
			::SetTimer(Hwnd(), LBTN_DOWN_TIMERID, LBTN_DOWN_TIME, 0);
			m_rctRepeat = inrc;
			_NotifyParent();
			return 0;
		}

		bool isNotifyNeeded = false;
		switch (inrc)
		{
		case RCT::CancelBtn:
			m_isCancelBtnClicked = true;
			isNotifyNeeded = true;
			break;
		case RCT::OkBtn:
			m_isOkBtnClicked = true;
			isNotifyNeeded = true;
			break;
		case RCT::Days:
		case RCT::Hours:
		case RCT::Minutes:
		case RCT::Seconds:
		case RCT::MilliSecs:
			if (_CanEdit() && m_rctEditing.ToggleEditingPart(inrc))
			{
				::SetTimer(Hwnd(), BLINK_TIMERID, BLINK_TIME, 0);
			}
			_RedrawDirectly();
			isNotifyNeeded = true;
			break;
		default:
			break;
		}
		if (isNotifyNeeded)
		{
			_NotifyParent();
		}
		return 0;
	}

	// WM_LBUTTONUP
	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint point) override
	{
		::KillTimer(Hwnd(), LBTN_DOWN_TIMERID);
		::KillTimer(Hwnd(), LBTN_REPT_TIMERID);
		return 0;
	}

	// WM_LBUTTONDBLCLK
	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint point) override
	{
		RCT inrc = _PtInRect((float)point.x, (float)point.y);

		bool isTimeDec = IsAnyTimePartDecBox(inrc);
		if (_CanEdit() && (isTimeDec || IsAnyTimePartIncBox(inrc)))
		{
			m_rctEditing.NudgeValue(isTimeDec ? Nudge::Dec : Nudge::Inc, inrc);
			_RedrawDirectly();
			_NotifyParent();
		}
		return 0;
	}

	// WM_TIMER message handler.
	virtual LRESULT _OnTimer(WPARAM ww, LPARAM ll) override
	{
		UINT_PTR nIDEvent = ww;
		XeASSERT(::IsWindow(Hwnd()));
		if (nIDEvent == LBTN_DOWN_TIMERID)
		{	// First auto repeat timer event.
			::KillTimer(Hwnd(), LBTN_DOWN_TIMERID);
			::SetTimer(Hwnd(), LBTN_REPT_TIMERID, LBTN_REPT_TIME, 0);
		}
		if (nIDEvent == LBTN_DOWN_TIMERID || nIDEvent == LBTN_REPT_TIMERID)
		{	// Auto repeat
			CPoint ptCurMouse;
			if (::GetCursorPos(&ptCurMouse))
			{
				::ScreenToClient(Hwnd(), &ptCurMouse);
				RCT inrc = _PtInRect((float)ptCurMouse.x, (float)ptCurMouse.y);
				if (inrc != m_rctRepeat)
				{
					::KillTimer(Hwnd(), LBTN_REPT_TIMERID);
					return 0;
				}
			}
			bool isTimeDec = IsAnyTimePartDecBox(m_rctRepeat);
			bool isTimeInc = IsAnyTimePartIncBox(m_rctRepeat);
			if (isTimeDec || isTimeInc)
			{
				m_rctEditing.NudgeValue(isTimeDec ? Nudge::Dec : Nudge::Inc, m_rctRepeat);
				_RedrawDirectly();
				_NotifyParent();
			}
			return 0;
		}
		if (nIDEvent == BLINK_TIMERID)
		{
			if (m_rctEditing.GetEditingPart() != RCT::None)
			{
				m_rctEditing.Blink();
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

	// WM_GETDLGCODE message handler.
	// See: https://learn.microsoft.com/en-us/archive/msdn-magazine/2005/march/making-static-links-keyboard-capable-launching-urls-from-an-app
	// See: http://pauldilascia.com/PixieLib.asp.html
	// See: https://jeffpar.github.io/kbarchive/kb/083/Q83302/
	virtual LRESULT _OnGetDlgCode(WPARAM wParam, LPARAM lParam) override
	{
		return DLGC_WANTCHARS | DLGC_WANTARROWS;
	}

	// WM_SETFOCUS message handler.
	virtual LRESULT _OnSetFocus(HWND hOldWnd) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		_RedrawDirectly();
		return 0;
	}

	// WM_KILLFOCUS message handler.
	virtual LRESULT _OnKillFocus(HWND hNewWnd) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_rctEditing.CancelEditing();	// Cancel edit time parts.
		_RedrawDirectly();
		return 0;
	}

	// WM_ENABLE message handler.
	virtual LRESULT _OnEnableWindow(bool isEnabled) override
	{
		XeASSERT(::IsWindow(Hwnd()));

		m_isEnabled = isEnabled;
		_RedrawDirectly();
		return 0;
	}
#pragma endregion message_processing
};

export class CXeDurationCtrlDropDown
{
protected:
	CXeUIcolorsIF* m_xeUI = nullptr;

public:
	CXeDurationCtrlDropDown(CXeUIcolorsIF* pUIcolors) : m_xeUI(pUIcolors) {}
	//virtual ~CXeDurationCtrlDropDown() {}

public:
	FILETIMESPAN ShowDropDown(HWND hParent, const CRect& rcParentCtrlScreenPos,
		const FILETIMESPAN& dur, DurCtrlMode mode = DurCtrlMode::DurationSM)
	{
		CXeDurationCtrl duration_ctrl(m_xeUI, mode);
		duration_ctrl.SetDuration(dur);
		CSize sCtrl = duration_ctrl.GetControlSize();
		int scrX = rcParentCtrlScreenPos.left, scrY = rcParentCtrlScreenPos.bottom;
		CRect rcCtrl(scrX, scrY, scrX + sCtrl.cx, scrY + sCtrl.cy);
		CXePopupCtrl popup(m_xeUI, &duration_ctrl, XeShowPopup::FadeIn80);
		popup.ShowPopup(hParent, rcCtrl, rcParentCtrlScreenPos);
		if (duration_ctrl.m_isOkBtnClicked)
		{
			return duration_ctrl.GetDuration();
		}
		return FILETIMESPAN();
	}
};

