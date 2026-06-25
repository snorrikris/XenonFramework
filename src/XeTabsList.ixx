module;

#include "os_minimal.h"
#include <algorithm>
#include <list>
#include <string>
#include "logging.h"

export module Xe.TabsList;

import Xe.UIcolorsIF;
import Xe.FileVwIF;
import Xe.mfc_types;
import Xe.Helpers;
import Xe.DefData;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class EMOVETAB { eFIRST = 0, eLAST, eLEFT, eRIGHT };

export enum class ETABVISBL { eNOT_VISIBLE = 0, eVISIBLE, ePARTIALLY_VISIBLE };

export enum class ECLOSETABS { eCLOSEOTHERS = 0, eCLOSETOLEFT, eCLOSETORIGHT, eCLOSEALLBUTPINNED };

constexpr int c_cxTAB_X_MARGIN = 2;		// X margin between tabs and buttons
constexpr int c_cxTEXT_X_MARGIN = 4;	// Text margin from icon left edge
constexpr int c_cyTEXT_Y_MARGIN = 9;	// Text Y margin from view bottom edge
constexpr int c_cxTEXT_R_MARGIN = 6;	// Text margin from text end to tab right edge
constexpr int c_cxCLOSE_R_MARGIN = 4;	// Close button right margin to tab right edge
constexpr int c_cxICON_SPACE = 18;		// Reserved space for icon (when needed).

export class CXeTabsList
{
private:
	VSRL::Logger& logger() const { return VSRL::s_pVSRL->GetInstance("CXeTabsList"); }

protected:
	CXeUIcolorsIF* m_xeUI = nullptr;

	// Handle to parent window of TabView.
	HWND m_hParent = NULL;

	// Dialog control Id of the currently visible file view in the splitter window.
	UINT m_uViewDlgCtrlId = 0;

public:
	std::vector<CVwInfo> m_list;

	/* ----- X offset -----
	* X offset is measured from the left edge of the tabs space in the UI.
	* The left edge of tabs space starts on the right hand side of the Tabs list drop down button.
	* The right edge of the tabs space is the left edge of the right hand side button (X scroll buttons).
	* A X offset of 0 means that the first (unpinned) tab in the list is painted at start of tabs space.
	* X offset is a positive number in the range 0 to m_xOffsetLimit.
	*/
	bool m_isOneRowUI = false;	// Is "simple" UI?
	int m_xOffset = 0;			// H scroll position. Not used when m_isOneRowUI = true.
	int m_xOffsetLimit = 0;		// (negative) limit for m_xOffset. (= cxTotalTabs - cxForTabs)

	// DataSourceId of view that has focus.
	// Note - the view with focus could be in the 'other' tab view.
	dsid_t m_dwDataSourceIdOfViewWithFocus;

	// true if the view with focus is in 'this' tab view.
	bool m_isFocusVwInThis = false;

	dsid_t m_curVwId;	// Updated every time UI is painted.

	CRect m_rcLeftTabListDropDownBtn;	// Client coords. of left tab list drop down button.
	bool m_isMouseOverLeftTabListDropDownBtn = false;

	CRect m_rcLeftXscrollBtn;	// Client coords. (in right hand side of UI) Left X scroll button.
	CRect m_rcRightXscrollBtn;	// Client coords. (in right hand side of UI) Right X scroll button.
	bool m_isMouseOverLeftXscrollButton = false, m_isMouseOverRightXscrollButton = false;

	// UI vars.
	int m_cxVw = 0, m_cyTabRow = 0;

	CXeTabsList(CXeUIcolorsIF* pUIcolors) : m_xeUI(pUIcolors) {}
	//~CXeTabsList(void) {}

	void OnTabViewCreated(HWND hParent, UINT uViewDlgCtrlId)
	{
		m_hParent = hParent;
		m_uViewDlgCtrlId = uViewDlgCtrlId;
		XeASSERT(m_hParent != NULL && m_uViewDlgCtrlId != 0);
	}

#pragma region Tab_Management
	void AddTab(const CVwInfo& tabinfo, dsid_t insertBeforeDataSourceId)
	{
		std::vector<CVwInfo>::iterator it = m_list.end();
		if (insertBeforeDataSourceId.is_valid())
		{
			it = std::find_if(m_list.begin(), m_list.end(),
					[&](CVwInfo& tabinfo) { return tabinfo.m_dsid == insertBeforeDataSourceId; });
			XeASSERT(it != m_list.end());	// insertBeforeDataSourceId should have been found.
		}
		else if (!insertBeforeDataSourceId.is_max())
		{
			it = m_list.begin();
		}
		// Ensure list is in "sane" order - all pinned tabs are before unpinned tabs in list.
		while (it != m_list.end() && it->m_isPinned == true)
		{
			++it;
		}
		m_list.insert(it, tabinfo);
	}

