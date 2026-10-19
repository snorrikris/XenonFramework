module;

#include "os_minimal.h"
#include <d2d1.h>
#include <dwrite.h>
#include "logging.h"

// Note - this module is ONLY used by XeUIcolors module - don't put anything else in here.
export module Xe.UI_helpers;

import std;
import Xe.UIcolorsIF;
import Xe.D2DWndBase;
import Xe.StringTools;

export struct XeFontDescription
{
	EXE_FONT			m_fontId;
	bool				m_isMonoSpaced;
	std::wstring        m_fontName;
	int                 m_fontPointSize = 0;	// User requested font point size.
	DWRITE_FONT_STYLE   m_style = DWRITE_FONT_STYLE_NORMAL;
	DWRITE_FONT_WEIGHT  m_weight = DWRITE_FONT_WEIGHT_NORMAL;
	DWRITE_FONT_STRETCH m_stretch = DWRITE_FONT_STRETCH_NORMAL;

	XeFontDescription() = default;
	XeFontDescription(EXE_FONT fontId, bool isMonoSpaced,
			const std::wstring& fontname, int fontpointsize,
			DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL)
			: m_fontId(fontId), m_isMonoSpaced(isMonoSpaced), m_fontName(fontname), m_fontPointSize(fontpointsize),
			m_style(style), m_weight(weight), m_stretch(stretch) {}
};

export struct XeD2DFont
{
	XeFontMetrics	m_tm;
	IDWriteTextFormat* m_pFont = nullptr;
};

struct XeDPIfontMap
{
	XeDPI							m_dpi;
	std::map<EXE_FONT, XeD2DFont>	m_map;

	int m_cxTooltipDefaultUnscaled = 212;

	double m_cxScaleFactor = 1.0, m_cyScaleFactor = 1.0;

	int m_cyGridTH_RowHeight = 0;

	MonospacedCharSize m_monospacedCharSize;

	XeDPIfontMap() = default;
	XeDPIfontMap(XeDPI dpi) : m_dpi(dpi) {}

	bool operator<(const XeDPIfontMap& rhs) const { return m_dpi < rhs.m_dpi; }
};

export class XeDWFontMap
{
private:
	VSRL::Logger& logger() { return VSRL::s_pVSRL->GetInstance("XeDWFontMap"); }

protected:
	IDWriteFactory*					m_pWriteFactory = nullptr;
	std::map<XeDPI, XeDPIfontMap>	m_mapDPI;

	XeFontMetrics					m_blankFM;

public:
	XeDWFontMap() = default;
	~XeDWFontMap()
	{
		XeASSERT(m_mapDPI.size() == 0);	// Call ReleaseFonts() before dtor.
		_release_fonts();
	}

	void SetFactoryPointer(IDWriteFactory* pWriteFactory)
	{
		m_pWriteFactory = pWriteFactory;
	}

	std::vector<XeDPI> GetDPIlist() const
	{
		std::vector<XeDPI> dpis;
		for (auto& [dpi, dpiMap] : m_mapDPI)
		{
			dpis.push_back(dpi);
		}
		return dpis;
	}

