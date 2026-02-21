module;

#include "os_minimal.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "XeAssert.h"
#include <d2d1.h>
#include <dwrite.h>

export module Xe.TreeCtrlEx;

import Xe.NaryTree;
import Xe.UIcolorsIF;
import Xe.ScrollBar;
import Xe.Menu;
import Xe.D2DWndBase;
import Xe.Utils;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

constexpr wchar_t XETREECTRLEXWND_CLASSNAME[] = L"XeTreeCtrlExWndClass";  // Window class name

constexpr UINT c_VSB_ID = 1001;
constexpr UINT c_HSB_ID = 1002;

struct TVITEM_mask
{
	UINT m_mask;

	TVITEM_mask(UINT mask) : m_mask(mask) {}

	bool isTVIF_CHILDREN() const
	{ return m_mask & TVIF_CHILDREN; }// The cChildren member is valid.

	bool isTVIF_DI_SETITEM() const
	{ return m_mask & TVIF_DI_SETITEM; }// The tree-view control will retain the supplied information and will not request it again. This flag is valid only when processing the TVN_GETDISPINFO notification.

	bool isTVIF_HANDLE() const
	{ return m_mask & TVIF_HANDLE; }// The hItem member is valid.

	bool isTVIF_PARAM() const
	{ return m_mask & TVIF_PARAM; }// The lParam member is valid.

	bool isTVIF_IMAGE() const
	{ return m_mask & TVIF_IMAGE; }// The iImage member is valid.

	bool isTVIF_SELECTEDIMAGE() const
	{ return m_mask & TVIF_SELECTEDIMAGE; }// The iSelectedImage member is valid.

	bool isTVIF_EXPANDEDIMAGE() const
	{ return m_mask & TVIF_EXPANDEDIMAGE; }// The iExpandedImage  member is valid.

	bool isTVIF_STATE() const
	{ return m_mask & TVIF_STATE; }// The state and stateMask members are valid.

	bool isTVIF_TEXT() const
	{ return m_mask & TVIF_TEXT; }// The pszText and cchTextMax members are valid.
};

export struct TreeItemData
{
	union {
		int64_t m_i64 = 0;
		struct {	// first line = bit0
			uint64_t	m_rgbColor			: 24;	// Text FG color
			uint64_t	m_hasColor			: 1;	// has Text FG color
			uint64_t	m_isColorIdColor	: 1;	// m_rgbColor is color ID value (CID)
			uint64_t	m_imagePID			: 10;	// PID
			uint64_t	m_selected_imagePID : 10;	// PID
			uint64_t	m_expanded_imagePID : 10;	// PID
			uint64_t	m_unused			: 8;
		};
	};
	TreeItemData() = default;
	TreeItemData(PID imagePID, PID selecetedimagePID = PID::None, PID expanded_imagePID = PID::None,
			CID text_colorCID = CID::Invalid)
	{
		SetImage(imagePID);
		SetSelectedImage(selecetedimagePID);
		SetExpandedImage(expanded_imagePID);
		if (text_colorCID != CID::Invalid)
		{
			SetColorID(text_colorCID);
		}
	}
	void SetColor(/*COLORREF*/uint32_t rgbColor)
	{
		m_rgbColor = rgbColor;
		m_hasColor = 1;
	}
	void SetColorID(CID colorId)
	{
		m_rgbColor = (uint64_t)colorId;
		m_hasColor = 1;
		m_isColorIdColor = 1;
	}
	CID GetColorID() const
	{
		XeASSERT(m_hasColor == 1 && m_isColorIdColor == 1);
		return (CID)m_rgbColor;
	}

	void SetImage(PID pid)
	{
		unsigned int id = (unsigned int)pid;
		XeASSERT(id <= 1023);
		m_imagePID = id;
	}
	bool HasImagePID() const { return m_imagePID != (uint64_t)PID::None; }
	PID GetImagePID() const
	{
		return (PID)m_imagePID;
	}

	void SetSelectedImage(PID pid)
	{
		unsigned int id = (unsigned int)pid;
		XeASSERT(id <= 1023);
		m_selected_imagePID = id;
	}
	bool HasSelectedImagePID() const { return m_selected_imagePID != (uint64_t)PID::None; }
	PID GetSelectedImagePID() const
	{
		return (PID)m_selected_imagePID;
	}

	void SetExpandedImage(PID pid)
	{
		unsigned int id = (unsigned int)pid;
		XeASSERT(id <= 1023);
		m_expanded_imagePID = id;
	}
	bool HasExpandedImagePID() const { return m_expanded_imagePID != (uint64_t)PID::None; }
	PID GetExpandedImagePID() const
	{
		return (PID)m_expanded_imagePID;
	}
};
static_assert(sizeof(TreeItemData) == 8, "TreeItemData size incorrect");

struct TreeCtrlExItem
{
	std::wstring  m_string;
	bool		 m_isGetTextCallback = false;
	bool		 m_hasChildren = false;
	uint64_t	 m_lParam = 0;
	TreeItemData m_data;

	TreeCtrlExItem() = default;
	TreeCtrlExItem(const wchar_t* pStr, size_t str_len, uint64_t lParam, const TreeItemData& item_data)
		: m_lParam(lParam)
	{
		if (pStr == LPSTR_TEXTCALLBACKW)
		{
			m_isGetTextCallback = true;
		}
		else if (pStr)
		{
			m_string.assign(pStr, str_len);
		}
		m_data = item_data;
	}
	//~TreeCtrlExItem()	// dtor for testing if delete all nodes
	//{
	//	if (m_lParam)
	//	{
	//		m_lParam = 0;
	//	}
	//}
};

typedef CXeNaryTreeNode<TreeCtrlExItem> CXeTreeNode;

typedef std::function<std::vector<ListBoxExItem>(TVITEMW*)> GetContextMenuCallbackFunc;
typedef std::function<void(UINT, TVITEMW*)> SelectedContextMenuItemCallbackFunc;

export class CXeTreeCtrlEx : public CXeD2DCtrlBase
{
protected:
	bool m_hasFocus = false, m_isEnabled = true;

	std::unique_ptr<CXeScrollBar> m_vScollBar;
	std::unique_ptr<CXeScrollBar> m_hScollBar;
	bool m_hasVscroll = false, m_hasHscroll = false;
	int m_cxSysSbV = 10, m_cySysSbH = 10;

	CXeNaryTree<TreeCtrlExItem> m_tree;

	size_t m_curIdx = 0xFFFFFFFF, m_topIdx = 0, m_itemCount = 0;

	int m_cxMaxStringWidth = 0;
	int m_cyItem = 15;
	int m_x_start = 2, m_y_start = 0;

	int m_cxLevelMargin = 20, m_cxyBox = 9, m_cxyBoxSign = 5, m_cxBoxMargin = 4, m_cyBoxMargin = 5;
	int m_cxyBoxSignMargin = 0, m_cxyBoxMiddle = m_cxyBox / 2;
	int m_cxCurItemBgMargin = 2;
	int m_cxyImgDec = 2, m_cxImgMargin = 2;
	CID m_dotsColorId = CID::CtrlTxtDis;
	CID m_boxOutlineColorId = m_dotsColorId, m_boxSignColorId = CID::CtrlTxt;

	Microsoft::WRL::ComPtr<ID2D1StrokeStyle> m_strokeStyleDots;

