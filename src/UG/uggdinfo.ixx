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
#include "../os_minimal.h"
#include <functional>
//#include "UGDtaSrc.h"
//#include "UGCell.h"
//#include "UGMemMan.h"
//#include "UGMultiS.h"
//#include "ugptrlst.h"
//#include "UGCelTyp.h"
//#include "UGDrwHnt.h"
//#include "..\XSuperTooltip.h"
#include "ugdefine.h"

export module Xe.UGGridInfo;

//import Xe.XSuperTooltip;
import Xe.UIcolorsIF;
import Xe.UGGridInfoIF;
import Xe.UGDtaSrc;
import Xe.UGCell;
import Xe.UGCelTyp;
import Xe.UGDrawHint;
import Xe.UGCell;
import Xe.UGDtaSrc;
import Xe.UGMem;
import Xe.UGMultiSelect;
import Xe.UGPtrList;

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
export typedef std::function<int(int col, long row, CPoint point, int section)> fnStartMenu;
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
export typedef std::function<int(int height)> fnOnTopHdgSized;
export typedef std::function<int(int width)> fnOnSideHdgSized;
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


//export struct UGCOLINFO
//{
//	int				width;
//	CUGDataSource* dataSource;
//	CUGCell* colDefault;
//	int				colTranslation;
//
//};
//class CXeUIcolorsIF;

export class CUGGridInfo : public CUGGridInfoIF
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
	virtual HCURSOR GetDefaultCursor() const override
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

	int GetColWidth(int col, int* width) const
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

	virtual int GetColWidth(int col) const override
	{
		int w;
		if (GetColWidth(col, &w) == UG_SUCCESS)
		{
			return w;
		}
		return 0;
	}

	virtual int GetNumberCols() const override { return m_numberCols; }
	virtual int	GetCurrentCol() const override { return m_currentCol; }
	virtual long GetCurrentRow() const override { return m_currentRow; }
	virtual int	GetLeftCol() const override { return m_leftCol; }
	virtual int	GetRightCol() const override { return m_rightCol; }
	virtual long GetTopRow() const override { return m_topRow; }
	virtual long GetBottomRow() const override { return m_bottomRow; }

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