	void InitializeFontsForDPI(XeDPI dpi, const std::vector<XeFontDescription>& fonts)
	{
		XeASSERT(dpi.is_valid());
		if (m_mapDPI.contains(dpi))
		{
			return;
		}
		XeASSERT(m_pWriteFactory);
		m_mapDPI[dpi] = XeDPIfontMap(dpi);
		XeDPIfontMap& dpiMap = m_mapDPI.at(dpi);
		dpiMap.m_monospacedCharSize.cyMultiLineTextHeight = 0;
		dpiMap.m_monospacedCharSize.cxGridCharWidth = 8;

		// Honestly stolen from: https://chromium.googlesource.com/chromium/src/+/51ba3e6902ecd7284d3be51db648127d1be2187f/ui/gfx/platform_font_win.cc
		Microsoft::WRL::ComPtr<IDWriteFontCollection> font_collection;
		HRESULT hrFC = m_pWriteFactory->GetSystemFontCollection(font_collection.GetAddressOf());
		if (!_LogIfErrorAndAssert(hrFC, "GetSystemFontCollection"))
		{
			return;
		}

		std::wstring dt(L"0000-00-00 00:00:00,000");
		std::wstring ab(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
		for (const XeFontDescription& font : fonts)
		{
			dpiMap.m_map[font.m_fontId] = XeD2DFont();
			XeD2DFont& font_inst = dpiMap.m_map.at(font.m_fontId);

			font_inst.m_tm = XeFontMetrics(dpi, font.m_fontId, font.m_isMonoSpaced, font.m_fontName, font.m_fontPointSize,
					font.m_style, font.m_weight, font.m_stretch);
			XeFontMetrics& tm = font_inst.m_tm;

			float fsDIPs = dpi.font_point_size_to_pixels(tm.m_fontPointSize);	// font size is in points - D2D wants DIPs.

			std::string fontnameA = xet::toUTF8(tm.m_fontName);

			// Create font object.
			HRESULT hr = m_pWriteFactory->CreateTextFormat(tm.m_fontName.c_str(), 0, tm.m_weight, tm.m_style,
					tm.m_stretch, fsDIPs, L"", /*&pTextFmt*/ &font_inst.m_pFont);
			if (!_LogIfErrorAndAssert(hr, std::string("CreateTextFormat ") + fontnameA))
			{
				continue;
			}

			D2D1_SIZE_F sizeF, sizeF_ab;
			HRESULT hr1 = GetTextSize(dt, dpi, font.m_fontId, sizeF);
			HRESULT hr2 = GetTextSize(ab, dpi, font.m_fontId, sizeF_ab);
			XeASSERT(hr1 == S_OK && hr2 == S_OK);
			if (!_LogIfErrorAndAssert(hr1, "GetTextSize " + fontnameA) && SUCCEEDED(hr2))
			{
				continue;
			}
			tm.m_aveCharWidth = (int)(sizeF_ab.width / (float)ab.size() + 0.5f);
			tm.m_height = (int)(sizeF.height + 0.5f);
			tm.m_sizeDatetime.cx = (int)(sizeF.width + 0.5f), tm.m_sizeDatetime.cy = (int)(sizeF.height + 0.5f);

			// Calculate some guessed values - in case get font metrics fails.
			tm.m_descent = (int)(sizeF.height / 6.0f + 0.5f);
			tm.m_ascent = tm.m_height - tm.m_descent;
			tm.m_intLeading = (int)(sizeF.height / 6.0f + 0.5f);
			tm.m_extLeading = 0;

			if (font.m_fontId == EXE_FONT::eUI_Font)
			{
				double cyUIfont = tm.GetHeight();

				// Calculate Data Grid row sizes.
				dpiMap.m_cyGridTH_RowHeight = (int)(cyUIfont * 1.125);

				double scaleFactor = cyUIfont / 18.0;
				if (scaleFactor > 1.0)
				{
					dpiMap.m_cxScaleFactor = dpiMap.m_cyScaleFactor = scaleFactor;
				}
			}
			else if (font.m_fontId == EXE_FONT::eMonospacedFont)
			{
				// Set fall-back values in case calculate metrics fails.
				dpiMap.m_monospacedCharSize.cyMultiLineTextHeight = tm.GetHeight();
				dpiMap.m_monospacedCharSize.cxGridCharWidth = tm.m_aveCharWidth;
			}

			if (SUCCEEDED(hrFC))
			{
				_CalculateFontMetrics(font_collection.Get(), font_inst);
			}
		}
	}

	void ReleaseFonts()
	{
		_release_fonts();
		m_mapDPI.clear();
	}

	bool HasFont(XeDPI dpi, EXE_FONT key) const
	{
		XeASSERT(dpi.is_valid());
		if (!m_mapDPI.contains(dpi))
		{
			XeASSERT(false);	// dpi map should exist.
			return false;
		}
		const XeDPIfontMap& dpiMap = m_mapDPI.at(dpi);
		if (dpiMap.m_map.contains(key))
		{
			return dpiMap.m_map.at(key).m_pFont != nullptr;
		}
		return false;
	}

	bool VerifyFontExists(XeDPI dpi, EXE_FONT key) const
	{
		if (!m_mapDPI.contains(dpi))
		{
			return false;	// dpi map does not exist.
		}
		const XeDPIfontMap& dpiMap = m_mapDPI.at(dpi);
		if (dpiMap.m_map.contains(key))
		{
			return dpiMap.m_map.at(key).m_pFont != nullptr;
		}
		return false;
	}

	IDWriteTextFormat* GetFont(XeDPI dpi, EXE_FONT key) const
	{
		XeASSERT(dpi.is_valid());
		const XeD2DFont* pTM = _GetFontInfoStructConst(dpi, key);
		XeASSERT(pTM && pTM->m_pFont);
		if (pTM)
		{
			return pTM->m_pFont;
		}
		return nullptr;
	}

	const XeFontMetrics& GetFontInfoConst(XeDPI dpi, EXE_FONT key) const
	{
		XeASSERT(dpi.is_valid());
		const XeD2DFont* pTM = _GetFontInfoStructConst(dpi, key);
		XeASSERT(pTM);
		if (pTM)
		{
			return pTM->m_tm;
		}
		return m_blankFM;
	}

	int GetValue(UIV val, XeDPI dpi) const
	{
		XeASSERT(m_mapDPI.contains(dpi));
		if (!m_mapDPI.contains(dpi))
		{
			return 0;
		}
		const XeDPIfontMap& dpiMap = m_mapDPI.at(dpi);
		switch (val)
		{
		case UIV::s_cyGridTH_RowHeight:
			return dpiMap.m_cyGridTH_RowHeight;
		case UIV::cxTooltipDefault:
			return (int)((double)dpiMap.m_cxTooltipDefaultUnscaled * dpiMap.m_cxScaleFactor + 0.5);
		case UIV::cxComboBoxButton:
			return (int)((double)18 * dpiMap.m_cxScaleFactor + 0.5);
		case UIV::cxComboBoxButtonArrow: {
			int cxTri = (int)((double)8 * dpiMap.m_cxScaleFactor + 0.5);
			if ((cxTri & 1)) { ++cxTri; }			// Must be even number
			return cxTri;
		}
		case UIV::cySubMenuArrow: {
			int cxTri = (int)((double)6 * dpiMap.m_cxScaleFactor + 0.5);
			if ((cxTri & 1)) { ++cxTri; }			// Must be even number
			return cxTri;
		}
		case UIV::cxyThickBorder:
			return 6;
		case UIV::cxyThinBorder:
			return 3;
		case UIV::cyWindowCaption:
			XeASSERT(dpiMap.m_map.contains(EXE_FONT::eUI_FontBold));
			if (dpiMap.m_map.contains(EXE_FONT::eUI_FontBold))
			{
				return dpiMap.m_map.at(EXE_FONT::eUI_FontBold).m_tm.GetHeight();
			}
			return 20;
		case UIV::cxyScrollbar:
			return std::max(::GetSystemMetrics(SM_CXVSCROLL), ::GetSystemMetrics(SM_CYHSCROLL));		//pixels
		}
		XeASSERT(FALSE);
		return 0;
	}

	UIScaleFactor GetUIScaleFactor(XeDPI dpi) const
	{
		XeASSERT(m_mapDPI.contains(dpi));
		if (!m_mapDPI.contains(dpi))
		{
			return UIScaleFactor(1.0, 1.0);
		}
		const XeDPIfontMap& dpiMap = m_mapDPI.at(dpi);
		return UIScaleFactor(dpiMap.m_cxScaleFactor, dpiMap.m_cyScaleFactor);
	}

	MonospacedCharSize GetLogGridCharSize(XeDPI dpi) const
	{
		XeASSERT(m_mapDPI.contains(dpi));
		if (!m_mapDPI.contains(dpi))
		{
			return MonospacedCharSize();
		}
		const XeDPIfontMap& dpiMap = m_mapDPI.at(dpi);
		return dpiMap.m_monospacedCharSize;
	}

	HRESULT GetTextSize(const std::wstring& txt, XeDPI dpi, EXE_FONT font, D2D1_SIZE_F& size)
	{
		return GetTextSize(txt.c_str(), txt.size(), dpi, font, size);
	}
	HRESULT GetTextSize(const wchar_t* text, size_t text_len, XeDPI dpi, EXE_FONT font, D2D1_SIZE_F& size)
	{
		HRESULT hr = S_OK;
		size.width = size.height = 0;
		IDWriteTextFormat* pUIfont = GetFont(dpi, font);
		if (!pUIfont)
		{
			return S_FALSE;
		}
		Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
		float cxDim = 10000.0f, cyDim = 1000.f;
		// Create a text layout
		if (font != EXE_FONT::eMonospacedFont)
		{
			hr = m_pWriteFactory->CreateTextLayout(text, (UINT32)text_len,
					pUIfont, cxDim, cyDim, textLayout.GetAddressOf());
		}
		else
		{
			float pixelsPerDip = 1.0f;
			hr = m_pWriteFactory->CreateGdiCompatibleTextLayout(text, (UINT32)text_len,
					pUIfont, cxDim, cyDim, pixelsPerDip, nullptr, TRUE, textLayout.GetAddressOf());
		}
		XeASSERT(hr == S_OK);
		if (SUCCEEDED(hr))
		{
			// Get text size  
			DWRITE_TEXT_METRICS textMetrics;
			hr = textLayout->GetMetrics(&textMetrics);
			if (SUCCEEDED(hr))
			{
				size = D2D1::SizeF((float)ceil(textMetrics.widthIncludingTrailingWhitespace),
						(float)ceil(textMetrics.height));
			}
		}
		return hr;
	}

protected:
	void _release_fonts()
	{
		for (auto& [dpi, dpiMap] : m_mapDPI)
		{
			for (auto& [eFont, tm] : dpiMap.m_map)
			{
				if (tm.m_pFont)
				{
					tm.m_pFont->Release();
					tm.m_pFont = nullptr;
				}
			}
		}
	}

	XeD2DFont* _GetFontInfoStruct(XeDPI dpi, EXE_FONT key)
	{
		if (!m_mapDPI.contains(dpi))
		{
			XeASSERT(false);	// dpi map should exist.
			return nullptr;
		}
		XeDPIfontMap& dpiMap = m_mapDPI.at(dpi);
		if (dpiMap.m_map.contains(key))
		{
			return &(dpiMap.m_map.at(key));
		}
		XeASSERT(false);	// struct should exist.

		// Return default font if requested font not found in map.
		if (key != EXE_FONT::eUI_Font)
		{
			return _GetFontInfoStruct(dpi, EXE_FONT::eUI_Font);
		}
		XeASSERT(false);
		return nullptr;
	}
	const XeD2DFont* _GetFontInfoStructConst(XeDPI dpi, EXE_FONT key) const
	{
		if (!m_mapDPI.contains(dpi))
		{
			XeASSERT(false);	// dpi map should exist.
			return nullptr;
		}
		const XeDPIfontMap& dpiMap = m_mapDPI.at(dpi);
		if (dpiMap.m_map.contains(key))
		{
			return &(dpiMap.m_map.at(key));
		}
		XeASSERT(false);	// struct should exist.

		// Return default font if requested font not found in map.
		if (key != EXE_FONT::eUI_Font)
		{
			return _GetFontInfoStructConst(dpi, EXE_FONT::eUI_Font);
		}
		XeASSERT(false);
		return nullptr;
	}

	void _CalculateFontMetrics(IDWriteFontCollection* pCollection, XeD2DFont& font_inst)
	{
		XeFontMetrics& tm = font_inst.m_tm;
		float fsDIPs = tm.m_dpi.font_point_size_to_pixels(tm.m_fontPointSize);	// font size is in points - D2D wants DIPs.
		std::string fontnameA = xet::toUTF8(tm.m_fontName);
		BOOL exists = FALSE;
		uint32_t index = 0;
		HRESULT hr = pCollection->FindFamilyName(tm.m_fontName.c_str(), &index, &exists);
		if (!_LogIfErrorAndAssert(hr, "FindFamilyName " + fontnameA))
		{
			return;
		}
		Microsoft::WRL::ComPtr<IDWriteFontFamily> font_family;
		if (index != UINT_MAX && exists)
		{
			hr = pCollection->GetFontFamily(index, font_family.GetAddressOf());
		}
		else
		{
			// If we fail to find a matching font, then fallback to the first font in
			// the list. This is what skia does as well.
			hr = pCollection->GetFontFamily(0, font_family.GetAddressOf());
		}
		if (!_LogIfErrorAndAssert(hr, "GetFontFamily " + fontnameA))
		{
			return;
		}
		Microsoft::WRL::ComPtr<IDWriteFont> dwrite_font;
		hr = font_family->GetFirstMatchingFont(tm.m_weight, tm.m_stretch, tm.m_style, dwrite_font.GetAddressOf());
		if (!_LogIfErrorAndAssert(hr, "GetFirstMatchingFont " + fontnameA))
		{
			return;
		}

		dwrite_font->GetMetrics(&tm.m_dfm);

		tm.m_ascent = (int)(tm.m_dfm.ascent * fsDIPs / tm.m_dfm.designUnitsPerEm + 0.5f);
		tm.m_descent = (int)(tm.m_dfm.descent * fsDIPs / tm.m_dfm.designUnitsPerEm + 0.5f);
		int capHeightPixels = (int)(tm.m_dfm.capHeight * fsDIPs / tm.m_dfm.designUnitsPerEm + 0.5f);
		tm.m_intLeading = tm.m_ascent - capHeightPixels;
		tm.m_extLeading = (int)(tm.m_dfm.lineGap * fsDIPs / tm.m_dfm.designUnitsPerEm + 0.5f);

		if (tm.m_fontId == EXE_FONT::eMonospacedFont)
		{
			Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace;
			hr = dwrite_font->CreateFontFace(fontFace.GetAddressOf());
			if (_LogIfErrorAndAssert(hr, "CreateFontFace"))
			{
				uint32_t codepoints[2]{ wchar_t{L'M'}, wchar_t{L'y'} };
				uint16_t glyphIdc[2]{ 0, 0 };

				hr = fontFace->GetGlyphIndicesW(codepoints, 2, glyphIdc);
				if (_LogIfErrorAndAssert(hr, "GetGlyphIndicesW"))
				{
					BOOL useGdiNatural = TRUE;
					// When set to FALSE, the metrics are the same as the metrics of GDI aliased text.
					// When set to TRUE, the metrics are the same as the metrics of text measured by GDI
					// using a font created with CLEARTYPE_NATURAL_QUALITY.

					DWRITE_GLYPH_METRICS glyph_meterics[2]{ 0, 0 };
					hr = fontFace->GetGdiCompatibleGlyphMetrics(fsDIPs, 1.0f, nullptr, useGdiNatural, glyphIdc, 2, glyph_meterics);
					if (_LogIfErrorAndAssert(hr, "GetGdiCompatibleGlyphMetrics"))
					{
						float cyHeight = (float)glyph_meterics[0].advanceHeight * 1.075f;
						float cy = cyHeight * fsDIPs / (float)tm.m_dfm.designUnitsPerEm;
						XeDPIfontMap& dpiMap = m_mapDPI.at(tm.m_dpi);
						dpiMap.m_monospacedCharSize.cyMultiLineTextHeight = (int)(cy + 0.5);
						dpiMap.m_monospacedCharSize.cxGridCharWidth
								= (int)((float)glyph_meterics[0].advanceWidth * fsDIPs / tm.m_dfm.designUnitsPerEm + 0.5f);
					}
				}
			}
		}
	}

	bool _LogIfErrorAndAssert(HRESULT hr, const std::string& fn)
	{
		XeASSERT(hr == S_OK);
		if (!SUCCEEDED(hr))
		{
			logger().error("%s failed. Error code: 0x%08X", fn.c_str(), hr);
		}
		return SUCCEEDED(hr);
	}
};

export struct XeD2DWndInfo
{
	CXeD2DWndBase* m_pWndObj = nullptr;
	std::wstring	m_className;
	std::wstring	m_parentClassName;
};
export class XeHwnd2ObjectMap
{
	std::map<HWND, XeD2DWndInfo> m_map;

