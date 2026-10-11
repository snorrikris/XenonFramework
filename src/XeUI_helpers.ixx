module;

#include "os_minimal.h"
#include <d2d1.h>
#include <dwrite.h>

export module Xe.UI_helpers;

import std;
import Xe.UIcolorsIF;
import Xe.D2DWndBase;

export struct XeD2DFont
{
	XeFontMetrics	m_tm;
	IDWriteTextFormat* m_pFont = nullptr;
};

struct XeDPIfontMap
{
	std::map<EXE_FONT, XeD2DFont> m_map;
};

export class XeDWFontMap
{
	std::map<XeDPI, XeDPIfontMap> m_mapDPI;

	XeFontMetrics m_blankFM;

public:
	XeDWFontMap() = default;
	~XeDWFontMap()
	{
		XeASSERT(m_mapDPI.size() == 0);	// Call clear() before dtor.
		_release_fonts();
	}

	void InitializeMap(XeDPI dpi)	// Called when fonts created.
	{
		XeASSERT(dpi.is_valid());
		if (m_mapDPI.contains(dpi))
		{
			return;
		}
		m_mapDPI[dpi] = XeDPIfontMap();
		XeDPIfontMap& dpiMap = m_mapDPI.at(dpi);
		for (size_t i = 0; i < static_cast<size_t>(EXE_FONT::eLastFont); ++i)
		{
			dpiMap.m_map[(EXE_FONT)i] = XeD2DFont();
		}
	}

	void ReleaseFonts()
	{
		_release_fonts();
	}

	void SetFont(XeDPI dpi, EXE_FONT key, IDWriteTextFormat* pTF)
	{
		XeASSERT(dpi.is_valid());
		if (!m_mapDPI.contains(dpi))
		{
			XeASSERT(false);	// dpi map should exist.
			return;
		}
		XeDPIfontMap& dpiMap = m_mapDPI.at(dpi);
		if (dpiMap.m_map.contains(key))
		{
			XeD2DFont& tm = dpiMap.m_map.at(key);
			XeASSERT(tm.m_pFont == nullptr);	// Font should not exist.
			tm.m_pFont = pTF;
		}
		else
		{
			XeASSERT(false);	// font struct should exist.
		}
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

	void SetFontInfo(XeDPI dpi, EXE_FONT key, const XeFontMetrics& tm)
	{
		XeASSERT(dpi.is_valid());
		XeD2DFont* pTM = _GetFontInfoStruct(dpi, key);
		XeASSERT(pTM);
		pTM->m_tm = tm;
	}

	void Clear()
	{
		_release_fonts();
		m_mapDPI.clear();
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

