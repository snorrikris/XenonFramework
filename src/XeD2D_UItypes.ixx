module;

#include "os_minimal.h"
#include <d2d1.h>
#include <dwrite.h>

export module Xe.D2D_UItypes;

import std;
import Xe.UIcolorsIF;
import Xe.mfc_types;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

constexpr auto c_FLT_MIN = std::numeric_limits<float>::min();

export enum class Nudge { Dec, Inc };
export enum class VT { Red, Green, Blue, Hue, Sat, Lum, None };

export struct XeD2D1_COLOR_F : public D2D1_COLOR_F
{
	XeD2D1_COLOR_F() { r = 0, g = 0, b = 0, a = 1.0f; }
	XeD2D1_COLOR_F(const D2D1_COLOR_F& color) : D2D1_COLOR_F(color) {}

	void set_rgba(float ir, float ig, float ib, float ia) { r = ir, g = ig, b = ib, a = ia; }
	void set_rgb(float ir, float ig, float ib) { r = ir, g = ig, b = ib; }

	void SetL(float L)	// L = 0 to 1
	{
		float hsl2[3] = { 0 };
		float rgb2[3] = { r,g,b };
		FromRGBtoHSL(rgb2, hsl2);
		hsl2[2] = L;
		FromHSLtoRGB(hsl2, rgb2);
		set_rgb(rgb2[0], rgb2[1], rgb2[2]);
	}

	void SetS(float S)	// S = 0 to 1
	{
		float hsl2[3] = { 0 };
		float rgb2[3] = { r,g,b };
		FromRGBtoHSL(rgb2, hsl2);
		hsl2[1] = S;
		FromHSLtoRGB(hsl2, rgb2);
		set_rgb(rgb2[0], rgb2[1], rgb2[2]);
	}

	void SetH(float H)	// H = 0 to 6
	{
		float hsl2[3] = { 0 };
		float rgb2[3] = { r,g,b };
		FromRGBtoHSL(rgb2, hsl2);
		hsl2[0] = H;
		FromHSLtoRGB(hsl2, rgb2);
		set_rgb(rgb2[0], rgb2[1], rgb2[2]);
	}

	void SetHSL(float H, float S, float L)
	{
		float hsl[3] = { H,S,L };
		float rgb[3] = { 0 };
		FromHSLtoRGB(hsl, rgb);
		set_rgb(rgb[0], rgb[1], rgb[2]);
	}

	float GetH() const { return _Get_H_or_S_or_L(0); }
	float GetS() const { return _Get_H_or_S_or_L(1); }
	float GetL() const { return _Get_H_or_S_or_L(2); }

	void SetColorAndL(const D2D1_COLOR_F& color, float L)
	{
		float hsl[3] = { 0 }, rgb[3] = {};
		rgb[0] = color.r;
		rgb[1] = color.g;
		rgb[2] = color.b;
		FromRGBtoHSL(rgb, hsl);
		hsl[2] = L;
		FromHSLtoRGB(hsl, rgb);
		set_rgb(rgb[0], rgb[1], rgb[2]);
	}

	std::wstring GetDisplayString(VT vt) const
	{
		return std::to_wstring(GetDisplayValue(vt));
	}
	int GetDisplayValue(VT vt) const
	{
		switch (vt)
		{
		case VT::Red:	return (int)(r * 255.0f);
		case VT::Green:	return (int)(g * 255.0f);
		case VT::Blue:	return (int)(b * 255.0f);
		case VT::Hue:	return (int)(GetH() / 6.0f * 360.0f);
		case VT::Sat:	return (int)(GetS() * 255.0f);
		case VT::Lum:	return (int)(GetL() * 255.0f);
		}
		return 0;
	}

	bool SetUsingDisplayValue(VT vt, int value)
	{
		if (vt == VT::Hue && (value < 0 || value > 359)) { return false; }
		if (value < 0 || value > 255) { return false; }
		switch (vt)
		{
		case VT::Red:	r = (float)value / 255.0f; break;
		case VT::Green:	g = (float)value / 255.0f; break;
		case VT::Blue:	b = (float)value / 255.0f; break;
		case VT::Hue:	SetH((float)value * 6.0f / 360.0f); break;
		case VT::Sat:	SetS((float)value / 255.0f); break;
		case VT::Lum:	SetL((float)value / 255.0f); break;
		}
		return true;
	}

