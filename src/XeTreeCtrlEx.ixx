module;

#include "os_minimal.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "XeAssert.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>

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

struct TreeUIparams
{
	int m_cyItem = 15;
	int m_x_start = 2, m_y_start = 0;

	int m_cxLevelMargin = 20, m_cxyBox = 9, m_cxyBoxSign = 5, m_cxBoxMargin = 4, m_cyBoxMargin = 5;
	int m_cxyBoxSignMargin = 0, m_cxyBoxMiddle = m_cxyBox / 2;
	int m_cxCurItemBgMargin = 2;
	int m_cxyImgDec = 2, m_cxImgMargin = 2;

	void Initialize(CXeUIcolorsIF* pUI)
	{
		const XeFontMetrics& tm = pUI->GetFontMetric(EXE_FONT::eUI_Font);
		m_cyItem = tm.GetLineHeight();
		if (m_cyItem == 0) { m_cyItem = 15; }
		if (!(m_cyItem & 1)) { ++m_cyItem; }
		UIScaleFactor sf = pUI->GetUIScaleFactor();
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
	}
};

struct TreeItemPaintData
{
	const CXeTreeNode*		m_pNode = nullptr;
	size_t					m_index = (size_t)-1;
	std::wstring			m_text;
	CRect					m_rcItem;				// Absolute client coords. (regardless of scrollbars position).
	bool					m_hasExpandButton = false;
	CRect					m_rcExpandButton;
	CRect					m_rcTxt;
	CRect					m_rcImage;
	PID						m_pid = PID::None;
	IWICFormatConverter*	m_pImg = nullptr;
	CSize					m_sizeImg;
	bool					m_isSelected = false;

	void CalculateRects(const CXeTreeNode* pNode, size_t idx,
			CXeUIcolorsIF* pUI, const TreeUIparams& uiParams)
	{
		m_pNode = pNode;
		m_index = idx;
		XeASSERT(m_pNode != nullptr && m_index >= 0);
		if (m_pNode == nullptr)
		{
			return;
		}

		int level = m_pNode->GetLevel();
		int yTop = uiParams.m_y_start + (uiParams.m_cyItem * (int)idx);
		m_rcItem.SetRect(0, yTop, 0, yTop + uiParams.m_cyItem);

		m_hasExpandButton = m_pNode->m_data.m_hasChildren || m_pNode->HasChildren();

		// Note - expand button box is always calculated - even when item does not have it.
		m_rcExpandButton = m_rcItem;
		m_rcExpandButton.left += uiParams.m_cxBoxMargin + (level * uiParams.m_cxLevelMargin);
		m_rcExpandButton.right = m_rcExpandButton.left + uiParams.m_cxyBox;
		m_rcExpandButton.top += uiParams.m_cyBoxMargin;
		m_rcExpandButton.bottom = m_rcExpandButton.top + uiParams.m_cxyBox;

		m_rcTxt = m_rcItem;
		m_rcTxt.left = uiParams.m_x_start + ((level + 1) * uiParams.m_cxLevelMargin);
		m_rcImage = m_rcTxt;
		m_rcImage.right = m_rcImage.left;
		CalculateImageRect(pUI, uiParams, false);

		m_rcTxt.right = m_rcTxt.left;
		m_rcItem.right = m_rcTxt.right;
	}

	void CalculateImageRect(CXeUIcolorsIF* pUI, const TreeUIparams& uiParams, bool isCurSelItem)
	{
		m_isSelected = isCurSelItem;
		int cxText = m_rcTxt.Width() > 0 ? m_rcTxt.Width() : 0;
		int xLeft = m_rcImage.left < m_rcTxt.left ? m_rcImage.left : m_rcTxt.left;
		m_rcImage.left = xLeft;
		m_rcImage.right = m_rcImage.left;
		m_rcImage.top = m_rcTxt.top + uiParams.m_cxyImgDec;
		m_rcImage.bottom = m_rcTxt.bottom - uiParams.m_cxyImgDec;
		m_pid = PID::None;
		if (m_pNode->m_data.m_data.HasImagePID())
		{
			m_pid = m_pNode->m_data.m_data.GetImagePID();
		}
		if (m_pNode->IsExpanded() && m_pNode->m_data.m_data.HasExpandedImagePID())
		{
			m_pid = m_pNode->m_data.m_data.GetExpandedImagePID();
		}

		if (isCurSelItem && m_pNode->m_data.m_data.HasSelectedImagePID())
		{
			m_pid = m_pNode->m_data.m_data.GetSelectedImagePID();
		}

		m_pImg = m_pid == PID::None ? nullptr : pUI->D2D_GetImage(m_pid, &m_sizeImg);
		if (m_pImg)
		{
			m_rcImage.right = m_rcImage.left + m_sizeImg.cx;
			int x = m_rcImage.left, y = m_rcImage.top, cx = m_rcImage.Width(), cy = m_rcImage.Height();
			int cxPng = m_sizeImg.cx;
			int cyPng = m_sizeImg.cy;
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
			m_rcImage.SetRect(x, y, x + cx, y + cy);

			m_rcTxt.left = m_rcImage.left + m_rcImage.Width() + uiParams.m_cxImgMargin;
			m_rcTxt.right = m_rcTxt.left + cxText;
		}
	}

