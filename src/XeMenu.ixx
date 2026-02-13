module;

#include "os_minimal.h"
#include <functional>
#include <map>
#include <cctype>
#include <string>
#include <memory>

export module Xe.Menu;

import Xe.UIcolorsIF;
import Xe.PopupCtrl;
import Xe.ListBoxExCommon;
import Xe.UItypes;
import Xe.Helpers;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef std::function<std::vector<ListBoxExItem>(UINT)> GetSubMenuCallbackFunc;

export class CXeSubMenu
{
public:
	std::wstring m_text;	// If 'this' is a part of a top-level menu then m_Text will be for example "&File".
						// If 'this' is a simple sub-menu (e.g. context menu) then m_text will be empty.

	std::vector<ListBoxExItem> m_items;	// Menu items of the sub-menu.
	std::map<UINT, std::vector<ListBoxExItem>> m_sub_menus;	// PopUp menues (if any).

	ListBoxExItem* FindItem(UINT uID)
	{
		const ListBoxExItem* pItem = FindItemConst(uID);
		return const_cast<ListBoxExItem*>(pItem);
	}

	const ListBoxExItem* FindItemConst(UINT uID) const
	{
		auto it = std::find_if(m_items.begin(), m_items.end(),
			[&](const ListBoxExItem& item) { return item.m_extra_data == uID; });
		if (it != m_items.end())
		{
			return &(*it);
		}
		for (const auto& [uPopupId, list] : m_sub_menus)
		{
			it = std::find_if(list.begin(), list.end(),
				[&](const ListBoxExItem& item) { return item.m_extra_data == uID; });
			if (it != list.end())
			{
				return &(*it);
			}
		}
		return nullptr;
	}
};

export class CXeMenuItems
{
public:
	std::vector<CXeSubMenu> m_menu_items;	
	// If 'this' is a top-level menu then m_menu_items will have the top-level items. E.g. File,Edit,View etc.
	// If 'this' is a simple sub-menu (e.g. context menu) then m_menu_items will only have one item.

	// Top level Menu shortcut keys. When user holds down the Alt key + another key to select a menu item.
	// key is the VK_KEY code, value is index into m_menu_items.
	std::map<WPARAM, size_t> m_menu_shortcut_keys;

	void clear()
	{
		m_menu_items.clear();
		m_menu_shortcut_keys.clear();
	}

	const std::vector<ListBoxExItem>* GetSubMenu(UINT uID) const
	{
		for (const CXeSubMenu& submenu : m_menu_items)
		{
			if (submenu.m_sub_menus.contains(uID))
			{
				const std::vector<ListBoxExItem>& popup = submenu.m_sub_menus.at(uID);
				return &(popup);
			}
		}
		return nullptr;
	}

	ListBoxExItem* FindItem(UINT uID)
	{
		const ListBoxExItem* pItem = FindItemConst(uID);
		return const_cast<ListBoxExItem*>(pItem);
	}

	const ListBoxExItem* FindItemConst(UINT uID) const
	{
		const ListBoxExItem* pItem = nullptr;
		for (const CXeSubMenu& submenu : m_menu_items)
		{
			if ((pItem = submenu.FindItemConst(uID)))
			{
				break;
			}
		}
		return pItem;
	}
};

class CXeMenu;
export typedef std::function<void(CXeMenu* pMenu, size_t top_level_index)> UpdateMenuCallbackFunc;

export class CXeMenu : public CXePopupCtrl
{
public:
	std::unique_ptr<CXeListBoxExCommon> m_cmn;

protected:
	GetSubMenuCallbackFunc m_getSubMenuCallback;
	HWND m_hParentWnd = nullptr;

	CXeMenuItems m_menu;

	// Pointer to parent menu - is null for the first menu.
	CXeMenu* m_pParentMenu = nullptr;

	// When item selected in a submenu - the selected command ID is stored here (in the first menu instance).
	UINT m_uSubmenuSelectedID = 0;

	UINT m_uNextSubMenuId = 0xFF00;

