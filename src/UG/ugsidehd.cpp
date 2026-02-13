/*************************************************************************
				Class Implementation : CUGSideHdg
**************************************************************************
	Source file : UGSideHd.cpp
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved
*************************************************************************/

#include "../os_minimal.h"
#include <string>

#include "ugdefine.h"
#include "UGCelTyp.h"
#include "uggdinfo.h"
#include "ugsidehd.h"
import Xe.UIcolorsIF;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/***************************************************
	Standard construction/desrtuction
***************************************************/
CUGSideHdg::CUGSideHdg(CXeUIcolorsIF* pUIcolors)
		: CXeD2DWndBase(pUIcolors)
{
	//init the varialbes
	m_isSizing	= FALSE;
	m_canSize	= FALSE;

	//set the row height focus rect to be offscreen
	m_focusRect.top = -1;
	m_focusRect.bottom = -1;
}

CUGSideHdg::~CUGSideHdg()
{
}

bool CUGSideHdg::CreateSideHdg(DWORD dwStyle, const CRect& rect, HWND hParentWnd, UINT nID)
{
	std::wstring classname = L"CUGSideHdg_WNDCLASS";
	m_GI->m_xeUI->RegisterWindowClass(classname, D2DCtrl_WndProc);
	HWND hWnd = CreateD2DWindow(0, classname.c_str(), nullptr, dwStyle,
			rect, hParentWnd, nID, true);
	m_isHideTooltipOnMouseLeave = false;
	return hWnd != 0;
}

/***************************************************
OnPaint
	This routine is responsible for gathering information on cells to draw,
	and draw in an optomized fashion.
Params:
	<none>
Returns:
	<none>
*****************************************************/
//void CUGSideHdg::OnPaint() 
void CUGSideHdg::_PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient)
{
	if ( m_GI->m_paintMode == FALSE )
		return;

	DrawCellsIntern(this, rcClient);
}

void CUGSideHdg::RedrawAll()
{
	_RedrawDirectly();
}

/***************************************************
DrawCellsIntern
	function is the key to the fast redraw functionality in the Ultimate Grid.
	This function is responsible for drawing cells within the grid area, it
	makes sure that only the cells that are marked as invalid
	(by calling CUGDrawHint::IsInvalid function) are redrawn.
Params:
	dc		- pointer DC to draw on
Returns:
	<none>
*****************************************************/
//void CUGSideHdg::DrawCellsIntern(CDC *dc)
void CUGSideHdg::DrawCellsIntern(CXeD2DRenderContext* pRctx, D2D1_RECT_F rcClient)
{
	ID2D1RenderTarget* pRT = pRctx->m_pCurrentRT;
	CRect rect(0,0,0,0), cellRect;
	CUGCell cell;
	CUGCellType * cellType;

	int xIndex, col;
	long yIndex, row = 0;

	m_bottomRow = m_GI->m_numberRows;

	for(yIndex = 0; yIndex <m_GI->m_numberRows; yIndex++)
	{
		if(yIndex == m_GI->m_numLockRows)
			yIndex = m_GI->m_topRow;

		row = yIndex;

		rect.top = rect.bottom;
		rect.bottom += m_GI->GetRowHeight(row);
		rect.right = 0;

		if(rect.top == rect.bottom)
			continue;

		for(xIndex = (m_GI->m_numberSideHdgCols*-1) ;xIndex <0;xIndex++)
		{	
			col = xIndex;
			row = yIndex;

			rect.left = rect.right;
			rect.right  += GetSHColWidth(col);

			//draw if invalid
			//if(m_drawHint.IsInvalid(col,row) != FALSE)
			{
				CopyRect(&cellRect,&rect);

				m_GI->GetCellIndirect(col,row,&cell);

				if(cell.IsPropertySet(UGCELL_JOIN_SET))
				{
					GetCellRect(col,row,&cellRect);
					m_GI->GetJoinStartCell(&col,&row,&cell);
				}

				cellType = m_GI->GetCellType(cell.GetCellType());
				cellType->OnDraw(pRctx, EXE_FONT::eUI_Font, &cellRect,col,row,&cell,0,0);
			}
		}
		if(rect.bottom > m_GI->m_gridHeight)
			break;
	}

	m_bottomRow = row;

	if(rect.bottom  < m_GI->m_gridHeight)
	{
		rect.top = rect.bottom;
		rect.bottom = m_GI->m_gridHeight;
		rect.left = 0;
		rect.right = m_GI->m_sideHdgWidth;
		// fill-in the area that is not covered by cells
		pRT->FillRectangle(RectFfromRect(rect), GetBrush(CID::GrdHdrFillBg));
	}
}

