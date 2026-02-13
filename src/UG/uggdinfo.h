/*************************************************************************
				Class Declaration : CUGGridInfo
**************************************************************************
	Source file : uggdinfo.cpp
	Header file : uggdinfoh
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		The CUGGridInfo class contains setup information
		about each sheet in the grid.  All of the memeber
		variables of this class are set externally by the
		CUGCtrl class.
	Details
		The CUGCtrl class holds an array of these
		classes where each sheet has one entry.
*************************************************************************/
#ifndef _uggdinfo_H_
#define _uggdinfo_H_

#include <functional>
#include "UGDtaSrc.h"
#include "UGCell.h"
#include "UGMemMan.h"
#include "UGMultiS.h"
#include "ugptrlst.h"
#include "UGCelTyp.h"
#include "UGDrwHnt.h"
#include "XeGridDefs.h"
//#include "..\XSuperTooltip.h"
//import Xe.XSuperTooltip;
import Xe.UIcolorsIF;

typedef std::function<int(long ID, int col, long row, long msg, long long param)> fnOnCellTypeNotify;
typedef std::function<int(int col, long row, CUGCell* cell)> fnGetCellIndirect;
typedef std::function<CUGCellType*(int type)> fnGetCellType;
typedef std::function<CUGCellType*(int col, long row)> fnGetCellTypeColRow;
typedef std::function<int(long row)> fnGetRowHeight;
typedef std::function<int(long row)> fnGetNonUniformRowHeight;
typedef std::function<int(int col,long row,CUGCell *cell)> fnSetCell;
typedef std::function<int()> fnRedrawAll;
typedef std::function<int(int col, long row)> fnRedrawCell;
typedef std::function<int(int col, long row, RECT* rect)> fnGetCellRect;
typedef std::function<int(int startCol, long startRow, int endCol, long endRow, RECT* rect)> fnGetRangeRect;
typedef std::function<void(int section, HWND hNewWnd)> fnOnKillFocusNewWnd;
typedef std::function<void(int section)> fnOnSetFocus;
typedef std::function<void(int section)> fnOnKillFocus;
typedef std::function<int(LPCTSTR string, BOOL cancelFlag, BOOL continueFlag, int continueCol, long continueRow)> fnEditCtrlFinished;
typedef std::function<int(int col, long row)> fnGotoCell;
typedef std::function<int(int col)> fnGotoCol;
typedef std::function<int(long row)> fnGotoRow;
typedef std::function<int(int col, long row, HWND edit, UINT* vcKey)> fnOnEditVerify;
typedef std::function<int(int x, int y, int* col, long* row)> fnGetCellFromPointColRow;
typedef std::function<int(int x, int y, int* ptcol, long* ptrow, RECT* rect)> fnGetCellFromPoint;
typedef std::function<int(CUGCell* cell1, CUGCell* cell2, int flags)> fnOnSortEvaluate;
//typedef std::function<void(CDC* dc, CDC* db_dc, int section)> fnOnScreenDCSetup;
typedef std::function<long()> fnGetNumberRows;
typedef std::function<void()> fnAdjustComponentSizes;
typedef std::function<int(int col, long row, POINT* point, int section)> fnStartMenu;
//typedef std::function<()> fn;
//typedef std::function<()> fn;
//typedef std::function<()> fn;
typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnLClicked;
typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnRClicked;
typedef std::function<void(int col, long row, RECT* rect, POINT* point, BOOL processed)> fnOnDClicked;
typedef std::function<void(int col, long row, POINT* point, UINT nFlags, BOOL processed)> fnOnMouseMove;
typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnTH_LClicked;
typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnTH_RClicked;
typedef std::function<void(int col, long row, RECT* rect, POINT* point, BOOL processed)> fnOnTH_DClicked;
typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnSH_LClicked;
typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnSH_RClicked;
typedef std::function<void(int col, long row, RECT* rect, POINT* point, BOOL processed)> fnOnSH_DClicked;
typedef std::function<void(int updn, RECT* rect, POINT* point, BOOL processed)> fnOnCB_LClicked;
typedef std::function<void(int updn, RECT* rect, POINT* point, BOOL processed)> fnOnCB_RClicked;
typedef std::function<void(RECT* rect, POINT* point, BOOL processed)> fnOnCB_DClicked;
typedef std::function<void(UINT* vcKey, BOOL processed)> fnOnKeyDown;
typedef std::function<void(UINT* vcKey, BOOL processed)> fnOnKeyUp;
typedef std::function<void(UINT* vcKey, BOOL processed)> fnOnCharDown;
typedef std::function<int(int col)> fnOnCanSizeCol;
typedef std::function<void(int col, int* width)> fnOnColSizing;
typedef std::function<void(int col, int* width)> fnOnColSized;
typedef std::function<int(long row)> fnOnCanSizeRow;
typedef std::function<void(long row, int* height)> fnOnRowSizing;
typedef std::function<void(long row, int* height)> fnOnRowSized;
typedef std::function<int()> fnOnCanSizeTopHdg;
typedef std::function<int()> fnOnCanSizeSideHdg;
typedef std::function<int(int* height)> fnOnTopHdgSizing;
typedef std::function<int(int* width)> fnOnSideHdgSizing;
typedef std::function<int(int* height)> fnOnTopHdgSized;
typedef std::function<int(int* width)> fnOnSideHdgSized;
typedef std::function<void()> fnOnColRowSizeFinished;
typedef std::function<int(int height)> fnSetTH_Height;
typedef std::function<int(int width)> fnSetSH_Width;
typedef std::function<int(int col, long row, int section, TOOLTIP_SETTINGS& ttSettings)> fnOnHint;
typedef std::function<int(int flag)> fnMoveTopRow;
typedef std::function<int(long row)> fnSetTopRow;
typedef std::function<int(int flag)> fnMoveCurrentRow;
typedef std::function<int(int col)> fnSetLeftCol;
typedef std::function<int(int flag)> fnMoveLeftCol;
typedef std::function<int(int flag)> fnMoveCurrentCol;
//typedef std::function<()> fn;
typedef std::function<void(int nScrolDir, long oldPos, long newPos)> fnOnViewMoved;
typedef std::function<int(int* col, long* row, CUGCell* cell)> fnGetJoinStartCell;
typedef std::function<int(int* col, long* row, int* col2, long* row2)> fnGetJoinRange;
typedef std::function<int(int col, int width, bool notify)> fnSetColWidth;
typedef std::function<int(long* newRow)> fnVerifyCurrentRow;
//typedef std::function<void(CDC* dc, RECT* rect)> fnOnDrawFocusRect;
typedef std::function<int(int startCol, int endCol, int CalcRange, int flag)> fnBestFit;
typedef std::function<int(int row, int height)> fnSetTH_RowHeight;
typedef std::function<int(int col, int width)> fnSetSH_ColWidth;
//typedef std::function<LRESULT(NM_PPTOOLTIP_NEED_TT* pNeedTT, CXSuperTooltip& xtooltip, HWND hWnd, int section)> fnMakeSuperTooltip;
typedef std::function<LRESULT(NM_PPTOOLTIP_NEED_TT* pNeedTT, CXeTooltipIF* xtooltip, HWND hWnd, int section)> fnMakeSuperTooltip;
typedef std::function<void()> fnHideTooltip;
typedef std::function<BOOL(int col)> fnOnColSwapStart;
typedef std::function<BOOL(int fromCol, int toCol)> fnOnCanColSwap;
typedef std::function<void(int fromCol, int toCol)> fnOnColSwapped;
typedef std::function<int(int fromCol, int toCol, BOOL insertBefore)> fnMoveColPosition;
typedef std::function<int(long row, int height)> fnSetRowHeight;
typedef std::function<void(UINT nSBCode, UINT nPos)> fnHScroll;


