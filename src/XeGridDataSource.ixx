module;

#include "os_minimal.h"
#include <string>
#include <functional>
#include "GridDefs.h"
#include "UG\UGCell.h"
#include "UG\XeGridDefs.h"

export module Xe.GridDataSource;

export import Xe.GridTableIF;

export import Xe.GridDataDefs;
import Xe.UIcolorsIF;
import Xe.Menu;
import Xe.mfc_types;
//import Xe.LogDefs;
import Xe.DefData;

export constexpr auto UG_ALIGNLEFT = 1;
export constexpr auto UG_ALIGNRIGHT = 2;
export constexpr auto UG_ALIGNCENTER = 4;
export constexpr auto UG_ALIGNTOP = 8;
export constexpr auto UG_ALIGNBOTTOM = 0x10;
export constexpr auto UG_ALIGNVCENTER = 0x20;

export constexpr auto UGCT_DROPLIST = 1;
export constexpr auto UGCT_DROPLISTHIDEBUTTON = 0x200;
export constexpr auto UGCT_DROPLIST_DBLCLICK_STARTDROP = 0x400;
export constexpr auto UGCT_CHECKBOX = 2;
export constexpr auto UGCT_PROGRESS = 4;
export constexpr auto UGCT_CHECKBOXFLAT = 0;
export constexpr auto UGCT_CHECKBOXCROSS = 0;

export constexpr auto GGC_COLORBTN = 0x20;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export struct structGridSelection
{	// Grid selection data (also used by data source when processing keyboard commands)
	int m_nStartCol = 0;
	int m_nEndCol = 0;
	long m_nStartRow = 0;
	long m_nEndRow = 0;
	BOOL m_fSelectionValid = FALSE;
	int m_nNumCellsSelected = 0;
	int m_nNumRowsSelected = 0;			// valid when entire rows are selected
	BOOL m_fEntireRowsSelected = FALSE;
	int m_nGridCurRow = 0;
	int m_nGridCurCol = 0;
	int m_nGridCurColFldIdx = 0;	// field index of current column
};

export struct stGridNotifyData
{
	EGRIDOP m_operation;

	// col, row - the cell coordinates of where the menu originated from
	int m_col;
	long m_row;

	// index of col in datasource.
	int m_fldidx;

	// section - identify for which portion of the gird the menu is for.
	//	  possible sections:
	//			UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
	//			UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
	int m_section;

	// ID of the menu command.
	int m_menuCmdId;

	// Status of selection in the grid.
	structGridSelection m_selection;

	// Parent view.
	HWND m_hParentView;

	bool m_bFlag;

	stGridNotifyData(EGRIDOP eOp, int col, int fldidx, long row, int section, int cmdId,
			structGridSelection selection, HWND hParentView)
	{
		m_operation = eOp;
		m_col = col;
		m_fldidx = fldidx;
		m_row = row;
		m_section = section;
		m_menuCmdId = cmdId;
		m_selection = selection;
		m_hParentView = hParentView;
	}
};

export typedef std::function<bool(stGridNotifyData& cmdData)> GridNotifyCallbackFunc;

export class CXeGridDataSource //: public CObject
{
protected:
	CGridTblInfo m_tblInfo;
	//DSType	m_dataSourceType = DSType::UNKNOWN;
	dsid_t m_dwDataSourceId;

public:
	CXeUIcolorsIF* m_xeUI = nullptr;

	CXeGridDataSource(CXeUIcolorsIF* pUIcolors) : m_xeUI(pUIcolors) {}
	virtual ~CXeGridDataSource() = default;

	//DSType GetDataSourceType()
	//{
	//	XeASSERT(m_dataSourceType != DSType::UNKNOWN); return m_dataSourceType;
	//}
	dsid_t GetDataSourceId() { return m_dwDataSourceId; }

	CGridTblInfo& GetGridTblInfo() { XeASSERT(m_tblInfo.IsInitialized()); return m_tblInfo; }

	virtual std::wstring GetDataGridSettingsName() const = 0;

	// Grid header support
	virtual bool HasGridHeader() { return false; }
	virtual std::string GetGridHeader() { return std::string(); }
	virtual std::string GetViewTabText() { return std::string(); }

	// optional keyboard command processing, return TRUE if key was processed
	// note - Grid selection data is valid when member function called
	virtual BOOL ProcessKeydownMessage(int nKey, eGridAction& action,
		structGridSelection& selection) {
		return FALSE;
	};

	// Called when SH dbl clicked - return TRUE if processed.
	virtual BOOL OnGridSH_DClicked(long row) { return FALSE; };

	// Called when current row in grid changed.
	virtual void OnCurRowChange(long oldrow, long newrow) {};

	// Drag and drop support - Note DROPEFFECT_NONE is returned by default.
	//virtual DROPEFFECT OnGridDragEnter(COleDataObject* pDataObject) { return DROPEFFECT_NONE; }

	//virtual DROPEFFECT OnGridDragOver(COleDataObject* pDataObject, int fldidx,
	//	long row)
	//{
	//	return DROPEFFECT_NONE;
	//}

	// [OUT] puarrKeyIDs = ptr to array - if valid: populated with KeyID's of all new rows.
	//virtual DROPEFFECT ProcessDragDrop(COleDataObject* pDataObject,
	//	int fldidx, long row, CWnd* pWnd, CUIntArray* puarrKeyIDs = 0)
	//{
	//	return DROPEFFECT_NONE;
	//}

