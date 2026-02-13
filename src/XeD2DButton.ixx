module;

#include "os_minimal.h"
#include <string>
#include <d2d1.h>
#include <dwrite.h>

export module Xe.D2DButton;

import Xe.UIcolorsIF;
import Xe.Helpers;
import Xe.D2DWndBase;
import Xe.ColorPicker;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************************
	TODO:

V	ASSERT when invalid style

V	Groupbox implement

	Radio button group (in dialog) - implement

V	Paint code - move to base class (or another place)

V	Image button - implement to specs. (except hBitmap is PID)

	BM_SETIMAGE message - implement (special flags too)

V	Implement all button control messages BM_GETCHECK...BM_SETDONTCLICK

	Do we need to implement any Button Control Notifications (BCN_xxx)

	BS_NOTIFY implement - see: https://learn.microsoft.com/en-us/windows/win32/controls/button-messages

	Handle change window style

	Handle WM_GETDLGCODE
****************************************************************************************/

// XeD2DButton special settings flags (wParam in BM_SETIMAGE message).
export constexpr DWORD D2D_BS_IGNORE_SPLIT_BS = 0x0001;	// Don't draw separator in split button and ignore the split style.
export constexpr DWORD D2D_BS_COLOR_BUTTON    = 0x0002;	// Button is a color picker button.
export constexpr DWORD D2D_BS_MAX_IMG_SIZE    = 0x0004;	// Button has image only and wants maximum image size.

struct D2DBtnStyle
{
	DWORD m_imageFlags = 0;	// wParam from BM_SETIMAGE message.

	bool IsIgnoreSplit() const { return m_imageFlags & D2D_BS_IGNORE_SPLIT_BS; }
	bool IsColorButton() const { return m_imageFlags & D2D_BS_COLOR_BUTTON; }
	bool IsMaxImgSize()  const { return m_imageFlags & D2D_BS_MAX_IMG_SIZE; }
};

constexpr wchar_t XED2DBUTTONWND_CLASSNAME[] = L"XeD2DButtonWndClass";  // Window class name

export class CXeD2DButton : public CXeD2DCtrlBase
{
#pragma region Class_data
protected:
	D2DBtnStyle m_d2dstyle;

	bool m_isToolbarMode = false;

	DWORD m_button_state = BST_UNCHECKED;

	PID m_imagePID = PID::None;

	bool m_isDown = false, m_isDropButtonDown = false;

	ColorPickerMode m_picker_mode = ColorPickerMode::Simple;	// When color button.
	COLORREF m_rgb = 0;											// When color button.

	Microsoft::WRL::ComPtr<IDWriteTextLayout> m_textLayout;

	// Check box (radio circle) offset from edge and width/height.
	float m_xyoCB = 4.5f, m_cxyCB = 12.0f;

	// Text margin from image.
	float m_xTextMargin = 3.0f;

	// Triangle - when split button.
	D2D1_RECT_F m_rcTriangle, m_rcDropButton;
	float m_xSplitButtonLine;

	D2D1_POINT_2F m_lastMousePoint;

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
	CXeD2DButton(CXeUIcolorsIF* pUIcolors) : CXeD2DCtrlBase(pUIcolors)
	{
		m_xeUI->RegisterWindowClass(XED2DBUTTONWND_CLASSNAME, D2DCtrl_WndProc, 0 /* no class style - don't want double click */);
	}
	//virtual ~CXeD2DButton() {}

	bool Create(LPCTSTR lpszCaption, DWORD dwStyle, const CRect& rect, HWND hParentWnd,
			UINT nID, const wchar_t* tooltip)
	{
		DWORD dwExStyle = 0;
		HWND hWnd = CreateD2DCtrl(dwExStyle, XED2DBUTTONWND_CLASSNAME, lpszCaption, dwStyle,
				rect, hParentWnd, nID, tooltip);
		return hWnd != 0;
	}