/************************************************
Update 
	function makes sure that the last column in the side heading has
	proper width before the window is redrawn.
Params:
	<none>
Returns:
	<none>
*************************************************/
void CUGSideHdg::Update()
{
	//calc the last col width
	int xIndex, width = 0;
	for(xIndex = -1;xIndex > (m_GI->m_numberSideHdgCols*-1);xIndex--)
	{
		width += GetSHColWidth(xIndex);
	}

	width = m_GI->m_sideHdgWidth - width;

	if( width < 0 )
		width = 0;

	m_GI->SetSH_ColWidth(xIndex,width);

	//redraw the window
	_RedrawDirectly();
}

/************************************************
Moved
	function makes sure that the grid view has moved vertically
	before the window update is allowed.  With this check the Ultimate
	Grid eliminates un-necessary redraws.
Params:
	<none>
Returns:
	<none>
*************************************************/
void CUGSideHdg::Moved()
{
	_RedrawDirectly();

	//if(m_GI->m_topRow == m_GI->m_lastTopRow && m_GI->m_currentRow == m_GI->m_lastRow)
	//	return;

	//if(GetUpdateRect(NULL))
	//{
	//	UpdateWindow();
	//	return;
	//}

	//int yScroll = 0;
	//long yIndex;
	//CDC * pDC;
	//BOOL redrawAll = FALSE;

	////if the top row has changed then calc the scroll distance
	//if(m_GI->m_topRow != m_GI->m_lastTopRow)
	//{
	//	if(m_GI->m_topRow > m_GI->m_lastTopRow)
	//	{
	//		for(yIndex = m_GI->m_lastTopRow;yIndex < m_GI->m_topRow; yIndex++)
	//		{
	//			yScroll += m_GI->GetRowHeight(yIndex);
	//			if(yScroll > m_GI->m_gridHeight)
	//			{
	//				redrawAll = TRUE;
	//				break;
	//			}
	//		}
	//	}
	//	else
	//	{
	//		for(yIndex = m_GI->m_topRow;yIndex < m_GI->m_lastTopRow; yIndex++)
	//		{
	//			yScroll -= m_GI->GetRowHeight(yIndex);
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
	//		//redraw the whole heading
	//		m_drawHint.AddToHint(m_GI->m_numberSideHdgCols * -1,0,0,m_GI->m_numberRows);
	//	}
	//}

	////create the device context
	//pDC = GetDC();

	////do scrolling of the window here
	//if(yScroll != 0)
	//{
	//	RECT scrollRect;
	//	GetClientRect(&scrollRect);
	//	scrollRect.top += m_GI->m_lockRowHeight;
	//	pDC->ScrollDC(0,-yScroll,&scrollRect,&scrollRect,NULL,NULL);
	//	
	//	//add the draw hints for the area the was uncovered by the scroll
	//	if(yScroll >0)
	//		m_drawHint.AddToHint(m_GI->m_numberSideHdgCols * -1,m_bottomRow,0,m_GI->m_numberRows);
	//	else if(yScroll < 0)
	//		m_drawHint.AddToHint(m_GI->m_numberSideHdgCols * -1,0,0,m_GI->m_lastTopRow);
	//}

	////add the last and current cells 
	//m_drawHint.AddHint(m_GI->m_numberSideHdgCols * -1,m_GI->m_lastRow,0,m_GI->m_lastRow);
	//m_drawHint.AddHint(m_GI->m_numberSideHdgCols * -1,m_GI->m_currentRow,0,m_GI->m_currentRow);
	////call the grids main drawing routine
	//DrawCellsIntern(pDC);
	////clear the draw hints
	//m_drawHint.ClearHints();

	////close the device context
	//ReleaseDC(pDC);
}

