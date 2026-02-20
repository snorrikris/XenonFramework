module;

#include "os_minimal.h"
#include <string>
#include <memory>

export module Demo.TextVw;

import Demo.FileVwBase;
import Xe.ScintillaEditControl;
import Xe.FileHelpers;

export class CDemoTextVw : public CDemoFileVwBase
{
protected:
	std::unique_ptr<CXeScintillaEditControl> m_textCtrl;

public:
	CDemoTextVw(CViewManagerDemoIF* pVwMgr, dsid_t dsId) : CDemoFileVwBase(pVwMgr, dsId)
	{
		std::wstring class_name = L"CDemoTextVwWndClass";
		m_xeUI->RegisterWindowClass(class_name, D2DCtrl_WndProc);

		DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		HWND hWnd = CreateD2DWindow(0, class_name.c_str(), nullptr, dwStyle,
			CRect(), m_xeUI->GetMainWindowHandle(), 0);
		m_textCtrl = std::make_unique<CXeScintillaEditControl>(m_xeUI);
		dwStyle = ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN
			| WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		m_textCtrl->Create(dwStyle, hWnd, CRect(), 0, nullptr);
	}

	virtual bool LoadFile(const std::wstring& pathname) override
	{
		bool isOk;
		std::string data = GetSmallTextFile(pathname, &isOk);
		if (isOk)
		{
			m_textCtrl->SetText(data.c_str(), data.size(), true);
			_SetPathname(pathname);
		}
		return isOk;
	}

	virtual void SetFocusToView() override
	{
		if (m_textCtrl->GetSafeHwnd())
		{
			m_textCtrl->SetFocus();
		}
	}

	virtual void OnChangedSettings(const ChangedSettings& chg_settings) override
	{
		bool isFontsChanged = m_xeUI->IsFontSettingsChanged(chg_settings);
		bool isColorsChanged = chg_settings.IsChanged(L"Colors", L"Color");
		if (isFontsChanged || isColorsChanged)
		{
			if (m_textCtrl)
			{
				m_textCtrl->OnChangedUIsettings(isFontsChanged, isColorsChanged);
			}
			RedrawWindow();
		}
	}

protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		// No painting needed - entire window is covered by a control.
	}

	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{
		UINT nType = (UINT)wParam;
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);

		LRESULT result = CXeD2DWndBase::_OnSize(hWnd, wParam, lParam);

		if (m_textCtrl->GetSafeHwnd())
		{
			m_textCtrl->MoveWindow(0, 0, cx, cy);
		}
		return result;
	}
};

