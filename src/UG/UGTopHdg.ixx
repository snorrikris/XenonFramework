module;

/*************************************************************************
				Class Implementation : CUGTopHdg
**************************************************************************
	Source file : UGTopHdg.cpp
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved
*************************************************************************/
/*************************************************************************
				Class Declaration : CUGTopHdg
**************************************************************************
	Source file : UGTopHdg.cpp
	Header file : UGTopHdg.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		The top heading (CUGTopHdg) object/window
		is responsible to draw cells and handle
		user's actions on the column heading.

	Keay features:
		- This class provides ability to resize
		  column width with the mouse.
		- as well as the height of the entire
		  top heading and rows it contains
		- mouse and keyboard messages are forwarded
		  to the CUGCtrl class as notifications.
		  OnTH_...

*************************************************************************/

#include "../os_minimal.h"

#include "ugdefine.h"

export module Xe.UGTopHdg;

import std;
import Xe.UIcolorsIF;

import Xe.D2DWndBase;
import Xe.FileTimeX;
import Xe.UGGridInfoIF;
import Xe.UGCelTyp;
import Xe.UGDrawHint;
import Xe.UGCell;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//class CUGGridInfo;

export class CUGTopHdg : public CXeD2DWndBase
{
public:
	CUGGridInfoIF* m_GI;		//pointer to the grid information

protected:
	CUGCell			m_cell;		//general purpose cell class


	BOOL			m_isSizing;			//sizing flag
	BOOL			m_canSize;			//sizing flag
	BOOL			m_colOrRowSizing;	// 0-col 1-row
	int				m_sizingColRow;		//column/row being sized
	int				m_sizingStartSize;	//original size
	int				m_sizingStartPos;	//original start pos
	int				m_sizingStartHeight;//original top heading total height

	RECT			m_focusRect;		//focus rect for column sizing option

	//CUGDrawHint		m_drawHint;		//cell drawing hints

	int				m_swapStartCol;
	int				m_swapEndCol;

	FILETIMEX		m_ftxLastBestFit;

public:
	/***************************************************
		Standard construction/desrtuction
	***************************************************/
	CUGTopHdg(CXeUIcolorsIF* pUIcolors) : CXeD2DWndBase(pUIcolors)
	{
		//init the varialbes
		m_isSizing = FALSE;
		m_canSize = FALSE;

		//set the last focus rect position
		m_focusRect.left = -1;
		m_focusRect.right = -1;
		m_focusRect.top = -1;
		m_focusRect.bottom = -1;

		m_swapStartCol = -1;	//columns to swap
		m_swapEndCol = -1;
	}

	virtual ~CUGTopHdg()
	{}