	// We keep the last hWnd removed - because it can happen that a window can receive messages
	// after it has been removed (had a case of WM_IME_NOTIFY after WM_NCDESTROY for ViewsListWnd).
	// When Get is called for a window not in the map - we just silently return null instead of ASSERT
	// when the handle is the last removed window.
	HWND m_last_hWnd_removed = 0;

public:
	XeHwnd2ObjectMap() = default;
	//~XeHwnd2ObjectMap() {}

	void Set(HWND hWnd, XeD2DWndInfo nfo)
	{
		if (m_map.contains(hWnd))
		{
			XeASSERT(false);	// Set called twice - why?
		}
		m_map[hWnd] = nfo;
	}

	bool HasHwnd(HWND hWnd) const
	{
		return m_map.contains(hWnd);
	}

	inline const XeD2DWndInfo* Get(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return &(m_map.at(hWnd));
		}
		if (hWnd != m_last_hWnd_removed)
		{
			XeASSERT(false);
		}
		return nullptr;
	}

	inline const XeD2DWndInfo* GetIfExists(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return &(m_map.at(hWnd));
		}
		return nullptr;
	}

	void Remove(HWND hWnd)
	{
		if (m_map.contains(hWnd))
		{
			m_map.erase(hWnd);
		}
		m_last_hWnd_removed = hWnd;
	}
};