typedef struct _UGCOLINFO
{
	int				width;
	CUGDataSource *	dataSource;
	CUGCell	*		colDefault;
	int				colTranslation;

}UGCOLINFO;
class CXeUIcolorsIF;

class  CUGGridInfo //: public CObject
{
public:
	CUGGridInfo();
	virtual ~CUGGridInfo();

	CXeUIcolorsIF* m_xeUI = nullptr;

	//column info
	int		m_numberCols;
	int		m_currentCol;
	int		m_lastCol;
	int		m_leftCol;
	int		m_lastLeftCol;
	int		m_maxLeftCol;
	int		m_defColWidth;
	int		m_rightCol;
	int		m_dragCol;

	UGCOLINFO*	m_colInfo;

	//row info
	long	m_numberRows;
	long	m_currentRow;
	long	m_lastRow;
	long	m_topRow;
	long	m_lastTopRow;
	long	m_maxTopRow;
	//int *	m_rowHeights;
	int		m_defRowHeight;
	int		m_uniformRowHeightFlag;	//true or false
	long	m_bottomRow;
	long	m_dragRow;

	//headings
	int		m_numberTopHdgRows;
	int		*m_topHdgHeights;

	int		m_numberSideHdgCols;
	int		*m_sideHdgWidths;

	//defaults
	CUGCell *	m_gridDefaults;
	CUGCell *	m_hdgDefaults;