#pragma region _fn_pointers_Set_by_UGCtrl
	// Callback functions into UGCtrl
	fnOnCellTypeNotify _fn_OnCellTypeNotify = nullptr;
	fnGetCellIndirect _fn_GetCellIndirect = nullptr;
	fnGetCellType _fn_GetCellType = nullptr;
	fnGetCellTypeColRow _fn_GetCellTypeColRow = nullptr;
	fnGetRowHeight _fn_GetRowHeight = nullptr;
	fnGetNonUniformRowHeight _fn_GetNonUniformRowHeight = nullptr;
	fnSetCell _fn_SetCell = nullptr;
	fnRedrawAll _fn_RedrawAll = nullptr;
	fnRedrawCell _fn_RedrawCell = nullptr;
	fnGetCellRect _fn_GetCellRect = nullptr;
	fnGetRangeRect _fn_GetRangeRect = nullptr;
	fnOnKillFocusNewWnd _fn_OnKillFocusNewWnd = nullptr;
	fnOnSetFocus _fn_OnSetFocus = nullptr;
	fnOnKillFocus _fn_OnKillFocus = nullptr;
	fnEditCtrlFinished _fn_EditCtrlFinished = nullptr;
	fnGotoCell _fn_GotoCell = nullptr;
	fnGotoCol _fn_GotoCol = nullptr;
	fnGotoRow _fn_GotoRow = nullptr;
	fnOnEditVerify _fn_OnEditVerify = nullptr;
	fnGetCellFromPointColRow _fn_GetCellFromPointColRow = nullptr;
	fnGetCellFromPoint _fn_GetCellFromPoint = nullptr;
	fnOnSortEvaluate _fn_OnSortEvaluate = nullptr;
	fnGetNumberRows _fn_GetNumberRows = nullptr;
	fnAdjustComponentSizes _fn_AdjustComponentSizes = nullptr;
	fnStartMenu _fn_StartMenu = nullptr;
	fnOnLClicked _fn_OnLClicked = nullptr;
	fnOnRClicked _fn_OnRClicked = nullptr;
	fnOnDClicked _fn_OnDClicked = nullptr;
	fnOnMouseMove _fn_OnMouseMove = nullptr;
	fnOnTH_LClicked _fn_OnTH_LClicked = nullptr;
	fnOnTH_RClicked _fn_OnTH_RClicked = nullptr;
	fnOnTH_DClicked _fn_OnTH_DClicked = nullptr;
	fnOnSH_LClicked _fn_OnSH_LClicked = nullptr;
	fnOnSH_RClicked _fn_OnSH_RClicked = nullptr;
	fnOnSH_DClicked _fn_OnSH_DClicked = nullptr;
	fnOnCB_LClicked _fn_OnCB_LClicked = nullptr;
	fnOnCB_RClicked _fn_OnCB_RClicked = nullptr;
	fnOnCB_DClicked _fn_OnCB_DClicked = nullptr;
	fnOnKeyDown _fn_OnKeyDown = nullptr;
	fnOnKeyUp _fn_OnKeyUp = nullptr;
	fnOnCharDown _fn_OnCharDown = nullptr;
	fnOnCanSizeCol _fn_OnCanSizeCol = nullptr;
	fnOnColSizing _fn_OnColSizing = nullptr;
	fnOnColSized _fn_OnColSized = nullptr;
	fnOnCanSizeRow _fn_OnCanSizeRow = nullptr;
	fnOnRowSizing _fn_OnRowSizing = nullptr;
	fnOnRowSized _fn_OnRowSized = nullptr;
	fnOnCanSizeTopHdg _fn_OnCanSizeTopHdg = nullptr;
	fnOnCanSizeSideHdg _fn_OnCanSizeSideHdg = nullptr;
	fnOnTopHdgSizing _fn_OnTopHdgSizing = nullptr;
	fnOnSideHdgSizing _fn_OnSideHdgSizing = nullptr;
	fnOnTopHdgSized _fn_OnTopHdgSized = nullptr;
	fnOnSideHdgSized _fn_OnSideHdgSized = nullptr;
	fnOnColRowSizeFinished _fn_OnColRowSizeFinished = nullptr;
	fnSetTH_Height _fn_SetTH_Height = nullptr;
	fnSetSH_Width _fn_SetSH_Width = nullptr;
	fnOnHint _fn_OnHint = nullptr;
	fnMoveTopRow _fn_MoveTopRow = nullptr;
	fnSetTopRow _fn_SetTopRow = nullptr;
	fnMoveCurrentRow _fn_MoveCurrentRow = nullptr;
	fnSetLeftCol _fn_SetLeftCol = nullptr;
	fnMoveLeftCol _fn_MoveLeftCol = nullptr;
	fnMoveCurrentCol _fn_MoveCurrentCol = nullptr;
	fnOnViewMoved _fn_OnViewMoved = nullptr;
	fnGetJoinStartCell _fn_GetJoinStartCell = nullptr;
	fnGetJoinRange _fn_GetJoinRange = nullptr;
	fnSetColWidth _fn_SetColWidth = nullptr;
	fnVerifyCurrentRow _fn_VerifyCurrentRow = nullptr;
	fnBestFit _fn_BestFit = nullptr;
	fnSetTH_RowHeight _fn_SetTH_RowHeight = nullptr;
	fnSetSH_ColWidth _fn_SetSH_ColWidth = nullptr;
	fnHideTooltip _fn_HideTooltip = nullptr;	// note hide all child wnd tooltips
	fnMakeSuperTooltip _fn_MakeSuperTooltip = nullptr;
	fnOnColSwapStart _fn_OnColSwapStart = nullptr;
	fnOnCanColSwap _fn_OnCanColSwap = nullptr;
	fnOnColSwapped _fn_OnColSwapped = nullptr;
	fnMoveColPosition _fn_MoveColPosition = nullptr;
	fnSetRowHeight _fn_SetRowHeight = nullptr;
	fnHScroll _fn_HScroll = nullptr;

	virtual int GetJoinStartCellColRow(int* col, long* row) override
	{
		return _fn_GetJoinStartCell(col, row, &m_cell);
	}