	// Remove CVwInfo structure from list.
	// Return nullptr if pView not found.
	// Return pointer to next view after (if more elements are after the removed one),
	// or the previous element (if more elements are before the removed one),
	// or nullptr if no more elements remain.
	CXeFileVwIF* RemoveTab(CXeFileVwIF* pView)
	{
		dsid_t dsid = pView->GetDataSourceId();
		auto it = std::find_if(m_list.begin(), m_list.end(),
				[&](CVwInfo& tabinfo) { return tabinfo.m_dsid == dsid; });
		if (it == m_list.end())
		{
			return nullptr;
		}
		it = m_list.erase(it);
		if (it == m_list.end() && m_list.size() > 0)
		{
			--it;
		}
		return it != m_list.end() ? it->m_pView : nullptr;
	}

	std::vector<CVwInfo> RemoveAllTabs()
	{
		std::vector<CVwInfo> tabList = m_list;
		m_list.clear();
		return tabList;
	}
#pragma endregion Tab_Management

#pragma region Tab_Info
	const CVwInfo* GetTab(CXeFileVwIF* pView) const
	{
		return pView ? GetTab(pView->GetDataSourceId()) : nullptr;
	}

	const CVwInfo* GetTab(dsid_t dsid) const
	{
		auto it = std::find_if(m_list.cbegin(), m_list.cend(),
			[&](const CVwInfo& tabinfo) { return tabinfo.m_dsid == dsid; });
		return (it != m_list.cend()) ? &(*it) : nullptr;
	}

	const CVwInfo* GetTab(HWND hWnd) const
	{
		auto it = std::find_if(m_list.cbegin(), m_list.cend(),
			[&](const CVwInfo& tabinfo) { return tabinfo.m_pView->GetHwndOfView() == hWnd; });
		return (it != m_list.cend()) ? &(*it) : nullptr;
	}

	CXeFileVwIF* GetTabViewPtr(dsid_t dsid) const
	{
		const CVwInfo* pTabInfo = GetTab(dsid);
		return pTabInfo ? pTabInfo->m_pView : nullptr;
	}

	bool IsFirstTab(CXeFileVwIF* pView) const
	{
		return _GetTabIdxOfItem(pView->GetDataSourceId()) == 0;
	}

	bool IsLastTab(CXeFileVwIF* pView) const
	{
		int idx = _GetTabIdxOfItem(pView->GetDataSourceId());
		return idx == (m_list.size() - 1);
	}

	const CVwInfo* GetTabFromDataSourceId(dsid_t dwDataSourceId) const
	{
		for (const CVwInfo& tabinfo : m_list)
		{
			if (tabinfo.m_dsid == dwDataSourceId)
			{
				return &tabinfo;
			}
		}
		return nullptr;
	}

	// Get number of structures in list.
	int GetTabCount() const { return (int)m_list.size(); }

	int GetPinnedTabCount() const
	{
		return (int)std::count_if(m_list.cbegin(), m_list.cend(), [](const CVwInfo& tabinfo) { return tabinfo.m_isPinned; });
	}

	int GetUnPinnedTabCount() const
	{
		return GetTabCount() - GetPinnedTabCount();
	}

	int GetNumTabRows() const
	{
		int row = 0;
		for (const CVwInfo& tabinfo : m_list)
		{
			row = std::max(row, tabinfo.m_nRow);
		}
		return row + 1;
	}

	std::vector<CVwInfo> GetAllTabs() const
	{
		std::vector<CVwInfo> tabList;
		for (const CVwInfo& tabinfo : m_list)
		{
			tabList.push_back(tabinfo);
		}
		return tabList;
	}

	const std::vector<CVwInfo>& GetTabList() const
	{
		return m_list;
	}