	UpdateMenuCallbackFunc m_updateMenuCallback = nullptr;

#pragma region Create
public:
	CXeMenu(CXeUIcolorsIF* pUIcolors, GetSubMenuCallbackFunc getSubMenu = nullptr)
		: CXePopupCtrl(pUIcolors, XeShowPopup::FadeIn80), m_getSubMenuCallback(getSubMenu)
	{
		_Init();
	}

	CXeMenu(CXeUIcolorsIF* pUIcolors, const std::vector<ListBoxExItem>& list,
		GetSubMenuCallbackFunc getSubMenu = nullptr)
		: CXePopupCtrl(pUIcolors, XeShowPopup::FadeIn80), m_getSubMenuCallback(getSubMenu)
	{
		_Init();
		CXeSubMenu submenu;
		submenu.m_items = list;
		m_menu.m_menu_items.push_back(submenu);
	}

	CXeMenu(CXeUIcolorsIF* pUIcolors, LPCTSTR menu_name_in_resources)
		: CXePopupCtrl(pUIcolors, XeShowPopup::FadeIn80),
		m_getSubMenuCallback([this](UINT uID) { return _GetSubMenu(uID); })
	{
		_Init();
		if (!_LoadAndParseMenuFromResources(menu_name_in_resources))
		{
			XeASSERT(FALSE);
		}
	}

	void SetUpdateMenuCallback(UpdateMenuCallbackFunc updateMenuCallback)
	{
		m_updateMenuCallback = updateMenuCallback;
	}

	// When main menu is shown (by toolbar) - we need a way to "know" when the mouse is over
	// another top-level menu item - so that we can close 'this' menu - and allow the toolbar
	// to show the "other" top-level menu.
	// Keyboard navigation is also supported - user can navigate the top level menu using left and right
	// arrow keys.
	void SetTopLevelMenuNavigationCallback(TopLevelMenuNavigationCallbackFunc topLevelMenuNavigationCallback)
	{
		m_cmn->m_topLevelMenuNavigationCallback = topLevelMenuNavigationCallback;
	}

protected:
	void _Init()
	{
		XeASSERT(m_xeUI);
		m_cmn = std::make_unique<CXeListBoxExCommon>(m_xeUI);
		m_cmn->Initialize(LBType::Menu,
			[this](WORD nfCode, int item_idx) { _OnLBnotify(nfCode, item_idx); });
		SetPopupPointer(m_cmn.get());
	}
#pragma endregion Create

public:
	size_t GetTopLevelAccelaratorKeyIndex(WPARAM wParam)
	{
		if (m_menu.m_menu_shortcut_keys.contains(wParam))
		{
			return m_menu.m_menu_shortcut_keys[wParam];
		}
		return 0xFFFFFFFF;
	}

	void SetMenuItems(const std::vector<ListBoxExItem>& list)
	{
		m_menu.m_menu_items.clear();
		CXeSubMenu submenu;
		submenu.m_items = list;
		m_menu.m_menu_items.push_back(submenu);
	}

	void Clear()
	{
		m_menu.m_menu_items.clear();
	}