	void CalculateTextRect(CXeUIcolorsIF* pUI)
	{
		CSize sizeText = pUI->GetTextSizeW(EXE_FONT::eUI_Font, m_text.c_str());
		m_rcTxt.right = m_rcTxt.left + sizeText.cx;
		m_rcItem.right = m_rcTxt.right;
	}

	// Note - during paint - 'this' object is copied - and then rects are offset by scroll pos H/V.
	void CalculateOffsetRects(int xHscrollPos, int yScrollPos)
	{
		m_rcItem.OffsetRect(-xHscrollPos, -yScrollPos);
		m_rcExpandButton.OffsetRect(-xHscrollPos, -yScrollPos);
		m_rcTxt.OffsetRect(-xHscrollPos, -yScrollPos);
		m_rcImage.OffsetRect(-xHscrollPos, -yScrollPos);
	}

	bool IsPointInTextArea(const CPoint& point, int yScrollPos, int xScrollPos) const
	{
		CRect rcTxt = GetTextAreaRect();
		rcTxt.OffsetRect(-xScrollPos, -yScrollPos);
		return rcTxt.PtInRect(point);
	}

	bool IsPointInExpandButton(const CPoint& point, int yScrollPos, int xScrollPos) const
	{
		CRect rcButton = m_rcExpandButton;
		rcButton.OffsetRect(-xScrollPos, -yScrollPos);
		return m_hasExpandButton && rcButton.PtInRect(point);
	}

	CRect GetTextAreaRect() const
	{
		CRect rc = m_rcTxt;
		rc.left = m_rcImage.left;
		return rc;
	}

	D2D1_RECT_F GetSelectedItemRect(const TreeUIparams& uiParams) const
	{
		CRect selRect(m_rcTxt);
		selRect.left = m_rcImage.left - uiParams.m_cxCurItemBgMargin;
		selRect.right += uiParams.m_cxCurItemBgMargin;
		D2D1_RECT_F selRectF = OffsetRectF(RectFfromRect(selRect), 0.5f, 0.5f);
		return selRectF;
	}

	COLORREF GetTextFgColor(CXeUIcolorsIF* pUI) const
	{
		COLORREF rgbFg = pUI->GetColor(m_isSelected ? CID::CtrlTxt : CID::CtrlCurItemTxt);
		if (m_pNode->m_data.m_data.m_isColorIdColor)
		{
			rgbFg = pUI->GetColor(m_pNode->m_data.m_data.GetColorID());
		}
		else if (m_pNode->m_data.m_data.m_hasColor)
		{
			rgbFg = m_pNode->m_data.m_data.m_rgbColor;
		}
		return rgbFg;
	}
};

struct TreePaintData
{
	// All visible nodes (i.e. nodes that are a child of expanded parent).
	std::vector<const CXeTreeNode*> m_visible_nodes;

	// Paint data for each visible node.
	// Note - m_visible_nodes and m_visible_nodes_paint_data have the same item count;
	std::vector<TreeItemPaintData> m_visible_nodes_paint_data;

	int m_cxMaxItemWidth = 0;

	size_t size() const
	{
		XeASSERT(m_visible_nodes.size() == m_visible_nodes_paint_data.size());
		return m_visible_nodes.size();
	}

	void clear()
	{
		m_visible_nodes.clear();
		m_visible_nodes_paint_data.clear();
		m_cxMaxItemWidth = 0;
	}