	void NudgeVT(Nudge nudge, VT vt)
	{
		switch (vt)
		{
		case VT::Red:	NudgeR(nudge); break;
		case VT::Green:	NudgeG(nudge); break;
		case VT::Blue:	NudgeB(nudge); break;
		case VT::Hue:	NudgeH(nudge); break;
		case VT::Sat:	NudgeS(nudge); break;
		case VT::Lum:	NudgeL(nudge); break;
		}
	}
	void NudgeR(Nudge nudge, float nudge_by = 0.01f) { _Nudge(nudge, r, nudge_by, 1.0f); }
	void NudgeG(Nudge nudge, float nudge_by = 0.01f) { _Nudge(nudge, g, nudge_by, 1.0f); }
	void NudgeB(Nudge nudge, float nudge_by = 0.01f) { _Nudge(nudge, b, nudge_by, 1.0f); }
	void NudgeH(Nudge nudge, float nudge_by = 0.06f)
	{
		float hsl[3] = { 0 };
		float rgb[3] = { r,g,b };
		FromRGBtoHSL(rgb, hsl);
		_Nudge(nudge, hsl[0], nudge_by, 5.99f);
		FromHSLtoRGB(hsl, rgb);
		set_rgb(rgb[0], rgb[1], rgb[2]);
	}
	void NudgeS(Nudge nudge, float nudge_by = 0.01f)
	{
		float hsl[3] = { 0 };
		float rgb[3] = { r,g,b };
		FromRGBtoHSL(rgb, hsl);
		_Nudge(nudge, hsl[1], nudge_by, 1.0f);
		FromHSLtoRGB(hsl, rgb);
		set_rgb(rgb[0], rgb[1], rgb[2]);
	}
	void NudgeL(Nudge nudge, float nudge_by = 0.01f)
	{
		float hsl[3] = { 0 };
		float rgb[3] = { r,g,b };
		FromRGBtoHSL(rgb, hsl);
		_Nudge(nudge, hsl[2], nudge_by, 1.0f);
		FromHSLtoRGB(hsl, rgb);
		set_rgb(rgb[0], rgb[1], rgb[2]);
	}

protected:
	void _Nudge(Nudge nudge, float& val, float nudge_by, float max_val)
	{
		if (nudge == Nudge::Inc)
			val += nudge_by;
		else
			val -= nudge_by;
		if (val < 0) val = 0;
		if (val > max_val) val = max_val;
	}

	float _Get_H_or_S_or_L(size_t idx) const // idx = 0 for H, 1 for S and 2 for L
	{
		float hsl[3] = { 0 };
		float rgb[3] = { r,g,b };
		FromRGBtoHSL(rgb, hsl);
		return idx < 3 ? hsl[idx] : 0.0f;
	}

public:
	static BYTE GetAValueFromRGB(DWORD rgb)
	{
		return (((rgb) >> 24));
	}

	void SetFromRGB(DWORD rgb, bool isIncludeAlpha)
	{
		r = (float)(GetRValue(rgb)) / 255.0f;
		g = (float)(GetGValue(rgb)) / 255.0f;
		b = (float)(GetBValue(rgb)) / 255.0f;
		if (isIncludeAlpha)
			a = (float)(GetAValueFromRGB(rgb)) / 255.0f;
		else
			a = 1.0f;
	}

	DWORD ToRGB(bool isIncludeAlpha)
	{
		DWORD rgb = RGB((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f));
		if (isIncludeAlpha)
			rgb |= ((unsigned int)(a * 255.0f) << 24);
		return rgb;
	}

	static void FromRGBtoHSL(float rgb[], float hsl[])
	{
		const float maxRGB = std::max(rgb[0], std::max(rgb[1], rgb[2]));
		const float minRGB = std::min(rgb[0], std::min(rgb[1], rgb[2]));
		const float delta2 = maxRGB + minRGB;
		hsl[2] = delta2 * 0.5f;

		const float delta = maxRGB - minRGB;
		if (delta < c_FLT_MIN)
			hsl[0] = hsl[1] = 0.0f;
		else
		{
			hsl[1] = delta / (hsl[2] > 0.5f ? 2.0f - delta2 : delta2);
			if (rgb[0] >= maxRGB)
			{
				hsl[0] = (rgb[1] - rgb[2]) / delta;
				if (hsl[0] < 0.0f)
					hsl[0] += 6.0f;
			}
			else if (rgb[1] >= maxRGB)
				hsl[0] = 2.0f + (rgb[2] - rgb[0]) / delta;
			else
				hsl[0] = 4.0f + (rgb[0] - rgb[1]) / delta;
		}
	}