	UINT ShowMenu(HWND hParentWnd, CPoint ptScreenPos, size_t topLevelSubMenuIndex)
	{
		m_hParentWnd = hParentWnd;
		XeASSERT(topLevelSubMenuIndex < m_menu.m_menu_items.size());
		if (m_updateMenuCallback)
		{
			m_updateMenuCallback(this, topLevelSubMenuIndex);
		}
		m_cmn->SetItemsList(_GetTopLevelSubMenu(topLevelSubMenuIndex));
		m_cmn->m_suppressMouseMoveMessages = 2;
		_UpdateShortcutKeyMap();
		CRect rcParentCtrl(ptScreenPos, ptScreenPos);
		CRect rc = m_cmn->CalcPopupMenuSize(ptScreenPos.x, ptScreenPos.y);
		ShowPopup(m_hParentWnd, rc, rcParentCtrl);
		UINT uSelectedID = 0;
		if (m_cmn->m_isSelEndOk || m_uSubmenuSelectedID)
		{
			if (m_uSubmenuSelectedID)
			{
				uSelectedID = m_uSubmenuSelectedID;
			}
			else if (m_cmn->m_uSelectedItemCommandID)
			{
				uSelectedID = m_cmn->m_uSelectedItemCommandID;
			}
		}
		m_uSubmenuSelectedID = 0;
		m_cmn->OnSetCurSelMsg((WPARAM)-1, 0);
		m_cmn->m_uSelectedItemCommandID = 0;
		if (uSelectedID && uSelectedID != ID_APP_EXIT)	// ID_APP_EXIT is handled by toolbar.
		{
			//TRACE("Menu WM_COMMAND - cmdid: %u\n", uSelectedID);
			::SendMessage(m_hParentWnd, WM_COMMAND, uSelectedID, 0);
		}
		return uSelectedID;
	}

	size_t GetTopLevelMenuItemCount() const { return m_menu.m_menu_items.size(); }

	const CXeSubMenu* GetTopLevelMenuItemConst(size_t index) const
	{
		return index < m_menu.m_menu_items.size() ? &(m_menu.m_menu_items.at(index)) : nullptr;
	}

	CXeSubMenu* GetTopLevelMenuItem(size_t index)
	{
		return index < m_menu.m_menu_items.size() ? &(m_menu.m_menu_items.at(index)) : nullptr;
	}

#pragma region CMenu_functions
	/*
	CheckMenuItem
		Adds check marks to or removes check marks from a menu item in the (pop-up) menu.
		Note - Only MF_BYCOMMAND is supported by XeMenu.
		The entire menu is searched for the nIDCheckItem.
	Parameters
		nIDCheckItem
		Specifies the menu item to be checked, as determined by nCheck.

		nCheck
		Specifies how to check the menu item and how to determine the item's position in the menu.
		The nCheck parameter can be a combination of MF_CHECKED or MF_UNCHECKED with MF_BYPOSITION
		or MF_BYCOMMAND flags. These flags can be combined by using the bitwise OR operator.
		They have the following meanings:
		MF_BYCOMMAND Specifies that the parameter gives the command ID of the existing menu item.
		This is the default.
		MF_BYPOSITION Specifies that the parameter gives the position of the existing menu item.
		The first item is at position 0.
		MF_CHECKED Acts as a toggle with MF_UNCHECKED to place the default check mark next to the item.
		MF_UNCHECKED Acts as a toggle with MF_CHECKED to remove a check mark next to the item.

		Return Value
		The previous state of the item : MF_CHECKED or MF_UNCHECKED,
		or 0xFFFFFFFF if the menu item did not exist.
	*/
	UINT CheckMenuItem(UINT nIDCheckItem, UINT nCheck)
	{
		if (nCheck & MF_BYPOSITION) { XeASSERT(FALSE); return 0xFFFFFFFF; }
		ListBoxExItem* pItem = m_menu.FindItem(nIDCheckItem);
		if (!pItem) { return 0xFFFFFFFF; }
		uint64_t state = pItem->m_item_data.m_isChecked;
		UINT prevState = state ? MF_CHECKED : MF_UNCHECKED;
		pItem->m_item_data.m_isChecked = (nCheck & MF_CHECKED) ? 1 : 0;
		return prevState;
	}