	bool CreateTBmode(LPCTSTR lpszCaption, DWORD dwStyle, const CRect& rect, HWND hParentWnd,
			UINT nID, const wchar_t* tooltip)
	{
		m_bgDialog = CID::XTbBarBg;
		m_isToolbarMode = true;
		return Create(lpszCaption, dwStyle, rect, hParentWnd, nID, tooltip);
	}

	bool CreateColorButton(DWORD dwStyle, const CRect& rect, HWND hParentWnd, UINT nID,
		const wchar_t* tooltip)
	{
		m_d2dstyle.m_imageFlags = D2D_BS_IGNORE_SPLIT_BS | D2D_BS_COLOR_BUTTON;
		dwStyle = (dwStyle & 0xFFFFF000) | BS_SPLITBUTTON;
		return Create(nullptr, dwStyle, rect, hParentWnd, nID, tooltip);
	}

	bool CreateColorButtonTBmode(DWORD dwStyle, const CRect& rect, HWND hParentWnd,
			UINT nID, const wchar_t* tooltip)
	{
		m_bgDialog = CID::XTbBarBg;
		m_isToolbarMode = true;
		return CreateColorButton(dwStyle, rect, hParentWnd, nID, tooltip);
	}

	void SetColorButtonMode()
	{
		m_d2dstyle.m_imageFlags = D2D_BS_IGNORE_SPLIT_BS | D2D_BS_COLOR_BUTTON;
		DWORD dwStyle = ::GetWindowLongW(Hwnd(), GWL_STYLE);
		dwStyle = (dwStyle & 0xFFFFF000) | BS_SPLITBUTTON;
		::SetWindowLongW(Hwnd(), GWL_STYLE, dwStyle);
		_RedrawDirectly();
	}

	bool IsColorButtonMode() const
	{
		return m_d2dstyle.m_imageFlags & D2D_BS_COLOR_BUTTON;
	}

	void SetTooltip(const std::wstring& tooltip)
	{
		m_tooltip_string = tooltip;
		_EnableTooltip(L"CXeD2DButton");
	}
#pragma endregion Create

#pragma region Misc
public:
	// CID::Invalid will reset to default
	void SetButtonBgColor(CID bgColor = CID::Invalid)
	{
		if (bgColor == CID::Invalid)
		{
			m_bgClr = CID::CtrlBg;
			m_bgDialog = m_isToolbarMode ? CID::XTbBarBg : CID::DialogBg;
		}
		else
		{
			m_bgClr = bgColor;
			m_bgDialog = bgColor;
		}
	}
#pragma endregion Misc

#pragma region ColorButton
public:
	COLORREF GetColor() const { return m_rgb; }
	void SetColor(COLORREF rgb)
	{
		m_rgb = rgb;
		_RedrawDirectly();
	}

	void SetColorPickerLayoutMode(bool isSimple)
	{
		m_picker_mode = isSimple ? ColorPickerMode::Simple : ColorPickerMode::Full;
	}
	bool GetColorPickerLayoutMode() const { return m_picker_mode == ColorPickerMode::Simple; }

protected:
	bool _ShowColorPicker()
	{
		CXeColorPickerDropDown pcr(m_xeUI, m_rgb);
		CRect rect;
		GetClientRect(rect);
		ClientToScreen(rect);
		pcr.ShowDropDown(Hwnd(), rect, m_picker_mode);
		if (pcr.HasUserSelectedColor())
		{
			m_picker_mode = pcr.GetColorPickerLayoutMode();
			m_rgb = pcr.GetSelectedColor();
			_RedrawDirectly();
			return true;
		}
		return false;
	}
#pragma endregion ColorButton

#pragma region CalculateUI
protected:
	void _CalculateDropDownButtonTriangle(const D2D1_RECT_F& rcClient)
	{
		D2D1_RECT_F rc = DeflateRectF(rcClient, 3.5f);
		rc.left = rc.right - 10;
		rc.bottom = rc.top + 10;
		rc = OffsetRectF(rc, -2.0f, 0.0f);
		CenterRectFInRectF(rc, rcClient, false, true);
		m_rcTriangle = rc;
		m_xSplitButtonLine = m_rcTriangle.left - 5;
		m_rcDropButton = rcClient;
		m_rcDropButton.left = m_xSplitButtonLine;
	}