	static void FromHSLtoRGB(const float hsl[], float rgb[])
	{
		if (hsl[1] < c_FLT_MIN)
			rgb[0] = rgb[1] = rgb[2] = hsl[2];
		else if (hsl[2] < c_FLT_MIN)
			rgb[0] = rgb[1] = rgb[2] = 0.0f;
		else
		{
			const float q = hsl[2] < 0.5f ? hsl[2] * (1.0f + hsl[1]) : hsl[2] + hsl[1] - hsl[2] * hsl[1];
			const float p = 2.0f * hsl[2] - q;
			float t[] = { hsl[0] + 2.0f, hsl[0], hsl[0] - 2.0f };

			for (int i = 0; i < 3; ++i)
			{
				if (t[i] < 0.0f)
					t[i] += 6.0f;
				else if (t[i] > 6.0f)
					t[i] -= 6.0f;

				if (t[i] < 1.0f)
					rgb[i] = p + (q - p) * t[i];
				else if (t[i] < 3.0f)
					rgb[i] = q;
				else if (t[i] < 4.0f)
					rgb[i] = p + (q - p) * (4.0f - t[i]);
				else
					rgb[i] = p;
			}
		}
	}
};

#pragma region Helpers
export [[nodiscard]] D2D1_POINT_2F MkPointF(int x, int y) { return D2D1_POINT_2F((float)x, (float)y); }
export [[nodiscard]] D2D1_POINT_2F MkPointF(const CPoint& pt) { return D2D1_POINT_2F((float)pt.x, (float)pt.y); }

export [[nodiscard]] D2D_SIZE_F MkSizeF(int cx, int cy) { return D2D_SIZE_F((float)cx, (float)cy); }
export [[nodiscard]] D2D_SIZE_F MkSizeF(const CSize& size) { return D2D_SIZE_F((float)size.cx, (float)size.cy); }

export [[nodiscard]] D2D1_RECT_F RectFfromRect(const CRect& rect)
{
	D2D1_RECT_F rcF;
	rcF.left = (float)rect.left;
	rcF.right = (float)rect.right;
	rcF.top = (float)rect.top;
	rcF.bottom = (float)rect.bottom;
	return rcF;
}

export [[nodiscard]] float WidthOf(const D2D1_RECT_F& rcF) { return std::abs(rcF.right - rcF.left); }
export [[nodiscard]] float HeightOf(const D2D1_RECT_F& rcF) { return std::abs(rcF.bottom - rcF.top); }
export [[nodiscard]] D2D_SIZE_F SizeOf(const D2D1_RECT_F& rcF) { return D2D_SIZE_F{ (rcF.right - rcF.left), (rcF.bottom - rcF.top) }; }

// Adjust position of rect1 such that its in the center of rect2 (H or V or both).
export void CenterRectFInRectF(D2D1_RECT_F& rect1, const D2D1_RECT_F& rect2, bool isHcenter, bool isVcenter)
{
	if (isHcenter)
	{
		float cx1 = (rect1.right - rect1.left);
		float cx2 = (rect2.right - rect2.left);
		float xOffset = (cx2 - (rect1.right - rect1.left)) / 2;
		rect1.left = rect2.left + xOffset;
		rect1.right = rect1.left + cx1;
	}
	if (isVcenter)
	{
		float cy1 = (rect1.bottom - rect1.top);
		float cy2 = (rect2.bottom - rect2.top);
		float yOffset = (cy2 - (rect1.bottom - rect1.top)) / 2;
		rect1.top = rect2.top + yOffset;
		rect1.bottom = rect1.top + cy1;
	}
}

export [[nodiscard]] D2D1_RECT_F OffsetRectF(const D2D1_RECT_F& rect, float xOffset, float yOffset)
{
	D2D1_RECT_F rc(rect);
	rc.left += xOffset;
	rc.right += xOffset;
	rc.top += yOffset;
	rc.bottom += yOffset;
	return rc;
}