	/*
	EnableMenuItem
	Enables, disables, or dims a menu item.
			Note - Only MF_BYCOMMAND is supported by XeMenu.
			The entire menu is searched for the nIDCheckItem.

	Parameters
		nIDEnableItem
		Specifies the menu item to be enabled, as determined by nEnable. This parameter can specify
		pop-up menu items as well as standard menu items.

		nEnable
		Specifies the action to take. It can be a combination of MF_DISABLED, MF_ENABLED, or MF_GRAYED,
		with MF_BYCOMMAND or MF_BYPOSITION. These values can be combined by using the C++ bitwise OR
		operator (|). These values have the following meanings:
			MF_BYCOMMAND Specifies that the parameter gives the command ID of the existing menu item.
			This is the default.

			MF_BYPOSITION Specifies that the parameter gives the position of the existing menu item.
			The first item is at position 0.

			MF_DISABLED Disables the menu item so that it cannot be selected but does not dim it.

			MF_ENABLED Enables the menu item so that it can be selected and restores it from its dimmed state.

			MF_GRAYED Disables the menu item so that it cannot be selected and dims it.

	Return Value
		Previous state (MF_DISABLED, MF_ENABLED, or MF_GRAYED) or -1 if not valid.

	Remarks
		The CreateMenu, InsertMenu, ModifyMenu, and LoadMenuIndirect member functions can also set the
		state (enabled, disabled, or dimmed) of a menu item.

		Using the MF_BYPOSITION value requires an application to use the correct CMenu. If the CMenu
		of the menu bar is used, a top-level menu item (an item in the menu bar) is affected. To set
		the state of an item in a pop-up or nested pop-up menu by position, an application must specify
		the CMenu of the pop-up menu.

		When an application specifies the MF_BYCOMMAND flag, Windows checks all pop-up menu items that
		are subordinate to the CMenu; therefore, unless duplicate menu items are present, using the
		CMenu of the menu bar is sufficient.
	*/
	UINT EnableMenuItem(UINT nIDEnableItem, UINT nEnable)
	{
		if (nEnable & MF_BYPOSITION) { XeASSERT(FALSE); return 0xFFFFFFFF; }
		ListBoxExItem* pItem = m_menu.FindItem(nIDEnableItem);
		if (!pItem) { return 0xFFFFFFFF; }
		uint64_t state = pItem->m_item_data.m_isItemDisabled;
		UINT prevState = state ? MF_DISABLED : MF_ENABLED;
		state = (nEnable & MF_DISABLED) ? 1 : 0;
		pItem->m_item_data.m_isItemDisabled = state;
		return prevState;
	}

