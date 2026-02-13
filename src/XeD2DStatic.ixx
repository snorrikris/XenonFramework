module;

#include "os_minimal.h"
#include <string>
#include <d2d1.h>
#include <dwrite.h>

export module Xe.D2DStatic;

import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.D2DWndBase;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

constexpr wchar_t XED2DSTATICWND_CLASSNAME[] = L"XeD2DStaticWndClass";  // Window class name

export class CXeD2DStatic : public CXeD2DCtrlBase
{
#pragma region Class_data
protected:
	bool								m_isToolbarMode = false;

	EXE_FONT							m_font = EXE_FONT::eUI_Font;

	PID									m_imagePID = PID::None;
	Microsoft::WRL::ComPtr<IWICBitmap>	m_iconBitmap;
	HICON								m_hIcon = 0;

	UINT								m_resourceId = 0;

	Microsoft::WRL::ComPtr<IDWriteTextLayout> m_textLayout;

	// Text margin from image.
	float m_xTextMargin = 3.0f;

	CID m_txtClr = CID::CtrlTxt, m_disTxtClr = CID::CtrlTxtDis, m_downTxtClr = CID::CtrlBg;
	CID m_urlTxtClr = CID::CtrlTxtUrl, m_urlTxtHotClr = CID::CtrlTxtUrlHot;
	CID m_bgClr = CID::CtrlBg, m_bgHotClr = CID::CtrlBtnHotBg, m_downBgClr = CID::CtrlTxt, m_disBgClr = CID::CtrlBgDis;
	CID m_bgDialog = CID::DialogBg;
	CID m_triangleUpClr = CID::CtrlTxt, m_triangleDownClr = CID::CtrlBg;
	CID m_borderClr = CID::CtrlBtnBorder, m_defBorderClr = CID::CtrlBtnDefBorder, m_splitLineClr = CID::CtrlBtnBorder;
	CID m_checkBoxClr = CID::CtrlTxt, m_disCheckBoxClr = CID::CtrlTxtDis, m_indetermCheckBoxClr = CID::CtrlTxtDis;
	CID m_focusRectClr = CID::CtrlTxtDis, m_groupBoxClr = CID::CtrlTxt;
#pragma endregion Class_data

#pragma region Create
public:
	CXeD2DStatic(CXeUIcolorsIF* pUIcolors) : CXeD2DCtrlBase(pUIcolors)
	{
		m_xeUI->RegisterWindowClass(XED2DSTATICWND_CLASSNAME, D2DCtrl_WndProc, 0 /* no class style - don't want double click */);
	}
	//virtual ~CXeD2DStatic() {}

	bool Create(LPCTSTR lpszCaption, DWORD dwStyle, DWORD dwExStyle, const CRect& rect, HWND hParentWnd, UINT nID,
		const wchar_t* tooltip)
	{
		HWND hWnd = CreateD2DCtrl(dwExStyle, XED2DSTATICWND_CLASSNAME, lpszCaption, dwStyle,
				rect, hParentWnd, nID, tooltip);
		return hWnd != 0;
	}

	bool Create(UINT resourceId, DWORD dwStyle, DWORD dwExStyle, const CRect& rect, HWND hParentWnd, UINT nID,
		const wchar_t* tooltip)
	{
		m_resourceId = resourceId;
		HWND hWnd = CreateD2DCtrl(dwExStyle, XED2DSTATICWND_CLASSNAME, NULL, dwStyle,
				rect, hParentWnd, nID, tooltip);
		return hWnd != 0;
	}
#pragma endregion Create

#pragma region CalculateUI
protected:
	D2D1_RECT_F _CalculateTextRect(const D2D1_RECT_F& rcClient)
	{
		D2D1_RECT_F rc = _GetFocusRect(rcClient);
		return rc;
	}