export [[nodiscard]] D2D1_RECT_F DeflateRectF(const D2D1_RECT_F& rect, float xyo)
{
	D2D1_RECT_F rc(rect);
	rc.left += xyo; rc.right -= xyo;
	rc.top += xyo; rc.bottom -= xyo;
	return rc;
}

export bool PointFInRectF(const D2D1_RECT_F& rect, const D2D1_POINT_2F& pt)
{
	return pt.x >= rect.left && pt.x < rect.right && pt.y >= rect.top && pt.y < rect.bottom;
}

export void SetRectFSize(D2D1_RECT_F& rect, const D2D_SIZE_F& size)
{
	rect.right = rect.left + size.width;
	rect.bottom = rect.top + size.height;
}

export [[nodiscard]] D2D1_RECT_F SetR(float x, float y, float cx, float cy)
{
	D2D1_RECT_F r;
	r.left = x;
	r.right = x + cx;
	r.top = y;
	r.bottom = y + cy;
	return r;
}

export [[nodiscard]] D2D1_ROUNDED_RECT SetRR(float x, float y, float cx, float cy, float radius = 1.0f)
{
	D2D1_ROUNDED_RECT r;
	r.radiusX = r.radiusY = radius;
	r.rect.left = x;
	r.rect.right = x + cx;
	r.rect.top = y;
	r.rect.bottom = y + cy;
	return r;
}

// Function to calculate the scaled size (for an image) (ChatGPT 4.0)
// Calculate two boxes, the second box should fit in first box but maintain aspect ratio,
// the first box does not change, the second box should be as big as possible.
export [[nodiscard]] D2D_SIZE_F CalculateFittedBox(const D2D_SIZE_F& sizeClient, const D2D_SIZE_F& sizeBox)
{
	float clientAspect = sizeClient.width / sizeClient.height;
	float boxAspect = sizeBox.width / sizeBox.height;
	D2D_SIZE_F result;
	if (boxAspect > clientAspect)	// Width is the limiting factor?
	{
		result.width = sizeClient.width;
		result.height = sizeClient.width / boxAspect;
	}
	else	// Height is the limiting factor.
	{
		result.height = sizeClient.height;
		result.width = sizeClient.height * boxAspect;
	}
	return result;
}

// Calculate max. size for image inside box and center inside box.
export [[nodiscard]] D2D1_RECT_F CalculateImageSizeAndCenterInsideBox(const D2D1_RECT_F& rcBox, const D2D_SIZE_F& img_size)
{
	D2D1_RECT_F rcImg(rcBox);
	float cxClient = (rcBox.right - rcBox.left);
	float cyClient = (rcBox.bottom - rcBox.top);
	D2D_SIZE_F new_img_size = CalculateFittedBox(D2D_SIZE_F(cxClient, cyClient), img_size);
	rcImg.right = rcImg.left + new_img_size.width;
	rcImg.bottom = rcImg.top + new_img_size.height;

	// Center img box inside box.
	float xOffset = (cxClient - (rcImg.right - rcImg.left)) / 2;
	float yOffset = (cyClient - (rcImg.bottom - rcImg.top)) / 2;
	rcImg.left += xOffset;
	rcImg.right += xOffset;
	rcImg.top += yOffset;
	rcImg.bottom += yOffset;
	return rcImg;
}

// Calculate image box size and position inside the supplied box.
// If isOnLeftSide = true - set image position on the left side of the box else center the image inside the box.
export [[nodiscard]] D2D1_RECT_F CalculateImageSizeAndPosition(bool isOnLeftSide, const D2D1_RECT_F& rcBox,
	const D2D_SIZE_F& img_size)
{
	D2D_SIZE_F size_text((rcBox.right - rcBox.left), (rcBox.bottom - rcBox.top));
	D2D_SIZE_F new_img_size = CalculateFittedBox(size_text, img_size);
	D2D1_RECT_F rcImg;
	rcImg.top = rcBox.top;
	if (isOnLeftSide)
	{
		rcImg.left = rcBox.left;
	}
	else
	{
		rcImg.left = rcBox.left + ((size_text.width - new_img_size.width) / 2);
	}
	rcImg.right = rcImg.left + new_img_size.width;
	rcImg.bottom = rcImg.top + new_img_size.height;
	return rcImg;
}