	//current cell
	CUGCell	*	m_currentCell;
	CUGCell		m_cell;			//general purpose cell object
		
	//sizes
	int m_topHdgHeight;		//pixels
	int m_sideHdgWidth;		//pixels
	int m_vScrollWidth;		//pixels
	int m_hScrollHeight;	//pixels
	int m_tabWidth;			//pixels

	int m_showHScroll;		//TRUE or FALSE
	int m_showVScroll;		//TRUE or FALSE

	int m_gridWidth;		//calcualated using the values above
	int m_gridHeight;

	CRect	m_gridRect;		//calcualated using the values above
	CRect	m_topHdgRect;
	CRect	m_sideHdgRect;
	CRect	m_cnrBtnRect;
	CRect	m_tabRect;
	CRect	m_vScrollRect;
	CRect	m_hScrollRect;

	//highlighting
	int		m_highlightRowFlag;		// TRUE or False
	int		m_multiSelectFlag;		// Multiselect mode
	int		m_currentCellMode;		// mode(bits) 1:focus rect 2:highlight
	BOOL	m_showFocusRect;
	BOOL	m_highLightCurrentCell;


	//other options
	int		m_mouseScrollFlag;		//TRUE or FALSE

	int		m_threeDHeight;			// 1 - n

	int		m_paintMode;			//if false then do not paint

	int		m_enablePopupMenu;		//TRUE or FALSE

	int		m_enableHints;			//TRUE or FALSE
	int		m_enableVScrollHints;	//TRUE or FALSE
	int		m_enableHScrollHints;	//TRUE or FALSE

	int		m_userSizingMode;		//0 -off 1-normal 2-update on the fly
	int		m_userBestSizeFlag;		//TRUE or FALSE

	int		m_enableJoins;			//TRUE or FALSE

	int		m_enableColSwapping;	//TRUE or FALSE

	//int		m_enableCellOverLap;	//TRUE or FALSE

	int		m_enableExcelBorders;	//TRUE or FALSE

	//scrollbars
	int		m_vScrollMode;			// 0-normal 2- tracking 3-joystick
	int		m_hScrollMode;			// 0-normal 2- tracking 

	// Scroling on partially visible cells
	BOOL	m_bScrollOnParialCells;

	//balistic 
	int		m_ballisticMode;		//0- off 1-increment 2-squared 3- cubed
	int		m_ballisticDelay;		//slow scroll delay
	int		m_ballisticKeyMode;		//0- off n - number of key repeats for speed
									//increase
	int		m_ballisticKeyDelay;	//slow scroll delay
	
	//column and row locking
	int		m_numLockCols;
	int		m_numLockRows;
	int		m_lockColWidth;
	int		m_lockRowHeight;

	
	//zooming multiplication factor
	float	m_zoomMultiplier;
	BOOL	m_zoomOn;

	//allow cells to be partially visible when moved into
	BOOL m_noPartlyVisible; //TRUE = must be visible, FALSE= may not be 

	int				m_defDataSourceIndex;
	CUGDataSource*	m_defDataSource;
	CUGMem*			m_CUGMem;	


	//movement type 0-keyboard 1-lbutton 2-rbutton 3-mousemove
	int		m_moveType;		
	//flags - if moved by mouse
	UINT	m_moveFlags;


	//multi-select
	CUGMultiSelect		* m_multiSelect;

