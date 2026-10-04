module;

#include "../os_minimal.h"
#include <algorithm>
#include <string>
#include <d2d1.h>
#include "ugdefine.h"

export module Xe.UGCelTypIF;

import Xe.UGCell;
//import Xe.UGGridInfoIF;
//
import Xe.UIcolorsIF;
//
import Xe.D2DRenderContext;

//#ifdef UG_ENABLE_PRINTING
//#undef UG_ENABLE_PRINTING
//#endif
//import Xe.FastStrings;

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif

//#ifndef _T
//#define _T(quote) TEXT(quote) 
//#endif

// MACROs used in printing bitmap
//#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER)) 
//#define RECTWIDTH(lpRect)   ((lpRect)->right - (lpRect)->left) 
//#define RECTHEIGHT(lpRect)  ((lpRect)->bottom - (lpRect)->top) 
//// WIDTHBYTES performs DWORD-aligning of DIB scanlines.  The "bits"
//// parameter is the bit count for the scanline (biWidth * biBitCount),
//// and this macro returns the number of DWORD-aligned bytes needed
//// to hold those bits.
//#ifndef WIDTHBYTES
//#define WIDTHBYTES(bits)        ((unsigned)((bits+31)&(~31))/8)  /* ULONG aligned ! */
//#endif

//class CUGGridInfo;

export class CUGCellTypeIF
{
public:
	/***************************************************
	GetName
		Returns a readable name for the cell type.
		Returned value is used to help end-users
		to see what cell type are available.
	Params:
		<none>
	Return
		cell type name
	****************************************************/
	virtual LPCTSTR GetName() = 0;

	/***************************************************
	GetUGID
		Returns a GUID for the cell type, this number
		is unique for each cell type and never changes.
		This number can be used to find the cell types
		added to this instance of the Ultimate Grid.
	Params:
		<none>
	Returns:
		UGID (which is actually a GUID)
	****************************************************/
	virtual LPCUGID GetUGID() = 0;

	/***************************************************
	GetEditArea
		Returns the editable area of a cell type.
		Some celltypes (i.e. drop list) require to
		have certain portion of the cell not covered
		by the edit control.  In case of the drop list
		the drop button should not be covered.
	Params:
		rect - pointer to the cell rectangle, it can
			   be used to modify the edit area
	Returns:
		UG_SUCCESS or UG_ERROR, currently the Utltimate
		Grid does not check the return value.
	****************************************************/
	virtual int GetEditArea(RECT* rect) = 0;

	/***************************************************
	OnSystemChange
		This function is called for each cell type
		when system settings change, such as screen
		resolution and or colors.
	Params:
		none
	Return:
		UG_SUCCESS - success
		UG_ERROR - error
	****************************************************/
	virtual int OnSystemChange() = 0;

	/***************************************************
	OnMessage
		This function is called when a windows message
		(UGCT_MESSAGE) was sent to this cell type.
		The wParam part of the message must contain the
		celltype ID (that is returned when a celltype
		is registered).
	Params:
		lParam - generic data, cell type dependant
				comes from the lParam paramenter of
				a windows message
	Return:
		Value is celltype dependant
	****************************************************/
	virtual long OnMessage(LPARAM lParam) = 0;

	/***************************************************
	OnCellTypeNotify
		This function is called whenever a celltype wants
		to fire a notification. By default this function
		calls through to CUGCtrl::OnCellTypeNotify, which
		developers generally override to handle the
		notifications. However this function is useful
		to override when creating a customized cell type
		based off an existing cell type.
	Params:
		ID - cell type ID
		col - column of the cell firing the notification
		row - row of the cell firing the notification
		msg - notification message
		param - notification dependant data
	Return:
		Value is dependant on the notification being fired
	****************************************************/
	virtual int OnCellTypeNotify(long ID, int col, long row, long msg, long long param) = 0;

	/***************************************************
	OnLClicked
		This function is called when the left mouse
		button is clicked over a cell using this cell
		type.
	Params:
		col - column that was clicked in
		row - row that was clicked in
		updn - TRUE if the mouse button just went down
			 - FALSE if the mouse button just went up
		rect - rectangle of the cell that was clicked in
		point - point where the mouse was clicked
	Return:
		TRUE - if the event was processed
		FALSE - if the event was not
	****************************************************/
	virtual BOOL OnLClicked(int col, long row, int updn, RECT* rect, POINT* point) = 0;