	GetContextMenuCallbackFunc m_getContextMenu = nullptr;
	SelectedContextMenuItemCallbackFunc m_selectedContextMenuItem = nullptr;

#pragma region Create
public:
	CXeTreeCtrlEx(CXeUIcolorsIF* pUIcolors) : CXeD2DCtrlBase(pUIcolors)
	{
		m_xeUI->RegisterWindowClass(XETREECTRLEXWND_CLASSNAME, D2DCtrl_WndProc);
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
		m_cyItem = tm.GetLineHeight();
		if (m_cyItem == 0) { m_cyItem = 15; }
		if (m_cyItem & 1) { ++m_cyItem; }
		UIScaleFactor sf = m_xeUI->GetUIScaleFactor();
		m_cxyBox = sf.ScaleX(m_cxyBox);
		if (!(m_cxyBox & 1)) { ++m_cxyBox; }			// Must be odd number
		m_cxyBoxSign = sf.ScaleX(m_cxyBoxSign);
		if (!(m_cxyBoxSign & 1)) { ++m_cxyBoxSign; }	// Must be odd number
		m_cyBoxMargin = (int)((double)(m_cyItem - m_cxyBox) / 2 + 0.5);
		if (m_cyBoxMargin & 1) { ++m_cyBoxMargin; }
		m_cxBoxMargin = sf.ScaleX(m_cxBoxMargin);
		m_cxLevelMargin = sf.ScaleX(m_cxLevelMargin);
		m_cxyBoxSignMargin = (m_cxyBox - m_cxyBoxSign) / 2;
		m_cxyBoxMiddle = m_cxyBox / 2;
		m_cxSysSbV = ::GetSystemMetrics(SM_CXVSCROLL);
		m_cySysSbH = ::GetSystemMetrics(SM_CYHSCROLL);
	}

	bool Create(DWORD dwStyle, DWORD dwExStyle, const CRect& rect, HWND hParentWnd, UINT nID)
	{
		dwStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		XeASSERT(hParentWnd);
		if (!::IsWindow(hParentWnd))
		{
			XeASSERT(FALSE);
			return FALSE;
		}
		HWND hWnd = CreateD2DWindow(dwExStyle, XETREECTRLEXWND_CLASSNAME, nullptr, dwStyle,
				rect, hParentWnd, nID);
		return hWnd != 0;
	}

	void SetCallback_GetContextMenu(GetContextMenuCallbackFunc getContextMenu,
			SelectedContextMenuItemCallbackFunc selectedContextMenuItem)
	{
		XeASSERT(getContextMenu && selectedContextMenuItem);	// MUST supply both callbacks.
		if (getContextMenu && selectedContextMenuItem)
		{
			m_getContextMenu = getContextMenu;
			m_selectedContextMenuItem = selectedContextMenuItem;
		}
	}
#pragma endregion Create

#pragma region Misc
protected:
	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		if (uMsg >= TV_FIRST && uMsg < HDM_FIRST)
		{
			return _OnTreeControlMessage(hWnd, uMsg, wParam, lParam);
		}
		return CXeD2DWndBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	// WM_SIZE message handler.
	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override
	{
		LRESULT lResult = CXeD2DWndBase::_OnSize(hWnd, wParam, lParam);
		XeASSERT(::IsWindow(Hwnd()));
		XeASSERT(!m_style.WS().hasTVS_UnsupportedStyle());

		_AdjustScrollBars();
		return lResult;
	}

	// WM_ENABLE message handler.
	virtual LRESULT _OnEnableWindow(bool isEnabled) override
	{
		XeASSERT(::IsWindow(Hwnd()));

		m_isEnabled = isEnabled;
		if (m_vScollBar)
		{
			m_vScollBar->EnableWindow(isEnabled);
		}
		if (m_hScollBar)
		{
			m_hScollBar->EnableWindow(isEnabled);
		}
		_RedrawDirectly();
		return 0;
	}
#pragma endregion Misc

#pragma region Create_UI_resources
protected:
	virtual void _CreatePaintResources(ID2D1RenderTarget* pRT) override
	{
		CXeD2DWndBase::_CreatePaintResources(pRT);

		// Create stroke style for dots lines.
		m_xeUI->D2D_GetFactory()->CreateStrokeStyle(
			D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
				D2D1_LINE_JOIN_MITER, 0.5f, D2D1_DASH_STYLE_DASH, 0.0f),
			nullptr, 0, &m_strokeStyleDots);
	}
#pragma endregion Create_UI_resources