/************************************************
OnLButtonDown
	Finds the cell that was clicked in, sends a notification
	updates the current cells position.
	It also sets mouse capture, which will be used with sizing operation.
Params:
	nFlags		- please see MSDN for more information on the parameters.
	point
Returns:
	<none>
*************************************************/
//void CUGSideHdg::OnLButtonDown(UINT nFlags, CPoint point)
LRESULT CUGSideHdg::_OnLeftDown(UINT nFlags, CPoint point)
{
	int col;
	long row;
	RECT rect;

	UNREFERENCED_PARAMETER(nFlags);

	if(::GetFocus() != m_GI->m_gridWnd)
		::SetFocus(m_GI->m_gridWnd);

	if(m_canSize)
	{
		m_isSizing = TRUE;
		SetCapture();
	}
	else if(GetCellFromPoint(&point,&col,&row,&rect) == UG_SUCCESS)
	{
		//send a notification to the cell type	
		BOOL processed = m_GI->GetCellTypeColRow(col,row)->OnLClicked(col,row,1,&rect,&point);
		//send a notification to the main grid class
		m_GI->OnSH_LClicked(col,row,1,&rect,&point,processed);
	}
	return 0;
}

/************************************************
OnLButtonUp
	Finds the cell that was clicked in, and sends a notification to 
	CUCtrl::OnSH_LClicked.  It also releases the mouse capture.
Params:
	nFlags		- please see MSDN for more information on the parameters.
	point
Returns:
	<none>
*************************************************/
//void CUGSideHdg::OnLButtonUp(UINT nFlags, CPoint point)
LRESULT CUGSideHdg::_OnLeftUp(UINT nFlags, CPoint point)
{
	int col;
	long row;
	RECT rect;

	UNREFERENCED_PARAMETER(nFlags);

	if(m_isSizing)
	{
		m_isSizing = FALSE;
		m_focusRect.top = -1;
		m_focusRect.bottom = -1;		
		ReleaseCapture();

		if(m_colOrRowSizing == 0)	//col sizing
		{
			m_GI->OnSideHdgSized(&m_GI->m_sideHdgWidth);
		}
		else
		{
			int height = m_GI->GetRowHeight(m_sizingColRow);
			// send the resize notification to all visible cells in this row
			for(int nIndex= m_GI->GetLeftCol(); nIndex< m_GI->GetRightCol(); nIndex++)
			{
				CUGCellType* pCellType= m_GI->GetCellTypeColRow(nIndex,m_sizingColRow);
				if(pCellType!=NULL)
				{
					pCellType->OnChangedCellHeight(nIndex,m_sizingColRow,&height);
				}
			}
			m_GI->OnRowSized(m_sizingColRow,&height);
			m_GI->SetRowHeight(m_sizingColRow,height);
		}
		m_GI->AdjustComponentSizes();
		
		m_GI->OnColRowSizeFinished();
	}
	else if(GetCellFromPoint(&point,&col,&row,&rect) == UG_SUCCESS)
	{
		//send a notification to the cell type	
		BOOL processed = m_GI->GetCellTypeColRow(col,row)->OnLClicked(col,row,0,&rect,&point);
		//send a notification to the main grid class
		m_GI->OnSH_LClicked(col,row,0,&rect,&point,processed);
	}	
	return 0;
}