	std::vector<CVwInfo> GetTabsToClose(ECLOSETABS eCloseOp, CXeFileVwIF* pSelView) const
	{
		std::vector<CVwInfo> tabList;
		bool isBeforeSel = true, isAfterSel = false;
		for (const CVwInfo& tabinfo : m_list)
		{
			if (tabinfo.m_pView == pSelView)
			{
				isBeforeSel = false;
			}
			if (eCloseOp == ECLOSETABS::eCLOSEALLBUTPINNED && !tabinfo.m_isPinned)
			{
				tabList.push_back(tabinfo);
			}
			else if ((isBeforeSel && (eCloseOp == ECLOSETABS::eCLOSETOLEFT || eCloseOp == ECLOSETABS::eCLOSEOTHERS))
				|| (isAfterSel && (eCloseOp == ECLOSETABS::eCLOSETORIGHT || eCloseOp == ECLOSETABS::eCLOSEOTHERS)))
			{
				tabList.push_back(tabinfo);
			}
			if (!isBeforeSel)
			{
				isAfterSel = true;
			}
		}
		return tabList;
	}

	bool CanCloseTabs(ECLOSETABS eCloseOp, CXeFileVwIF* pSelView) const
	{
		dsid_t dsid = pSelView->GetDataSourceId();
		auto it = std::find_if(m_list.begin(), m_list.end(),
			[&](const CVwInfo& tabinfo) { return tabinfo.m_dsid == dsid; });
		bool notFound = it == m_list.end();
		bool isFirst = it == m_list.begin();
		bool isLast = notFound ? false : ++it == m_list.end();
		if (notFound
			|| m_list.size() <= 1
			|| (isFirst && eCloseOp == ECLOSETABS::eCLOSETOLEFT)
			|| (isLast && eCloseOp == ECLOSETABS::eCLOSETORIGHT))
		{
			return false;
		}
		return true;
	}

	const CVwInfo* GetTabAtIndex(UINT uIndex) const
	{
		UINT u = 0;
		for (const CVwInfo& tabinfo : m_list)
		{
			if (u == uIndex)
			{
				return &tabinfo;
			}
			++u;
		}
		return nullptr;
	}

	// Return 0xFFFF if not found.
	UINT GetIndexOfTab(CXeFileVwIF* pView) const
	{
		dsid_t dsid = pView->GetDataSourceId();
		UINT u = 0;
		for (const CVwInfo& tabinfo : m_list)
		{
			if (tabinfo.m_dsid == dsid)
			{
				return u;
			}
			++u;
		}
		return 0xFFFF;
	}

	UINT GetIndexOfCurrentTab() const
	{
		UINT u = 0;
		for (const CVwInfo& tabinfo : m_list)
		{
			if (tabinfo.m_dsid == m_curVwId)
			{
				return u;
			}
			++u;
		}
		return 0xFFFF;
	}

	bool IsTabPinned(dsid_t dataSourceId) const
	{
		const CVwInfo* pVwNfo = GetTab(dataSourceId);
		if (pVwNfo)
		{
			return pVwNfo->m_isPinned;
		}
		return false;
	}
#pragma endregion Tab_Info

	void SendMessageToAllViews(UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		for (CVwInfo& tabinfo : m_list)
		{
			XeASSERT(tabinfo.m_pView);
			if (tabinfo.m_pView)
			{
				::SendMessageW(tabinfo.m_pView->GetHwndOfView(), uMessage, wParam, lParam);
			}
		}
	}

#pragma region Tab_Move_Pin
	bool CanMoveTab(EMOVETAB eMove, const CXeFileVwIF* pViewToMove) const
	{
		try
		{
			auto it = _FindTabConst(pViewToMove);
			if (it == m_list.cend()) { return false; }
			if (eMove == EMOVETAB::eLEFT || eMove == EMOVETAB::eFIRST)
			{
				if (it == m_list.cbegin()) { return false; }
				if (it->m_isPinned)
				{
					if (it != m_list.cbegin())
					{
						return true;
					}
				}
				else
				{
					auto first_unpinned_it = _FindFirstUnpinnedTabConst();
					if (first_unpinned_it != m_list.cend() && it != first_unpinned_it)
					{
						return true;
					}
				}
			}
			else if (eMove == EMOVETAB::eRIGHT || eMove == EMOVETAB::eLAST)
			{
				auto it_next_right = std::next(it, 1);
				if (it_next_right != m_list.cend() && it_next_right->m_isPinned == it->m_isPinned)
				{
					return true;
				}
			}
		}
		catch (const std::exception& ex)
		{
			logger().error("CanMoveTab failed. Error: %s", ex.what());
		}
		return false;
	}

