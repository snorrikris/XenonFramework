module;

#include "os_minimal.h"
#include <string>
#include <cstring>
#include <functional>
#include <memory>
//#include "CustomWndMsgs.h"

export module Xe.ViewsListWnd;

import Xe.UserSettingsForUI;
import Xe.ScrollBar;
import Xe.FileVwIF;
import Xe.UIcolorsIF;
import Xe.D2DWndBase;
//import Xe.FileContainerUI_IF;
import Xe.Helpers;
//import Xe.LogDefs;
import Xe.DefData;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef std::function<void(dsid_t dwDataSourceId)> NotifyTabListWndDestroyedCallbackFunc;

constexpr int nIDC_SCROLLV = 31098;

constexpr int c_cxViewListIcon = 15;
constexpr int c_cxViewListIconMargin = 3;
constexpr int c_cxViewListColMargin = 10;
constexpr int c_nViewListMinNumItemsInColumn = 10;

constexpr int c_cxVL_ClientMargin = 10, c_cyVL_ClientMargin = 10;
constexpr int c_cyVL_PathMarginTop = 5;
constexpr int c_cyVL_PathMarginBottom = 5;

constexpr wchar_t XEVIEWSLISTWND_CLASSNAME[] = L"XeViewsListWndClass";  // Window class name

export class CViewsListWnd : public CXeD2DWndBase
{
	dsid_t m_dwDataSourceIdOfCurrentView;

	NotifyTabListWndDestroyedCallbackFunc m_notifyCallback = nullptr;

	CVwInfoList m_viewList;
	Iterator_VwInfoList m_iterCurItem;
	dsid_t m_dwSelItemDataSourceId;
	dsid_t m_dwMouseOverItemDataSourceId;

	bool m_hasScrollbar = false;
	std::unique_ptr<CXeScrollBar> m_scrollbarV;
	SCROLLINFO m_scrinfoV = { 0 };
	CRect m_rcSB;

	int m_cxClientNeeded = 0, m_cyClientNeeded = 0;
	int m_yFirstRow = 0, m_xFirstCol = 0, m_cxColumn = 0, m_nNumItemsInCol = 0;

	int m_cyTitle = 25;
	int m_cyPath = 20;
	int m_cyViewListRow = 20;
	int m_cxMaxName = 0, m_cxMaxPath = 0;;

public:
	CViewsListWnd(CXeUIcolorsIF* pUIcolors, HWND hParent, std::vector<CXeFileVwIF*>& allViews,
		CRect& rcMaxSize, dsid_t curViewId,
		NotifyTabListWndDestroyedCallbackFunc notifyDestroyedCallback)
		: CXeD2DWndBase(pUIcolors), m_notifyCallback(notifyDestroyedCallback), m_dwDataSourceIdOfCurrentView(curViewId)
	{
		XeASSERT(m_notifyCallback);

		m_xeUI->RegisterWindowClass(XEVIEWSLISTWND_CLASSNAME, D2DCtrl_WndProc);

		m_cyTitle = _GetViewsListTitleTextHeight();
		m_cyViewListRow = m_cyPath = _GetViewsListPathTextHeight();
		_MakeViewList(hParent, allViews);

		m_iterCurItem = std::find_if(m_viewList.begin(), m_viewList.end(), 
			[curViewId](const auto& item) { return item.m_dsid == curViewId; });

		_CalculateItems(hParent, rcMaxSize);

		int x = rcMaxSize.left + ((rcMaxSize.Width() / 2) - (m_cxClientNeeded / 2));
		int y = rcMaxSize.top;
		HWND hWnd = CreateD2DWindow(WS_EX_CONTROLPARENT, XEVIEWSLISTWND_CLASSNAME, nullptr, WS_POPUP | WS_VISIBLE,
			CRect(x, y, x + m_cxClientNeeded, y + m_cyClientNeeded), hParent, 0);
		// Note - OnSize has been called.
		m_scrollbarV = std::make_unique<CXeScrollBar>(m_xeUI);
		if (m_hasScrollbar)
		{
			m_scrollbarV->Create(WS_CHILD | WS_VISIBLE | SBS_VERT, m_rcSB, hWnd, nIDC_SCROLLV, nullptr);
			m_scrinfoV.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
			m_scrollbarV->SetScrollInfo(&m_scrinfoV, FALSE);
		}
	}
	//~CViewsListWnd();

protected:
	void _MakeViewList(HWND hParent, std::vector<CXeFileVwIF*>& allViews)
	{
		m_viewList.clear();
		m_cxMaxName = m_cxMaxPath = 0;
		std::vector<std::wstring> names, paths;
		for (CXeFileVwIF* pView : allViews)
		{
			CVwInfo item(pView);
			names.push_back(item.GetViewName());
			paths.push_back(item.GetPathName());
			CSize sizeTxt = m_xeUI->GetTextSizeW(EXE_FONT::eUI_Font, item.GetViewName().c_str());
			item.m_cxViewName = sizeTxt.cx;
			m_viewList.push_back(item);
		}
		m_cxMaxName = m_xeUI->GetMaxTextWidthUsingFontW(names, EXE_FONT::eUI_Font);
		m_cxMaxPath = m_xeUI->GetMaxTextWidthUsingFontW(paths, EXE_FONT::eUI_Font);
	}