	//cursors
	// Get the 'app' cursor from s_xeUI
	HCURSOR GetDefaultCursor();
	//HCURSOR m_arrowCursor;
	HCURSOR m_WEResizseCursor;
	HCURSOR m_NSResizseCursor;

	//margins for drawing text in cells
	int m_margin;	

	//overlay objects
	CUGPtrList	*m_CUGOverlay;

	int m_trackingWndMode; // 0-normal 1-stay close

	BOOL m_bCancelMode;

	BOOL m_bExtend;

	HWND m_ctrlWnd = nullptr;
	HWND m_gridWnd = nullptr;
	HWND m_sideHdgWnd = nullptr;

	//CUGDrawHint		m_drawHintGrid;		//grid cell drawing hints

	//editing
	BOOL	m_editInProgress = FALSE;		//TRUE or FALSE
	long	m_editRow = -1;
	int		m_editCol = -1;
	HWND m_editCtrl = nullptr;				//edit control currently being used
	//CWnd* m_maskedEditCtrl = nullptr;
	CUGCell m_editCell;
	HWND m_editParent = nullptr;

	//tab sizing flag
	BOOL m_tabSizing = FALSE;

	//BOOL m_findDialogRunning = FALSE;
	//BOOL m_findDialogStarted = FALSE;
	//BOOL m_findInAllCols = TRUE;

	int GetColWidth(int col, int* width)
	{
		if (col >= m_numberCols)
			return UG_ERROR;

		if (col < 0)
		{	// side heading column
			//translate the col number into a 0 based positive index
			col = (col * -1) - 1;

			if (col >= m_numberSideHdgCols)
				return UG_ERROR;

			*width = m_sideHdgWidths[col];
		}
		else
		{	// grid column
			*width = m_colInfo[col].width;
		}
		return UG_SUCCESS;
	}

	int GetColWidth(int col)
	{
		int w;
		if (GetColWidth(col, &w) == UG_SUCCESS)
		{
			return w;
		}
		return 0;
	}

	int GetNumberCols() { return m_numberCols; }

	int	GetCurrentCol() { return m_currentCol; }
	long GetCurrentRow() { return m_currentRow; }
	int	GetLeftCol() { return m_leftCol; }
	int	GetRightCol() { return m_rightCol; }
	long GetTopRow() { return m_topRow; }
	long GetBottomRow() { return m_bottomRow; }

	int	SetTH_HeightValue(int height) {

		if (height < 0 || height >1024)
			return UG_ERROR;

		m_topHdgHeight = height;

		//adjust the height for each top heading row
		int totalHeight = 0;
		int loop;
		double adjust;
		//find the total old height
		for (loop = 0; loop < m_numberTopHdgRows; loop++) {
			totalHeight += m_topHdgHeights[loop];
		}
		//find the adjustment value
		adjust = (double)height / (double)totalHeight;
		//adjust each row height
		for (loop = 0; loop < m_numberTopHdgRows; loop++) {
			m_topHdgHeights[loop] = (int)(m_topHdgHeights[loop] * adjust + 0.5);
		}
		return UG_SUCCESS;
	}

