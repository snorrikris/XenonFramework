module;

#include "os_minimal.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

export module Xe.CalendarCtrl;

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

/* Notify messages (sent to parent)
DTN_DATETIMECHANGE	(NMDATETIMECHANGE structure)
DTN_DROPDOWN
DTN_CLOSEUP
*/

constexpr UINT LBTN_DOWN_TIMERID = 1001;
constexpr UINT LBTN_DOWN_TIME = 200;		// mS - time to first auto repeat.
constexpr UINT LBTN_REPT_TIMERID = 1002;
constexpr UINT LBTN_REPT_TIME = 100;		// mS - repeat interval.
constexpr UINT BLINK_TIMERID = 1003;
constexpr UINT BLINK_TIME = 500;			// mS - repeat interval.

export class CalCtrlMode
{
public:
	enum CCM_Enum
	{
		FullRange,			// Date/time "from" - date/time "to", calendar, duration
		SimpleDateTime,		// Date/time only with Ok and Cancel buttons - medium big font
		SimpleDateTimeSM,	// Date/time only with Ok and Cancel buttons - normal UI font
		SimpleDateTime2,	// Date/time only - medium big font
		SimpleDateTimeSM2	// Date/time only - normal UI font
	};

protected:
	CCM_Enum m_e = CCM_Enum::FullRange;

public:
	CalCtrlMode() = default;
	/* implicit */ constexpr CalCtrlMode(CCM_Enum e) : m_e(e) {}

	// Allows comparisons with Enum constants.
	constexpr operator CCM_Enum() const { return m_e; }

	// Needed to prevent if(c).
	explicit operator bool() const = delete;

	bool IsFullMode() const { return m_e == CCM_Enum::FullRange; }

	bool IsMinimalMode() const { return m_e == CCM_Enum::SimpleDateTimeSM2; }

	bool HasOkAndCancelButtons() const
	{
		return m_e == CCM_Enum::FullRange || m_e == CCM_Enum::SimpleDateTime
				|| m_e == CCM_Enum::SimpleDateTimeSM;
	}

	EXE_FONT GetDateTimeFontForMode() const
	{
		switch (m_e)
		{
		case CCM_Enum::FullRange:
			return EXE_FONT::eTabListTitleFont;
		case CCM_Enum::SimpleDateTime:
		case CCM_Enum::SimpleDateTime2:
			return EXE_FONT::eMediumBig;
		case CCM_Enum::SimpleDateTimeSM:
		case CCM_Enum::SimpleDateTimeSM2:
			return EXE_FONT::eUI_Font;
		}
		XeASSERT(false);	// Unknown mode.
		return EXE_FONT::eUI_Font;
	}
};

typedef std::function<void()> CalendarCtrlNotifyCallbackFunc;

constexpr wchar_t XECALENDARWND_CLASSNAME[] = L"XeCalendarWndClass";  // Window class name

export class CXeCalendarCtrl : public CXePopupCtrlBase, public CXeD2DCtrlBase
{
#pragma region class_data
	enum class SEL { DateTime, Month, Year };
	enum class RCT {
			None, FromOnBox, FromBox, ToOnBox, ToBox, MonthDecBox, MonthIncBox, MonthBox, YearBox, 
			Years, Months, Days, Hours, Minutes, Seconds, MilliSecs,
			YearsDec, YearsInc, MonthsDec, MonthsInc, DaysDec, DaysInc,
			HoursDec, HoursInc, MinutesDec, MinutesInc, SecondsDec, SecondsInc, MilliSecsDec, MilliSecsInc, 
			NowBtn, OkBtn, CancelBtn, CalendarDay, CalendarMonth, CalendarYear,
			YearPageLeft, YearPageRight /* only when selecting year */
	};

	struct XeCalPartBuffer
	{
		wchar_t m_digits[5];
		int m_num_digits = 0;
		RCT m_part = RCT::None;

		XeCalPartBuffer(RCT part, WORD value) : m_part(part)
		{
			if (part == RCT::Years)
			{
				swprintf_s(m_digits, 5, L"%04u", value);
				m_num_digits = 4;
			}
			else if (part == RCT::MilliSecs)
			{
				swprintf_s(m_digits, 5, L"%03u", value);
				m_num_digits = 3;
			}
			else
			{
				swprintf_s(m_digits, 5, L"%02u", value);
				m_num_digits = 2;
			}
		}

		std::wstring as_string() const { return std::wstring(m_digits); }

		WORD as_value() const
		{
			return (WORD)xen::stoi(as_string());
		}
	};

	class XeCalPartEdit
	{
		std::vector<XeCalPartBuffer> m_buffers;
		RCT m_editing = RCT::None;
		int m_cur_digit = 0;
		bool m_isBlinkOn = false;

		XeCalPartBuffer* _GetBuffer(RCT part)
		{
			auto it = std::find_if(m_buffers.begin(), m_buffers.end(),
					[part](XeCalPartBuffer& buf) { return buf.m_part == part; });
			return it != m_buffers.end() ? &(*it) : nullptr;
		}

