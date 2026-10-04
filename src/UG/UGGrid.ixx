module;

/*************************************************************************
				Class Implementation : CUGGrid
**************************************************************************
	Source file : UGGrid.cpp
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved
*************************************************************************/
/*************************************************************************
				Class Declaration : CUGGrid
**************************************************************************
	Source file : UGGrid.cpp
	Header file : UGGrid.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		The CUGGrid takes care of the main grid area,
		this is where all of the cells are drawn and
		most user interaction takes place.
	Details
		- This class handles drawing of all cells,
		  excluding cells on headings.
		- All of the mouse and keyboard events
		  are routed to the CUGCtrl derived class
		  in form of virtual functions (notifications).
*************************************************************************/

#include "../os_minimal.h"
#include <d2d1.h>
#include <dwrite.h>

#include "ugdefine.h"

export module Xe.UGGrid;

import std;
import Xe.UIcolorsIF;

import Xe.D2DWndBase;
import Xe.UGGridInfoIF;
import Xe.UGDrawHint;
import Xe.UGCelTyp;
import Xe.UGCell;
import Xe.UGMultiSelect;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef WM_MOUSEWHEEL
#define ON_WM_MOUSEWHEEL
#endif

//class CUGGridInfo;

export class CUGGrid : public CXeD2DWndBase
{
public:
	CUGGridInfoIF* m_GI;			//pointer to the grid information

	//CBitmap *		m_bitmap;		//double buffering
	//int				m_doubleBufferMode;

	CUGCell			m_cell;			//general purpose cell object

	long			m_keyRepeatCount; //key ballistics repeat counter

	int				m_hasFocus;

	BOOL			m_cellTypeCapture;

	BOOL			m_tempDisableFocusRect;

protected:
	// This protected member is used to prevent the grid from un-matched
	// mouse button events.  This member was added in attempt to work around
	// a Windows bug, when user double clicks within a window 4 messages are
	// sent: WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK, WM_LBUTTONUP
	// On many occasions popup windows are closed on the double click event
	// causing the last mouse up message be sent to any window under current
	// mouse cursor.
	BOOL m_bHandledMouseDown;

public:
	/***************************************************
		Standard construction/desrtuction
	***************************************************/
	CUGGrid(CXeUIcolorsIF* pUIcolors) : CXeD2DWndBase(pUIcolors)
	{
		//m_doubleBufferMode	= FALSE;
		m_keyRepeatCount = 0;
		m_hasFocus = FALSE;
		m_cellTypeCapture = FALSE;
		//m_bitmap			= NULL;
		m_tempDisableFocusRect = FALSE;
		m_bHandledMouseDown = FALSE;
	}

	virtual ~CUGGrid()
	{
		//if(m_bitmap != NULL)
		//	delete m_bitmap;
	}