#pragma endregion _fn_pointers_Set_by_UGCtrl

#pragma region ImplCUGGridInfoIF_fn_
	virtual int OnCellTypeNotify(long ID, int col, long row, long msg, long long param)
	{
		return _fn_OnCellTypeNotify(ID, col, row, msg, param);
	}
	virtual int GetCellIndirect(int col, long row, CUGCell* cell)
	{
		return _fn_GetCellIndirect(col, row, cell);
	}
	virtual CUGCellTypeIF* GetCellType(int type)
	{
		return _fn_GetCellType(type);
	}
	virtual CUGCellTypeIF* GetCellTypeColRow(int col, long row)
	{
		return _fn_GetCellTypeColRow(col, row);
	}
	virtual int GetRowHeight(long row)
	{
		return _fn_GetRowHeight(row);
	}
	virtual int GetNonUniformRowHeight(long row)
	{
		return _fn_GetNonUniformRowHeight(row);
	}
	virtual int SetCell(int col, long row, CUGCell* cell)
	{
		return _fn_SetCell(col, row, cell);
	}
	virtual int RedrawAll()
	{
		return _fn_RedrawAll();
	}
	virtual int RedrawCell(int col, long row)
	{
		return _fn_RedrawCell(col, row);
	}
	virtual int GetCellRect(int col, long row, RECT* rect)
	{
		return _fn_GetCellRect(col, row, rect);
	}
	virtual int GetRangeRect(int startCol, long startRow, int endCol, long endRow, RECT* rect)
	{
		return _fn_GetRangeRect(startCol, startRow, endCol, endRow, rect);
	}
	virtual void OnKillFocusNewWnd(int section, HWND hNewWnd)
	{
		return _fn_OnKillFocusNewWnd(section, hNewWnd);
	}
	virtual void OnSetFocus(int section)
	{
		return _fn_OnSetFocus(section);
	}
	virtual void OnKillFocus(int section)
	{
		return _fn_OnKillFocus(section);
	}
	virtual int EditCtrlFinished(LPCTSTR string, BOOL cancelFlag, BOOL continueFlag, int continueCol, long continueRow)
	{
		return _fn_EditCtrlFinished(string, cancelFlag, continueFlag, continueCol, continueRow);
	}
	virtual int GotoCell(int col, long row)
	{
		return _fn_GotoCell(col, row);
	}
	virtual int GotoCol(int col)
	{
		return _fn_GotoCol(col);
	}
	virtual int GotoRow(long row)
	{
		return _fn_GotoRow(row);
	}
	virtual int OnEditVerify(int col, long row, HWND edit, UINT* vcKey)
	{
		return _fn_OnEditVerify(col, row, edit, vcKey);
	}
	virtual int GetCellFromPointColRow(int x, int y, int* col, long* row)
	{
		return _fn_GetCellFromPointColRow(x, y, col, row);
	}
	virtual int GetCellFromPoint(int x, int y, int* ptcol, long* ptrow, RECT* rect)
	{
		return _fn_GetCellFromPoint(x, y, ptcol, ptrow, rect);
	}
	virtual int OnSortEvaluate(CUGCell* cell1, CUGCell* cell2, int flags)
	{
		return _fn_OnSortEvaluate(cell1, cell2, flags);
	}
	virtual long GetNumberRows()
	{
		return _fn_GetNumberRows();
	}
	virtual void AdjustComponentSizes()
	{
		_fn_AdjustComponentSizes();
	}
	virtual int StartMenu(int col, long row, CPoint point, int section)
	{
		return _fn_StartMenu(col, row, point, section);
	}
	virtual void OnLClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnLClicked(col, row, updn, rect, point, processed);
	}
	virtual void OnRClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnRClicked(col, row, updn, rect, point, processed);
	}
	virtual void OnDClicked(int col, long row, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnDClicked(col, row, rect, point, processed);
	}
	virtual void OnMouseMove(int col, long row, POINT* point, UINT nFlags, BOOL processed)
	{
		_fn_OnMouseMove(col, row, point, nFlags, processed);
	}
	virtual void OnTH_LClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnTH_LClicked(col, row, updn, rect, point, processed);
	}
	virtual void OnTH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnTH_RClicked(col, row, updn, rect, point, processed);
	}
	virtual void OnTH_DClicked(int col, long row, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnTH_DClicked(col, row, rect, point, processed);
	}
	virtual void OnSH_LClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnSH_LClicked(col, row, updn, rect, point, processed);
	}
	virtual void OnSH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnSH_RClicked(col, row, updn, rect, point, processed);
	}
	virtual void OnSH_DClicked(int col, long row, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnSH_DClicked(col, row, rect, point, processed);
	}
	virtual void OnCB_LClicked(int updn, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnCB_LClicked(updn, rect, point, processed);
	}
	virtual void OnCB_RClicked(int updn, RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnCB_RClicked(updn, rect, point, processed);
	}
	virtual void OnCB_DClicked(RECT* rect, POINT* point, BOOL processed)
	{
		_fn_OnCB_DClicked(rect, point, processed);
	}
	virtual void OnKeyDown(UINT* vcKey, BOOL processed)
	{
		_fn_OnKeyDown(vcKey, processed);
	}
	virtual void OnKeyUp(UINT* vcKey, BOOL processed)
	{
		_fn_OnKeyUp(vcKey, processed);
	}
	virtual void OnCharDown(UINT* vcKey, BOOL processed)
	{
		_fn_OnCharDown(vcKey, processed);
	}
	virtual int OnCanSizeCol(int col)
	{
		return _fn_OnCanSizeCol(col);
	}
	virtual void OnColSizing(int col, int* width)
	{
		_fn_OnColSizing(col, width);
	}
	virtual void OnColSized(int col, int* width)
	{
		_fn_OnColSized(col, width);
	}
	virtual int OnCanSizeRow(long row)
	{
		return _fn_OnCanSizeRow(row);
	}
	virtual void OnRowSizing(long row, int* height)
	{
		_fn_OnRowSizing(row, height);
	}
	virtual void OnRowSized(long row, int* height)
	{
		_fn_OnRowSized(row, height);
	}
	virtual int OnCanSizeTopHdg()
	{
		return _fn_OnCanSizeTopHdg();
	}
	virtual int OnCanSizeSideHdg()
	{
		return _fn_OnCanSizeSideHdg();
	}
	virtual int OnTopHdgSizing(int* height)
	{
		return _fn_OnTopHdgSizing(height);
	}
	virtual int OnSideHdgSizing(int* width)
	{
		return _fn_OnSideHdgSizing(width);
	}
	virtual int OnTopHdgSized(int height)
	{
		return _fn_OnTopHdgSized(height);
	}
	virtual int OnSideHdgSized(int width)
	{
		return _fn_OnSideHdgSized(width);
	}
	virtual void OnColRowSizeFinished()
	{
		_fn_OnColRowSizeFinished();
	}
	virtual int SetTH_Height(int height)
	{
		return _fn_SetTH_Height(height);
	}
	virtual int SetSH_Width(int width)
	{
		return _fn_SetSH_Width(width);
	}
	virtual int OnHint(int col, long row, int section, TOOLTIP_SETTINGS& ttSettings)
	{
		return _fn_OnHint(col, row, section, ttSettings);
	}
	virtual int MoveTopRow(int flag)
	{
		return _fn_MoveTopRow(flag);
	}
	virtual int SetTopRow(long row)
	{
		return _fn_SetTopRow(row);
	}
	virtual int MoveCurrentRow(int flag)
	{
		return _fn_MoveCurrentRow(flag);
	}
	virtual int SetLeftCol(int col)
	{
		return _fn_SetLeftCol(col);
	}
	virtual int MoveLeftCol(int flag)
	{
		return _fn_MoveLeftCol(flag);
	}
	virtual int MoveCurrentCol(int flag)
	{
		return _fn_MoveCurrentCol(flag);
	}
	virtual void OnViewMoved(int nScrolDir, long oldPos, long newPos)
	{
		_fn_OnViewMoved(nScrolDir, oldPos, newPos);
	}
	virtual int GetJoinStartCell(int* col, long* row, CUGCell* cell)
	{
		return _fn_GetJoinStartCell(col, row, cell);
	}
	virtual int GetJoinRange(int* col, long* row, int* col2, long* row2)
	{
		return _fn_GetJoinRange(col, row, col2, row2);
	}
	virtual int SetColWidth(int col, int width, bool notify)
	{
		return _fn_SetColWidth(col, width, notify);
	}
	virtual int VerifyCurrentRow(long* newRow)
	{
		return _fn_VerifyCurrentRow(newRow);
	}
	virtual int BestFit(int startCol, int endCol, int CalcRange, int flag)
	{
		return _fn_BestFit(startCol, endCol, CalcRange, flag);
	}
	virtual int SetTH_RowHeight(int row, int height)
	{
		return _fn_SetTH_RowHeight(row, height);
	}
	virtual int SetSH_ColWidth(int col, int width)
	{
		return _fn_SetSH_ColWidth(col, width);
	}
	virtual LRESULT MakeSuperTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT, HWND hWnd, int section)
	{
		return _fn_MakeSuperTooltip(pNeedTT, hWnd, section);
	}
	virtual void HideTooltip()
	{
		_fn_HideTooltip();
	}
	virtual BOOL OnColSwapStart(int col)
	{
		return _fn_OnColSwapStart(col);
	}
	virtual BOOL OnCanColSwap(int fromCol, int toCol)
	{
		return _fn_OnCanColSwap(fromCol, toCol);
	}
	virtual void OnColSwapped(int fromCol, int toCol)
	{
		return _fn_OnColSwapped(fromCol, toCol);
	}
	virtual int MoveColPosition(int fromCol, int toCol, BOOL insertBefore)
	{
		return _fn_MoveColPosition(fromCol, toCol, insertBefore);
	}
	virtual int SetRowHeight(long row, int height)
	{
		return _fn_SetRowHeight(row, height);
	}
	virtual void HScroll(UINT nSBCode, UINT nPos)
	{
		_fn_HScroll(nSBCode, nPos);
	}