		RCT _GetNext(RCT rct)
		{
			switch (rct)
			{
			case RCT::Years:		return RCT::Months;	
			case RCT::Months:		return RCT::Days;	
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
			case RCT::Years:		return RCT::None;	
			case RCT::Months:		return RCT::Years;		
			case RCT::Days:			return RCT::Months;	
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
			XeCalPartBuffer* pBuf = _GetBuffer(m_editing);
			if (pBuf)
			{
				m_cur_digit = isMoveNext ? 0 : pBuf->m_num_digits - 1;
				m_isBlinkOn = false;
			}
		}

		WORD _GetValue(RCT part, const SYSTEMTIME& st)
		{
			switch (part)
			{
			case RCT::Years:		return st.wYear;			
			case RCT::Months:		return st.wMonth;			
			case RCT::Days:			return st.wDay;				
			case RCT::Hours:		return st.wHour;			
			case RCT::Minutes:		return st.wMinute;			
			case RCT::Seconds:		return st.wSecond;			
			case RCT::MilliSecs:	return st.wMilliseconds;	
			}
			return 0;
		}

		SYSTEMTIMEX _GetAsSystemtime() const
		{
			SYSTEMTIMEX st{};
			XeASSERT(m_buffers.size() == 7);
			if (m_buffers.size() == 7)
			{
				st.wYear			= m_buffers[0].as_value();
				st.wMonth			= m_buffers[1].as_value();
				st.wDay				= m_buffers[2].as_value();
				st.wHour			= m_buffers[3].as_value();
				st.wMinute			= m_buffers[4].as_value();
				st.wSecond			= m_buffers[5].as_value();
				st.wMilliseconds	= m_buffers[6].as_value();
			}
			return st;
		}

	public:
		void Set(const FILETIMEX& dt, RCT editing)
		{
			m_editing = editing;
			m_cur_digit = 0;
			m_isBlinkOn = false;
			if (editing == RCT::None)
			{
				return;
			}
			SYSTEMTIME st = dt.ToSystemTime();
			m_buffers.clear();
			m_buffers.push_back(XeCalPartBuffer(RCT::Years,		_GetValue(RCT::Years,		st)));
			m_buffers.push_back(XeCalPartBuffer(RCT::Months,	_GetValue(RCT::Months,		st)));
			m_buffers.push_back(XeCalPartBuffer(RCT::Days,		_GetValue(RCT::Days,		st)));
			m_buffers.push_back(XeCalPartBuffer(RCT::Hours,		_GetValue(RCT::Hours,		st)));
			m_buffers.push_back(XeCalPartBuffer(RCT::Minutes,	_GetValue(RCT::Minutes,		st)));
			m_buffers.push_back(XeCalPartBuffer(RCT::Seconds,	_GetValue(RCT::Seconds,		st)));
			m_buffers.push_back(XeCalPartBuffer(RCT::MilliSecs, _GetValue(RCT::MilliSecs,	st)));
		}

		RCT part() const { return m_editing; }

		void Blink() { m_isBlinkOn = !m_isBlinkOn; }

		std::wstring GetText(RCT rct, const SYSTEMTIME& st)
		{
			if (m_editing == RCT::None)
			{
				XeCalPartBuffer tmp(rct, _GetValue(rct, st));
				return tmp.as_string();
			}
			XeCalPartBuffer* pBuf = _GetBuffer(rct);
			if (pBuf)
			{
				std::wstring s(pBuf->m_digits);
				if (m_editing == rct && m_isBlinkOn && m_cur_digit >= 0 && m_cur_digit < s.size())
				{
					s[m_cur_digit] = L'#';
				}
				return s;
			}
			return std::wstring();
		}

		std::vector<RCT> GetErrors() const
		{
			std::vector<RCT> errors;
			SYSTEMTIMEX st = _GetAsSystemtime();
			if (!st.IsValidYear()		)	{ errors.push_back(RCT::Years); }
			if (!st.IsValidMonth()		)	{ errors.push_back(RCT::Months); }
			if (!st.IsValidDay()		)	{ errors.push_back(RCT::Days); }
			if (!st.IsValidHour()		)	{ errors.push_back(RCT::Hours); }
			if (!st.IsValidMinute()		)	{ errors.push_back(RCT::Minutes); }
			if (!st.IsValidSecond()		)	{ errors.push_back(RCT::Seconds); }
			if (!st.IsValidMilliSecond())	{ errors.push_back(RCT::MilliSecs); }
			return errors;
		}

		bool AnyErrors() const
		{
			SYSTEMTIMEX st = _GetAsSystemtime();
			return !st.IsValidYear() || !st.IsValidMonth() || !st.IsValidDay() || !st.IsValidHour()
					|| !st.IsValidMinute() || !st.IsValidSecond() || !st.IsValidMilliSecond();
		}

		FILETIMEX GetFiletime() const
		{
			XeASSERT(!AnyErrors());
			SYSTEMTIMEX st = _GetAsSystemtime();
			return FILETIMEX(st);
		}

		// Return true if date/time value has (maybe) changed.
		bool OnKeyDown(WPARAM ww)
		{
			XeCalPartBuffer* pBuf = _GetBuffer(m_editing);
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
			if (ww >= '0' && ww <= '9' && m_cur_digit < 5)
			{
				XeCalPartBuffer* pBuf = _GetBuffer(m_editing);
				if (pBuf)
				{
					pBuf->m_digits[m_cur_digit] = (wchar_t)ww;
					if (++m_cur_digit >= pBuf->m_num_digits)
					{
						_MoveEditPart(true);
					}
					return true;
				}
			}
			else if (ww == ' ' || ww == '-' || ww == '/' || ww == ',' || ww == '.')
			{
				_MoveEditPart(true);
			}
			return false;
		}
	};

	struct SEL_DATE
	{
		int col = -1, row = -1;
		FILETIMEX tp;

		SEL_DATE() = default;
		SEL_DATE(int c, int r, const FILETIMEX& t) : col(c), row(r), tp(t) {}

		bool IsValid() const { return col >= 0 && row >= 0; }
	};

	enum class ActiveDT { From, To };

protected:
	bool m_isPopupCtrl = false;	// true when 'this' is a popup control.

	CalCtrlMode m_mode = CalCtrlMode::FullRange;

	CSize m_sizeCtrl;

	SEL m_selecting = SEL::DateTime;
	XeCalPartEdit m_rctEditing;

	RCT m_rctHover = RCT::None;
	int m_hover_col, m_hover_row;

	RCT m_rctRepeat = RCT::None;

	int m_firstYear = 2025;

	std::vector<std::wstring> m_months{ L"January" ,L"February", L"March", L"April", L"May", L"June", L"July", L"August", L"September", L"October", L"November", L"December", };
	std::vector<std::wstring> m_days{ L"MON", L"TUE", L"WED" , L"THU" , L"FRI" , L"SAT" , L"SUN" };

	////////////////////////////////////////////////////////////////////////////////////////
	// Paint data
	float weekDayY = 0, weekDayX = 0;
	float labelCX = 0, labelCY = 0, monthColCX = 0;

	FILETIMEX m_fromDTbefore, m_toDTbefore;

public:
	bool m_isOkBtnClicked = false, m_isCancelBtnClicked = false;
	FILETIMEX m_fromDT, m_toDT;
	ActiveDT m_actDT = ActiveDT::From;

	// Parent window sets the callback (used when calendar control is just a normal control - i.e. not a popup).
	CalendarCtrlNotifyCallbackFunc m_nfCallback = nullptr;
#pragma endregion class_data

#pragma region Create
public:
	CXeCalendarCtrl(CXeUIcolorsIF* pUIcolors, CalCtrlMode mode)
			: CXeD2DCtrlBase(pUIcolors), m_mode(mode)
	{
		m_xeUI->RegisterWindowClass(XECALENDARWND_CLASSNAME, D2DCtrl_WndProc);

		m_xeUI->D2D_GetFont(EXE_FONT::eTabListTitleFont);	// Create big text UI font.
		m_xeUI->D2D_GetFont(EXE_FONT::eMediumBig);

		m_fromDT = FILETIMEX::LocalNow(), m_toDT = mode == CalCtrlMode::FullRange ? FILETIMEX::LocalNow() : FILETIMEX();
		m_actDT = ActiveDT::From;

		m_sizeCtrl = _CalculateUI();
	}
	//virtual ~CXeCalendarCtrl() {}