	/*
	ModifyMenu
	Changes an existing menu item at the position specified by nPosition.
		Note - Only MF_BYCOMMAND is supported by XeMenu.
		This call should only be used to change the menu text.
		MF_OWNERDRAW not supported, MF_SEPARATOR not supported, MF_POPUP not supported.
		The entire menu is searched for the nIDCheckItem.

	Parameters
		nPosition
		Specifies the menu item to be changed. The nFlags parameter can be used to interpret nPosition
		in the following ways:
			nFlags	Interpretation of nPosition
			MF_BYCOMMAND	Specifies that the parameter gives the command ID of the existing menu item.
							This is the default if neither MF_BYCOMMAND nor MF_BYPOSITION is set.
			MF_BYPOSITION	Specifies that the parameter gives the position of the existing menu item.
							The first item is at position 0.

		nFlags
		Specifies how nPosition is interpreted and gives information about the changes to be made to the
		menu item. For a list of flags that may be set, see the AppendMenu member function.

		nIDNewItem
		Specifies either the command ID of the modified menu item or, if nFlags is set to MF_POPUP,
		the menu handle (HMENU) of a pop-up menu. The nIDNewItem parameter is ignored (not needed)
		if nFlags is set to MF_SEPARATOR.

		lpszNewItem
		Specifies the content of the new menu item. The nFlags parameter can be used to interpret
		lpszNewItem in the following ways:
			nFlags			Interpretation of lpszNewItem
			MF_OWNERDRAW	Contains an application-supplied 32-bit value that the application can use to maintain additional data associated with the menu item. This 32-bit value is available to the application when it processes MF_MEASUREITEM and MF_DRAWITEM.
			MF_STRING		Contains a long pointer to a null-terminated string or to a CString.
			MF_SEPARATOR	The lpszNewItem parameter is ignored (not needed).

	Return Value
		Nonzero if the function is successful; otherwise 0.

	Remarks
		The application specifies the new state of the menu item by setting values in nFlags.
		If this function replaces a pop-up menu associated with the menu item, it destroys the
		old pop-up menu and frees the memory used by the pop-up menu.

		When nIDNewItem specifies a pop-up menu, it becomes part of the menu in which it is inserted.
		If that menu is destroyed, the inserted menu will also be destroyed. An inserted menu should
		be detached from a CMenu object to avoid conflict.

		Whenever a menu that resides in a window is changed (whether or not the window is displayed),
		the application should call CWnd::DrawMenuBar. To change the attributes of existing menu items,
		it is much faster to use the CheckMenuItem and EnableMenuItem member functions.
	*/
	BOOL ModifyMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0, const wchar_t* lpszNewItem = NULL)
	{
		if ((nFlags & MF_BYPOSITION) || (nFlags & MF_OWNERDRAW) || (nFlags & MF_SEPARATOR)
			|| (nFlags & MF_POPUP) || !lpszNewItem)
		{
			XeASSERT(FALSE);
			return FALSE;
		}
		ListBoxExItem* pItem = m_menu.FindItem(nPosition);
		if (!pItem) { return FALSE; }
		if (lpszNewItem)
		{
			pItem->m_string = lpszNewItem;
		}
		pItem->m_item_data.m_isItemDisabled = (nFlags & MF_DISABLED) ? 1 : 0;
		pItem->m_item_data.m_isChecked = (nFlags & MF_CHECKED) ? 1 : 0;
		return TRUE;
	}

	bool AppendPopupMenu(size_t top_level_index,	// Top level menu index (= 0 for first drop-down menu).
		const ListBoxExItem& popup,					// This item is the popup menu item that is added at 
													// bottom of the drop-down menu.
		const std::vector<ListBoxExItem>& list)		// This is the popup menu (sub-menu).
	{
		if (top_level_index == 0 && m_menu.m_menu_items.size() == 0)
		{
			m_menu.m_menu_items.push_back(CXeSubMenu());
		}
		if (top_level_index < m_menu.m_menu_items.size())
		{
			ListBoxExItem item_copy = popup;
			item_copy.m_extra_data = m_uNextSubMenuId++;
			item_copy.m_item_data.m_isSubMenu = 1;
			CXeSubMenu& submenu = m_menu.m_menu_items[top_level_index];
			submenu.m_items.push_back(item_copy);
			submenu.m_sub_menus.insert_or_assign((UINT)item_copy.m_extra_data, list);
			return true;
		}
		return false;
	}

	bool AppendMenu(size_t top_level_index,	// Top level menu index (= 0 for first drop-down menu).
		const ListBoxExItem& item)			// This item is the menu item that is added at 
											// bottom of the drop-down menu.
	{
		if (top_level_index == 0 && m_menu.m_menu_items.size() == 0)
		{
			m_menu.m_menu_items.push_back(CXeSubMenu());
		}
		if (top_level_index < m_menu.m_menu_items.size())
		{
			m_menu.m_menu_items[top_level_index].m_items.push_back(item);
			return true;
		}
		return false;
	}
#pragma endregion CMenu_functions