	bool CreateGrid(DWORD dwStyle, const CRect& rect, HWND hParentWnd, UINT nID)
	{
		std::wstring classname = L"CUGGrid_WNDCLASS";
		m_GI->GetXeUI()->RegisterWindowClass(classname, D2DCtrl_WndProc);
		dwStyle = dwStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		HWND hWnd = CreateD2DWindow(0, classname.c_str(), nullptr, dwStyle,
			rect, hParentWnd, nID, true);
		m_isHideTooltipOnMouseLeave = false;
		return hWnd != 0;
	}

protected:
	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		return CXeD2DWndBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
	}

	//LRESULT OnMouseLeave(WPARAM wparam, LPARAM lparam)
	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override
	{
		//POINT point;
		//GetCursorPos(&point);
		//CWnd *pWnd = WindowFromPoint(point);
		//HWND hWnd = pWnd->GetSafeHwnd();
		//if (hWnd != m_xtooltip->GetSafeHwnd())	// Don't hide tooltip if cursor is over it.
		//{
		//	m_xtooltip->HideTooltip();
		//}
		if (!m_xeUI->IsMouseOverTooltip(Hwnd()))
		{
			_HideTooltip();
		}
		return CXeD2DWndBase::_OnMouseLeave(wparam, lparam);
	}

	virtual LRESULT _OnNotify_NeedTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT) override
	{
		XeASSERT(pNeedTT);
		if (pNeedTT && !m_GI->EditInProgress())	// Don't show tooltips when a cell is in edit mode.
		{
			return m_GI->MakeSuperTooltip(pNeedTT, GetSafeHwnd(), UG_GRID);
		}
		return 0;
	}

	/***************************************************
	OnPaint
		This routine is responsible for gathering the
		information to draw, get the selected state
		plus draw in an optomized fashion
	Params:
		<none>
	Returns:
		<none>
	*****************************************************/
	//void OnPaint() 
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override
	{
		if (!m_GI->PaintMode())
			return;

		pRT->Clear(m_xeUI->GetColorF(CID::GrdCellDefBg));

		//CClientDC dc(this);
		//redraw the cells in any invalid region
		//CRect clipRect;

		//if ( ::GetUpdateRect(Hwnd(), clipRect, FALSE) == 0 )
		//	return;

		::ValidateRect(Hwnd(), NULL);

		//if( dc.GetClipBox( clipRect ) != NULLREGION )
		//if ( clipRect.left == 0 && clipRect.right == 0 &&
		//	 clipRect.top == 0 && clipRect.bottom == 0 )
		//{
		//	// redraw all cells in the grid.
		//	m_GI->m_drawHintGrid.AddHint(0,0,m_GI->m_numberCols,m_GI->m_numberRows);
		//}
		//else
		{
			int startCol = 0,
				endCol = m_GI->NumberCols();
			long startRow = 0,
				endRow = m_GI->NumberRows();
			// determine the top-left and bottom-right cells of the clip rectangle
			//m_GI->GetCellFromPointColRow( clipRect.left, clipRect.top, &startCol, &startRow );
			//m_GI->GetCellFromPointColRow( clipRect.right, clipRect.bottom, &endCol, &endRow );
			// if the bottom right cell in the range is part of a join,
			// than get information on 'join end' cell.
			CUGCell cell;
			//m_ctrl->GetCell( endCol, endRow, &cell );
			m_GI->GetCellIndirect(endCol, endRow, &cell);
			BOOL bOrigin;
			int stCol;
			long stRow;
			if (cell.GetJoinInfo(&bOrigin, &stCol, &stRow) == UG_SUCCESS)
			{
				endCol = endCol + stCol;
				endRow = endRow + stRow;
			}

			// redraw the region of cells that are affected by the invalid rect
			//m_GI->m_drawHintGrid.AddHint( startCol, startRow, endCol, endRow );

			// if the current cell is not part of the clip region,
			// add the current cell to the draw hints
			//if (!(startCol >= m_GI->GetCurrentCol() && endCol <= m_GI->GetCurrentCol() &&
			//	 startRow >= m_GI->GetCurrentRow() && endRow <= m_GI->GetCurrentRow()))
			//{	
			//	if ( m_GI->m_highlightRowFlag == FALSE )
			//		m_GI->m_drawHintGrid.AddHint( m_GI->GetCurrentCol(), m_GI->GetCurrentRow());
			//	else
			//		m_GI->m_drawHintGrid.AddHint( 0, m_GI->GetCurrentRow(), m_GI->m_numberCols, m_GI->GetCurrentRow());
			//}
		}

		//double buffering
		//CDC * db_dc = NULL;
		//if(m_doubleBufferMode)
		//{
		//	db_dc = new CDC;
		//	db_dc->CreateCompatibleDC(NULL);
		//	db_dc->SelectObject(m_bitmap);
		//}

		//DrawCellsIntern(&dc,db_dc);
		DrawCellsIntern(this);
		//m_GI->m_drawHintGrid.ClearHints();

		//if(db_dc!= NULL)
		//	delete db_dc;
	}

	/***************************************************
	PaintDrawHintsNow
		Redraws all of the cells in that have been added
		the the CUGDrawHints (m_drawHint).
	Params:
		<none>
	Returns:
		<none>
	*****************************************************/
	//void PaintDrawHintsNow(LPCRECT invalidateRect){
	//
	//	InvalidateRect(invalidateRect,FALSE);
	//
	//	CPaintDC dc(this); // device context for painting
	//
	//	//double buffering
	//	CDC * db_dc = NULL;
	//	if(m_doubleBufferMode){
	//		db_dc = new CDC;
	//		db_dc->CreateCompatibleDC(NULL);
	//		db_dc->SelectObject(m_bitmap);
	//	}
	//	
	//	DrawCellsIntern(&dc,db_dc);
	//
	//	if(db_dc!= NULL)
	//		delete db_dc;
	//}

	/***************************************************
	DrawCellsIntern
		function is the key to the fast redraw functionality in the Ultimate Grid.
		This function is responsible for drawing cells within the grid area, it
		makes sure that only the cells that are marked as invalid
		(by calling CUGDrawHint::IsInvalid function) are redrawn.
	Params:
		dc		- screen DC
		db_dc	- off-screen DC, used when double buffering is enabled.
	Returns:
		<none>
	*****************************************************/
	//void DrawCellsIntern(CDC *dc,CDC *db_dc)
	void DrawCellsIntern(CXeD2DRenderContext* pRctx)
	{
		ID2D1RenderTarget* pRT = pRctx->m_pCurrentRT;
		RECT		rect = { 0,0,0,0 };
		RECT		cellRect;
		RECT		tempRect;

		CUGCell		cell, tempCell;
		//std::wstring	string;
		CSize		size;

		RECT		focusRect = { -1,-1,-1,-1 };
		CUGCellTypeIF* cellType;

		int			rightBlank = -1;
		int			bottomBlank = -1;

		int			col, x;
		long		row, y;

		int			selectBlock;

		//int			dcID;
		//CDC*		origDC = dc;

		//double buffering
		//if(m_doubleBufferMode)
		//{
		//	origDC = dc;
		//	dc =  db_dc;
		//	origDC->SelectObject(m_GI->m_xeUI->GetFont(EXE_FONT::eUI_Font));
		//	dc->SelectObject(m_GI->m_xeUI->GetFont(EXE_FONT::eUI_Font));
		//}
		//else{
		//	dc->SelectObject(m_GI->m_xeUI->GetFont(EXE_FONT::eUI_Font));
		//}

		//m_GI->OnScreenDCSetup(origDC,db_dc,UG_GRID);

		//set the right and bottom vars to point to
		//the extremes, if the right or bottom is
		//sooner then they will be updated in the
		//main drawing loop
		m_GI->SetRightCol(m_GI->NumberCols());
		m_GI->SetBottomRow(m_GI->NumberRows());

		//main draw loop, this loop goes through all visible
		//cells and checks to see if they need redrawing
		//if they do then the cell is retrieved and drawn
		for (y = 0; y < m_GI->NumberRows(); y++)
		{
			//skip rows hidden under locked rows
			if (y == m_GI->NumLockRows())
				y = m_GI->TopRow();

			row = y;

			//calc the top bottom and right side of the rect 
			//for the current cell to be drawn
			rect.top = rect.bottom;

			if (m_GI->UniformRowHeightFlag())
				rect.bottom += m_GI->DefRowHeight();
			else
			{
				rect.bottom += m_GI->GetNonUniformRowHeight(row);
				//rect.bottom += m_GI->m_rowHeights[row];
			}

			if (rect.top == rect.bottom)
				continue;

			rect.right = 0;

			//check all visible cells in the current row to 
			//see if they need drawing
			for (x = 0; x < m_GI->NumberCols(); x++) {

				//skip cols hidden under locked cols
				if (x == m_GI->NumLockCols())
					x = m_GI->LeftCol();

				row = y;
				col = x;

				//calc the left and right side of the rect
				rect.left = rect.right;
				rect.right += m_GI->GetColInfo(col).width;

				if (rect.left == rect.right)
					continue;

				//check to see if the cell need to be redrawn
				//if(m_GI->m_drawHintGrid.IsInvalid(col,row) != FALSE)
				{
					//copy the rect, then use the cellRect from here
					//this is done since the cellRect may be modified
					CopyRect(&cellRect, &rect);

					//get the cell to draw
					m_GI->GetCellIndirect(col, row, &cell);

					//check to see if the cell is joined
					if (cell.IsPropertySet(UGCELL_JOIN_SET)) {
						m_GI->GetCellRect(col, row, &cellRect);
						m_GI->GetJoinStartCell(&col, &row, &cell);
						//if(m_GI->m_drawHintGrid.IsValid(col,row))
						//	continue;
						//m_GI->m_drawHintGrid.SetAsValid(col,row);
					}

					//get the cell type to draw the cell
					if (cell.IsPropertySet(UGCELL_CELLTYPE_SET)) {
						cellType = m_GI->GetCellType(cell.GetCellType());
					}
					else {
						cellType = m_GI->GetCellType(-1);
					}


					//dcID = dc->SaveDC();

					//draw the cell, check to see if it is 'current' and/or selected
					CopyRect(&tempRect, &cellRect);
					if (row == m_GI->CurrentRow() && (col == m_GI->CurrentCol() || m_GI->HighlightRowFlag()))
						cellType->OnDraw(pRctx, EXE_FONT::eUI_Font, &cellRect, col, row, &cell, 0, 1);
					else {
						if (m_GI->IsSelected(col, row, &selectBlock))
							cellType->OnDraw(pRctx, EXE_FONT::eUI_Font, &cellRect, col, row, &cell, selectBlock + 1, 0);
						else
							cellType->OnDraw(pRctx, EXE_FONT::eUI_Font, &cellRect, col, row, &cell, 0, 0);
					}
					CopyRect(&cellRect, &tempRect);

					//draw a black line at the right edge of the locked columns
					//if(m_GI->m_numLockCols >0){
					//	if(col == m_GI->m_leftCol){
					//		dc->SelectObject(GetStockObject(BLACK_PEN));
					//		dc->MoveTo(cellRect.left-1,cellRect.top);
					//		dc->LineTo(cellRect.left-1,cellRect.bottom+1);
					//	}
					//	else if(col == m_GI->m_numLockCols-1){
					//		dc->SelectObject(GetStockObject(BLACK_PEN));
					//		dc->MoveTo(cellRect.right-1,cellRect.top);
					//		dc->LineTo(cellRect.right-1,cellRect.bottom+1);
					//	}
					//}
					//draw a black line at the bottom of the locked rows
					//if(m_GI->m_numLockRows >0){
					//	if(row == m_GI->m_topRow ){
					//		dc->SelectObject(GetStockObject(BLACK_PEN));
					//		dc->MoveTo(cellRect.left,cellRect.top-1);
					//		dc->LineTo(cellRect.right+1,cellRect.top-1);
					//	}
					//	else if(row == m_GI->m_numLockRows-1){
					//		dc->SelectObject(GetStockObject(BLACK_PEN));
					//		dc->MoveTo(cellRect.left,cellRect.bottom-1);
					//		dc->LineTo(cellRect.right+1,cellRect.bottom-1);
					//	}
					//}

					//dc->RestoreDC(dcID);
				}

				//check to see if the focus rect should be drawn
				//this function should be called all the time
				//even if it is off screen
				if (row == m_GI->CurrentRow() && (col == m_GI->CurrentCol() ||
					m_GI->HighlightRowFlag())) {
					CopyRect(&focusRect, &cellRect);
				}

				//check to see if the right side of the rect is past the edge
				//of the grid drawing area, if so then break
				if (rect.right > m_GI->GridWidth()) {
					m_GI->SetRightCol(col);
					break;
				}

			}

			//check to see if there is blank space on the right side of the grid
			//drawing area
			if (rect.right < m_GI->GridWidth() && m_GI->RightCol() == m_GI->NumberCols()) {
				rightBlank = rect.right;
			}

			//check to see if the bottom of the rect is past the bottom of the 
			//grid drawing area, if so then break
			if (rect.bottom > m_GI->GridHeight()) {
				m_GI->SetBottomRow(row);
				break;
			}

			//check for extra rows
			if (y >= (m_GI->NumberRows() - 1)) {
				long origNumRows = m_GI->NumberRows();
				long newRow = y + 1;
				m_GI->VerifyCurrentRow(&newRow);
				//if(m_GI->m_numberRows > origNumRows){
				//	m_GI->m_drawHintGrid.AddHint(0,y+1,m_GI->m_numberCols-1,y+1);
				//}
			}
		}

		//clear the draw hints
		//m_GI->m_drawHintGrid.ClearHints();

		//check to see if there is blank space on the bottom of the grid
		//drawing area
		if (rect.bottom < m_GI->GridHeight() && m_GI->BottomRow() == m_GI->NumberRows())
			bottomBlank = rect.bottom;

		//fill in the blank grid drawing areas
		if (rightBlank >= 0 || bottomBlank >= 0) {
			//CBrush brush(m_GI->m_xeUI->GetColor(CID::GrdPaperBg));
			//CBrush brush(m_ctrl->OnGetDefBackColor(UG_GRID));
			GetClientRect(&rect);
			if (rightBlank >= 0) {
				rect.left = rightBlank;
				//dc->FillRect(&rect,&brush);
				pRT->FillRectangle(RectFfromRect(rect), GetBrush(CID::GrdPaperBg));
			}
			if (bottomBlank >= 0) {
				rect.left = 0;
				rect.top = bottomBlank;
				//dc->FillRect(&rect,&brush);
				pRT->FillRectangle(RectFfromRect(rect), GetBrush(CID::GrdPaperBg));
			}
		}

		//double buffering
		//if(m_doubleBufferMode){
		//	dc = origDC;
		//	dc->BitBlt(0,0,m_GI->m_gridWidth,m_GI->m_gridHeight,db_dc,0,0,SRCCOPY);
		//}

		//draw the focus rect, if the flag was set above
		if (!m_tempDisableFocusRect) {
			if ((m_hasFocus /*|| m_GI->m_findDialogRunning*/) && !m_GI->EditInProgress())
			{
				if (m_GI->HighlightRowFlag())
				{
					focusRect.left = 0;

					if (rect.right < m_GI->GridWidth())
						focusRect.right = rect.right;
					else
					{
						if (m_GI->Extend())
							focusRect.right = m_GI->GridWidth();
						else
						{
							int iStartCol = m_GI->GetLeftCol(), iEndCol = m_GI->GetNumberCols() - 1;
							int iWidth = 0;
							for (int iLoop = iStartCol; iLoop <= iEndCol; iLoop++)
								iWidth += m_GI->GetColWidth(iLoop);

							focusRect.right = iWidth;
						}
					}
				}
				_DrawFocusRect(pRT, RectFfromRect(focusRect));
			}
		}

		//reset temporary flags
		m_tempDisableFocusRect = FALSE;
	}