/************************************************
OnLButtonDblClk
	Finds the cell that was clicked in and sends a notification to
	CUCtrl::OnSH_DClicked.
Params:
	nFlags		- mouse button states
	point		- point where the mouse is
Returns:
	<none>
*************************************************/
//void CUGSideHdg::OnLButtonDblClk(UINT nFlags, CPoint point)
LRESULT CUGSideHdg::_OnLeftDoubleClick(UINT nFlags, CPoint point)
{
	int col;
	long row;
	RECT rect;

	UNREFERENCED_PARAMETER(nFlags);

	if(GetCellFromPoint(&point,&col,&row,&rect) == UG_SUCCESS)
	{
		//send a notification to the cell type	
		BOOL processed = m_GI->GetCellTypeColRow(col,row)->OnLClicked(col,row,0,&rect,&point);
		//send a notification to the main grid class
		m_GI->OnSH_DClicked(col,row,&rect,&point,processed);
	}
	return 0;
}

/************************************************
OnMouseMove
	Checks to see if the mouse is over a cell separation line, if it is then it
	checks to see if sizing is allowed.  If it is then the cusor is changed to
	the sizing cusror.
	If sizing is in progress then row height or column width within the heading are
	updated.
Params:
	nFlags		- mouse button states
	point		- point where the mouse is
Returns:
	<none>
*************************************************/
//void CUGSideHdg::OnMouseMove(UINT nFlags, CPoint point)
LRESULT CUGSideHdg::_OnMouseMove(UINT nFlags, CPoint point)
{
	CXeD2DWndBase::_OnMouseMove(nFlags, point);

	//check to see ifthe mouse is over a cell separation
	//but only if the mouse is not currently sizing 
	if(m_isSizing == FALSE && (nFlags&MK_LBUTTON) == 0 && m_GI->m_userSizingMode >0)
	{
		m_canSize = FALSE;

		//side heading column width sizing
		int width = m_GI->m_sideHdgWidth;
		for(int col = 0; col < m_GI->m_numberSideHdgCols ;col++)
		{
			if(point.x < width+3 && point.x > width-3)
			{
				if(m_GI->OnCanSizeSideHdg() == FALSE)
					return 0;

				m_canSize = TRUE;
				m_colOrRowSizing = 0;
				m_sizingColRow = col;
				m_sizingStartSize = m_GI->m_sideHdgWidths[col];
				m_sizingStartPos = point.x;
				m_sizingStartWidth = m_GI->m_sideHdgWidth;

				SetCursor(m_GI->m_WEResizseCursor);
				return 0;
			}
			width -= m_GI->m_sideHdgWidths[col];
		}

		//side heading row height sizing
		int height = 0;
		for(long row = 0;row < m_GI->m_numberRows;row++)
		{
			if(row == m_GI->m_numLockRows)
				row = m_GI->m_topRow;

			height += m_GI->GetRowHeight(row);
			if(height > m_GI->m_gridHeight)
				break;

			if(point.y < height+3 && point.y > height-3)
			{
				if(m_GI->GetRowHeight(row+1) == 0 && (row+1) < m_GI->m_numberRows)
					row++;

				if(m_GI->OnCanSizeRow(row) == FALSE)
					return 0;

				m_canSize = TRUE;
				m_colOrRowSizing = 1;
				m_sizingColRow = row;
				m_sizingStartSize = m_GI->GetRowHeight(row);
				m_sizingStartPos = point.y;
				if(m_GI->m_uniformRowHeightFlag)
				{
					if( 0 != m_GI->m_defRowHeight )
						m_sizingNumRowsDown =  point.y / m_GI->m_defRowHeight;
				}	
				SetCursor(m_GI->m_NSResizseCursor);
				return 0;
			}
		}
		return 0;
	}

	//perform the sizing
	else if(m_isSizing)
	{
		if(m_colOrRowSizing == 0)	//col sizing
		{
			int width = m_sizingStartSize +( point.x - m_sizingStartPos);
			if(width <0)
				width = 0;
			m_GI->m_sideHdgWidths[m_sizingColRow] = width;
			width = m_sizingStartWidth + (point.x - m_sizingStartPos);
			if(width <0)
				width = 0;

			m_GI->OnSideHdgSizing(&width);

			m_GI->m_sideHdgWidth = width;
			m_GI->AdjustComponentSizes();

		}
		else					//row sizing
		{
			int height;
			if(m_GI->m_uniformRowHeightFlag)
			{
				if(point.y <1)
					point.y = 1;
				if(m_sizingNumRowsDown>0)
					height = point.y / m_sizingNumRowsDown;
				else
					height = point.y;
			}
			else
				height = m_sizingStartSize+(point.y - m_sizingStartPos);

			if(height < 0)
				height = 0 ;

			// send the resize notification to all visible cells in this row
			for(int nIndex= m_GI->GetLeftCol(); nIndex< m_GI->GetRightCol(); nIndex++)
			{
				CUGCellType* pCellType= m_GI->GetCellTypeColRow(nIndex,m_sizingColRow);
				if(pCellType!=NULL)
				{
					pCellType->OnChangingCellHeight(nIndex,m_sizingColRow,&height);
				}
			}

			m_GI->OnRowSizing(m_sizingColRow,&height);

			m_GI->SetRowHeight(m_sizingColRow,height);

			if(m_GI->m_userSizingMode == 1) // focus rect
			{
				Update();
				//CDC* dc = m_GI->m_gridWnd->GetDC();
				//dc->DrawFocusRect(&m_focusRect);
				m_focusRect.top = point.y-1;
				m_focusRect.bottom = point.y+1;
				m_focusRect.left = 0;
				m_focusRect.right = m_GI->m_gridWidth;
				//dc->DrawFocusRect(&m_focusRect);
				//m_GI->m_gridWnd->ReleaseDC(dc);

			}
			else
				m_GI->RedrawAll();
		}
	}

//#pragma NOTE("SK MOD to show custom cursor")
	// Call CUGCtrl::OnMouseMove -> calls CXeGrid::OnMouseMove to set custom cursor.
	if( !m_isSizing && !m_canSize )
	{
		int col;
		long row;
		RECT rect;
		if( GetCellFromPoint( &point, &col, &row, &rect ) != UG_SUCCESS )
			return 0;
		m_GI->OnMouseMove( col, row, &point, nFlags, FALSE );
	}
	return 0;
}