	bool CreateTopHdg(DWORD dwStyle, const CRect& rect, HWND hParentWnd, UINT nID)
	{
		std::wstring classname = L"CUGTopHdg_WNDCLASS";
		m_GI->GetXeUI()->RegisterWindowClass(classname, D2DCtrl_WndProc);
		dwStyle = dwStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		HWND hWnd = CreateD2DWindow(0, classname.c_str(), nullptr, dwStyle, rect, hParentWnd, nID);
		return hWnd != 0;
	}

protected:
	/***************************************************
	OnPaint
		This routine is responsible for gathering information on cells to draw,
		and draw in an optomized fashion.
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

		DrawCellsIntern(this);
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
	void DrawCellsIntern(CXeD2DRenderContext* pRctx)
	{
		ID2D1RenderTarget* pRT = pRctx->m_pCurrentRT;
		CRect rect(0, 0, 0, 0), cellRect;
		CUGCell cell;
		CUGCellTypeIF* cellType;
		//int dcID;
		int xIndex, col;
		long yIndex, row;

		int blankRight = 0;

		for (yIndex = (m_GI->NumberTopHdgRows() * -1); yIndex < 0; yIndex++)
		{
			row = yIndex;

			for (xIndex = 0; xIndex < m_GI->NumberCols(); xIndex++)
			{
				if (xIndex == m_GI->NumLockCols())
					xIndex = m_GI->LeftCol();
				col = xIndex;
				row = yIndex;

				//draw if invalid
				//if(m_drawHint.IsInvalid(col,row) != FALSE)
				{
					GetCellRect(col, row, &rect);
					CopyRect(&cellRect, &rect);

					m_GI->GetCellIndirect(col, row, &cell);

					if (cell.IsPropertySet(UGCELL_JOIN_SET))
					{
						GetCellRect(col, row, &cellRect);
						m_GI->GetJoinStartCell(&col, &row, &cell);
					}

					if (cellRect.left < cellRect.right)
					{
						cellType = m_GI->GetCellType(cell.GetCellType());

						if (m_swapEndCol >= 0 && col == m_swapStartCol)
							cellType->OnDraw(pRctx, EXE_FONT::eUI_FontBold, &cellRect, col, row, &cell, 1, 0);
						else
							cellType->OnDraw(pRctx, EXE_FONT::eUI_FontBold, &cellRect, col, row, &cell, 0, 0);
					}
				}
				if (rect.right > m_GI->GridWidth())
					break;
			}
			if (blankRight < rect.right)
				blankRight = rect.right;
		}
		if (blankRight < m_GI->GridWidth())
		{
			rect.top = 0;
			rect.bottom = m_GI->TopHdgHeight();
			rect.left = blankRight;
			rect.right = m_GI->GridWidth();
			// fill-in the area that is not covered by cells
			// for some reason the next line calls CXeGrid::OnKillFocus ? ? ? ? ? WTF
			pRT->FillRectangle(RectFfromRect(rect), GetBrush(CID::GrdHdrFillBg));
		}
	}

public:
	/************************************************
	Update
		function makes sure that the last row in the top heading has proper
		height before the window is redrawn.
	Params:
		<none>
	Return:
		<none>
	*************************************************/
	void Update()
	{
		//calc the last row height
		//find the row
		int yIndex, height = 0;
		for (yIndex = -1; yIndex > (m_GI->NumberTopHdgRows() * -1); yIndex--)
		{
			height += GetTHRowHeight(yIndex);
		}

		height = m_GI->TopHdgHeight() - height;

		if (height < 0)
			height = 0;

		m_GI->SetTH_RowHeight(yIndex, height);

		//redraw the window
		_RedrawDirectly();
	}

	/************************************************
	Moved
		function makes sure that the grid view has moved horizontally
		before the window update is allowed.  With this check the Ultimate
		Grid eliminates un-necessary redraws.
	Params:
		<none>
	Returns:
		<none>
	*************************************************/
	void Moved()
	{
		if (m_GI->LeftCol() == m_GI->LastLeftCol())
			return;

		//redraw the window
		_RedrawDirectly();
	}

protected:
	/***************************************************
	CheckForUserResize
		function is called when user clicks the left mouse button or moves the
		mouse cursor over the area in the top heading that would allow column
		or row sizing.  This function is used to determine if sizing is allowed.
		Instead of having a return value, this function sets member vairables
		to proper state.
	Params:
		point		- pos of the mouse pointer
	Returns:
		<none>
	*****************************************************/
	void CheckForUserResize(CPoint* point)
	{
		if (m_GI->UserSizingMode() == FALSE)
			return;

		//top heading column sizing
		int width = 0;
		for (int col = 0; col < m_GI->NumberCols(); col++)
		{
			if (col == m_GI->NumLockCols() && col < m_GI->LeftCol())
				col = m_GI->LeftCol();

			width += m_GI->GetColWidth(col);
			if (width > m_GI->GridWidth())
				break;

			if (point->x < width + 3 && point->x > width - 3)
			{
				if (m_GI->GetColWidth(col + 1) == 0 && (col + 1) < m_GI->NumberCols())
					col++;

				if (m_GI->OnCanSizeCol(col) == FALSE)
					return;

				m_canSize = TRUE;
				m_colOrRowSizing = 0;				// 0-col 1-row
				m_sizingColRow = col;				//column/row being sized
				m_sizingStartSize = m_GI->GetColWidth(col);//original size
				m_sizingStartPos = point->x;			//original start pos

				SetCursor(m_GI->WEResizseCursor());
				return;
			}
		}

		//top heading row sizing
		int height = m_GI->TopHdgHeight();
		for (int row = 0; row < m_GI->NumberTopHdgRows(); row++)
		{
			if (point->y < height + 3 && point->y > height - 3)
			{
				if (m_GI->OnCanSizeTopHdg() == FALSE)
					return;

				m_canSize = TRUE;
				m_colOrRowSizing = 1;				// 0-col 1-row
				m_sizingColRow = row;				//column/row being sized
				m_sizingStartSize = m_GI->GetTopHdgRowHeight(row);//original size
				m_sizingStartPos = point->y;			//original start pos
				m_sizingStartHeight = m_GI->TopHdgHeight();

				SetCursor(m_GI->NSResizseCursor());
				return;
			}
			height -= m_GI->GetTopHdgRowHeight(row);
		}

		if (m_canSize)
		{
			m_canSize = FALSE;
			SetCursor(m_GI->GetDefaultCursor()/*m_GI->m_arrowCursor*/);
		}
	}

