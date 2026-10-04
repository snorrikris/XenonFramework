module;

#include "../os_minimal.h"
#include <string>
#include "ugdefine.h"

export module Xe.UGGridInfoIF;

import Xe.mfc_types;
import Xe.UIcolorsIF;

import Xe.UGCell;
export import Xe.UGCelTypIF;

class CUGDataSource;
class CUGMultiSelect;

export struct UGCOLINFO
{
	int				width;
	CUGDataSource*	dataSource;
	CUGCell*		colDefault;
	int				colTranslation;
};

export struct TOOLTIP_SETTINGS
{
	std::wstring m_strTooltipText;
	bool m_bCoverCell = false;			// true when tooltip window should cover the cell, false when tooltip shown below cell.
	bool m_bCanChangeXpos = false;		// When tooltip is wide the X position can be changed, more to the left.
};

export class CUGGridInfoIF
{
public:
	virtual int GetNumberCols() const = 0;
	virtual int	GetCurrentCol() const = 0;
	virtual long GetCurrentRow() const = 0;
	virtual int	GetLeftCol() const = 0;
	virtual int	GetRightCol() const = 0;
	virtual long GetTopRow() const = 0;
	virtual long GetBottomRow() const = 0;

	virtual int GetColWidth(int col) const = 0;
	virtual HCURSOR GetDefaultCursor() const = 0;
	virtual int GetJoinStartCellColRow(int* col, long* row) = 0;

#pragma region CUGGridInfoIF_fn_
	virtual int OnCellTypeNotify(long ID, int col, long row, long msg, long long param) = 0;
	virtual int GetCellIndirect(int col, long row, CUGCell* cell) = 0;
	virtual CUGCellTypeIF* GetCellType(int type) = 0;
	virtual CUGCellTypeIF* GetCellTypeColRow(int col, long row) = 0;
	virtual int GetRowHeight(long row) = 0;
	virtual int GetNonUniformRowHeight(long row) = 0;
	virtual int SetCell(int col, long row, CUGCell* cell) = 0;
	virtual int RedrawAll() = 0;
	virtual int RedrawCell(int col, long row) = 0;
	virtual int GetCellRect(int col, long row, RECT* rect) = 0;
	virtual int GetRangeRect(int startCol, long startRow, int endCol, long endRow, RECT* rect) = 0;
	virtual void OnKillFocusNewWnd(int section, HWND hNewWnd) = 0;
	virtual void OnSetFocus(int section) = 0;
	virtual void OnKillFocus(int section) = 0;
	virtual int EditCtrlFinished(LPCTSTR string, BOOL cancelFlag, BOOL continueFlag, int continueCol, long continueRow) = 0;
	virtual int GotoCell(int col, long row) = 0;
	virtual int GotoCol(int col) = 0;
	virtual int GotoRow(long row) = 0;
	virtual int OnEditVerify(int col, long row, HWND edit, UINT* vcKey) = 0;
	virtual int GetCellFromPointColRow(int x, int y, int* col, long* row) = 0;
	virtual int GetCellFromPoint(int x, int y, int* ptcol, long* ptrow, RECT* rect) = 0;
	virtual int OnSortEvaluate(CUGCell* cell1, CUGCell* cell2, int flags) = 0;
	virtual long GetNumberRows() = 0;
	virtual void AdjustComponentSizes() = 0;
	virtual int StartMenu(int col, long row, CPoint point, int section) = 0;
	virtual void OnLClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnRClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnDClicked(int col, long row, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnMouseMove(int col, long row, POINT* point, UINT nFlags, BOOL processed) = 0;
	virtual void OnTH_LClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnTH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnTH_DClicked(int col, long row, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnSH_LClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnSH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnSH_DClicked(int col, long row, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnCB_LClicked(int updn, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnCB_RClicked(int updn, RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnCB_DClicked(RECT* rect, POINT* point, BOOL processed) = 0;
	virtual void OnKeyDown(UINT* vcKey, BOOL processed) = 0;
	virtual void OnKeyUp(UINT* vcKey, BOOL processed) = 0;
	virtual void OnCharDown(UINT* vcKey, BOOL processed) = 0;
	virtual int OnCanSizeCol(int col) = 0;
	virtual void OnColSizing(int col, int* width) = 0;
	virtual void OnColSized(int col, int* width) = 0;
	virtual int OnCanSizeRow(long row) = 0;
	virtual void OnRowSizing(long row, int* height) = 0;
	virtual void OnRowSized(long row, int* height) = 0;
	virtual int OnCanSizeTopHdg() = 0;
	virtual int OnCanSizeSideHdg() = 0;
	virtual int OnTopHdgSizing(int* height) = 0;
	virtual int OnSideHdgSizing(int* width) = 0;
	virtual int OnTopHdgSized(int height) = 0;
	virtual int OnSideHdgSized(int width) = 0;
	virtual void OnColRowSizeFinished() = 0;
	virtual int SetTH_Height(int height) = 0;
	virtual int SetSH_Width(int width) = 0;
	virtual int OnHint(int col, long row, int section, TOOLTIP_SETTINGS& ttSettings) = 0;
	virtual int MoveTopRow(int flag) = 0;
	virtual int SetTopRow(long row) = 0;
	virtual int MoveCurrentRow(int flag) = 0;
	virtual int SetLeftCol(int col) = 0;
	virtual int MoveLeftCol(int flag) = 0;
	virtual int MoveCurrentCol(int flag) = 0;
	virtual void OnViewMoved(int nScrolDir, long oldPos, long newPos) = 0;
	virtual int GetJoinStartCell(int* col, long* row, CUGCell* cell) = 0;
	virtual int GetJoinRange(int* col, long* row, int* col2, long* row2) = 0;
	virtual int SetColWidth(int col, int width, bool notify) = 0;
	virtual int VerifyCurrentRow(long* newRow) = 0;
	virtual int BestFit(int startCol, int endCol, int CalcRange, int flag) = 0;
	virtual int SetTH_RowHeight(int row, int height) = 0;
	virtual int SetSH_ColWidth(int col, int width) = 0;
	virtual LRESULT MakeSuperTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT, HWND hWnd, int section) = 0;
	virtual void HideTooltip() = 0;
	virtual BOOL OnColSwapStart(int col) = 0;
	virtual BOOL OnCanColSwap(int fromCol, int toCol) = 0;
	virtual void OnColSwapped(int fromCol, int toCol) = 0;
	virtual int MoveColPosition(int fromCol, int toCol, BOOL insertBefore) = 0;
	virtual int SetRowHeight(long row, int height) = 0;
	virtual void HScroll(UINT nSBCode, UINT nPos) = 0; 
#pragma endregion CUGGridInfoIF_fn_

#pragma region GettersSetters
	virtual BOOL CancelMode() const = 0;			//m_bCancelMode
	virtual BOOL Extend() const = 0;				//m_bExtend
	virtual int BallisticDelay() const = 0;			//m_ballisticDelay
	virtual int BallisticMode() const = 0;			//m_ballisticMode
	virtual int BallisticKeyDelay() const = 0;		//m_ballisticKeyDelay
	virtual int BallisticKeyMode() const = 0;		//m_ballisticKeyMode
	virtual long BottomRow() const = 0;				//m_bottomRow
	virtual void SetBottomRow(long row) = 0;
	virtual int CurrentCol() const = 0;				//m_currentCol
	virtual long CurrentRow() const = 0;			//m_currentRow
	virtual int CurrentCellMode() const = 0;		//m_currentCellMode
	virtual int DefColWidth() const = 0;			//m_defColWidth
	virtual int DefRowHeight() const = 0;			//m_defRowHeight
	virtual void SetDefRowHeight(int cy) = 0;	//m_defRowHeight
	virtual CUGDataSource* DefDataSource() const = 0;		//m_defDataSource
	virtual CUGCell& EditCell() = 0;					//m_editCell
	virtual int DragCol() const = 0;				//m_dragCol
	virtual long DragRow() const = 0;				//m_dragRow
	virtual void SetDragCol(int col) = 0;
	virtual void SetDragRow(long row) = 0;
	virtual int EditCol() const = 0;				//m_editCol
	virtual bool EditInProgress() const = 0;		//m_editInProgress
	virtual long EditRow() const = 0;				//m_editRow
	virtual int EnableColSwapping() const = 0;		//m_enableColSwapping
	virtual int EnableJoins() const = 0;			//m_enableJoins
	virtual int EnableExcelBorders() const = 0;		//m_enableExcelBorders
	virtual void EnablePopupMenu(bool enable) = 0;	//m_enablePopupMenu
	virtual bool IsEnablePopupMenu() const = 0;		//m_enablePopupMenu
	virtual int GridHeight() const = 0;				//m_gridHeight
	virtual int GridWidth() const = 0;				//m_gridWidth
	virtual HWND GridWnd() const = 0;				//m_gridWnd
	virtual HWND CtrlWnd() const = 0;				//m_ctrlWnd
	virtual int HScrollMode() const = 0;			//m_hScrollMode
	virtual CRect HScrollRect() const = 0;			//m_hScrollRect
	virtual int HighlightRowFlag() const = 0;		//m_highlightRowFlag
	virtual int LastLeftCol() const = 0;			//m_lastLeftCol
	virtual long LastTopRow() const = 0;			//m_lastTopRow
	virtual int LeftCol() const = 0;				//m_leftCol
	virtual int RightCol() const = 0;				//m_rightCol
	virtual void SetRightCol(int col) = 0;
	virtual int LockColWidth() const = 0;			//m_lockColWidth
	virtual int LockRowHeight() const = 0;			//m_lockRowHeight
	virtual int MaxLeftCol() const = 0;				//m_maxLeftCol
	virtual long MaxTopRow() const = 0;				//m_maxTopRow
	virtual void SetMoveType(int type) = 0;			//m_moveType
	virtual UINT MoveFlags() const = 0;				//m_moveFlags
	virtual void SetMoveFlags(UINT flags) = 0;
	virtual int MultiSelectFlag() const = 0;		//m_multiSelectFlag
	virtual int NumLockCols() const = 0;			//m_numLockCols
	virtual int NumLockRows() const = 0;			//m_numLockRows
	virtual int NumberCols() const = 0;				//m_numberCols
	virtual long NumberRows() const = 0;			//m_numberRows
	virtual int NumberSideHdgCols() const = 0;		//m_numberSideHdgCols
	virtual int NumberTopHdgRows() const = 0;		//m_numberTopHdgRows
	virtual bool PaintMode() const = 0;				//m_paintMode
	virtual bool ShowHScroll() const = 0;			//m_showHScroll
	virtual BOOL ScrollOnPartialCells() const = 0;	//m_bScrollOnParialCells
	virtual int SideHdgWidth() const = 0;			//m_sideHdgWidth
	virtual void SetSideHdgWidth(int cx) = 0;
	virtual int TopHdgHeight() const = 0;			//m_topHdgHeight
	virtual void SetTopHdgHeight(int cy) = 0;
	virtual long TopRow() const = 0;				//m_topRow
	virtual int ThreeDHeight() const = 0;			//m_threeDHeight
	virtual int UniformRowHeightFlag() const = 0;	//m_uniformRowHeightFlag
	virtual int UserBestSizeFlag() const = 0;		//m_userBestSizeFlag
	virtual int UserSizingMode() const = 0;			//m_userSizingMode
	virtual int VScrollMode() const = 0;			//m_vScrollMode
	virtual HCURSOR NSResizseCursor() const = 0;	//m_NSResizseCursor
	virtual HCURSOR WEResizseCursor() const = 0;	//m_WEResizseCursor
	virtual CXeUIcolorsIF* GetXeUI() = 0;			//m_xeUI

	virtual UGCOLINFO& GetColInfo(int colIdx) const = 0;	//m_colInfo
	virtual int GetSideHdgColWidth(int colIdx) const = 0;	//m_sideHdgWidths
	virtual void SetSideHdgColWidth(int colIdx, int cx) = 0;
	virtual int GetTopHdgRowHeight(int rowIdx) const = 0;	//m_topHdgHeights
	virtual void SetTopHdgRowHeight(int rowIdx, int cy) = 0;
	virtual int IsSelected(int col, long row, int* block = nullptr) const = 0;
#pragma endregion GettersSetters
};

/*************************************************************************
				Class Implementation : CUGDataSource
**************************************************************************
	Source file : UGDtaSrc.cpp
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved
*************************************************************************/
/*************************************************************************
				Class Declaration : CUGDataSource
**************************************************************************
	Source file : UGDtaSrc.cpp
	Header file : UGDtaSrc.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		The CUGDataSource class is used by the grid
		as standard interface between the grid
		and its data.  The Ultimate Grid relies
		on CUGDataSource derived class to provide
		it with all of the information that needs
		to be displayed.

		Datasources can be practically anything
		i.e.	arrays
				linked lists
				databases
				flat files
				real-time feeds (sensors)
				calculations

	Details
		This is a base class which all other datasources
		must be derived from. By defining a standard
		interface to the data, an abstract layer is
		created which allows the uderlying data to
		come from any source, plus allows the datasource
		to be changed without any code re-write.

		At the minimum only ONE virtual function must
		be overwitten it is the GetCell. GetCell is
		called by the grid when it needs information
		about a particular cell.

		Even though the grid generally works on a cell by
		cell basis, many datasource (such as databases)
		tend to work on a row by row basis. To allow data
		to be written to the datasource in this manner
		transaction writing can be used within a datasource
		by overwritting the transaction functions.

		If a derived datasource cannot return the number
		of rows that is contains, then overwrite
		the OnHitBottom virtual function. This allows for
		the grid to ask the datasource for new rows on the fly.

		Stanard return values from a datasource are
			UG_NA		- not implemented (-1)
			UG_SUCCESS	- success (0)
			1 and up	- error codes
*************************************************************************/
export class CUGDataSource
{
protected:
	long m_ID;

public:
	CUGGridInfoIF* m_GI = nullptr;

	/***************************************************
		Standard construction/desrtuction
	***************************************************/
	CUGDataSource()
	{
		//m_ctrl = NULL;
		m_ID = -1;
	}

	virtual ~CUGDataSource()
	{}

	//used to check to see if the data source supports a standard function
	//BOOL IsFunctionSupported(long type);

	/***************************************************
	SetID
		Called by the framework to inform the data source
		which index value it is assigned.  This same index
		value is also returned by the AddDataSource function.
	Params:
		ID	- index value assigned to the datasource
	Return:
		<none>
	***************************************************/
	void SetID(long ID)
	{
		m_ID = ID;
	}

	/***************************************************
	GetID
		Is used to determine which index value is assigned
		to given data source class.
	Params:
		<none>
	Return:
		long	- the index assigned.
	***************************************************/
	long GetID()
	{
		return m_ID;
	}

	/////////////////////////////////////////////////////////////////////////////
	//	Virtual Functions

	/***************************************************
	Open
		A virtual function that provides standard interface
		for openning of the data source.  It is most often
		used to open database files, connections, etc.
	Params:
		name
		option
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int Open(LPCTSTR name, LPCTSTR option)
	{
		UNREFERENCED_PARAMETER(name);
		UNREFERENCED_PARAMETER(option);
		return UG_NA;
	}

	/***************************************************
	IsOpen
		virtual function is most commonly used by datasources
		that bind to a database or some form of an external data.
		It is used to provide feedback to the developer who requires
		to know if a connection to the database is currently open
		or closed.

		This virtual function is not applicable to memory based
		datasources (ie. grid's default CUGMem).
	Params:
		<none>
	Return:
		FALSE if the datasource is closed and TRUE if it is open.
	***************************************************/
	virtual BOOL IsOpen()
	{
		return FALSE;
	}

	/***************************************************
	SetPassword
		A virtual function that provides standard interface
		to set user name and password used to open the data
		source.
	Params:
		user	- user name to use
		pass	- password
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int SetPassword(LPCTSTR user, LPCTSTR pass)
	{
		UNREFERENCED_PARAMETER(user);
		UNREFERENCED_PARAMETER(pass);
		return UG_NA;
	}

	/***************************************************
	Close
		A virtual function that provides standard interface
		to close the data source file or connection.
	Params:
		<none>
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int Close()
	{
		return UG_NA;
	}

	/***************************************************
	Save
		A virtual function that provides standard interface
		that allows the data source to be saved to file.
		This function is mostly needed with datasources that
		either do not update the data source as user makes
		changes (just like Excel), or allow for the gird's
		bound data to be saved to a file.
	Params:
		<none>
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int Save()
	{
		return UG_NA;
	}

	/***************************************************
	SaveAs
		A virtual function that provides standard interface
		that allows the data source to be saved to file.
		This function is mostly needed with datasources that
		either do not update the data source as user makes
		changes (just like Excel), or allow for the gird's
		bound data to be saved to a file.
	Params:
		name	- file or connection name
		option	- save options
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int SaveAs(LPCTSTR name, LPCTSTR option)
	{
		UNREFERENCED_PARAMETER(name);
		UNREFERENCED_PARAMETER(option);
		return UG_NA;
	}

	/***************************************************
	GetNumRows
		A virtual function that provides standard interface
		for the grid to find out how many rows are in the
		data source.  If the data source is not able to
		determine how many rows there are, than it should
		only return a value greater than zero defining how
		many rows it is aware of.  The grid then will use
		the OnHitBottom notification to check if there are
		additional rows.
	Params:
		<none>
	Return:
		UG_NA		not available
		long		number of rows
	****************************************************/
	virtual long GetNumRows()
	{
		return UG_NA;
	}

	/***************************************************
	GetNumCols
		A virtual function that provides standard interface
		for the grid to find out how many columns are in the
		data source.
	Params:
		<none>
	Return:
		UG_NA		not available
		long		number of cols
	****************************************************/
	virtual int GetNumCols()
	{
		return UG_NA;
	}

	/***************************************************
	GetColName
		A virtual function that provides standard interface
		to provide the grid with the name of a column.
	Params:
		col		- column number for which to return name
		string	- pointer to a string which should be populated
				  with the column name.
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int GetColName(int col, std::wstring* string)
	{
		UNREFERENCED_PARAMETER(col);
		UNREFERENCED_PARAMETER(*string);
		return UG_NA;
	}

	/***************************************************
	GetColFromName
		A virtual function that provides standard interface
		to return a column number based on the column name.
	Params:
		name	- name of the column to look for
		col		- ponter to an integer value representing
				  column location.
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int GetColFromName(LPCTSTR name, int* col)
	{
		UNREFERENCED_PARAMETER(name);
		UNREFERENCED_PARAMETER(*col);
		return UG_NA;
	}

	/***************************************************
	GetColType
		A virtual function that provides standard interface
		to return the default data type set to a column.
	Params:
		col		- column of interest
		type	- ponter to an integer that stores information
				  about the data type.  Possible data types can be:
					UGCELLDATA_STRING	(1)	string
					UGCELLDATA_NUMBER	(2)	number
					UGCELLDATA_BOOL		(3)	booliean
					UGCELLDATA_TIME		(4)	date/time
					UGCELLDATA_CURRENCY	(5)	currency
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int GetColType(int col, int* type)
	{
		UNREFERENCED_PARAMETER(col);
		UNREFERENCED_PARAMETER(*type);
		return UG_NA;
	}

	/***************************************************
	AppendRow
		A virtual function that provides standard interface
		to append a new row at the end of the current data
		in the data source.
	Params:
		<none>
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int AppendRow()
	{
		return UG_NA;
	}

	/***************************************************
	AppendRow
		This version of the AppendRow function allows
		to append new row with pre-populated cell objects.
		This is mostly used when adding new records to
		tables that require for some entries to have
		a value (ie. secondary keys, IDs, etc).
	Params:
		cellList	- pointer to array of cell objects
		numCells	- integer value indicating number of
					  elements in the array.
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int AppendRow(CUGCell** cellList, int numCells)
	{
		UNREFERENCED_PARAMETER(**cellList);
		UNREFERENCED_PARAMETER(numCells);
		return UG_NA;
	}

	/***************************************************
	InsertRow
		A virtual function that provides standard interface
		to inserts a row at specified location in the
		current data.
	Params:
		row			- position at which to insert new row
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int InsertRow(long row)
	{
		UNREFERENCED_PARAMETER(row);
		return UG_NA;
	}

	/***************************************************
	AppendCol
		A virtual function that provides standard interface
		to append a new column at the end of the current data
		in the data source.
	Params:
		<none>
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int AppendCol()
	{
		return UG_NA;
	}

	/***************************************************
	InsertCol
		A virtual function that provides standard interface
		to inserts a column at specified location in the
		current data.
	Params:
		col			- column at which to insert new column
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int InsertCol(int col)
	{
		UNREFERENCED_PARAMETER(col);
		return UG_NA;
	}

	/***************************************************
	DeleteRow
		A virtual function that provides standard interface
		to delete specified row from the data source.
	Params:
		row			- indicates the row number to delete
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int DeleteRow(long row)
	{
		UNREFERENCED_PARAMETER(row);
		return UG_NA;
	}

	/***************************************************
	DeleteCol
		A virtual function that provides standard interface
		to delete specified column from the data source.
	Params:
		col			- indicates the column number to delete
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int DeleteCol(int col)
	{
		UNREFERENCED_PARAMETER(col);
		return UG_NA;
	}

	/***************************************************
	Empty
		A virtual function that provides standard interface
		to delete everything that is storred in the data source.
		This function should be accompanied by functions that
		set number of columnd and rows to zero.
	Params:
		<none>
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int Empty()
	{
		return UG_NA;
	}

	/***************************************************
	Delete
		A virtual function that provides standard interface
		to delete a single cell from the data source.
		Deleting a single cell from the data source usually
		means that cell's value should be cleared (deleted).
	Params:
		col, row	- coordinates of the cell to delete.
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int DeleteCell(int col, long row)
	{
		UNREFERENCED_PARAMETER(col);
		UNREFERENCED_PARAMETER(row);
		return UG_NA;
	}

	/***************************************************
	GetCell
		A virtual function that provides standard way
		for the grid to populate a cell object.  This
		function is called as a result of the
		CUGCtrl::GetCell being called.
	Params:
		col, row	- coordinates of the cell to retrieve
					  information on.
		cell		- pointer to CUGCell object to populate
					  with the information found.
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int GetCell(int col, long row, CUGCell* cell)
	{
		UNREFERENCED_PARAMETER(col);
		UNREFERENCED_PARAMETER(row);
		UNREFERENCED_PARAMETER(*cell);
		return UG_NA;
	}

	/***************************************************
	SetCell
		This virtual function is called as a result of
		a call to CUGCtrl::SetCell in attempts to set
		new value to a cell in the data source.
	Params:
		col, row	- coordinates of the cell to set new
					  information to.
		cell		- pointer to CUGCell object to that
					  contains new cell's value.
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int SetCell(int col, long row, CUGCell* cell)
	{
		UNREFERENCED_PARAMETER(col);
		UNREFERENCED_PARAMETER(row);
		UNREFERENCED_PARAMETER(*cell);
		return UG_NA;
	}

	/***************************************************
	FindFirst
		A virtual function that provides standard interface
		to find first occurence of specified value (string)
		in a column based on the flags passed in.
	Params:
		string		- string to look for
		col			- pointer to integer value identifying
					  which column contains the found value
		row			- pointer to a long integer value that
					  identifys the row which contains the
					  value found
		flags		- find flags
						(1) UG_FIND_PARTIAL
						(2) UG_FIND_CASEINSENSITIVE
						(4) UG_FIND_UP
						(8) UG_FIND_ALLCOLUMNS
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int FindFirst(std::wstring* string, int* col, long* row, long flags)
	{
		UNREFERENCED_PARAMETER(*string);
		UNREFERENCED_PARAMETER(*col);
		UNREFERENCED_PARAMETER(*row);
		UNREFERENCED_PARAMETER(flags);
		return UG_NA;
	}

	/***************************************************
	FindFirst
		A virtual function that provides standard interface
		to find next occurence of specified value (string)
		in a column based on the flags passed in.
	Params:
		string		- string to look for
		col			- pointer to integer value identifying
					  which column contains the found value
		row			- pointer to a long integer value that
					  identifys the row which contains the
					  value found
		flags		- find flags
						(1) UG_FIND_PARTIAL
						(2) UG_FIND_CASEINSENSITIVE
						(4) UG_FIND_UP
						(8) UG_FIND_ALLCOLUMNS
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int FindNext(std::wstring* string, int* col, long* row, int flags)
	{
		UNREFERENCED_PARAMETER(*string);
		UNREFERENCED_PARAMETER(*col);
		UNREFERENCED_PARAMETER(*row);
		UNREFERENCED_PARAMETER(flags);
		return UG_NA;
	}

	/***************************************************
	SortBy
		A virtual function that provides standard interface
		to sort data in the data source.  This function is
		never called by the grid, but it can be called
		directly.
	Params:
		col			- the column to sort
		flags		- sort flag identifying the sort direction
						UG_SORT_ASCENDING
						UG_SORT_DESCENDING
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int SortBy(int col, int flags)
	{
		UNREFERENCED_PARAMETER(col);
		UNREFERENCED_PARAMETER(flags);
		return UG_NA;
	}

	/***************************************************
	SortBy
		A virtual function that provides standard interface
		to sort data in the data source.  This function is
		called when user calls CUGCtrl::SortBy
	Params:
		cols		- array of columns to be sorted, in the
					  sort order.
		num			- number of elements in the array
		flags		- sort flag identifying the sort direction
						UG_SORT_ASCENDING
						UG_SORT_DESCENDING
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int SortBy(int* cols, int num, int flags)
	{
		UNREFERENCED_PARAMETER(*cols);
		UNREFERENCED_PARAMETER(num);
		UNREFERENCED_PARAMETER(flags);
		return UG_NA;
	}

	/***************************************************
	SetOption
		Datasource dependant function. Used to set data source
		specific information and modes of operation
	Params:
		option		- integer identifying the option to set
		param1		- option depanded parameter
		param2		- option depanded parameter
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int SetOption(int option, long param1, long param2)
	{
		UNREFERENCED_PARAMETER(option);
		UNREFERENCED_PARAMETER(param1);
		UNREFERENCED_PARAMETER(param2);
		return UG_NA;
	}

	/***************************************************
	GetOption
		Datasource dependant function. Used to get data source
		specific information and modes of operation
	Params:
		option		- integer identifying the option to set
		param1		- option depanded parameter
		param2		- option depanded parameter
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int GetOption(int option, long& param1, long& param2)
	{
		UNREFERENCED_PARAMETER(option);
		UNREFERENCED_PARAMETER(param1);
		UNREFERENCED_PARAMETER(param2);
		return UG_NA;
	}

	/****************************************************
	GetPrevNonBlankCol
		A virtual function that provides standard interface
		for the cell type to complete the cell over lap
		functionality.
	Params:
		col			- pointer to an integer value used
					  identify which column the cell type
					  is working with, and to return column
					  number that contains a value.
		row			- row the cell type is working with.
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int GetPrevNonBlankCol(int* col, long row)
	{
		UNREFERENCED_PARAMETER(*col);
		UNREFERENCED_PARAMETER(row);
		return UG_NA;
	}

	/****************************************************
	StartTransaction
		A virtual function that provides standard method
		to start a transaction.  Very important when
		working with databases.
	Params:
		<none>
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int StartTransaction()
	{
		return UG_NA;
	}

	/****************************************************
	CancelTransaction
		A virtual function that provides standard method
		to cancel (undo) changes that were made after
		last call to the StartTransaction.
	Params:
		<none>
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int CancelTransaction()
	{
		return UG_NA;
	}

	/****************************************************
	FinishTransaction
		A virtual function that provides standard method
		to make permanent the changes that were made
		after the last call to the StartTransaction.
	Params:
		<none>
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int FinishTransaction()
	{
		return UG_NA;
	}

	/***************************************************
	OnHitBottom
		This notification allows for dynamic row loading,
		it will be called when the grid's drawing function
		has hit the last row.  It allows the grid to ask
		the datasource/developer if there are additional
		rows to be displayed.
	Params:
		numrows		- known number of rows in the grid
		rowspast	- number of extra rows that the grid
					  is looking for in the datasource
		rowsfound	- number of rows actually found,
					  usually equal to rowspast or zero.
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int OnHitBottom(long numrows, long rowspast, long* rowsfound)
	{
		UNREFERENCED_PARAMETER(numrows);
		UNREFERENCED_PARAMETER(rowspast);
		UNREFERENCED_PARAMETER(*rowsfound);
		return UG_NA;
	}

	/***************************************************
	OnHitTop
		Is called when the user has scrolled all the way
		to the top of the grid.
	Params:
		numrows		- known number of rows in the grid
		rowspast	- number of extra rows that the grid
					  is looking for in the datasource
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual int OnHitTop(long numrows, long rowspast, long* rowsfound)
	{
		UNREFERENCED_PARAMETER(numrows);
		UNREFERENCED_PARAMETER(rowspast);
		UNREFERENCED_PARAMETER(*rowsfound);
		return UG_NA;
	}

	/***************************************************
	OnRowChange
		Sent whenever the current row changes
	Params:
		oldrow		- row that is loosing the locus
		newrow		- row that user moved into
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual void OnRowChange(long oldRow, long newRow)
	{
		UNREFERENCED_PARAMETER(oldRow);
		UNREFERENCED_PARAMETER(newRow);
	}

	/***************************************************
	OnColChange
		Sent whenever the current column changes
	Params:
		oldcol		- column that is loosing the focus
		newcol		- column that the user move into
	Return:
		UG_NA		not available
		UG_SUCCESS	success
		1...		error codes (data source dependant)
	****************************************************/
	virtual void OnColChange(int oldCol, int newCol)
	{
		UNREFERENCED_PARAMETER(oldCol);
		UNREFERENCED_PARAMETER(newCol);
	}

	/***************************************************
	OnCanMove
		is sent when a cell change action was instigated
		( user clicked on another cell, used keyboard arrows,
		or Goto[...] function was called ).
	Params:
		oldcol, oldrow -
		newcol, newrow - cell that is gaining focus
	Return:
		TRUE - to allow the move
		FALSE - to prevent new cell from gaining focus
	****************************************************/
	virtual int OnCanMove(int oldcol, long oldrow, int newcol, long newrow)
	{
		UNREFERENCED_PARAMETER(oldcol);
		UNREFERENCED_PARAMETER(oldrow);
		UNREFERENCED_PARAMETER(newcol);
		UNREFERENCED_PARAMETER(newrow);
		return TRUE;
	}

	/***************************************************
	OnEditStart
		This message is sent whenever the grid is ready to
		start editing a cell
	Params:
		col, row - location of the cell that edit was requested over
		edit -	pointer to a pointer to the edit control,
				allows for swap of edit control if edit
				control is swapped permanently (for the
				whole grid) is it better to use 'SetNewEditClass'
				function.
	Return:
		TRUE - to allow the edit to start
		FALSE - to prevent the edit from starting
	****************************************************/
	virtual int OnEditStart(int col, long row, HWND edit)
	{
		UNREFERENCED_PARAMETER(col);
		UNREFERENCED_PARAMETER(row);
		UNREFERENCED_PARAMETER(edit);
		return TRUE;
	}
	/***************************************************
	OnEditVerify
		This notification is sent every time the user hits
		a key while in edit mode.  It is mostly used to create
		custom behavior of the edit contol, because it is
		so eazy to allow or disallow keys hit.
	Params:
		col, row	- location of the edit cell
		edit		-	pointer to the edit control
		vcKey		- virtual key code of the pressed key
	Return:
		TRUE - to accept pressed key
		FALSE - to do not accept the key
	****************************************************/
	virtual int OnEditVerify(int col, long row, HWND edit, UINT* vcKey)
	{
		UNREFERENCED_PARAMETER(col);
		UNREFERENCED_PARAMETER(row);
		UNREFERENCED_PARAMETER(*edit);
		UNREFERENCED_PARAMETER(*vcKey);
		return TRUE;
	}
	/***************************************************
	OnEditFinish
		This notification is sent when the edit is being finised
	Params:
		col, row	- coordinates of the edit cell
		edit		- pointer to the edit control
		string		- actual string that user typed in
		cancelFlag	- indicates if the edit is being cancelled
	Return:
		TRUE - to allow the edit it proceede
		FALSE - to force the user back to editing of that same cell
	****************************************************/
	virtual int OnEditFinish(int col, long row, HWND edit, LPCTSTR string, BOOL cancelFlag)
	{
		UNREFERENCED_PARAMETER(col);
		UNREFERENCED_PARAMETER(row);
		UNREFERENCED_PARAMETER(*edit);
		UNREFERENCED_PARAMETER(string);
		UNREFERENCED_PARAMETER(cancelFlag);
		return TRUE;
	}
	/***************************************************
	OnEditContinue
		This notification is called when the user pressed
		'tab' or 'enter' keys Here you have a chance
		to modify the destination cell
	Params:
		oldcol, oldrow - edit cell that is loosing edit focus
		newcol, newrow - cell that the edit is going into,
						 by changing their values you are able
						 to change where to edit next
	Return:
		TRUE - allow the edit to continue
		FALSE - to prevent the move, the edit will be stopped
	****************************************************/
	virtual int OnEditContinue(int oldcol, long oldrow, int* newcol, long* newrow)
	{
		UNREFERENCED_PARAMETER(oldcol);
		UNREFERENCED_PARAMETER(oldrow);
		UNREFERENCED_PARAMETER(*newcol);
		UNREFERENCED_PARAMETER(*newrow);
		return TRUE;
	}
};