/************************************************
OnRButtonDown
	function sends a mouse click notification to the main grid class, 
	checks to see if menus are enabled. If so then menu specific
	notifications are sent as well.
Params:
	nFlags		- mouse button states
	point		- point where the mouse is
Returns:
	<none>
*************************************************/
//void CUGSideHdg::OnRButtonDown(UINT nFlags, CPoint point)
LRESULT CUGSideHdg::_OnRightDown(UINT nFlags, CPoint point)
{
	int col;
	long row;
	RECT rect;

	UNREFERENCED_PARAMETER(nFlags);

	if(::GetFocus() != m_GI->m_gridWnd)
		::SetFocus(m_GI->m_gridWnd);

	if(GetCellFromPoint(&point,&col,&row,&rect) == UG_SUCCESS)
	{
		//send a notification to the cell type	
		BOOL processed = m_GI->GetCellTypeColRow(col,row)->OnLClicked(col,row,0,&rect,&point);
		//send a notification to the main grid class
		m_GI->OnSH_RClicked(col,row,1,&rect,&point,processed);
	}

	if(m_GI->m_enablePopupMenu)
	{
		ClientToScreen(&point);
		m_GI->StartMenu(col,row,&point,UG_SIDEHEADING);
	}
	return 0;
}

/************************************************
OnRButtonUp
	Sends a mouse click notification to the main grid class.
Params:
	nFlags		- mouse button states
	point		- point where the mouse is
Returns:
	<none>
*************************************************/
//void CUGSideHdg::OnRButtonUp(UINT nFlags, CPoint point)
LRESULT CUGSideHdg::_OnRightUp(UINT nFlags, CPoint point)
{
	int col;
	long row;
	RECT rect;

	UNREFERENCED_PARAMETER(nFlags);

	if(GetCellFromPoint(&point,&col,&row,&rect) == UG_SUCCESS)
	{
		//send a notification to the cell type	
		BOOL processed = m_GI->GetCellTypeColRow(col,row)->OnLClicked(col,row,0,&rect,&point);
		//send a notification to the main grid class
		m_GI->OnSH_RClicked(col,row,0,&rect,&point,processed);
	}
	return 0;
}

