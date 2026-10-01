module;

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
#include <functional>
//#include "UGDtaSrc.h"
//#include "UGCell.h"
//#include "UGMemMan.h"
//#include "UGMultiS.h"
//#include "ugptrlst.h"
//#include "UGCelTyp.h"
//#include "UGDrwHnt.h"
//#include "XeGridDefs.h"

export module Xe.UGGridInfo;
//#include "..\XSuperTooltip.h"
//import Xe.XSuperTooltip;
//import Xe.UIcolorsIF;
import Xe.UGDtaSrc;
import Xe.UGCell;

export typedef std::function<int(long ID, int col, long row, long msg, long long param)> fnOnCellTypeNotify;
export typedef std::function<int(int col, long row, CUGCell* cell)> fnGetCellIndirect;
export typedef std::function<CUGCellType* (int type)> fnGetCellType;
export typedef std::function<CUGCellType* (int col, long row)> fnGetCellTypeColRow;
export typedef std::function<int(long row)> fnGetRowHeight;
export typedef std::function<int(long row)> fnGetNonUniformRowHeight;
export typedef std::function<int(int col, long row, CUGCell* cell)> fnSetCell;
export typedef std::function<int()> fnRedrawAll;
export typedef std::function<int(int col, long row)> fnRedrawCell;
export typedef std::function<int(int col, long row, RECT* rect)> fnGetCellRect;
export typedef std::function<int(int startCol, long startRow, int endCol, long endRow, RECT* rect)> fnGetRangeRect;
export typedef std::function<void(int section, HWND hNewWnd)> fnOnKillFocusNewWnd;
export typedef std::function<void(int section)> fnOnSetFocus;
export typedef std::function<void(int section)> fnOnKillFocus;
export typedef std::function<int(LPCTSTR string, BOOL cancelFlag, BOOL continueFlag, int continueCol, long continueRow)> fnEditCtrlFinished;
export typedef std::function<int(int col, long row)> fnGotoCell;
export typedef std::function<int(int col)> fnGotoCol;
export typedef std::function<int(long row)> fnGotoRow;
export typedef std::function<int(int col, long row, HWND edit, UINT* vcKey)> fnOnEditVerify;
export typedef std::function<int(int x, int y, int* col, long* row)> fnGetCellFromPointColRow;
export typedef std::function<int(int x, int y, int* ptcol, long* ptrow, RECT* rect)> fnGetCellFromPoint;
export typedef std::function<int(CUGCell* cell1, CUGCell* cell2, int flags)> fnOnSortEvaluate;
//export typedef std::function<void(CDC* dc, CDC* db_dc, int section)> fnOnScreenDCSetup;
export typedef std::function<long()> fnGetNumberRows;
export typedef std::function<void()> fnAdjustComponentSizes;
export typedef std::function<int(int col, long row, POINT* point, int section)> fnStartMenu;
//export typedef std::function<()> fn;
//export typedef std::function<()> fn;
//export typedef std::function<()> fn;
export typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnLClicked;
export typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnRClicked;
export typedef std::function<void(int col, long row, RECT* rect, POINT* point, BOOL processed)> fnOnDClicked;
export typedef std::function<void(int col, long row, POINT* point, UINT nFlags, BOOL processed)> fnOnMouseMove;
export typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnTH_LClicked;
export typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnTH_RClicked;
export typedef std::function<void(int col, long row, RECT* rect, POINT* point, BOOL processed)> fnOnTH_DClicked;
export typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnSH_LClicked;
export typedef std::function<void(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)> fnOnSH_RClicked;
export typedef std::function<void(int col, long row, RECT* rect, POINT* point, BOOL processed)> fnOnSH_DClicked;
export typedef std::function<void(int updn, RECT* rect, POINT* point, BOOL processed)> fnOnCB_LClicked;
export typedef std::function<void(int updn, RECT* rect, POINT* point, BOOL processed)> fnOnCB_RClicked;
export typedef std::function<void(RECT* rect, POINT* point, BOOL processed)> fnOnCB_DClicked;
export typedef std::function<void(UINT* vcKey, BOOL processed)> fnOnKeyDown;
export typedef std::function<void(UINT* vcKey, BOOL processed)> fnOnKeyUp;
export typedef std::function<void(UINT* vcKey, BOOL processed)> fnOnCharDown;
export typedef std::function<int(int col)> fnOnCanSizeCol;
export typedef std::function<void(int col, int* width)> fnOnColSizing;
export typedef std::function<void(int col, int* width)> fnOnColSized;
export typedef std::function<int(long row)> fnOnCanSizeRow;
export typedef std::function<void(long row, int* height)> fnOnRowSizing;
export typedef std::function<void(long row, int* height)> fnOnRowSized;
export typedef std::function<int()> fnOnCanSizeTopHdg;
export typedef std::function<int()> fnOnCanSizeSideHdg;
export typedef std::function<int(int* height)> fnOnTopHdgSizing;
export typedef std::function<int(int* width)> fnOnSideHdgSizing;
export typedef std::function<int(int* height)> fnOnTopHdgSized;
export typedef std::function<int(int* width)> fnOnSideHdgSized;
export typedef std::function<void()> fnOnColRowSizeFinished;
export typedef std::function<int(int height)> fnSetTH_Height;
export typedef std::function<int(int width)> fnSetSH_Width;
export typedef std::function<int(int col, long row, int section, TOOLTIP_SETTINGS& ttSettings)> fnOnHint;
export typedef std::function<int(int flag)> fnMoveTopRow;
export typedef std::function<int(long row)> fnSetTopRow;
export typedef std::function<int(int flag)> fnMoveCurrentRow;
export typedef std::function<int(int col)> fnSetLeftCol;
export typedef std::function<int(int flag)> fnMoveLeftCol;
export typedef std::function<int(int flag)> fnMoveCurrentCol;
//export typedef std::function<()> fn;
export typedef std::function<void(int nScrolDir, long oldPos, long newPos)> fnOnViewMoved;
export typedef std::function<int(int* col, long* row, CUGCell* cell)> fnGetJoinStartCell;
export typedef std::function<int(int* col, long* row, int* col2, long* row2)> fnGetJoinRange;
export typedef std::function<int(int col, int width, bool notify)> fnSetColWidth;
export typedef std::function<int(long* newRow)> fnVerifyCurrentRow;
//export typedef std::function<void(CDC* dc, RECT* rect)> fnOnDrawFocusRect;
export typedef std::function<int(int startCol, int endCol, int CalcRange, int flag)> fnBestFit;
export typedef std::function<int(int row, int height)> fnSetTH_RowHeight;
export typedef std::function<int(int col, int width)> fnSetSH_ColWidth;
//export typedef std::function<LRESULT(NM_PPTOOLTIP_NEED_TT* pNeedTT, CXSuperTooltip& xtooltip, HWND hWnd, int section)> fnMakeSuperTooltip;
export typedef std::function<LRESULT(NM_PPTOOLTIP_NEED_TT* pNeedTT, HWND hWnd, int section)> fnMakeSuperTooltip;
export typedef std::function<void()> fnHideTooltip;
export typedef std::function<BOOL(int col)> fnOnColSwapStart;
export typedef std::function<BOOL(int fromCol, int toCol)> fnOnCanColSwap;
export typedef std::function<void(int fromCol, int toCol)> fnOnColSwapped;
export typedef std::function<int(int fromCol, int toCol, BOOL insertBefore)> fnMoveColPosition;
export typedef std::function<int(long row, int height)> fnSetRowHeight;
export typedef std::function<void(UINT nSBCode, UINT nPos)> fnHScroll;


