module;

#include "os_minimal.h"
#include <d2d1.h>
#include <dwrite.h>
#include <d2d1effects.h>
#include <d2d1_1.h>

export module Xe.D2DRenderContext;

import std;
import Xe.UIcolorsIF;
export import Xe.mfc_types;
export import Xe.D2D_UItypes;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class XeD2DBrushMap
{
	CXeUIcolorsIF* m_xeUI;

	ID2D1RenderTarget* m_pRT;

	std::map<uint32_t, ID2D1SolidColorBrush*> m_map;

public:
	XeD2DBrushMap(CXeUIcolorsIF* pUIcolors, ID2D1RenderTarget* pRT) : m_xeUI(pUIcolors), m_pRT(pRT) {}
	~XeD2DBrushMap()
	{
		for (auto const& [key, p] : m_map)
		{
			p->Release();
		}
	}

	ID2D1SolidColorBrush* GetOrCreate(CID key)
	{
		uint32_t rgb = m_xeUI->GetColor(key);
		return GetOrCreate(rgb);
	}

	ID2D1SolidColorBrush* GetOrCreate(uint32_t rgb)
	{
		if (m_map.contains(rgb))
		{
			return m_map.at(rgb);
		}
		else
		{
			if (_CreateSolidColorBrush(rgb))
			{
				return m_map.at(rgb);
			}
		}

		XeASSERT(false);

		// Create brush failed, return first brush in map.
		auto it = m_map.begin();
		XeASSERT(it != m_map.end());	// No brushes in map - returning null.
		return it != m_map.end() ? it->second : nullptr;
	}

protected:
	bool _CreateSolidColorBrush(uint32_t rgb)
	{
		XeD2D1_COLOR_F col;
		col.SetFromRGB(rgb, false);
		ID2D1SolidColorBrush* pBrush = nullptr;
		HRESULT hr = m_pRT->CreateSolidColorBrush(col, &pBrush);
		XeASSERT(hr == S_OK);
		m_map[rgb] = pBrush;
		return hr == S_OK;
	}
};

// Base helper class for window that uses Direct2D for painting.
export class CXeD2DRenderContext
{
#pragma region Class_data
protected:
	CXeUIcolorsIF* m_xeUI = nullptr;

	Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> m_renderTargetClientDC;

	XeRRectMap m_rr;

private:
	std::unique_ptr<XeD2DBrushMap> m_brushes;

public:
	// Current render target we are using for painting (can be used by external classes to paint - e.g. D2DToolbar)
	ID2D1RenderTarget* m_pCurrentRT = nullptr;
#pragma endregion Class_data

#pragma region Create
public:
	CXeD2DRenderContext(CXeUIcolorsIF* pUIcolors)
		: m_xeUI(pUIcolors)
	{
		// Note - derived class can create more fonts at this point
		// (to use for calculating the window size needed).
	}
	virtual ~CXeD2DRenderContext() {}
#pragma endregion Create

#pragma region Create_UI_resources
public:
	ID2D1SolidColorBrush* GetBrush(CID key)
	{
		XeASSERT(m_brushes.get());	// m_brushes is only valid during _Paint call
		if (!m_brushes.get())
		{
			return nullptr;
		}
		return m_brushes->GetOrCreate(key);
	}

	ID2D1SolidColorBrush* GetBrush(uint32_t rgb)
	{
		XeASSERT(m_brushes.get());	// m_brushes is only valid during _Paint call
		if (!m_brushes.get())
		{
			return nullptr;
		}
		return m_brushes->GetOrCreate(rgb);
	}

	static Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> GetD2SolidBrush(ID2D1RenderTarget* pRT, D2D1_COLOR_F cc)
	//static CComPtr<ID2D1SolidColorBrush> GetD2SolidBrush(ID2D1RenderTarget* pRT, D2D1_COLOR_F cc)
	{
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> b = 0;
		//CComPtr<ID2D1SolidColorBrush> b = 0;
		pRT->CreateSolidColorBrush(cc, &b);
		return b;
	}
#pragma endregion Create_UI_resources

#pragma region UI_Drawing
protected:
	void _PaintU(ID2D1RenderTarget* pRT, const CRect& rc)
	{
		if (!rc.IsRectEmpty())
		{
			_OnPrepareForPainting(pRT);
			_PaintF(pRT, RectFfromRect(rc));
			_OnEndPainting();
		}
	}

	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) = 0;

	void _OnPrepareForPainting(ID2D1RenderTarget* pRT)
	{
		m_brushes = std::make_unique<XeD2DBrushMap>(m_xeUI, pRT);	// Create brushes each time painting
		m_pCurrentRT = pRT;
	}

	void _OnEndPainting()
	{
		m_brushes.reset();
		m_pCurrentRT = nullptr;
	}
#pragma endregion UI_Drawing