/************************************************
OnSetCursor
	The framework calls this member function if mouse input is not captured
	and the mouse causes cursor movement within the CWnd object.
Params:
	pWnd		- please see MSDN for more information on the parameters.
	nHitTest
	message
Returns:
	Nonzero to halt further processing, or 0 to continue.
*************************************************/
//BOOL CUGSideHdg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
LRESULT CUGSideHdg::_OnSetCursor(WPARAM wParam, LPARAM lParam)
{
	//UNREFERENCED_PARAMETER(*pWnd);
	//UNREFERENCED_PARAMETER(nHitTest);
	//UNREFERENCED_PARAMETER(message);

	if (!m_canSize)
	{
		SetCursor(m_GI->GetDefaultCursor()/*m_GI->m_arrowCursor*/);
	}
	else if(m_colOrRowSizing == 0)
		SetCursor(m_GI->m_WEResizseCursor);
	else
		SetCursor(m_GI->m_NSResizseCursor);
		
	return 1;
}

/************************************************
GetCellRect
	Returns the rectangle for the given cell co-ordinates.
	If the cell is joined then the given co-ordinates 
	are modified to point to the start cell for the join.
Params:
	col		- column to find the rect of
	row		- row to find the rect of
	rect	- rectangle to calculate
Returns:
	UG_SUCCESS	- success
*************************************************/
int CUGSideHdg::GetCellRect(int col,long row,RECT *rect)
{
	return GetCellRect(&col,&row,rect);

}
/************************************************
GetCellRect
	Returns the rectangle for the given cell co-ordinates.
	If the cell is joined then the given co-ordinates 
	are modified to point to the start cell for the join.
Params:
	col		- column to find the rect of
	row		- row to find the rect of
	rect	- rectangle to calculate
Returns:
	UG_SUCCESS	- success
*************************************************/
int CUGSideHdg::GetCellRect(int *col,long *row,RECT *rect)
{
	int xIndex,yIndex;
	int width	= 0;
	int height	= 0;

	int startCol	= *col;
	long startRow	= *row;
	int endCol		= *col;
	long endRow		= *row;

	rect->left		= 0;
	rect->top		= 0;
	rect->right		= m_GI->m_sideHdgWidth;
	rect->bottom	= 0;
	
	//if the specified cell is within a join then find the joined range
	if(m_GI->m_enableJoins)
	{
		if(m_GI->GetJoinRange(&startCol,&startRow,&endCol,&endRow) == UG_SUCCESS)
		{
			*col = startCol;
			*row = startRow;
		}
	}

	//find the col
	for(xIndex= (m_GI->m_numberSideHdgCols * -1);xIndex < 0;xIndex++)
	{
		if(xIndex == startCol)
			rect->left = width;
		
		width += GetSHColWidth(xIndex);

		if(xIndex == endCol)
		{
			rect->right = width;
			break;
		}
	}

	//find the row
	if(startRow >= m_GI->m_numLockRows)
	{
		rect->top = m_GI->m_lockRowHeight;
		rect->bottom = m_GI->m_lockRowHeight;
	}
	for(yIndex= 0;yIndex < m_GI->m_numberRows ;yIndex++)
	{		
		if(yIndex == m_GI->m_numLockRows)
			yIndex = m_GI->m_topRow;

		if(yIndex == startRow)
			rect->top = height;

		height += m_GI->GetRowHeight(yIndex);

		if(yIndex == endRow)
		{
			rect->bottom = height;	
			break;
		}
	}

	return UG_SUCCESS;
}