	D2D1_RECT_F _GetCheckBoxRect(const D2D1_RECT_F& rcClient) const
	{
		D2D1_RECT_F cb;
		float cyMargin = ((rcClient.bottom - rcClient.top) - m_cxyCB) / 2;
		if (m_style.WS().IsBS_LEFTTEXT())	// Text on left side of check box (radio circle)?
		{
			cb.right = rcClient.right - m_xyoCB;
			cb.left = cb.right - m_cxyCB;
		}
		else
		{
			cb.left = rcClient.left + m_xyoCB;
			cb.right = cb.left + m_cxyCB;
		}
		cb.top = rcClient.top + cyMargin;
		cb.bottom = cb.top + m_cxyCB;
		return cb;
	}

	D2D1_RECT_F _CalculateTextRect(const D2D1_RECT_F& rcClient)
	{
		D2D1_RECT_F rc = _GetFocusRect(rcClient);
		if (m_style.WS().IsCheckOrRadioBox())
		{
			if (m_style.WS().IsBS_LEFTTEXT())	// Text on left side of check box (radio circle)?
			{
				rc.right = rcClient.right - (m_xyoCB + m_cxyCB + m_xyoCB);
			}
			else
			{
				rc.left = rcClient.left + (m_xyoCB + m_cxyCB + m_xyoCB);
			}
		}
		else if (m_style.WS().IsGroupBox())
		{
			rc = _GetGroupBoxRect(rcClient);
			const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
			float xOfs = (float)tm.m_aveCharWidth * 1.0f;
			rc.left += xOfs;
			rc.right -= xOfs;
			rc.top = rcClient.top;
			rc.bottom = rc.top + (float)tm.GetHeight();
		}
		else if (m_style.WS().IsSplitButton())
		{
			_CalculateDropDownButtonTriangle(rcClient);
			rc.right = m_xSplitButtonLine - 2.0f;
		}
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

	D2D1_RECT_F _GetGroupBoxRect(const D2D1_RECT_F& rcClient) const
	{
		D2D1_RECT_F rc(rcClient);
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		float xOfs = (float)tm.m_aveCharWidth / 2.0f;
		float yOfs = (float)tm.GetHeight() / 2.0f;
		//rc.left += xOfs;
		//rc.right -= xOfs;
		rc.top += yOfs;
		//rc.bottom -= yOfs;
		return rc;
	}
#pragma endregion CalculateUI

#pragma region UI_Drawing
protected:
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

		_PaintInternal(this, rcClient, isEnabled, m_isDown, m_isHovering);
	}

public:
	/*
	* Note - the _PaintInternal function is called both from 'this' class and from D2DToolbar.
	* When called from the toolbar - pRctx is the context of the toolbar - meaning that we should not use any 
	* internal (of 'this' class) paint resources.
	*/
	void _PaintInternal(CXeD2DRenderContext* pRctx, D2D1_RECT_F rcClient, bool isEnabled, bool isDown, bool isHovering)
	{
		ID2D1RenderTarget* pRT = pRctx->m_pCurrentRT;
		XeASSERT(pRT);
		std::wstring txt = GetWindowTextStdW(Hwnd());
		bool hasText = txt.size() > 0;
		bool hasImage = m_imagePID != PID::None;
		bool isDefButton = m_style.WS().IsDefaultButton();
		bool isShowImageAndText = hasImage && !m_style.WS().HasImageStyle();
		bool isShowImageOnly = hasImage && m_style.WS().HasImageStyle();

		bool isInvertBgFg = m_style.WS().IsCheck_Radio_Url_or_GroupBox() ? false : isDown;
		bool isHoverBg = m_style.WS().IsCheck_Radio_Url_or_GroupBox() ? false : isHovering;
		bool isTBimgBtnUp = m_isToolbarMode && isShowImageOnly && !(isDown || isHovering);
		CID bgCID = isInvertBgFg ? m_downBgClr : isHoverBg ? m_bgHotClr : m_bgClr;
		CID fgCID = isInvertBgFg ? m_downTxtClr : isEnabled ? m_txtClr : m_disTxtClr;
		if (m_style.WS().IsUrlButton() && isEnabled)
		{
			fgCID = isDown ? m_urlTxtClr : isHovering ? m_urlTxtHotClr : m_urlTxtClr;
		}

		D2D1_RECT_F rcText = _CalculateTextRect(rcClient);

		// Fill background.
		if (m_style.WS().IsSplitButton() && !m_d2dstyle.IsIgnoreSplit())
		{
			D2D1_RECT_F rcBtn(rcClient);
			rcBtn.right = m_rcDropButton.left;
			CID bgBtn = isDown ? m_downBgClr : m_bgClr;
			CID bgDrp = m_isDropButtonDown ? m_downBgClr : m_bgClr;
			if (isHovering && !isDown && !m_isDropButtonDown)
			{
				if (PointFInRectF(rcBtn, m_lastMousePoint))
				{
					bgBtn = m_bgHotClr;
				}
				else if (PointFInRectF(m_rcDropButton, m_lastMousePoint))
				{
					bgDrp = m_bgHotClr;
				}
			}
			pRT->FillRectangle(rcBtn, pRctx->GetBrush(bgBtn));
			pRT->FillRectangle(m_rcDropButton, pRctx->GetBrush(bgDrp));
		}
		else
		{
			CID bg = m_style.WS().IsCheck_Radio_Url_or_GroupBox() || isTBimgBtnUp ? m_bgDialog : bgCID;
			pRT->FillRectangle(rcClient, pRctx->GetBrush(bg));
		}

		if (hasImage)
		{
			// Draw image.
			bool isImgOnLeftSide = isShowImageOnly ? false : hasText /*&& m_style.HasImageStyle()*/;
			D2D1_RECT_F rcImgBox = m_d2dstyle.IsMaxImgSize() ? rcClient : rcText;
			D2D1_RECT_F rcImg = pRctx->_DrawButtonImage(pRT, m_imagePID, rcImgBox, isImgOnLeftSide, isEnabled, isHovering);
			if (!isShowImageOnly && hasText)
			{
				// Calculate text box.
				rcText.left = rcImg.right + m_xTextMargin;
			}
		}

		if (m_style.WS().IsCheckBox())
		{
			pRctx->_DrawCheckBox(pRT, _GetCheckBoxRect(rcClient), m_button_state,
					isEnabled ? m_checkBoxClr : m_disCheckBoxClr, m_indetermCheckBoxClr, isHovering);
		}
		else if (m_style.WS().IsRadioButton())
		{
			pRctx->_DrawRadioButton(pRT, _GetCheckBoxRect(rcClient), m_button_state,
					isEnabled ? m_checkBoxClr : m_disCheckBoxClr, isHovering);
		}
		else if (m_style.WS().IsGroupBox())
		{
			D2D1_RECT_F rcGrpBox = _GetGroupBoxRect(rcClient);
			D2D1_ROUNDED_RECT rcGB;
			rcGB.rect = rcGrpBox;
			rcGB.radiusX = rcGB.radiusY = 4.0f;
			pRT->DrawRoundedRectangle(rcGB, pRctx->GetBrush(m_groupBoxClr), 1.0f);
		}
		else
		{
			// Draw border (toolbar image button has no border in the up state).
			bool isDrawBorder = isTBimgBtnUp ? false : true;
			if (m_style.WS().IsUrlButton() && !m_style.WS().IsDefaultButton())
			{
				isDrawBorder = false;	// Plain Url button has no border.
			}
			if (isDrawBorder)
			{
				ID2D1SolidColorBrush* pBrdBrsh = pRctx->GetBrush(isDefButton && isEnabled ? m_defBorderClr : m_borderClr);
				D2D1_RECT_F rcBorder(rcClient);
				if (isDefButton)
				{
					rcBorder = DeflateRectF(rcBorder, 1.0f);
				}
				pRT->DrawRectangle(rcBorder, pBrdBrsh, isDefButton ? 2.0f : 1.0f);
			}
		}

		if (m_style.WS().IsSplitButton())
		{
			bool isTriangleDown = m_d2dstyle.IsIgnoreSplit() ? isDown : m_isDropButtonDown;
			CID triangleColor = isTriangleDown ? m_triangleDownClr : isEnabled ? m_triangleUpClr : m_disTxtClr;
			pRctx->_DrawFilledTriangle(pRT, m_rcTriangle, false, triangleColor);
			if (!m_d2dstyle.IsIgnoreSplit())
			{
				pRT->DrawLine({ m_xSplitButtonLine, rcClient.top }, { m_xSplitButtonLine, rcClient.bottom },
						pRctx->GetBrush(m_splitLineClr), 2.0f);
			}
		}

		if (m_d2dstyle.IsColorButton())
		{
			XeD2D1_COLOR_F clrD2;
			clrD2.SetFromRGB((COLORREF)m_rgb, false);
			Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> colorBrush = GetD2SolidBrush(pRT, clrD2);
			D2D1_RECT_F rcColor = DeflateRectF(rcText, 2.0f);
			pRT->FillRectangle(rcColor, colorBrush.Get());
		}

		// Draw text.
		if (!isShowImageOnly && hasText)
		{
			if (!m_textLayout.Get())
			{
				_CreateTextLayout(pRT, rcText, txt, fgCID);
			}
			if (m_style.WS().IsUrlButton())
			{
				m_textLayout->SetUnderline(isHovering, DWRITE_TEXT_RANGE(0, (UINT32)txt.size()));
			}
			if (m_style.WS().IsGroupBox())
			{
				D2D1_SIZE_F sizeTxt = _GetTextSize();
				D2D1_RECT_F rcTxtBg(rcText);
				SetRectFSize(rcTxtBg, sizeTxt);
				float cxTextFull = rcText.right - rcText.left;
				if (m_style.WS().IsBS_RIGHT() && cxTextFull > sizeTxt.width)
				{
					rcTxtBg = OffsetRectF(rcTxtBg, (cxTextFull - sizeTxt.width), 0.0f);
				}
				else if (m_style.WS().IsBS_CENTER() && cxTextFull > sizeTxt.width)
				{
					CenterRectFInRectF(rcTxtBg, rcText, true, false);
				}
				pRT->FillRectangle(rcTxtBg, pRctx->GetBrush(m_bgDialog));
			}
			pRT->DrawTextLayout(D2D1_POINT_2F(rcText.left, rcText.top), m_textLayout.Get(),
					pRctx->GetBrush(fgCID), D2D1_DRAW_TEXT_OPTIONS_NONE);
		}

		if (::GetFocus() == Hwnd())
		{
			_DrawFocusRect(pRT, _GetFocusRect(rcClient), m_focusRectClr);
		}
	}

protected:
	void _CreateTextLayout(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rcText, const std::wstring& txt, CID fgCID)
	{
		IDWriteTextFormat* pUIfont = m_xeUI->D2D_GetFont(EXE_FONT::eUI_Font);
		HRESULT hr = m_xeUI->D2D_GetWriteFactory()->CreateTextLayout(txt.c_str(), (UINT32)txt.size(),
			pUIfont, (rcText.right - rcText.left), (rcText.bottom - rcText.top), &m_textLayout);
		XeASSERT(hr == S_OK);
		if (m_textLayout.Get())
		{
			DWRITE_TEXT_ALIGNMENT align = m_style.WS().IsBS_LEFT() ? DWRITE_TEXT_ALIGNMENT_LEADING
				: m_style.WS().IsBS_RIGHT() ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_CENTER;
			m_textLayout->SetTextAlignment(align);
			DWRITE_PARAGRAPH_ALIGNMENT valign = m_style.WS().IsBS_TOP() ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR
				: m_style.WS().IsBS_BOTTOM() ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
			m_textLayout->SetParagraphAlignment(valign);
			m_textLayout->SetWordWrapping(m_style.WS().IsBS_MULTILINE() ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
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

#pragma region MessageHandlers
protected:
	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		if ((uMsg >= BM_GETCHECK && uMsg <= BM_SETDONTCLICK)
				|| (uMsg >= BCM_GETIDEALSIZE && uMsg <= BCM_SETSHIELD))
		{
			return _OnButtonControlMessage(uMsg, wParam, lParam);
		}
		return CXeD2DCtrlBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	LRESULT _OnButtonControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case BM_GETCHECK:
			return m_button_state;
		case BM_SETCHECK    :
			m_button_state = (DWORD)wParam;
			_RedrawDirectly();
			break;
		case BM_GETSTATE:
			return _OnGetState();
		case BM_SETSTATE:
			m_isHovering = (bool)wParam;
			_RedrawDirectly();
			break;
		case BM_SETSTYLE:
			XeASSERT(false);	// Not supported (yet).
			break;
		case BM_CLICK:
			XeASSERT(false);	// Not supported (yet).
			break;
		case BM_GETIMAGE:
			return (LRESULT)m_imagePID;
		case BM_SETIMAGE:
			return _OnSetImage(wParam, lParam);
		case BM_SETDONTCLICK:
			break;

		case BCM_GETIDEALSIZE:
		case BCM_GETIMAGELIST:
		case BCM_GETNOTE:
		case BCM_GETNOTELENGTH:
		case BCM_GETSPLITINFO:
		case BCM_GETTEXTMARGIN:
		case BCM_SETDROPDOWNSTATE:
		case BCM_SETIMAGELIST:
		case BCM_SETNOTE:
		case BCM_SETSHIELD:
		case BCM_SETSPLITINFO:
		case BCM_SETTEXTMARGIN:
			XeASSERT(false);	// Not supported (yet).
			break;
		}
		return 0;
	}

	LRESULT _OnGetState()
	{
		bool hasFocus = ::GetFocus() == Hwnd();
		LRESULT state = m_button_state | (m_isDown ? BST_PUSHED : 0) | (m_isHovering ? BST_HOT : 0)
				| (hasFocus ? BST_FOCUS : 0) | (m_isDropButtonDown ? BST_DROPDOWNPUSHED : 0);
		return state;
	}

	LRESULT _OnSetImage(WPARAM wParam, LPARAM lParam)
	{
		PID pid = (PID)lParam;
		if (!IsValidPID(pid))
		{
			XeASSERT(false);
			return 0;
		}
		LRESULT ret = (LRESULT)m_imagePID;
		m_imagePID = pid;
		m_d2dstyle.m_imageFlags = (DWORD)wParam;
		if (m_textLayout.Get())
		{
			m_textLayout.Reset();
		}
		_RedrawDirectly();
		return ret;
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

	virtual LRESULT _OnCancelMode() override
	{
		m_isDown = m_isDropButtonDown = false;
		return CXeD2DCtrlBase::_OnCancelMode();
	}

	virtual LRESULT _OnNcHitTest(WPARAM wParam, LPARAM lParam) override
	{
		if (m_style.WS().IsGroupBox())
		{
			return HTNOWHERE;
		}
		return CXeD2DCtrlBase::_OnNcHitTest(wParam, lParam);
	}

	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint pt) override
	{
		if (m_style.WS().IsGroupBox())
		{
			return 0;
		}
		if (m_style.WS().IsSplitButton() && !m_d2dstyle.IsIgnoreSplit()
				&& PointFInRectF(m_rcDropButton, {(float)pt.x, (float)pt.y}))
		{
			m_isDropButtonDown = true;
		}
		else
		{
			m_isDown = true;
		}
		if (m_style.WS().IsAutoCheckOrRadioBox())
		{
			if (m_style.WS().IsAutoCheckBox())
			{
				if (m_style.WS().Is3stateCheckBox())
				{
					m_button_state = m_button_state == BST_INDETERMINATE ? BST_CHECKED
							: m_button_state == BST_CHECKED ? BST_UNCHECKED : BST_INDETERMINATE;
				}
				else
				{
					m_button_state = m_button_state == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED;
				}
			}
			else
			{
					m_button_state = m_button_state == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED;
			}
		}

		if (m_style.WS().hasTabStop())
		{
			_TakeFocusAndRedraw();
		}
		else
		{
			_RedrawDirectly();
		}
		return 0;
	}

	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint pt) override
	{
		if (m_style.WS().IsGroupBox())
		{
			return 0;
		}
		bool isDown = m_isDown, isDropButtonDown = m_isDropButtonDown;
		m_isDown = m_isDropButtonDown = false;
		_RedrawDirectly();
		HWND hWndParent = ::GetParent(Hwnd());
		UINT uID = ::GetDlgCtrlID(Hwnd());
		if (isDown)
		{
			bool isSendNotify = true;
			if (m_d2dstyle.IsColorButton())
			{
				isSendNotify = _ShowColorPicker();
			}
			if (isSendNotify)
			{
				// Note - BN_CLICKED notify code is = 0 (it's in the high word of wParam in MicroSoft specs.)
				::SendMessageW(hWndParent, WM_COMMAND, uID, (LPARAM)Hwnd());
			}
		}
		else if (isDropButtonDown)
		{
			NMBCDROPDOWN nf;
			nf.hdr.hwndFrom = Hwnd();
			nf.hdr.idFrom = uID;
			nf.hdr.code = BCN_DROPDOWN;
			nf.rcButton.left = (LONG)(m_rcDropButton.left + 0.5);
			nf.rcButton.right = (LONG)(m_rcDropButton.right + 0.5);
			nf.rcButton.top = (LONG)(m_rcDropButton.top + 0.5);
			nf.rcButton.bottom = (LONG)(m_rcDropButton.bottom + 0.5);
			LPARAM lp = (LPARAM)&nf;
			::SendMessageW(hWndParent, WM_NOTIFY, uID, lp);
		}
		return 0;
	}