	bool Create(DWORD dwStyle, const CRect& rect, HWND hParentWnd,
		UINT nID, const wchar_t* tooltip)
	{
		DWORD dwExStyle = 0;
		HWND hWnd = CreateD2DCtrl(dwExStyle, XECALENDARWND_CLASSNAME, nullptr, dwStyle,
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

	void SetDateTimes(const FILETIMEX& fromDT, const FILETIMEX& toDT)
	{
		m_fromDT = fromDT, m_toDT = toDT;
		m_actDT = m_fromDT.IsValid() ? ActiveDT::From : m_toDT.IsValid() ? ActiveDT::To : ActiveDT::From;
		if (::IsWindow(Hwnd()))
		{
			_RedrawDirectly();
		}
	}

protected:
	const FILETIMEX& _GetActiveDTconst() const { return m_actDT == ActiveDT::From ? m_fromDT : m_toDT; }
	FILETIMEX& _GetActiveDT() { return m_actDT == ActiveDT::From ? m_fromDT : m_toDT; }
	const FILETIMEX& _GetInactiveDTconst() const { return m_actDT == ActiveDT::From ? m_toDT : m_fromDT; }
	FILETIMEX& _GetInactiveDT() { return m_actDT == ActiveDT::From ? m_toDT : m_fromDT; }

	FILETIMESPAN _GetDuration() const
	{
		FILETIMESPAN ts;
		if (m_fromDT.IsValid() && m_toDT.IsValid())
		{
			ts = m_toDT - m_fromDT;
		}
		return ts;
	}

	bool _IsActiveDTvalid() const { return _GetActiveDTconst().IsValid(); }

	WORD _Nudge(Nudge nudge, WORD w, WORD min, WORD max)
	{
		if (nudge == Nudge::Dec)
		{
			w = w == min ? max : w - 1;
		}
		else
		{
			w = w == max ? min : w + 1;
		}
		return w;
	}

	void _NudgeTime(Nudge nudge, RCT inrc)
	{
		FILETIMEX& dt = _GetActiveDT();
		if (!dt.IsValid())
		{
			return;
		}
		FILETIMEX ft = dt;
		SYSTEMTIMEX st;
		::FileTimeToSystemTime((FILETIME*)&ft, &st);
		switch (inrc)
		{
		case RCT::YearsDec:
		case RCT::YearsInc:
		case RCT::Years:
			st.wYear = _Nudge(nudge, st.wYear, 1601, 3000);
			if (!st.IsDayValidForMonth())
			{
				st.wDay = st.GetMaxDayForMonth();
			}
			break;
		case RCT::MonthsDec:
		case RCT::MonthsInc:
		case RCT::Months:
			st.wMonth = _Nudge(nudge, st.wMonth, 1, 12);
			if (!st.IsDayValidForMonth())
			{
				st.wDay = st.GetMaxDayForMonth();
			}
			break;
		case RCT::Days:
		case RCT::DaysDec:
		case RCT::DaysInc:
			st.wDay = _Nudge(nudge, st.wDay, 1, st.GetMaxDayForMonth());
			break;
		case RCT::HoursDec:
		case RCT::HoursInc:
		case RCT::Hours:
			st.wHour = _Nudge(nudge, st.wHour, 0, 23);
			break;
		case RCT::MinutesDec:
		case RCT::MinutesInc:
		case RCT::Minutes:
			st.wMinute = _Nudge(nudge, st.wMinute, 0, 59);
			break;
		case RCT::SecondsDec:
		case RCT::SecondsInc:
		case RCT::Seconds:
			st.wSecond = _Nudge(nudge, st.wSecond, 0, 59);
			break;
		case RCT::MilliSecsDec:
		case RCT::MilliSecsInc:
		case RCT::MilliSecs: {
			WORD& w = st.wMilliseconds;
			WORD step = 100, max = 999;
			if (nudge == Nudge::Dec)
			{
				w = w < step ? max : w - step;
			}
			else
			{
				w = w > (max - step) ? 0 : w + step;
			}
			}
			break;
		case RCT::MonthDecBox:
		case RCT::MonthIncBox:
			st.wMonth = _Nudge(nudge, st.wMonth, 1, 12);
			if (nudge == Nudge::Dec && st.wMonth == 12)
			{
				--st.wYear;
			}
			if (nudge == Nudge::Inc && st.wMonth == 1)
			{
				++st.wYear;
			}
			if (!st.IsDayValidForMonth())
			{
				st.wDay = st.GetMaxDayForMonth();
			}
			break;
		}
		FILETIMEX ft_new(st);
		dt = ft_new;
		_RedrawDirectly();
	}

	RCT _PtInRect(const float& x, const float& y)
	{
		int key = m_rr.PtInRect(x, y);
		if (key < 0)
		{
			return RCT::None;
		}
		RCT rct = (RCT)key;
		if (m_selecting != SEL::Year && (rct == RCT::YearPageLeft || rct == RCT::YearPageRight))
		{
			rct = RCT::None;
		}
		return rct;
	}

	bool IsAnyMonthYearBox(RCT rct) const
	{
		return rct == RCT::MonthBox || rct == RCT::YearBox || rct == RCT::MonthDecBox || rct == RCT::MonthIncBox;
	}

	bool IsAnyTimePartBox(RCT rct) const
	{
		return rct == RCT::Years || rct == RCT::Months || rct == RCT::Days
				|| rct == RCT::Hours || rct == RCT::Minutes || rct == RCT::Seconds || rct == RCT::MilliSecs;
	}

	bool IsAnyTimePartDecBox(RCT rct) const
	{
		return rct == RCT::YearsDec || rct == RCT::MonthsDec || rct == RCT::DaysDec || rct == RCT::HoursDec
				|| rct == RCT::MinutesDec || rct == RCT::SecondsDec || rct == RCT::MilliSecsDec;
	}

	bool IsAnyTimePartIncBox(RCT rct) const
	{
		return rct == RCT::YearsInc || rct == RCT::MonthsInc || rct == RCT::DaysInc || rct == RCT::HoursInc
				|| rct == RCT::MinutesInc || rct == RCT::SecondsInc || rct == RCT::MilliSecsInc;
	}

	int _GetCalendarMonthColFromX(const float& x)
	{
		float x1_col = weekDayX;
		for (int c = 0; c < 2; ++c)
		{
			if (x >= x1_col && x < (x1_col + monthColCX))
			{
				return c;
			}
			x1_col += monthColCX;
		}
		return -1;
	}

	FILETIMEX _GetCalendarMonthDateFromPoint(const float& x, const float& y)
	{
		int col = _GetCalendarMonthColFromX(x), row = _GetCalendarRowFromY(y);
		if (col >= 0 && row >= 0)
		{
			FILETIMEX ft = _GetActiveDTconst();
			SYSTEMTIME st = ft.ToSystemTime();
			st.wMonth = (WORD)((row * 2) + col + 1);
			if (st.wMonth == 2 && st.wDay > 28)
			{
				st.wDay = 28;
			}
			else if ((st.wMonth == 4 || st.wMonth == 6 || st.wMonth == 9 || st.wMonth == 11) && st.wDay > 30)
			{
				st.wDay = 30;
			}
			FILETIMEX ft_new(st);
			return ft_new;
		}
		return FILETIMEX();
	}

	int _GetCalendarYearColFromX(const float& x)
	{
		return _GetCalendarDayColumnFromX(x);
	}

	FILETIMEX _GetCalendarYearDateFromPoint(const float& x, const float& y)
	{
		int col = _GetCalendarYearColFromX(x), row = _GetCalendarRowFromY(y);
		if (col >= 0 && row >= 0)
		{
			SYSTEMTIME st = _GetActiveDTconst().ToSystemTime();
			st.wYear = (WORD)m_firstYear + (row * 7) + col;
			FILETIMEX ft_new(st);
			return ft_new;
		}
		return FILETIMEX();
	}

	int _GetCalendarDayColumnFromX(const float& x)
	{
		float x1_col = weekDayX;
		for (int c = 0; c < 7; ++c)
		{
			if (x >= x1_col && x < (x1_col + labelCX))
			{
				return c;
			}
			x1_col += labelCX;
		}
		return -1;
	}

	int _GetCalendarRowFromY(const float& y)
	{
		float y1_row = weekDayY + labelCY;
		for (int r = 0; r < 6; ++r)
		{
			if (y >= y1_row && y < (y1_row + labelCY))
			{
				return r;
			}
			y1_row += labelCY;
		}
		return -1;
	}

	SEL_DATE _GetCalendarDayDateFromPoint(const float& x, const float& y)
	{
		int col = _GetCalendarDayColumnFromX(x), row = _GetCalendarRowFromY(y);
		if (col >= 0 && row >= 0)
		{
			FILETIMEX tp = _GetFirstMondayOfCalendar(_GetActiveDTconst());
			tp += FILETIMESPAN::FromDays( (row * 7) + col );
			return SEL_DATE(col, row, tp);
		}
		return SEL_DATE();
	}

	FILETIMEX _GetFirstMondayOfCalendar(const FILETIMEX& ft)
	{
		SYSTEMTIME st = ft.ToSystemTime();
		st.wDay = 1;
		st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;
		FILETIMEX ft1(st);
		SYSTEMTIME st1 = ft1.ToSystemTime();
		int subtract_days = st1.wDayOfWeek == 0 ? 6 : st1.wDayOfWeek - 1; // 0 = sunday, 6 = saturday
		ft1 -= FILETIMESPAN::FromDays(subtract_days);
		return ft1;
	}

	void _NotifyParent()
	{
		HWND hWndParent = ::GetParent(Hwnd());
		if (hWndParent)
		{
			NMDATETIMECHANGE nf{ 0 };
			nf.nmhdr.hwndFrom = Hwnd();
			nf.nmhdr.code = DTN_DATETIMECHANGE;
			nf.nmhdr.idFrom = GetDlgCtrlID();
			nf.dwFlags = m_fromDT.IsValid() ? GDT_VALID : GDT_NONE;
			nf.st = m_fromDT.ToSystemTime();
			::SendMessageW(hWndParent, WM_NOTIFY, nf.nmhdr.code, reinterpret_cast<LPARAM>(&nf));
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
		EXE_FONT mode_font = m_mode.GetDateTimeFontForMode();

		m_xeUI->D2D_GetTextSize(std::wstring(L"W"), EXE_FONT::eUI_Font, szF);
		float mul = m_mode.IsFullMode() ? 1.5f : m_mode.IsMinimalMode() ? 0.5f : 1.0f;
		float marginL = szF.width * mul, spaceV = marginL * 0.5f, wndCX = 0;
		float marginT = marginL;
		float x = marginL, y = marginT;

		m_xeUI->D2D_GetTextSize(std::wstring(L"WED"), EXE_FONT::eUI_Font, szF);
		labelCX = szF.width * 1.8f;
		labelCY = szF.height * 1.5f;

		monthColCX = (7 * labelCX) / 2;

		m_xeUI->D2D_GetTextSize(std::wstring(L"Cancel"), EXE_FONT::eUI_Font, szF);
		float okBtnCX = szF.width * (m_mode.IsFullMode() ? 1.8f : 1.4f);
		float okBtnCY = szF.height * (m_mode.IsFullMode() ? 1.5f : 1.2f);

		if (m_mode.IsFullMode())
		{
			// From / To boxes
			m_xeUI->D2D_GetTextSize(std::wstring(L"0000-00-00 00:00:00,000"), EXE_FONT::eUI_Font, szF);
			float dateBoxCX = szF.width * 1.1f;
			float dateBoxCY = szF.height * 1.3f;
			m_rr.Set((int)RCT::FromOnBox, x, y, dateBoxCY, dateBoxCY);
			x += dateBoxCY;
			m_rr.Set((int)RCT::FromBox, x, y, dateBoxCX, dateBoxCY);
			x += dateBoxCX + marginL;
			m_rr.Set((int)RCT::ToOnBox, x, y, dateBoxCY, dateBoxCY);
			x += dateBoxCY;
			m_rr.Set((int)RCT::ToBox, x, y, dateBoxCX, dateBoxCY);
			y += dateBoxCY + spaceV;
			
			// Window width
			wndCX = (3 * marginL) + (2 * dateBoxCY) + (2 * dateBoxCX);

			// Month Year line
			m_xeUI->D2D_GetTextSize(std::wstring(L"0"), mode_font, szF);
			float monthDecCXY = szF.height;
			D2D1_ROUNDED_RECT monthDecBox = m_rr.Set((int)RCT::MonthDecBox, marginL, y, monthDecCXY, monthDecCXY);
			D2D1_ROUNDED_RECT monthIncBox = m_rr.Set((int)RCT::MonthIncBox, wndCX - marginL - monthDecCXY, y, monthDecCXY, monthDecCXY);
			x = monthDecBox.rect.right;
			float cx = (monthIncBox.rect.left - monthDecBox.rect.right - marginL) / 2.0f;
			D2D1_ROUNDED_RECT monthBox = m_rr.Set((int)RCT::MonthBox, x, y, cx, monthDecCXY);
			x = monthBox.rect.right + marginL;
			m_rr.Set((int)RCT::YearBox, x, y, cx, monthDecCXY);
			y += labelCY + spaceV;

			// Calendar weekdays line
			weekDayY = y;
			weekDayX = (wndCX - (7 * labelCX)) / 2;

			// Year page boxes
			x = weekDayX;
			m_rr.Set((int)RCT::YearPageLeft, x, y, labelCX, labelCY);
			x += 6 * labelCX;
			m_rr.Set((int)RCT::YearPageRight, x, y, labelCX, labelCY);

			y += (7 * labelCY) + spaceV;
		}

		// Calculate timeBox time boxes
		m_xeUI->D2D_GetTextSize(std::wstring(L"0000"), mode_font, szF);
		float yyyyCX = szF.width * 1.2f;
		m_xeUI->D2D_GetTextSize(std::wstring(L"00"), mode_font, szF);
		float hhCX = szF.width * 1.2f;
		float hhCY = szF.height;
		m_xeUI->D2D_GetTextSize(std::wstring(L"000"), mode_font, szF);
		float msCX = szF.width * 1.2f;
		float cx_dot = hhCY * 0.3f;
		float cy_inc = hhCY * 0.3f, cx_inc = hhCX * 0.8f;
		float cy_spc = hhCY * 0.05f;
		x = m_mode.IsMinimalMode() ? marginL * 0.5f : marginL;
		float x_ofs = (yyyyCX - cx_inc) / 2;
		m_rr.Set((int)RCT::Years, x, y, yyyyCX, hhCY);
		m_rr.Set((int)RCT::YearsInc, x + x_ofs, y - cy_inc - cy_spc, cx_inc, cy_inc);
		m_rr.Set((int)RCT::YearsDec, x + x_ofs, y + hhCY + cy_spc, cx_inc, cy_inc);
		x += yyyyCX + cx_dot;
		x_ofs = (hhCX - cx_inc) / 2;
		m_rr.Set((int)RCT::Months, x, y, hhCX, hhCY);
		m_rr.Set((int)RCT::MonthsInc, x + x_ofs, y - cy_inc - cy_spc, cx_inc, cy_inc);
		m_rr.Set((int)RCT::MonthsDec, x + x_ofs, y + hhCY + cy_spc, cx_inc, cy_inc);
		x += hhCX + cx_dot;
		m_rr.Set((int)RCT::Days, x, y, hhCX, hhCY);
		m_rr.Set((int)RCT::DaysInc, x + x_ofs, y - cy_inc - cy_spc, cx_inc, cy_inc);
		m_rr.Set((int)RCT::DaysDec, x + x_ofs, y + hhCY + cy_spc, cx_inc, cy_inc);
		x += hhCX + cx_dot;
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
		if (!m_mode.IsFullMode())
		{
			wndCX = x + msCX + (marginL * 0.5f);	// Window width when small single date/time selection
		}

		if (m_mode.IsFullMode())
		{
			// Now button
			m_xeUI->D2D_GetTextSize(std::wstring(L"Now"), EXE_FONT::eUI_Font, szF);
			float nowBtnCX = szF.width * 1.8f;
			x = wndCX - nowBtnCX - marginL;
			y = msIncBox.rect.top + (((msDecBox.rect.bottom - msIncBox.rect.top) - okBtnCY) / 2);
			m_rr.Set((int)RCT::NowBtn, x, y, nowBtnCX, okBtnCY);
		}

		// Ok and Cancel buttons
		XeASSERT(wndCX > 0);
		if (m_mode.HasOkAndCancelButtons())
		{
			float cxOkCancelButtons = (2 * okBtnCX) + marginL;
			x = m_mode.IsFullMode() ? wndCX - cxOkCancelButtons - marginL
					: (wndCX - cxOkCancelButtons) / 2;	// SimpleDateTime
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
		pRT->Clear(m_xeUI->GetColorF(CID::DialogBg));
		D2D1_ROUNDED_RECT rcR(rc, 1, 1);
		pRT->DrawRoundedRectangle(rcR, GetBrush(CID::CtrlBorder));

		const FILETIMEX& actDT = _GetActiveDTconst();
		const FILETIMEX& inactDT = _GetInactiveDTconst();

		if (m_mode.IsFullMode())
		{
			_DrawDateBoxes(pRT);
			_DrawYearMonth(pRT, actDT);

			if (m_selecting == SEL::DateTime)
			{
				_DrawWeekDays(pRT);
				if (actDT.IsValid())
				{
					_DrawCalendar(pRT, actDT, inactDT);
				}
			}
			else if (m_selecting == SEL::Month)
			{
				_DrawMonths(pRT);
			}
			else if (m_selecting == SEL::Year)
			{
				_DrawYears(pRT);
			}

			_DrawButton(pRT, (int)RCT::NowBtn, std::wstring(L"Now"), m_rctHover == RCT::NowBtn);
		}

		_DrawTimeBox(pRT, actDT);

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

	void _DrawWeekDays(ID2D1RenderTarget* pRT)
	{
		float x = weekDayX;
		for (const std::wstring& day : m_days)
		{
			_DrawText2(pRT, day, SetRR(x, weekDayY, labelCX, labelCY));
			x += labelCX;
		}
	}

	void _DrawCalendar(ID2D1RenderTarget* pRT, const FILETIMEX& dt, const FILETIMEX& dt_inact)
	{
		FILETIMEX tp = _GetFirstMondayOfCalendar(dt);
		SYSTEMTIME st = dt.ToSystemTime();
		WORD sel_month = st.wMonth;
		WORD sel_day = st.wDay;
		SYSTEMTIME st_inact{};
		bool isInactSameDate = false;
		if (dt_inact.IsValid())
		{
			st_inact = dt_inact.ToSystemTime();
			isInactSameDate = st.wDay == st_inact.wDay && st.wMonth == st_inact.wMonth && st.wYear == st_inact.wYear;
		}
		float y = weekDayY + labelCY;
		for (int row = 0; row < 6; ++row)
		{
			float x = weekDayX;
			for (int col = 0; col < 7; ++col)
			{
				st = tp.ToSystemTime();
				bool isSelMonth = sel_month == st.wMonth;
				std::wstring day = std::to_wstring(st.wDay);
				_DrawText2(pRT, day, SetRR(x, y, labelCX, labelCY), isSelMonth ? CID::CtrlTxt : CID::CtrlBtnBorder);
				if ((sel_month == st.wMonth && sel_day == st.wDay)
						|| (m_rctHover == RCT::CalendarDay && col == m_hover_col && row == m_hover_row))
				{
					_DrawRoundedRectangle(pRT, SetRR(x, y, labelCX, labelCY), CID::CtrlBtnDefBorder);
				}
				if (!isInactSameDate
						&& st_inact.wYear == st.wYear && st_inact.wMonth == st.wMonth && st_inact.wDay == st.wDay)
				{
					_DrawRoundedRectangle(pRT, SetRR(x, y, labelCX, labelCY), CID::CtrlBtnBorder);
				}
				x += labelCX;
				tp += FILETIMESPAN::FromDays(1);
			}
			y += labelCY;
		}
	}
	
	void _DrawMonths(ID2D1RenderTarget* pRT)
	{
		size_t month_idx = 0;
		float y = weekDayY + labelCY;
		for (int row = 0; row < 6; ++row)
		{
			float x = weekDayX;
			for (int col = 0; col < 2; ++col)
			{
				_DrawText2(pRT, m_months[month_idx], SetRR(x, y, monthColCX, labelCY));
				if (m_rctHover == RCT::CalendarMonth && col == m_hover_col && row == m_hover_row)
				{
					_DrawRoundedRectangle(pRT, SetRR(x, y, monthColCX, labelCY), CID::CtrlBtnDefBorder);
				}
				x += monthColCX;
				++month_idx;
			}
			y += labelCY;
		}
	}

	void _DrawYears(ID2D1RenderTarget* pRT)
	{
		_DrawText(pRT, std::wstring(L"<"), (int)RCT::YearPageLeft, CID::CtrlTxt, EXE_FONT::eTabListTitleFont);
		if (m_rctHover == RCT::YearPageLeft)
		{
			_DrawRoundedRectangle(pRT, (int)RCT::YearPageLeft, CID::CtrlBtnDefBorder);
		}
		_DrawText(pRT, std::wstring(L">"), (int)RCT::YearPageRight, CID::CtrlTxt, EXE_FONT::eTabListTitleFont);
		if (m_rctHover == RCT::YearPageRight)
		{
			_DrawRoundedRectangle(pRT, (int)RCT::YearPageRight, CID::CtrlBtnDefBorder);
		}
		SYSTEMTIME st = _GetActiveDTconst().ToSystemTime();
		int year = m_firstYear, selDT_year = st.wYear;
		int hoverYear = m_rctHover == RCT::CalendarYear ? (m_firstYear + m_hover_row * 7 + m_hover_col) : -1;
		float y = weekDayY + labelCY;
		for (int row = 0; row < 6; ++row)
		{
			float x = weekDayX;
			for (int col = 0; col < 7; ++col)
			{
				std::wstring s = std::to_wstring(year);
				_DrawText2(pRT, s, SetRR(x, y, labelCX, labelCY));
				if (year == selDT_year || year == hoverYear)
				{
					_DrawRoundedRectangle(pRT, SetRR(x, y, labelCX, labelCY), CID::CtrlBtnDefBorder);
				}
				x += labelCX;
				++year;
			}
			y += labelCY;
		}
	}

	void _DrawDateBoxes(ID2D1RenderTarget* pRT)
	{
		FILETIMESPAN dur = _GetDuration();
		bool isNegativeDur = dur.IsNegative();
		bool isFromActive = m_actDT == ActiveDT::From;
		bool isToActive = m_actDT == ActiveDT::To;

		_FillRoundedRectangle(pRT, (int)RCT::FromOnBox, CID::CtrlBg);
		_FillRoundedRectangle(pRT, (int)RCT::ToOnBox, CID::CtrlBg);
		_FillRoundedRectangle(pRT, (int)RCT::FromBox, CID::CtrlBg);
		_FillRoundedRectangle(pRT, (int)RCT::ToBox, CID::CtrlBg);
		_DrawRoundedRectangle(pRT, (int)RCT::FromOnBox,
				isFromActive || m_rctHover == RCT::FromOnBox ? CID::CtrlBtnDefBorder : CID::CtrlBtnBorder, 2.0f);
		_DrawRoundedRectangle(pRT, (int)RCT::FromBox,
				isFromActive || m_rctHover == RCT::FromBox ? CID::CtrlBtnDefBorder : CID::CtrlBtnBorder, 2.0f);
		_DrawRoundedRectangle(pRT, (int)RCT::ToOnBox,
				isToActive || m_rctHover == RCT::ToOnBox ? CID::CtrlBtnDefBorder : CID::CtrlBtnBorder, 2.0f);
		_DrawRoundedRectangle(pRT, (int)RCT::ToBox,
				isToActive || m_rctHover == RCT::ToBox ? CID::CtrlBtnDefBorder : CID::CtrlBtnBorder, 2.0f);

		if (m_fromDT.IsValid())
		{
			_DrawCheckmark(pRT, (int)RCT::FromOnBox);
			std::wstring s = m_fromDT.ToStrW(DATEFMT::ISOplus_mS);
			CID colorId = isToActive && isNegativeDur ? CID::MrgLogNoDataBg : CID::CtrlTxt;
			_DrawText(pRT, s, (int)RCT::FromBox, colorId);
		}
		if (m_toDT.IsValid())
		{
			_DrawCheckmark(pRT, (int)RCT::ToOnBox);
			std::wstring s = m_toDT.ToStrW(DATEFMT::ISOplus_mS);
			CID colorId = isFromActive && isNegativeDur ? CID::MrgLogNoDataBg : CID::CtrlTxt;
			_DrawText(pRT, s, (int)RCT::ToBox, colorId);
		}
	}

	void _DrawYearMonth(ID2D1RenderTarget* pRT, const FILETIMEX& dt)
	{
		if (dt.IsValid())
		{
			SYSTEMTIME st = dt.ToSystemTime();
			std::wstring s = st.wMonth >= 1 && st.wMonth <= 12 ? m_months[st.wMonth - 1] : std::wstring();
			_DrawText(pRT, s, (int)RCT::MonthBox, CID::CtrlTxt, EXE_FONT::eTabListTitleFont, DWRITE_TEXT_ALIGNMENT_TRAILING);
			s = std::to_wstring(st.wYear);
			_DrawText(pRT, s, (int)RCT::YearBox, CID::CtrlTxt, EXE_FONT::eTabListTitleFont, DWRITE_TEXT_ALIGNMENT_LEADING);
			_DrawText(pRT, std::wstring(L"<"), (int)RCT::MonthDecBox, CID::CtrlTxt, EXE_FONT::eTabListTitleFont);
			_DrawText(pRT, std::wstring(L">"), (int)RCT::MonthIncBox, CID::CtrlTxt, EXE_FONT::eTabListTitleFont);

			if (IsAnyMonthYearBox(m_rctHover))
			{
				_DrawRoundedRectangle(pRT, m_rr.Get((int)m_rctHover), CID::CtrlBtnDefBorder);
			}
		}
	}

	void _DrawTimeBox(ID2D1RenderTarget* pRT, const FILETIMEX& dt)
	{
		if (dt.IsValid())
		{
			SYSTEMTIME st = dt.ToSystemTime();
			EXE_FONT eFont = m_mode.GetDateTimeFontForMode();

			if (m_rctEditing.part() != RCT::None)
			{
				_FillRoundedRectangle(pRT, (int)m_rctEditing.part(), CID::CtrlBg);
			}
			_DrawText(pRT, m_rctEditing.GetText(RCT::Years, st), (int)RCT::Years, CID::CtrlTxt, eFont);

			_DrawTextBetweenBoxes(pRT, std::wstring(L"-"), (int)RCT::Years, (int)RCT::Months);

			_DrawText(pRT, m_rctEditing.GetText(RCT::Months, st), (int)RCT::Months, CID::CtrlTxt, eFont);

			_DrawTextBetweenBoxes(pRT, std::wstring(L"-"), (int)RCT::Months, (int)RCT::Days);

			_DrawText(pRT, m_rctEditing.GetText(RCT::Days, st), (int)RCT::Days, CID::CtrlTxt, eFont);

			_DrawText(pRT, m_rctEditing.GetText(RCT::Hours, st), (int)RCT::Hours, CID::CtrlTxt, eFont);

			_DrawTextBetweenBoxes(pRT, std::wstring(L":"), (int)RCT::Hours, (int)RCT::Minutes);

			_DrawText(pRT, m_rctEditing.GetText(RCT::Minutes, st), (int)RCT::Minutes, CID::CtrlTxt, eFont);

			_DrawTextBetweenBoxes(pRT, std::wstring(L":"), (int)RCT::Minutes, (int)RCT::Seconds);

			_DrawText(pRT, m_rctEditing.GetText(RCT::Seconds, st), (int)RCT::Seconds, CID::CtrlTxt, eFont);

			_DrawTextBetweenBoxes(pRT, std::wstring(L","), (int)RCT::Seconds, (int)RCT::MilliSecs);

			_DrawText(pRT, m_rctEditing.GetText(RCT::MilliSecs, st), (int)RCT::MilliSecs, CID::CtrlTxt, eFont);

			if (IsAnyTimePartBox(m_rctHover))
			{
				_DrawRoundedRectangle(pRT, (int)m_rctHover, CID::CtrlBtnDefBorder);
			}

			if (m_rctEditing.part() != RCT::None)
			{
				_DrawRoundedRectangle(pRT, (int)m_rctEditing.part(), CID::CtrlBtnDefBorder);

				std::vector<RCT> errors = m_rctEditing.GetErrors();
				for (RCT err_rct : errors)
				{
					_DrawRoundedRectangle(pRT, (int)err_rct, CID::MrgLogNoDataBg);
				}
			}

			bool isHoverDecBox = IsAnyTimePartDecBox(m_rctHover);
			if (isHoverDecBox || IsAnyTimePartIncBox(m_rctHover))
			{
				_DrawTriangle(pRT, (int)m_rctHover, isHoverDecBox, CID::CtrlBtnDefBorder);
			}

			FILETIMESPAN ts = _GetDuration();
			if (!ts.IsZero())
			{
				const D2D1_ROUNDED_RECT& okBtn = m_rr.Get((int)RCT::OkBtn);
				float cy = okBtn.rect.bottom - okBtn.rect.top;
				float x = m_rr.Get((int)RCT::Years).rect.left, y = okBtn.rect.top;
				float cx = okBtn.rect.right - x;
				D2D1_ROUNDED_RECT durBox = SetRR(x, y, cx, cy);
				std::wstring d = L"Duration: " + ts.ToStrW(SPANFMT::HMSmS);
				_DrawText2(pRT, d, durBox, ts.IsNegative() ? CID::MrgLogNoDataBg : CID::CtrlTxt, EXE_FONT::eUI_Font,
						DWRITE_TEXT_ALIGNMENT_LEADING);
			}
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
		if (m_rctEditing.part() != RCT::None)
		{
			if (m_rctEditing.OnKeyDown(ww) && !m_rctEditing.AnyErrors())
			{
				FILETIMEX& activeDT = _GetActiveDT();
				activeDT = m_rctEditing.GetFiletime();
				_NotifyParent();
			}
			_RedrawDirectly();
		}
		return 0;
	}

	virtual LRESULT _OnChar(WPARAM ww, LPARAM ll)
	{
		if (m_rctEditing.part() != RCT::None)
		{
			if (m_rctEditing.OnChar(ww) && !m_rctEditing.AnyErrors())
			{
				FILETIMEX& activeDT = _GetActiveDT();
				activeDT = m_rctEditing.GetFiletime();
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
		if (inrc == RCT::None && m_mode.IsFullMode())
		{
			RCT activity = m_selecting == SEL::Year ? RCT::CalendarYear : m_selecting == SEL::Month
					? RCT::CalendarMonth : RCT::CalendarDay;
			int col = m_selecting == SEL::DateTime ? _GetCalendarDayColumnFromX((float)point.x) : m_selecting == SEL::Month
					? _GetCalendarMonthColFromX((float)point.x) : _GetCalendarYearColFromX((float)point.x);
			int row = _GetCalendarRowFromY((float)point.y);
			if (col >= 0 && row >= 0)
			{
				bool isColRowChanged = m_hover_col != col || m_hover_row != row;
				inrc = activity;
				m_hover_col = col;
				m_hover_row = row;
				if (m_rctHover == activity)
				{
					_RedrawDirectly();
				}
			}
		}
		if (inrc != m_rctHover)
		{
			m_rctHover = inrc;
			_RedrawDirectly();
		}
		return CXeD2DCtrlBase::_OnMouseMove(nFlags, point);
	}

	// WM_MOUSELEAVE message handler.
	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		if (!m_isPopupCtrl)
		{
			m_rctHover = RCT::None;
		}
		return CXeD2DCtrlBase::_OnMouseLeave(0, 0);
	}

	// WM_MOUSEWHEEL
	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint point) override
	{
		::ScreenToClient(Hwnd(), &point);
		Nudge nudge = zDelta > 0 ? Nudge::Inc : Nudge::Dec;

		if (m_rctEditing.part() == RCT::None)
		{
			RCT inrc = _PtInRect((float)point.x, (float)point.y);
			if (IsAnyTimePartBox(inrc))
			{
				_NudgeTime(nudge, inrc);
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

		FILETIMEX& activeDT = _GetActiveDT();
		SYSTEMTIME st = activeDT.ToSystemTime();
		RCT inrc = _PtInRect((float)point.x, (float)point.y);

		// If user is editing time parts and clicks anywhere in the control (except the time parts boxes)?
		if (m_rctEditing.part() != RCT::None && !IsAnyTimePartBox(inrc))
		{
			m_rctEditing.Set(FILETIMEX(), RCT::None);	// Cancel edit time parts.
			_RedrawDirectly();
		}

		bool isTimeDec = IsAnyTimePartDecBox(inrc);
		bool isTimeInc = IsAnyTimePartIncBox(inrc);
		if (isTimeDec || isTimeInc)
		{
			_NudgeTime(isTimeDec ? Nudge::Dec : Nudge::Inc, inrc);
			m_selecting = SEL::DateTime;
			::SetTimer(Hwnd(), LBTN_DOWN_TIMERID, LBTN_DOWN_TIME, 0);
			m_rctRepeat = inrc;
			_NotifyParent();
			return 0;
		}

		bool isRedrawAndNotifyNeeded = false;
		switch (inrc)
		{
		case RCT::MonthBox:
			m_selecting = m_selecting == SEL::Month ? SEL::DateTime : SEL::Month;
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::YearBox:
			m_selecting = m_selecting == SEL::Year ? SEL::DateTime : SEL::Year;
			m_firstYear = st.wYear - ((2 * 7) + 3);
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::FromOnBox:
			if (m_fromDT.IsValid())
			{
				m_fromDTbefore = m_fromDT;
				m_fromDT.SetZero();
			}
			else
			{
				if (m_fromDTbefore.IsValid())
				{
					m_fromDT = m_fromDTbefore;
				}
				else if (m_toDT.IsValid())
				{
					m_fromDT = m_toDT - FILETIMESPAN::FromHours(1);
				}
				else
				{
					m_fromDT.SetLocalNow();
				}
			}
			m_actDT = ActiveDT::From;
			m_selecting = SEL::DateTime;
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::ToOnBox:
			if (m_toDT.IsValid())
			{
				m_toDTbefore = m_toDT;
				m_toDT.SetZero();
			}
			else
			{
				if (m_toDTbefore.IsValid())
				{
					m_toDT = m_toDTbefore;
				}
				else if (m_fromDT.IsValid())
				{
					m_toDT = m_fromDT + FILETIMESPAN::FromHours(1);
				}
				else
				{
					m_toDT.SetLocalNow();
				}
			}
			m_actDT = ActiveDT::To;
			m_selecting = SEL::DateTime;
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::FromBox:
			m_actDT = ActiveDT::From;
			m_selecting = SEL::DateTime;
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::ToBox:
			m_actDT = ActiveDT::To;
			m_selecting = SEL::DateTime;
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::MonthDecBox:
			if (_IsActiveDTvalid())
			{
				_NudgeTime(Nudge::Dec, inrc);
				m_selecting = SEL::DateTime;
				isRedrawAndNotifyNeeded = true;
			}
			break;
		case RCT::MonthIncBox:
			if (_IsActiveDTvalid())
			{
				_NudgeTime(Nudge::Inc, inrc);
				m_selecting = SEL::DateTime;
				isRedrawAndNotifyNeeded = true;
			}
			break;
		case RCT::NowBtn:
			activeDT.SetLocalNow();
			m_selecting = SEL::DateTime;
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::CancelBtn:
			m_isCancelBtnClicked = true;
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::OkBtn:
			m_isOkBtnClicked = true;
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::YearPageLeft:
			m_firstYear -= 6 * 7;
			if (m_firstYear < 1601)
			{
				m_firstYear = 1601;
			}
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::YearPageRight:
			m_firstYear += 6 * 7;
			isRedrawAndNotifyNeeded = true;
			break;
		case RCT::Years:
		case RCT::Months:
		case RCT::Days:
		case RCT::Hours:
		case RCT::Minutes:
		case RCT::Seconds:
		case RCT::MilliSecs:
			m_selecting = SEL::DateTime;
			m_rctEditing.Set(activeDT, m_rctEditing.part() == inrc ? RCT::None : inrc);
			if (m_rctEditing.part() != RCT::None)
			{
				::SetTimer(Hwnd(), BLINK_TIMERID, BLINK_TIME, 0);
			}
			isRedrawAndNotifyNeeded = true;
			break;
		default:
			break;
		}
		if (isRedrawAndNotifyNeeded)
		{
			_RedrawDirectly();
			_NotifyParent();
			return 0;
		}

		isRedrawAndNotifyNeeded = false;
		if (m_selecting == SEL::DateTime && m_mode.IsFullMode())
		{
			SEL_DATE sel_date = _GetCalendarDayDateFromPoint((float)point.x, (float)point.y);
			if (sel_date.IsValid())
			{
				SYSTEMTIME st_new = sel_date.tp.ToSystemTime();
				st_new.wHour = st.wHour;
				st_new.wMinute = st.wMinute;
				st_new.wSecond = st.wSecond;
				st_new.wMilliseconds = st.wMilliseconds;
				activeDT.FromSystemTime(st_new);
				isRedrawAndNotifyNeeded = true;
			}
		}
		else if (m_selecting == SEL::Month)
		{
			FILETIMEX selDT = _GetCalendarMonthDateFromPoint((float)point.x, (float)point.y);
			if (selDT.IsValid())
			{
				activeDT = selDT;
				m_selecting = SEL::DateTime;
				isRedrawAndNotifyNeeded = true;
			}
		}
		else if (m_selecting == SEL::Year)
		{
			FILETIMEX selDT = _GetCalendarYearDateFromPoint((float)point.x, (float)point.y);
			if (selDT.IsValid())
			{
				activeDT = selDT;
				m_selecting = SEL::DateTime;
				isRedrawAndNotifyNeeded = true;
			}
		}
		if (isRedrawAndNotifyNeeded)
		{
			_RedrawDirectly();
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
		FILETIMEX& activeDT = _GetActiveDT();
		bool isActiveDTvalid = activeDT.IsValid();
		RCT inrc = _PtInRect((float)point.x, (float)point.y);

		bool isTimeDec = IsAnyTimePartDecBox(inrc);
		if (isTimeDec || IsAnyTimePartIncBox(inrc))
		{
			_NudgeTime(isTimeDec ? Nudge::Dec : Nudge::Inc, inrc);
			m_selecting = SEL::DateTime;
			_RedrawDirectly();
			_NotifyParent();
			return 0;
		}

		switch (inrc)
		{
		case RCT::MonthDecBox:
			if (isActiveDTvalid)
			{
				_NudgeTime(Nudge::Dec, RCT::MonthsDec);
				_RedrawDirectly();
				_NotifyParent();
			}
			break;
		case RCT::MonthIncBox:
			if (isActiveDTvalid)
			{
				_NudgeTime(Nudge::Inc,RCT::MonthsInc);
				_RedrawDirectly();
				_NotifyParent();
			}
			break;
		default:
			break;
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
				_NudgeTime(isTimeDec ? Nudge::Dec : Nudge::Inc, m_rctRepeat);
				_NotifyParent();
			}
			return 0;
		}
		if (nIDEvent == BLINK_TIMERID)
		{
			if (m_rctEditing.part() != RCT::None)
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
		m_rctEditing.Set(FILETIMEX(), RCT::None);	// Cancel edit time parts.
		_RedrawDirectly();
		return 0;
	}
#pragma endregion message_processing
};

export struct SelectedDateTime
{
	bool isUserSelected = false;	// == true if user made a selection (clicked the Ok button).

	FILETIMEX_RANGE dates;

	SelectedDateTime() = default;
	SelectedDateTime(const FILETIMEX& from, const FILETIMEX& to)
			: dates(from, to) {}
	SelectedDateTime(bool user_selected, const FILETIMEX& from, const FILETIMEX& to)
			: isUserSelected(user_selected), dates(from, to) {}

	bool IsFromValid() const { return dates.m_ftFrom.IsValid(); }
	bool IsToValid() const { return dates.m_ftTo.IsValid(); }
};

export class CXeCalendarCtrlDropDown
{
protected:
	CXeUIcolorsIF* m_xeUI = nullptr;

public:
	CXeCalendarCtrlDropDown(CXeUIcolorsIF* pUIcolors) : m_xeUI(pUIcolors) {}
	//virtual ~CXeCalendarCtrlDropDown() {}

public:
	SelectedDateTime ShowDropDown(HWND hParent, const CRect& rcParentCtrlScreenPos,
		const FILETIMEX_RANGE& dates, CalCtrlMode mode = CalCtrlMode::FullRange)
	{
		CXeCalendarCtrl calendar_ctrl(m_xeUI, mode);
		calendar_ctrl.SetDateTimes(dates.m_ftFrom, dates.m_ftTo);
		CSize sCtrl = calendar_ctrl.GetControlSize();
		int scrX = rcParentCtrlScreenPos.left, scrY = rcParentCtrlScreenPos.bottom;
		CRect rcCtrl(scrX, scrY, scrX + sCtrl.cx, scrY + sCtrl.cy);
		CXePopupCtrl popup(m_xeUI, &calendar_ctrl, XeShowPopup::FadeIn80);
		popup.ShowPopup(hParent, rcCtrl, rcParentCtrlScreenPos);
		if (calendar_ctrl.m_isOkBtnClicked)
		{
			return SelectedDateTime(true, calendar_ctrl.m_fromDT, calendar_ctrl.m_toDT);
		}
		return SelectedDateTime();
	}
};