#pragma endregion ImplCUGGridInfoIF_fn_

#pragma region GettersSetters
public:
	virtual BOOL CancelMode() const					override { return m_bCancelMode; }
	virtual BOOL Extend() const						override { return m_bExtend; }
	virtual int BallisticDelay() const				override { return m_ballisticDelay; }
	virtual int BallisticMode() const				override { return m_ballisticMode; }
	virtual int BallisticKeyDelay() const			override { return m_ballisticKeyDelay; }
	virtual int BallisticKeyMode() const			override { return m_ballisticKeyMode; }
	virtual void SetBottomRow(long row)				override {		  m_bottomRow = row; }
	virtual long BottomRow() const					override { return m_bottomRow; }
	virtual int CurrentCol() const					override { return m_currentCol; }
	virtual long CurrentRow() const					override { return m_currentRow; }
	virtual int CurrentCellMode() const				override { return m_currentCellMode; }
	virtual int DefColWidth() const					override { return m_defColWidth; }
	virtual int DefRowHeight() const				override { return m_defRowHeight; }
	virtual void SetDefRowHeight(int cy)			override {		  m_defRowHeight = cy; }
	virtual CUGDataSource* DefDataSource() const	override { return m_defDataSource; }
	virtual CUGCell& EditCell()						override { return m_editCell; }
	virtual void SetDragCol(int col)				override {		  m_dragCol = col; }
	virtual void SetDragRow(long row)				override {		  m_dragCol = row; }
	virtual int DragCol() const						override { return m_dragCol; }
	virtual long DragRow() const					override { return m_dragRow; }
	virtual int EditCol() const						override { return m_editCol; }
	virtual bool EditInProgress() const				override { return m_editInProgress; }
	virtual long EditRow() const					override { return m_editRow; }
	virtual int EnableColSwapping() const			override { return m_enableColSwapping; }
	virtual int EnableJoins() const					override { return m_enableJoins; }
	virtual int EnableExcelBorders() const			override { return m_enableExcelBorders; }
	virtual void EnablePopupMenu(bool enable)		override {		  m_enablePopupMenu = enable; }
	virtual bool IsEnablePopupMenu() const			override { return m_enablePopupMenu; }
	virtual int GridHeight() const					override { return m_gridHeight; }
	virtual int GridWidth() const					override { return m_gridWidth; }
	virtual HWND GridWnd() const					override { return m_gridWnd; }
	virtual HWND CtrlWnd() const					override { return m_ctrlWnd; }
	virtual int HScrollMode() const					override { return m_hScrollMode; }
	virtual CRect HScrollRect() const				override { return m_hScrollRect; }
	virtual int HighlightRowFlag() const			override { return m_highlightRowFlag; }
	virtual int LastLeftCol() const					override { return m_lastLeftCol; }
	virtual long LastTopRow() const					override { return m_lastTopRow; }
	virtual int LeftCol() const						override { return m_leftCol; }
	virtual int RightCol() const					override { return m_rightCol; }
	virtual void SetRightCol(int col)				override {		  m_rightCol = col; }
	virtual int LockColWidth() const				override { return m_lockColWidth; }
	virtual int LockRowHeight() const				override { return m_lockRowHeight; }
	virtual int MaxLeftCol() const					override { return m_maxLeftCol; }
	virtual long MaxTopRow() const					override { return m_maxTopRow; }
	virtual UINT MoveFlags() const					override { return m_moveFlags; }
	virtual void SetMoveType(int type)				override {		  m_moveType = type; }
	virtual void SetMoveFlags(UINT flags)			override {		  m_moveFlags = flags; }
	virtual int MultiSelectFlag() const				override { return m_multiSelectFlag; }
	virtual int NumLockCols() const					override { return m_numLockCols; }
	virtual int NumLockRows() const					override { return m_numLockRows; }
	virtual int NumberCols() const					override { return m_numberCols; }
	virtual long NumberRows() const					override { return m_numberRows; }
	virtual int NumberSideHdgCols() const			override { return m_numberSideHdgCols; }
	virtual int NumberTopHdgRows() const			override { return m_numberTopHdgRows; }
	virtual bool PaintMode() const					override { return m_paintMode; }
	virtual bool ShowHScroll() const				override { return m_showHScroll; }
	virtual BOOL ScrollOnPartialCells() const		override { return m_bScrollOnParialCells; }
	virtual int SideHdgWidth() const				override { return m_sideHdgWidth; }
	virtual void SetSideHdgWidth(int cx)			override {		  m_sideHdgWidth = cx; }
	virtual void SetTopHdgHeight(int cy)			override {		  m_topHdgHeight = cy; }
	virtual int TopHdgHeight() const				override { return m_topHdgHeight; }
	virtual long TopRow() const						override { return m_topRow; }
	virtual int ThreeDHeight() const				override { return m_threeDHeight; }
	virtual int UniformRowHeightFlag() const		override { return m_uniformRowHeightFlag; }
	virtual int UserBestSizeFlag() const			override { return m_userBestSizeFlag; }
	virtual int UserSizingMode() const				override { return m_userSizingMode; }
	virtual int VScrollMode() const					override { return m_vScrollMode; }
	virtual HCURSOR NSResizseCursor() const			override { return m_NSResizseCursor; }
	virtual HCURSOR WEResizseCursor() const			override { return m_WEResizseCursor; }
	virtual CXeUIcolorsIF* GetXeUI()				override { return m_xeUI; }

	virtual UGCOLINFO& GetColInfo(int colIdx) const override
	{
		return m_colInfo[colIdx];
	}
	virtual int GetSideHdgColWidth(int colIdx) const override
	{
		return m_sideHdgWidths[colIdx];
	}
	virtual void SetSideHdgColWidth(int colIdx, int cx) override
	{
		m_sideHdgWidths[colIdx] = cx;
	}
	virtual int GetTopHdgRowHeight(int rowIdx) const override
	{
		return m_topHdgHeights[rowIdx];
	}
	virtual void SetTopHdgRowHeight(int rowIdx, int cy) override
	{
		m_topHdgHeights[rowIdx] = cy;
	}
	virtual int IsSelected(int col, long row, int* block = nullptr) const override
	{
		return m_multiSelect->IsSelected(col, row, block);
	}
#pragma endregion GettersSetters
};