	// Sorting support
	int m_nSortByCol = -1;		// = -1 when not sorted.
	BOOL m_fSortAscending = TRUE;

	// Reset list sorting to unsorted.
	virtual void SortUnsorted() {};

	// Sort list by column [fldidx].
	virtual void SortBy(int fldidx, int nSortflag) {};

	////////////////////////////////////////////////////////////////////
	// Grid - custom menu support. Data source can add to the grid menu.

	// OnGridMenuStart - called by AdoGrid when menu is about to be shown.
	// [IN]	pMenu = pointer to menu already populated by AdoGrid.
	//		'this' data source can add menu items to pMenu.
	//		Use menu item identifier IDMG_FIRST_CUSTOM or higher for inserted item(s).
	// [IN]	fldidx, row	- the cell coordinates of where the menu originated from
	// [IN]	section		- identify for which portion of the gird the menu is for.
	//					possible sections:
	//						GRID_SECTION_GRID, GRID_SECTION_TOPHEADING,
	//						GRID_SECTION_SIDEHEADING, GRID_SECTION_CORNERBUTTON
	// [IN]	selection = current selection data for grid.
	//	Return: TRUE - to allow menu to show
	//			FALSE - to prevent the menu from popping up
	virtual BOOL OnGridMenuStart(CXeMenu* pMenu, int fldidx, long row, int section,
		structGridSelection& selection)
	{
		return TRUE;
	}	// Default - allow menu unmodified.

// OnGridMenuCommand - called by AdoGrid when menu item was selected by user.
// [IN]  nMenuItemID = menu item identifier.
//		 NOTE - 'this' data source should only process it's own menu items.
// [IN]  fldidx, row	- the cell coordinates of where the menu originated from
// [IN]  section		- identify for which portion of the gird the menu is for.
//					possible sections:
//						GRID_SECTION_GRID, GRID_SECTION_TOPHEADING,
//						GRID_SECTION_SIDEHEADING, GRID_SECTION_CORNERBUTTON
// [IN]  selection = current selection data for grid.
// [IN]  pWnd = pointer to parent window (grid).
// [OUT] action = if menu item was processed (TRUE returned)
//					set action grid must perform.
//					NO_ACTION or REDRAW_ROW or REDRAW_ALL or SET_GRID_FROM_DS.
//	Return: TRUE - if menu item was processed.
//			FALSE - if menu item was NOT processed. Grid will process it.
	virtual BOOL OnGridMenuCommand(int nMenuItemID, int fldidx, long row, int section,
		structGridSelection& selection, HWND hWnd, eGridAction& action)
	{
		return FALSE;
	}	// Default - item not processed.

////////////////////////////////////////////////////////////////////

// This member is called by AdoGrid from within OnMouseMove.
// Data source can return a ID_CUR_XXX id from shared cursors collection or 0 to use
// normal cursor.
	virtual UINT GetCustomCursorID(int fldidx, long row, UINT uFlags)
	{
		return 0;
	}

	// Grid edit support funtions
	virtual BOOL FieldEditOK(int fldidx, long row, int& flags, int& maxlen)
	{
		return FALSE;
	};
	virtual BOOL ProcessEdit(int fldidx, long row, LPCTSTR string,
		eGridAction& action, HWND hWnd) {
		return TRUE;
	};

	// Grid calls this member from within OnEditStart when user want to edit a cell.
	// (user has double clicked a cell or pressed F2).
	// By overriding this member the data source can override normal cell editing here. 
	// Return TRUE if cell editing done.
	// Return FALSE if grid should take care of cell edit normally. (DEFAULT)
	virtual BOOL EditField(int fldidx, long row, CRect& rectCell,
		eGridAction& action, HWND hGridWnd) {
		return FALSE;
	}

	// This member is called by grid when user is editing a cell.
	// The data source MUST NOT modify the row count while edit is in progress.
	// Data sources that need to know when grid is in cell edit mode should override
	// this member. See implementation in TxPlistAdoDS.
	virtual void SetGridEditInProgressFlag(BOOL fEditInProgress = TRUE)
	{
		m_fGridEditInProgress = fEditInProgress;
	}

	BOOL IsGridInEditCellMode() { return m_fGridEditInProgress; }

private:
	BOOL m_fGridEditInProgress = FALSE;

public:
	/////////////////////////////////////////////////////////////////////
	// CUGDataSource overrides 
	virtual long GetNumRows() { return 0; };	// number of rows in Grid list view

	virtual void GetCellForGrid(int col, long row, CUGCell* cell, const structGridSelection& selData) = 0;

	// Number of lines in the cells of row.
	virtual int GetGridRowLineCount(long row) { return 1; };

	virtual bool GetListBoxData(int fldidx, long row, std::vector<ListBoxExItem>& list,
		CRect& cellRect, HWND hParentGrid) { return FALSE; }

	// When cell is of type UGCT_DROPLIST and user is putting the cell into edit mode
	// this function is called to know if edit text is allowed.
	virtual bool IsDropListEditType(int col, long row) { return false; }

	virtual void ListBoxCheckBoxChecked(int col, int row, ListBoxExItem* pItem) {}

	virtual BOOL GetHintForCell(int col, long row, TOOLTIP_SETTINGS& ttSettings) { return FALSE; }
};