	bool MoveTab(EMOVETAB eMove, CXeFileVwIF* pViewToMove)
	{
		try
		{
			if (!CanMoveTab(eMove, pViewToMove)) { return false; }
			auto it = _FindTab(pViewToMove);
			if (it == m_list.end())
			{
				XeASSERT(false);
				return false;
			}
			CVwInfo tabToMove = *it;	// Copy item before erase
			it = m_list.erase(it);
			if (eMove == EMOVETAB::eFIRST)
			{
				it = tabToMove.m_isPinned ? m_list.begin() : _FindFirstUnpinnedTab();
			}
			else if (eMove == EMOVETAB::eLAST)
			{
				it = tabToMove.m_isPinned ? _FindFirstUnpinnedTab() : m_list.end();
			}
			else
			{
				it= std::next(it, (eMove == EMOVETAB::eLEFT ? -1 : 1));
			}
			m_list.insert(it, tabToMove);
			return true;
		}
		catch (const std::exception& ex)
		{
			logger().error("MoveTab failed. Error: %s", ex.what());
		}
		return false;
	}

	// Return true if tab to move can be inserted before tab at (pViewAt).
	bool CanMoveTabAt(CXeFileVwIF* pViewToMove, CXeFileVwIF* pViewAt)
	{
		try
		{
			auto it_to_move = _FindTabConst(pViewToMove);
			auto it_at = _FindTabConst(pViewAt);
			if (pViewToMove == pViewAt || it_to_move == m_list.cend() || it_at == m_list.cend())
			{
				return false;
			}
			if (it_at != m_list.cbegin())
			{
				auto it2 = std::prev(it_at, 1);
				if (it_to_move->m_dsid == it2->m_dsid)	// Is pViewAt the one to the right of pViewToMove?
				{
					return it_to_move->m_isPinned == it_at->m_isPinned;	// Can move if same pin state.
				}
			}
			if (it_to_move->m_isPinned == it_at->m_isPinned)
			{
				return true;
			}
			if (it_to_move->m_isPinned && it_at != m_list.cbegin())
			{
				--it_at;
				return it_to_move->m_dsid != it_at->m_dsid && it_at->m_isPinned;
			}
		}
		catch (const std::exception& ex)
		{
			logger().error("CanMoveTabAt failed. Error: %s", ex.what());
		}
		return false;
	}

	bool MoveTab(CXeFileVwIF* pViewToMove, CXeFileVwIF* pViewAt)
	{
		try
		{
			if (!CanMoveTabAt(pViewToMove, pViewAt)) { return false; }
			auto it = _FindTab(pViewToMove);
			auto it_at = _FindTab(pViewAt);
			if (it_at != m_list.cbegin())
			{
				auto it2 = std::prev(it_at, 1);
				if (it->m_dsid == it2->m_dsid)	// Is pViewAt the one to the right of pViewToMove?
				{
					std::swap(*it, *it_at);
					return true;
				}
			}

			CVwInfo tabToMove = *it;	// Copy item before erase
			m_list.erase(it);
			auto it_ins_pos = _FindTab(pViewAt);
			m_list.insert(it_ins_pos, tabToMove);
			return true;
		}
		catch (const std::exception& ex)
		{
			logger().error("MoveTab failed. Error: %s", ex.what());
		}
		return false;
	}

	// Returns true if recalc. UI needed.
	bool PinTab(bool isPinned, CXeFileVwIF* pViewToPin)
	{
		try
		{
			auto itTabToPin = _FindTab(pViewToPin);
			if (itTabToPin == m_list.end() || itTabToPin->m_pView != pViewToPin)
			{
				XeASSERT(FALSE);	// Not found - should not happen.
				return false;
			}
			CVwInfo vwInfoTabToPin = *itTabToPin;	// Copy item before erase
			vwInfoTabToPin.m_isPinned = isPinned;
			vwInfoTabToPin.ClearUIvars();
			m_list.erase(itTabToPin);
			auto it_ins_pos = _FindFirstUnpinnedTab();
			m_list.insert(it_ins_pos, vwInfoTabToPin);
			return true;
		}
		catch (const std::exception& ex)
		{
			logger().error("PinTab failed. Error: %s", ex.what());
		}
		return false;
	}
#pragma endregion Tab_Move_Pin

#pragma region UI_helpers
	// return true if redraw window needed.
	bool UpdateIdOfViewWithFocus(dsid_t dwDataSourceId)
	{
		bool isFocusVwInThis = GetTabFromDataSourceId(dwDataSourceId) != nullptr;
		if (m_dwDataSourceIdOfViewWithFocus != dwDataSourceId
			|| m_isFocusVwInThis != isFocusVwInThis)
		{
			m_dwDataSourceIdOfViewWithFocus = dwDataSourceId;
			m_isFocusVwInThis = isFocusVwInThis;
			return true;
		}
		return false;
	}