	virtual LRESULT _OnKeyDown(WPARAM wParam, LPARAM lParam) override
	{
		if (!GetSafeHwnd())
		{
			return false;
		}
		if (wParam == VK_TAB)
		{
			CXeShiftCtrlAltKeyHelper sca;
			bool isListValid = m_iterCurItem != m_viewList.end();
			if (isListValid)
			{
				if (sca.IsOnlyShiftCtrlDown())	// Reverse direction iteration
				{
					if (m_iterCurItem == m_viewList.begin())
					{
						m_iterCurItem = m_viewList.end();
						m_iterCurItem--;
					}
					else
					{
						m_iterCurItem--;
					}
				}
				else if (sca.IsOnlyCtrlDown())
				{
					m_iterCurItem++;
					if (m_iterCurItem == m_viewList.end())
					{
						m_iterCurItem = m_viewList.begin();
					}
				}
				m_dwSelItemDataSourceId = m_iterCurItem->m_dsid;
				if (m_hasScrollbar)
				{
					int nCurItemRow = _GetCurItemRow();
					int nNewScrollPos = -1;
					if (nCurItemRow < m_scrinfoV.nPos)
					{
						nNewScrollPos = nCurItemRow;
					}
					else if (nCurItemRow > (m_scrinfoV.nPos + (int)m_scrinfoV.nPage - 1))
					{
						nNewScrollPos = nCurItemRow - m_scrinfoV.nPage + 1;
					}
					if (nNewScrollPos >= 0)
					{
						m_scrinfoV.nPos = nNewScrollPos;
						m_scrollbarV->SetScrollInfo(&m_scrinfoV);
						_CalculateItemsForScrollbarPos();
					}
				}
				_RedrawDirectly();
				return true;
			}
		}
		return 0;
	}
	virtual LRESULT _OnKeyUp(WPARAM wParam, LPARAM lParam) override
	{
		if (!GetSafeHwnd())
		{
			return false;
		}
		if (wParam == VK_CONTROL)
		{
			::DestroyWindow(Hwnd());
			return true;
		}
		return 0;
	}

	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		return CXeD2DWndBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	virtual LRESULT _OnNcDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam) override
	{
		LRESULT lResult = CXeD2DWndBase::_OnNcDestroy(hwnd, wParam, lParam);
		// DataSourceId of user selected item ( = 0 if user did not select).
		m_notifyCallback(m_dwSelItemDataSourceId);
		// Note - 'this' has been deleted - at this point.
		return lResult;
	}

	virtual LRESULT _OnKillFocus(HWND hNewWnd) override
	{
		::DestroyWindow(Hwnd());
		return 0;
	}

	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		dsid_t curVwId = m_iterCurItem != m_viewList.end() ? m_iterCurItem->m_dsid : dsid_t();
		_DrawViewsListWndUI(pRT, rc, m_viewList, curVwId, m_dwMouseOverItemDataSourceId);
	}

	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{
		dsid_t dwDataSourceIdAtPoint = _GetDataSourceOfItemAtPoint(point);
		if (dwDataSourceIdAtPoint != m_dwMouseOverItemDataSourceId)
		{
			m_dwMouseOverItemDataSourceId = dwDataSourceIdAtPoint;
			_RedrawDirectly();
		}
		return CXeD2DWndBase::_OnMouseMove(nFlags, point);
	}

	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override
	{
		if (m_dwMouseOverItemDataSourceId.is_valid())
		{
			m_dwMouseOverItemDataSourceId.reset();
			_RedrawDirectly();
		}
		return CXeD2DWndBase::_OnMouseLeave(wparam, lparam);
	}

	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint pt) override
	{
		dsid_t dwDataSourceIdAtPoint = _GetDataSourceOfItemAtPoint(pt);
		if (dwDataSourceIdAtPoint.is_valid())
		{
			m_dwSelItemDataSourceId = dwDataSourceIdAtPoint;
			_RedrawDirectly();
			::DestroyWindow(Hwnd());
		}
		return 0;
	}

	virtual LRESULT _OnVScroll(WPARAM wParam, LPARAM lParam) override
	{
		UINT nSBCode = LOWORD(wParam), nPos = HIWORD(wParam);
		HWND hWndSB = (HWND)lParam;
		UINT uSB_ID = ::GetDlgCtrlID(hWndSB);
		if (uSB_ID == m_scrollbarV->GetDlgCtrlID())	// our scrollbar?
		{
			m_scrinfoV.fMask = SIF_ALL;
			m_scrollbarV->GetScrollInfo(&m_scrinfoV);

			BOOL fPosChanged = TRUE;	// display redraw flag
			switch (nSBCode)
			{
			case SB_LINELEFT:
				m_scrinfoV.nPos -= 1;
				break;

			case SB_LINERIGHT:
				m_scrinfoV.nPos += 1;
				break;

			case SB_PAGELEFT:
				m_scrinfoV.nPos -= m_scrinfoV.nPage;
				break;

			case SB_PAGERIGHT:
				m_scrinfoV.nPos += m_scrinfoV.nPage;
				break;

			case SB_THUMBTRACK:
				m_scrinfoV.nPos = m_scrinfoV.nTrackPos;
				break;

			default:
				fPosChanged = FALSE;	// no display redraw
			}
			if (fPosChanged)
			{
				int nActualMax = 1 + (m_scrinfoV.nMax - m_scrinfoV.nPage);
				if (m_scrinfoV.nPos < m_scrinfoV.nMin)
					m_scrinfoV.nPos = m_scrinfoV.nMin;
				else if (m_scrinfoV.nPos > nActualMax)	// > max scroll value
					m_scrinfoV.nPos = nActualMax;
				m_scrinfoV.fMask = SIF_POS;
				m_scrollbarV->SetScrollInfo(&m_scrinfoV);
				_CalculateItemsForScrollbarPos();
				_RedrawDirectly();
			}
		}
		return 0;
	}

	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint pt) override
	{
		UINT nSBCode = zDelta > 0 ? SB_LINELEFT : SB_LINERIGHT;
		WPARAM wParam = MAKEWPARAM(nSBCode, 0);
		LPARAM lParam = (LPARAM)m_scrollbarV->GetSafeHwnd();
		_OnVScroll(wParam, lParam);
		return TRUE;
	}

	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam) override
	{
		SetCursor(m_xeUI->GetAppCursor(false));
		return TRUE;
	}

	dsid_t _GetDataSourceOfItemAtPoint(CPoint point)
	{
		for (CVwInfo& item : m_viewList)
		{
			if (item.m_rcPos.PtInRect(point))
			{
				return item.m_dsid;
			}
		}
		return dsid_t();
	}

	void _CalculateItems(HWND hParent, CRect& rcMaxSize)
	{
		// TODO: consider: what is max value for cxMaxName?
		m_cxColumn = c_cxViewListIcon + c_cxViewListIconMargin + m_cxMaxName;

		int cxMax = m_cxMaxPath > rcMaxSize.Width() ? m_cxMaxPath : rcMaxSize.Width();
		int nMaxCols = cxMax / (m_cxColumn + c_cxViewListColMargin);
		if (nMaxCols == 0)
		{
			nMaxCols = 1;
		}

		int nNumItems = (int)m_viewList.size();
		double numColsNeeded = (double)nNumItems / (double)c_nViewListMinNumItemsInColumn;
		int nNumCols = numColsNeeded > 2 ? 3 : numColsNeeded > 1 ? 2 : 1;
		if (nNumCols > nMaxCols)
		{
			nNumCols = nMaxCols;
		}
		m_nNumItemsInCol = nNumItems / nNumCols;
		if (nNumItems % nNumCols != 0)
		{
			m_nNumItemsInCol++;
		}
		if (m_nNumItemsInCol < c_nViewListMinNumItemsInColumn)
		{
			m_nNumItemsInCol = c_nViewListMinNumItemsInColumn;
		}

		m_yFirstRow = c_cyVL_ClientMargin + m_cyTitle + c_cyVL_PathMarginTop + m_cyPath + c_cyVL_PathMarginBottom;

		// Calculate needed client area size.
		int cxForColumns = (nNumCols * m_cxColumn) + ((nNumCols - 1) * c_cxViewListColMargin);
		m_cxClientNeeded = (cxForColumns > m_cxMaxPath ? cxForColumns : m_cxMaxPath) + (c_cxVL_ClientMargin * 2);

		int cyForColumns = m_nNumItemsInCol * m_cyViewListRow;
		m_cyClientNeeded = m_yFirstRow + cyForColumns + c_cyVL_ClientMargin;
		if (m_cyClientNeeded > rcMaxSize.Height())
		{
			m_hasScrollbar = true;
			int cxSBV = GetSystemMetrics(SM_CXVSCROLL);
			m_cxClientNeeded += cxSBV;

			int cyMax = rcMaxSize.Height();
			int cyForColsMin = m_cyViewListRow * 3;
			cyForColumns = cyMax - m_yFirstRow - c_cyVL_ClientMargin;
			if (cyForColumns < cyForColsMin)
			{
				cyForColumns = cyForColsMin;
			}
			else
			{
				cyForColumns = (cyForColumns / m_cyViewListRow) * m_cyViewListRow;
			}
			m_cyClientNeeded = m_yFirstRow + cyForColumns + c_cyVL_ClientMargin;

			m_scrinfoV.cbSize = sizeof(SCROLLINFO);
			m_scrinfoV.nMax = m_nNumItemsInCol;
			m_scrinfoV.nPage = cyForColumns / m_cyViewListRow;

			int nCurItemRow = _GetCurItemRow();
			if (nCurItemRow > (int)m_scrinfoV.nPage)
			{
				m_scrinfoV.nPos = nCurItemRow - m_scrinfoV.nPage + 1;
			}

			m_rcSB.left = m_cxClientNeeded - cxSBV - 1;
			m_rcSB.right = m_rcSB.left + cxSBV;
			m_rcSB.top = m_yFirstRow - 2;
			m_rcSB.bottom = m_cyClientNeeded - c_cyVL_ClientMargin;
		}

		m_xFirstCol = (m_cxClientNeeded - cxForColumns) / 2;

		_CalculateItemsForScrollbarPos();
	}

	void _CalculateItemsForScrollbarPos()
	{
		int nFirstVisibleRow = m_hasScrollbar ? m_scrinfoV.nPos : 0, nCurRow = 0;
		int nLastVisibleRow = m_hasScrollbar ? nFirstVisibleRow + m_scrinfoV.nPage - 1 : m_nNumItemsInCol + 1;
		int yStart = m_yFirstRow - (nFirstVisibleRow * m_cyViewListRow);
		int nItemInColCount = 0, x = m_xFirstCol, y = yStart;
		for (CVwInfo& item : m_viewList)
		{
			if (nCurRow >= nFirstVisibleRow && nCurRow <= nLastVisibleRow)
			{
				item.m_rcPos.SetRect(x, y, x + m_cxColumn, y + m_cyViewListRow);
			}
			else
			{
				item.m_rcPos.SetRectEmpty();
			}
			y += m_cyViewListRow;

			if (++nItemInColCount >= m_nNumItemsInCol)
			{
				nItemInColCount = 0;
				y = yStart;
				nCurRow = 0;
				x += (m_cxColumn + c_cxViewListColMargin);
			}
			else
			{
				nCurRow++;
			}
		}
	}

	int _GetCurItemRow()
	{
		bool isCurItemValid = m_iterCurItem != m_viewList.end();
		if (isCurItemValid)
		{
			int nCurRow = 0, nItemInColCount = 0;
			for (CVwInfo& item : m_viewList)
			{
				if (item.m_dsid == m_iterCurItem->m_dsid)
				{
					return nCurRow;
				}
				if (++nItemInColCount >= m_nNumItemsInCol)
				{
					nItemInColCount = 0;
					nCurRow = 0;
				}
				else
				{
					nCurRow++;
				}
			}
		}
		return 0;
	}