#pragma region UI_Drawing_Helpers
public:
	void _DrawButtons(ID2D1RenderTarget* pRT, const std::vector<int>& keys)
	{
		for (int key : keys)
		{
			_DrawButton(pRT, key);
		}
	}
	void _DrawButton(ID2D1RenderTarget* pRT, int rect_key, bool isAct = false)
	{
		const XeD2Rect& r = m_rr.Get(rect_key);
		_DrawButton(pRT, r, r.m_text, isAct, r.m_isEnabled);
	}
	void _DrawButton(ID2D1RenderTarget* pRT, int rect_key, const std::wstring& txt, bool isAct, bool isEna = true)
	{
		_DrawButton(pRT, m_rr.Get(rect_key), txt, isAct, isEna);
	}
	void _DrawButton(ID2D1RenderTarget* pRT, const D2D1_ROUNDED_RECT& btn_rect, const std::wstring& txt,
		bool isAct, bool isEna = true, bool isDown = false)
	{
		CID bgCID = isDown ? CID::CtrlTxt : CID::CtrlBg;
		CID fgCID = isDown ? CID::CtrlBg : isEna ? CID::CtrlTxt : CID::CtrlBtnBorder;
		pRT->FillRoundedRectangle(btn_rect, GetBrush(bgCID));
		ID2D1SolidColorBrush* pBrdBrsh = GetBrush(isAct ? CID::CtrlBtnDefBorder : CID::CtrlBtnBorder);
		pRT->DrawRoundedRectangle(btn_rect, pBrdBrsh, 1);
		IDWriteTextFormat* pUIfont = m_xeUI->D2D_GetFont(EXE_FONT::eUI_Font);
		pUIfont->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		pUIfont->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		pRT->DrawText(txt.c_str(), (UINT32)txt.size(), pUIfont, btn_rect.rect, GetBrush(fgCID));
	}

	void _DrawTexts(ID2D1RenderTarget* pRT, const std::vector<int>& keys)
	{
		for (int key : keys)
		{
			const XeD2Rect& r = m_rr.Get(key);
			_DrawText2(pRT, r.m_text, r, r.m_textColorId, r.m_fontId, r.m_alignment, r.m_valignment);
		}
	}
	void _DrawText(ID2D1RenderTarget* pRT, const std::wstring& txt, int rect_key, CID colorId = CID::CtrlTxt,
		EXE_FONT eFont = EXE_FONT::eUI_Font, DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT valignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
	{
		const D2D1_ROUNDED_RECT& rect = m_rr.Get(rect_key);
		_DrawText2(pRT, txt, rect, colorId, eFont, alignment, valignment);
	}
	void _DrawText2(ID2D1RenderTarget* pRT, const std::wstring& txt, const D2D1_ROUNDED_RECT& rect, CID colorId = CID::CtrlTxt,
		EXE_FONT eFont = EXE_FONT::eUI_Font, DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT valignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
	{
		_CreateAndDrawTextLayout(pRT, txt, rect.rect, colorId, eFont, alignment, valignment);
	}

	void _DrawTextBetweenBoxes(ID2D1RenderTarget* pRT, const std::wstring& s, int rect1_key, int rect2_key, CID colorId = CID::CtrlTxt,
		EXE_FONT eFont = EXE_FONT::eUI_Font, DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT valignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
	{
		_DrawTextBetweenBoxes(pRT, s, m_rr.Get(rect1_key), m_rr.Get(rect2_key), colorId, eFont, alignment, valignment);
	}
	void _DrawTextBetweenBoxes(ID2D1RenderTarget* pRT, const std::wstring& s,
		const D2D1_ROUNDED_RECT& b1, const D2D1_ROUNDED_RECT& b2, CID colorId = CID::CtrlTxt,
		EXE_FONT eFont = EXE_FONT::eUI_Font, DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT valignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
	{
		D2D1_RECT_F rc = SetR(b1.rect.right, b1.rect.top,
				(b2.rect.left - b1.rect.right), (b1.rect.bottom - b1.rect.top));
		_CreateAndDrawTextLayout(pRT, s, rc, colorId, eFont, alignment, valignment);
	}

	[[nodiscard]] Microsoft::WRL::ComPtr<IDWriteTextLayout> _CreateTextLayout(
			const std::wstring& txt, const D2D1_SIZE_F& sizeBox, EXE_FONT font = EXE_FONT::eUI_Font,
			DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING,
			DWRITE_PARAGRAPH_ALIGNMENT paraalign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
			DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP)
	{
		Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
		IDWriteTextFormat* pUIfont = m_xeUI->D2D_GetFont(font);
		HRESULT hr = m_xeUI->D2D_GetWriteFactory()->CreateTextLayout(txt.c_str(), (UINT32)txt.size(),
			pUIfont, sizeBox.width, sizeBox.height, textLayout.GetAddressOf());
		XeASSERT(hr == S_OK);
		if (textLayout.Get())
		{
			textLayout->SetTextAlignment(alignment);
			textLayout->SetParagraphAlignment(paraalign);
			textLayout->SetWordWrapping(wrapping);
		}
		return textLayout;
	}

	void _CreateAndDrawTextLayout(ID2D1RenderTarget* pRT, const std::wstring& txt, int x, int y, int cx, int cy,
			CID fg, EXE_FONT font = EXE_FONT::eUI_Font,
			DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING,
			DWRITE_PARAGRAPH_ALIGNMENT paraalign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
			DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP)
	{
		D2D1_RECT_F rectF((float)x, (float)y, (float)(x + cx), (float)(y + cy));
		_CreateAndDrawTextLayout(pRT, txt, rectF, fg, font, alignment, paraalign, wrapping);
	}

	void _CreateAndDrawTextLayout(ID2D1RenderTarget* pRT, const std::wstring& txt, const CRect& rect,
			CID fg, EXE_FONT font = EXE_FONT::eUI_Font,
			DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING,
			DWRITE_PARAGRAPH_ALIGNMENT paraalign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
			DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP)
	{
		D2D1_RECT_F rectF = RectFfromRect(rect);
		_CreateAndDrawTextLayout(pRT, txt, rectF, fg, font, alignment, paraalign, wrapping);
	}

	void _CreateAndDrawTextLayout(ID2D1RenderTarget* pRT, const std::wstring& txt, const D2D1_RECT_F& rect,
			CID fg, EXE_FONT font = EXE_FONT::eUI_Font,
			DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING,
			DWRITE_PARAGRAPH_ALIGNMENT paraalign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
			DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP)
	{
		D2D1_SIZE_F sizeBox{ WidthOf(rect), HeightOf(rect) };
		Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout = _CreateTextLayout(txt, sizeBox, font,
				alignment, paraalign, wrapping);
		_DrawTextLayout(pRT, textLayout.Get(), { (float)rect.left, (float)rect.top }, rect, GetBrush(fg));
	}

	void _CreateAndDrawTextLayout(ID2D1RenderTarget* pRT, const std::wstring& txt, const CRect& rect,
			DWORD rgbTxt, EXE_FONT font = EXE_FONT::eUI_Font,
			DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING,
			DWRITE_PARAGRAPH_ALIGNMENT paraalign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
			DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP)
	{
		D2D1_RECT_F rectF = RectFfromRect(rect);
		XeD2D1_COLOR_F clrTxt;
		clrTxt.SetFromRGB(rgbTxt, false);
		_CreateAndDrawTextLayout(pRT, txt, rectF, clrTxt, font, alignment, paraalign, wrapping);
	}

	void _CreateAndDrawTextLayout(ID2D1RenderTarget* pRT, const std::wstring& txt, const D2D1_RECT_F& rect,
			XeD2D1_COLOR_F clrTxt, EXE_FONT font = EXE_FONT::eUI_Font,
			DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING,
			DWRITE_PARAGRAPH_ALIGNMENT paraalign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
			DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP)
	{
		D2D1_SIZE_F sizeBox{ WidthOf(rect), HeightOf(rect) };
		Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout = _CreateTextLayout(txt, sizeBox, font,
			alignment, paraalign, wrapping);
		_DrawTextLayout(pRT, textLayout.Get(), { (float)rect.left, (float)rect.top }, rect, clrTxt);
	}

	void _CreateAndDrawTextLayout(ID2D1RenderTarget* pRT, const std::wstring& txt, const D2D1_RECT_F& rect,
			DWORD rgbTxt, EXE_FONT font = EXE_FONT::eUI_Font,
			DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING,
			DWRITE_PARAGRAPH_ALIGNMENT paraalign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
			DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP)
	{
		D2D1_SIZE_F sizeBox{ WidthOf(rect), HeightOf(rect) };
		Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout = _CreateTextLayout(txt, sizeBox, font,
			alignment, paraalign, wrapping);
		_DrawTextLayout(pRT, textLayout.Get(), { (float)rect.left, (float)rect.top }, rect, rgbTxt);
	}

	void _DrawTextLayout(ID2D1RenderTarget* pRT, IDWriteTextLayout* pLayout, const D2D1_POINT_2F& pt,
			const D2D1_RECT_F& rcClipping, ID2D1SolidColorBrush* pBrush)
	{
		XeASSERT(pLayout);
		if (pLayout)
		{
			pRT->PushAxisAlignedClip(rcClipping, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			pRT->DrawTextLayout(pt, pLayout, pBrush);
			pRT->PopAxisAlignedClip();
		}
	}

	void _DrawTextLayout(ID2D1RenderTarget* pRT, IDWriteTextLayout* pLayout, const D2D1_POINT_2F& pt,
			const D2D1_RECT_F& rcClipping, XeD2D1_COLOR_F clrTxt)
	{
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> colorBrush = GetD2SolidBrush(pRT, clrTxt);
		_DrawTextLayout(pRT, pLayout, pt, rcClipping, colorBrush.Get());
	}

	void _DrawTextLayout(ID2D1RenderTarget* pRT, IDWriteTextLayout* pLayout, const D2D1_POINT_2F& pt,
			const D2D1_RECT_F& rcClipping, DWORD rgbTxt)
	{
		XeD2D1_COLOR_F clrTxt;
		clrTxt.SetFromRGB(rgbTxt, false);
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> colorBrush = GetD2SolidBrush(pRT, clrTxt);
		_DrawTextLayout(pRT, pLayout, pt, rcClipping, colorBrush.Get());
	}

	void _DrawIconAndFilename(ID2D1RenderTarget* pRT, const std::wstring& filename,
			const CRect& rc, EXE_FONT eFont, COLORREF rgbTxt, PID pid, bool isSmallIcon,
			bool isIconSpaceReserved, const CRect& rcProgress)
	{
		if (!rcProgress.IsRectEmpty())
		{
			pRT->FillRectangle(RectFfromRect(rcProgress), GetBrush(CID::LogFilterHdrBg));
		}
		bool hasIcon = pid != PID::None;
		CRect rcTxt = rc;
		CRect rcIcn = rc;
		if (hasIcon || isIconSpaceReserved)
		{
			int cxyIcn = isSmallIcon ? 16 : 24;
			rcTxt.left += isSmallIcon ? 18 : 28;
			if (rcIcn.Height() > cxyIcn)
			{
				rcIcn.top = rcIcn.top + (int)((double)(rcIcn.Height() - cxyIcn) / 2 + 0.5);
				rcIcn.bottom = rcIcn.top + cxyIcn;
			}
			rcIcn.right = rcIcn.left + cxyIcn;
		}
		if (hasIcon)
		{
			_DrawButtonImage(pRT, pid, RectFfromRect(rcIcn), true, true, false);
		}

		_CreateAndDrawTextLayout(pRT, filename, rcTxt, rgbTxt, eFont);
	}

	void _DrawRoundedRectangle(ID2D1RenderTarget* pRT, int rect_key, CID colorId, float strokeWidth = 1.0f)
	{
		_DrawRoundedRectangle(pRT, m_rr.Get(rect_key), colorId, strokeWidth);
	}
	void _DrawRoundedRectangle(ID2D1RenderTarget* pRT, const D2D1_ROUNDED_RECT& rect, CID colorId, float strokeWidth = 1.0f)
	{
		pRT->DrawRoundedRectangle(rect, GetBrush(colorId), strokeWidth);
	}

	void _FillRoundedRectangle(ID2D1RenderTarget* pRT, int rect_key, CID colorId)
	{
		_FillRoundedRectangle(pRT, m_rr.Get(rect_key), colorId);
	}
	void _FillRoundedRectangle(ID2D1RenderTarget* pRT, const D2D1_ROUNDED_RECT& rect, CID colorId)
	{
		pRT->FillRoundedRectangle(rect, GetBrush(colorId));
	}

	void _DrawColorRect(ID2D1RenderTarget* pRT, const D2D1_COLOR_F& clr, const D2D1_RECT_F& rect)
	{
		auto b2 = GetD2SolidBrush(pRT, clr);
		pRT->FillRectangle(rect, b2.Get());
	}

	void _DrawFocusRect(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rect,
			CID colorId = CID::CtrlTxtDis, float strokeWidth = 1.0f)
	{
		ID2D1StrokeStyle* pStrokeStyle = m_xeUI->D2D_GetFocusStrokeStyle();
		XeASSERT(pStrokeStyle);
		if (pStrokeStyle)
		{
			pRT->DrawRectangle(rect, GetBrush(colorId), strokeWidth, pStrokeStyle);
		}
	}

	void _DrawCheckmark(ID2D1RenderTarget* pRT, int rect_key, CID colorId = CID::CtrlTxt)
	{
		_DrawCheckmark(pRT, m_rr.Get(rect_key), colorId);
	}
	void _DrawCheckmark(ID2D1RenderTarget* pRT, const D2D1_ROUNDED_RECT& box, CID colorId = CID::CtrlTxt)
	{
		_DrawCheckmark(pRT, box.rect, colorId);
	}
	void _DrawCheckmark(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rect, CID colorId = CID::CtrlTxt)
	{
		float cy = rect.bottom - rect.top;
		float x1 = rect.left + cy * 0.25f, y1 = rect.top + cy * 0.35f;
		float x2 = rect.left + cy * 0.5f, y2 = rect.top + cy * 0.75f;
		float x3 = rect.left + cy * 0.75f, y3 = rect.top + cy * 0.2f;
		pRT->DrawLine({ x1, y1 }, { x2, y2 }, GetBrush(colorId), 2);
		pRT->DrawLine({ x2, y2 }, { x3, y3 }, GetBrush(colorId), 2);
	}

	void _DrawCheckBox(ID2D1RenderTarget* pRT, D2D1_RECT_F rcCB, DWORD button_state,
		CID lineClr, CID interFillClr, bool isHovering, bool isDrawRectangle = true)
	{
		if (isDrawRectangle)
		{
			pRT->DrawRectangle(rcCB, GetBrush(lineClr), isHovering ? 2.0f : 1.0f);
		}
		if (button_state == BST_CHECKED)
		{
			_DrawCheckmark(pRT, rcCB, lineClr);
		}
		else if (button_state == BST_INDETERMINATE)
		{
			float cxy = 2.0f;
			rcCB.left += cxy; rcCB.right -= cxy; rcCB.top += cxy; rcCB.bottom -= cxy;
			pRT->FillRectangle(rcCB, GetBrush(interFillClr));
		}
	}

	void _DrawRadioButton(ID2D1RenderTarget* pRT, D2D1_RECT_F rcCB, DWORD button_state,
		CID lineClr, bool isHovering)
	{
		D2D1_ELLIPSE elli;
		elli.point.x = rcCB.left + ((rcCB.right - rcCB.left) / 2);
		elli.point.y = rcCB.top + ((rcCB.bottom - rcCB.top) / 2);
		elli.radiusX = elli.radiusY = (rcCB.right - rcCB.left) / 2;
		pRT->DrawEllipse(elli, GetBrush(lineClr), isHovering ? 2.0f : 1.0f);
		if (button_state == BST_CHECKED)
		{
			elli.radiusX = elli.radiusY = (rcCB.right - rcCB.left) / 3;
			pRT->FillEllipse(elli, GetBrush(lineClr));
		}
	}

	void _DrawTriangle(ID2D1RenderTarget* pRT, int rect_key, bool isUp, CID colorId)
	{
		_DrawTriangle(pRT, m_rr.Get(rect_key).rect, isUp, colorId);
	}
	void _DrawTriangle(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rect, bool isUp, CID colorId)
	{
		ID2D1SolidColorBrush* brush = GetBrush(colorId);
		float x_mid = rect.left + (rect.right - rect.left) / 2;
		if (isUp)
		{
			pRT->DrawLine({ rect.left, rect.top }, { rect.right, rect.top }, brush, 1);
			pRT->DrawLine({ rect.right, rect.top }, { x_mid, rect.bottom }, brush, 1);
			pRT->DrawLine({ x_mid, rect.bottom }, { rect.left, rect.top }, brush, 1);
		}
		else
		{
			pRT->DrawLine({ rect.left, rect.bottom }, { rect.right, rect.bottom }, brush, 1);
			pRT->DrawLine({ rect.right, rect.bottom }, { x_mid, rect.top }, brush, 1);
			pRT->DrawLine({ x_mid, rect.top }, { rect.left, rect.bottom }, brush, 1);
		}
	}

	Microsoft::WRL::ComPtr<ID2D1PathGeometry> _MakeTriangle(const D2D1_RECT_F& rect, bool isUp) const
	{
		float x_mid = rect.left + (rect.right - rect.left) / 2;
		if (isUp)
		{
			return _MakeTriangle({ rect.left, rect.bottom }, { rect.right, rect.bottom }, { x_mid, rect.top });
		}
		else
		{
			return _MakeTriangle({ rect.left, rect.top }, { rect.right, rect.top }, { x_mid, rect.bottom });
		}
	}

	Microsoft::WRL::ComPtr<ID2D1PathGeometry> _MakeTriangle(const D2D1_POINT_2F& pt1, const D2D1_POINT_2F& pt2, const D2D1_POINT_2F& pt3) const
	{
		Microsoft::WRL::ComPtr<ID2D1GeometrySink> pSink;
		HRESULT hr = S_OK;
		Microsoft::WRL::ComPtr<ID2D1PathGeometry> pPathGeometry;
		// Create a path geometry.
		if (SUCCEEDED(hr))
		{
			hr = m_xeUI->D2D_GetFactory()->CreatePathGeometry(&pPathGeometry);

			if (SUCCEEDED(hr))
			{
				// Write to the path geometry using the geometry sink.
				hr = pPathGeometry->Open(&pSink);

				if (SUCCEEDED(hr))
				{
					pSink->BeginFigure(pt1, D2D1_FIGURE_BEGIN_FILLED);
					pSink->AddLine(pt2);
					pSink->AddLine(pt3);
					pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
					hr = pSink->Close();
				}
			}
		}
		return pPathGeometry;
	}

	Microsoft::WRL::ComPtr<ID2D1PathGeometry> _MakeRectGeometry(const D2D1_RECT_F& rc) const
	{
		Microsoft::WRL::ComPtr<ID2D1GeometrySink> pSink;
		Microsoft::WRL::ComPtr<ID2D1PathGeometry> pPathGeometry;
		HRESULT hr = m_xeUI->D2D_GetFactory()->CreatePathGeometry(&pPathGeometry);
		XeASSERT(hr == S_OK);
		if (SUCCEEDED(hr))
		{
			// Write to the path geometry using the geometry sink.
			hr = pPathGeometry->Open(&pSink);
			XeASSERT(hr == S_OK);
			if (SUCCEEDED(hr))
			{
				pSink->BeginFigure({ rc.left, rc.top }, D2D1_FIGURE_BEGIN_FILLED);
				pSink->AddLine({ rc.right, rc.top });
				pSink->AddLine({ rc.right, rc.bottom });
				pSink->AddLine({ rc.left, rc.bottom });
				pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
				hr = pSink->Close();
			}
		}
		return pPathGeometry;
	}

	void _DrawFilledTriangle(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rect, bool isUp, CID colorId)
	{
		Microsoft::WRL::ComPtr<ID2D1PathGeometry> tri = _MakeTriangle(rect, isUp);
		pRT->DrawGeometry(tri.Get(), GetBrush(colorId), 1.0f);
		pRT->FillGeometry(tri.Get(), GetBrush(colorId));
	}

	void _DrawSubMenuArrow(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rcBtn, CID fgColor)
	{
		float cyTri = (float)m_xeUI->GetValue(UIV::cySubMenuArrow);
		float cxTri = cyTri / 2.0f;
		float cyTriMargin = (HeightOf(rcBtn) - cyTri) / 2.0f;
		float x1 = rcBtn.left, x2 = x1 + (cyTri / 2.0f);
		float y1 = rcBtn.top + cyTriMargin, y2 = y1 + cyTri;
		float y3 = y1 + (cyTri / 2.0f);

		Microsoft::WRL::ComPtr<ID2D1PathGeometry> tri = _MakeTriangle(D2D1_POINT_2F(x1, y1), D2D1_POINT_2F(x2, y3), D2D1_POINT_2F(x1, y2));
		pRT->DrawGeometry(tri.Get(), GetBrush(fgColor), 1.0f);
		pRT->FillGeometry(tri.Get(), GetBrush(fgColor));
	}

	void _DrawComboBoxButton(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rcBtn, CID fgColor, CID bgColor, bool isFillBg = true)
	{
		if (isFillBg)
		{
			pRT->FillRectangle(rcBtn, GetBrush(bgColor));
		}

		float cxTri = (float)m_xeUI->GetValue(UIV::cxComboBoxButtonArrow);
		float cyTri = cxTri / 2.0f, cxTriMargin = (WidthOf(rcBtn) - cxTri) / 2.0f;
		float cyTriMargin = (HeightOf(rcBtn) - cyTri) / 2.0f;
		float x1 = rcBtn.right - (cxTriMargin + cxTri), x2 = rcBtn.right - cxTriMargin;
		float x3 = x1 + (cxTri / 2.0f);
		float y1 = rcBtn.top + cyTriMargin, y2 = rcBtn.top + cyTriMargin + cyTri;

		Microsoft::WRL::ComPtr<ID2D1PathGeometry> tri = _MakeTriangle(D2D1_POINT_2F(x1, y1), D2D1_POINT_2F(x2, y1), D2D1_POINT_2F(x3, y2));
		pRT->DrawGeometry(tri.Get(), GetBrush(fgColor), 1.0f);
		pRT->FillGeometry(tri.Get(), GetBrush(fgColor));
	}

	void _DrawVsplitIcon(ID2D1RenderTarget* pRT, D2D1_RECT_F rc2, CID bg1, CID bg2, CID fg)
	{
		pRT->FillRectangle(rc2, GetBrush(bg1));
		rc2.top += 2.0f;
		rc2.bottom -= 2.0f;
		pRT->FillRectangle(rc2, GetBrush(bg2));

		D2D1_POINT_2F pt[3];
		float x_c = rc2.left + WidthOf(rc2) / 2.0f;
		pt[0].x = x_c;
		pt[0].y = rc2.top + 1.0f;
		pt[1].x = pt[0].x - 3.0f;
		pt[1].y = pt[0].y + 4.0f;
		pt[2].x = pt[1].x + 7.0f;
		pt[2].y = pt[1].y;
		Microsoft::WRL::ComPtr<ID2D1PathGeometry> tri = _MakeTriangle(pt[0], pt[1], pt[2]);
		pRT->DrawGeometry(tri.Get(), GetBrush(fg), 1.0f);
		pRT->FillGeometry(tri.Get(), GetBrush(fg));

		float x1 = rc2.left + 2.0f, x2 = rc2.right - 2.0f, y = rc2.top + 6.0f;
		pRT->DrawLine(D2D1_POINT_2F(x1, y), D2D1_POINT_2F(x2, y), GetBrush(fg));

		y += 2;
		pRT->DrawLine(D2D1_POINT_2F(x1, y), D2D1_POINT_2F(x2, y), GetBrush(fg));

		D2D1_POINT_2F pb[3];
		pb[0].x = x_c - 2.0f;
		pb[0].y = y + 2.0f;
		pb[1].x = pb[0].x + 5.0f;
		pb[1].y = pb[0].y;
		pb[2].x = x_c;
		pb[2].y = pb[0].y + 3.0f;
		Microsoft::WRL::ComPtr<ID2D1PathGeometry> tri2 = _MakeTriangle(pb[0], pb[1], pb[2]);
		pRT->DrawGeometry(tri2.Get(), GetBrush(fg), 1.0f);
		pRT->FillGeometry(tri2.Get(), GetBrush(fg));
	}

	enum class BtnTp { left, right, up, down };
	void _DrawScrollbarButton(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rcBtn, CID fgColor, BtnTp tp, CID bgColor = CID::Invalid)
	{
		bool isHorz = tp == BtnTp::left || tp == BtnTp::right;
		float cxTri = isHorz ? 5.0f : 9.0f, cyTri = isHorz ? 9.0f : 5.0f;
		float cyTriMargin = (HeightOf(rcBtn) - cyTri) / 2.0f;
		float cxTriMargin = (WidthOf(rcBtn) - cxTri) / 2.0f;
		float x1 = rcBtn.left + cxTriMargin, x2 = x1 + cxTri;
		float y1 = rcBtn.top + (HeightOf(rcBtn) / 2.0f), y2 = rcBtn.top + cyTriMargin;
		float x3 = x2, y3 = y2 + cyTri;
		if (tp == BtnTp::right)
		{
			std::swap(x1, x2);
			x3 = x2;
		}
		else if (!isHorz)
		{
			x1 = (rcBtn.left + WidthOf(rcBtn)) / 2.0f;
			x2 = rcBtn.left + cxTriMargin;
			y1 = rcBtn.top + cyTriMargin, y2 = y1 + cyTri;
			x3 = x2 + cxTri, y3 = y2;
			if (tp == BtnTp::down)
			{
				std::swap(y1, y2);
				y3 = y2;
			}
		}

		if (bgColor != CID::Invalid)
		{
			pRT->FillRectangle(rcBtn, GetBrush(bgColor));
		}
		Microsoft::WRL::ComPtr<ID2D1PathGeometry> tri = _MakeTriangle(D2D1_POINT_2F(x1, y1), D2D1_POINT_2F(x2, y2), D2D1_POINT_2F(x3, y3));
		pRT->DrawGeometry(tri.Get(), GetBrush(fgColor), 1.0f);
		pRT->FillGeometry(tri.Get(), GetBrush(fgColor));
	}

	D2D1_MATRIX_5X4_F _GetHotMatrix() const
	{
		D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
			1.1f, 0, 0, 0,
			0, 1.1f, 0, 0,
			0, 0, 1.1f, 0,
			0, 0, 0, 1,
			0, 0, 0, 0);
		return matrix;
	}

	// Draw image inside the supplied box.
	// pRT				Render context
	// imagePID			PID of image to draw
	// rcBox			Box to draw image inside of.
	// isImgOnLeftSide	Set image position on the left side of the box else center the image inside the box.
	// isEnabled		(false) Draw image in "disabled" style (monochrome)
	// isHovering		Draw image "hot" style.
	// return image rect.
	D2D1_RECT_F _DrawButtonImage(ID2D1RenderTarget* pRT, PID imagePID, const D2D1_RECT_F& rcBox,
		bool isImgOnLeftSide, bool isEnabled, bool isHovering)
	{
		IWICFormatConverter* pImg = m_xeUI->D2D_GetImage(imagePID);
		return _DrawImage(pRT, pImg, rcBox, isImgOnLeftSide, isEnabled, isHovering);
	}

	D2D1_RECT_F _DrawImage(ID2D1RenderTarget* pRT, IWICFormatConverter* pImg, const D2D1_RECT_F& rcBox,
		bool isImgOnLeftSide, bool isEnabled, bool isHovering)
	{
		D2D1_RECT_F rcImg{ rcBox.left, rcBox.top, rcBox.left, rcBox.bottom }; // cx = 0.
		Microsoft::WRL::ComPtr<ID2D1Bitmap> pD2DBitmap;
		HRESULT hr = pRT->CreateBitmapFromWicBitmap(pImg, NULL, pD2DBitmap.GetAddressOf());
		if (!(pImg && SUCCEEDED(hr)))
		{
			return rcImg;
		}
		auto img_size = pD2DBitmap->GetSize();
		if (img_size.width > 0 && img_size.height > 0)
		{
			ID2D1DeviceContext* pDeviceContext = nullptr;
			pRT->QueryInterface(__uuidof(ID2D1DeviceContext), (void**)&pDeviceContext);
			XeASSERT(pDeviceContext);

			Microsoft::WRL::ComPtr<ID2D1Effect> scaleEffect;
			pDeviceContext->CreateEffect(CLSID_D2D12DAffineTransform, scaleEffect.GetAddressOf());

			Microsoft::WRL::ComPtr<ID2D1Effect> effect;
			bool isEffectNeeded = !isEnabled || isHovering;
			if (isEffectNeeded)
			{
				if (isHovering)	// Draw image 'hot'.
				{
					pDeviceContext->CreateEffect(CLSID_D2D1ColorMatrix, effect.GetAddressOf());
					effect->SetInput(0, pD2DBitmap.Get());
					effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, _GetHotMatrix());
				}
				else if (!isEnabled)	// Draw image monochrome.
				{
					pDeviceContext->CreateEffect(CLSID_D2D1Saturation, &effect);
					effect->SetInput(0, pD2DBitmap.Get());
					effect->SetValue(D2D1_SATURATION_PROP_SATURATION, 0.0f);
				}
				scaleEffect->SetInputEffect(0, effect.Get());
			}
			else
			{
				scaleEffect->SetInput(0, pD2DBitmap.Get());
			}

			// Calculate the image box - size and position (image will be scaled to fit)
			rcImg = CalculateImageSizeAndPosition(isImgOnLeftSide, rcBox, img_size);

			// Scale image to fit the image box.
			D2D_SIZE_F img_scale = CalculateImageScaleForBox(rcImg, img_size);
			D2D1::Matrix3x2F mtx = D2D1::Matrix3x2F::Scale(img_scale, D2D1::Point2F(0, 0));
			scaleEffect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, mtx);

			D2D1_POINT_2F targetOffset(rcImg.left, rcImg.top);
			pDeviceContext->DrawImage(scaleEffect.Get(), targetOffset);
			pDeviceContext->Release();
		}
		return rcImg;
	}

	Microsoft::WRL::ComPtr<IWICBitmap> _CreateFromHicon(HICON hIcon) const
	{
		Microsoft::WRL::ComPtr<IWICBitmap> d2d_icon_bitmap;
		IWICImagingFactory* pWICFactory = m_xeUI->D2D_GetWICFactory();
		HRESULT hr = pWICFactory->CreateBitmapFromHICON(hIcon, d2d_icon_bitmap.GetAddressOf());
		XeASSERT(hr == S_OK);
		return d2d_icon_bitmap;
	}

	void _DrawIcon(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rcIcon, IWICBitmap* pWICBitmap) const
	{
		XeASSERT(pWICBitmap);
		if (!pWICBitmap)
		{
			return;
		}
		D2D1_BITMAP_PROPERTIES bitmapProps{};
		bitmapProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		bitmapProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
		bitmapProps.dpiX = 96;
		bitmapProps.dpiY = 96;
		Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
		HRESULT hr = pRT->CreateBitmapFromWicBitmap(pWICBitmap, bitmapProps, bitmap.GetAddressOf());
		XeASSERT(hr == S_OK);
		if (SUCCEEDED(hr))
		{
			pRT->DrawBitmap(bitmap.Get(), rcIcon);
		}
	}

	void _DrawCloseButton(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rcCloseBtn, bool isMouseOverCloseBtn, CID bgColor)
	{
		CID fgColor = isMouseOverCloseBtn ? bgColor : CID::CtrlTxt;
		bgColor = isMouseOverCloseBtn ? CID::CtrlTxt : bgColor;
		pRT->FillRectangle(rcCloseBtn, GetBrush(bgColor));
		float cxClsBtn = 9.0f, cyClsBtn = 8.0f;
		float cxMrg = (WidthOf(rcCloseBtn) - cxClsBtn) / 2.0f;
		float cyMrg = (HeightOf(rcCloseBtn) - cyClsBtn) / 2.0f;
		float xO = rcCloseBtn.left + cxMrg, yO = rcCloseBtn.top + cyMrg;
		float x1 = xO, y1 = yO, x2 = x1 + cxClsBtn - 1.0f, y2 = y1 + cyClsBtn;
		pRT->DrawLine({ x1,y1 }, { x2,y2 }, GetBrush(fgColor), 2.0f);
		x1 = xO, y1 = yO + cyClsBtn, x2 = x1 + cxClsBtn - 1.0f, y2 = yO;
		pRT->DrawLine({ x1,y1 }, { x2,y2 }, GetBrush(fgColor), 2.0f);
	}

	void _DrawChevronButton(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rcBtn, bool isMouseOverBtn, CID bgColor)
	{
		CID fgColor = isMouseOverBtn ? bgColor : CID::CtrlTxt;
		bgColor = isMouseOverBtn ? CID::CtrlTxt : bgColor;
		pRT->FillRectangle(rcBtn, GetBrush(bgColor));
		float cxCvrn = 11.0f, cyCvrn = 12.0f;
		float cxMrg = (WidthOf(rcBtn) - cxCvrn) / 2.0f;
		float cyMrg = (HeightOf(rcBtn) - cyCvrn) / 2.0f;
		float xO = rcBtn.left + cxMrg, yO = rcBtn.top + cyMrg;
		int i = 2;
		ID2D1SolidColorBrush* pBrush = GetBrush(fgColor);
		do {
			float x1 = xO, y1 = yO, x2 = x1 + 5.0f, y2 = y1 + 5.0f, x3 = x2 + 6.0f, y3 = y2 - 6.0f;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			pRT->DrawLine({ x2, y2 }, { x3, y3 }, pBrush);
			x1 = xO + 1, y1 = yO, x2 = x1 + 4.0f, y2 = y1 + 4.0f, x3 = x2 + 5.0f, y3 = y2 - 5.0f;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			pRT->DrawLine({ x2, y2 }, { x3, y3 }, pBrush);
			yO += 6;
		} while (--i);
	}

	void _DrawPinButton(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rcPinBtn, bool isPinned, bool isMouseOverPinBtn, CID bgColor)
	{
		CID fgColor = isMouseOverPinBtn ? bgColor : CID::CtrlTxt;
		bgColor = isMouseOverPinBtn ? CID::CtrlTxt : bgColor;
		pRT->FillRectangle(rcPinBtn, GetBrush(bgColor));
		float cxClsBtn = isPinned ? 7.0f : 8.0f, cyClsBtn = isPinned ? 8.0f : 7.0f;
		float cxMrg = (WidthOf(rcPinBtn) - cxClsBtn) / 2.0f;
		float cyMrg = (HeightOf(rcPinBtn) - cyClsBtn) / 2.0f;
		float xO = rcPinBtn.left + cxMrg, yO = rcPinBtn.top + cyMrg;
		ID2D1SolidColorBrush* pBrush = GetBrush(fgColor);
		if (isPinned)
		{
			float x1 = xO + 1.0f, y1 = yO, x2 = x1, y2 = y1 + 4.0f;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			x1 = xO; x2 = x1 + 7.0f;
			y1 = y2;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			x1 = xO + 1.0f, y1 = yO, x2 = x1 + 5.0f, y2 = y1;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			x1 = xO + 4.0f, y1 = yO, x2 = x1, y2 = y1 + 5.0f;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			x1 = xO + 5.0f, y1 = yO, x2 = x1, y2 = y1 + 5.0f;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			x1 = xO + 3.0f, y1 = yO + 4.0f, x2 = x1, y2 = y1 + 4.0f;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
		}
		else
		{
			float x1 = xO, y1 = yO + 3.0f, x2 = x1 + 4.0f, y2 = y1;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			x1 = xO + 3; y1 = yO; x2 = x1; y2 = y1 + 7;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			x1 = xO + 3, y1 = yO + 1, x2 = x1 + 5, y2 = y1;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			x1 = xO + 7, y1 = yO + 1, x2 = x1, y2 = y1 + 5;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			x1 = xO + 4, y1 = yO + 4, x2 = x1 + 3, y2 = y1;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
			x1 = xO + 4, y1 = yO + 5, x2 = x1 + 3, y2 = y1;
			pRT->DrawLine({ x1, y1 }, { x2, y2 }, pBrush);
		}
	}

	/// <summary>
	/// Draw "3D" border. 
	/// Left edge and top edge are drawn using tlColor, right edge and bottom edge are drawn using brColor.
	/// If bgColor is valid to rect is filled using that color.
	/// </summary>
	void _Draw3Dborder(ID2D1RenderTarget* pRT, const D2D1_RECT_F& rc, CID tlColor, CID brColor, CID bgFill = CID::Invalid)
	{
		if (bgFill != CID::Invalid)
		{
			pRT->FillRectangle(rc, GetBrush(bgFill));
		}
		// border left and top
		pRT->DrawLine({ rc.left, rc.bottom - 1.0f }, { rc.left, rc.top }, GetBrush(tlColor));
		pRT->DrawLine({ rc.left, rc.top }, { rc.right - 1.0f, rc.top }, GetBrush(tlColor));
		// border bottom and right
		pRT->DrawLine({ rc.right - 1.0f, rc.top },	{ rc.right - 1, rc.bottom - 1.0f }, GetBrush(brColor));
		pRT->DrawLine({ rc.right - 1.0f, rc.bottom - 1.0f }, { rc.left, rc.bottom - 1.0f }, GetBrush(brColor));
	}

	void _DrawXhatchLines(ID2D1RenderTarget* pRT, CID lineClr,
		float& x, float y1, float y2, float cxPatWidth, bool drawXhatch = true)
	{
		ID2D1SolidColorBrush* pLineBrush = GetBrush(lineClr);
		float cxFull = cxPatWidth, cxHalf = cxPatWidth / 2.0f;
		pRT->DrawLine({ x, y1 }, { (x + cxHalf), y2 }, pLineBrush);
		pRT->DrawLine({ (x + cxHalf), y2 }, { (x + cxFull), y1 }, pLineBrush);
		if (drawXhatch)
		{
			pRT->DrawLine({ x, y2 }, { (x + cxHalf), y1 }, pLineBrush);
			pRT->DrawLine({ (x + cxHalf), y1 }, { (x + cxFull), y2 }, pLineBrush);
		}
		x += cxFull;
	}

	void _DrawPattern(ID2D1RenderTarget* pRT, LBitemBgPattern pattern, const D2D1_RECT_F& rc, 
			float yMarginTop = 0.0f, float yMarginBottom = 0.0f, bool drawXhatch = true)
	{
		if (pattern == LBitemBgPattern::None)
		{
			return;
		}
		bool isShortPitch = pattern == LBitemBgPattern::GrayXhatch1 || pattern == LBitemBgPattern::RedXhatch1;
		float cxPatWidth = isShortPitch ? 48.0f : 96.0f;
		bool isGray = pattern == LBitemBgPattern::GrayXhatch1 || pattern == LBitemBgPattern::GrayXhatch2;
		CID uPenColorId = isGray ? CID::TmlTrackBgNDln : CID::TmlTrackBGInvalid;
		//Microsoft::WRL::ComPtr<ID2D1PathGeometry> rect_geo = _MakeRectGeometry(rc);
		//// Create a layer.
		//Microsoft::WRL::ComPtr<ID2D1Layer> layer;
		//HRESULT hr_layer = pRT->CreateLayer(NULL, layer.GetAddressOf());
		//XeASSERT(hr_layer == S_OK);
		//if (SUCCEEDED(hr_layer))
		//{
		//	//pRT->SetTransform();
		//	// Push the layer with the geometric mask.
		//	pRT->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), rect_geo.Get()), layer.Get());
		//}
		pRT->PushAxisAlignedClip(rc, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		float cy = rc.bottom - rc.top;
		if ((cy - (yMarginTop + yMarginBottom)) < 8.0f)
		{
			yMarginTop = yMarginBottom = 0.0f;
		}
		float xa = rc.left, xb = xa - (cxPatWidth / 4.0f);
		float y1 = rc.top + yMarginTop, y2 = rc.bottom - yMarginBottom;
		while (xa < (rc.right + cxPatWidth))
		{
			_DrawXhatchLines(pRT, uPenColorId, xa, y1, y2, cxPatWidth, drawXhatch);
			_DrawXhatchLines(pRT, uPenColorId, xb, y1, y2, cxPatWidth, drawXhatch);
		}
		//if (SUCCEEDED(hr_layer))
		//{
		//	pRT->PopLayer();
		//}
		pRT->PopAxisAlignedClip();
	}

#pragma endregion UI_Drawing_Helpers
};