	// Do mouse over processing, return true if redraw window needed.
	bool DoMouseOverProcessing(CPoint point, bool& isMouseOverTabIdxChanged)
	{
		bool isRedrawNeeded = isMouseOverTabIdxChanged = false;
		bool bfL = m_isMouseOverLeftTabListDropDownBtn;
		bool bfXL = m_isMouseOverLeftXscrollButton, bfXR = m_isMouseOverRightXscrollButton;
		m_isMouseOverLeftTabListDropDownBtn = m_rcLeftTabListDropDownBtn.PtInRect(point);
		m_isMouseOverLeftXscrollButton = m_rcLeftXscrollBtn.PtInRect(point);
		m_isMouseOverRightXscrollButton = m_rcRightXscrollBtn.PtInRect(point);
		isRedrawNeeded = m_isMouseOverLeftTabListDropDownBtn != bfL
				|| m_isMouseOverLeftXscrollButton != bfXL || m_isMouseOverRightXscrollButton != bfXR;
		bool mouseIsOverTabArea = !(m_isMouseOverLeftTabListDropDownBtn || m_isMouseOverLeftXscrollButton || m_isMouseOverRightXscrollButton);
		for (CVwInfo& tabinfo : m_list)
		{
			int xOffset = m_isOneRowUI || tabinfo.m_isPinned ? 0 : GetXoffsetForPaint();
			if (mouseIsOverTabArea && tabinfo.m_rcTab.HitTestWithOffset(point, -xOffset, 0))
			{
				if (!tabinfo.m_bIsMouseOver)
				{
					tabinfo.m_bIsMouseOver = isRedrawNeeded = isMouseOverTabIdxChanged = true;
				}
			}
			else
			{
				if (tabinfo.m_bIsMouseOver)
				{
					tabinfo.m_bIsMouseOver = false;
					isRedrawNeeded = isMouseOverTabIdxChanged = true;
				}
			}

			if (mouseIsOverTabArea && tabinfo.m_rcCloseBtn.HitTestWithOffset(point, -xOffset, 0))
			{
				if (!tabinfo.m_bIsMouseOverCloseBtn)
				{
					tabinfo.m_bIsMouseOverCloseBtn = isRedrawNeeded = true;
				}
			}
			else
			{
				if (tabinfo.m_bIsMouseOverCloseBtn)
				{
					tabinfo.m_bIsMouseOverCloseBtn = false;
					isRedrawNeeded = true;
				}
			}
			if (mouseIsOverTabArea && tabinfo.m_rcPinBtn.HitTestWithOffset(point, -xOffset, 0))
			{
				if (!tabinfo.m_bIsMouseOverPinBtn)
				{
					tabinfo.m_bIsMouseOverPinBtn = isRedrawNeeded = true;
				}
			}
			else
			{
				if (tabinfo.m_bIsMouseOverPinBtn)
				{
					tabinfo.m_bIsMouseOverPinBtn = false;
					isRedrawNeeded = true;
				}
			}
		}
		return isRedrawNeeded;
	}

	CVwInfo* GetTabAtPoint(CPoint point, bool* pIsOverCloseBtn = nullptr, bool* pIsOverPinBtn = nullptr)
	{
		if (IsPointInTabListUIbuttons(point))
		{
			return nullptr;
		}
		auto it = std::find_if(m_list.begin(), m_list.end(),
				[&](CVwInfo& tabinfo)
				{
					int xOffset = m_isOneRowUI || tabinfo.m_isPinned ? 0 : GetXoffsetForPaint();
					return tabinfo.m_rcTab.HitTestWithOffset(point, -xOffset, 0);
				});
		if (it == m_list.end())
		{
			return nullptr;
		}
		int xOffset = m_isOneRowUI || it->m_isPinned ? 0 : GetXoffsetForPaint();
		if (pIsOverCloseBtn)
		{
			*pIsOverCloseBtn = it->m_rcCloseBtn.HitTestWithOffset(point, -xOffset, 0);
		}
		if (pIsOverPinBtn)
		{
			*pIsOverPinBtn = it->m_rcPinBtn.HitTestWithOffset(point, -xOffset, 0);
		}
		return &(*it);
	}