public:
	/***************************************************
	RedrawCell
		function is called to redraw a single cell, it will add provided cell
		to draw hints (CUGDrawHint) and forces the window to update.
	Params:
		col, row	- coordinates of cell to redraw
	Returns:
		<none>
	*****************************************************/
	void RedrawCell(int col, long row)
	{
		//m_GI->m_drawHintGrid.AddHint(col,row);
		Moved();
	}

	/***************************************************
	RedrawRow
		function is called to redraw a single cell, it will add provided cells
		to draw hints (CUGDrawHint) and forces the window to update.
	Params:
		row			- indicates the row number to update.
	Returns:
		<none>
	*****************************************************/
	void RedrawRow(long row)
	{
		//m_GI->m_drawHintGrid.AddHint(0,row,m_GI->m_numberCols-1,row);
		Moved();
	}

	/***************************************************
	RedrawCol
		function is called to redraw a single cell, it will add provided cells
		to draw hints (CUGDrawHint) and forces the window to update.
	Params:
		col			- indicates the column number to update.
	Returns:
		<none>
	*****************************************************/
	void RedrawCol(int col)
	{
		//m_GI->m_drawHintGrid.AddHint(col,0,col,m_GI->m_numberRows-1);
		Moved();
	}

	/***************************************************
	RedrawRange
		function is called to redraw a single cell, it will add provided cells
		to draw hints (CUGDrawHint) and forces the window to update.
	Params:
		startCol,	- parameters passed in indicate the range of cells to be redrawn
		startRow,	  the range's top-left corned is indicated by the StartCol and
		endCol,		  StartRow, and the bottom-right corner by the endCol and endRow.
		endRow
	Returns:
		<none>
	*****************************************************/
	void RedrawRange(int startCol, long startRow, int endCol, long endRow)
	{
		//m_GI->m_drawHintGrid.AddHint(startCol,startRow,endCol,endRow);
		Moved();
	}

	/***************************************************
	TempDisableFocusRect
	Params:
		<none>
	Returns:
		<none>
	*****************************************************/
	void TempDisableFocusRect() {
		m_tempDisableFocusRect = TRUE;
	}


	/***************************************************
	Update
		function forces window update.
	Params:
		<none>
	Returns:
		<none>
	*****************************************************/
	void Update()
	{
		_RedrawDirectly();
		return;
	}

	/***************************************************
	Moved
		This routine recalcs the internal variables
		plus causes the grid to re-draw itself
	Params:
		<none>
	Returns:
		<none>
	*****************************************************/
	void Moved()
	{
		_RedrawDirectly();

		//#pragma BUG("This is a temporary fix for Grid drawing bug when overlapped by a window")
		//		Invalidate();
		//		UpdateWindow();
		//		return;

			//int xScroll = 0;
			//int yScroll = 0;
			//int x;
			//long y;
			////CDC * pDC, *screenDC,*db_DC = NULL;

			//if(GetUpdateRect(NULL)){
			//	UpdateWindow();
			//	return;
			//}
			//if(m_GI->m_findDialogRunning){
			//	Invalidate();
			//	UpdateWindow();
			//	return;
			//}
			//if(m_GI->m_enableVScrollHints || m_GI->m_enableHScrollHints){
			//	Invalidate();
			//	UpdateWindow();
			//	return;
			//}

			///*	If the grid window is overlapped by other windows, 
			//	we should repaint the whole grid.*/
			//HWND hPrevWnd, hNextWnd, hActiveWnd, hDtWnd;
			//RECT rectGrid, rectOther, rectDest, rectSub, rectVisible;

			//// Check if the grid window is overlapped by other windows
			//GetWindowRect(&rectGrid);
			//::CopyRect(&rectVisible, &rectGrid);
			//for (hPrevWnd = Hwnd(); (hNextWnd = ::GetWindow(hPrevWnd, GW_HWNDPREV)) != NULL; hPrevWnd = hNextWnd){
			//    ::GetWindowRect(hNextWnd, &rectOther);
			//	if (::IsWindowVisible(hNextWnd) && IntersectRect(&rectDest, &rectGrid, &rectOther)){
			//		::SubtractRect(&rectSub, &rectVisible, &rectDest);
			//		::CopyRect(&rectVisible, &rectSub);
			//	}
			//}
			//if ( !EqualRect(&rectVisible, &rectGrid) ) {
			//	Invalidate();
			//	UpdateWindow();
			//	return;
			//}

			//// Check if the grid is cliped by the active window (e.g. MDI frame window)
			//hActiveWnd = ::GetActiveWindow();
			//if ( (NULL != hActiveWnd) && (Hwnd() != hActiveWnd) ) {
			//	::GetWindowRect(hActiveWnd, &rectOther);
			//	if (::IsWindowVisible(hActiveWnd) && IntersectRect(&rectDest, &rectGrid, &rectOther)){
			//		if ( !EqualRect(&rectDest, &rectGrid) ) {
			//			Invalidate();
			//			UpdateWindow();
			//			return;
			//		}
			//	}
			//}

			//// Check if the grid is cliped by the desktop window
			//hDtWnd = ::GetDesktopWindow();
			//if ( (NULL != hDtWnd) && (Hwnd() != hDtWnd) ) {
			//	::GetWindowRect(hDtWnd, &rectOther);
			//	if (::IsWindowVisible(hDtWnd) && IntersectRect(&rectDest, &rectGrid, &rectOther)){
			//		if ( !EqualRect(&rectDest, &rectGrid) ) {
			//			Invalidate();
			//			UpdateWindow();
			//			return;
			//		}
			//	}
			//}

			//BOOL redrawAll = FALSE;

			////if the left col has changed then calc the scroll distance
			//if(m_GI->m_leftCol != m_GI->m_lastLeftCol)
			//{
			//	if(m_GI->m_leftCol > m_GI->m_lastLeftCol)
			//	{
			//		for(x = m_GI->m_lastLeftCol;x < m_GI->m_leftCol; x++)
			//		{
			//			xScroll += m_GI->GetColWidth(x);
			//			if(xScroll > m_GI->m_gridWidth)
			//			{
			//				redrawAll = TRUE;
			//				break;
			//			}
			//		}
			//	}
			//	else
			//	{
			//		for(x = m_GI->m_leftCol;x < m_GI->m_lastLeftCol; x++)
			//		{
			//			xScroll -= m_GI->GetColWidth(x);
			//			if(xScroll < -m_GI->m_gridWidth)
			//			{
			//				redrawAll = TRUE;
			//				break;
			//			}
			//		}
			//	}
			//	if(redrawAll)
			//	{
			//		xScroll = 0;
			//		//redraw the whole grid
			//		m_GI->m_drawHintGrid.AddHint(0,0,m_GI->m_numberCols,m_GI->m_numberRows);
			//	}
			//}

			////if the top row has changed then calc the scroll distance
			//if(m_GI->m_topRow != m_GI->m_lastTopRow)
			//{
			//	if(m_GI->m_topRow > m_GI->m_lastTopRow)
			//	{
			//		for(y = m_GI->m_lastTopRow;y < m_GI->m_topRow; y++)
			//		{
			//			yScroll += m_GI->GetRowHeight(y);
			//			if(yScroll > m_GI->m_gridHeight)
			//			{
			//				redrawAll = TRUE;
			//				break;
			//			}
			//		}
			//	}
			//	else
			//	{
			//		for(y = m_GI->m_topRow;y < m_GI->m_lastTopRow; y++)
			//		{
			//			yScroll -= m_GI->GetRowHeight(y);
			//			if(yScroll < -m_GI->m_gridHeight)
			//			{
			//				redrawAll = TRUE;
			//				break;
			//			}
			//		}
			//	}
			//	if(redrawAll)
			//	{
			//		yScroll = 0;
			//		//redraw the whole grid
			//		m_GI->m_drawHintGrid.AddHint(0,0,m_GI->m_numberCols,m_GI->m_numberRows);
			//	}
			//}

			////create the dc
			////screenDC = GetDC();
			////if(m_doubleBufferMode)
			////{
			////	pDC = new CDC;
			////	pDC->CreateCompatibleDC(NULL);
			////	pDC->SelectObject(m_bitmap);
			////	db_DC = pDC;
			////}
			////else
			////{
			////	pDC = screenDC;
			////}

			////do scrolling of the window here
			//if(xScroll != 0 || yScroll != 0)
			//{
			//	RECT scrollRect;
			//	GetClientRect(&scrollRect);
			//	if(xScroll == 0)
			//		scrollRect.top += m_GI->m_lockRowHeight;
			//	if(yScroll == 0)
			//		scrollRect.left += m_GI->m_lockColWidth;
			//	pDC->ScrollDC(-xScroll,-yScroll,&scrollRect,&scrollRect,NULL,NULL);
			//	
			//	//add the draw hints for the area the was uncovered by the scroll
			//	if(xScroll >0)
			//		m_GI->m_drawHintGrid.AddHint(m_GI->m_rightCol,0,m_GI->m_numberCols,m_GI->m_numberRows);
			//	else if(xScroll < 0)
			//		m_GI->m_drawHintGrid.AddHint(0,0,m_GI->m_lastLeftCol,m_GI->m_numberRows);
			//	if(yScroll >0)
			//		m_GI->m_drawHintGrid.AddHint(0,m_GI->m_bottomRow,m_GI->m_numberCols,m_GI->m_numberRows);
			//	else if(yScroll < 0)
			//		m_GI->m_drawHintGrid.AddHint(0,0,m_GI->m_numberCols,m_GI->m_lastTopRow);
			//}

			////add the last and current cells - add row support later
			//if(m_GI->m_highlightRowFlag)
			//{
			//	m_GI->m_drawHintGrid.AddHint(0,m_GI->m_lastRow,m_GI->m_numberCols,m_GI->m_lastRow);
			//	m_GI->m_drawHintGrid.AddHint(0,m_GI->m_currentRow,m_GI->m_numberCols,m_GI->m_currentRow);
			//}
			//else
			//{
			//	m_GI->m_drawHintGrid.AddHint(m_GI->m_lastCol,m_GI->m_lastRow);
			//	m_GI->m_drawHintGrid.AddHint(m_GI->m_currentCol,m_GI->m_currentRow);
			//}

			////call the grids main drawing routine
			//DrawCellsIntern(screenDC,db_DC);

			////double buffering
			////if(m_doubleBufferMode)
			////	delete db_DC;
			//
			////close the device context
			////ReleaseDC(screenDC);
	}

	void HideTooltip() { _HideTooltip(); }