// Calculate scale for image (img_size) such that it will fit inside the box (rcImgBox).
// Return scale as size object (same scale for X and Y - to use in scale effect when drawing image).
export [[nodiscard]] D2D_SIZE_F CalculateImageScaleForBox(const D2D1_RECT_F& rcImgBox, const D2D_SIZE_F& img_size)
{
	// Scale image to fit the image box.
	float scaleXY = 1.0f;
	float cxBox = (rcImgBox.right - rcImgBox.left);
	float cyBox = (rcImgBox.bottom - rcImgBox.top);
	if (cxBox > 0 && cyBox > 0)
	{
		float scaleX = img_size.width > cxBox ? cxBox / img_size.width : 1.0f;
		float scaleY = img_size.height > cyBox ? cyBox / img_size.height : 1.0f;
		scaleXY = std::min(scaleX, scaleY);
	}
	return D2D_SIZE_F(scaleXY, scaleXY);
}
#pragma endregion Helpers

// Rect plus some properties, can be used as button.
export class XeD2Rect : public D2D1_ROUNDED_RECT
{
public:
	std::wstring	m_text;
	bool			m_isEnabled = true;
	CID				m_textColorId = CID::CtrlTxt;
	EXE_FONT		m_fontId = EXE_FONT::eUI_Font;
	DWRITE_TEXT_ALIGNMENT		m_alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
	DWRITE_PARAGRAPH_ALIGNMENT	m_valignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
};

// Helper class - store rect in a keyed map.
// Key can be for example an enum value.
export class XeRRectMap
{
	std::map<int, XeD2Rect> m_map;

	XeD2Rect m_empty_rect{};

public:
	XeD2Rect Set(int key, float x, float y, float cx, float cy, float radius = 1.0F,
		const std::wstring& txt = std::wstring(), bool isEnabled = true)
	{
		XeD2Rect r;
		r.radiusX = r.radiusY = 1.0f;
		r.rect.left = x;
		r.rect.right = x + cx;
		r.rect.top = y;
		r.rect.bottom = y + cy;
		r.m_text = txt;
		r.m_isEnabled = isEnabled;
		m_map[key] = r;
		return r;
	}

	void SetText(int key, const std::wstring& txt)
	{
		if (m_map.contains(key))
		{
			XeD2Rect& r = m_map.at(key);
			r.m_text = txt;
		}
	}

	void SetEnabled(int key, bool isEnabled)
	{
		if (m_map.contains(key))
		{
			XeD2Rect& r = m_map.at(key);
			r.m_isEnabled = isEnabled;
		}
	}

	void SetFont(const std::vector<int>& keys, CID colorId = CID::CtrlTxt, EXE_FONT eFont = EXE_FONT::eUI_Font,
		DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT valignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
	{
		for (int key : keys)
		{
			SetFont(key, colorId, eFont, alignment, valignment);
		}
	}
	void SetFont(int key, CID colorId = CID::CtrlTxt, EXE_FONT eFont = EXE_FONT::eUI_Font,
		DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_CENTER,
		DWRITE_PARAGRAPH_ALIGNMENT valignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
	{
		if (m_map.contains(key))
		{
			XeD2Rect& r = m_map.at(key);
			r.m_textColorId = colorId;
			r.m_fontId = eFont;
			r.m_alignment = alignment;
			r.m_valignment = valignment;
		}
	}

	void ResetAllRects(const float& x, const float& y)
	{
		for (auto const& [key, r] : m_map)
		{
			ResetRect(key);
		}
	}
	void ResetRects(const std::vector<int>& keys)
	{
		for (int key : keys)
		{
			ResetRect(key);
		}
	}
	void ResetRect(int key)
	{
		if (m_map.contains(key))
		{
			XeD2Rect& r = m_map.at(key);
			r.radiusX = r.radiusY = 1.0f;
			r.rect.left = 0;
			r.rect.right = 0;
			r.rect.top = 0;
			r.rect.bottom = 0;
		}
	}

	const XeD2Rect& Get(int key) const
	{
		if (m_map.contains(key))
		{
			return m_map.at(key);
		}
		return m_empty_rect;
	}

	int PtInRect(const float& x, const float& y) const
	{
		for (auto const& [key, r] : m_map)
		{
			if (x >= r.rect.left && x <= r.rect.right && y >= r.rect.top && y <= r.rect.bottom)
			{
				return key;
			}
		}
		return -1;
	}
};