	ETABVISBL IsTabVisible(CXeFileVwIF* pView)
	{
		const CVwInfo* pTabInfo = GetTab(pView->GetDataSourceId());
		XeASSERT(pTabInfo);
		if (!pTabInfo)
		{
			return ETABVISBL::eNOT_VISIBLE;
		}
		if (m_isOneRowUI || pTabInfo->m_isPinned)	// All tabs are visible when "simple UI", pinned tabs are all visible.
		{
			return ETABVISBL::eVISIBLE;
		}
		CRect rcTab = pTabInfo->m_rcTab;
		rcTab.OffsetRect(-GetXoffsetForPaint(), 0);
		int left = m_rcLeftTabListDropDownBtn.right, right = m_rcLeftXscrollBtn.left;
		int xL = rcTab.left, xR = rcTab.right - 1;
		return xL >= left && xL < right && xR < right ? ETABVISBL::eVISIBLE
				: (xL >= left && xL < right) || (xR > left && xR < right) ? ETABVISBL::ePARTIALLY_VISIBLE
				: ETABVISBL::eNOT_VISIBLE;
	}

	bool IsPointInTabListUIbuttons(const CPoint& point) const
	{
		return m_rcLeftTabListDropDownBtn.PtInRect(point)
				|| m_rcLeftXscrollBtn.PtInRect(point) || m_rcRightXscrollBtn.PtInRect(point);
	}

	bool IsPointInXscrollButtons(const CPoint& point) const
	{
		return m_rcLeftXscrollBtn.PtInRect(point) || m_rcRightXscrollBtn.PtInRect(point);
	}

protected:
	// Get tab index of view (0...x). returns -1 if not found.
	int _GetTabIdxOfItem(dsid_t dsid) const 
	{
		int idx = 0;
		for (const CVwInfo& tabinfo : m_list)
		{
			if (tabinfo.m_dsid == dsid)
			{
				return idx;
			}
			idx++;
		}
		return -1;
	}

	std::vector<CVwInfo>::iterator _FindTab(const CXeFileVwIF* pView)
	{
		return std::find_if(m_list.begin(), m_list.end(),
				[pView](const CVwInfo& tabinfo) { return tabinfo.m_pView == pView; });
	}
	std::vector<CVwInfo>::const_iterator _FindTabConst(const CXeFileVwIF* pView) const
	{
		return std::find_if(m_list.cbegin(), m_list.cend(),
				[pView](const CVwInfo& tabinfo) { return tabinfo.m_pView == pView; });
	}
	std::vector<CVwInfo>::iterator _FindFirstUnpinnedTab()
	{
		return std::find_if(m_list.begin(), m_list.end(),
				[](const CVwInfo& tabinfo) { return !tabinfo.m_isPinned; });
	}
	std::vector<CVwInfo>::const_iterator _FindFirstUnpinnedTabConst() const
	{
		return std::find_if(m_list.cbegin(), m_list.cend(),
				[](const CVwInfo& tabinfo) { return !tabinfo.m_isPinned; });
	}
#pragma endregion UI_helpers

#pragma region CalculateUI
public:
	void CalculateUI(int cxVw)
	{
		m_cxVw = cxVw;
		m_cyTabRow = _GetTabViewRowHeight();
		_RecalculateTabsViewUI();
	}

	// Return true if height changed.
	bool RecalculateUI()
	{
		int cyBefore = GetTabViewHeight();
		_RecalculateTabsViewUI();
		return cyBefore != GetTabViewHeight();
	}

	int GetTabViewRowHeight() const { return m_cyTabRow; }

	int GetTabViewHeight() const { return m_cyTabRow * GetNumTabRows(); }

	int GetTabViewWidth() const { return m_cxVw; }

	void MakeCurrentTabVisible(CXeFileVwIF* pCurrentView)
	{
		if (!pCurrentView)
		{
			XeASSERT(m_list.size() == 0);	// We have tabs but not current view - why?
			return;
		}
		dsid_t dsid = pCurrentView->GetDataSourceId();
		int cxUnpinnedTabsTotalBeforeCurrent = 0, cxCurrentTab = 0;;
		for (CVwInfo& tabInfo : m_list)
		{
			if (tabInfo.m_dsid == dsid)
			{
				cxCurrentTab = _CXforTab(tabInfo);
				break;
			}
			if (!tabInfo.m_isPinned)
			{
				cxUnpinnedTabsTotalBeforeCurrent += _CXforTab(tabInfo);
			}
		}
		if (cxCurrentTab == 0)
		{
			XeASSERT(FALSE);
			return;
		}
		if (IsTabVisible(pCurrentView) == ETABVISBL::eVISIBLE)
		{
			return;
		}
		int cxVw = m_rcLeftXscrollBtn.left - m_rcLeftTabListDropDownBtn.right;
		int xWantedForTab = cxVw > cxCurrentTab ? (cxVw - cxCurrentTab) / 2 : 0;
		m_xOffset = GetValueInValidRange(cxUnpinnedTabsTotalBeforeCurrent - xWantedForTab, 0, m_xOffsetLimit);
	}

