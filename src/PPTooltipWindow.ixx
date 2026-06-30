module;

#include "os_minimal.h"
#include <string>
#include <format>
#include <d2d1.h>
#include <dwrite.h>

export module Xe.PPTooltipWindow;

import Xe.PPHtmlDrawer;
import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.D2DWndBase;
import Xe.Monitors;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CPPTOOLTIP_TRACE XeTRACE
//#define CPPTOOLTIP_TRACE (__noop)		// +++hd
#define MM_CPPTOOLTIP_TRACE XeTRACE
//#define MM_CPPTOOLTIP_TRACE (__noop)	// +++sk

#define TIMER_HIDE		0x101 //the identifier of the timer for hide the tooltip
#define TIMER_SHOW		0x100 //the identifier of the timer for show the tooltip

constexpr wchar_t PPTOOLTIPWINDOW_CLASSNAME[] = L"PPToolTipWindow";

export class CPPToolTipWindow : public CXeD2DWndBase
{
#pragma region Class_data
protected:
	enum TooltipState {
		PPTOOLTIP_STATE_HIDDEN = 0,
		PPTOOLTIP_STATE_SHOWN
	};

	std::unique_ptr<CPPHtmlDrawer> m_drawer; //HTML drawer object

	HWND m_hParentWndMainFrame = 0; // The handle of the main frame parent window.

	HWND m_hCurrentParentWnd = 0;	// Handle to current parent window (note - is always valid).

	CPoint m_ptOriginal;

	// Info about last displayed tool
	PPTOOLTIP_INFO m_tiNextTool; //Info about next tooltip

	// Info about current tool
	TooltipState m_nTooltipState = PPTOOLTIP_STATE_HIDDEN;

	//Default values for the window
	DWORD m_dwTimeAutoPop = 60000; //Retrieve the length of time the tool tip window remains visible if the pointer is stationary within a tool's bounding rectangle
	DWORD m_dwTimeInitial = 500; //Retrieve the length of time the pointer must remain stationary within a tool's bounding rectangle before the tool tip window appears

	CRect m_rcTipArea; //The bound rect around the tip's area in the client coordinates.
	CRect m_rcTooltip; //The bound rect around the body of the tooltip in the client coordinates.

	std::wstring m_logName;
#pragma endregion Class_data

#pragma region Create
public:
	CPPToolTipWindow(CXeUIcolorsIF* pUIcolors) : CXeD2DWndBase(pUIcolors)
	{
		m_drawer = std::make_unique<CPPHtmlDrawer>(pUIcolors);
		m_drawer->SetMaxWidth(0);
	}
	//virtual ~CPPToolTipWindow() {}

	BOOL Create(const wchar_t* szNameForLogging, HWND hParentWndMainFrame)
	{
		CPPTOOLTIP_TRACE("CPPToolTipWindow::Create\n");

		m_logName = szNameForLogging;

		// Register the window class if it has not already been registered.
		m_xeUI->RegisterWindowClass(PPTOOLTIPWINDOW_CLASSNAME, D2DCtrl_WndProc);

		m_hParentWndMainFrame = m_hCurrentParentWnd = hParentWndMainFrame;
		XeASSERT(hParentWndMainFrame);	// Parent window not created yet.
		HWND hWnd = _CreateTooltipWindow();
		if (!hWnd)
		{
			return false;
		}

		return TRUE;
	}

	void DestroyWindow() { ::DestroyWindow(Hwnd()); }

protected:
	HWND _CreateTooltipWindow()
	{
		DWORD dwStyle = WS_POPUP;
		DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
		HWND hWnd = CreateD2DWindow(dwExStyle, PPTOOLTIPWINDOW_CLASSNAME,
				nullptr, dwStyle, CRect(), m_hCurrentParentWnd, NULL);
		XeASSERT(hWnd == Hwnd());
		return hWnd;
	}

	LRESULT _OnDestroy() override
	{
		CPPTOOLTIP_TRACE("CPPToolTipWindow::DestroyWindow()\n");
		Pop();
		return 0;
	}
#pragma endregion Create

protected:
	void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		CPPTOOLTIP_TRACE("CPPToolTipWindow::OnPaint()\n");