	D2D1_RECT_F _GetFocusRect(const D2D1_RECT_F& rcClient) const
	{
		D2D1_RECT_F rc(rcClient);
		float xyo = 2.5f;
		rc.left += xyo; rc.right -= xyo;
		rc.top += xyo; rc.bottom -= xyo;
		return rc;
	}
#pragma endregion CalculateUI

#pragma region UI_Drawing
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override
	{
		bool isStyleChanged = m_style.Update();
		if (isStyleChanged && m_textLayout.Get())
		{
			m_textLayout.Reset();	// Re-create text object.
		}

		float cxClient = (rcClient.right - rcClient.left);
		float cyClient = (rcClient.bottom - rcClient.top);
		if (cxClient < 1 || cyClient < 1)
		{
			return;	// Nothing to paint.
		}

		bool isEnabled = ::IsWindowEnabled(Hwnd());

		_PaintInternal(this, rcClient, isEnabled/*, m_isDown, m_isHovering*/);

		if (m_control_has_border)
		{
			pRT->DrawRectangle(rcClient, GetBrush(CID::CtrlTxtDis));
		}
	}

public:
	/*
	* Note - the _PaintInternal function is called both from 'this' class and from D2DToolbar.
	* When called from the toolbar - pRctx is the context of the toolbar - meaning that we should not use any 
	* internal (of 'this' class) paint resources.
	*/
	void _PaintInternal(CXeD2DRenderContext* pRctx, D2D1_RECT_F rcClient, bool isEnabled)
	{
		ID2D1RenderTarget* pRT = pRctx->m_pCurrentRT;
		XeASSERT(pRT);
		std::wstring txt = GetWindowTextStdW(Hwnd());
		bool hasText = txt.size() > 0;
		bool hasImage = m_imagePID != PID::None || m_resourceId != 0 || m_hIcon != 0;
		bool isShowImageAndText = hasImage && !m_style.WS().HasSS_ImageStyle();
		bool isShowImageOnly = hasImage && m_style.WS().HasSS_ImageStyle();

		CID fgCID = isEnabled ? m_txtClr : m_disTxtClr;

		D2D1_RECT_F rcText = _CalculateTextRect(rcClient);

		// Fill background.
		pRT->Clear(m_xeUI->GetColorF(m_bgDialog));

		if (hasImage)
		{
			if ((m_resourceId != 0 || m_hIcon != 0) && m_style.WS().HasSS_IconStyle())
			{
				if (!m_iconBitmap)
				{
					HICON hIcon = m_hIcon != 0 ? m_hIcon
							: ::LoadIconW(m_xeUI->GetInstanceHandle(), MAKEINTRESOURCE(m_resourceId));
					if (hIcon)
					{
						m_iconBitmap = _CreateFromHicon(hIcon);
					}
				}
				if (m_iconBitmap)
				{
					D2D1_RECT_F rcIcon(rcText);
					float cxIcon = WidthOf(rcIcon), cyIcon = HeightOf(rcIcon);
					if (cxIcon < cyIcon)
					{
						rcIcon.bottom = rcIcon.top + cxIcon;
					}
					if (cyIcon < cxIcon)
					{
						rcIcon.right = rcIcon.left + cyIcon;
					}
					_DrawIcon(pRT, rcIcon, m_iconBitmap.Get());
				}
			}
			else if (m_imagePID != PID::None)
			{
				// Draw image.
				bool isImgOnLeftSide = isShowImageOnly ? false : hasText;
				D2D1_RECT_F rcImg = pRctx->_DrawButtonImage(pRT, m_imagePID, rcText, isImgOnLeftSide, isEnabled, false);
				if (!isShowImageOnly && hasText)
				{
					// Calculate text box.
					rcText.left = rcImg.right + m_xTextMargin;
				}
			}
		}

		// Draw text.
		if (!isShowImageOnly && hasText)
		{
			if (!m_textLayout.Get())
			{
				_CreateTextLayout(pRT, rcText, txt, fgCID);
			}
			if (m_textLayout.Get())
			{
				pRT->DrawTextLayout(D2D1_POINT_2F(rcText.left, rcText.top), m_textLayout.Get(),
						pRctx->GetBrush(fgCID), D2D1_DRAW_TEXT_OPTIONS_NONE);
			}
		}
	}

protected:
	void _CreateTextLayout(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rcText, const std::wstring& txt, CID fgCID)
	{
		IDWriteTextFormat* pUIfont = m_xeUI->D2D_GetFont(m_font);
		HRESULT hr = m_xeUI->D2D_GetWriteFactory()->CreateTextLayout(txt.c_str(), (UINT32)txt.size(),
			pUIfont, (rcText.right - rcText.left), (rcText.bottom - rcText.top), &m_textLayout);
		XeASSERT(hr == S_OK);
		if (m_textLayout.Get())
		{
			DWRITE_TEXT_ALIGNMENT align = m_style.WS().IsSS_LEFT() ? DWRITE_TEXT_ALIGNMENT_LEADING
				: m_style.WS().IsSS_RIGHT() ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_CENTER;
			m_textLayout->SetTextAlignment(align);
			DWRITE_PARAGRAPH_ALIGNMENT valign = /*m_style.IsBS_TOP() ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR
				: m_style.IsBS_BOTTOM() ? DWRITE_PARAGRAPH_ALIGNMENT_FAR :*/ DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			m_textLayout->SetParagraphAlignment(valign);
			m_textLayout->SetWordWrapping(/*m_style.IsBS_MULTILINE() ? DWRITE_WORD_WRAPPING_WRAP :*/ DWRITE_WORD_WRAPPING_NO_WRAP);
		}
	}

	D2D1_SIZE_F _GetTextSize()
	{
		D2D1_SIZE_F size{ 0, 0 };
		XeASSERT(m_textLayout.Get());
		if (m_textLayout.Get())
		{
			// Get text size  
			DWRITE_TEXT_METRICS textMetrics;
			HRESULT hr = m_textLayout->GetMetrics(&textMetrics);
			size = D2D1::SizeF((float)ceil(textMetrics.widthIncludingTrailingWhitespace),
				(float)ceil(textMetrics.height));
		}
		return size;
	}
#pragma endregion UI_Drawing

#pragma region Create_UI_resources
protected:
	virtual void _CreatePaintResources(ID2D1RenderTarget* pRT) override
	{
		CXeD2DCtrlBase::_CreatePaintResources(pRT);
	}
#pragma endregion Create_UI_resources

#pragma region MessageHandlers
protected:
	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		return CXeD2DCtrlBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{
		if (m_textLayout.Get())
		{
			m_textLayout.Reset();
		}
		return CXeD2DCtrlBase::_OnSize(hWnd, wParam, lParam);
	}

	virtual LRESULT _OnSetText(WPARAM wParam, LPARAM lParam) override
	{
		if (m_textLayout.Get())
		{
			m_textLayout.Reset();
		}
		return CXeD2DCtrlBase::_OnSetText(wParam, lParam);
	}
#pragma endregion MessageHandlers

#pragma region MFC_functions
public:
	void SetIcon(HICON hIcon)
	{
		m_hIcon = hIcon;
	}

	void SetFont(EXE_FONT font)
	{
		m_font = font;
		m_textLayout.Reset();
	}
#pragma endregion MFC_functions
};