#pragma region ViewsListWnd_Drawing
protected:
	int _GetViewsListTitleTextHeight()
	{
		return std::max((int)26, m_xeUI->GetFontMetric(EXE_FONT::eTabListTitleFont).GetHeight());
	}

	int _GetViewsListPathTextHeight()
	{
		return std::max((int)18, m_xeUI->GetFontMetric(EXE_FONT::eUI_Font).GetHeight());
	}

	void _DrawViewsListWndUI(ID2D1RenderTarget* pRT, D2D1_RECT_F rc,
			const CVwInfoList& list, dsid_t curVwId, dsid_t mouseOvrVwId)
	{
		pRT->FillRectangle(rc, GetBrush(CID::CtrlBg));
		int cxVw = (int)WidthOf(rc), cyVw = (int)HeightOf(rc);
		int cyTitle = _GetViewsListTitleTextHeight();
		int cyPath = _GetViewsListPathTextHeight();
		CRect rcTitle(c_cxVL_ClientMargin, c_cyVL_ClientMargin, cxVw, c_cyVL_ClientMargin + cyTitle);
		int yPathTop = c_cyVL_ClientMargin + cyTitle + c_cyVL_PathMarginTop;
		CRect rcPath(c_cxVL_ClientMargin, yPathTop, cxVw, yPathTop + cyPath);

		for (const CVwInfo& item : list)
		{
			if (!item.m_pView || item.m_rcPos.IsRectEmpty())
			{
				continue;
			}
			//CXeFileContainerUI_IF* ui_ds = item.m_pView->GetFileContainerUI_IF();
			//const FileMetadata& md = ui_ds->GetMetadata();
			//PID pid = PID::None;
			PID pid = item.m_pView->GetViewPID();
			//if (md.m_dataSourceType.IsMergedLogsType())	// Is Merge?
			//{
			//	pid = md.m_isMergeRebuildNeeded ? PID::RebuildMergeNeededPng : PID::MergePng;
			//}
			//else if (md.m_fileIcon != FileIcon::None)
			//{
			//	pid = m_xeUI->GetFileIconPID(md.m_fileIcon);
			//}
			//COLORREF rgbTxt = md.m_rgbLogFileColor;
			COLORREF rgbTxt = item.m_pView->GetViewTitleTextColor();
			if (item.m_dsid == curVwId)
			{
				//_DrawIconAndFilename(pRT, md.m_strLogFilename, rcTitle, EXE_FONT::eTabListTitleFont, rgbTxt, pid, false, CRect());
				_DrawIconAndFilename(pRT, item.GetViewName(), rcTitle, EXE_FONT::eTabListTitleFont, rgbTxt, pid, false, CRect());
				_CreateAndDrawTextLayout(pRT, item.GetPathName(), rcPath, CID::CtrlTxt);
				rgbTxt = m_xeUI->GetColor(CID::TabTxtFg);
			}
			if (item.m_dsid == curVwId || item.m_dsid == mouseOvrVwId)
			{
				_DrawCurItemBg(pRT, item.m_rcPos);
			}
			//_DrawIconAndFilename(pRT, md.m_strLogFilename, item.m_rcPos, EXE_FONT::eUI_Font, rgbTxt, pid, true, CRect());
			_DrawIconAndFilename(pRT, item.GetViewName(), item.m_rcPos, EXE_FONT::eUI_Font, rgbTxt, pid, true, CRect());
		}
	}

	void _DrawCurItemBg(ID2D1RenderTarget* pRT, CRect rcBG)
	{
		double cyTxt = std::max((double)m_xeUI->GetFontMetric(EXE_FONT::eUI_Font).GetHeight(), 18.0);
		rcBG.top += (int)(((double)rcBG.Height() - cyTxt) / 2 + 0.5);
		rcBG.bottom = rcBG.top + (int)cyTxt;
		rcBG.left -= 3;
		rcBG.right += 2;
		pRT->FillRectangle(RectFfromRect(rcBG), GetBrush(CID::GrdHdrCurCell));
	}
#pragma endregion ViewsListWnd_Drawing
};