#pragma region Painting
protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override
	{
		CRect rcReal = _GetRealClientRect();	// Client rect minus scrollbars (if any).
		int xOffset = _GetHscrollInfo().nPos;
		CRect rcPaint(rcReal);
		pRT->FillRectangle(RectFfromRect(rcPaint), GetBrush(CID::CtrlBg)); // Fill background

		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);

		int x = m_x_start, y = m_y_start;
		int cxMax = m_cxMaxStringWidth;
		size_t itemCount = m_tree.GetVisibleNodeCount();
		size_t itemsInViewCount = rcPaint.Height() / m_cyItem;
		std::vector<const CXeTreeNode*> nodes
			= m_tree.GetVisibleNodesIdxRange(m_topIdx, itemsInViewCount + 1);
		BoolFlags64 drawVlines;
		if (nodes.size() > 0)
		{
			drawVlines = _InitializeDrawVlinesForFirstNodeInView(nodes[0]);
		}
		HWND hParentWnd = ::GetParent(Hwnd());
		size_t i = m_topIdx;
		for (const CXeTreeNode* pNode : nodes)
		{
			std::wstring tmp_str;
			const std::wstring* pItemStr = &tmp_str;
			if (pNode->m_data.m_isGetTextCallback)
			{
				NMTVDISPINFOW di{ 0 };
				di.hdr.hwndFrom = Hwnd();
				di.hdr.idFrom = GetDlgCtrlID();
				di.hdr.code = TVN_GETDISPINFO;
				di.item.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
				di.item.hItem = reinterpret_cast<HTREEITEM>(m_tree.GetNodeHandle(pNode));
				di.item.state = pNode->IsExpanded() ? TVIS_EXPANDED : 0;
				di.item.stateMask = TVIS_EXPANDED;
				di.item.lParam = pNode->m_data.m_lParam;
				::SendMessage(hParentWnd, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&di);
				if (di.item.pszText)
				{
					tmp_str = di.item.pszText;
				}
			}
			else
			{
				pItemStr = &(pNode->m_data.m_string);
			}
			CRect rcItem(rcPaint);
			rcItem.top = m_y_start + (m_cyItem * (int)(i - m_topIdx));
			rcItem.bottom = rcItem.top + m_cyItem;
			rcItem.left -= xOffset;
			CRect rcTxt(rcItem);
			rcTxt.left += _CalculateTextLeftEdge(pNode->GetLevel());

			PID pid = PID::None;
			if (pNode->m_data.m_data.HasImagePID())
			{
				pid = pNode->m_data.m_data.GetImagePID();
			}
			if (pNode->IsExpanded() && pNode->m_data.m_data.HasExpandedImagePID())
			{
				pid = pNode->m_data.m_data.GetExpandedImagePID();
			}

			bool isCurSelItem = i == m_curIdx;
			if (isCurSelItem && pNode->m_data.m_data.HasSelectedImagePID())
			{
				pid = pNode->m_data.m_data.GetSelectedImagePID();
			}
			CRect rcImage(rcTxt);
			rcImage.top += m_cxyImgDec;
			rcImage.bottom -= m_cxyImgDec;
			int cxImage = 0;
			CSize sizeImg;
			IWICFormatConverter* pImg = pid == PID::None ? nullptr : m_xeUI->D2D_GetImage(pid, &sizeImg);
			if (pImg)
			{
				int x = rcImage.left, y = rcImage.top, cx = rcImage.Width(), cy = rcImage.Height();
				int cxPng = sizeImg.cx;
				int cyPng = sizeImg.cy;
				if (cx > cxPng)
				{
					cx = cyPng;
				}
				if (cy > cyPng)
				{
					y = y + (cy - cyPng) / 2;
					cy = cyPng;
				}
				if (cx < cy)
				{
					cy = cx;
				}
				if (cx > cy)
				{
					cx = cy;
				}
				CRect rcCalcImage(x, y, x + cx, y + cy);


				cxImage = rcCalcImage.Width();
			}

			CSize sizeText = m_xeUI->GetTextSizeW(EXE_FONT::eUI_Font, pItemStr->c_str());
			if (isCurSelItem && m_isEnabled)
			{
				CRect selRect(rcTxt);
				selRect.left -= m_cxCurItemBgMargin;
				int cxImageBg = cxImage > 0 ? cxImage + m_cxImgMargin : 0;
				selRect.right = selRect.left + cxImageBg + sizeText.cx + m_cxCurItemBgMargin;
				pRT->FillRectangle(RectFfromRect(selRect), GetBrush(CID::CtrlCurItemBg));
				if (m_hasFocus)
				{
					_DrawFocusRect(pRT, RectFfromRect(selRect));
				}
			}

			_DrawExpandButtonAndVlines(pRT, pNode, rcItem, drawVlines);

			if (pImg)
			{
				_DrawImage(pRT, pImg, RectFfromRect(rcImage), true, m_isEnabled, false);
				rcTxt.left += cxImage + m_cxImgMargin;
			}

			COLORREF rgbFg = m_xeUI->GetColor(isCurSelItem ? CID::CtrlTxt : CID::CtrlCurItemTxt);
			if (pNode->m_data.m_data.m_isColorIdColor)
			{
				rgbFg = m_xeUI->GetColor(pNode->m_data.m_data.GetColorID());
			}
			else if (pNode->m_data.m_data.m_hasColor)
			{
				rgbFg = pNode->m_data.m_data.m_rgbColor;
			}
			if (!m_isEnabled)
			{
				rgbFg = m_xeUI->GetColor(CID::CtrlTxtDis);
			}
			_CreateAndDrawTextLayout(pRT, *pItemStr, RectFfromRect(rcTxt), rgbFg);
			sizeText.cx += (rcTxt.left + 15);
			if (sizeText.cx > cxMax)
			{
				cxMax = sizeText.cx;
			}
			y += m_cyItem;
			if (y > rcPaint.bottom)
			{
				break;
			}
			++i;
		}

		_UpdateHVsizeAndAdjustScrollBars(itemCount, cxMax);
	}

	BoolFlags64 _InitializeDrawVlinesForFirstNodeInView(const CXeTreeNode* pNode)
	{
		BoolFlags64 drawVlines;
		drawVlines.SetFlag(0);	// Initialized (note - level 0 has no V line).
		const CXeTreeNode* pParentNode = pNode->GetConstParentNode();
		while (pParentNode)	// Parent node of root is null.
		{
			int parent_node_level = pParentNode->GetLevel();
			if (!pParentNode->IsLastChild())
			{
				drawVlines.SetFlag((uint8_t)parent_node_level);
			}
			pParentNode = pParentNode->GetConstParentNode();
		}
		return drawVlines;
	}

	void _DrawExpandButtonAndVlines(
		ID2D1RenderTarget* pRT, const CXeTreeNode* pNode, const CRect& rcItem, BoolFlags64 &drawVlines)
	{
		bool isRootNode = pNode->IsRootNode();
		bool isLastChild = pNode->IsLastChild();
		int level = pNode->GetLevel();
		CRect rcBox = _GetButtonBox(rcItem, level);
		int xMiddleLine = rcBox.left + m_cxyBoxMiddle;
		int yMiddleLine = rcBox.top + m_cxyBoxMiddle;
		int xTxtLeft = _CalculateTextLeftEdge(level);
		int x1 = rcBox.left, x2 = rcBox.right - 1, y1 = rcBox.top, y2 = rcBox.bottom - 1;
		if (_hasNodeChildren(pNode))	// Draw expand box button?
		{
			pRT->DrawRectangle(RectFfromRect(rcBox), GetBrush(m_boxOutlineColorId));
			if (!isRootNode)
			{	// Draw dots above (V)
				_DrawDots(pRT, xMiddleLine, rcItem.top, rcBox.top - rcItem.top, true);
			}
			x2 += 2;
			_DrawDots(pRT, x2, yMiddleLine, xTxtLeft - x2 - 2, false);	// Draw dots after (H)
			if (!isLastChild)
			{
				y2 += 2;
				_DrawDots(pRT, xMiddleLine, y2, rcItem.bottom - y2, true);// Draw dots below (V)
			}
			x1 += m_cxyBoxSignMargin;
			pRT->DrawLine({ (float)x1, (float)yMiddleLine }, { (float)(x1 + m_cxyBoxSign), (float)yMiddleLine},
					GetBrush(m_boxSignColorId));			// Draw the '-'.
			if (!pNode->IsExpanded())
			{
				x2 = xMiddleLine;
				y2 = rcBox.top + m_cxyBoxSignMargin;
				pRT->DrawLine({ (float)x2, (float)y2 }, { (float)x2, (float)(y2 + m_cxyBoxSign) },
						GetBrush(m_boxSignColorId));		// Draw the '|'.
			}
		}
		else
		{
			if (isLastChild)	// Draw 'L' shaped line?
			{	// Draw dots to middle (V)
				_DrawDots(pRT, xMiddleLine, rcItem.top, yMiddleLine - rcItem.top, true);
			}
			else
			{	// Draw vertial dots line.
				_DrawDots(pRT, xMiddleLine, rcItem.top, rcItem.Height(), true);
			}
			// Draw horizontal line from the middle until text begin.
			x2 = xMiddleLine + (isLastChild ? 0 : 2);
			_DrawDots(pRT, x2, yMiddleLine, xTxtLeft - x2 - 4, false);
		}
		for (int lower_level = level - 1; lower_level > 0; --lower_level)
		{
			xMiddleLine -= m_cxLevelMargin;	// Draw vertial dots line.
			if (drawVlines.GetFlag((uint8_t)lower_level))
			{
				_DrawDots(pRT, xMiddleLine, rcItem.top, rcItem.Height(), true);
			}
		}
		if (level > 0)
		{
			drawVlines.Set((uint8_t)level, !isLastChild);
		}
	}

	void _DrawDots(ID2D1RenderTarget* pRT, int x, int y, int len, bool isVertical)
	{
		D2D1_POINT_2F pt1{ (float)x, (float)y }, pt2{ (float)x, (float)(y + len) };
		if (!isVertical)
		{
			pt2.x = (float)(x + len);
			pt2.y = (float)y;
		}
		pRT->DrawLine(pt1, pt2, GetBrush(m_dotsColorId), 1.0f, m_strokeStyleDots.Get());
	}
#pragma endregion Painting