	size_t GetIndexOfFirstVisibleItem(int yClientTopWithScrollBarOffset) const
	{
		size_t idx = 0;
		for (; idx < m_visible_nodes_paint_data.size(); ++idx)
		{
			const TreeItemPaintData& pdata = m_visible_nodes_paint_data[idx];
			if (pdata.m_rcItem.top >= yClientTopWithScrollBarOffset
					|| pdata.m_rcItem.bottom > yClientTopWithScrollBarOffset)
			{
				return idx;
			}
		}
		return (size_t)-1;
	}

	CSize GetViewSize() const
	{
		if (m_visible_nodes_paint_data.size() > 0)
		{
			size_t lastIdx = m_visible_nodes_paint_data.size() - 1;
			int yLast = m_visible_nodes_paint_data[lastIdx].m_rcItem.bottom;
			return CSize(m_cxMaxItemWidth, yLast);
		}
		return CSize();
	}

	const TreeItemPaintData* GetItemFromYposition(int yPos, int yScrollPos) const
	{
		for (const TreeItemPaintData& item : m_visible_nodes_paint_data)
		{
			CRect rcItem = item.m_rcItem;
			rcItem.OffsetRect(0, -yScrollPos);
			if (yPos >= rcItem.top && yPos < rcItem.bottom)
			{
				return &item;
			}
		}
		return nullptr;
	}

	const TreeItemPaintData* GetItemAt(size_t idx) const
	{
		if (idx < m_visible_nodes_paint_data.size())
		{
			return &(m_visible_nodes_paint_data[idx]);
		}
		return nullptr;
	}

	const TreeItemPaintData* FindNode(const CXeTreeNode* pNode) const
	{
		for (const TreeItemPaintData& item : m_visible_nodes_paint_data)
		{
			if (item.m_pNode == pNode)
			{
				return &item;
			}
		}
		return nullptr;
	}
};

enum class NodeExpand { None = 0, Expand, Collapse, Toggle };

export class CXeTreeCtrlEx : public CXeD2DCtrlBase
{
protected:
	bool m_hasFocus = false, m_isEnabled = true;

	std::unique_ptr<CXeScrollBar> m_vScollBar;
	std::unique_ptr<CXeScrollBar> m_hScollBar;
	bool m_hasVscroll = false, m_hasHscroll = false;
	int m_cxSysSbV = 10, m_cySysSbH = 10;
	CRect m_rcHSB, m_rcVSB;

	CXeNaryTree<TreeCtrlExItem> m_tree;

	TreePaintData m_paint_data;

	size_t m_curIdx = 0xFFFFFFFF;

	TreeUIparams m_uiParams;
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
		m_uiParams.Initialize(m_xeUI);
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
	void _CalculatePaintDataAndRedraw()
	{
		_CalculatePaintData();
		_AdjustScrollBars();
		_RedrawDirectly();
	}

	void _CalculatePaintData()
	{
		m_paint_data.clear();
		const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);