	virtual LRESULT _OnMouseActivate(WPARAM wParam, LPARAM lParam) override
	{
		if (m_style.WS().IsGroupBox())
		{
			return MA_NOACTIVATEANDEAT;
		}
		return CXeD2DCtrlBase::_OnMouseActivate(wParam, lParam);
	}

	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{
		if (m_style.WS().IsGroupBox())
		{
			return 0;	// Don't want mouse tracking messages when 'this' is group box.
		}
		if (m_style.WS().IsSplitButton() && !m_d2dstyle.IsIgnoreSplit())
		{
			m_lastMousePoint.x = (float)point.x;
			m_lastMousePoint.y = (float)point.y;
			_RedrawDirectly();
		}
		return CXeD2DCtrlBase::_OnMouseMove(nFlags, point);
	}

	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override
	{
		if (m_style.WS().IsGroupBox())
		{
			return 0;	// Don't want mouse tracking messages when 'this' is group box.
		}
		m_isDown = m_isDropButtonDown = false;
		return CXeD2DCtrlBase::_OnMouseLeave(wparam, lparam);
	}

	virtual LRESULT _OnMouseHover(WPARAM wparam, LPARAM lparam) override
	{
		if (m_style.WS().IsGroupBox())
		{
			return 0;	// Don't want mouse tracking messages when 'this' is group box.
		}
		return CXeD2DCtrlBase::_OnMouseHover(wparam, lparam);

	}

	virtual LRESULT _OnKeyDown(WPARAM wParam, LPARAM lParam) override
	{
		UINT nChar = (UINT)wParam;
		if (nChar == VK_SPACE)
		{
			CRect rcClient = _GetClientRect();
			CPoint pt(rcClient.Width() / 2, rcClient.Height() / 2);
			_OnLeftDown(0, pt);
			_OnLeftUp(0, pt);
		}
		return 0;
	}
#pragma endregion MessageHandlers
};

