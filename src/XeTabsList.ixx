module;

#include "os_minimal.h"
#include <algorithm>
#include <list>
#include <string>
#include "logging.h"

export module Xe.TabsList;

import Xe.FileVwIF;
import Xe.mfc_types;
//import Xe.LogDefs;
import Xe.DefData;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class EMOVETAB { eFIRST = 0, eLAST, eLEFT, eRIGHT };

export enum class ETABVISBL { eNOT_VISIBLE = 0, eVISIBLE, ePARTIALLY_VISIBLE };

export enum class ECLOSETABS { eCLOSEOTHERS = 0, eCLOSETOLEFT, eCLOSETORIGHT, eCLOSEALLBUTPINNED };

export class CXeTabsList
{
private:
	VSRL::Logger& logger() const { return VSRL::s_pVSRL->GetInstance("CXeTabsList"); }

public:
	CVwInfoList m_list;

	int m_nFirstVisibleUnpinnedTab = -1;	// Index of first unpinned visible tab, = -1 when no unpinned tabs.

	// DataSourceId of view that has focus.
	// Note - the view with focus could be in the 'other' tab view.
	dsid_t m_dwDataSourceIdOfViewWithFocus;

	// true if the view with focus is in 'this' tab view.
	bool m_isFocusVwInThis = false;

	dsid_t m_curVwId;	// Updated every time UI is painted.

	CRect m_rcLeftTabListDropDownBtn;	// Client coords. of left tab list drop down button.
	CRect m_rcRightTabListDropDownBtn;	// Client coords. of right tab list drop down button.
	bool m_isMouseOverLeftTabListDropDownBtn = false;
	bool m_isMouseOverRightTabListDropDownBtn = false;

	CXeTabsList() {}
	//~CXeTabsList(void) {}

#pragma region Tab_Management
	void AddTab(const CVwInfo& tabinfo, dsid_t insertBeforeDataSourceId)
	{
		if (insertBeforeDataSourceId.is_valid())
		{
			auto it = std::find_if(m_list.begin(), m_list.end(),
				[&](CVwInfo& tabinfo)
				{
					return tabinfo.m_dsid == insertBeforeDataSourceId;
				});
			if (it != m_list.end())
			{
				m_list.insert(it, tabinfo);
			}
			else
			{
				XeASSERT(FALSE);	// insertBeforeDataSourceId should have been found.
				m_list.push_back(tabinfo);
			}
		}
		else if (insertBeforeDataSourceId.is_max())
		{
			m_list.push_back(tabinfo);
		}
		else
		{
			m_list.push_front(tabinfo);
		}
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
		auto it2 = it;
		bool notFound = it == m_list.end();
		bool isLast = notFound ? false : ++it2 == m_list.end();
		if (notFound)
		{
			return nullptr;
		}
		CXeFileVwIF* pNext = nullptr;
		if (m_list.size() > 1)
		{
			if (isLast)
			{
				it--;
			}
			else
			{
				it++;
			}
			pNext = it->m_pView;
		}
		m_list.remove_if([&](CVwInfo& tabinfo) { return tabinfo.m_dsid == dsid; });
		return pNext;
	}