#pragma region Focus
	// WM_SETFOCUS message handler.
	virtual LRESULT _OnSetFocus(HWND hOldWnd) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_hasFocus = true;
		_RedrawDirectly();
		_NotifyParent(NM_SETFOCUS);
		return 0;
	}

	// WM_KILLFOCUS message handler.
	virtual LRESULT _OnKillFocus(HWND hNewWnd) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		m_hasFocus = false;
		_RedrawDirectly();
		_NotifyParent(NM_KILLFOCUS);
		return 0;
	}

	// Q83302: HOWTO: Use the WM_GETDLGCODE Message
	// https://jeffpar.github.io/kbarchive/kb/083/Q83302/
	// Q104637: HOWTO: Trap Arrow Keys in an Edit Control of a Dialog Box
	// https://jeffpar.github.io/kbarchive/kb/104/Q104637/
	// WM_GETDLGCODE message handler.
	virtual LRESULT _OnGetDlgCode(WPARAM wParam, LPARAM lParam) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		BOOL bHasTabStop = (::GetWindowLong(Hwnd(), GWL_STYLE) & WS_TABSTOP) ? TRUE : FALSE;

		LRESULT lResult = DLGC_WANTARROWS | DLGC_WANTCHARS;
		if (lParam)	// lParam points to an MSG structure?
		{
			LPMSG lpmsg = (LPMSG)lParam;
			if ((lpmsg->message == WM_KEYDOWN	// Keyboard input?
				|| lpmsg->message == WM_KEYUP)
				&& lpmsg->wParam != VK_TAB		// AND NOT TAB key?
				&& lpmsg->wParam != VK_ESCAPE)	// AND NOT ESC key?
			{
				if (bHasTabStop)				// 'this' window has Tab stop?
				{
					lResult |= DLGC_WANTMESSAGE;	// We want keyboard input (except TAB and ESC).
					// Note - windows will set focus to 'this' (and send WM_SETFOCUS)
					//        if we return DLGC_WANTMESSAGE here.
				}
			}
		}
		else
		{
			if (bHasTabStop)
				lResult |= DLGC_WANTTAB;	// 'this' has WS_TABSTOP style - we want focus.
			else
				lResult |= DLGC_STATIC;		// no tab stop - we don't want focus.
		}
		return lResult;
	}
#pragma endregion Focus

#pragma region Helpers
	void _UpdateHVsizeAndAdjustScrollBars(size_t item_count, int cxMax)
	{
		if (m_itemCount != item_count || m_cxMaxStringWidth != cxMax)
		{
			m_itemCount = item_count;
			m_cxMaxStringWidth = cxMax;
			_AdjustScrollBars();
			_RedrawDirectly();
		}
	}

	bool _SetSelectedPosition(size_t idx, UINT action)
	{
		if (idx < m_itemCount && idx != m_curIdx)
		{
			HWND hParentWnd = ::GetParent(Hwnd());
			NMTREEVIEWW nmt = _InitializeNMTREEVIEW(action, TVN_SELCHANGING, m_curIdx, idx);
			LRESULT result = ::SendMessage(hParentWnd, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&nmt);
			if (result)
			{
				return false;
			}
			if (!_IsVisible(idx))
			{
				_EnsureIsVisible(idx);
			}
			m_curIdx = idx;
			nmt.hdr.code = TVN_SELCHANGED;
			::SendMessage(hParentWnd, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&nmt);
			_RedrawDirectly();
			return true;
		}
		return false;
	}

	LRESULT _NotifyParent(UINT nfCode)
	{
		NMHDR hdr{ 0 };
		_InitializeNMHDR(&hdr, nfCode);
		HWND hParentWnd = ::GetParent(Hwnd());
		return ::SendMessage(hParentWnd, WM_NOTIFY, hdr.idFrom, (LPARAM)&hdr);
	}

	bool _IsVisible(size_t idx) const
	{
		CRect rcClient = _GetRealClientRect();
		CRect rcItem = _GetItemRect(idx);
		return rcItem.top >= rcClient.top && rcItem.bottom <= rcClient.bottom;
	}

	CRect _GetRealClientRect() const
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		rcClient.right = m_hasVscroll ? rcClient.right - m_cxSysSbV : rcClient.right;
		rcClient.bottom = m_hasHscroll ? rcClient.bottom - m_cySysSbH : rcClient.bottom;
		return rcClient;
	}

	int _GetClientRightEdge() const
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		int xClientRight = m_hasVscroll ? rcClient.right - m_cxSysSbV : rcClient.right;
		return xClientRight;
	}

	void _EnsureIsVisible(size_t idx)
	{
		if (idx < m_topIdx)
		{
			m_topIdx = idx;
		}
		else
		{
			while (m_topIdx < m_itemCount)
			{
				if (_IsVisible(idx))
				{
					break;
				}
				++m_topIdx;
			}
		}
		if (m_vScollBar)
		{
			m_vScollBar->SetScrollPos((int)m_topIdx, TRUE);
		}
	}

	CRect _GetButtonBox(const CRect& rcTxt, int level)
	{
		CRect rcBox = rcTxt;
		rcBox.left += m_cxBoxMargin + (level * m_cxLevelMargin);
		rcBox.right = rcBox.left + m_cxyBox;
		rcBox.top += m_cyBoxMargin;
		rcBox.bottom = rcBox.top + m_cxyBox;
		return rcBox;
	}

	CRect _GetItemRect(size_t idx) const
	{
		int y = m_y_start + (m_cyItem * (int)(idx - m_topIdx));
		return CRect(0, y, _GetClientRightEdge(), y + m_cyItem);
	}

	int _CalculateTextLeftEdge(int level)
	{
		return m_x_start + ((level + 1) * m_cxLevelMargin);
	}

	void _InitializeNMHDR(NMHDR* pNMHDR, UINT code)
	{
		pNMHDR->hwndFrom = Hwnd();
		pNMHDR->idFrom = GetDlgCtrlID();
		pNMHDR->code = code;
	}

	void _InitializeTVITEM(TVITEMW* pTVITEM, size_t idx)
	{
		const CXeTreeNode* pNode = m_tree.GetNodeAtIdx(idx);
		if (pNode)
		{
			_InitializeTVITEM(pTVITEM, m_tree.GetNodeHandle(pNode));
		}
	}

	void _InitializeTVITEM(TVITEMW* pTVITEM, HXETREEITEM hItem)
	{
		const CXeTreeNode* pNode = m_tree.GetNode(hItem);
		XeASSERT(pNode);
		pTVITEM->mask = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE;
		pTVITEM->hItem = reinterpret_cast<HTREEITEM>(hItem);
		pTVITEM->state = pNode && pNode->IsExpanded() ? TVIS_EXPANDED : 0;
		pTVITEM->stateMask = TVIS_EXPANDED;
		pTVITEM->lParam = pNode ? pNode->m_data.m_lParam : 0;
		if (pNode && !pNode->m_data.m_isGetTextCallback)
		{
			wchar_t* pStr = const_cast<wchar_t*>(pNode->m_data.m_string.c_str());
			pTVITEM->pszText = pStr;
			pTVITEM->cchTextMax = (int)pNode->m_data.m_string.size();
			pTVITEM->mask |= TVIF_TEXT;
		}
	}

	NMTREEVIEWW _InitializeNMTREEVIEW(UINT action, UINT code, size_t oldIdx, size_t newIdx)
	{
		NMTREEVIEWW nmt{ 0 };
		_InitializeNMHDR(&nmt.hdr, code);
		nmt.action = action;
		_InitializeTVITEM(&nmt.itemOld, oldIdx);
		_InitializeTVITEM(&nmt.itemNew, newIdx);
		return nmt;
	}

	LRESULT _SendItemExpandingNotify(const CXeTreeNode* pNode, bool isExpanded)
	{
		HXETREEITEM hItem = m_tree.GetNodeHandle(pNode);
		NMTREEVIEWW nmt{ 0 };
		_InitializeNMHDR(&nmt.hdr, TVN_ITEMEXPANDING);
		nmt.action = isExpanded ? TVE_EXPAND : TVE_COLLAPSE;
		_InitializeTVITEM(&nmt.itemNew, hItem);
		nmt.itemNew.state = isExpanded ? TVIS_EXPANDED : 0;
		HWND hParentWnd = ::GetParent(Hwnd());
		return ::SendMessage(hParentWnd, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&nmt);
	}

	bool _hasNodeChildren(const CXeTreeNode* pNode) const
	{
		return pNode->m_data.m_hasChildren || pNode->HasChildren();
	}

	void _DeleteAllItems()
	{
		m_tree.DeleteRoot();	// Delete all
		m_vScollBar.reset();
		m_hScollBar.reset();
		m_hasVscroll = m_hasHscroll = false;
		m_curIdx = 0xFFFFFFFF;
		m_topIdx = m_itemCount = 0;
	}
