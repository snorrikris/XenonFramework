module;

#include "os_minimal.h"

export module Xe.ProgressCtrl;

import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.D2DWndBase;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

constexpr wchar_t XEPROGRESSCTRLWND_CLASSNAME[] = L"XeProgressCtrlWndClass";  // Window class name

export class CXeProgressCtrl : public CXeD2DCtrlBase
{
protected:
	int m_min = 0, m_max = 100, m_pos = 0, m_step = 1;

public:
	CXeProgressCtrl(CXeUIcolorsIF* pUIcolors) : CXeD2DCtrlBase(pUIcolors)
	{
		m_xeUI->RegisterWindowClass(XEPROGRESSCTRLWND_CLASSNAME, D2DCtrl_WndProc);
	}

	//virtual ~CXeProgressCtrl() {}

	bool Create(DWORD dwStyle, DWORD dwExStyle, const CRect& rect, HWND hParentWnd, UINT nID, const wchar_t* tooltip)
	{
		XeASSERT(hParentWnd);
		if (!::IsWindow(hParentWnd))
		{
			XeASSERT(FALSE);
			return FALSE;
		}
		HINSTANCE hInstance = m_xeUI->GetInstanceHandle();
		HWND hWnd = CreateD2DCtrl(dwExStyle, XEPROGRESSCTRLWND_CLASSNAME, nullptr, dwStyle,
				rect, hParentWnd, nID, tooltip);
		return hWnd != 0;
	}

public:
	int SetPos(int pos)
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_pos = pos;
		_RedrawDirectly();
		return m_pos;
	}

	void SetRange(short nLower, short nUpper)
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_min = nLower;
		m_max = nUpper;
		_RedrawDirectly();
	}
	void SetRange32(int nLower, int nUpper)
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_min = nLower;
		m_max = nUpper;
		_RedrawDirectly();
	}
	int GetPos() const
	{
		return m_pos;
	}
	int OffsetPos(int nPos)
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_pos += nPos;
		_RedrawDirectly();
		return m_pos;
	}
	int SetStep(int nStep)
	{
		m_step = nStep;
		return m_step;
	}
	int StepIt()
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_pos += m_step;
		_RedrawDirectly();
		return m_pos;
	}

protected:
	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		if (uMsg == PBM_SETPOS)
		{
			m_pos = (int)wParam;
			return 0;
		}
		else if (uMsg == PBM_SETRANGE32)
		{
			m_min = (int)wParam;
			m_max = (int)lParam;
			return 0;
		}
		else if (uMsg == PBM_SETRANGE)
		{
			m_min = (int)((short)LOWORD(lParam));
			m_max = (int)((short)HIWORD(lParam));
			return 0;
		}
		return CXeD2DCtrlBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

#pragma region Paint
protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override
	{
		pRT->Clear(m_xeUI->GetColorF(CID::CtrlBg)); // Fill background

		float progress = m_pos < m_min ? 0.0f : m_pos > m_max ? 1.0f : 0.0f;
		if (m_min < m_max && m_pos >= m_min && m_pos <= m_max)
		{
			progress = (float)(m_pos - m_min) / (float)(m_max - m_min);
		}
		D2D1_RECT_F rcProgress(rcClient);
		rcProgress.right = rcProgress.left + (WidthOf(rcClient) * progress);
		if (progress > 0.0f)
		{
			pRT->FillRectangle(rcProgress, GetBrush(CID::GrdProgressBg));
		}

		pRT->DrawRectangle(rcClient, GetBrush(CID::CtrlBtnBorder));
	}
#pragma endregion Paint
};