protected:
	void _OnLBnotify(WORD nfCode, int item_idx)
	{
		if (nfCode == LB_EX_NOTIFY_SUBMENU_SELECTED || nfCode == LB_EX_NOTIFY_SUBMENU_SELECTED_BY_KEYBOARD)
		{
			const ListBoxExItem* pSelItem = m_cmn->GetItemDataAtIndexConst(item_idx);
			CRect rcItemScr = m_cmn->GetItemScreenRect(item_idx);
			if (pSelItem && m_getSubMenuCallback)
			{
				int nSelIdx = nfCode == LB_EX_NOTIFY_SUBMENU_SELECTED_BY_KEYBOARD ? 0 : -1;
				_ShowSubMenu(pSelItem, rcItemScr, nSelIdx);
			}
		}
	}

	void _ShowSubMenu(const ListBoxExItem* pSelItem, const CRect& rcItemScr, int nSelIdx)
	{
		std::vector<ListBoxExItem> list = m_getSubMenuCallback((UINT)pSelItem->m_extra_data);
		if (list.size() == 0) { return; }
		std::unique_ptr<CXeMenu> submenu = std::make_unique<CXeMenu>(m_xeUI, list, m_getSubMenuCallback);
		submenu->m_pParentMenu = this;
		submenu->m_hParentWnd = m_hParentWnd;
		submenu->m_cmn->m_pParentMenu = m_cmn.get();
		submenu->m_cmn->m_rcParentMenuItemScreenPos = rcItemScr;
		submenu->m_cmn->SetItemsList(submenu->_GetTopLevelSubMenu(0), nSelIdx);
		submenu->_UpdateShortcutKeyMap();
		submenu->SetTopLevelMenuNavigationCallback(m_cmn->m_topLevelMenuNavigationCallback);
		CRect rc = submenu->m_cmn->CalcPopupMenuSize(rcItemScr.right, rcItemScr.top);
		submenu->ShowPopup(m_hParentWnd, rc, rcItemScr);
		m_cmn->m_nTopLevelMenuNavigationItemIndex = submenu->m_cmn->m_nTopLevelMenuNavigationItemIndex;
		m_cmn->m_uSelectedItemCommandID = submenu->m_cmn->m_uSelectedItemCommandID;
		if (submenu->m_cmn->m_isSelEndOk)
		{
			CXeMenu* pFirstMenu = submenu->_FindFirstMenuInstance();
			XeASSERT(pFirstMenu);	// First menu should have been found.
			if (pFirstMenu)
			{
				//TRACE("Submenu select - cmdid: %u\n", submenu->m_cmn->m_uSelectedItemCommandID);
				pFirstMenu->m_uSubmenuSelectedID = submenu->m_cmn->m_uSelectedItemCommandID;
			}
		}
	}

	CXeMenu* _FindFirstMenuInstance()
	{
		CXeMenu* pMenu = m_pParentMenu;
		while (pMenu)
		{
			if (pMenu->m_pParentMenu == nullptr)
			{
				break;
			}
			pMenu = pMenu->m_pParentMenu;
		}
		return pMenu;
	}

	std::vector<ListBoxExItem> _GetTopLevelSubMenu(size_t index)
	{
		if (index < m_menu.m_menu_items.size())
		{
			return m_menu.m_menu_items[index].m_items;
		}
		return std::vector<ListBoxExItem>();
	}

	void _UpdateShortcutKeyMap()
	{
		m_cmn->m_menu_shortcut_keys.clear();
		const std::vector<ListBoxExItem>& list = m_cmn->GetItemsList();
		for (const ListBoxExItem& item : list)
		{
			wchar_t sc = _GetShortcutKey(item.m_string);
			if (sc)
			{
				m_cmn->m_menu_shortcut_keys[sc] = (UINT)item.m_extra_data;
			}
		}
	}

	wchar_t _GetShortcutKey(const std::wstring& str) const
	{
		std::string::size_type pos = str.find(L'&');
		if (pos != std::string::npos && str.size() > (pos + 1))
		{
			wchar_t sc = str[pos + 1];
			if (sc > L' ' && sc != L'&')
			{
				sc = std::toupper(sc);
				return sc;
			}
		}
		return 0;
	}

	bool _LoadAndParseMenuFromResources(const wchar_t* menu_name_in_resources)
	{
		m_menu.clear();
		CXeBuffer buf = GetResource(menu_name_in_resources, RT_MENU);
		if (buf.m_buffer_size < 4) { return false; }
		// MENUHEADER
		MENUITEMTEMPLATEHEADER* pHdr = (MENUITEMTEMPLATEHEADER*)buf.m_buffer.get();
		if (pHdr->versionNumber != 0 && pHdr->offset != 0)
		{
			XeASSERT(FALSE);	// Only MENU format supported (not MENUEX)
			return false;
		}
		uint8_t* pData = buf.m_buffer.get() + 4;
		size_t len = buf.m_buffer_size - 4;
		uint8_t* pDataEnd = buf.m_buffer.get() + buf.m_buffer_size;
		// Note - MENUITEMTEMPLATE structs follow the header - but those are variable in size.
		CXeSubMenu* pCurSubMenu = nullptr;
		UINT uCurSubMenuId = 0;;
		bool isNextItemWithSeparator = false;
		CXeMenuItemInfo item;
		while (pData < pDataEnd)
		{
			size_t item_len = item.ParseMenuItem(pData, len);
			if (item_len == 0)
			{
				break;
			}
			if (item.IsPopUp())
			{
				if (!pCurSubMenu)	// Is this new top level menu item?
				{
					pCurSubMenu = _AddSubMenu();
					pCurSubMenu->m_text = item.m_text;
					isNextItemWithSeparator = false;
				}
				else
				{	// This is a sub-popup-menu.
					item.m_wID = m_uNextSubMenuId;
					pCurSubMenu->m_sub_menus.insert_or_assign(m_uNextSubMenuId, std::vector<ListBoxExItem>());
					pCurSubMenu->m_items.push_back(_MakeMenuItem(item, isNextItemWithSeparator));
					uCurSubMenuId = m_uNextSubMenuId;	// Next item will go into sub-menu.
					isNextItemWithSeparator = false;
				}
			}
			else
			{
				if (item.IsSeparator())
				{
					isNextItemWithSeparator = true;
				}
				else
				{
					ListBoxExItem menu_item = _MakeMenuItem(item, isNextItemWithSeparator);
					if (uCurSubMenuId)
					{
						pCurSubMenu->m_sub_menus[uCurSubMenuId].push_back(menu_item);
					}
					else
					{
						pCurSubMenu->m_items.push_back(menu_item);
					}
					isNextItemWithSeparator = false;
				}
				if (item.IsLastItemInSubMenu())	// Is last item in sub-menu?
				{
					if (uCurSubMenuId)
					{
						uCurSubMenuId = 0;
						++m_uNextSubMenuId;
					}
					else
					{
						pCurSubMenu = nullptr;
					}
				}
			}
			pData += item_len;
			len -= item_len;
		}
		_UpdateTopLevelMenuShortcutKeyMap();
		return true;
	}

	CXeSubMenu* _AddSubMenu()
	{
		m_menu.m_menu_items.push_back(CXeSubMenu());
		CXeSubMenu& submenu = m_menu.m_menu_items.back();
		return &submenu;
	}

	ListBoxExItem _MakeMenuItem(const CXeMenuItemInfo& item, bool hasSeparator)
	{
		IsSeparator isSeparator = hasSeparator ? IsSeparator::yes : IsSeparator::no;
		IsChecked isChecked = item.IsChecked() ? IsChecked::yes : IsChecked::no;
		IsEnabled isEnabled = item.IsEnabled() ? IsEnabled::yes : IsEnabled::no;
		IsSubMenu isSubMenu = item.IsPopUp() ? IsSubMenu::yes : IsSubMenu::no;
		ListBoxExItem lb_item = ListBoxExItem(item.m_wID, item.m_text, isSeparator, isChecked, isEnabled, isSubMenu);
		//lb_item.m_item_data.m_isCheckbox = 0;
		return lb_item;
	}

	void _UpdateTopLevelMenuShortcutKeyMap()
	{
		size_t index = 0;
		for (const CXeSubMenu& tl_menu : m_menu.m_menu_items)
		{
			wchar_t sc = _GetShortcutKey(tl_menu.m_text);
			if (sc)
			{
				m_menu.m_menu_shortcut_keys[sc] = index;
			}
			++index;
		}
	}

	std::vector<ListBoxExItem> _GetSubMenu(UINT uID)
	{
		const std::vector<ListBoxExItem>* pSubMenu = m_menu.GetSubMenu(uID);
		if (pSubMenu)
		{
			return *pSubMenu;
		}
		return std::vector<ListBoxExItem>();
	}
};