	// Callback functions into UGCtrl
	fnOnCellTypeNotify OnCellTypeNotify = nullptr;
	fnGetCellIndirect GetCellIndirect = nullptr;
	fnGetCellType GetCellType = nullptr;
	fnGetCellTypeColRow GetCellTypeColRow = nullptr;
	fnGetRowHeight GetRowHeight = nullptr;
	fnGetNonUniformRowHeight GetNonUniformRowHeight = nullptr;
	fnSetCell SetCell = nullptr;
	fnRedrawAll RedrawAll = nullptr;
	fnRedrawCell RedrawCell = nullptr;
	fnGetCellRect GetCellRect = nullptr;
	fnGetRangeRect GetRangeRect = nullptr;
	fnOnKillFocusNewWnd OnKillFocusNewWnd = nullptr;
	fnOnSetFocus OnSetFocus = nullptr;
	fnOnKillFocus OnKillFocus = nullptr;
	fnEditCtrlFinished EditCtrlFinished = nullptr;
	fnGotoCell GotoCell = nullptr;
	fnGotoCol GotoCol = nullptr;
	fnGotoRow GotoRow = nullptr;
	fnOnEditVerify OnEditVerify = nullptr;
	fnGetCellFromPointColRow GetCellFromPointColRow = nullptr;
	fnGetCellFromPoint GetCellFromPoint = nullptr;
	fnOnSortEvaluate OnSortEvaluate = nullptr;
	//fnOnScreenDCSetup OnScreenDCSetup = nullptr;
	fnGetNumberRows GetNumberRows = nullptr;
	fnAdjustComponentSizes AdjustComponentSizes = nullptr;
	fnStartMenu StartMenu = nullptr;
	fnOnLClicked OnLClicked = nullptr;
	fnOnRClicked OnRClicked = nullptr;
	fnOnDClicked OnDClicked = nullptr;
	fnOnMouseMove OnMouseMove = nullptr;
	fnOnTH_LClicked OnTH_LClicked = nullptr;
	fnOnTH_RClicked OnTH_RClicked = nullptr;
	fnOnTH_DClicked OnTH_DClicked = nullptr;
	fnOnSH_LClicked OnSH_LClicked = nullptr;
	fnOnSH_RClicked OnSH_RClicked = nullptr;
	fnOnSH_DClicked OnSH_DClicked = nullptr;
	fnOnCB_LClicked OnCB_LClicked = nullptr;
	fnOnCB_RClicked OnCB_RClicked = nullptr;
	fnOnCB_DClicked OnCB_DClicked = nullptr;
	fnOnKeyDown OnKeyDown = nullptr;
	fnOnKeyUp OnKeyUp = nullptr;
	fnOnCharDown OnCharDown = nullptr;
	fnOnCanSizeCol OnCanSizeCol = nullptr;
	fnOnColSizing OnColSizing = nullptr;
	fnOnColSized OnColSized = nullptr;
	fnOnCanSizeRow OnCanSizeRow = nullptr;
	fnOnRowSizing OnRowSizing = nullptr;
	fnOnRowSized OnRowSized = nullptr;
	fnOnCanSizeTopHdg OnCanSizeTopHdg = nullptr;
	fnOnCanSizeSideHdg OnCanSizeSideHdg = nullptr;
	fnOnTopHdgSizing OnTopHdgSizing = nullptr;
	fnOnSideHdgSizing OnSideHdgSizing = nullptr;
	fnOnTopHdgSized OnTopHdgSized = nullptr;
	fnOnSideHdgSized OnSideHdgSized = nullptr;
	fnOnColRowSizeFinished OnColRowSizeFinished = nullptr;
	fnSetTH_Height SetTH_Height = nullptr;
	fnSetSH_Width SetSH_Width = nullptr;
	fnOnHint OnHint = nullptr;
	fnMoveTopRow MoveTopRow = nullptr;
	fnSetTopRow SetTopRow = nullptr;
	fnMoveCurrentRow MoveCurrentRow = nullptr;
	fnSetLeftCol SetLeftCol = nullptr;
	fnMoveLeftCol MoveLeftCol = nullptr;
	fnMoveCurrentCol MoveCurrentCol = nullptr;
	fnOnViewMoved OnViewMoved = nullptr;
	fnGetJoinStartCell GetJoinStartCell = nullptr;
	int GetJoinStartCellColRow(int* col, long* row)
	{
		return GetJoinStartCell(col, row, &m_cell);
	}
	fnGetJoinRange GetJoinRange = nullptr;
	fnSetColWidth SetColWidth = nullptr;
	fnVerifyCurrentRow VerifyCurrentRow = nullptr;
	//fnOnDrawFocusRect OnDrawFocusRect = nullptr;
	fnBestFit BestFit = nullptr;
	fnSetTH_RowHeight SetTH_RowHeight = nullptr;
	fnSetSH_ColWidth SetSH_ColWidth = nullptr;
	fnHideTooltip HideTooltip = nullptr;	// note hide all child wnd tooltips
	fnMakeSuperTooltip MakeSuperTooltip = nullptr;
	fnOnColSwapStart OnColSwapStart = nullptr;
	fnOnCanColSwap OnCanColSwap = nullptr;
	fnOnColSwapped OnColSwapped = nullptr;
	fnMoveColPosition MoveColPosition = nullptr;
	fnSetRowHeight SetRowHeight = nullptr;
	fnHScroll HScroll = nullptr;
};

#endif // _uggdinfo_H_