		TT_Render rndr;
		rndr.m_pRT = pRT;
		rndr.m_font = m_xeUI->D2D_GetFont(EXE_FONT::eUI_Font);
		rndr.m_text_brush = GetBrush(CID::TT_Text);
		rndr.m_bg_brush = GetBrush(CID::TT_Bg);
		rndr.m_border_brush = GetBrush(CID::TT_BgBorder);

		//pRT->FillRectangle(rc, rndr.m_bg_brush);
		pRT->Clear(m_xeUI->GetColorF(CID::TT_Bg)); // Fill background

		m_drawer->DrawPreparedOutput(rndr, &m_rcTipArea);

		D2D1_ROUNDED_RECT rr;
		rr.rect = rc;
		rr.radiusX = 2;
		rr.radiusY = 2;
		pRT->DrawRoundedRectangle(rr, rndr.m_border_brush, 1.0f);
	}

	bool _FilterMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, const CPoint& msg_pt) override
	{
		RelayEvent(uMsg, msg_pt);
		return false;	// Message should be processed normally
	}

	BOOL RelayEvent(UINT message, CPoint pt_msg)
	{
		//ENG: Disables a message translation if object was't created (thanks to Stoil Todorov)
		//RUS: Запрет обработки сообщений если объект не создан
		if (NULL == GetSafeHwnd())
			return FALSE;

		XeASSERT(m_hCurrentParentWnd);

		switch (message)
		{
		case WM_RBUTTONDOWN:
			// The user has interrupted the current tool - dismiss it
			CPPTOOLTIP_TRACE("CPPToolTipWindow::Pop() in RelayEvent because WM_RBUTTONDOWN\n");
			Pop();
			break;
		case WM_LBUTTONDOWN:
			CPPTOOLTIP_TRACE("CPPToolTipWindow::WM_LBUTTONDOWN\n");
			if (IsCursorOverTooltip())
			{
				//Left Button was pressed over the tooltip
				CPoint pt = pt_msg;
				::ScreenToClient(Hwnd(), &pt);
				m_drawer->OnLButtonDown(&pt, 0);
			} //if
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_NCLBUTTONDOWN:
		case WM_NCLBUTTONDBLCLK:
		case WM_NCRBUTTONDOWN:
		case WM_NCRBUTTONDBLCLK:
		case WM_NCMBUTTONDOWN:
		case WM_NCMBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
			//		// The user has interrupted the current tool - dismiss it
			//		if (!(m_tiDisplayed.nBehaviour & PPTOOLTIP_NOCLOSE_MOUSEDOWN))
			CPPTOOLTIP_TRACE("CPPToolTipWindow::Pop() in RelayEvent because mouse or keyb\n");
			Pop();
			break;
		case WM_NCMOUSEMOVE:
			::ScreenToClient(m_hCurrentParentWnd, &pt_msg);
		case WM_MOUSEMOVE:
			_OnRelayMsg_MouseMove(pt_msg);
			break;
		} //switch

		return FALSE;
	} //End RelayEvent

	void _OnRelayMsg_MouseMove(const CPoint& pt_msg)
	{
		if (m_ptOriginal.x == pt_msg.x && m_ptOriginal.y == pt_msg.y)
		{
			return;
		}
		// The mouse pointer's position was changed

		HWND hWnd = NULL;
		CPoint pt;
		CRect rect;
		PPTOOLTIP_INFO ti;
		std::wstring strTemp;

		m_ptOriginal = pt = pt_msg;
		CPPTOOLTIP_TRACE("On mouse move. orig=%d,%d\n", m_ptOriginal.x, m_ptOriginal.y);

		if (IsCursorOverTooltip())
		{
			MM_CPPTOOLTIP_TRACE("over tooltip ==========\n");

			//ENG: Mouse over a tooltip and tracking mode was disabled
			//RUS: Курсор над тултипом при выключенном режиме "тракинга"
			if (!(m_tiNextTool.nBehaviour & PPTOOLTIP_NOCLOSE_OVER))
			{
				//ENG: A tooltip don't hides when mouse over him
				//RUS: Если не установлен стиль не закрывания тултипа если курсор над ним
				CPPTOOLTIP_TRACE("CPPToolTipWindow - over tooltip == HideTooltip\n");
				_HideTooltip();
			}
			else
			{
				//ENG: Resetup autopop timer
				//RUS: Переустанавливаем таймер автозакрытия тултипа
				SetAutoPopTimer();
			}
		}
		else
		{
			MM_CPPTOOLTIP_TRACE("CPPToolTipWindow - not over tooltip ==========\n");
			////ENG: Searching a toolbar's item
			////RUS: Ищем элемент на панели инструментов
			//if (NULL == hWnd)	// +++sk
			//{
			//	if (SendNotifyNeedTT(&pt, ti))
			//	{
			//		hWnd = ti.hTWnd;
			//	}
			//} //if
			//if (NULL == hWnd)
			//{
			//	//ENG: An item with a tooltip wasn't found
			//	//RUS: Ни один элемент, отображающий тултип, не найден
			//	m_hwndDisplayedTool = NULL;
			//	m_tiDisplayed.rectBounds.SetRectEmpty();
			//	MM_CPPTOOLTIP_TRACE("CPPToolTipWindow::KillTimer(TIMER_SHOW) An item with a tooltip wasn't found\n");
			//	::KillTimer(Hwnd(), TIMER_SHOW);
			//	_HideTooltip();
			//}
			//else
			{
				//MM_CPPTOOLTIP_TRACE("CPPToolTipWindow - found tool =========\n");
				//if ((hWnd != m_hwndDisplayedTool) || (ti.rectBounds != m_tiDisplayed.rectBounds/* m_rcDisplayedTool*/))
				//{
				//	//ENG: Sets new tooltip for the new window or for the new window's item
				//	//RUS: Если новое окно или новый элемент окна, то установить новый тултип
				//	SetNewTooltip(hWnd, ti);
				//}
				//else
				{
					//ENG: Nothing was changed
					//RUS: Если ни окно, ни элемент окна не изменялись
					MM_CPPTOOLTIP_TRACE("CPPToolTipWindow - calling SetAutoPopTimer ==========\n");
					//ENG: A tooltip don't must when a mouse is over window
					//RUS: Тултип не должен прятаться пока находится над окном
					SetAutoPopTimer();
				} //if
			} //if
		} //if
	}