export struct UGCOLINFO
{
	int				width;
	CUGDataSource* dataSource;
	CUGCell* colDefault;
	int				colTranslation;

};
//class CXeUIcolorsIF;

export class CUGGridInfo //: public CObject
{
public:
	CUGGridInfo()
	{
		//column information
		m_numberCols = 0;
		m_currentCol = -1;
		m_lastCol = -1;
		m_leftCol = 0;
		m_lastLeftCol = 0;
		m_maxLeftCol = 0;
		m_defColWidth = 75;
		m_colInfo = NULL;
		m_rightCol = 0;
		m_dragCol = -1;

		//row information
		m_numberRows = 0;
		m_currentRow = -1;
		m_lastRow = -1;
		m_topRow = 0;
		m_lastTopRow = 0;
		m_maxTopRow = 0;
		//m_rowHeights		= NULL;
		m_defRowHeight = 20;
		m_uniformRowHeightFlag = FALSE;
		m_bottomRow = 0;
		m_dragRow = -1;

		//heading information
		m_numberTopHdgRows = 1;
		m_topHdgHeights = new int[1];
		m_topHdgHeights[0] = 20;

		m_numberSideHdgCols = 1;
		m_sideHdgWidths = new int[1];
		m_sideHdgWidths[0] = 25;

		//defaults
		m_gridDefaults = new CUGCell;
		m_hdgDefaults = new CUGCell;
		m_hdgDefaults->SetBackColor(GetSysColor(COLOR_BTNFACE));
		m_hdgDefaults->SetBorder(UG_BDR_RAISED);
		m_hdgDefaults->SetAlignment(UG_ALIGNCENTER | UG_ALIGNVCENTER);

		//sizes
		m_topHdgHeight = 20;		//pixels
		m_sideHdgWidth = 40;		//pixels
		m_vScrollWidth = GetSystemMetrics(SM_CXVSCROLL);		//pixels
		m_hScrollHeight = GetSystemMetrics(SM_CYHSCROLL);		//pixels
		m_tabWidth = 0;		//pixels
		m_showHScroll = TRUE;		//TRUE or FALSE
		m_showVScroll = TRUE;		//TRUE or FALSE
		m_gridHeight = 0;		//pixels

		//highlighting
		m_highlightRowFlag = FALSE;	//TRUE or False
		m_multiSelectFlag = FALSE;	//TRUE or False
		m_currentCellMode = 1;		//mode(bits) 1:focus rect 2:highlight

		//other options
		m_mouseScrollFlag = TRUE;		//TRUE or FALSE
		m_threeDHeight = 1;		// 1 - n
		m_paintMode = FALSE;		//if false then do not paint
		m_enablePopupMenu = FALSE;	//TRUE or FALSE

		// hints
		m_enableHints = FALSE;	//TRUE or FALSE
		m_enableVScrollHints = FALSE;	//TRUE or FALSE
		m_enableHScrollHints = FALSE;	//TRUE or FALSE

		m_userSizingMode = 1;		//0 -off 1-normal 2-update on the fly
		m_userBestSizeFlag = TRUE;

		m_enableJoins = TRUE;
		//m_enableCellOverLap		= TRUE;
		m_enableColSwapping = FALSE;
		m_enableExcelBorders = TRUE;

		//scrollbars
		m_vScrollMode = 0;		// UG_SCROLLNORMAL, or UG_SCROLLTRACKING, or UG_SCROLLJOYSTICK
		m_hScrollMode = 0;		// UG_SCROLLNORMAL, or UG_SCROLLTRACKING

		// enable scrolling on parially visible cells
		m_bScrollOnParialCells = TRUE;

		//balistic 
		m_ballisticMode = 0;		//0- off 1-increment 2-squared 3- cubed
		m_ballisticDelay = 200;		//slow scroll delay
		m_ballisticKeyMode = 0;		//0- off
		m_ballisticKeyDelay = 0;		//slow scroll delay

		//column and row locking
		m_numLockCols = 0;
		m_numLockRows = 0;
		m_lockColWidth = 0;
		m_lockRowHeight = 0;

		//zooming multiplication factor
		m_zoomMultiplier = 0;
		m_zoomOn = FALSE;

		// default data source
		m_CUGMem = new CUGMem;
		m_defDataSource = m_CUGMem;
		m_defDataSourceIndex = 0;

		m_moveType = 0;  //keyboard - default
		m_multiSelect = new CUGMultiSelect;

		// mouse cursors
		//m_arrowCursor		= LoadCursor(NULL,IDC_ARROW);
		m_WEResizseCursor = LoadCursor(NULL, IDC_SIZEWE);
		m_NSResizseCursor = LoadCursor(NULL, IDC_SIZENS);

		m_margin = 4;
		m_CUGOverlay = new CUGPtrList;

		m_trackingWndMode = 0;	//normal mode 1-stay close

		m_bCancelMode = FALSE;

		m_bExtend = TRUE;
	}
	virtual ~CUGGridInfo()
	{
		if (m_colInfo != NULL)
		{
			for (int loop = 0; loop < m_numberCols; loop++)
			{
				if (m_colInfo[loop].colDefault != NULL)
					delete m_colInfo[loop].colDefault;
			}
			delete[] m_colInfo;
		}

		//if(m_rowHeights	!= NULL)
		//	delete[] m_rowHeights;
		if (m_topHdgHeights != NULL)
			delete[] m_topHdgHeights;
		if (m_sideHdgWidths != NULL)
			delete[] m_sideHdgWidths;
		if (m_gridDefaults != NULL)
			delete m_gridDefaults;
		if (m_hdgDefaults != NULL)
			delete m_hdgDefaults;
		if (m_multiSelect != NULL)
			delete m_multiSelect;
		if (m_CUGOverlay != NULL)
			delete m_CUGOverlay;
		if (m_CUGMem != NULL)
			delete m_CUGMem;
	}

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

	UGCOLINFO* m_colInfo;

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
	int* m_topHdgHeights;

	int		m_numberSideHdgCols;
	int* m_sideHdgWidths;

	//defaults
	CUGCell* m_gridDefaults;
	CUGCell* m_hdgDefaults;

	//current cell
	CUGCell* m_currentCell;
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
	CUGDataSource* m_defDataSource;
	CUGMem* m_CUGMem;


	//movement type 0-keyboard 1-lbutton 2-rbutton 3-mousemove
	int		m_moveType;
	//flags - if moved by mouse
	UINT	m_moveFlags;


	//multi-select
	CUGMultiSelect* m_multiSelect;

	//cursors
	// Get the 'app' cursor from s_xeUI
	HCURSOR GetDefaultCursor()
	{
		return m_xeUI->GetAppCursor();
	}
	//HCURSOR m_arrowCursor;
	HCURSOR m_WEResizseCursor;
	HCURSOR m_NSResizseCursor;

	//margins for drawing text in cells
	int m_margin;

	//overlay objects
	CUGPtrList* m_CUGOverlay;

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