#pragma endregion Helpers

#pragma region Mouse
	// WM_LBUTTONDOWN message handler.
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint pt) override
	{
		return _HandleMouseMessage(MMsg::LDown, nFlags, pt);
	}

	// WM_LBUTTONUP message handler.
	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint pt) override
	{
		return _HandleMouseMessage(MMsg::LUp, nFlags, pt);
	}

	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint pt) override
	{
		return _HandleMouseMessage(MMsg::LDblClick, nFlags, pt);
	}

	// WM_RBUTTONDOWN message handler.
	virtual LRESULT _OnRightDown(UINT nFlags, CPoint pt) override
	{
		return _HandleMouseMessage(MMsg::RDown, nFlags, pt);
	}

	// WM_RBUTTONUP message handler.
	virtual LRESULT _OnRightUp(UINT nFlags, CPoint pt) override
	{
		return _HandleMouseMessage(MMsg::RUp, nFlags, pt);
	}

	virtual LRESULT _OnRightDoubleClick(UINT nFlags, CPoint pt) override
	{
		return _HandleMouseMessage(MMsg::RDblClick, nFlags, pt);
	}

	enum class MMsg { LDown, LUp, LDblClick, RDown, RUp, RDblClick };
	LRESULT _HandleMouseMessage(MMsg mmsg, UINT nFlags, CPoint point)
	{
		UINT nfCode = mmsg == MMsg::LDown ? NM_CLICK : mmsg == MMsg::LDblClick ? NM_DBLCLK :
				mmsg == MMsg::RDown ? NM_RCLICK : mmsg == MMsg::RDblClick ? NM_RDBLCLK : 0;
		size_t idx = m_cyItem > 0 ? m_topIdx + (point.y / m_cyItem) : (size_t)-1;
		const CXeTreeNode* pNode = m_tree.GetNodeAtIdx(idx);
		CRect rcItem;
		bool isClickInExpandBox = false, isClickInItemTextArea = false;
		if (pNode)
		{
			rcItem = _GetItemRect(idx);
			int xOffset = _GetHscrollInfo().nPos;
			if (_hasNodeChildren(pNode))
			{
				CRect rcBox = _GetButtonBox(rcItem, pNode->GetLevel());
				rcBox.OffsetRect(-xOffset, 0);
				isClickInExpandBox = rcBox.PtInRect(point);
			}
			rcItem.left = _CalculateTextLeftEdge(pNode->GetLevel()) - xOffset;
			isClickInItemTextArea = rcItem.PtInRect(point);
			if ((mmsg == MMsg::LDown && isClickInExpandBox)
				|| (mmsg == MMsg::LDblClick && isClickInItemTextArea))
			{
				if (_SendItemExpandingNotify(pNode, !pNode->IsExpanded()) == 0)
				{
					m_tree.SetExpandedFlag(pNode, !pNode->IsExpanded());
					_UpdateHVsizeAndAdjustScrollBars(m_tree.GetVisibleNodeCount(), 0);
					_SetSelectedPosition(idx, TVC_BYMOUSE);
				}
				else
				{
					idx = (size_t)-1;
				}
			}
		}
		if (idx != (size_t)-1)
		{
			_SetSelectedPosition(idx, TVC_BYMOUSE);
			_RedrawDirectly();
			SetFocus();
		}
		if (isClickInItemTextArea && pNode && nfCode == NM_RCLICK
				&& m_getContextMenu && m_selectedContextMenuItem)
		{
			TVITEMW tvItem;
			_InitializeTVITEM(&tvItem, m_tree.GetNodeHandle(pNode));
			std::vector<ListBoxExItem> menuItems = m_getContextMenu(&tvItem);
			if (menuItems.size())
			{
				CXeMenu menu(m_xeUI, menuItems);
				CPoint ptMenu(point);
				ClientToScreen(&ptMenu);
				UINT selectedID = menu.ShowMenu(Hwnd(), ptMenu, 0);
				if (selectedID)
				{
					m_selectedContextMenuItem(selectedID, &tvItem);
				}
			}
		}
		if (nfCode)
		{
			if (_NotifyParent(nfCode) != 0) { return 0; }
		}
		return 0;
	}

	// WM_MOUSEWHEEL message handler.
	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint pt) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		short xPos = (short)pt.x;
		short yPos = (short)pt.y;

		WORD wSBcode = 0xFFFF;
		if (zDelta >= WHEEL_DELTA)
		{
			wSBcode = SB_LINEUP;
		}
		else if (zDelta <= -WHEEL_DELTA)
		{
			wSBcode = SB_LINEDOWN;
			zDelta = -zDelta;
		}
		if (wSBcode != 0xFFFF && m_vScollBar)
		{
			do
			{
				SendMessage(WM_VSCROLL, MAKELONG(wSBcode, 0), (LPARAM)m_vScollBar->Hwnd());
			} while ((zDelta -= WHEEL_DELTA) >= WHEEL_DELTA);
			SendMessage(WM_VSCROLL, MAKELONG(SB_ENDSCROLL, 0), (LPARAM)m_vScollBar->Hwnd());
		}
		return 0;	// Message was processed.
	}

	// WM_CANCELMODE message handler.
	//virtual LRESULT _OnCancelMode() override
	//{
	//	XeASSERT(::IsWindow(Hwnd()));

	//	// Need to handle WM_CANCELMODE message.
	//	// See -> http://support.microsoft.com/kb/74548/en-us
	//}
#pragma endregion Mouse