		m_paint_data.m_visible_nodes = m_tree.GetVisibleNodesIdxRange(0, ((size_t)-1));
		m_paint_data.m_visible_nodes_paint_data.resize(m_paint_data.m_visible_nodes.size());
		HWND hParentWnd = ::GetParent(Hwnd());
		size_t idx = 0;
		for (const CXeTreeNode* pNode : m_paint_data.m_visible_nodes)
		{
			TreeItemPaintData& pdata = m_paint_data.m_visible_nodes_paint_data[idx];

			pdata.CalculateRects(pNode, idx, m_xeUI, m_uiParams);

			++idx;
		}
	}

	void _GetTextAndCalculateRect(HWND hParentWnd, TreeItemPaintData& pdata)
	{
		if (pdata.m_pNode->m_data.m_isGetTextCallback)
		{
			NMTVDISPINFOW di{ 0 };
			di.hdr.hwndFrom = Hwnd();
			di.hdr.idFrom = GetDlgCtrlID();
			di.hdr.code = TVN_GETDISPINFO;
			di.item.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
			di.item.hItem = reinterpret_cast<HTREEITEM>(m_tree.GetNodeHandle(pdata.m_pNode));
			di.item.state = pdata.m_pNode->IsExpanded() ? TVIS_EXPANDED : 0;
			di.item.stateMask = TVIS_EXPANDED;
			di.item.lParam = pdata.m_pNode->m_data.m_lParam;
			::SendMessage(hParentWnd, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&di);
			if (di.item.pszText)
			{
				pdata.m_text = di.item.pszText;
			}
		}
		else
		{
			pdata.m_text = pdata.m_pNode->m_data.m_string;
		}
		pdata.CalculateTextRect(m_xeUI);

		if (pdata.m_rcTxt.right > m_paint_data.m_cxMaxItemWidth)
		{
			m_paint_data.m_cxMaxItemWidth = pdata.m_rcTxt.right;
		}
	}

	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override
	{
		HWND hParentWnd = ::GetParent(Hwnd());
		int cxMaxItemWidthBeforePaint = m_paint_data.m_cxMaxItemWidth;
		int xOffset = _GetHscrollInfo().nPos;
		int yOffset = _GetVscrollInfo().nPos;
		pRT->Clear(m_xeUI->GetColorF(CID::CtrlBg)); // Fill background
		BoolFlags64 drawVlines;
		size_t idx = m_paint_data.GetIndexOfFirstVisibleItem(yOffset);
		for (; idx < m_paint_data.size(); ++idx)
		{
			TreeItemPaintData& pdata_unchanged = m_paint_data.m_visible_nodes_paint_data[idx];
			_GetTextAndCalculateRect(hParentWnd, pdata_unchanged);

			// Copy item (we need to apply scroll offsets).
			TreeItemPaintData pdata = pdata_unchanged;
			pdata.CalculateOffsetRects(xOffset, yOffset);

			if (!drawVlines.GetFlag(0))
			{
				drawVlines = _InitializeDrawVlinesForFirstNodeInView(pdata.m_pNode);
			}

			if (pdata.m_isSelected && m_isEnabled)
			{
				D2D1_RECT_F selRectF = pdata.GetSelectedItemRect(m_uiParams);
				pRT->FillRectangle(selRectF, GetBrush(CID::CtrlCurItemBg));
				if (m_hasFocus)
				{
					_DrawFocusRect(pRT, selRectF);
				}
			}

			_DrawExpandButtonAndVlines(pRT, pdata, drawVlines);

			if (pdata.m_pImg)
			{
				_DrawImage(pRT, pdata.m_pImg, RectFfromRect(pdata.m_rcImage), true, m_isEnabled, false);
			}

			COLORREF rgbFg = m_isEnabled ? pdata.GetTextFgColor(m_xeUI) : m_xeUI->GetColor(CID::CtrlTxtDis);
			_CreateAndDrawTextLayout(pRT, pdata.m_text, RectFfromRect(pdata.m_rcTxt), rgbFg);

			if (pdata.m_rcItem.bottom > rcClient.bottom)
			{
				break;	// No more visible items.
			}
		}
		if (m_hasHscroll && m_hasVscroll)
		{
			CRect rcCorner = m_rcHSB;
			rcCorner.left = rcCorner.right;
			rcCorner.right = m_rcVSB.right;
			pRT->FillRectangle(RectFfromRect(rcCorner), GetBrush(CID::CtrlBg));
		}
		if (m_control_has_border)
		{
			rcClient.left += 0.5f;
			rcClient.right -= 0.5f;
			rcClient.top += 0.5f;
			rcClient.bottom -= 0.5f;
			pRT->DrawRectangle(rcClient, GetBrush(CID::CtrlBorderThin));
		}
		if (cxMaxItemWidthBeforePaint != m_paint_data.m_cxMaxItemWidth)
		{
			_AdjustScrollBars();
		}
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

	void _DrawExpandButtonAndVlines(ID2D1RenderTarget* pRT, const TreeItemPaintData& pdata, BoolFlags64& drawVlines)
	{
		const CXeTreeNode* pNode = pdata.m_pNode;
		D2D1_RECT_F rcItem = OffsetRectF(RectFfromRect(pdata.m_rcItem), 0.5f, 0.5f);
		D2D1_RECT_F rcBox = OffsetRectF(RectFfromRect(pdata.m_rcExpandButton), 0.5f, 0.5f);
		D2D1_RECT_F rcImage = OffsetRectF(RectFfromRect(pdata.m_rcImage), 0.5f, 0.5f);
		bool isLastChild = pNode->IsLastChild();
		int level = pNode->GetLevel();
		float xMiddleLine = rcBox.left + m_uiParams.m_cxyBoxMiddle;
		float yMiddleLine = rcBox.top + m_uiParams.m_cxyBoxMiddle;
		float xTxtLeft = rcImage.left;
		float x1 = rcBox.left, x2 = rcBox.right - 1, y1 = rcBox.top, y2 = rcBox.bottom - 1;
		if (_hasNodeChildren(pNode))	// Draw expand box button?
		{
			pRT->DrawRectangle(rcBox, GetBrush(m_boxOutlineColorId));
			if (!pNode->IsRootNode())
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
			x1 += m_uiParams.m_cxyBoxSignMargin;
			pRT->DrawLine({ (float)x1, (float)yMiddleLine }, { (float)(x1 + m_uiParams.m_cxyBoxSign), (float)yMiddleLine },
					GetBrush(m_boxSignColorId));			// Draw the '-'.
			if (!pNode->IsExpanded())
			{
				x2 = xMiddleLine;
				y2 = rcBox.top + m_uiParams.m_cxyBoxSignMargin;
				pRT->DrawLine({ (float)x2, (float)y2 }, { (float)x2, (float)(y2 + m_uiParams.m_cxyBoxSign) },
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
				_DrawDots(pRT, xMiddleLine, rcItem.top, HeightOf(rcItem), true);
			}
			// Draw horizontal line from the middle until text begin.
			x2 = xMiddleLine + (isLastChild ? 0 : 2);
			_DrawDots(pRT, x2, yMiddleLine, xTxtLeft - x2 - 4, false);
		}
		for (int lower_level = level - 1; lower_level > 0; --lower_level)
		{
			xMiddleLine -= m_uiParams.m_cxLevelMargin;	// Draw vertial dots line.
			if (drawVlines.GetFlag((uint8_t)lower_level))
			{
				_DrawDots(pRT, xMiddleLine, rcItem.top, HeightOf(rcItem), true);
			}
		}
		if (level > 0)
		{
			drawVlines.Set((uint8_t)level, !isLastChild);
		}
	}

	void _DrawDots(ID2D1RenderTarget* pRT, float x, float y, float len, bool isVertical)
	{
		D2D1_POINT_2F pt1{ x, y }, pt2{ x, (y + len) };
		if (!isVertical)
		{
			pt2.x = (x + len);
			pt2.y = y;
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
	bool _SetSelectedPosition(size_t idx, UINT action)
	{
		if (idx < m_paint_data.size())
		{
			HWND hParentWnd = ::GetParent(Hwnd());
			NMTREEVIEWW nmt = _InitializeNMTREEVIEW(action, TVN_SELCHANGING, m_curIdx, idx);
			LRESULT result = ::SendMessage(hParentWnd, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&nmt);
			if (result)
			{
				return false;
			}
			_EnsureIsVisible(idx);
			if (m_curIdx >= 0 && m_curIdx < m_paint_data.size())
			{
				TreeItemPaintData& pdata_unselected = m_paint_data.m_visible_nodes_paint_data[m_curIdx];
				pdata_unselected.CalculateImageRect(m_xeUI, m_uiParams, false);
			}
			TreeItemPaintData& pdata_selected = m_paint_data.m_visible_nodes_paint_data[idx];
			pdata_selected.CalculateImageRect(m_xeUI, m_uiParams, true);
			m_curIdx = idx;
			nmt.hdr.code = TVN_SELCHANGED;
			::SendMessage(hParentWnd, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&nmt);
			_RedrawDirectly();
			return true;
		}
		return false;
	}

	void _ExpandOrCollapseNode(const CXeTreeNode* pNode, NodeExpand expand, bool isSetAsSelected, UINT selected_action)
	{
		XeASSERT(pNode != nullptr && expand != NodeExpand::None);
		bool isExpandNode = expand == NodeExpand::Expand ? true : expand == NodeExpand::Collapse ? false
				: expand == NodeExpand::Toggle ? !pNode->IsExpanded() : false;
		if (_SendItemExpandingNotify(pNode, isExpandNode) == 0)
		{
			m_tree.SetExpandedFlag(pNode, isExpandNode);
			_CalculatePaintData();
			if (isSetAsSelected)
			{
				const TreeItemPaintData* pPData = m_paint_data.FindNode(pNode);
				XeASSERT(pPData);
				if (pNode)
				{
					_SetSelectedPosition(pPData->m_index, selected_action);
				}
			}
			else
			{
				_RedrawDirectly();
			}
		}
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

	void _EnsureIsVisible(size_t idx)
	{
		if (idx >= m_paint_data.size() || !m_hasVscroll)
		{
			return;
		}
		const TreeItemPaintData& pdata = m_paint_data.m_visible_nodes_paint_data[idx];
		CRect rcItem = pdata.m_rcItem;
		int nPos = _GetVscrollInfo().nPos;
		rcItem.OffsetRect(0, -nPos);
		CRect rcClient = _GetRealClientRect();
		int yOffsetTop = rcClient.top + rcItem.top;				// positive when item top is inside client rect.
		int yOffsetBottom = rcClient.bottom - rcItem.bottom;	// positive when item bottom is inside client rect.
		if (yOffsetTop < 0)
		{
			nPos += yOffsetTop;
		}
		else if (yOffsetBottom < 0)
		{
			nPos += std::abs(yOffsetBottom);
		}
		if (nPos != _GetVscrollInfo().nPos)
		{
			m_vScollBar->SetScrollPos(nPos, TRUE);
		}
	}

	LRESULT _NotifyParent(UINT nfCode)
	{
		NMHDR hdr{ 0 };
		_InitializeNMHDR(&hdr, nfCode);
		HWND hParentWnd = ::GetParent(Hwnd());
		return ::SendMessage(hParentWnd, WM_NOTIFY, hdr.idFrom, (LPARAM)&hdr);
	}

	CRect _GetRealClientRect() const
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		rcClient.right = m_hasVscroll ? rcClient.right - m_cxSysSbV : rcClient.right;
		rcClient.bottom = m_hasHscroll ? rcClient.bottom - m_cySysSbH : rcClient.bottom;
		return rcClient;
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

	bool _hasNodeChildren(const CXeTreeNode* pNode) const
	{
		return pNode->m_data.m_hasChildren || pNode->HasChildren();
	}

	void _DeleteAllItems()
	{
		m_tree.DeleteRoot();	// Delete all
		if (m_vScollBar->GetSafeHwnd())
		{
			m_vScollBar->ShowWindow(SW_HIDE);
		}
		if (m_hScollBar->GetSafeHwnd())
		{
			m_hScollBar->ShowWindow(SW_HIDE);
		}
		m_hasVscroll = m_hasHscroll = false;
		m_curIdx = 0xFFFFFFFF;
		m_paint_data.clear();
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
		int yScrollPos = _GetVscrollInfo().nPos, xScrollPos = _GetHscrollInfo().nPos;
		const TreeItemPaintData* pItem = m_paint_data.GetItemFromYposition(point.y, yScrollPos);
		if (pItem)
		{
			bool isClickInItemTextArea = pItem->IsPointInTextArea(point, yScrollPos, xScrollPos);
			bool isClickInExpandBox = pItem->IsPointInExpandButton(point, yScrollPos, xScrollPos);

			if ((mmsg == MMsg::LUp && isClickInExpandBox)
					|| (mmsg == MMsg::LDblClick && isClickInItemTextArea))
			{
				_ExpandOrCollapseNode(pItem->m_pNode, NodeExpand::Toggle, true, TVC_BYMOUSE);
			}
			else if (mmsg == MMsg::LDown)
			{
				if (::GetFocus() != Hwnd())
				{
					SetFocus();
				}
				if (isClickInItemTextArea)
				{
					_SetSelectedPosition(pItem->m_index, TVC_BYMOUSE);
				}
			}
			else if (isClickInItemTextArea && nfCode == NM_RCLICK
					&& m_getContextMenu && m_selectedContextMenuItem)
			{
				TVITEMW tvItem;
				_InitializeTVITEM(&tvItem, m_tree.GetNodeHandle(pItem->m_pNode));
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
		size_t newIdx = 0xFFFFFFFF, itemCount = m_paint_data.size();
		SCROLLINFO si = _GetVscrollInfo();
		CXeShiftCtrlAltKeyHelper sca;
		size_t itemsInView = _GetRealClientRect().Height() / m_uiParams.m_cyItem;
		const CXeTreeNode* pNode = m_tree.GetNodeAtIdx(m_curIdx);
		bool isCurNodeExpanded = pNode ? pNode->IsExpanded() : false;
		bool hasCurNodeChildren = pNode ? _hasNodeChildren(pNode) : false;
		bool isExpandContractNode = false;
		switch (wParam)
		{
		case VK_PRIOR:
			newIdx = m_curIdx < itemsInView ? 0 : m_curIdx - itemsInView;
			break;

		case VK_NEXT:
			newIdx = (m_curIdx + itemsInView) > (itemCount - 1) ? (itemCount - 1) : (m_curIdx + itemsInView);
			break;

		case VK_HOME:
			newIdx = 0;
			break;

		case VK_END:
			newIdx = (itemCount - 1);
			break;

		case VK_LEFT:
		case VK_RIGHT:
			if (pNode && !sca.IsShiftOrCtrlOrAltDown())
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

		if (isExpandContractNode)
		{
			_ExpandOrCollapseNode(pNode, NodeExpand::Toggle, true, TVC_BYKEYBOARD);
		}
		else if (newIdx != 0xFFFFFFFF)
		{
			_SetSelectedPosition(newIdx, TVC_BYKEYBOARD);
		}

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
		CRect rcActualClient(rcClient);
		CSize sizeView = m_paint_data.GetViewSize();
		sizeView.cy += m_uiParams.m_cyItem;
		m_hasVscroll = sizeView.cy > rcActualClient.Height();
		if (m_hasVscroll)
		{
			rcActualClient.right -= m_cxSysSbV;
		}
		m_hasHscroll = sizeView.cx > rcActualClient.Width();
		if (m_hasHscroll)
		{
			rcActualClient.bottom -= m_cySysSbH;
			if (!m_hasVscroll)
			{
				m_hasVscroll = sizeView.cy > rcActualClient.Height();
			}
		}

		if (m_hasVscroll)
		{
			m_rcVSB = rcClient;
			m_rcVSB.bottom = m_hasHscroll ? rcClient.bottom - m_cySysSbH : rcClient.bottom;
			m_rcVSB.left = m_rcVSB.right - m_cxSysSbV;
			if (m_control_has_border)
			{
				++m_rcVSB.top;
				if (!m_hasHscroll)
				{
					--m_rcVSB.bottom;
				}
				--m_rcVSB.left;
				--m_rcVSB.right;
			}
			SCROLLINFO si{ 0 };
			si.cbSize = sizeof(SCROLLINFO);
			if (m_vScollBar)
			{
				m_vScollBar->ShowWindow(SW_SHOW);
				m_vScollBar->SetWindowPos(nullptr, m_rcVSB.left, m_rcVSB.top, m_rcVSB.Width(), m_rcVSB.Height(),
						SWP_NOCOPYBITS | SWP_NOZORDER);
				si.fMask = SIF_RANGE;
			}
			else
			{
				m_vScollBar = std::make_unique<CXeScrollBar>(m_xeUI);
				m_vScollBar->Create(WS_CHILD | WS_VISIBLE | SBS_VERT, m_rcVSB, Hwnd(), c_VSB_ID, nullptr);
				si.fMask = SIF_ALL;
				si.nPage = (rcActualClient.Height() - (m_control_has_border ? 2 : 0));
				si.nPos = (int)m_curIdx * m_uiParams.m_cyItem;
				si.nTrackPos = 0;
			}
			si.nMin = 0;
			si.nMax = sizeView.cy;
			m_vScollBar->SetScrollInfo(&si, TRUE);
		}
		else
		{
			m_rcVSB.SetRectEmpty();
			if (m_vScollBar)
			{
				m_vScollBar->ShowWindow(SW_HIDE);
			}
		}

		if (m_hasHscroll)
		{
			m_rcHSB = rcClient;
			m_rcHSB.top = m_rcHSB.bottom - m_cySysSbH;
			m_rcHSB.right = m_hasVscroll ? rcClient.right - m_cxSysSbV : rcClient.right;
			if (m_control_has_border)
			{
				++m_rcHSB.left;
				--m_rcHSB.right;
				--m_rcHSB.top;
				--m_rcHSB.bottom;
			}
			SCROLLINFO si{ 0 };
			si.cbSize = sizeof(SCROLLINFO);
			if (m_hScollBar)
			{
				m_hScollBar->ShowWindow(SW_SHOW);
				m_hScollBar->SetWindowPos(nullptr, m_rcHSB.left, m_rcHSB.top, m_rcHSB.Width(), m_rcHSB.Height(),
					SWP_NOCOPYBITS | SWP_NOZORDER);
				si.fMask = SIF_RANGE;
			}
			else
			{
				m_hScollBar = std::make_unique<CXeScrollBar>(m_xeUI);
				m_hScollBar->Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, m_rcHSB, Hwnd(), c_HSB_ID, nullptr);
				si.fMask = SIF_ALL;
				si.nPage = (rcActualClient.Width() - (m_control_has_border ? 2 : 0));
				si.nPos = (int)0;
				si.nTrackPos = 0;
			}
			si.nMin = 0;
			si.nMax = sizeView.cx;
			m_hScollBar->SetScrollInfo(&si, TRUE);
		}
		else
		{
			m_rcHSB.SetRectEmpty();
			if (m_hScollBar)
			{
				m_hScollBar->ShowWindow(SW_HIDE);
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
		HWND hSBwnd = (HWND)lParam;
		UINT uSB_ID = ::GetDlgCtrlID(hSBwnd);
		if (m_vScollBar && uSB_ID == m_vScollBar->GetDlgCtrlID())	// our scrollbar?
		{
			SCROLLINFO si = _GetVscrollInfo();
			UINT nPos = si.nPos;

			switch (nSBCode)
			{
			case SB_LINELEFT:
				nPos = nPos > (UINT)m_uiParams.m_cyItem ? nPos - m_uiParams.m_cyItem : 0;
				break;

			case SB_LINERIGHT:
				nPos += m_uiParams.m_cyItem;
				break;

			case SB_PAGELEFT:
				nPos = nPos >= si.nPage ? nPos - si.nPage : 0;
				break;

			case SB_PAGERIGHT:
				nPos += si.nPage;
				break;

			case SB_THUMBTRACK:
				nPos = si.nTrackPos;
				break;
			}
			if (nPos != si.nPos)
			{
				m_vScollBar->SetScrollPos(nPos, TRUE);
				_RedrawDirectly();
			}
		}
		return 0;
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
		HWND hSBwnd = (HWND)lParam;
		UINT uSB_ID = ::GetDlgCtrlID(hSBwnd);
		if (m_hScollBar && uSB_ID == m_hScollBar->GetDlgCtrlID())	// our scrollbar?
		{
			SCROLLINFO si = _GetHscrollInfo();
			UINT nPos = si.nPos;

			switch (nSBCode)
			{
			case SB_LINELEFT:
				nPos -= 10;
				break;

			case SB_LINERIGHT:
				nPos += 10;
				break;

			case SB_PAGELEFT:
				nPos -= si.nPage;
				break;

			case SB_PAGERIGHT:
				nPos += si.nPage;
				break;

			case SB_THUMBTRACK:
				nPos = si.nTrackPos;
				break;
			}
			if (nPos != si.nPos)
			{
				m_hScollBar->SetScrollPos(nPos, TRUE);
				_RedrawDirectly();
			}
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
		LRESULT result = 0;
		if (isRoot)
		{
			if (m_tree.CreateRoot(item))
			{
				result = reinterpret_cast<LRESULT>(m_tree.GetRoot());
			}
		}
		else if (pTVI->hParent)
		{
			item.m_hasChildren = mask.isTVIF_CHILDREN() ? pTVI->item.cChildren > 0 : false;
			CXeTreeNode* pParent = reinterpret_cast<CXeTreeNode*>(pTVI->hParent);
			result = reinterpret_cast<LPARAM>(m_tree.AddNode(pParent, item));
		}
		if (result != 0)
		{
			if (IsWindowVisible())
			{
				_CalculatePaintDataAndRedraw();
			}
		}
		return result;
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
			_CalculatePaintDataAndRedraw();
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
		NodeExpand expand =   ((wParam & TVE_EXPAND)   != 0) ? NodeExpand::Expand
							: ((wParam & TVE_COLLAPSE) != 0) ? NodeExpand::Collapse
							: ((wParam & TVE_TOGGLE)   != 0) ? NodeExpand::Toggle   : NodeExpand::Collapse;
		bool isNotSupported = (wParam & TVE_COLLAPSERESET) || (wParam & TVE_EXPANDPARTIAL);
		XeASSERT(!isNotSupported);
		_ExpandOrCollapseNode(pNode, expand, false, 0);
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