	void Hscroll(int xScroll)
	{
		m_xOffset = GetValueInValidRange(m_xOffset + xScroll, 0, m_xOffsetLimit);
	}

	inline int GetXoffsetForPaint() const
	{
		return m_xOffset - m_rcLeftTabListDropDownBtn.right;
	}

protected:
	static CSize _GetTabViewDropListButtonSize() { return CSize(20, 20); }
	static CSize _GetTabViewXscrollButtonSize() { return CSize(15, 20); }
	static CSize _GetTabViewCloseAndPinButtonSize() { return CSize(15, 15); }

	int _GetTabViewRowHeight() const
	{
		return std::max((int)20, m_xeUI->GetFontMetric(EXE_FONT::eUI_Font).GetHeight()) + c_cyTEXT_Y_MARGIN;
	}

	void _RecalculateTabsViewUI()
	{
		/* *** Fitting model ***
		* Pinned tabs and unpinned tabs will occupy one row if all pinned tabs + all unpinned tabs
		* fit on the row (along with the UI buttons).
		* If pinned tabs need more than one row - then unpinned tabs will not share a row with the
		* pinned tabs. In this case all unpinned tabs occupy the bottom row.
		*/

		m_isOneRowUI = true;	// Assume that one row for UI is enough.
		int x_firstTabAfterLB = _SetTabListUIbuttonRects(0 /*row*/);
		int x_nextTab = x_firstTabAfterLB;

		CSize listBtn = _GetTabViewDropListButtonSize();
		CSize scrollBtn = _GetTabViewXscrollButtonSize();
		CSize closeBtn = _GetTabViewCloseAndPinButtonSize();
		int cxForOneRow = listBtn.cx;
		int cxUnpinnedTabsTotal = 0;
		int numPinnedTabs = 0, numUnpinnedTabs = 0;
		for (CVwInfo& tabInfo : m_list)
		{
			XeASSERT(tabInfo.m_pView);
			CSize sizeText = tabInfo.m_pView
				? m_xeUI->GetTextSizeW(EXE_FONT::eUI_FontBold, tabInfo.m_pView->GetViewName().c_str())
				: CSize();
			bool hasIcon = tabInfo.m_pView->GetViewPID() != PID::None;
			tabInfo.m_cxTabNeeded = c_cxTEXT_X_MARGIN
				+ (hasIcon ? c_cxICON_SPACE : 0)	// Add space for icon
				+ sizeText.cx
				+ c_cxTEXT_R_MARGIN
				+ (closeBtn.cx * 2)		// Pin button and Close button
				+ c_cxCLOSE_R_MARGIN;	// add space for close button

			x_nextTab = _SetTabRects(tabInfo, x_nextTab, 0 /*row*/);
			cxForOneRow += _CXforTab(tabInfo);
			if (tabInfo.m_isPinned)
			{
				++numPinnedTabs;
			}
			else
			{
				++numUnpinnedTabs;
				cxUnpinnedTabsTotal += _CXforTab(tabInfo);
			}
		}
		m_isOneRowUI = cxForOneRow < m_cxVw;
		if (m_isOneRowUI)	// If one row is enough for all tabs - then no more work is needed.
		{
			m_xOffset = 0;
			return;
		}

		/*
		* When UI is more than one row or wider than view
		* - m_xOffset is offset into calculated (unpinned) tabs horizontal position.
		* Pinned tabs have calculated position in the view.
		* Unpinned tabs position in the view depends on the value of m_xOffset.
		* The first unpinned tab has X = 0.
		*/

		x_nextTab = 0;	// First tab x pos.
		int row = 0;
		bool isFittingPinnedTabs = numPinnedTabs > 0, isAnyPinnedTabFitted = false;
		for (CVwInfo& tabInfo : m_list)
		{
			if (isFittingPinnedTabs && !tabInfo.m_isPinned)	// All pinned tabs fitted?
			{
				isFittingPinnedTabs = false;
				++row;										// Place unpinned tabs on next row down.
				x_nextTab = 0;
			}
			if (isFittingPinnedTabs)
			{
				if ((x_nextTab + _CXforTab(tabInfo)) > m_cxVw
					&& isAnyPinnedTabFitted /* make sure row 0 has a tab */)
				{
					++row;
					x_nextTab = 0;
				}
				x_nextTab = _SetTabRects(tabInfo, x_nextTab, row);
				isAnyPinnedTabFitted = true;
			}
			else	// Fitting unpinned tabs
			{
				x_nextTab = _SetTabRects(tabInfo, x_nextTab, row);
			}
		}

		int cxForTabs = m_cxVw - (listBtn.cx + scrollBtn.cx * 2);
		m_xOffsetLimit = cxUnpinnedTabsTotal > cxForTabs ? cxUnpinnedTabsTotal - cxForTabs : 0;

		int lastRow = GetNumTabRows() - 1;
		if (numUnpinnedTabs == 0)	// No unpinned tabs?
		{
			// Need to reposition last row tabs to fit L drop down button.
			for (CVwInfo& tabInfo : m_list)
			{
				if (tabInfo.m_nRow == lastRow)
				{
					_SetTabRects(tabInfo, tabInfo.m_rcTab.left + x_firstTabAfterLB, lastRow);
				}
			}
		}
		_SetTabListUIbuttonRects(lastRow);	// Put drop down buttons on last row
		Hscroll(0);	// Ensure xOffset limit.
	}