/************************************************
GetCellFromPoint
	Returns the column and row that lies over the given x,y co-ordinates.
	The co-ordinates are relative to the top left hand corner of the grid.
Params:
	point - point to check
	col	  - column that lies over the point
	row   - row that lies over the point
	rect  - rectangle of the cell found
Returns:
	UG_SUCCESS	- if a matching cell was found 
	UG_ERROR	- point provided does not correspond to a cell
*************************************************/
int CUGSideHdg::GetCellFromPoint(CPoint *point,int *col,long *row,RECT *rect)
{
	int ptsFound = 0;
	int xIndex;
	long yIndex;

	rect->left		=0;
	rect->top		=0;
	rect->right		=0;
	rect->bottom	=0;

	//find the row
	for(yIndex=0;yIndex<m_GI->m_numberRows;yIndex++)
	{	
		if(yIndex == m_GI->m_numLockRows)
			yIndex = m_GI->m_topRow;
		
		rect->bottom += m_GI->GetRowHeight(yIndex);

		if(rect->bottom > point->y)
		{
			rect->top = rect->bottom - m_GI->GetRowHeight(yIndex);
			ptsFound ++;
			*row = yIndex;
			break;
		}
	}

	//find the col
	for(xIndex= m_GI->m_numberSideHdgCols * -1 ;xIndex <0 ;xIndex++)
	{
		rect->right += GetSHColWidth(xIndex);

		if(rect->right > point->x)
		{
			rect->left = rect->right - GetSHColWidth(xIndex);
			ptsFound ++;
			*col = xIndex;
			break;
		}
	}

	if(ptsFound == 2)
		return UG_SUCCESS;

	*col = -1;
	*row = -1;
	return UG_ERROR;
}

/************************************************
GetSHColWidth
	function returns height (in pixels) of a side heading' column that user is
	interested in.  The column number passed in has to be a negative value ranging
	from zero to negative value of number of side heading columns set.
	((m_GI->m_numberSideHdgCols) * -1)
Params:
	col			- indicates row of interest
Returns:
	width in pixels of the column in question, or zero if column not found.
*************************************************/
int CUGSideHdg::GetSHColWidth(int col)
{
	//translate the column number into a 0 based positive index
	col = (col * -1) -1;

	if(col <0 || col > m_GI->m_numberSideHdgCols)
		return 0;

	return m_GI->m_sideHdgWidths[col];
}

/************************************************
OnHelpHitTest
	Sent as a result of context sensitive help
	being activated (with mouse) over side heading
Params:
	WPARAM - not used
	LPARAM - x, y coordinates of the mouse event
Returns:
	Context help ID to be displayed
*************************************************/
//LRESULT CUGSideHdg::OnHelpHitTest(WPARAM, LPARAM lParam)
//{
//	// this message is fired as result of mouse activated
//	// context help being hit over the grid.
//	// return context help ID to be looked up
//	return 0;
//}

/************************************************
OnHelpInfo
	Sent as a result of context sensitive help
	being activated (with mouse) over side heading
	if the grid is on the dialog
Params:
	HELPINFO - structure that contains information on selected help topic
Returns:
	TRUE or FALSE to allow further processing of this message
*************************************************/
//BOOL CUGSideHdg::OnHelpInfo(HELPINFO* pHelpInfo) 
//{
//	return FALSE;
//}

LRESULT CUGSideHdg::_OnNotify_NeedTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT)
{
	XeASSERT(pNeedTT && m_xtooltip);
	if (pNeedTT && m_xtooltip)
	{
		return m_GI->MakeSuperTooltip(pNeedTT, m_xtooltip, GetSafeHwnd(), UG_SIDEHEADING);
	}
	return 0;
}

//LRESULT CUGSideHdg::OnMouseLeave(WPARAM wparam, LPARAM lparam)
LRESULT CUGSideHdg::_OnMouseLeave(WPARAM wparam, LPARAM lparam)
{
	if (m_xtooltip && !m_xtooltip->IsMouseOverTooltip())	// Don't hide tooltip if cursor is over it.
	{
		_HideTooltip();
	}
	return CXeD2DWndBase::_OnMouseLeave(wparam, lparam);
}