#pragma region Keyboard
protected:
	// WM_KEYDOWN message handler.
	virtual LRESULT _OnKeyDown(WPARAM wParam, LPARAM lParam) override
	{
		XeASSERT(::IsWindow(Hwnd()));
		size_t newIdx = 0xFFFFFFFF;
		SCROLLINFO si = _GetVscrollInfo();
		bool fCtrlKeyDown = (::GetKeyState(VK_CONTROL) & 0x8000) ? true : false;
		bool fShiftKeyDown = (::GetKeyState(VK_SHIFT) & 0x8000) ? true : false;
		bool fMenuKeyDown = (::GetKeyState(VK_MENU) & 0x8000) ? true : false;
		bool isNoShiftCtrlMenuKeyDown = !(fCtrlKeyDown || fShiftKeyDown || fMenuKeyDown);
		const CXeTreeNode* pNode = m_tree.GetNodeAtIdx(m_curIdx);
		bool isCurNodeExpanded = pNode ? pNode->IsExpanded() : false;
		bool hasCurNodeChildren = pNode ? _hasNodeChildren(pNode) : false;
		bool isExpandContractNode = false;
		switch (wParam)
		{
		case VK_PRIOR:
			newIdx = m_curIdx < si.nPage ? 0 : m_curIdx - si.nPage;
			break;

		case VK_NEXT:
			newIdx = m_curIdx + si.nPage > m_itemCount - 1 ? m_itemCount - 1 : m_curIdx + si.nPage;
			break;

		case VK_HOME:
			newIdx = 0;
			break;

		case VK_END:
			newIdx = m_itemCount - 1;
			break;

		case VK_LEFT:
		case VK_RIGHT:
			if (pNode && isNoShiftCtrlMenuKeyDown)
			{
				bool isLeft = wParam == VK_LEFT;
				if (hasCurNodeChildren)
				{
					if (isCurNodeExpanded)
					{
						if (isLeft)
						{
							isExpandContractNode = true; // Left btn = contract node
						}
						else
						{
							// Right btn = goto first child (when already expanded)
							newIdx = m_curIdx + 1;
						}
					}
					else
					{
						if (isLeft)
						{
							// Left btn = goto parent node (when cur. node contracted)
							newIdx = m_tree.GetIndexOfNode(pNode->GetParentNode());
						}
						else
						{
							isExpandContractNode = true; // expand node
						}
					}
				}
				else if (isLeft)
				{
					// Left btn = goto parent node (when cur. node has no children)
					newIdx = m_tree.GetIndexOfNode(pNode->GetParentNode());
				}
			}
			break;

		case VK_UP:
			newIdx = m_curIdx - 1;
			break;

		case VK_DOWN:
			newIdx = m_curIdx + 1;
			break;
		}

		if (isExpandContractNode
			&& _SendItemExpandingNotify(pNode, !pNode->IsExpanded()) == 0)
		{
			m_tree.SetExpandedFlag(pNode, !pNode->IsExpanded());
			_UpdateHVsizeAndAdjustScrollBars(m_tree.GetVisibleNodeCount(), 0);
		}

		_SetSelectedPosition(newIdx, TVC_BYKEYBOARD);

		return 0;	// Indicate we processed this message (eat all key msgs).
		// Note - testing shows that windows looks for another child control to process
		// keyboard input if we don't return 0 here (and we lose focus).
	}

	// WM_KEYUP message handler.
	virtual LRESULT _OnKeyUp(WPARAM wParam, LPARAM lParam) override
	{
		return 0;	// Indicate we processed this message (eat all key msgs).
	}
#pragma endregion Keyboard

#pragma region Scrolling
protected:
	void _AdjustScrollBars()
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		CRect rcCopy(rcClient);
		size_t itemsInViewCount = rcCopy.Height() / m_cyItem;
		m_hasVscroll = m_itemCount > itemsInViewCount;
		if (m_hasVscroll)
		{
			rcCopy.right -= m_cxSysSbV;
		}
		m_hasHscroll = m_cxMaxStringWidth > rcCopy.Width();
		if (m_hasHscroll)
		{
			rcCopy.bottom -= m_cySysSbH;
			itemsInViewCount = rcCopy.Height() / m_cyItem;
			if (!m_hasVscroll)
			{
				m_hasVscroll = m_itemCount > itemsInViewCount;
			}
		}

		_AdjustVscroll(rcClient, (int)itemsInViewCount);
		_AdjustHscroll(rcClient, rcCopy.Width());
	}

	void _AdjustVscroll(const CRect& rcClient, int nPage)
	{
		if (m_hasVscroll)
		{
			CRect rcVSB(rcClient);
			rcVSB.bottom = m_hasHscroll ? rcClient.bottom - m_cySysSbH : rcClient.bottom;
			rcVSB.left = rcVSB.right - m_cxSysSbV;
			if (m_vScollBar)
			{
				m_vScollBar->ShowWindow(SW_SHOW);
				m_vScollBar->SetWindowPos(nullptr, rcVSB.left, rcVSB.top, rcVSB.Width(), rcVSB.Height(),
					SWP_NOCOPYBITS | SWP_NOZORDER);
			}
			else
			{
				m_vScollBar = std::make_unique<CXeScrollBar>(m_xeUI);
				m_vScollBar->Create(WS_CHILD | WS_VISIBLE | SBS_VERT, rcVSB, Hwnd(), c_VSB_ID, nullptr);
			}
			SCROLLINFO si{ 0 };
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = (int)m_itemCount - 1;
			si.nPage = nPage;
			si.nPos = (int)m_topIdx;
			si.nTrackPos = 0;
			m_vScollBar->SetScrollInfo(&si, TRUE);
		}
		else
		{
			if (m_vScollBar)
			{
				m_vScollBar->ShowWindow(SW_HIDE);
			}
		}
	}

	SCROLLINFO _GetVscrollInfo() const
	{
		SCROLLINFO si{ 0 };
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nPage = 1;
		if (m_vScollBar && m_hasVscroll)
		{
			m_vScollBar->GetScrollInfo(&si);
		}
		return si;
	}

	virtual LRESULT _OnVScroll(WPARAM wParam, LPARAM lParam) override
	{
		UINT nSBCode = LOWORD(wParam);
		UINT nPos = HIWORD(wParam);
		HWND hSBwnd = (HWND)lParam;
		UINT uSB_ID = ::GetDlgCtrlID(hSBwnd);
		if (m_vScollBar && uSB_ID == m_vScollBar->GetDlgCtrlID())	// our scrollbar?
		{
			SCROLLINFO si = _GetVscrollInfo();

			size_t topIdx = 0xFFFFFFFF;
			switch (nSBCode)
			{
			case SB_LINELEFT:
				topIdx = m_topIdx > 0 ? m_topIdx - 1 : 0;
				break;

			case SB_LINERIGHT:
				topIdx = m_topIdx + 1;
				break;

			case SB_PAGELEFT:
				topIdx = m_topIdx >= si.nPage ? m_topIdx - si.nPage : 0;
				break;

			case SB_PAGERIGHT:
				topIdx = m_topIdx + si.nPage;
				break;

			case SB_THUMBTRACK:
				topIdx = si.nTrackPos;
				break;
			}
			if (topIdx < m_itemCount)
			{
				size_t topIdxMax = m_itemCount - si.nPage;
				m_topIdx = topIdx > topIdxMax ? topIdxMax : topIdx;
				m_vScollBar->SetScrollPos((int)m_topIdx, TRUE);
				_RedrawDirectly();
			}
		}
		return 0;
	}

	void _AdjustHscroll(const CRect& rcClient, int nPage)
	{
		if (m_hasHscroll)
		{
			CRect rcHSB(rcClient);
			rcHSB.top = rcHSB.bottom - m_cySysSbH;
			rcHSB.right = m_hasVscroll ? rcClient.right - m_cxSysSbV : rcClient.right;
			if (m_hScollBar)
			{
				m_hScollBar->ShowWindow(SW_SHOW);
				m_hScollBar->SetWindowPos(nullptr, rcHSB.left, rcHSB.top, rcHSB.Width(), rcHSB.Height(),
					SWP_NOCOPYBITS | SWP_NOZORDER);
			}
			else
			{
				m_hScollBar = std::make_unique<CXeScrollBar>(m_xeUI);
				m_hScollBar->Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, rcHSB, Hwnd(), c_HSB_ID, nullptr);
			}
			SCROLLINFO si{ 0 };
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = m_cxMaxStringWidth;
			si.nPage = nPage;
			si.nPos = (int)0;
			si.nTrackPos = 0;
			m_hScollBar->SetScrollInfo(&si, TRUE);
		}
		else
		{
			if (m_hScollBar)
			{
				m_hScollBar->ShowWindow(SW_HIDE);
			}
		}
	}

	SCROLLINFO _GetHscrollInfo() const
	{
		SCROLLINFO si{ 0 };
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nPage = 1;
		if (m_hScollBar && m_hasHscroll)
		{
			m_hScollBar->GetScrollInfo(&si);
		}
		return si;
	}

	virtual LRESULT _OnHScroll(WPARAM wParam, LPARAM lParam) override
	{
		UINT nSBCode = LOWORD(wParam);
		UINT nPos = HIWORD(wParam);
		HWND hSBwnd = (HWND)lParam;
		UINT uSB_ID = ::GetDlgCtrlID(hSBwnd);
		if (m_hScollBar && uSB_ID == m_hScollBar->GetDlgCtrlID())	// our scrollbar?
		{
			SCROLLINFO si = _GetHscrollInfo();

			int newX = si.nPos;
			switch (nSBCode)
			{
			case SB_LINELEFT:
				newX -= 10;
				break;

			case SB_LINERIGHT:
				newX += 10;
				break;

			case SB_PAGELEFT:
				newX -= si.nPage;
				break;

			case SB_PAGERIGHT:
				newX += si.nPage;
				break;

			case SB_THUMBTRACK:
				newX = si.nTrackPos;
				break;
			}
			int maxX = m_cxMaxStringWidth - (int)si.nPage;
			if (newX < 0)
			{
				newX = 0;
			}
			else if (newX > maxX)
			{
				newX = maxX;
			}
			m_hScollBar->SetScrollPos(newX, TRUE);
			_RedrawDirectly();
		}
		return 0;
	}