	// Set Tab rect, pin button rect, close button rect. Return X pos of next tab.
	int _SetTabRects(CVwInfo& tabInfo, int x, int row)
	{
		CSize closeBtn = _GetTabViewCloseAndPinButtonSize();
		int yBtn = 2 + (int)((double)(m_cyTabRow - closeBtn.cy) / 2 + 0.5);
		int xTabRightEdge = x + tabInfo.m_cxTabNeeded;
		int y = row * m_cyTabRow;
		int xBtn = xTabRightEdge - closeBtn.cx - c_cxCLOSE_R_MARGIN - 1;
		tabInfo.m_rcCloseBtn.SetRect(xBtn, y + yBtn, xBtn + closeBtn.cx, y + yBtn + closeBtn.cy);
		xBtn -= closeBtn.cx;
		tabInfo.m_rcPinBtn.SetRect(xBtn, y + yBtn, xBtn + closeBtn.cx, y + yBtn + closeBtn.cy);
		tabInfo.m_rcTab.SetRect(x, y, xTabRightEdge, y + m_cyTabRow - 1);
		tabInfo.m_nRow = row;
		return tabInfo.m_rcTab.right + c_cxTAB_X_MARGIN;
	};

	int _CXforTab(const CVwInfo& tabInfo) const { return tabInfo.m_cxTabNeeded + c_cxTAB_X_MARGIN; }

	// Calculate left tab list drop down button and X scroll buttons coords. for row.
	// Return X pos. for first tab after Left drop button.
	int _SetTabListUIbuttonRects(int row)
	{
		CRect& rcLB = m_rcLeftTabListDropDownBtn;
		CRect& rcXL = m_rcLeftXscrollBtn;
		CRect& rcXR = m_rcRightXscrollBtn;
		int xListBtnL = 0;
		CSize listBtn = _GetTabViewDropListButtonSize();
		CSize scrollBtn = _GetTabViewXscrollButtonSize();
		int yListBtns = (int)((double)(m_cyTabRow - listBtn.cy) / 2 + 0.5);
		yListBtns += (m_cyTabRow * row);
		rcLB.SetRect(xListBtnL, yListBtns, xListBtnL + listBtn.cx, yListBtns + listBtn.cy);
		int xListBtnR = m_cxVw - (scrollBtn.cx * 2);
		if (xListBtnR < rcLB.right || m_isOneRowUI)
		{
			// No space for X scroll buttons OR one row UI - don't need X scroll buttons.
			rcXL.SetRect(m_cxVw, yListBtns, m_cxVw, yListBtns + scrollBtn.cy);
			rcXR.SetRect(m_cxVw, yListBtns, m_cxVw, yListBtns + scrollBtn.cy);
		}
		else
		{
			rcXL.SetRect(xListBtnR, yListBtns, xListBtnR + scrollBtn.cx, yListBtns + scrollBtn.cy);
			xListBtnR = rcXL.right;
			rcXR.SetRect(xListBtnR, yListBtns, xListBtnR + scrollBtn.cx, yListBtns + scrollBtn.cy);
		}
		return rcLB.right + c_cxTAB_X_MARGIN;	// First tab x pos. after L drop button.
	}
#pragma endregion CalculateUI
};