export class XeHwnd2CallbackMap
{
	std::map<HWND, DialogClosedCallbackFunc> m_map;

public:
	XeHwnd2CallbackMap() = default;
	//~XeHwnd2CallbackMap() {}

	void Set(HWND hWnd, DialogClosedCallbackFunc callback)
	{
		if (m_map.contains(hWnd))
		{
			XeASSERT(false);	// Set called twice - why?
		}
		m_map[hWnd] = callback;
	}

	bool HasHwnd(HWND hWnd) const
	{
		return m_map.contains(hWnd);
	}

	inline DialogClosedCallbackFunc Get(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return m_map.at(hWnd);
		}
		XeASSERT(false);
		return nullptr;
	}

	inline DialogClosedCallbackFunc GetIfExists(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return m_map.at(hWnd);
		}
		return nullptr;
	}

	void Remove(HWND hWnd)
	{
		if (m_map.contains(hWnd))
		{
			m_map.erase(hWnd);
		}
	}
};

export class XeHwnd2KeyboardFilterMap
{
	std::map<HWND, FilterKeyboardMessageCallbackFunc> m_map;

public:
	XeHwnd2KeyboardFilterMap() = default;
	//~XeHwnd2KeyboardFilterMap() {}

	void Set(HWND hWnd, FilterKeyboardMessageCallbackFunc filter_callback)
	{
		if (m_map.contains(hWnd))
		{
			XeASSERT(false);	// Set called twice - why?
		}
		m_map[hWnd] = filter_callback;
	}

	bool HasHwnd(HWND hWnd) const
	{
		return m_map.contains(hWnd);
	}

	inline FilterKeyboardMessageCallbackFunc Get(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return m_map.at(hWnd);
		}
		XeASSERT(false);
		return nullptr;
	}

	inline FilterKeyboardMessageCallbackFunc GetIfExists(HWND hWnd) const
	{
		if (m_map.contains(hWnd))
		{
			return m_map.at(hWnd);
		}
		return nullptr;
	}

	void Remove(HWND hWnd)
	{
		if (m_map.contains(hWnd))
		{
			m_map.erase(hWnd);
		}
	}
};