	/***************************************************
	OnRClicked
		This function is called when the right mouse
		button is clicked over a cell using this cell
		type.
	Params:
		col - column that was clicked in
		row - row that was clicked in
		updn - TRUE if the mouse button just went down
			 - FALSE if the mouse button just went up
		rect - rectangle of the cell that was clicked in
		point - point where the mouse was clicked
	Return:
		TRUE - if the event was processed
		FALSE - if the event was not
	****************************************************/
	virtual BOOL OnRClicked(int col, long row, int updn, RECT* rect, POINT* point) = 0;

	/***************************************************
	OnDClicked
		This function is called when the left mouse
		button is double clicked over a cell using this cell
		type.
	Params:
		col - column that was clicked in
		row - row that was clicked in
		rect - rectangle of the cell that was clicked in
		point - point where the mouse was clicked
	Return:
		TRUE - if the event was processed
		FALSE - if the event was not
	****************************************************/
	virtual BOOL OnDClicked(int col, long row, RECT* rect, POINT* point) = 0;

	/***************************************************
	OnKeyDown
		This function is called when a cell of this type
		has focus and a key is pressed.(See WM_KEYDOWN)
	Params:
		col - column that has focus
		row - row that has focus
		vcKey - pointer to the virtual key code, of the
			key that was pressed
	Return:
		TRUE - if the event was processed
		FALSE - if the event was not
	****************************************************/
	virtual BOOL OnKeyDown(int col, long row, UINT* vcKey) = 0;

	/***************************************************
	OnKeyUp
		This function is called when a cell of this type
		has focus and a key was unpressed.(See WM_KEYUP)
	Params:
		col - column that has focus
		row - row that has focus
		vcKey - pointer to the virtual key code, of the
			key that was unpressed
	Return:
		TRUE - if the event was processed
		FALSE - if the event was not
	****************************************************/
	virtual BOOL OnKeyUp(int col, long row, UINT* vcKey) = 0;

	/***************************************************
	OnCharDown
		This function is called when a cell of this type
		has focus and a printable key is pressed.(WM_CHARDOWN)
	Params:
		col - column that has focus
		row - row that has focus
		vcKey - pointer to the virtual key code, of the
			key that was pressed
	Return:
		TRUE - if the event was processed
		FALSE - if the event was not
	****************************************************/
	virtual BOOL OnCharDown(int col, long row, UINT* vcKey) = 0;

	/***************************************************
	OnMouseMove
		This function is called when the mouse  is over
		a cell of this celltype.
	Params:
		col - column that the mouse is over
		row - row that the mouse is over
		point - point where the mouse is
		flags - mouse move flags (see WM_MOUSEMOVE)
	Return:
		TRUE - if the event was processed
		FALSE - if the event was not
	****************************************************/
	virtual BOOL OnMouseMove(int col, long row, POINT* point, UINT flags) = 0;

	/***************************************************
	OnChangedCellWidth
		This notification is sent to all visible
		cells is affected column when the user has
		changed width of a column.
	Params:
		col - column that the mouse is over
		row - row that the mouse is over
		width - pointer to new column width
	Return:
		<none>
	****************************************************/
	virtual void OnChangedCellWidth(int col, long row, int* width) = 0;

	/***************************************************
	OnChangingCellWidth
		This notification is sent to all visible
		cells is affected column while the user is
		changing width of a column.
	Params:
		col - column that the mouse is over
		row - row that the mouse is over
		width - pointer to new column width
	Return:
		<none>
	****************************************************/
	virtual void OnChangingCellWidth(int col, long row, int* width) = 0;

	/***************************************************
	OnChangedCellHeight
		This notification is sent to all visible
		cells is affected row when the user has
		changed height of a column.
	Params:
		col - column that the mouse is over
		row - row that the mouse is over
		height - pointer to new row height
	Return:
		<none>
	****************************************************/
	virtual void OnChangedCellHeight(int col, long row, int* height) = 0;

	/***************************************************
	OnChangingCellHeight
		This notification is sent to all visible
		cells is affected row while the user is
		changing height of a row.
	Params
		col - column that the mouse is over
		row - row that the mouse is over
		height - pointer to new row height
	Return
		<none>
	****************************************************/
	virtual void OnChangingCellHeight(int col, long row, int* height) = 0;

	/***************************************************
	SetOption
		This virtual function, although not implemented
		by default by any of cell types that are
		shipped with Ultimate Grid, was added
		to provide standard way to set additional
		properies of a cell.
	Params:
		option - option ID number
		param - value for the specified option
	Return:
		UG_NA - not implemented
		UG_SUCCESS - success
		UG_ERROR - error
	****************************************************/
	virtual int SetOption(long option, long param) = 0;