public:
	void SetNewTooltip(HWND hWndParent, CPoint pt, const PPTOOLTIP_INFO& ti)
	{
		CPPTOOLTIP_TRACE("CPPToolTipWindow::SetNewTooltip(hWnd=0x%08X, CRect(left=%d, top=%d, right=%d, bottom=%d))\n",
			hWndParent, ti.rectBounds.left, ti.rectBounds.top, ti.rectBounds.right, ti.rectBounds.bottom);

		CPPTOOLTIP_TRACE("m_nTooltipState = %d\n", m_nTooltipState);

		Pop();

		if (m_hCurrentParentWnd != hWndParent)
		{
			CPPTOOLTIP_TRACE("Parent has changed\n");
			::DestroyWindow(Hwnd());

			m_hCurrentParentWnd = hWndParent;
			_CreateTooltipWindow();
		}

		//m_hCurrentParentWnd = hWndParent;
		//::SetParent(Hwnd(), m_hCurrentParentWnd);
		m_tiNextTool = ti;
		m_ptOriginal = pt;

		//ENG: Hides a tooltip
		//RUS: Прячем тултип если он показан или показывается
		if (PPTOOLTIP_STATE_SHOWN == m_nTooltipState)
		{
			CPPTOOLTIP_TRACE("CPPToolTipWindow - SHOWING or SHOWN == HideTooltip\n");
			_HideTooltip();
		}

		//+++sk
		// Prevent tootip from showing if any mouse button down or if shift, ctrl or alt down.
		//if (PreventTooltipShowing())
		//{
		//	CPPTOOLTIP_TRACE("CPPToolTipWindow::PreventTooltipShowing\n");
		//	return;
		//}

		//ENG: Start the show timer
		//RUS: Начинаем показ нового тултипа
		//if (bDisplayWithDelay && m_dwTimeInitial)
		{
			CPPTOOLTIP_TRACE("CPPToolTipWindow - Start the show timer - delay=%d\n", m_dwTimeInitial);
			::SetTimer(Hwnd(), TIMER_SHOW, m_dwTimeInitial, NULL);
		}
		//else
		//{
		//	_OnTimer(TIMER_SHOW, 0);
		//}
	} //End of SetNewTooltip