	CVwInfoList RemoveAllTabs()
	{
		CVwInfoList tabList;
		tabList.splice(tabList.end(), m_list);
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

	//std::vector<CVwInfo> GetTabsWithListId(UINT uListId) const
	//{
	//	std::vector<CVwInfo> tabList;
	//	for (const CVwInfo& tabinfo : m_list)
	//	{
	//		UINT tabListId = tabinfo.m_pView
	//			? tabinfo.m_pView->GetConstFileContainerUI_IF()->GetMetadata().m_uListId : 0;
	//		if (tabListId == uListId)
	//		{
	//			tabList.push_back(tabinfo);
	//		}
	//	}
	//	return tabList;
	//}

	//std::vector<CVwInfo> GetTabsOfType(const std::vector<DSType>& list) const
	//{
	//	std::vector<CVwInfo> tabList;
	//	for (const CVwInfo& tabinfo : m_list)
	//	{
	//		if (tabinfo.m_pView)
	//		{
	//			DSType dsType
	//				= tabinfo.m_pView->GetConstFileContainerUI_IF()->GetDataSourceType();
	//			if (std::find(list.cbegin(), list.cend(), dsType) != list.cend())
	//			{
	//				tabList.push_back(tabinfo);
	//			}

	//		}
	//	}
	//	return tabList;
	//}

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
			CVwInfo tabToMove = *it;	// Copy item before erase
			std::list<CVwInfo>::iterator it_ins_pos;
			if (eMove == EMOVETAB::eFIRST)
			{
				it_ins_pos = tabToMove.m_isPinned ? m_list.begin() : _FindFirstUnpinnedTab();
			}
			else if (eMove == EMOVETAB::eLAST)
			{
				it_ins_pos = tabToMove.m_isPinned ? _FindFirstUnpinnedTab() : m_list.end();
			}
			else
			{
				it_ins_pos = std::next(it, (eMove == EMOVETAB::eLEFT ? -1 : 2));
			}
			if (it == it_ins_pos) { std::advance(it_ins_pos, 1); }
			m_list.erase(it);
			m_list.insert(it_ins_pos, tabToMove);
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
		bool bfL = m_isMouseOverLeftTabListDropDownBtn, bfR = m_isMouseOverRightTabListDropDownBtn;
		m_isMouseOverLeftTabListDropDownBtn = m_rcLeftTabListDropDownBtn.PtInRect(point);
		m_isMouseOverRightTabListDropDownBtn = m_rcRightTabListDropDownBtn.PtInRect(point);
		isRedrawNeeded = m_isMouseOverLeftTabListDropDownBtn != bfL
			|| m_isMouseOverRightTabListDropDownBtn != bfR;
		bool mouseIsOverTabArea = !(m_isMouseOverLeftTabListDropDownBtn || m_isMouseOverRightTabListDropDownBtn);
		for (CVwInfo& tabinfo : m_list)
		{
			if (mouseIsOverTabArea && tabinfo.m_rcTab.PtInRect(point))
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

			if (mouseIsOverTabArea && tabinfo.m_rcCloseBtn.PtInRect(point))
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
			if (mouseIsOverTabArea && tabinfo.m_rcPinBtn.PtInRect(point))
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

	CVwInfo* GetTabAtPoint(CPoint point)
	{
		if (IsPointInDropListButtons(point))
		{
			return nullptr;
		}
		auto it = std::find_if(m_list.begin(), m_list.end(),
			[&](CVwInfo& tabinfo) { return tabinfo.m_rcTab.PtInRect(point); });
		return (it != m_list.end()) ? &(*it) : nullptr;
	}

	ETABVISBL IsTabVisible(CXeFileVwIF* pView, const CRect& rcCli)
	{
		const CVwInfo* pTabInfo = GetTab(pView->GetDataSourceId());
		XeASSERT(pTabInfo);
		if (!pTabInfo || pTabInfo->m_rcTab.IsRectEmpty())
			return ETABVISBL::eNOT_VISIBLE;
		ETABVISBL eVisible = ETABVISBL::eNOT_VISIBLE;
		int xL = pTabInfo->m_rcTab.left, xR = pTabInfo->m_rcTab.right - 1;
		if (xL >= rcCli.left && xL < rcCli.right)
			eVisible = ETABVISBL::ePARTIALLY_VISIBLE;
		if (xR < rcCli.right)
			eVisible = ETABVISBL::eVISIBLE;
		return eVisible;
	}

	void MakeCurrentTabVisible(CXeFileVwIF* pCurrentView, const CRect& rcCli)
	{
		dsid_t dsid = pCurrentView ? pCurrentView->GetDataSourceId() : dsid_t();
		auto it = std::find_if(m_list.begin(), m_list.end(),
			[&](CVwInfo& tabinfo) { return tabinfo.m_dsid == dsid; });
		int tabIdx = _GetTabIdxOfItem(dsid);
		if (it == m_list.end() || tabIdx == -1)
		{
			XeASSERT(FALSE);
			return;
		}
		if (GetUnPinnedTabCount() == 0)
		{
			m_nFirstVisibleUnpinnedTab = -1;
			return;
		}
		CVwInfo& tabinfo = *it;
		if (tabinfo.m_isPinned || IsTabVisible(pCurrentView, rcCli) == ETABVISBL::eVISIBLE)
		{
			return;
		}
		auto itCandidate = it;
		if (tabIdx > m_nFirstVisibleUnpinnedTab)	// Is current tab after first visible tab?
		{
			int cxTabs = tabinfo.m_cxTabNeeded;
			while (it != m_list.begin() && !it->m_isPinned && cxTabs < rcCli.Width())
			{
				--it;
				if ((cxTabs + it->m_cxTabNeeded) < rcCli.Width())
				{
					cxTabs += it->m_cxTabNeeded;
					itCandidate = it;
				}
				else
				{
					break;
				}
			}
		}
		m_nFirstVisibleUnpinnedTab = _GetTabIdxOfItem(itCandidate->m_dsid);
	}

	// Decrement first visible tab index when:
	//		Number of unpinned tabs > 1
	//		Last tab is fully visible and not pinned
	//		cx unused space after last tab is wide enough for more tabs
	void DecrementFirstVisibleTabWhenNeeded(const CRect& rcCli)
	{
		if (GetUnPinnedTabCount() <= 1)
		{
			return;
		}
		auto it = m_list.rbegin();	// it is last tab.
		if (it == m_list.rend() || it->m_rcTab.IsRectEmpty())
		{
			return;
		}
		int cxEmptySpace = rcCli.right - it->m_rcTab.right;	// How much empty space after last tab.
		auto it_candidate = it;
		++it;	// it is second to last tab.
		while (it != m_list.rend() && !it->m_isPinned)
		{
			if (it->m_cxTabNeeded > cxEmptySpace)
			{
				break;
			}
			cxEmptySpace -= it->m_cxTabNeeded;
			it_candidate = it;
			++it;
		}
		int idx_candidate = _GetTabIdxOfItem(it_candidate->m_dsid);
		if (idx_candidate < m_nFirstVisibleUnpinnedTab)
		{
			m_nFirstVisibleUnpinnedTab = idx_candidate;
		}
	}

	bool IsPointInDropListButtons(const CPoint& point) const
	{
		return m_rcLeftTabListDropDownBtn.PtInRect(point)
			|| m_rcRightTabListDropDownBtn.PtInRect(point);
	}
#pragma endregion UI_helpers

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

	std::list<CVwInfo>::iterator _FindTab(const CXeFileVwIF* pView)
	{
		return std::find_if(m_list.begin(), m_list.end(),
			[pView](const CVwInfo& tabinfo) { return tabinfo.m_pView == pView; });
	}
	std::list<CVwInfo>::const_iterator _FindTabConst(const CXeFileVwIF* pView) const
	{
		return std::find_if(m_list.cbegin(), m_list.cend(),
			[pView](const CVwInfo& tabinfo) { return tabinfo.m_pView == pView; });
	}
	std::list<CVwInfo>::iterator _FindFirstUnpinnedTab()
	{
		return std::find_if(m_list.begin(), m_list.end(),
			[](const CVwInfo& tabinfo) { return !tabinfo.m_isPinned; });
	}
	std::list<CVwInfo>::const_iterator _FindFirstUnpinnedTabConst() const
	{
		return std::find_if(m_list.cbegin(), m_list.cend(),
			[](const CVwInfo& tabinfo) { return !tabinfo.m_isPinned; });
	}
};