#pragma endregion Scrolling

#pragma region TreeCtrl_message_handlers
protected:
	LRESULT _OnTreeControlMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case TVM_DELETEITEM:			return OnDeleteItemMsg(wParam, lParam);
		case TVM_EXPAND:				return OnExpandMsg(wParam, lParam);
		case TVM_GETCOUNT:				return OnGetCountMsg(wParam, lParam);
		case TVM_GETITEM:				return OnGetItemMsg(wParam, lParam);
		case TVM_GETNEXTITEM:			return OnGetNextItemMsg(wParam, lParam);
		case TVM_INSERTITEM:			return OnInsertItemMsg(wParam, lParam);
		case TVM_CREATEDRAGIMAGE:		
		case TVM_EDITLABEL:				
		case TVM_ENDEDITLABELNOW:		
		case TVM_ENSUREVISIBLE:			
		case TVM_GETBKCOLOR:			
		case TVM_GETEDITCONTROL:		
		case TVM_GETEXTENDEDSTYLE:		
		case TVM_GETIMAGELIST:			
		case TVM_GETINDENT:				
		case TVM_GETINSERTMARKCOLOR:	
		case TVM_GETISEARCHSTRING:		
		case TVM_GETITEMHEIGHT:			
		case TVM_GETITEMPARTRECT:		
		case TVM_GETITEMRECT:			
		case TVM_GETITEMSTATE:			
		case TVM_GETLINECOLOR:			
		case TVM_GETSCROLLTIME:			
		case TVM_GETSELECTEDCOUNT:		
		case TVM_GETTEXTCOLOR:			
		case TVM_GETTOOLTIPS:			
		case TVM_GETUNICODEFORMAT:		
		case TVM_GETVISIBLECOUNT:		
		case TVM_HITTEST:				
		case TVM_MAPACCIDTOHTREEITEM:	
		case TVM_MAPHTREEITEMTOACCID:	
		case TVM_SELECTITEM:			
		case TVM_SETAUTOSCROLLINFO:		
		case TVM_SETBKCOLOR:			
		case TVM_SETBORDER:				
		case TVM_SETEXTENDEDSTYLE:		
		case TVM_SETHOT:				
		case TVM_SETIMAGELIST:			
		case TVM_SETINDENT:				
		case TVM_SETINSERTMARK:			
		case TVM_SETINSERTMARKCOLOR:	
		case TVM_SETITEM:				
		case TVM_SETITEMHEIGHT:			
		case TVM_SETLINECOLOR:			
		case TVM_SETSCROLLTIME:			
		case TVM_SETTEXTCOLOR:			
		case TVM_SETTOOLTIPS:			
		case TVM_SETUNICODEFORMAT:		
		case TVM_SHOWINFOTIP:			
		case TVM_SORTCHILDREN:			
		case TVM_SORTCHILDRENCB:		
			return OnNotSupportedMsg(wParam, lParam);
		}
		XeASSERT(FALSE);	// Unknown tree message - need to add support?
		return 0;
	}

	/* Inserts a new item in a tree-view control.
	You can send this message explicitly or by using the TreeView_InsertItem macro.
	Parameters:
	wParam=Must be zero.
	lParam=Pointer to a TVINSERTSTRUCT structure that specifies the attributes of the tree-view item.
	Return value - HTREEITEM handle to the new item if successful, or NULL otherwise. */
	LRESULT OnInsertItemMsg(WPARAM wParam, LPARAM lParam)
	{
		TVINSERTSTRUCTW* pTVI = (TVINSERTSTRUCTW*)lParam;

		bool isRoot = pTVI->hParent == nullptr || pTVI->hParent == TVI_ROOT;
		TVITEM_mask mask(pTVI->item.mask);

		const wchar_t* pText = mask.isTVIF_TEXT() ? pTVI->item.pszText : nullptr;
		uint64_t param = mask.isTVIF_PARAM() ? pTVI->item.lParam : 0;
		TreeItemData item_data;
		if (mask.isTVIF_IMAGE())
		{
			item_data.SetImage((PID)pTVI->itemex.iImage);
		}
		if (mask.isTVIF_SELECTEDIMAGE())
		{
			item_data.SetSelectedImage((PID)pTVI->itemex.iSelectedImage);
		}
		if (mask.isTVIF_EXPANDEDIMAGE())
		{
			item_data.SetExpandedImage((PID)pTVI->itemex.iExpandedImage);
		}
		if (pTVI->itemex.iReserved != 0)
		{
			if (pTVI->itemex.iReserved < 0)
			{
				item_data.SetColorID((CID)(-pTVI->itemex.iReserved));
			}
			else
			{
				item_data.SetColor(pTVI->itemex.iReserved);
			}
		}
		TreeCtrlExItem item(pText, pTVI->item.cchTextMax, param, item_data);
		if (isRoot)
		{
			if (m_tree.CreateRoot(item))
			{
				return reinterpret_cast<LPARAM>(m_tree.GetRoot());
			}
			return 0;
		}
		else if (pTVI->hParent)
		{
			item.m_hasChildren = mask.isTVIF_CHILDREN() ? pTVI->item.cChildren > 0 : false;
			CXeTreeNode* pParent = reinterpret_cast<CXeTreeNode*>(pTVI->hParent);
			return reinterpret_cast<LPARAM>(m_tree.AddNode(pParent, item));
		}
		return 0;
	}

	/* Removes an item and all its children from a tree-view control.
	Parameters:
	wParam=Must be zero.
	lParam=HTREEITEM handle to the item to delete.
	If lParam is set to TVI_ROOT or to NULL, all items are deleted.
	Return value - TRUE if successful, or FALSE otherwise.
	Remarks: It is not safe to delete items in response to a notification such as TVN_SELCHANGING.
	Once an item is deleted, its handle is invalid and cannot be used.
	The parent window receives a TVN_DELETEITEM notification code when each item is removed.
	If the item label is being edited, the edit operation is canceled and the parent window receives
	the TVN_ENDLABELEDIT notification code.
	If you delete all items in a tree-view control that has the TVS_NOSCROLL style, items subsequently
	added may not display properly. For more information, see TreeView_DeleteAllItems. */
	LRESULT OnDeleteItemMsg(WPARAM wParam, LPARAM lParam)
	{
		if (lParam == 0 || lParam == (LPARAM)TVI_ROOT)
		{
			_DeleteAllItems();
		}
		else
		{
			XeASSERT(FALSE);	// Not supported.
		}
		return TRUE;
	}

	/* The TVM_EXPAND message expands or collapses the list of child items associated with
	the specified parent item, if any.
	Parameters:
	wParam=Action flag. This parameter can be one or more of the following values:
	Value				Meaning
	TVE_COLLAPSE		Collapses the list.
	TVE_COLLAPSERESET	Collapses the list and removes the child items. The TVIS_EXPANDEDONCE
	                    state flag is reset. This flag must be used with the TVE_COLLAPSE flag.
	TVE_EXPAND			Expands the list.
	TVE_EXPANDPARTIAL	Version 4.70. Partially expands the list. In this state the child
						items are visible and the parent item's plus sign (+), indicating
						that it can be expanded, is displayed. This flag must be used in
						combination with the TVE_EXPAND flag.
	TVE_TOGGLE			Collapses the list if it is expanded or expands it if it is collapsed.
	lParam=Handle to the parent item to expand or collapse.
	Return value - Returns nonzero if the operation was successful, or zero otherwise.
	Remarks: Expanding a node that is already expanded is considered a successful operation
	and SendMessage returns a nonzero value. Collapsing a node returns zero if the node is
	already collapsed; otherwise it returns nonzero. Attempting to expand or collapse a node
	that has no children is considered a failure and SendMessage returns zero.
	When an item is first expanded by a TVM_EXPAND message, the action generates
	TVN_ITEMEXPANDING and TVN_ITEMEXPANDED notification codes and the item's TVIS_EXPANDEDONCE
	state flag is set. As long as this state flag remains set, subsequent TVM_EXPAND messages
	do not generate TVN_ITEMEXPANDING or TVN_ITEMEXPANDED notifications.
	To reset the TVIS_EXPANDEDONCE state flag, you must send a TVM_EXPAND message with the
	TVE_COLLAPSE and TVE_COLLAPSERESET flags set. Attempting to explicitly set
	TVIS_EXPANDEDONCE will result in unpredictable behavior.
	The expand operation may fail if the owner of the treeview control denies the operation
	in response to a TVN_ITEMEXPANDING notification. */
	LRESULT OnExpandMsg(WPARAM wParam, LPARAM lParam)
	{
		CXeTreeNode* pNode = reinterpret_cast<CXeTreeNode*>(lParam);
		bool IsExpand = wParam & TVE_EXPAND;
		bool isCollapse = wParam & TVE_COLLAPSE;
		bool isToggle = wParam & TVE_TOGGLE;
		bool isNotSupported = (wParam & TVE_COLLAPSERESET) || (wParam & TVE_EXPANDPARTIAL);
		XeASSERT(!isNotSupported);
		m_tree.SetExpandedFlag(pNode, IsExpand ? true : isCollapse ? false
			: isToggle ? !pNode->IsExpanded() : pNode->IsExpanded());
		_SendItemExpandingNotify(pNode, pNode->IsExpanded());
		return TRUE;
	}

	/* Retrieves the tree-view item that bears the specified relationship to a specified item.
	Parameters:
	wParam=Flag specifying the item to retrieve.
	This parameter can be one of the following values:
	Value					Meaning
	TVGN_CARET				Retrieves the currently selected item.
	TVGN_CHILD				Retrieves the first child item of the item specified by the hitem
							parameter.
	TVGN_DROPHILITE			Retrieves the item that is the target of a drag-and-drop operation.
	TVGN_FIRSTVISIBLE		Retrieves the first item that is visible in the tree-view window.
	TVGN_LASTVISIBLE		Version 4.71. Retrieves the last expanded item in the tree.
							This does not retrieve the last item visible in the tree-view window.
	TVGN_NEXT				Retrieves the next sibling item.
	TVGN_NEXTSELECTED		Windows Vista and later. Retrieves the next selected item.
	TVGN_NEXTVISIBLE		Retrieves the next visible item that follows the specified item.
							The specified item must be visible. Use the TVM_GETITEMRECT message
							to determine whether an item is visible.
	TVGN_PARENT				Retrieves the parent of the specified item. 
	TVGN_PREVIOUS			Retrieves the previous sibling item.
	TVGN_PREVIOUSVISIBLE	Retrieves the first visible item that precedes the specified item.
							The specified item must be visible. Use the TVM_GETITEMRECT message
							to determine whether an item is visible.
	TVGN_ROOT				Retrieves the topmost or very first item of the tree-view control.
	lParam=Handle to an item.
	Return value - handle to the item if successful. For most cases, the message returns a NULL
	value to indicate an error. See the Remarks section for details.
	Remarks: This message will return NULL if the item being retrieved is the root node of the
	tree. For example, if you use this message with the TVGN_PARENT flag on a first-level
	child of the tree view's root node, the message will return NULL. */
	LRESULT OnGetNextItemMsg(WPARAM wParam, LPARAM lParam)
	{
		const CXeTreeNode* pNode = nullptr;
		if (wParam == TVGN_CARET)
		{
			pNode = m_tree.GetNodeAtIdx(m_curIdx);
		}
		else if (wParam == TVGN_ROOT)
		{
			pNode = m_tree.HasRoot() ? m_tree.GetRoot() : nullptr;
		}
		else
		{
			XeASSERT(FALSE);	// Not supported.
		}
		if (pNode)
		{
			return reinterpret_cast<LRESULT>(m_tree.GetNodeHandle(pNode));
		}
		return 0;
	}

	/* Retrieves some or all of a tree-view item's attributes.
	Parameters:
	wParam=Must be zero.
	lParam=Pointer to a TVITEM structure that specifies the information to retrieve and
	receives information about the item. With version 4.71 and later, you can use a TVITEMEX
	structure instead.
	Return value - TRUE if successful, or FALSE otherwise.
	Remarks: When the message is sent, the hItem member of the TVITEM or TVITEMEX structure
	identifies the item to retrieve information about, and the mask member specifies the
	attributes to retrieve.
	If the TVIF_TEXT flag is set in the mask member of the TVITEM or TVITEMEX structure,
	the pszText member must point to a valid buffer and the cchTextMax member must be set to
	the number of characters in that buffer. */
	LRESULT OnGetItemMsg(WPARAM wParam, LPARAM lParam)
	{
		if (lParam)
		{
			TVITEMW* pItem = reinterpret_cast<TVITEMW*>(lParam);
			_InitializeTVITEM(pItem, (HXETREEITEM)pItem->hItem);
			TVITEM_mask mask(pItem->mask);
			if (mask.isTVIF_IMAGE())
			{
				XeASSERT(FALSE);	// Not supported
			}
			if (mask.isTVIF_SELECTEDIMAGE())
			{
				XeASSERT(FALSE);	// Not supported
			}
			return TRUE;
		}
		return FALSE;
	}

	LRESULT OnGetCountMsg(WPARAM wParam, LPARAM lParam)
	{
		return m_tree.GetTotalNodeCount();
	}

	LRESULT OnNotSupportedMsg(WPARAM wParam, LPARAM lParam)
	{
		XeASSERT(FALSE);	// Not supported in this implementation of a listbox.
		return 0;
	}
#pragma endregion TreeCtrl_message_handlers

#pragma region MFC_functions
public:
	DWORD_PTR GetItemData(HTREEITEM hItem) const
	{
		XeASSERT(::IsWindow(Hwnd()));
		XeASSERT(hItem != NULL);
		TVITEM item;
		item.hItem = hItem;
		item.mask = TVIF_PARAM;
		::SendMessage(Hwnd(), TVM_GETITEM, 0, (LPARAM)&item);
		return item.lParam;
	}
#pragma endregion MFC_functions
};