protected:
	// Note - pt is in screen coordinates
	void DisplayTooltip(CPoint pt)
	{
		CPPTOOLTIP_TRACE("CPPToolTipWindow::PrepareDisplayTooltip() - pt.x=%d, pt.y=%d\n", pt.x, pt.y);

		//If string and icon are not exist then exit
		if (m_tiNextTool.sTooltip.empty())
			return;

		HWND hWnd = m_tiNextTool.hTWnd;	//+++sk
		if (hWnd == NULL)
		{
			pt.x += 24;
			pt.y += 22;
		}
		else
		{
			CRect rcCtrl;
			::GetWindowRect(hWnd, rcCtrl);
			pt.x = rcCtrl.left + 1;
			pt.y = rcCtrl.bottom;
			pt.x += m_tiNextTool.ptTipOffset.x;
			pt.y += m_tiNextTool.ptTipOffset.y;
		}

		//calculate the width and height of the box dynamically
		CSize sz(0, 0);
		m_drawer->PrepareOutput(m_tiNextTool.sTooltip.c_str(), &sz);

		m_rcTipArea.SetRect(0, 0, sz.cx, sz.cy);
		m_rcTooltip = m_rcTipArea;

		//Inflates on MARGIN_CX and MARGIN_CY sizes
		int nMarginY = 4, nMarginX = 6;
		m_rcTipArea.OffsetRect(nMarginX, nMarginY);
		m_rcTooltip.InflateRect(0, 0, 2 * nMarginX, 2 * nMarginY);

		//ENG: Sets a tooltip on the screen
		//RUS: Устанавливаем тултип на экране
		CRect rect = _AdjustTooltipOnScreenPosition(pt, m_rcTooltip.Size());

		CPPTOOLTIP_TRACE("SetWindowPos left=%d, top=%d, right=%d, bottom=%d\n", rect.left, rect.top, rect.right, rect.bottom);
		SetWindowPos(NULL, rect, SWP_SHOWWINDOW | SWP_NOACTIVATE);
		RedrawWindow();
	} //End of PrepareDisplayTooltip

	//BOOL PreventTooltipShowing()
	//{
	//	BOOL fLmouse = (::GetKeyState(VK_LBUTTON) & 0x8000) ? TRUE : FALSE;
	//	BOOL fRmouse = (::GetKeyState(VK_RBUTTON) & 0x8000) ? TRUE : FALSE;
	//	BOOL fMmouse = (::GetKeyState(VK_MBUTTON) & 0x8000) ? TRUE : FALSE;
	//	BOOL fShiftKeyDown = (::GetKeyState(VK_SHIFT) & 0x8000) ? TRUE : FALSE;
	//	BOOL fMenuKeyDown = (::GetKeyState(VK_MENU) & 0x8000) ? TRUE : FALSE;
	//	if (fLmouse || fRmouse || fMmouse || fShiftKeyDown || fMenuKeyDown)
	//		return TRUE;
	//	return FALSE;
	//}

	LRESULT _OnTimer(WPARAM wParam, LPARAM lParam) override
	{
		UINT_PTR nIDEvent = wParam;
		CPoint pt;
		switch (nIDEvent)
		{
		case TIMER_SHOW:
			CPPTOOLTIP_TRACE("CPPToolTipWindow::OnTimerShow\n");
			//ENG: Kill SHOW timer 
			//RUS: Убить таймер ожидания показа тултипа
			::KillTimer(Hwnd(), TIMER_SHOW);
			//ENG: Get current mouse coordinates
			//RUS: Получить текущее положение тултипа
			GetCursorPos(&pt);
			::ScreenToClient(m_hCurrentParentWnd, &pt);
			//if ((pt.x != m_ptOriginal.x) || (pt.y != m_ptOriginal.y))
			if ((std::abs(pt.x - m_ptOriginal.x) > 3) || (std::abs(pt.y - m_ptOriginal.y) > 3))
			{
				//ENG: If mouse coordinates was changed
				//RUS: Если курсор сдвинулся, то уничтожить тултип
				CPPTOOLTIP_TRACE("CPPToolTipWindow::OnTimerShow(HideTooltip) mouse has moved. pt=%d,%d orig=%d,%d\n", pt.x, pt.y, m_ptOriginal.x, m_ptOriginal.y);
				_HideTooltip();
			}
			else if (PPTOOLTIP_STATE_HIDDEN == m_nTooltipState)
			{
				// Hide all other tooltips owned by 'this' process.
				//m_xeUI->HideOtherTooltips(GetSafeHwnd());

				//Display first step
				DisplayTooltip(m_ptOriginal);

				CPPTOOLTIP_TRACE("CPPToolTipWindow::OnTimerShow(Shown)\n");
				m_nTooltipState = PPTOOLTIP_STATE_SHOWN; //Tooltip is already show
				SetAutoPopTimer();
			} //if
			break;
		case TIMER_HIDE:
			CPPTOOLTIP_TRACE("CPPToolTipWindow::OnTimerHide()\n");
			Pop();
			break;
		default:
			return CXeD2DWndBase::_OnTimer(wParam, lParam);
			break;
		} //switch
		return 0;	// Message was processed.
	} //End of the WM_TIMER handler

	void _HideTooltip()
	{
		if (m_nTooltipState == PPTOOLTIP_STATE_SHOWN)
		{
			CPPTOOLTIP_TRACE("CPPToolTipWindow::HideTooltip(CurState=%d)\n", m_nTooltipState);
			_OnTimer(TIMER_HIDE, 0);
		}
	}

	void SetAutoPopTimer()
	{
		if (m_dwTimeAutoPop)
		{
			CPPTOOLTIP_TRACE("CPPToolTipWindow::SetAutoPopTimer()\n");
			::SetTimer(Hwnd(), TIMER_HIDE, m_dwTimeAutoPop, NULL);
		}
	}

	void Pop()
	{
		CPPTOOLTIP_TRACE("CPPToolTipWindow::Pop()\n");

		m_nTooltipState = PPTOOLTIP_STATE_HIDDEN;
		::KillTimer(Hwnd(), TIMER_SHOW);
		::KillTimer(Hwnd(), TIMER_HIDE);
		if (IsWindowVisible())
		{
			::ShowWindow(Hwnd(), SW_HIDE);
			//::SetParent(Hwnd(), m_hParentWndMainFrame);
		}
		m_tiNextTool = PPTOOLTIP_INFO();
	}

	[[nodiscard]] CRect _AdjustTooltipOnScreenPosition(const CPoint& pt_onscreen, const CSize& sizeTooltip)
	{
		CRect rect{ pt_onscreen, sizeTooltip };
		CMonitor mon = CMonitors::GetNearestMonitor(pt_onscreen);
		CRect rMonitor;
		mon.GetMonitorRect(&rMonitor);
		CPPTOOLTIP_TRACE("CPPToolTipWindow - rMonitor:  left=%d  top=%d  right=%d  bottom=%d --------\n",
				rMonitor.left, rMonitor.top, rMonitor.right, rMonitor.bottom);
		CPPTOOLTIP_TRACE("CPPToolTipWindow - rect:  left=%d  top=%d  right=%d  bottom=%d --------\n",
				rect.left, rect.top, rect.right, rect.bottom);

		if (rect.right > rMonitor.right)
		{
			rect.OffsetRect(rMonitor.right - rect.right, 0);
		}
		if (rect.left < rMonitor.left)
		{
			rect.OffsetRect(rMonitor.left - rect.left, 0);
		}
		if (rect.bottom > rMonitor.bottom)
		{
			CPPTOOLTIP_TRACE("CPPToolTipWindow - off the screen -----------\n");
			rect.OffsetRect(0, rMonitor.bottom - rect.bottom);
			CPPTOOLTIP_TRACE("CPPToolTipWindow - rect:  left=%d  top=%d  right=%d  bottom=%d ----------\n",
					rect.left, rect.top, rect.right, rect.bottom);
		}
		if (rect.top < rMonitor.top)
		{
			rect.OffsetRect(0, rMonitor.top - rect.top);
		}
		return rect;
	}

	BOOL IsCursorOverTooltip() const
	{
		XeASSERT(m_hCurrentParentWnd);
		if (IsWindowVisible())	// Is tooltip visible?
		{
			POINT pt;
			::GetCursorPos(&pt);
			HWND hWnd = ::WindowFromPoint(pt);
			return hWnd == Hwnd();
		}
		return FALSE;
	}
};