	/************************************************
	OnMouseMove
		Checks to see if the mouse is over a cell separation line, if it is then it
		checks to see if sizing is allowed.  If it is then the cusor is changed to
		the sizing cusror.
		If sizing is in progress then column width or top heading row height are
		updated.
		This is also where the column swapping operation is handled.
	Params:
		nFlags		- mouse button states
		point		- point where the mouse is
	Return:
		<none>
	*************************************************/
	//void OnMouseMove(UINT nFlags, CPoint point) 
	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{
		CXeD2DWndBase::_OnMouseMove(nFlags, point);

		//check to see if the mouse is over a cell separation 
		//if the mouse is not currently sizing
		if (m_isSizing == FALSE && (nFlags & MK_LBUTTON) == 0 && m_GI->UserSizingMode() > 0)
		{
			//check for user resize position
			CheckForUserResize(&point);
		}
		else if (m_isSizing)
		{
			if (m_colOrRowSizing == 0)	//col sizing
			{
				if (point.x < m_sizingStartPos - m_sizingStartSize)
					point.x = m_sizingStartPos - m_sizingStartSize;

				int width = m_sizingStartSize + (point.x - m_sizingStartPos);

				// send notifications to cell types.  In order to provide acceptable performance
				// this notification will only be sent to the visible rows.
				for (int nIndex = m_GI->GetTopRow(); nIndex < m_GI->GetBottomRow(); nIndex++)
				{
					CUGCellTypeIF* pCellType = m_GI->GetCellTypeColRow(m_sizingColRow, nIndex);
					if (pCellType != NULL)
					{
						pCellType->OnChangingCellWidth(m_sizingColRow, nIndex, &width);
					}
				}
				//send notification to control
				m_GI->OnColSizing(m_sizingColRow, &width);

				//just draw a focus rect
				if (m_GI->UserSizingMode() == 1)
				{
					m_GI->SetColWidth(m_sizingColRow, width, true);
					Update();

					//CDC* dc = m_GI->m_gridWnd->GetDC();
					//dc->DrawFocusRect(&m_focusRect);
					m_focusRect.top = 0;
					m_focusRect.bottom = m_GI->GridHeight();
					m_focusRect.left = point.x - 1;
					m_focusRect.right = point.x + 1;
					//dc->DrawFocusRect(&m_focusRect);
					//m_GI->m_gridWnd->ReleaseDC(dc);

				}
				else	//update on the fly
				{
					m_GI->SetColWidth(m_sizingColRow, width, true);
					m_GI->RedrawAll();
				}
			}
			else	//row sizing
			{
				int height = m_sizingStartSize + (point.y - m_sizingStartPos);
				if (height < 0)
					height = 0;
				m_GI->SetTopHdgRowHeight(m_sizingColRow, height);

				height = m_sizingStartHeight + (point.y - m_sizingStartPos);
				if (height < 0)
					height = 0;

				if (m_GI->OnTopHdgSizing(&height) == TRUE)
				{
					m_GI->SetTopHdgHeight(height);
					m_GI->AdjustComponentSizes();
				}
			}
		}
		//check for column swapping
		else if (m_GI->EnableColSwapping() && m_swapStartCol >= 0)
		{
			MSG msg;

			//while column swapping enable mouse scrolling of the grid
			if (point.x < 0 || point.x > m_GI->GridWidth())
			{
				//remove the focus rectangle
				//CDC* dc = m_GI->m_gridWnd->GetDC();
				//dc->DrawFocusRect(&m_focusRect);
				m_focusRect.left = -1;
				m_focusRect.right = -1;
				//m_GI->m_gridWnd->ReleaseDC(dc);

				while (1)
				{
					if (point.x < 0)
						m_GI->MoveLeftCol(UG_LINEUP);
					else if (point.x > m_GI->GridWidth())
						m_GI->MoveLeftCol(UG_LINEDOWN);

					//check for messages, if ther are none then scroll some more
					while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
					{
						if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONUP)
							return 0;

						GetMessage(&msg, NULL, 0, 0);
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}
			else
			{
				int col, row;
				RECT rect;
				point.y = 1;

				//find the column that the mouse is over
				if (GetCellFromPoint(&point, &col, &row, &rect) != UG_SUCCESS)
					return 0;

				//only start swapping if the mouse has moved over
				//a different column since the mouse button was pressed
				if (col != m_swapStartCol || m_swapEndCol >= 0)
				{
					if ((point.x - rect.left) > ((rect.right - rect.left) / 2))
					{
						col++;
						rect.left = rect.right;
					}

					if (col > m_GI->NumberCols())
						col = m_GI->NumberCols();

					//the firt time this is called redraw the top heading
					//so that the startswap cell is updated
					if (m_swapEndCol < 0)
					{
						m_swapEndCol = col;
						Update();
					}

					//store the current col number
					m_swapEndCol = col;

					//draw a focus rect showing where the swap will
					//take place
					//CDC* dc = m_GI->m_gridWnd->GetDC();
					//dc->DrawFocusRect(&m_focusRect);
					m_focusRect.top = 0;
					m_focusRect.bottom = m_GI->GridHeight();
					m_focusRect.left = rect.left;
					m_focusRect.right = rect.left + 2;
					//dc->DrawFocusRect(&m_focusRect);
					//m_GI->m_gridWnd->ReleaseDC(dc);
				}
			}
		}

		//#pragma NOTE("SK MOD to show custom cursor")
			// Call CUGCtrl::OnMouseMove -> calls CXeGrid::OnMouseMove to set custom cursor.
		if (!m_isSizing && !m_canSize)
		{
			int col, row;
			RECT rect;
			if (GetCellFromPoint(&point, &col, &row, &rect) != UG_SUCCESS)
				return 0;
			m_GI->OnMouseMove(col, row, &point, nFlags, FALSE);
		}
		return 0;
	}

	/************************************************
	OnLButtonDown
		Finds the cell that was clicked in, sends a notification
		updates the current cells position.
		It also sets mouse capture, which will later be used during column
		swaping operation.
	Params:
		nFlags		- please see MSDN for more information on the parameters.
		point
	Returns:
		<none>
	*************************************************/
	//void OnLButtonDown(UINT nFlags, CPoint point)
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint point) override
	{
		int col;
		int row;
		RECT rect;

		UNREFERENCED_PARAMETER(nFlags);

		if (GetFocus() != m_GI->GridWnd())
			::SetFocus(m_GI->GridWnd());

		if (m_canSize)
		{
			m_isSizing = TRUE;
			SetCapture();
		}
		else if (GetCellFromPoint(&point, &col, &row, &rect) == UG_SUCCESS)
		{
			//store the column where the button was pressed
			//just in case a swap is to take place
			if (m_GI->OnColSwapStart(col) != FALSE)
				m_swapStartCol = col;

			//send a notification to the cell type	
			BOOL processed = m_GI->GetCellTypeColRow(col, row)->OnLClicked(col, row, 1, &rect, &point);
			//send a notification to the main grid class
			m_GI->OnTH_LClicked(col, row, 1, &rect, &point, processed);
		}

		SetCapture();
		return 0;
	}