	/***************************************************
	GetOption
		This virtual function, although not implemented
		by default by any of cell types that are
		shipped with Ultimate Grid, was added
		to provide standard way to get additional
		properies of a cell.
	Params:
		option - option ID number
		param - value for the specified option
	Return:
		UG_NA - not implemented
		UG_SUCCESS - success
		UG_ERROR - error
	****************************************************/
	virtual int GetOption(long option, long* param) = 0;

	/***************************************************
	OnSetFocus
		This function is called when a cell of this
		type receives focus.
	Params
		col - column that just received focus
		row - row that just received focus
		cell - pointer to the cell object located at col/row
	Return
		<none>
	****************************************************/
	virtual void OnSetFocus(int col, long row, CUGCell* cell) = 0;

	/***************************************************
	OnKillFocus
		This function is called when a cell of this
		type loses focus.
	Params
		col - column that just lost focus
		row - row that just lost focus
		cell - pointer to the cell object located at col/row
	Return
		<none>
	****************************************************/
	virtual void OnKillFocus(int col, long row, CUGCell* cell) = 0;

	/***************************************************
	OnDraw
		The Ultimate Grid calls this vistual function
		every time it is drawing a cell.  It is upto
		the individual cell type to properly draw itself.
	Params:
		dc		- device context to draw the cell with
		rect	- rectangle to draw the cell in
		col		- column that is being drawn
		row		- row that is being drawn
		cell	- cell that is being drawn
		selected- TRUE if the cell is selected, otherwise FALSE
		current - TRUE if the cell is the current cell, otherwise FALSE
	Return
		<none>
	****************************************************/
	virtual void OnDraw(CXeD2DRenderContext* pRctx, EXE_FONT eFont, RECT* rect, int col, long row, CUGCell* cell,
		int selected, int current) = 0;

	/***************************************************
	DrawText
		This function is the standard text drawing routine
		used by this cell type and used by many others.
	Params:
		dc		- device context to draw the cell with
		rect	- rectangle to draw the cell in
		col		- column that is being drawn
		row		- row that is being drawn
		cell	- cell that is being drawn
		selected- TRUE if the cell is selected, otherwise FALSE
		current - TRUE if the cell is the current cell, otherwise FALSE
	Return:
		<none>
	****************************************************/
	virtual void DrawText(CXeD2DRenderContext* pRctx, EXE_FONT eFont, RECT* rect, int offset, int col, long row, CUGCell* cell, int selected, int current) = 0;

	/***************************************************
	DrawBackground
	Params:
	Return:
		<none>
	****************************************************/
	virtual void DrawBackground(CXeD2DRenderContext* pRctx, RECT* rect, COLORREF backcolor) = 0;

	/***************************************************
	Draw Border
		Draws a border using the style set, possible
		styles are:
						   Left			   Top			   Right		   Bottom
					   |---------------|---------------|---------------|---------------
			Thin:		UG_BDR_LTHIN	UG_BDR_TTHIN	UG_BDR_RTHIN	UG_BDR_BTHIN
			Medium:		UG_BDR_LMEDIUM	UG_BDR_TMEDIUM	UG_BDR_RMEDIUM	UG_BDR_BMEDIUM
			Thick:		UG_BDR_LTHICK	UG_BDR_TTHICK	UG_BDR_RTHICK	UG_BDR_BTHICK
			3DRecess:	UG_BDR_RECESSED
			3DRaised:	UG_BDR_RAISED
	Params:
		dc		- device context to draw on
		rect	- is the area to draw the border in
		rectout	- returns the area inside the border
		cell	- cell for which to draw the border for.
	Returns:
		<none>
	****************************************************/
	virtual void DrawBorder(CXeD2DRenderContext* pRctx, RECT* rect, RECT* rectout, CUGCell* cell) = 0;

	/****************************************************
	GetBestSize
		Returns the best (nominal) size for a cell using
		this cell type, with the given cell properties.
	Params:
		dc		- device context to use to calc the size on
		size	- return the best size in this param
		cell	- pointer to a cell object to use for the calc.
	Return:
		<none>
	*****************************************************/
	virtual void GetBestSize(CSize* size, CUGCell* cell) = 0;

	/****************************************************
	OnScrolled
		This event is called for all celltypes currently added
		to the grid when the view area is scrolled.
	Params:
		col, row	- cell coordinates identifying current cell
		cell		- pointer to current cell
	Return:
		<none>
	*****************************************************/
	virtual void OnScrolled(int col, long row, CUGCell* cell) = 0;
};