protected:
	/***************************************************
	OnKeyDown
		Passes all keystrokes to a callback routine in CUGCtrl (OnKeyDown)
		then processes them.  The callback has ability to diallow further processing
		of the event.
	Params:
		nChar		- please see MSDN for more information on the parameters.
		nRepCnt
		nFlags
	Returns:
		<none>
	*****************************************************/
	//void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	virtual LRESULT _OnKeyDown(WPARAM wParam, LPARAM lParam) override
	{
		//UNREFERENCED_PARAMETER(nRepCnt);
		//UNREFERENCED_PARAMETER(nFlags);
		UINT nChar = (UINT)wParam;

		BOOL fCtrlKeyDown = (::GetKeyState(VK_CONTROL) & 0x8000) ? TRUE : FALSE;
		BOOL fShiftKeyDown = (::GetKeyState(VK_SHIFT) & 0x8000) ? TRUE : FALSE;
		BOOL fMenuKeyDown = (::GetKeyState(VK_MENU) & 0x8000) ? TRUE : FALSE;

		m_GI->SetMoveType(0);	//key(default)

		int increment = 1; //default number of units to move

		if (m_GI->BallisticKeyMode() > 0)
		{
			m_keyRepeatCount++;

			int value = (m_keyRepeatCount / m_GI->BallisticKeyMode());
			increment = value * value * value + 1;

			if (m_GI->BallisticKeyDelay() > 0)
			{
				if (value == 0 && m_keyRepeatCount > 1)
					Sleep(m_GI->BallisticKeyDelay());
			}
		}

		//send a notify message to the cell type class
		BOOL processed = m_GI->GetCellTypeColRow(m_GI->CurrentCol(), m_GI->CurrentRow())->
			OnKeyDown(m_GI->CurrentCol(), m_GI->CurrentRow(), &nChar);

		//send a keydown notify message
		m_GI->OnKeyDown(&nChar, processed);

		int  curCol = m_GI->CurrentCol();
		long curRow = m_GI->CurrentRow();
		int  JoinCellStartCol, JoinCellEndCol, colOff, rowOff;
		long JoinCellStartRow, JoinCellEndRow;

		//process the keystroke
		if (nChar == VK_LEFT)
		{
			JoinCellStartCol = curCol - increment;
			JoinCellStartRow = curRow;
			if ((JoinCellStartCol >= 0) && (m_GI->GetJoinStartCellColRow(&JoinCellStartCol, &JoinCellStartRow) == UG_SUCCESS))
			{
				m_GI->GotoCell(m_GI->DragCol() - (curCol - JoinCellStartCol),
					m_GI->DragRow() - (curRow - JoinCellStartRow));
			}
			else
				m_GI->GotoCol(m_GI->DragCol() - increment);
		}
		else if (nChar == VK_RIGHT)
		{
			JoinCellStartCol = curCol;
			JoinCellStartRow = curRow;
			colOff = increment;
			if (m_GI->GetJoinRange(&JoinCellStartCol, &JoinCellStartRow, &JoinCellEndCol, &JoinCellEndRow) == UG_SUCCESS)
				colOff = (JoinCellEndCol - JoinCellStartCol) + increment;

			JoinCellStartRow = curRow;
			JoinCellStartCol = curCol + colOff;
			if (m_GI->GetJoinStartCellColRow(&JoinCellStartCol, &JoinCellStartRow) == UG_SUCCESS)
			{
				m_GI->GotoCell(m_GI->DragCol() - (curCol - JoinCellStartCol),
					m_GI->DragRow() - (curRow - JoinCellStartRow));
			}
			else
				m_GI->GotoCol(m_GI->DragCol() + colOff);
		}
		else if (nChar == VK_UP)
		{
			if (fCtrlKeyDown && !(fShiftKeyDown || fMenuKeyDown))
			{
				if (m_GI->CurrentRow() < m_GI->BottomRow())
				{
					m_GI->SetTopRow(m_GI->TopRow() - 1);	// Ctrl + UP - Move current row down
				}
			}
			else
			{
				JoinCellStartCol = curCol;
				JoinCellStartRow = curRow - increment;
				if ((JoinCellStartRow >= 0)
					&& (m_GI->GetJoinStartCellColRow(&JoinCellStartCol, &JoinCellStartRow)
						== UG_SUCCESS))
				{
					m_GI->GotoCell(m_GI->DragCol() - (curCol - JoinCellStartCol),
						m_GI->DragRow() - (curRow - JoinCellStartRow));
				}
				else
					m_GI->GotoRow(m_GI->DragRow() - increment);
			}
		}
		else if (nChar == VK_DOWN)
		{
			if (fCtrlKeyDown && !(fShiftKeyDown || fMenuKeyDown))
			{
				if (m_GI->CurrentRow() > m_GI->TopRow())
				{
					m_GI->SetTopRow(m_GI->TopRow() + 1);	// Ctrl + DOWN - Move current row up
				}
			}
			else
			{
				JoinCellStartCol = curCol;
				JoinCellStartRow = curRow;
				rowOff = increment;
				if (m_GI->GetJoinRange(&JoinCellStartCol, &JoinCellStartRow, &JoinCellEndCol, &JoinCellEndRow) == UG_SUCCESS)
					rowOff = (JoinCellEndRow - JoinCellStartRow) + increment;

				JoinCellStartCol = curCol;
				JoinCellStartRow = curRow + rowOff;
				if (m_GI->GetJoinStartCellColRow(&JoinCellStartCol, &JoinCellStartRow) == UG_SUCCESS)
				{
					m_GI->GotoCell(m_GI->DragCol() - (curCol - JoinCellStartCol),
						m_GI->DragRow() - (curRow - JoinCellStartRow));
				}
				else
					m_GI->GotoRow(m_GI->DragRow() + rowOff);
			}
		}
		else if (nChar == VK_PRIOR)
			m_GI->MoveCurrentRow(UG_PAGEUP);
		else if (nChar == VK_NEXT)
			m_GI->MoveCurrentRow(UG_PAGEDOWN);
		else if (nChar == VK_HOME) {
			if (GetKeyState(VK_CONTROL) < 0)
				m_GI->MoveCurrentRow(UG_TOP);
			else
				m_GI->MoveCurrentCol(UG_LEFT);
		}
		else if (nChar == VK_END)
		{
			if (GetKeyState(VK_CONTROL) < 0)
				m_GI->MoveCurrentRow(UG_BOTTOM);
			else
				m_GI->MoveCurrentCol(UG_RIGHT);
		}
		return 0;
	}

	/***************************************************
	OnLButtonDown
		Finds the cell that was clicked in, sends a notification
		updates the current cells position.
		It also sets mouse capture.
	Params:
		nFlags		- please see MSDN for more information on the parameters.
		point
	Returns:
		<none>
	*****************************************************/
	//void OnLButtonDown(UINT nFlags, CPoint point)
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint point) override
	{
		if (::GetFocus() != Hwnd())
			::SetFocus(Hwnd());

		if (m_GI->EditInProgress())
			return 0;

		int col = -1;
		long row = -1;
		RECT rect;
		BOOL processed = FALSE;

		SetCapture();

		//setup the move type flags
		m_GI->SetMoveType(1);	//lbutton
		m_GI->SetMoveFlags(nFlags);

		//check to see what cell was clicked in, and move there
		if (m_GI->GetCellFromPoint(point.x, point.y, &col, &row, &rect) == UG_SUCCESS) {

			//#pragma NOTE("SKmod - changed because of drag and drop")
					// MOVED to Lbtn. up.
					//m_ctrl->GotoCell(col,row);

					//send a notification to the cell type	
			processed = m_GI->GetCellTypeColRow(col, row)->OnLClicked(col, row, 1, &rect, &point);

			if (processed)
				m_cellTypeCapture = TRUE;
		}

		m_GI->OnLClicked(col, row, 1, &rect, &point, processed);

		m_GI->SetMoveType(0);	//key(default)

		// indicate that this mouse button down event was handled by the grid,
		// allowing for the mouse button up to be processed.
		m_bHandledMouseDown = TRUE;
		return 0;
	}

	/***************************************************
	OnLButtonUp
		Finds the cell that was clicked in
		Sends a notification to CUCtrl::OnLClicked
		Releases mouse capture
	Params:
		nFlags		- please see MSDN for more information on the parameters.
		point
	Returns:
		<none>
	*****************************************************/
	//void OnLButtonUp(UINT nFlags, CPoint point)
	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint point) override
	{
		UNREFERENCED_PARAMETER(nFlags);

		int col = -1;
		long row = -1;
		RECT rect;
		BOOL processed = FALSE;

		if (m_bHandledMouseDown != TRUE)
			return 0;

		if (m_GI->GetCellFromPoint(point.x, point.y, &col, &row, &rect) == UG_SUCCESS) {

			//#pragma NOTE("SKmod - changed because of drag and drop")
					// MOVED from Lbtn. dn.
					//setup the move type flags
			m_GI->SetMoveType(1);	//lbutton
			m_GI->SetMoveFlags(nFlags);
			m_GI->GotoCell(col, row);
			m_GI->SetMoveType(0);	//key(default)

			//send a notification to the cell type	
			processed = m_GI->GetCellTypeColRow(col, row)->OnLClicked(col, row, 0, &rect, &point);
		}

		m_GI->OnLClicked(col, row, 0, &rect, &point, processed);

		ReleaseCapture();

		m_GI->SetDragCol(m_GI->CurrentCol());
		m_GI->SetDragRow(m_GI->CurrentRow());

		m_cellTypeCapture = FALSE;
		m_bHandledMouseDown = FALSE;
		return 0;
	}

	/***************************************************
	OnLButtonDblClk
		Finds the cell that was clicked in
		Sends a notification to CUCtrl::OnDClicked
	Params:
		<none>
	Returns:
		<none>
	*****************************************************/
	//void OnLButtonDblClk(UINT nFlags, CPoint point)
	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint point) override
	{
		UNREFERENCED_PARAMETER(nFlags);

		int col = -1;
		long row = -1;
		RECT rect;
		BOOL processed = FALSE;

		if (m_GI->GetCellFromPoint(point.x, point.y, &col, &row, &rect) == UG_SUCCESS) {

			//send a notification to the cell type	
			processed = m_GI->GetCellTypeColRow(col, row)->OnDClicked(col, row, &rect, &point);
		}

		m_GI->OnDClicked(col, row, &rect, &point, processed);
		return 0;
	}

	/***************************************************
	OnMouseMove
		Finds the cell that the mouse is over and if the left mouse button is down
		then the current cell postion is updated
		Sends optional notification messages
		Also scrolls the current cell if the mouse is outside the window area.
	Params:
		nFlags		- please see MSDN for more information on the parameters.
		point
	Returns:
		<none>
	*****************************************************/
	//void OnMouseMove(UINT nFlags, CPoint point)
	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{
		CXeD2DWndBase::_OnMouseMove(nFlags, point);

		int col;
		long row;
		BOOL vertViewMove = FALSE,
			horsViewMove = FALSE;

		if (m_GI->EditInProgress())
			return 0;

		if (m_cellTypeCapture)
		{
			col = m_GI->CurrentCol();
			row = m_GI->CurrentRow();
			//send a notification to the cell type
			BOOL processed = m_GI->GetCellTypeColRow(col, row)->OnMouseMove(col, row, &point, nFlags);
			//send a notification to the main grid class
			m_GI->OnMouseMove(col, row, &point, nFlags, processed);

			return 0;
		}


		m_GI->SetMoveType(3);	//mouse move
		m_GI->SetMoveFlags(nFlags);

		//check to see if the mouse is over a cell
		if (m_GI->GetCellFromPointColRow(point.x, point.y, &col, &row) == UG_SUCCESS)
		{
			//send a notification to the cell type
			BOOL processed = m_GI->GetCellTypeColRow(col, row)->OnMouseMove(col, row, &point, nFlags);
			//send a notification to the main grid class
			m_GI->OnMouseMove(col, row, &point, nFlags, processed);
		}

		//#pragma NOTE("This functionality disabled because of drag and drop")
		if (FALSE/*nFlags & MK_LBUTTON*/)
		{
			MSG msg;
			int moved = FALSE;

			while (1)
			{
				if (m_GI->GetCellFromPointColRow(point.x, point.y, &col, &row) == UG_SUCCESS)
				{
					if (row == m_GI->BottomRow())
						vertViewMove = TRUE;
					if (col == m_GI->RightCol())
						horsViewMove = TRUE;

					// The 'm_bScrollOnParialCells' flag controls the scrolling behavior
					// when the mouse is dragged over partially visible cell.  By default
					// we will treat this area same as area 'outside of the grid' meaning
					// that the grid will scroll using current ballistics settigs.
					// You can set this flag to FALSE if you do not want the grid to
					// scroll when user's mouse is over the partially visible cells,
					// the grid will only do that when mouse is moved outside of the view.
					if (m_GI->ScrollOnPartialCells() == FALSE && (vertViewMove == TRUE || horsViewMove == TRUE))
						return 0;

					if (m_bHandledMouseDown != TRUE)
						return 0;

					//send a notification to the cell type
					if (m_GI->NumLockCols() == 0 && m_GI->NumLockRows() == 0)
						m_GI->GotoCell(col, row);
					else
					{
						if (m_GI->DragCol() > m_GI->NumLockCols() && col < m_GI->NumLockCols())
							m_GI->MoveCurrentCol(UG_LINEUP);
						else
							m_GI->GotoCol(col);

						if (m_GI->DragRow() > m_GI->NumLockRows() && row < m_GI->NumLockRows())
							m_GI->MoveCurrentRow(UG_LINEUP);
						else
							m_GI->GotoRow(row);
					}
				}
				//if the mouse is off the left side
				if (point.x < 0)
				{
					//if ballistic mode
					if (m_GI->BallisticMode())
					{
						int increment = (int)pow((double)((point.x * -1) / m_GI->DefColWidth() + 1),
							m_GI->BallisticMode());
						m_GI->GotoCol(m_GI->DragCol() - increment);
						if (increment == 1)
							Sleep(m_GI->BallisticDelay());
					}
					else
						m_GI->MoveCurrentCol(UG_LINEUP);
					moved = TRUE;
				}
				//if the mouse is off the right side
				else if (point.x > m_GI->GridWidth() || horsViewMove)
				{
					//if ballistic mode
					if (m_GI->BallisticMode())
					{
						int increment = (int)pow((double)((point.x - m_GI->GridWidth()) /
							m_GI->DefColWidth() + 1), m_GI->BallisticMode());
						m_GI->GotoCol(m_GI->DragCol() + increment);
						if (increment == 1)
							Sleep(m_GI->BallisticDelay());
					}
					else
						m_GI->MoveCurrentCol(UG_LINEDOWN);
					moved = TRUE;
				}
				//if the mouse is above the top
				if (point.y < 0)
				{
					//if ballistic mode
					if (m_GI->BallisticMode())
					{
						long increment = (long)pow((double)((point.y * -1) / m_GI->DefRowHeight() + 1),
							m_GI->BallisticMode());
						m_GI->GotoRow(m_GI->DragRow() - increment);
						if (increment == 1)
							Sleep(m_GI->BallisticDelay());
					}
					else
						m_GI->MoveCurrentRow(UG_LINEUP);
					moved = TRUE;
				}
				//if the mouse is below the bottom
				else if (point.y > m_GI->GridHeight() || vertViewMove)
				{
					//if ballistic mode
					if (m_GI->BallisticMode())
					{
						long increment = (long)pow((double)((point.y - m_GI->GridHeight()) /
							m_GI->DefRowHeight() + 1), m_GI->BallisticMode());
						m_GI->GotoRow(m_GI->DragRow() + increment);
						if (increment == 1)
							Sleep(m_GI->BallisticDelay());
					}
					else
						m_GI->MoveCurrentRow(UG_LINEDOWN);
					moved = TRUE;
				}


				moved = FALSE;

				//check for messages, if there are not then scroll some more
				while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
				{
					if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONUP)
					{
						m_GI->SetMoveType(0);	//key(default)

						return 0;
					}
					GetMessage(&msg, NULL, 0, 0);
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

		m_GI->SetMoveType(0);	//key - default
		return 0;
	}

	/***************************************************
	OnRButtonDown
		Finds the cell that the mouse is in
		Sends series of notifications to the CUGCrtl
		Pops up a menu if enabled and populated.
	Params:
		nFlags		- please see MSDN for more information on the parameters.
		point
	Returns:
		<none>
	*****************************************************/
	//void OnRButtonDown(UINT nFlags, CPoint point)
	virtual LRESULT _OnRightDown(UINT nFlags, CPoint point) override
	{
		if (m_GI->EditInProgress())
			return 0;

		int col = -1;
		long row = -1;
		BOOL processed = FALSE;
		RECT rect;

		m_GI->SetMoveType(2);	//2button
		m_GI->SetMoveFlags(nFlags);

		if (m_GI->GetCellFromPoint(point.x, point.y, &col, &row, &rect) == UG_SUCCESS)
		{
			m_GI->GotoCell(col, row);
			//send a notification to the cell type	
			processed = m_GI->GetCellTypeColRow(col, row)->OnRClicked(col, row, 1, &rect, &point);
		}

		m_GI->OnRClicked(col, row, 1, &rect, &point, processed);


		m_GI->SetMoveType(0);	//key(default)

		if (m_GI->IsEnablePopupMenu())
		{
			ClientToScreen(&point);
			m_GI->StartMenu(col, row, point, UG_GRID);
		}

		m_GI->SetMoveType(0);//key - default
		m_bHandledMouseDown = TRUE;
		return 0;
	}

	/***************************************************
	OnRButtonUp
		Finds the cell that the mouse is in
		Sends a OnRClicked notification to the CUGCtrl.
	Params:
		nFlags		- please see MSDN for more information on the parameters.
		point
	Returns:
		<none>
	*****************************************************/
	//void OnRButtonUp(UINT nFlags, CPoint point)
	virtual LRESULT _OnRightUp(UINT nFlags, CPoint point) override
	{
		UNREFERENCED_PARAMETER(nFlags);

		int col = -1;
		long row = -1;
		RECT rect;
		BOOL processed = FALSE;

		if (m_bHandledMouseDown != TRUE)
			return 0;

		if (m_GI->GetCellFromPoint(point.x, point.y, &col, &row, &rect) == UG_SUCCESS)
		{
			//send a notification to the cell type	
			processed = m_GI->GetCellTypeColRow(col, row)->OnRClicked(col, row, 0, &rect, &point);
		}

		m_GI->OnRClicked(col, row, 0, &rect, &point, processed);

		m_GI->SetDragCol(m_GI->CurrentCol());
		m_GI->SetDragRow(m_GI->CurrentRow());
		m_bHandledMouseDown = FALSE;
		return 0;
	}

	/***************************************************
	OnChar
		passed the handling of this event to the OnCharDown in both the cell's
		celltype and the CUGCtrl.
	Params:
		nChar		- please see MSDN for more information on the parameters.
		nRepCnt
		nFlags
	Returns:
		<none>
	*****************************************************/
	//void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
	virtual LRESULT _OnChar(WPARAM wParam, LPARAM lParam) override
	{
		UINT nChar = (UINT)wParam;
		//UNREFERENCED_PARAMETER(nRepCnt);
		//UNREFERENCED_PARAMETER(nFlags);
		BOOL processed = FALSE;

		//send a notification to the cell type	
		processed = m_GI->GetCellTypeColRow(m_GI->CurrentCol(), m_GI->CurrentRow())->
			OnCharDown(m_GI->CurrentCol(), m_GI->CurrentRow(), &nChar);

		m_GI->OnCharDown(&nChar, processed);
		return 0;
	}

	/***************************************************
	PreCreateWindow
		The Utlimate Grid overwrites this function to further customize the
		window that is created.
	Params:
		cs			- please see MSDN for more information on the parameters.
	Returns:
		Nonzero if the window creation should continue; 0 to indicate creation failure.
	*****************************************************/
	//BOOL PreCreateWindow(CREATESTRUCT& cs)
	//{
	//	cs.style = cs.style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	//	return CWnd::PreCreateWindow(cs);
	//}

	/***************************************************
	OnSetFocus
		The grid uses this event handler to setup internal variables and passes
		further processig to both the CWnd and CUGCtrl.
	Params:
		pOldWnd		- pointer to window that lost the focus
	Returns:
		<none>
	*****************************************************/
	//void OnSetFocus(CWnd* pOldWnd)
	virtual LRESULT _OnSetFocus(HWND hOldWnd) override
	{
		m_hasFocus = TRUE;
		m_cellTypeCapture = FALSE;
		m_keyRepeatCount = 0;

		// pass the notification handling
		//CWnd::OnSetFocus( pOldWnd );
		m_GI->OnSetFocus(UG_GRID);
		// redraw the current cell
		CRect cellRect;

		if (m_GI->HighlightRowFlag() == TRUE)
		{
			m_GI->GetRangeRect(0, m_GI->CurrentRow(), m_GI->NumberCols() - 1, m_GI->CurrentRow(), cellRect);
			//m_GI->m_drawHintGrid.AddHint( 0, m_GI->m_currentRow, m_GI->m_numberCols - 1, m_GI->m_currentRow );
		}
		else
		{
			m_GI->GetCellRect(m_GI->CurrentCol(), m_GI->CurrentRow(), cellRect);
			//m_GI->m_drawHintGrid.AddHint( m_GI->m_currentCol, m_GI->m_currentRow );
		}

		//PaintDrawHintsNow( cellRect );
		_RedrawDirectly();
		return 0;
	}

	/***************************************************
	OnKillFocus
		clears internal flag indicating that the grid no longer has focus
		and passes further processign to both the CWnd and CUGCtrl.
	Params:
		pNewWnd		- pointer to window that is gaining the focus.
	Returns:
		<none>
	*****************************************************/
	//void OnKillFocus(CWnd* pNewWnd)
	virtual LRESULT _OnKillFocus(HWND hNewWnd) override
	{
		m_hasFocus = FALSE;

		//CWnd::OnKillFocus(pNewWnd);
		// notify the grid that the grid lost focus
		m_GI->OnKillFocusNewWnd(UG_GRID, hNewWnd);

		RedrawCell(m_GI->CurrentCol(), m_GI->CurrentRow());
		return 0;
	}

	/***************************************************
	OnMouseActivate
		The framework calls this member function when the cursor is in an inactive
		window and the user presses a mouse button.
	Params:
		pDesktopWnd	- please see MSDN for more information on the parameters.
		nHitTest
		message
	Returns:
		Specifies to activate the CWnd and to discard the mouse event.
	*****************************************************/
	//int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message) 
	virtual LRESULT _OnMouseActivate(WPARAM wParam, LPARAM lParam) override
	{
		//CWnd::OnMouseActivate( pDesktopWnd, nHitTest, message );
		return MA_ACTIVATE;
	}

	/***************************************************
	SetDoubleBufferMode
		turns on and off the double buffer functionality.  When the double buffer
		mode is on then the grid will first is drawn to an off-screen DC before
		the results are copied over to the screen.
	Params:
		mode		- double buffer mode (TRUE or FALSE)
	Returns:
		UG_SUCCESS	- on success
		UG_ERROR	- if creation of off-screen buffer was unsuccessful
	*****************************************************/
	//int SetDoubleBufferMode(int mode)
	//{
	//	if(m_bitmap != NULL)
	//	{
	//		delete m_bitmap;
	//		m_bitmap = NULL;
	//	}
	//
	//	m_doubleBufferMode = mode;
	//
	//	if(mode == TRUE)
	//	{
	//		m_bitmap = new CBitmap;
	//
	//		CDC * pDC = GetDC();
	//		RECT rect;
	//	
	//		GetClientRect(&rect);
	//		if(rect.right ==0)
	//			rect.right =1;
	//		if(rect.bottom ==0)
	//			rect.bottom =1;
	//
	//		if(m_bitmap->CreateCompatibleBitmap(pDC,rect.right,rect.bottom) == 0)
	//		{
	//			delete m_bitmap;
	//			m_bitmap = NULL;
	//			m_doubleBufferMode = FALSE;
	//			return UG_ERROR;
	//		}
	//
	//		//clear the bitmap
	//		CBitmap * oldbitmap = pDC->SelectObject(m_bitmap);
	//		if(oldbitmap)
	//			pDC->SelectObject(oldbitmap);
	//
	//		ReleaseDC(pDC);
	//	}
	//	
	//	return UG_SUCCESS;
	//}

	/***************************************************
	OnMouseWheel
		The framework calls this member function as a user rotates the mouse
		wheel and encounters the wheel's next notch.
	Params:
		nFlags		- please see MSDN for more information on the parameters.
		zDelta
		pt
	Returns:
		Nonzero if mouse wheel scrolling is enabled; otherwise 0.
	*****************************************************/
	#ifdef WM_MOUSEWHEEL
	//BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint pt) override
	{
		if (!m_GI->EditInProgress())
		{
			int distance = zDelta / 120;
			m_GI->SetMoveType(4);
			if (m_GI->SetTopRow(m_GI->TopRow() - (distance * 3)) == UG_SUCCESS)
			{
				m_GI->HideTooltip();
			}
			//m_GI->m_sideHdgWnd->Invalidate();
			_RedrawDirectly();
		}

		//return CWnd::OnMouseWheel(nFlags, zDelta, pt);
		return 0;
	}
	#endif

	//void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
	virtual LRESULT _OnMouseHWheel(WORD fwKeys, short zDelta, CPoint pt) override
	{
		if (m_GI->ShowHScroll())	// Is H scrollbar visible?
		{
			_HideTooltip();
			UINT uSBcode = zDelta < 0 ? SB_LINELEFT : SB_LINERIGHT;
			//m_GI->m_CUGHScroll->HScroll(uSBcode, 0);
			m_GI->HScroll(uSBcode, 0);
		}
		return 0;
	}

	/***************************************************
	OnSetCursor
		The framework calls this member function if mouse input is not captured
		and the mouse causes cursor movement within the CWnd object.
	Params:
		pWnd		- please see MSDN for more information on the parameters.
		nHitTest
		message
	Returns:
		Nonzero to halt further processing, or 0 to continue.
	*****************************************************/
	//BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam) override
	{
		//UNREFERENCED_PARAMETER(*pWnd);
		//UNREFERENCED_PARAMETER(nHitTest);
		//UNREFERENCED_PARAMETER(message);

		SetCursor(m_GI->GetDefaultCursor()/*m_GI->m_arrowCursor*/);
		return TRUE;
	}

	/***************************************************
	OnKeyUp
		The framework calls this member function when a nonsystem key is released,
		the grid passes this event to the cell type and CUGCtrl class for further
		processing.
	Params:
		nChar		- please see MSDN for more information on the parameters.
		nRepCnt
		nFlags
	Returns:
		<none>
	*****************************************************/
	//void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
	virtual LRESULT _OnKeyUp(WPARAM wParam, LPARAM lParam) override
	{
		//UNREFERENCED_PARAMETER(nRepCnt);
		//UNREFERENCED_PARAMETER(nFlags);
		UINT nChar = (UINT)wParam;
		m_keyRepeatCount = 0;

		//send a notify message to the cell type class
		BOOL processed = m_GI->GetCellTypeColRow(m_GI->CurrentCol(), m_GI->CurrentRow())->
			OnKeyUp(m_GI->CurrentCol(), m_GI->CurrentRow(), &nChar);

		//send a keyup notify message
		m_GI->OnKeyUp(&nChar, processed);
		return 0;
	}

	/***************************************************
	OnGetDlgCode
		The Ultimate Grid handles this event to ensure proper behavior when
		grid is placed on a dialog.
	Params:
		<none>
	Returns:
		Please see MSDN for complete list of valid return values.
	*****************************************************/
	//UINT OnGetDlgCode() 
	virtual LRESULT _OnGetDlgCode(WPARAM wParam, LPARAM lParam) override
	{
		HWND hWndFocus = ::GetFocus();
		if (::IsChild(Hwnd(), hWndFocus))
			return DLGC_WANTALLKEYS | DLGC_WANTARROWS;
		else
			return /*CWnd::OnGetDlgCode() |*/ DLGC_WANTARROWS | DLGC_WANTALLKEYS;
	}

	/***************************************************
	GetScrollBarCtrl
		function is used to allow the grid to share its scrollbars with
		parent window.
	Params:
		nBar		- scrollbar to return pointer to.
	Returns:
		Pointer to a sibling scroll-bar control.
	*****************************************************/
	//CScrollBar* GetScrollBarCtrl(int nBar) const
	//{
	////#pragma NOTE("SKMOD to support CXeScrollBar")
	//	if(nBar==SB_HORZ)
	//		return (CXeScrollBarMarkers*)m_ctrl->m_CUGHScroll;
	//	else
	//		return (CXeScrollBarMarkers*)m_ctrl->m_CUGVScroll;
	//}

	/***************************************************
	OnVScroll
		The framework calls this member function when the user clicks the window's vertical scroll bar.
	Params:
		nSBCode		- please see MSDN for more information on the parameters.
		nPos
		pScrollBar
	Returns:
		<none>
	*****************************************************/
	//void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
	virtual LRESULT _OnVScroll(WPARAM wParam, LPARAM lParam) override
	{
		UINT nSBCode = LOWORD(wParam);
		UINT nPos = HIWORD(wParam);
		HWND hSBwnd = (HWND)lParam;
		::SendMessageW(m_GI->CtrlWnd(), WM_VSCROLL, MAKEWPARAM(nSBCode, nPos), (LPARAM)hSBwnd);
		//::SendMessage(m_GI->m_ctrlWnd, WM_VSCROLL,MAKEWPARAM(nSBCode,nPos),
		//	(LPARAM)(pScrollBar!=NULL ? pScrollBar->GetSafeHwnd() : NULL));
		return 0;
	}

	/***************************************************
	OnHScroll
		The framework calls this member function when the user clicks the window's horizontal scroll bar.
	Params:
		nSBCode		- please see MSDN for more information on the parameters.
		nPos
		pScrollBar
	Returns:
		<none>
	*****************************************************/
	//void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
	virtual LRESULT _OnHScroll(WPARAM wParam, LPARAM lParam) override
	{
		UINT nSBCode = LOWORD(wParam);
		UINT nPos = HIWORD(wParam);
		HWND hSBwnd = (HWND)lParam;
		::SendMessageW(m_GI->CtrlWnd(), WM_HSCROLL, MAKEWPARAM(nSBCode, nPos), (LPARAM)hSBwnd);
		//::SendMessage(m_GI->m_ctrlWnd, WM_HSCROLL,MAKEWPARAM(nSBCode,nPos),
		//	(LPARAM)(pScrollBar!=NULL ? pScrollBar->GetSafeHwnd() : NULL));
		return 0;
	}

	/***************************************************
	OnHelpHitTest
		Sent as a result of context sensitive help
		being activated (with mouse) over main grid area
	Params:
		WPARAM - not used
		LPARAM - x, y coordinates of the mouse event
	Return:
		Context help ID to be displayed
	*****************************************************/
	//LRESULT OnHelpHitTest(WPARAM, LPARAM lParam)
	//{
	//	// return context help ID to be looked up
	//	return 0;
	//}

	/***************************************************
	OnHelpInfo
		Sent as a result of context sensitive help
		being activated (with mouse) in main grid area
		if the grid is on the dialog
	Params:
		HELPINFO - structure that contains information on selected help topic
	Return:
		TRUE or FALSE to allow further processing of this message
	*****************************************************/
	//BOOL OnHelpInfo(HELPINFO* pHelpInfo) 
	//{
	//	//if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	//	//{
	//	//	// this message is fired as result of mouse activated help in a dialog
	//	//	int col;
	//	//	long row;
	//
	//	//	if ( GetKeyState( VK_F1 ) < 0 )
	//	//	{
	//	//		col = m_ctrl->GetCurrentCol();
	//	//		row = m_ctrl->GetCurrentRow();
	//	//	}
	//	//	else
	//	//	{
	//	//		CRect rect(0,0,0,0);
	//	//		CPoint point;
	//	//		point.x = pHelpInfo->MousePos.x;
	//	//		point.y = pHelpInfo->MousePos.y;
	//
	//	//		ScreenToClient( &point );
	//	//		int xPos = point.x,
	//	//			yPos = point.y;
	//	//		m_ctrl->GetCellFromPoint( xPos, yPos, &col, &row, &rect );
	//	//	}
	//
	//	//	DWORD dwTopicID = m_ctrl->OnGetContextHelpID( col, row, UG_GRID );
	//	//	if ( dwTopicID == 0 )
	//	//		return FALSE;
	//
	//	//	// call context help with ID returned by the user notification
	//	//	AfxGetApp()->WinHelp( dwTopicID );
	//	//	// prevent further handling of this message
	//	//	return TRUE;
	//	//}
	//	return FALSE;
	//}
};