	/************************************************
	OnLButtonUp
		Finds the cell that was clicked in
		Sends a notification to CUCtrl::OnTH_LClicked
		Releases the mouse capture, and completes column swapping (if enabled)
	Params:
		nFlags		- please see MSDN for more information on the parameters.
		point
	Returns:
		<none>
	*************************************************/
	//void OnLButtonUp(UINT nFlags, CPoint point) 
	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint point) override
	{
		int col;
		int row;
		RECT rect;

		UNREFERENCED_PARAMETER(nFlags);

		if (m_isSizing) {

			if (m_colOrRowSizing == 0)	//col sizing
			{
				//send notifications
				int width = m_GI->GetColWidth(m_sizingColRow);
				// send notifications to cell types.  In order to provide acceptable performance
				// this notification will only be sent to the visible rows.
				for (int nIndex = m_GI->GetTopRow(); nIndex < m_GI->GetBottomRow(); nIndex++)
				{
					CUGCellTypeIF* pCellType = m_GI->GetCellTypeColRow(m_sizingColRow, nIndex);
					if (pCellType != NULL)
					{
						pCellType->OnChangedCellWidth(m_sizingColRow, nIndex, &width);
					}
				}
				m_GI->OnColSized(m_sizingColRow, &width);
				if (width != m_GI->GetColWidth(m_sizingColRow))
					m_GI->SetColWidth(m_sizingColRow, width, true);
			}
			else
			{
				m_GI->OnTopHdgSized(m_GI->TopHdgHeight());
			}
			m_isSizing = FALSE;

			_RedrawDirectly();
			m_GI->AdjustComponentSizes();

			m_GI->OnColRowSizeFinished();
		}
		else if (GetCellFromPoint(&point, &col, &row, &rect) == UG_SUCCESS)
		{
			//send a notification to the cell type	
			BOOL processed = m_GI->GetCellTypeColRow(col, row)->OnLClicked(col, row, 0, &rect, &point);

			bool shouldNotify = true;
			if (!m_ftxLastBestFit.IsZeroOrMax())
			{
				FILETIMEX ftxNow = FILETIMEX::UTCnow();
				FILETIMESPAN diff = ftxNow - m_ftxLastBestFit;
				if (diff < FILETIMESPAN(FT_MILLISECOND * 500))
				{
					shouldNotify = false;	// Suppress NF if last OnLButtonDblClk (best fit) less than 500mS ago.
				}
			}

			if (shouldNotify)
			{
				//send a notification to the main grid class
				m_GI->OnTH_LClicked(col, row, 0, &rect, &point, processed);
			}
		}

		//column swapping
		if (m_GI->EnableColSwapping() && m_swapStartCol >= 0)
		{
			int end = m_swapEndCol;
			if (m_swapStartCol < end) // this needs to be done since the internal
				end--;				 // calc. does not take into account that the 
			// start col will be removed and all the cols
			// are going to slide over one position
			if (m_GI->OnCanColSwap(m_swapStartCol, end) != FALSE)
			{
				m_GI->MoveColPosition(m_swapStartCol, m_swapEndCol, TRUE);

				// send OnColSwapped notification to inform user that the swap is completed
				m_GI->OnColSwapped(m_swapStartCol, end);

				//only redraw if swapping or a potential swap took place
				if (m_swapEndCol >= 0)
				{
					m_swapEndCol = -1;
					m_GI->RedrawAll();
				}
				else
				{
					_RedrawDirectly();
				}
			}
			else
			{
				m_swapEndCol = -1;
				m_GI->RedrawAll();
			}
		}

		//reset variables
		m_swapStartCol = -1;
		m_swapEndCol = -1;
		m_focusRect.left = -1;
		m_focusRect.right = -1;

		ReleaseCapture();
		return 0;
	}

	/************************************************
	OnLButtonDblClk
		Finds the cell that was clicked in and sends a notification to
		CUCtrl::OnTH_DClicked.
		If the user double clicked on the column separator area than calls
		BestFit on 20 rows starting on the top most visible row.  Once this
		operation is completed the OnChangedCellWidth, and OnColSized
		notifications will be sent.
	Params:
		<none>
	Returns:
		<none>
	*************************************************/
	//void OnLButtonDblClk(UINT nFlags, CPoint point) 
	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint point) override
	{
		int		col,
			row;
		RECT	rect;
		BOOL	processed = FALSE;

		UNREFERENCED_PARAMETER(nFlags);

		if (m_canSize)
		{
			//check to see if the column should be BestFit
			if (m_GI->UserBestSizeFlag())
			{
				m_ftxLastBestFit.SetUTCnow();
				m_GI->BestFit(m_sizingColRow, m_sizingColRow, 20, UG_BESTFIT_TOPHEADINGS);
				CheckForUserResize(&point);

				//send notifications
				int width = m_GI->GetColWidth(m_sizingColRow);
				// send notifications to cell types.  In order to provide acceptable performance
				// this notification will only be sent to the visible rows.
				for (int nIndex = m_GI->GetTopRow(); nIndex < m_GI->GetBottomRow(); nIndex++)
				{
					CUGCellTypeIF* pCellType = m_GI->GetCellTypeColRow(m_sizingColRow, nIndex);
					if (pCellType != NULL)
					{
						pCellType->OnChangedCellWidth(m_sizingColRow, nIndex, &width);
					}
				}
				m_GI->OnColSized(m_sizingColRow, &width);
				if (width != m_GI->GetColWidth(m_sizingColRow))
					m_GI->SetColWidth(m_sizingColRow, width, true);

				m_isSizing = FALSE;
			}
		}
		else if (GetCellFromPoint(&point, &col, &row, &rect) == UG_SUCCESS)
		{
			//send a notification to the cell type	
			processed = m_GI->GetCellTypeColRow(col, row)->OnDClicked(col, row, &rect, &point);
			//send a notification to the main grid class
			m_GI->OnTH_DClicked(col, row, &rect, &point, processed);
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
	Return:
		<none>
	*************************************************/
	//void OnRButtonDown(UINT nFlags, CPoint point) 
	virtual LRESULT _OnRightDown(UINT nFlags, CPoint point) override
	{
		int col;
		int row;
		RECT rect;

		UNREFERENCED_PARAMETER(nFlags);

		if (GetFocus() != m_GI->GridWnd())
			::SetFocus(m_GI->GridWnd());

		if (GetCellFromPoint(&point, &col, &row, &rect) == UG_SUCCESS)
		{
			//send a notification to the cell type	
			BOOL processed = m_GI->GetCellTypeColRow(col, row)->OnRClicked(col, row, 0, &rect, &point);
			//send a notification to the main grid class
			m_GI->OnTH_RClicked(col, row, 1, &rect, &point, processed);
		}

		if (m_GI->IsEnablePopupMenu())
		{
			ClientToScreen(&point);
			m_GI->StartMenu(col, row, point, UG_TOPHEADING);
		}
		return 0;
	}

	/************************************************
	OnRButtonUp
		Sends a mouse click notification to the main grid class.
	Params:
		nFlags		- mouse button states
		point		- point where the mouse is
	Return:
		<none>
	*************************************************/
	//void OnRButtonUp(UINT nFlags, CPoint point) 
	virtual LRESULT _OnRightUp(UINT nFlags, CPoint point) override
	{
		int col;
		int row;
		RECT rect;

		UNREFERENCED_PARAMETER(nFlags);

		if (GetCellFromPoint(&point, &col, &row, &rect) == UG_SUCCESS)
		{
			//send a notification to the cell type	
			BOOL processed = m_GI->GetCellTypeColRow(col, row)->OnRClicked(col, row, 0, &rect, &point);
			//send a notification to the main grid class
			m_GI->OnTH_RClicked(col, row, 0, &rect, &point, processed);
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
	//BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam) override
	{
		//UNREFERENCED_PARAMETER(*pWnd);
		//UNREFERENCED_PARAMETER(nHitTest);
		//UNREFERENCED_PARAMETER(message);

		if (!m_canSize)
		{
			SetCursor(m_GI->GetDefaultCursor()/*m_GI->m_arrowCursor*/);
			return 1;
		}
		else if (m_colOrRowSizing == 0)
			SetCursor(m_GI->WEResizseCursor());
		else
			SetCursor(m_GI->NSResizseCursor());

		return 1;

	}

public:
	/************************************************
	PreCreateWindow
		The Utlimate Grid overwrites this function to further customize the
		top heading window that is created.
	Params:
		cs			- please see MSDN for more information on the parameters.
	Returns:
		Nonzero if the window creation should continue; 0 to indicate creation failure.
	*************************************************/
	//BOOL PreCreateWindow(CREATESTRUCT& cs) 
	//{
	//	cs.style = cs.style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	//
	//	return CWnd::PreCreateWindow(cs);
	//}

	/************************************************
	GetCellRect
		Returns the rectangle for the given cell co-ordinates.
		If the cell is joined then the given co-ordinates
		are modified to point to the start cell for the join.
	Params
		col		- column to find the rect of
		row		- row to find the rect of
		rect	- rectangle to calculate
	Return
		UG_SUCCESS	- success
	*************************************************/
	int GetCellRect(int col, long row, RECT* rect)
	{
		return GetCellRect(&col, &row, rect);
	}
	/************************************************
	GetCellRect
		Returns the rectangle for the given cell co-ordinates.
		If the cell is joined then the given co-ordinates
		are modified to point to the start cell for the join.
	Params
		col		- column to find the rect of
		row		- row to find the rect of
		rect	- rectangle to calculate
	Return
		UG_SUCCESS	- success, this function will never fail
	*************************************************/
	int GetCellRect(int* col, long* row, RECT* rect)
	{
		int xIndex, yIndex;
		int width = 0;
		int height = 0;

		int startCol = *col;
		int startRow = *row;
		int endCol = *col;
		int endRow = *row;

		rect->left = 0;
		rect->top = 0;
		rect->right = 0;
		rect->bottom = m_GI->TopHdgHeight();

		//if the specified cell is within a join then find the joined range
		if (m_GI->EnableJoins())
		{
			if (GetJoinRange(&startCol, &startRow, &endCol, &endRow) == UG_SUCCESS)
			{
				*col = startCol;
				*row = startRow;
			}
		}

		//find the col
		if (startCol >= m_GI->NumLockCols())	//if the col is not within the lock region
		{
			rect->left = m_GI->LockColWidth();
			rect->right = m_GI->LockColWidth();
		}

		for (xIndex = 0; xIndex < m_GI->NumberCols(); xIndex++)
		{
			if (xIndex == m_GI->NumLockCols())
				xIndex = m_GI->LeftCol();

			if (xIndex == startCol)
				rect->left = width;

			width += m_GI->GetColWidth(xIndex);

			if (xIndex == endCol)
			{
				rect->right = width;
				break;
			}
		}

		//find the row
		for (yIndex = (m_GI->NumberTopHdgRows() * -1); yIndex < 0; yIndex++)
		{
			if (yIndex == startRow)
				rect->top = height;

			height += GetTHRowHeight(yIndex);

			if (yIndex == endRow)
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
	Return:
		UG_SUCCESS	- if a matching cell was found
		UG_ERROR	- point provided does not correspond to a cell
	*************************************************/
	int GetCellFromPoint(CPoint* point, int* col, int* row, RECT* rect)
	{
		int ptsFound = 0;
		int xIndex, yIndex;

		rect->left = 0;
		rect->top = 0;
		rect->right = 0;
		rect->bottom = 0;

		//find the col
		for (xIndex = 0; xIndex < m_GI->NumberCols(); xIndex++)
		{
			if (xIndex == m_GI->NumLockCols())
				xIndex = m_GI->LeftCol();

			rect->right += m_GI->GetColWidth(xIndex);

			if (rect->right > point->x)
			{
				rect->left = rect->right - m_GI->GetColWidth(xIndex);
				ptsFound++;
				*col = xIndex;
				break;
			}
		}

		//find the row
		for (yIndex = -m_GI->NumberTopHdgRows(); yIndex < 0; yIndex++)
		{
			rect->bottom += GetTHRowHeight(yIndex);

			if (rect->bottom > point->y)
			{
				rect->top = rect->bottom - GetTHRowHeight(yIndex);
				ptsFound++;
				*row = yIndex;
				break;
			}
		}

		if (ptsFound == 2)
		{
			// check if the found cell is part of a join, if
			// it is than adjust the information found
			int edCol, edRow;

			if (GetJoinRange(col, row, &edCol, &edRow) == UG_SUCCESS)
			{
				long tempRow = *row;
				GetCellRect(col, &tempRow, rect);
				*row = tempRow;
			}

			return UG_SUCCESS;
		}

		*col = -1;
		*row = -1;

		return UG_ERROR;
	}

	/***************************************************
	GetJoinRange
		function returns join information related to the cell specified
		in the col and row parameters.  These parameters might be changed
		to represent starting col, row position of the join.
	Params:
		col, row	- identify the cell to retrieve join information on,
					  these parameters will represent the starting pos
					  of the join.
		endCol,		- will be set to the end col, row position of the
		endRow		  join.
	Returns:
		UG_SUCCESS	- on success
		UG_ERROR	- if joins are disabled, or cell specified is not part of a join
	*****************************************************/
	int GetJoinRange(int* col, int* row, int* endCol, int* endRow)
	{
		if (m_GI->EnableJoins() == FALSE)
			return UG_ERROR;

		int startCol;
		long startRow, joinRow;
		BOOL origin;
		CUGCell cell;

		m_GI->GetCellIndirect(*col, *row, &cell);
		if (cell.IsPropertySet(UGCELL_JOIN_SET) == FALSE)
			return UG_ERROR;

		cell.GetJoinInfo(&origin, &startCol, &startRow);
		if (!origin)
		{
			*col += startCol;
			*row += (int)startRow;
			m_GI->GetCellIndirect(*col, *row, &cell);
		}

		cell.GetJoinInfo(&origin, endCol, &joinRow);
		*endCol += *col;
		*endRow = joinRow + *row;

		return UG_SUCCESS;
	}

	/***************************************************
	GetTHRowHeight
		function returns height (in pixels) of the top heading row that user is
		interested in.  The row number passed in has to be a negaitve value ranging
		from zero to negative value of number of top heading rows set.
		((m_GI->m_numberTopHdgRows) * -1)
	Params:
		row			- indicates row of interest
	Returns:
		height in pixels of the row in question, or zero if
		row not found.
	*****************************************************/
	int GetTHRowHeight(int row)
	{
		//translate the row number into a 0 based positive index
		row = (row * -1) - 1;

		if (row <0 || row > m_GI->NumberTopHdgRows())
			return 0;

		return m_GI->GetTopHdgRowHeight(row);
	}

	/************************************************
	OnHelpHitTest
		Sent as a result of context sensitive help
		being activated (with mouse) over top heading
	Params:
		WPARAM - not used
		LPARAM - x, y coordinates of the mouse event
	Return:
		Context help ID to be displayed
	*************************************************/
	//LRESULT OnHelpHitTest(WPARAM, LPARAM lParam)
	//{
	//	// return context help ID to be looked up
	//	return 0;
	//}

	/************************************************
	OnHelpInfo
		Sent as a result of context sensitive help
		being activated (with mouse) over top heading
		if the grid is on the dialog
	Params:
		HELPINFO - structure that contains information on selected help topic
	Return:
		TRUE or FALSE to allow further processing of this message
	*************************************************/
	//BOOL OnHelpInfo(HELPINFO* pHelpInfo) 
	//{
	//	return FALSE;
	//}

	/***************************************************
	OnToolHitTest
		The framework calls this member function to detemine whether a point is in
		the bounding rectangle of the specified tool. If the point is in the
		rectangle, it retrieves information about the tool.
	Params:
		point	- please see MSDN for more information on the parameters.
		pTI
	Returns:
		If 1, the tooltip control was found; If -1, the tooltip control was not found.
	*****************************************************/
	//INT_PTR OnToolHitTest(  CPoint point, TOOLINFO *pTI ) const
	//{
	//	int col;
	//	long row;
	//	CRect rect;
	//	static int lastCol = -2;
	//	static int lastRow = -2;
	//
	//	if(m_GI->GetCellFromPoint(point.x, point.y, &col, &row, &rect ) == UG_SUCCESS)
	//	{
	//		if(col != lastCol || row != lastRow)
	//		{
	//			lastCol = col;
	//			lastRow = row;
	//			CancelToolTips();
	//			return -1;
	//		}
	//
	//		pTI->cbSize = sizeof(TOOLINFO);
	//		pTI->uFlags =  TTF_NOTBUTTON | TTF_ALWAYSTIP |TTF_IDISHWND ;
	//#pragma warning(disable:4311)
	//#pragma warning(disable:4302)
	//		pTI->uId = (UINT)m_hWnd;
	//#pragma warning(default:4311)
	//#pragma warning(default:4302)
	//		pTI->hwnd = (HWND)m_hWnd;
	//		pTI->lpszText = LPSTR_TEXTCALLBACK;
	//		return 1;
	//	}
	//	return -1;
	//}

	/***************************************************
	ToolTipNeedText (TTN_NEEDTEXT)
		message handler will be called whenever a text is required for a tool tip.
		The Ultimate Grid uses the CUGCtrl::OnHint virtual function to obtain
		proper text to be displayed in the too tip.
	Params:
			id			- Identifier of the control that sent the notification. Not
						  used. The control id is taken from the NMHDR structure.
			pTTTStruct	- A pointer to theNMTTDISPINFO structure. This structure is
						  also discussed further in The TOOLTIPTEXT Structure.
			pResult		- A pointer to result code you can set before you return.
						  TTN_NEEDTEXT handlers can ignore the pResult parameter.
	Returns:
		<none>
	*****************************************************/
	BOOL ToolTipNeedText(UINT id, NMHDR* pTTTStruct, LRESULT* pResult)
	{
		UNREFERENCED_PARAMETER(id);
		UNREFERENCED_PARAMETER(*pResult);

		TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pTTTStruct;

		//static CString string;
		int col;
		int row;
		CPoint point;
		CRect rect;

		GetCursorPos(&point);
		ScreenToClient(&point);

		if (GetCellFromPoint(&point, &col, &row, &rect) == UG_SUCCESS)
		{
			TOOLTIP_SETTINGS ttSettings;
			if (m_GI->OnHint(col, row, UG_TOPHEADING, ttSettings) == TRUE)
			{
				pTTT->lpszText = const_cast<LPTSTR>((LPCTSTR)ttSettings.m_strTooltipText.c_str());
				return TRUE;
			}
		}

		return FALSE;
	}
};

