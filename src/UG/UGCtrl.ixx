module;

/*************************************************************************
				Class Implementation : CUGCtrl
**************************************************************************
	Source file : UGCtrl.cpp
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved
*************************************************************************/

#include "..\os_minimal.h"
//#include <afxext.h>         // MFC extensions
//#include <afxole.h>			// Needed for drag and drop support in Grid
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <d2d1.h>
#include <dwrite.h>
#include <tchar.h>
//namespace Gdiplus
//{
//	using std::min;
//	using std::max;
//}
//#include <gdiplus.h>

import Xe.UIcolorsIF;
#include "ugdefine.h"
#include "UGDtaSrc.h"
#include "ugptrlst.h"
#include "UGCell.h"
#include "UGCelTyp.h"
#include "ugdltype.h"
#include "ugcbtype.h"
#include "ugctarrw.h"
#include "UGCTprogress.h"
//#include "UGEditBase.h"
#include "UGEdit.h"
//#include "UGMEdit.h"
#include "UGMemMan.h"
#include "UGDrwHnt.h"
#include "UGMultiS.h"
#include "uggdinfo.h"
#include "XeGridDefs.h"
//#include "..\PPTooltip.h"
//#include "..\XSuperTooltip.h"
#include "UGGrid.h"
#include "UGCell.h"
#include "UGTopHdg.h"
#include "ugvscrol.h"
#include "ughscrol.h"
//#include "UGCnrBtn.h"
//#include "ugtab.h"
//#include "UGHint.h"
#include "ugsidehd.h"
#include "ugformat.h"
#include "UGCTsarw.h"
#include "UGCTurlbtn.h"
//#ifdef UG_ENABLE_PRINTING
//#undef UG_ENABLE_PRINTING
//#endif
#pragma warning(disable:5202)
import Xe.Helpers;
//import Xe.HelpersMFC;

#include <vector>
import Xe.GridDataSource;
import Xe.ColorPicker;
import Xe.UserSettings;
import Xe.StringTools;

#include "..\GridDefs.h"

#include "UGCtrl.h"

//#include "..\CustomWndMsgs.h"

#define ID_EDIT_FIND                    0xE124

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif
#pragma warning(default:5202)

#pragma warning(disable:5201)
#pragma warning(disable:4091)

export module Xe.Grid;

export CUGCtrl;

#pragma warning(default:4091)
#pragma warning(default:5201)

module :private;

CUGCtrl::CUGCtrl(CXeGridDataSource* pDS, const wchar_t* strRegSectionName,
	GridNotifyCallbackFunc gridNotifyCallback /*= nullptr*/) : CXeD2DCtrlBase(pDS->m_xeUI)
{
	XeASSERT(pDS && pDS->m_xeUI);
	m_xeUI = pDS->m_xeUI;
	m_strRegSectionName = strRegSectionName;

	m_pInitialDataSource = pDS;

	m_GridNotifyCallback = gridNotifyCallback;

	m_dropListType = std::make_unique<CUGDropListType>(pDS->m_xeUI);

	//m_ppenWvBorder = new CPen(PS_SOLID, 1, RGB(128, 128, 192));

	m_fCancelNext_OnEditContinue = FALSE;

	//m_pDropSource = new CXeOleDropSource;

	m_contructorResults = UG_SUCCESS;

	m_menu = std::make_unique<CXeMenu>(m_xeUI);
	m_menu->SetUpdateMenuCallback(
		[this](CXeMenu* pMenu, size_t top_level_index) { _UpdateMenuCallback(pMenu, top_level_index); });

	/********************************************
	set up the internal classes
	*********************************************/
	try {
		//data source
		m_dataSrcList = NULL;
		m_dataSrcListLength = 0;
		m_dataSrcListMaxLength = 0;

		//m_fontList =			new CUGPtrList;
		//m_bitmapList = new CUGPtrList;
		m_cellTypeList = new CUGPtrList;
		m_cellStylesList = new CUGPtrList;
		m_validateList = new CUGPtrList;
		m_displayFormatList = new CUGPtrList;
		m_CUGGrid = new CUGGrid(pDS->m_xeUI);
		m_CUGTopHdg = new CUGTopHdg(pDS->m_xeUI);
		m_CUGSideHdg = new CUGSideHdg(pDS->m_xeUI);
		m_CUGVScroll = new CUGVScroll(pDS->m_xeUI);
		m_CUGHScroll = new CUGHScroll(pDS->m_xeUI);
		//m_CUGCnrBtn = new CUGCnrBtn;
		//m_CUGTab = new CUGTab;

		//scroll hint window
//#ifdef UG_ENABLE_SCROLLHINTS
//		m_CUGHint = new CUGHint;
//#endif

//#ifdef UG_ENABLE_PRINTING
//		m_CUGPrint = new CUGPrint;
//#endif
	}
	catch (...) {
		m_contructorResults = 1;
		return;
	}

	//tacking window
	//m_trackingWnd = NULL;

	//Set up the 1 sheet and get the GridInfo class pointer
	//to that sheet
	m_currentSheet = 0;
	m_numberSheets = 0;
	m_GIList = NULL;
	m_GI = NULL;
	SetNumberSheets(1);
	m_GI = m_GIList[0];

	m_CUGGrid->m_GI = m_GI;
	m_CUGTopHdg->m_GI = m_GI;
	m_CUGSideHdg->m_GI = m_GI;
	//m_CUGCnrBtn->m_GI = m_GI;
	m_CUGVScroll->m_GI = m_GI;
	m_CUGHScroll->m_GI = m_GI;
	//m_CUGTab->m_GI = m_GI;

//#ifdef UG_ENABLE_SCROLLHINTS
//	//m_CUGHint->m_ctrl		= this;
//	m_CUGHint->m_GI = m_GI;
//#endif

	//#ifdef UG_ENABLE_PRINTING
	//	m_CUGPrint->m_ctrl	= this;
	//	m_CUGPrint->m_GI	= m_GI;
	//#endif

	//#ifdef UG_ENABLE_DRAGDROP
	//m_dropTarget.m_ctrl = this;
	//#endif
	//m_defEditCtrl.m_ctrl	= this;
	//m_defMaskedEditCtrl.m_ctrl	= this;
	m_defEditCtrl.m_GI = m_GI;
	//m_defMaskedEditCtrl.m_GI = m_GI;

	m_GI->m_editInProgress = FALSE;

	//add the standard cell types
	AddCellType(&m_normalCellType);
	AddCellType(m_dropListType.get());
	AddCellType(&m_checkBoxType);
	AddCellType(&m_arrowType);
	AddCellType(&m_progressType);

	//add a NULL datasource to the beginning of the
	//data source list, since index 0 is not used
	AddDataSource(NULL);

	m_enableUpdate = TRUE;

	//m_findReplaceDialog = NULL;
	//m_GI->m_findDialogRunning = FALSE;
	//m_GI->m_findDialogStarted = FALSE;
	//m_GI->m_findInAllCols = FALSE;

	m_GI->m_tabSizing = FALSE;

	// Set all function pointers in grid info class
	m_GI->OnCellTypeNotify = [this](long ID, int col, long row, long msg, long long param) { return OnCellTypeNotify(ID, col, row, msg, param); };
	m_GI->GetCellIndirect = [this](int col, long row, CUGCell* cell) { return GetCellIndirect(col, row, cell); };
	m_GI->GetCellType = [this](int type) { return GetCellType(type); };
	m_GI->GetCellTypeColRow = [this](int col, long row) { return GetCellType(col, row); };
	m_GI->GetRowHeight = [this](long row) { return GetRowHeight(row); };
	m_GI->GetNonUniformRowHeight = [this](long row) { return GetNonUniformRowHeight(row); };
	m_GI->SetCell = [this](int col, long row, CUGCell* cell) { return SetCell(col, row, cell); };
	m_GI->RedrawAll = [this]() { return RedrawAll(); };
	m_GI->RedrawCell = [this](int col, long row) { return RedrawCell(col, row); };
	m_GI->GetCellRect = [this](int col, long row, RECT* rect) { return GetCellRect(col, row, rect); };
	m_GI->GetRangeRect = [this](int startCol, long startRow, int endCol, long endRow, RECT* rect) { return GetRangeRect(startCol, startRow, endCol, endRow, rect); };
	m_GI->OnKillFocusNewWnd = [this](int section, HWND hNewWnd) { OnKillFocus(section, hNewWnd); };
	m_GI->OnSetFocus = [this](int section) { OnSetFocus(section); };
	m_GI->OnKillFocus = [this](int section) { OnKillFocus(section); };
	m_GI->EditCtrlFinished = [this](LPCTSTR string, BOOL cancelFlag, BOOL continueFlag, int continueCol, long continueRow) { return EditCtrlFinished(string, cancelFlag, continueFlag, continueCol, continueRow); };
	m_GI->GotoCell = [this](int col, long row) { return GotoCell(col, row); };
	m_GI->GotoCol = [this](int col) { return GotoCol(col); };
	m_GI->GotoRow = [this](long row) { return GotoRow(row); };
	m_GI->OnEditVerify = [this](int col, long row, HWND edit, UINT* vcKey) { return OnEditVerify(col, row, edit, vcKey); };
	m_GI->GetCellFromPointColRow = [this](int x, int y, int* col, long* row) { return GetCellFromPoint(x, y, col, row); };
	m_GI->GetCellFromPoint = [this](int x, int y, int* ptcol, long* ptrow, RECT* rect) { return GetCellFromPoint(x, y, ptcol, ptrow, rect); };
	m_GI->OnSortEvaluate = [this](CUGCell* cell1, CUGCell* cell2, int flags) { return OnSortEvaluate(cell1, cell2, flags); };
	//m_GI->OnScreenDCSetup = [this](CDC* dc, CDC* db_dc, int section) { OnScreenDCSetup(dc, db_dc, section); };
	m_GI->GetNumberRows = [this]() { return GetNumberRows(); };
	m_GI->AdjustComponentSizes = [this]() { AdjustComponentSizes(); };
	m_GI->StartMenu = [this](int col, long row, POINT* point, int section) { return StartMenu(col, row, point, section); };
	m_GI->OnLClicked = [this](int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) { OnLClicked(col, row, updn, rect, point, processed); };
	m_GI->OnRClicked = [this](int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) { OnRClicked(col, row, updn, rect, point, processed); };
	m_GI->OnDClicked = [this](int col, long row, RECT* rect, POINT* point, BOOL processed) { OnDClicked(col, row, rect, point, processed); };
	m_GI->OnMouseMove = [this](int col, long row, POINT* point, UINT nFlags, BOOL processed) { OnMouseMove(col, row, point, nFlags, processed); };
	m_GI->OnTH_LClicked = [this](int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) { OnTH_LClicked(col, row, updn, rect, point, processed); };
	m_GI->OnTH_RClicked = [this](int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) { OnTH_RClicked(col, row, updn, rect, point, processed); };
	m_GI->OnTH_DClicked = [this](int col, long row, RECT* rect, POINT* point, BOOL processed) { OnTH_DClicked(col, row, rect, point, processed); };
	m_GI->OnSH_LClicked = [this](int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) { OnSH_LClicked(col, row, updn, rect, point, processed); };
	m_GI->OnSH_RClicked = [this](int col, long row, int updn, RECT* rect, POINT* point, BOOL processed) { OnSH_RClicked(col, row, updn, rect, point, processed); };
	m_GI->OnSH_DClicked = [this](int col, long row, RECT* rect, POINT* point, BOOL processed) { OnSH_DClicked(col, row, rect, point, processed); };
	m_GI->OnCB_LClicked = [this](int updn, RECT* rect, POINT* point, BOOL processed) { OnCB_LClicked(updn, rect, point, processed); };
	m_GI->OnCB_RClicked = [this](int updn, RECT* rect, POINT* point, BOOL processed) { OnCB_RClicked(updn, rect, point, processed); };
	m_GI->OnCB_DClicked = [this](RECT* rect, POINT* point, BOOL processed) { OnCB_DClicked(rect, point, processed); };
	m_GI->OnKeyDown = [this](UINT* vcKey, BOOL processed) { OnKeyDown(vcKey, processed); };
	m_GI->OnKeyUp = [this](UINT* vcKey, BOOL processed) { OnKeyUp(vcKey, processed); };
	m_GI->OnCharDown = [this](UINT* vcKey, BOOL processed) { OnCharDown(vcKey, processed); };
	m_GI->OnCanSizeCol = [this](int col) { return OnCanSizeCol(col); };
	m_GI->OnColSizing = [this](int col, int* width) { OnColSizing(col, width); };
	m_GI->OnColSized = [this](int col, int* width) { OnColSized(col, width); };
	m_GI->OnCanSizeRow = [this](long row) { return OnCanSizeRow(row); };
	m_GI->OnRowSizing = [this](long row, int* height) { OnRowSizing(row, height); };
	m_GI->OnRowSized = [this](long row, int* height) { OnRowSized(row, height); };
	m_GI->OnCanSizeTopHdg = [this]() { return OnCanSizeTopHdg(); };
	m_GI->OnCanSizeSideHdg = [this]() { return OnCanSizeSideHdg(); };
	m_GI->OnTopHdgSizing = [this](int* height) { return OnTopHdgSizing(height); };
	m_GI->OnSideHdgSizing = [this](int* width) { return OnSideHdgSizing(width); };
	m_GI->OnTopHdgSized = [this](int* height) { return OnTopHdgSized(height); };
	m_GI->OnSideHdgSized = [this](int* width) { return OnSideHdgSized(width); };
	m_GI->OnColRowSizeFinished = [this]() { OnColRowSizeFinished(); };
	m_GI->SetTH_Height = [this](int height) { return SetTH_Height(height); };
	m_GI->SetSH_Width = [this](int width) { return SetSH_Width(width); };
	m_GI->OnHint = [this](int col, long row, int section, TOOLTIP_SETTINGS& ttSettings) { return OnHint(col, row, section, ttSettings); };
	m_GI->MoveTopRow = [this](int flag) { return MoveTopRow(flag); };
	m_GI->SetTopRow = [this](long row) { return SetTopRow(row); };
	m_GI->MoveCurrentRow = [this](int flag) { return MoveCurrentRow(flag); };
	m_GI->SetLeftCol = [this](int col) { return SetLeftCol(col); };
	m_GI->MoveLeftCol = [this](int flag) { return MoveLeftCol(flag); };
	m_GI->MoveCurrentCol = [this](int flag) { return MoveCurrentCol(flag); };
	m_GI->OnViewMoved = [this](int nScrolDir, long oldPos, long newPos) { OnViewMoved(nScrolDir, oldPos, newPos); };
	m_GI->GetJoinStartCell = [this](int* col, long* row, CUGCell* cell) { return GetJoinStartCell(col, row, cell); };
	m_GI->GetJoinRange = [this](int* col, long* row, int* col2, long* row2) { return GetJoinRange(col, row, col2, row2); };
	m_GI->SetColWidth = [this](int col, int width, bool notify) { return SetColWidth(col, width, notify); };
	m_GI->VerifyCurrentRow = [this](long* newRow) { return VerifyCurrentRow(newRow); };
	//m_GI->OnDrawFocusRect = [this](CDC* dc, RECT* rect) { OnDrawFocusRect(dc, rect); };
	m_GI->BestFit = [this](int startCol, int endCol, int CalcRange, int flag) { return BestFit(startCol, endCol, CalcRange, flag); };
	m_GI->SetTH_RowHeight = [this](int row, int height) { return SetTH_RowHeight(row, height); };
	m_GI->SetSH_ColWidth = [this](int col, int width) { return SetSH_ColWidth(col, width); };
	m_GI->MakeSuperTooltip = [this](NM_PPTOOLTIP_NEED_TT* pNeedTT, CXeTooltipIF* xtooltip, HWND hWnd, int section) { return MakeSuperTooltip(pNeedTT, xtooltip, hWnd, section); };
	m_GI->HideTooltip = [this]() { HideTooltip(); };
	m_GI->OnColSwapStart = [this](int col) { return OnColSwapStart(col); };
	m_GI->OnCanColSwap = [this](int fromCol, int toCol) { return OnCanColSwap(fromCol, toCol); };
	m_GI->OnColSwapped = [this](int fromCol, int toCol) { OnColSwapped(fromCol, toCol); };
	m_GI->MoveColPosition = [this](int fromCol, int toCol, BOOL insertBefore) { return MoveColPosition(fromCol, toCol, insertBefore); };
	m_GI->SetRowHeight = [this](long row, int height) { return SetRowHeight(row, height); };
	m_GI->HScroll = [this](UINT nSBCode, UINT nPos) { HScroll(nSBCode, nPos); };
}

/***************************************************
Destructor
	clean up all memory that was allocated
****************************************************/
CUGCtrl::~CUGCtrl()
{
	// Note DS deleted by view

	//delete m_ppenWvBorder;

	//delete m_pDropSource;

	//int loop;

	if(m_dataSrcList != NULL)
		delete[] m_dataSrcList;
		
	//delete the font list
	//for(loop = 0;loop < m_fontList->GetMaxCount();loop++){
	//	CFont* font = (CFont*)m_fontList->GetPointer(loop);
	//	if(font != NULL)
	//		delete font;
	//}
	//if(m_fontList !=  NULL)
	//	delete m_fontList;

	//delete the bitmap list
	//for(loop = 0;loop < m_bitmapList->GetMaxCount();loop++){
	//	CBitmap* bitmap = (CBitmap*)m_bitmapList->GetPointer(loop);
	//	if(bitmap != NULL)
	//		delete bitmap;
	//}
	//if(m_bitmapList != NULL)
	//	delete m_bitmapList;

	if(m_cellTypeList != NULL)
		delete m_cellTypeList;
	
	if(m_cellStylesList != NULL)
		delete m_cellStylesList;
	
	if(m_validateList != NULL)
		delete m_validateList;
	
	if(m_displayFormatList != NULL)
		delete m_displayFormatList;
	
	if(m_CUGGrid != NULL)
		delete m_CUGGrid;
	
	if(m_CUGTopHdg != NULL)
		delete m_CUGTopHdg;
	
	if(m_CUGSideHdg != NULL)
		delete m_CUGSideHdg;
	
	if(m_CUGVScroll != NULL)
		delete m_CUGVScroll;
	
	if(m_CUGHScroll != NULL)
		delete m_CUGHScroll;
	
	//if(m_CUGCnrBtn != NULL)
	//	delete m_CUGCnrBtn;

	//scroll hint window
	//#ifdef UG_ENABLE_SCROLLHINTS
	//	if(m_CUGHint != NULL)
	//		delete m_CUGHint;
	//#endif

	//#ifdef UG_ENABLE_PRINTING
	//	if(m_CUGPrint != NULL)
	//		delete m_CUGPrint;
	//#endif

	//if(m_CUGTab	!= NULL)
	//	delete m_CUGTab;
	
	if(m_GIList != NULL){
		for(int loop = 0; loop < m_numberSheets; loop++){
			if(m_GIList[loop] != NULL)
				delete m_GIList[loop];
		}
		delete[] m_GIList;
	}


}

/***************************************************
Message Map
	Processes default windows messages
****************************************************/
//const UINT ugmsg_FindDialog = RegisterWindowMessage(FINDMSGSTRING);

LRESULT CUGCtrl::_OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return CXeD2DWndBase::_OnOtherMessage(hWnd, uMsg, wParam, lParam);
}

//BEGIN_MESSAGE_MAP(CUGCtrl, CWnd)
//	//{{AFX_MSG_MAP(CUGCtrl)
//	ON_WM_SIZE()
//	ON_WM_CREATE()
//	ON_WM_DESTROY()
//	ON_WM_PAINT()
//	ON_WM_NCPAINT()
//	ON_WM_SETFOCUS()
//	ON_WM_SYSCOLORCHANGE()
//	ON_WM_HSCROLL()
//	ON_WM_VSCROLL()
//	ON_WM_GETDLGCODE()
//	ON_WM_ERASEBKGND()
//	ON_REGISTERED_MESSAGE(ugmsg_FindDialog ,ProcessFindDialog)
//	ON_MESSAGE( UGCT_MESSAGE, OnCellTypeMessage)
//	//ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
//	//ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
//	//ON_NOTIFY(UDM_TOOLTIP_DISPLAY, 0, OnPPToolTipNotify)
//	//ON_NOTIFY(UDM_TOOLTIP_NEED_TT, 0, OnPPTTNF_NeedTT)
//	//}}AFX_MSG_MAP
//	//ON_MESSAGE(WMU_DATA_GRID_COL_WIDTH_CHANGED, OnColWidthChangedMessage)
//	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
//
//END_MESSAGE_MAP()

/***************************************************
OnEraseBkgnd
****************************************************/
//BOOL CUGCtrl::OnEraseBkgnd( CDC* pDC )
//{
//	UNREFERENCED_PARAMETER(pDC);
//	return 1;
//}

/***************************************************
OnSysColorChange
This function is called when system colors are changed
It then notifies all of the cell type classes, so
they can update themselves
****************************************************/
//void CUGCtrl::OnSysColorChange() 
//{
//	CWnd::OnSysColorChange();
//
//	//call all the child windows and cell types 
//	// OnSystemChange members
//	//check to see if there is room in the current list					 
//
//	int loop;
//	int count = m_cellTypeList->GetCount();
//
//	for(loop=0;loop <count;loop++){
//		if(m_cellTypeList->GetPointer(loop) != NULL){
//			((CUGCellType *)m_cellTypeList->GetPointer(loop))->OnSystemChange();
//		}
//	}
//	
//	// update the headings, because they do not automatically respond to a change in system color
//	// only change the default, since we want headings that have been manually set to not be affected
//	m_GI->m_hdgDefaults->SetBackColor( GetSysColor( COLOR_BTNFACE ) );
//}

/***************************************************
OnHScroll
	pass all scroll messages over to the scroll child
	window
****************************************************/
//void CUGCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
LRESULT CUGCtrl::_OnHScroll(WPARAM wParam, LPARAM lParam)
{
	//UNREFERENCED_PARAMETER(*pScrollBar);
	UINT nSBCode = LOWORD(wParam);
	UINT nPos = HIWORD(wParam);
	HWND hSBwnd = (HWND)lParam;
	m_CUGHScroll->HScroll(nSBCode, nPos);
	return 0;
}

/***************************************************
OnVScroll
	pass all scroll messages over to the scroll child
	window
****************************************************/
//void CUGCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
LRESULT CUGCtrl::_OnVScroll(WPARAM wParam, LPARAM lParam)
{
	//UNREFERENCED_PARAMETER(*pScrollBar);
	UINT nSBCode = LOWORD(wParam);
	UINT nPos = HIWORD(wParam);
	HWND hSBwnd = (HWND)lParam;
	if (nSBCode == SB_THUMBTRACK || SB_THUMBPOSITION)
	{
		SCROLLINFO si = { 0 };
		si.cbSize = sizeof(si);
		si.fMask = SIF_TRACKPOS;
		m_CUGVScroll->GetScrollInfo(&si);
		nPos = si.nTrackPos;
	}
	m_CUGVScroll->VScroll(nSBCode, nPos);
	return 0;
}


/***************************************************
OnSize
	Sizes all the child windows whenever the main 
	window sizes. This includes the Grid, Headings
	and Scroll bars, tabs, etc.
****************************************************/
//void CUGCtrl::OnSize(UINT nType, int cx, int cy) 
LRESULT CUGCtrl::_OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	//UNREFERENCED_PARAMETER(nType);
	//UNREFERENCED_PARAMETER(cx);
	//UNREFERENCED_PARAMETER(cy);

	if (m_isComponentsCreated)
	{
		_RedrawDirectly();
		EnableUpdate(FALSE);
		AdjustComponentSizes();
		EnableUpdate(TRUE);
	}
	return CXeD2DWndBase::_OnSize(hWnd, wParam, lParam);
}

LRESULT CUGCtrl::_OnGetDlgCode(WPARAM wParam, LPARAM lParam)
{
	return DLGC_WANTALLKEYS | DLGC_WANTARROWS;
}

/***************************************************
OnCellTypeMessage
	Used to send Celltypes a windows message.
	The message is UGCT_NOTIFY. wParam must contain
	the ID of a valid CellType. lParam is for general
	purpose data, and its use if defined by each
	celltype.
Params:
	wParam - Celltype ID
	lParam - message data
Return
	0 - if the messsage was processed
	1 - if the message was not processed
****************************************************/
LRESULT CUGCtrl::OnCellTypeMessage(WPARAM wParam, LPARAM lParam){

	CUGCellType* ct = GetCellType((int)wParam);
	if(ct == NULL)
		return 1;

	return ct->OnMessage(lParam);
}

/***************************************************
EditCancel
	Purpose
		Cancels any editing if it is in progress
	Params
		none
	Return
		UG_SUCCESS	success
		UG_ERROR	error
****************************************************/
int CUGCtrl::EditCancel(){

	if(m_GI->m_editInProgress == FALSE)
		return UG_SUCCESS;
	
	m_GI->m_editInProgress = FALSE;
		
	if(GetFocus() == m_GI->m_editCtrl)
		m_CUGGrid->SetFocus();

	//hide the edit control
	::ShowWindow(m_GI->m_editCtrl, SW_HIDE);

	return UG_SUCCESS;
}

/***************************************************
	Purpose
		This function is to be called by all edit
		controls that are used within the grid when
		editing is finished or canceled.
	Params
		string		- the string that is in the edit control
		cancelFlag	- edit cancel state
		continueFlag- edit continue state
		continueCol	- continue col, if being continued
		continueRow - continue row, if being continued
	Return
		TRUE - if successful
		FALSE - if the current editing must continue
****************************************************/
int CUGCtrl::EditCtrlFinished(LPCTSTR string,BOOL cancelFlag,
							  BOOL continueFlag,int continueCol,
							  long continueRow){
	
	if(m_GI->m_editInProgress == FALSE)
		cancelFlag = TRUE;

	//call the OnEditFinished notify in the datasource...
	if( m_GI->m_defDataSource->OnEditFinish(m_GI->m_editCol, m_GI->m_editRow,
		m_GI->m_editCtrl,string,cancelFlag) == TRUE)
	{
     //call the OnEditFinished notify
	 if( OnEditFinish(m_GI->m_editCol, m_GI->m_editRow, m_GI->m_editCtrl,string,cancelFlag) != FALSE)
	 {
		//hide the edit control
		::ShowWindow(m_GI->m_editCtrl, SW_HIDE);

		//save the info
		if(!cancelFlag)
		{
			CUGCell editCell;
			GetCellIndirect(m_GI->m_editCol, m_GI->m_editRow, &editCell );

			switch( editCell.GetDataType())
			{
			case UGCELLDATA_NUMBER:
				editCell.SetText( string );
				editCell.SetDataType( UGCELLDATA_NUMBER );
				break;
			case UGCELLDATA_BOOL:
				editCell.SetText( string );
				editCell.SetDataType( UGCELLDATA_BOOL );
				break;
			case UGCELLDATA_TIME:
				editCell.SetText( string );
				editCell.SetDataType( UGCELLDATA_TIME );
				break;
			case UGCELLDATA_CURRENCY:
				editCell.SetText( string );
				editCell.SetDataType( UGCELLDATA_CURRENCY );
				break;
			default:
				editCell.SetText( string );
			}

			SetCell(m_GI->m_editCol, m_GI->m_editRow, &editCell );
		}

		// If the newly edited string is larger than its column's
		// width and the cell overlapping is enabled, but edit control
		// did not auto expand and content of neighbouring (overflow)
		// cells is empty ... then redraw all cells involved.
		CRect editRect;
		::GetClientRect(m_GI->m_editCtrl, &editRect);

		if (editRect.Width() <= GetColWidth(m_GI->m_editCol) &&
			//( m_GI->m_enableCellOverLap == TRUE ) &&
			m_GI->m_editCol < m_GI->m_numberCols - 1)
		{
			const wchar_t* pCellText = QuickGetText(m_GI->m_editCol + 1, m_GI->m_editRow);
			if (pCellText && std::wcslen(pCellText) == 0)
			{
				// The edit control is the same (or smaller) width as its column,
				// and the first cell to the left is empty.  Now lets check
				// if the text contained in the edit control will fit into cell entiely.
				//CDC *offDc = GetDC();
				//CFont *cellFont = m_GI->m_editCtrl->GetFont();
				//CFont *oldFont = NULL;
				//
				//if ( cellFont != NULL )
				//	oldFont = offDc->SelectObject( cellFont );

				//GetCellRect(m_GI->m_editCol, m_GI->m_editRow, editRect );
				//offDc->DrawText( string, editRect, DT_CALCRECT );

				//// clean up after the off screen DC
				//if ( cellFont != NULL )
				//	offDc->SelectObject( oldFont );

				//// Release obtained DC
				//ReleaseDC( offDc );

				//if ( editRect.Width() > GetColWidth(m_GI->m_editCol ))
				{	// We should redraw more than just the currently edited cell
					RedrawAll();
				}
			}
		}
		
		m_GI->m_editInProgress = FALSE;

		//check to see if editing continues in a new cell
		if(continueFlag && m_GI->m_editParent == m_CUGGrid->GetSafeHwnd()){

			// adjust target continue cell if cell part of a join
			int startCol, endCol;
			long startRow, endRow;

			startCol = continueCol;
			startRow = continueRow;

			// is continue cell part of a join?
			if(continueCol < m_GI->m_numberCols &&
				continueCol > -1 &&
				continueRow < m_GI->m_numberRows &&
				continueRow > -1 ) {
				if(GetJoinRange(&startCol, &startRow, &endCol, &endRow)==UG_SUCCESS) {
					// yes... adjust column
					if(m_GI->m_editCol == startCol) {
						// go past end of join we're in
						if(continueCol > startCol && continueCol <= endCol)
							continueCol += (endCol-startCol);
					}
					else {
						// go to start of join we're entering
						if(continueCol != startCol)
							continueCol = startCol;
					}
					
					// adjust row .. 
					if(m_GI->m_editRow == startRow) {
						if(continueRow > startRow && continueRow <= endRow)
							continueRow += (endRow - startRow);
					}
					else {
						if(continueRow != startRow)
							continueRow = startRow;
					}
				}
			}
				
			if(continueCol < 0)
				continueCol = m_GI->m_numberCols -1;
			else if(continueCol >= m_GI->m_numberCols)
				continueCol = 0;
			if(continueRow < 0)
				continueRow = m_GI->m_numberRows -1;
			else if(continueRow >= m_GI->m_numberRows)
				continueRow = 0;

			if(m_GI->m_defDataSource->OnEditContinue(m_GI->m_editCol, m_GI->m_editRow,&continueCol,&continueRow) == TRUE)
			{
				if(OnEditContinue(m_GI->m_editCol, m_GI->m_editRow,&continueCol,&continueRow) != FALSE)
				{
					StartEdit(continueCol,continueRow,0);
					return TRUE;
				}
			}
		}
	 }
	}

	if(m_GI->m_editInProgress == FALSE)
	{	// The edit is finished, redraw the updated cell
		m_GI->m_editInProgress = FALSE;
		RedrawCell(m_GI->m_editCol, m_GI->m_editRow );
		return TRUE;
	}

	return FALSE;
}

/***************************************************
AdjustComponentSizes
	Purpose
		Sizes all of the child windows to match the 
		parent window size. A OnAdjustComponentSizes
		notification is called so that the calculated
		sizes can be modified
	Params
		none
	Return
		none
****************************************************/
void CUGCtrl::AdjustComponentSizes()
{
	int loop;
	RECT rect;
	int vScrollWidth = m_GI->m_vScrollWidth;
	int hScrollHeight = m_GI->m_hScrollHeight;
	int origTabWidth = m_GI->m_tabWidth;

	GetClientRect(&rect);

	// calculate the width and height of the grid area
	int maxWidth = rect.right - m_GI->m_sideHdgWidth;
	int maxHeight = rect.bottom - m_GI->m_topHdgHeight;
	int width = 0, height = 0;
	// calculate the width, and stop as soon as the width
	// exceeds the grid window rect
	for(loop = 0;loop < m_GI->m_numberCols;loop++)
	{
		// SK MOD - make last col fill available horizontal space.
		if (loop == (m_GI->m_numberCols - 1))
		{
			int lastColWidth = maxWidth - width - vScrollWidth;
			if (lastColWidth > 30)
			{
				m_GI->m_colInfo[loop].width = lastColWidth;
			}
		}

		width += GetColWidth(loop);
		if(width > maxWidth)
			break;
	}
	// calculate the height, and stop as soon as the height
	// exceeds the grid window rect
	for(loop = 0;loop < m_GI->m_numberRows;loop++)
	{
		height += GetRowHeight(loop);
		if(height > maxHeight)
			break;
	}
	// if the horizontal scroll bar or the tab contols will be visible,
	// than decrease total window height by the height of the scroll bar
	if ( m_GI->m_tabWidth > 0 || width > maxWidth )
		maxHeight -= m_GI->m_hScrollHeight;
	// if the vertical scroll bar will be visible, than decrease total
	// window width by the width of the scroll bar
	if(height > maxHeight)
		maxWidth -= m_GI->m_vScrollWidth;
	// determine if the horizontal scroll bar needs to be visible
	if(width <= maxWidth && m_GI->m_tabWidth <= 0)
		m_GI->m_hScrollHeight = 0;
	// determine if the vertical scroll bar needs to be visible
	if(height <= maxHeight)
		m_GI->m_vScrollWidth = 0;
	// if the h. scroll bar is not needed when working with tabs
	// than hide it and maximize tab window
	if( width <= maxWidth && m_GI->m_tabWidth > 0 )
		m_GI->m_tabWidth = rect.right + 6;
	// also make sure that that tab width does not extend beyond the window
	else if( m_GI->m_tabWidth > 0 )
		if( m_GI->m_tabWidth >= maxWidth - m_GI->m_vScrollWidth )
			m_GI->m_tabWidth = ( maxWidth - m_GI->m_vScrollWidth );

	//setup the vScroll Rect
	m_GI->m_vScrollRect.left	= rect.right - m_GI->m_vScrollWidth;
	m_GI->m_vScrollRect.top		= 0;
	m_GI->m_vScrollRect.right	= rect.right;
	m_GI->m_vScrollRect.bottom  = rect.bottom - m_GI->m_hScrollHeight;

	//setup the hScroll Rect
	m_GI->m_hScrollRect.left	= m_GI->m_tabWidth;
	m_GI->m_hScrollRect.top		= rect.bottom - m_GI->m_hScrollHeight;
	m_GI->m_hScrollRect.right	= rect.right - m_GI->m_vScrollWidth;
	m_GI->m_hScrollRect.bottom  = rect.bottom;

	//setup the tabRect
	m_GI->m_tabRect.left		= 0;
	m_GI->m_tabRect.top			= rect.bottom - m_GI->m_hScrollHeight;
	m_GI->m_tabRect.right		= m_GI->m_tabWidth;
	m_GI->m_tabRect.bottom		= rect.bottom;

	//setup the topHeadingRect
	m_GI->m_topHdgRect.left		= m_GI->m_sideHdgWidth;
	m_GI->m_topHdgRect.top		= 0;
	m_GI->m_topHdgRect.right	= rect.right - m_GI->m_vScrollWidth;
	m_GI->m_topHdgRect.bottom	= m_GI->m_topHdgHeight;

	//setup the side headingRect
	m_GI->m_sideHdgRect.left	= 0;
	m_GI->m_sideHdgRect.top		= m_GI->m_topHdgHeight;
	m_GI->m_sideHdgRect.right	= m_GI->m_sideHdgWidth;
	m_GI->m_sideHdgRect.bottom	= rect.bottom -	m_GI->m_hScrollHeight;

	//setup the corner buttonRect
	m_GI->m_cnrBtnRect.left		= 0;
	m_GI->m_cnrBtnRect.top		= 0;
	m_GI->m_cnrBtnRect.right	= m_GI->m_sideHdgWidth;
	m_GI->m_cnrBtnRect.bottom	= m_GI->m_topHdgHeight;

	//setup the GridRect
	m_GI->m_gridRect.left		= m_GI->m_sideHdgWidth;
	m_GI->m_gridRect.top		= m_GI->m_topHdgHeight;
	m_GI->m_gridRect.right		= m_GI->m_topHdgRect.right;
	m_GI->m_gridRect.bottom		= m_GI->m_sideHdgRect.bottom;
	m_GI->m_gridWidth = m_GI->m_gridRect.right - m_GI->m_gridRect.left;
	m_GI->m_gridHeight = m_GI->m_gridRect.bottom - m_GI->m_gridRect.top;

	//call the nofity function
	OnAdjustComponentSizes(&m_GI->m_gridRect,&m_GI->m_topHdgRect,
		&m_GI->m_sideHdgRect,&m_GI->m_cnrBtnRect,&m_GI->m_vScrollRect,
		&m_GI->m_hScrollRect,&m_GI->m_tabRect);

	if ( m_GI->m_paintMode == TRUE )
	{
		if(m_GI->m_tabSizing)
		{
			m_CUGHScroll->MoveWindow(&m_GI->m_hScrollRect);
			//m_CUGTab->MoveWindow(&m_GI->m_tabRect);
			//m_CUGTab->Update();
			m_CUGHScroll->Update();
			::ValidateRect(Hwnd(), NULL);
		}
		else
		{
			m_CUGGrid->MoveWindow(&m_GI->m_gridRect);
			m_CUGTopHdg->MoveWindow(&m_GI->m_topHdgRect);
			m_CUGSideHdg->MoveWindow(&m_GI->m_sideHdgRect);
			//m_CUGCnrBtn->MoveWindow(&m_GI->m_cnrBtnRect);
			m_CUGVScroll->MoveWindow(&m_GI->m_vScrollRect);
			m_CUGHScroll->MoveWindow(&m_GI->m_hScrollRect);
			//m_CUGTab->MoveWindow(&m_GI->m_tabRect);

			CalcTopRow();
			CalcLeftCol();
			Update();
		}
	}
	//make sure that the scroll dimensions are put back to their
	//original values
	m_GI->m_vScrollWidth = vScrollWidth;
	m_GI->m_hScrollHeight = hScrollHeight;

	m_GI->m_tabWidth = origTabWidth;
	m_GI->m_lastTopRow = m_GI->m_topRow;
	m_GI->m_lastLeftCol = m_GI->m_leftCol;	
}

/***************************************************
OnCreate
	Purpose
		This function retrieves the control's parent's 
		font, then creates the child windows
****************************************************/
//int CUGCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
//{
//	if (CWnd::OnCreate(lpCreateStruct) == -1)
//		return -1;
//	
//	CreateChildWindows();
//
//	//m_xtooltip->Create(this);
//	//m_xtooltip->SetBorder(m_xeUI->GetColor(CID::TT_BgBorder));
//
//	return 0;
//}

/***************************************************
OnDestroy
	Purpose
		This message handler is called just before 
		grid window is about to be destroyed.
		We will use this handler to destroy
		tool tip window.
****************************************************/
//void CUGCtrl::OnDestroy() 
//{
//	CWnd::OnDestroy();
//
//	// Destroy tooltip window
//#ifdef UG_ENABLE_SCROLLHINTS
//	m_CUGHint->DestroyWindow();
//#endif
//
//	//m_xtooltip->DestroyWindow();
//}

/***************************************************
CreateChildWindows
	Puropse
		This function creates all the child windows
		and passes them a pointer to this class as well
		as a pointer to the internal data structure
		(child windows include the grid, headings and
		scroll bars).
****************************************************/
BOOL CUGCtrl::CreateChildWindows(){

	RECT rect ={0,0,0,0};

	//set up the child windows
	//m_CUGGrid->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,this,1,NULL);
	//m_CUGTopHdg->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,this,2,NULL);
	//m_CUGSideHdg->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,this,3,NULL);
	//m_CUGCnrBtn->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,this,4,NULL);
	//m_CUGTab->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,this,5,NULL);

	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	m_CUGGrid->CreateGrid(dwStyle, rect, Hwnd(), 1);
	m_CUGTopHdg->CreateTopHdg(dwStyle, rect, Hwnd(), 2);
	m_CUGSideHdg->CreateSideHdg(dwStyle, rect, Hwnd(), 3);
	//m_CUGCnrBtn->CreateCrnBtn(WS_CHILD | WS_VISIBLE, rect, Hwnd(), 4);
	//m_CUGTab->CreateTab(WS_CHILD | WS_VISIBLE, rect, Hwnd(), 5);

	m_CUGVScroll->Create(SBS_VERT | dwStyle,rect,Hwnd(),6, nullptr);
	m_CUGHScroll->Create(SBS_HORZ | dwStyle,rect,Hwnd(),7, nullptr);

	//set up the hint window
	//#ifdef UG_ENABLE_SCROLLHINTS
	//m_CUGHint->Create(this);
	//#endif

	//setup the default edit control
	if(m_defEditCtrl.CreateUGEdit(rect, Hwnd(), UG_EDITID, true) == false) {
		//fail
	}
	//if(m_defEditCtrl.Create(WS_CHILD|WS_HSCROLL|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL,rect,CWnd::FromHandle(Hwnd()), UG_EDITID) == FALSE) {
	//	//fail
	//}
	//if(m_defMaskedEditCtrl.Create(WS_CHILD|ES_AUTOHSCROLL,rect, CWnd::FromHandle(Hwnd()),UG_MEDITID) == FALSE){
	//	//fail
	//}
	
	return TRUE;
}

/***************************************************
OnPaint
	Purpose
		This routine is responsible for painting any area
		that is not covered by one of the child windows
		(such as the bottom right corner)
****************************************************/
//void CUGCtrl::OnPaint() 
void CUGCtrl::_PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient)
{
	if ( m_GI->m_paintMode == FALSE )
		return;

	::ValidateRect(Hwnd(), NULL);

	pRT->FillRectangle(RectFfromRect(m_GI->m_cnrBtnRect), GetBrush(CID::GrdBRcorner));

	if (m_GI->m_vScrollRect.Width() > 0 && m_GI->m_hScrollRect.Height() > 0)
	{
		// By default the Ultimate Grid will not cover a small
		// rectangle (bottom/right) where the scrollbars meet.
		// This code fills that area with a button face color
		//CDC * dc = GetDC();
	//#pragma NOTE("SKMOD to support custom colors")
		//CBrush brush(m_xeUI->GetColor(CID::GrdBRcorner));
		CRect rect;
		GetClientRect( rect );
		// adjust the rect to only cover the required area
		rect.left = rect.right - GetVS_Width();
		rect.top = rect.bottom - GetHS_Height();
		//dc->FillRect(&rect,&brush);
		// clean up
		//ReleaseDC(dc);

		pRT->FillRectangle(RectFfromRect(rect), GetBrush(CID::GrdBRcorner));
		// draw remainder of the grid window
		//AdjustComponentSizes();
	}
}

//void CUGCtrl::OnNcPaint()
//{
//	CRect rcW, rcC;
//	GetWindowRect(&rcW);
//	GetClientRect(&rcC);
//	int nW = rcW.Width() - rcC.Width();
//	int nH = rcW.Height() - rcC.Height();
//
//	if ((nW == 0 && nH == 0)				// No border?
//		|| m_xeUI->GetColor(CID::GrdBorder) == CLR_INVALID)	// OR Use windows default border style?
//	{
//		Default();
//		return;
//	}
//
//	rcW.OffsetRect(-rcW.left, -rcW.top);	// Make coords. TL = 0,0
//
//	CWindowDC dc(this);
//	dc.Draw3dRect(&rcW, m_xeUI->GetColor(CID::GrdBorder), m_xeUI->GetColor(CID::GrdBorder));
//	if (nW > 2 && m_xeUI->GetColor(CID::GrdBorder) != CLR_INVALID)
//	{
//		rcW.DeflateRect(1, 1);
//		dc.Draw3dRect(&rcW, m_xeUI->GetColor(CID::GrdBorder), m_xeUI->GetColor(CID::GrdBorder));
//	}
//
//	// Do not call CStatic::OnNcPaint() for painting messages
//}

//BOOL CUGCtrl::PreTranslateMessage(MSG * pMsg)
//{
//	if (m_xtooltip->RelayEvent(pMsg))
//		return TRUE;	// Message was processed by tooltip and should not be dipatched.
//
//	return CWnd::PreTranslateMessage(pMsg);
//}

/***************************************************
PreCreateWindow
	Purpose
		This routine makes sure that certian window
		styles are used
****************************************************/
//BOOL CUGCtrl::PreCreateWindow(CREATESTRUCT& cs) 
//{
//	cs.style |= WS_CLIPCHILDREN;
//	return CWnd::PreCreateWindow(cs);
//}
/************************************************
*************************************************/
//UINT CUGCtrl::OnGetDlgCode() 
//{
//	return DLGC_WANTALLKEYS|DLGC_WANTARROWS;
//}

/***************************************************
OnSetFocus
	Purpose
		Sets the focus to the grid child window
****************************************************/
//void CUGCtrl::OnSetFocus(CWnd* pOldWnd) 
LRESULT CUGCtrl::_OnSetFocus(HWND hOldWnd)
{
	//UNREFERENCED_PARAMETER(*pOldWnd);
	m_CUGGrid->SetFocus();
	m_CUGGrid->Update();
	return 0;
}
/***************************************************
****************************************************/
BOOL CUGCtrl::CreateGrid(DWORD dwStyle, DWORD dwExStyle, const CRect& rect, HWND hParentWnd, UINT nID)
{
	//BOOL rt = Create((LPCTSTR)NULL,(LPCTSTR)_T(""),(DWORD) dwStyle, (const RECT&) rect, 
	//	CWnd::FromHandle(hParentWnd),(UINT) nID, (CCreateContext*)NULL);

	std::wstring classname = L"CUGCtrl_WNDCLASS";
	m_xeUI->RegisterWindowClass(classname, D2DCtrl_WndProc);
	HINSTANCE hInstance = m_xeUI->GetInstanceHandle();
	dwStyle = dwStyle | WS_CLIPCHILDREN | WS_TABSTOP;
	HWND hWnd = CreateD2DWindow(dwExStyle, classname.c_str(), nullptr, dwStyle,
			rect, hParentWnd, nID);
	if(GetSafeHwnd())
	{
		//SetDefFont(m_xeUI->GetFont(EXE_FONT::eUI_Font) /*nIndex*/);

		CUGCell cell;
		GetHeadingDefault(&cell);
		cell.SetFont(EXE_FONT::eUI_FontBold);
		SetHeadingDefault(&cell);

		if (CreateChildWindows())
		{
			m_GI->m_ctrlWnd = Hwnd();
			m_GI->m_gridWnd = m_CUGGrid->GetSafeHwnd();
			m_GI->m_sideHdgWnd = m_CUGSideHdg->GetSafeHwnd();

			m_isComponentsCreated = true;
			m_GI->m_editCtrl = m_defEditCtrl.m_hWnd;

			OnSetup();
			OnSheetSetup(0);
		}

	}

	// Allow drawing after the grid is initialized
	m_GI->m_paintMode = TRUE;
	// Adjust the grid's components to fit current setup
	AdjustComponentSizes();

	return hWnd != 0;
 }
/***************************************************
AttachGrid
	Purpose
		Attaches the CUGCtrl window from a dialog to the class
		Make sure the OnSetup(); function has been called
		before the dialog is opened, otherwise it will fail
****************************************************/
//BOOL CUGCtrl::AttachGrid(CWnd * wnd,UINT ID)
//{
//	// Disable drawing while the grid is initializing
//	m_GI->m_paintMode = FALSE;
//
//	if( SubclassDlgItem(ID,wnd))
//	{
//		long style = GetWindowLong(Hwnd(),GWL_STYLE);
//		style = style|WS_CLIPCHILDREN|WS_TABSTOP;
//		SetWindowLong(Hwnd(),GWL_STYLE,style);
//
////#pragma NOTE("SK MOD - set GWL_USERDATA as 0x531d2ea7 as a means to identify grid windows")
//		::SetWindowLongPtr( Hwnd(), GWLP_USERDATA, 0x531d2ea7 );
//
//		// if the parent window is specified
//		if(wnd!= NULL)
//		{
//			//LOGFONT logFont;
//			//CFont *pTempFont = wnd->GetFont();
//			//pTempFont->GetLogFont( &logFont );
//			
//			// ceate a font object based on the font information retrieved from
//			// parent window.  This font will be used as grid's default font.
//			//int nIndex = AddFont( logFont.lfHeight, logFont.lfWidth, logFont.lfEscapement,
//			//		 logFont.lfOrientation, logFont.lfWeight, logFont.lfItalic,
//			//		 logFont.lfUnderline, logFont.lfStrikeOut, logFont.lfCharSet,
//			//		 logFont.lfOutPrecision, logFont.lfClipPrecision, 
//			//		 logFont.lfQuality, logFont.lfPitchAndFamily, logFont.lfFaceName );
//
//			SetDefFont(m_xeUI->GetFont(EXE_FONT::eUI_Font) /*nIndex*/ );
//
//			// create a font that will be used for the heading cells.  This object
//			// is almost identical to the grid's default font, except its weight
//			// was increased by 200.
//			//nIndex = AddFont( logFont.lfHeight, logFont.lfWidth, logFont.lfEscapement,
//			//		 logFont.lfOrientation, logFont.lfWeight + 200, logFont.lfItalic,
//			//		 logFont.lfUnderline, logFont.lfStrikeOut, logFont.lfCharSet,
//			//		 logFont.lfOutPrecision, logFont.lfClipPrecision, 
//			//		 logFont.lfQuality, logFont.lfPitchAndFamily, logFont.lfFaceName );
//
//			CUGCell cell;
//			GetHeadingDefault( &cell );
//			cell.SetFont(m_xeUI->GetFont(EXE_FONT::eUI_FontBold));
//			SetHeadingDefault( &cell );
//		}
//
//		CreateChildWindows();
//		// When WS_EX_RTLREADING style was specified for the place holder
//		// window, then set the grid to be in RTL layout mode.
//		style = GetWindowLong( Hwnd(), GWL_EXSTYLE );
//		if ( style&WS_EX_RTLREADING )
//			SetGridLayout( 1 );
//
//		OnSetup();
//		OnSheetSetup(0);
//
//		// Allow drawing after the grid is initialized
//		m_GI->m_paintMode = TRUE;
//		// Adjust the grid's components to fit current setup
//		AdjustComponentSizes();
//
//		return TRUE;
//	}
//
//	return FALSE;
//}
/***************************************************
CalcTopRow
	Purpose
		Calculates the Max top row allowed then adjusts
		the TopRow if needed (m_maxTopRow). This function
		is called internally
****************************************************/
void CUGCtrl::CalcTopRow(){
	
	long oldMaxRow = m_GI->m_maxTopRow;

	//calc the max top row
	long row;
	int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
	m_GI->m_maxTopRow = m_GI->m_numLockRows;
	for(row = m_GI->m_numberRows; row >= 0; row--){
		height -= GetRowHeight(row);
		if(height < 0){
			m_GI->m_maxTopRow = row +1;
			if(m_GI->m_maxTopRow >= m_GI->m_numberRows)
				m_GI->m_maxTopRow = m_GI->m_numberRows -1;
			break;
		}
	}

	//make sure that the max row is not below 0
	if(m_GI->m_maxTopRow < m_GI->m_numLockRows)
		m_GI->m_maxTopRow = m_GI->m_numLockRows;
	if(m_GI->m_topRow < m_GI->m_numLockRows)
		m_GI->m_topRow = m_GI->m_numLockRows;

	//adjust the top row if needed
	if(m_GI->m_topRow > m_GI->m_maxTopRow)
		m_GI->m_topRow = m_GI->m_maxTopRow;

	//check to see if the vscrollbar should be shown
	//or hidden
	if(oldMaxRow == 0 && m_GI->m_maxTopRow > 0)
		AdjustComponentSizes();
	if(oldMaxRow > 0 && m_GI->m_maxTopRow == 0)
		AdjustComponentSizes();
}
/***************************************************
CalcLeftCol
	Purpose
		Calculates the MAX left column then adjusts the
		left column if needed (m_maxLeftCol). This function
		is called internally
****************************************************/
void CUGCtrl::CalcLeftCol(){

	//calc the max left col
	int col;
	int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
	m_GI->m_maxLeftCol = m_GI->m_numLockCols;
	for(col = m_GI->m_numberCols ; col >= 0; col --){
		width -= GetColWidth(col);
		if(width <0){
			m_GI->m_maxLeftCol = col+1;
			if( m_GI->m_maxLeftCol >= m_GI->m_numberCols)
				m_GI->m_maxLeftCol = m_GI->m_numberCols -1;
			break;
		}
	}
	
	//check to make sure the values are not below 0
	if(m_GI->m_maxLeftCol < m_GI->m_numLockCols)
		m_GI->m_maxLeftCol = m_GI->m_numLockCols;
	if(m_GI->m_leftCol < m_GI->m_numLockCols)
		m_GI->m_leftCol = m_GI->m_numLockCols;

	//adjust the left column if needed
	if(m_GI->m_leftCol > m_GI->m_maxLeftCol)
		m_GI->m_leftCol = m_GI->m_maxLeftCol;

}

/***************************************************
AdjustTopRow
	Purpose
		Adjusts the top row so that that current row 
		is in within view. This function is called
		internally
****************************************************/
void CUGCtrl::AdjustTopRow(){

	//if the top row is greater then set it equal
	if(m_GI->m_topRow > m_GI->m_dragRow){
		if(m_GI->m_dragRow >= m_GI->m_numLockRows)
			m_GI->m_topRow = m_GI->m_dragRow;
	}
		

	//if the top row is equal then return
	if(m_GI->m_topRow == m_GI->m_dragRow)
		return;

	//if less then check to see if it is within 
	//the height of the grid for distance
	if(m_GI->m_uniformRowHeightFlag == FALSE){
		long row;
		int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
		for(row = m_GI->m_dragRow; row >= m_GI->m_topRow; row--){
			height -= GetRowHeight(row);
			if(height < 0){
				m_GI->m_topRow = row +1;
				break;
			}
		}
	}
	else{
		int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
		long row = m_GI->m_dragRow - height / m_GI->m_defRowHeight + 1;
		if(m_GI->m_topRow < row)
			m_GI->m_topRow = row;
	}	
}
/***************************************************
AdjustLeftCol 
	Purpose
		Adjusts the left column so that that current 
		col is in view. This function is called 
		internally
****************************************************/
void CUGCtrl::AdjustLeftCol(){

	//if the left col row is greater then set it equal
	if(m_GI->m_leftCol > m_GI->m_dragCol){
		if(m_GI->m_dragCol >= m_GI->m_numLockCols)
			m_GI->m_leftCol = m_GI->m_dragCol;
	}

	//if the left col is equal then return
	if(m_GI->m_leftCol == m_GI->m_dragCol)
		return;

	//if less then check to see if it is within 
	//the width of the grid for distance
	int col;
	int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
	for(col = m_GI->m_dragCol; col >= m_GI->m_leftCol; col--){
		width -= GetColWidth(col);
		if(width < 0 && col < m_GI->m_maxLeftCol){
			m_GI->m_leftCol = col +1;
			break;
		}
	}
}
/***************************************************
Update
	Purpose
		General purpose update, updates are sent to all child
		windows - each child window is then responsible for
		checking all variables that affect it.
****************************************************/
void CUGCtrl::Update(){

	if(!m_enableUpdate)
		return;

	m_CUGGrid->Update();
	//m_CUGCnrBtn->Update();
	m_CUGVScroll->Update();
	m_CUGHScroll->Update();
	//m_CUGTab->Update();
	m_CUGTopHdg->Update();
	m_CUGSideHdg->Update();

	//ValidateRect(NULL);
}
/***************************************************
Moved
	Purpose
		-Movement notify function
		-This function checks to see what moved in the grid
		then notifies the appropriate child windows
		-Processes movements for the multiple-selection class
		-stores the current cell
****************************************************/
void CUGCtrl::Moved()
{
	//call the multiselect manager
	if(m_GI->m_multiSelectFlag && !m_GI->m_editInProgress){
		if(m_GI->m_moveType == 0) //keyboard
			m_GI->m_multiSelect->OnKeyMove(m_GI->m_dragCol,m_GI->m_dragRow);
		else if(m_GI->m_moveType == 1)	//left click
			m_GI->m_multiSelect->OnLClick(m_GI->m_dragCol,m_GI->m_dragRow,m_GI->m_moveFlags);
		else if(m_GI->m_moveType == 2)  //right click
			m_GI->m_multiSelect->OnRClick(m_GI->m_dragCol,m_GI->m_dragRow,m_GI->m_moveFlags);
		else if(m_GI->m_moveType == 3)	//mouse move
			m_GI->m_multiSelect->OnMouseMove(m_GI->m_dragCol,m_GI->m_dragRow,m_GI->m_moveFlags);

		m_GI->m_multiSelect->GetOrigCell(&m_GI->m_currentCol,&m_GI->m_currentRow);
	}
	else{
		m_GI->m_currentCol = m_GI->m_dragCol;
		m_GI->m_currentRow = m_GI->m_dragRow;
	}

	//send notify messages
	BOOL cellChange = FALSE;
	//col change
	if(m_GI->m_lastCol != m_GI->m_currentCol){
		m_GI->m_defDataSource->OnColChange(m_GI->m_lastCol,m_GI->m_currentCol);
		OnColChange(m_GI->m_lastCol,m_GI->m_currentCol);
		cellChange = TRUE;
	}
	//row change
	if(m_GI->m_lastRow != m_GI->m_currentRow){
		m_GI->m_defDataSource->OnRowChange(m_GI->m_lastRow,m_GI->m_currentRow);
		OnRowChange(m_GI->m_lastRow,m_GI->m_currentRow);
		cellChange = TRUE;
	}
	//cell change
	if(cellChange){
		OnCellChange(m_GI->m_lastCol,m_GI->m_currentCol,m_GI->m_lastRow,m_GI->m_currentRow);
	}

	BOOL scrolled = FALSE;
	int nOldRightCol = GetRightCol();
	long nOldBottomRow = GetBottomRow();

	//left col change
	if(m_GI->m_lastLeftCol != m_GI->m_leftCol){
		OnLeftColChange(m_GI->m_lastLeftCol,m_GI->m_leftCol);
		scrolled = TRUE;
	}
	//top row change
	if(m_GI->m_lastTopRow != m_GI->m_topRow){
		OnTopRowChange(m_GI->m_lastTopRow,m_GI->m_topRow);
		scrolled = TRUE;
	}

	//move the tracking window if need be
	//MoveTrackingWindow();	

	//notify the cell types of the move
	if(cellChange) {
		m_GI->m_cell.ClearAll();
		GetCellIndirect(m_GI->m_lastCol,m_GI->m_lastRow,&m_GI->m_cell);
		GetCellType(m_GI->m_cell.GetCellType())->
			OnKillFocus(m_GI->m_lastCol,m_GI->m_lastRow,&m_GI->m_cell);

		m_GI->m_cell.ClearAll();
		GetCellIndirect(m_GI->m_currentCol,m_GI->m_currentRow,&m_GI->m_cell);
		GetCellType(m_GI->m_cell.GetCellType())->
			OnSetFocus(m_GI->m_currentCol,m_GI->m_currentRow,&m_GI->m_cell);
	}

	if ( m_GI->m_paintMode == TRUE )
	{
		//notify the components of the move
		m_CUGGrid->Moved();
		m_CUGSideHdg->Moved();
		m_CUGTopHdg->Moved();
		m_CUGVScroll->Moved();
		m_CUGHScroll->Moved();
		//m_CUGCnrBtn->Moved();
	}

	//update the last... variables
	m_GI->m_lastTopRow = m_GI->m_topRow;
	m_GI->m_lastRow = m_GI->m_currentRow;
	m_GI->m_lastLeftCol = m_GI->m_leftCol;
	m_GI->m_lastCol = m_GI->m_currentCol;

	int nIndex;
	int nNewRightCol = GetRightCol();
	long nNewBottomRow = GetBottomRow();

	// Inform all affected cells that the view was scrolled
	if ( m_GI->m_lastTopRow != m_GI->m_topRow || nOldBottomRow != nNewBottomRow )
	{
		// Determine direction of the scroll
		if ( m_GI->m_lastTopRow > m_GI->m_topRow || nOldBottomRow > nNewBottomRow )
		{	// user scrolled down, send notifications to 
			for( nIndex = m_GI->m_leftCol; nIndex < nNewRightCol; nIndex ++ )
			{
				// old top row ...
				m_GI->m_cell.ClearAll();
				GetCellIndirect( nIndex, m_GI->m_lastTopRow, &m_GI->m_cell );
				GetCellType(m_GI->m_cell.GetCellType())->OnScrolled( nIndex, m_GI->m_lastTopRow, &m_GI->m_cell );
				// and new bottom row
				m_GI->m_cell.ClearAll();
				GetCellIndirect( nIndex, m_GI->m_lastTopRow, &m_GI->m_cell );
				GetCellType(m_GI->m_cell.GetCellType())->OnScrolled( nIndex, m_GI->m_lastTopRow, &m_GI->m_cell );
			}
		}
		else
		{	// user scrolled up, send notifications to 
			for( nIndex = m_GI->m_leftCol; nIndex < nNewRightCol; nIndex ++ )
			{
				// new top row
				m_GI->m_cell.ClearAll();
				GetCellIndirect( nIndex, m_GI->m_topRow, &m_GI->m_cell );
				GetCellType(m_GI->m_cell.GetCellType())->OnScrolled( nIndex, m_GI->m_topRow, &m_GI->m_cell );
				// and old bottom row
				m_GI->m_cell.ClearAll();
				GetCellIndirect( nIndex, nOldBottomRow, &m_GI->m_cell );
				GetCellType(m_GI->m_cell.GetCellType())->OnScrolled( nIndex, nOldBottomRow, &m_GI->m_cell );
			}
		}
	}
	else if ( m_GI->m_lastLeftCol != m_GI->m_leftCol || nOldRightCol != nNewRightCol )
	{
		// Determine direction of the scroll
		if ( m_GI->m_lastLeftCol > m_GI->m_leftCol || nOldRightCol > nNewRightCol )
		{	// user scrolled right, send notifications to 
			for( nIndex = m_GI->m_topRow; nIndex < nNewBottomRow; nIndex ++ )
			{
				// old left column ...
				m_GI->m_cell.ClearAll();
				GetCellIndirect( m_GI->m_lastLeftCol, nIndex, &m_GI->m_cell );
				GetCellType(m_GI->m_cell.GetCellType())->OnScrolled( m_GI->m_lastLeftCol, nIndex, &m_GI->m_cell );
				// and new right column
				m_GI->m_cell.ClearAll();
				GetCellIndirect( nNewRightCol, nIndex, &m_GI->m_cell );
				GetCellType(m_GI->m_cell.GetCellType())->OnScrolled( nNewRightCol, nIndex, &m_GI->m_cell );
			}
		}
		else
		{	// user scrolled left, send notifications to	
			for( nIndex = m_GI->m_topRow; nIndex < nNewBottomRow; nIndex ++ )
			{
				// new left column ...
				m_GI->m_cell.ClearAll();
				GetCellIndirect( m_GI->m_leftCol, nIndex, &m_GI->m_cell );
				GetCellType(m_GI->m_cell.GetCellType())->OnScrolled( m_GI->m_leftCol, nIndex, &m_GI->m_cell );
				// and old right column
				m_GI->m_cell.ClearAll();
				GetCellIndirect( nOldRightCol, nIndex, &m_GI->m_cell );
				GetCellType(m_GI->m_cell.GetCellType())->OnScrolled( nOldRightCol, nIndex, &m_GI->m_cell );
			}
		}
	}

	// Also send the OnScrolled notification to the current cell.
	m_GI->m_cell.ClearAll();
	GetCellIndirect( m_GI->m_currentCol, m_GI->m_currentRow, &m_GI->m_cell );
	GetCellType(m_GI->m_cell.GetCellType())->OnScrolled( m_GI->m_currentCol, m_GI->m_currentRow, &m_GI->m_cell );
}

/***************************************************
GetCellType
	Purpose
		Returns the pointer to the specified cell type.
		When a cell type is added to the grid it is given
		an index number. This index can be used to look
		up its coresponding pointer
	Params
		type - index value to look up
	Return
		pointer to a cell type, if the index is not
		found then the default cell type pointer
		is returned
****************************************************/
CUGCellType* CUGCtrl::GetCellType(int type){

	if(type <= 0)
		return &m_normalCellType;

	CUGCellType* ct = (CUGCellType*)m_cellTypeList->GetPointer(type);
	if(ct == NULL)
		return &m_normalCellType;
	else
		return ct;
}

/***************************************************
GetCellType
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::GetCellType(CUGCellType* type){
	
	return m_cellTypeList->GetPointerIndex((void*)type);
}

/***************************************************
GetCellType
	Purpose
		Returns the cell type for a given cell
	Params
		col,row - cell to return the cell type of
	Return
		pointer to a cell type, if the cell is not
		found then the default cell type pointer
		is returned
****************************************************/
CUGCellType * CUGCtrl::GetCellType(int col,long row)
{
	m_GI->m_cell.ClearAll();

	if(GetCellIndirect(col,row,&m_GI->m_cell) == UG_SUCCESS)
	{
		if(m_GI->m_cell.IsPropertySet(UGCELL_CELLTYPE_SET))
			return GetCellType(m_GI->m_cell.GetCellType());
		else
			return &m_normalCellType;
	}

	return GetCellType(-1);
}

/***************************************************
****************************************************
	MOVEMENT FUNTIONS    MOVEMENT FUNCTIONS
****************************************************
****************************************************/

/***************************************************
VerifyTopRow
	Purpose
		Checks to see that the new given position of the
		top row is valid, and will adjust it if ness.
	Params
	
	Return

****************************************************/
int CUGCtrl::VerifyTopRow(long* newRow){

	if(*newRow > m_GI->m_maxTopRow)
		*newRow = m_GI->m_maxTopRow;

	if(*newRow < m_GI->m_numLockRows)
		*newRow = m_GI->m_numLockRows;
	
	return UG_SUCCESS;
}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::VerifyCurrentRow(long* newRow){

	if(*newRow >= m_GI->m_numberRows){

		//perform hit bottom
		long rowsfound = 0;
		m_GI->m_defDataSource->OnHitBottom(m_GI->m_numberRows,
			*newRow - m_GI->m_numberRows +1,&rowsfound);
		OnHitBottom(m_GI->m_numberRows,*newRow - m_GI->m_numberRows +1,rowsfound);

		if(m_GI->m_dragRow == m_GI->m_numberRows-1)
			return UG_ERROR;
		else
			*newRow = m_GI->m_numberRows -1;
	}

	if(*newRow < 0)
		*newRow = 0;
	
	if(m_GI->m_enableJoins){
		m_GI->m_cell.ClearAll();
		GetCellIndirect(m_GI->m_dragCol,*newRow,&m_GI->m_cell);
		if(m_GI->m_cell.IsPropertySet(UGCELL_JOIN_SET)){
			int startCol = m_GI->m_dragCol;
			long startRow = *newRow;
			int endCol;
			long endRow;
			GetJoinRange(&startCol,&startRow,&endCol,&endRow);

			if(*newRow > m_GI->m_dragRow){
				if(m_GI->m_dragRow >= startRow){
					if(endRow+1 < m_GI->m_numberRows)
						*newRow = endRow+1;
				}
				else{
					*newRow = startRow;
				}
			}
				else{ 
				if(m_GI->m_dragRow <= endRow){
					if(startRow -1 >0)
						*newRow = startRow-1;
				}
			}
		}
	}

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::VerifyLeftCol(int* newCol){

	if(*newCol > m_GI->m_maxLeftCol)
		*newCol = m_GI->m_maxLeftCol;

	if(*newCol < m_GI->m_numLockCols)
		*newCol = m_GI->m_numLockCols;

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::VerifyCurrentCol(int* newCol){

	if(*newCol >= m_GI->m_numberCols){
		if(m_GI->m_dragCol == m_GI->m_numberCols -1)
			return UG_ERROR;
		else
			*newCol = m_GI->m_numberCols -1;
	}
	if(*newCol == m_GI->m_dragCol)
		return UG_SUCCESS;

	if(*newCol < 0)
		*newCol = 0;

	if(m_GI->m_enableJoins){
		m_GI->m_cell.ClearAll();
		GetCellIndirect(*newCol,m_GI->m_dragRow,&m_GI->m_cell);
		if(m_GI->m_cell.IsPropertySet(UGCELL_JOIN_SET)){
			int startCol = *newCol;
			long startRow = m_GI->m_dragRow;
			int endCol;
			long endRow;
			GetJoinRange(&startCol,&startRow,&endCol,&endRow);

			if(*newCol > m_GI->m_dragCol){
				if(m_GI->m_dragCol >= startCol){
					if(endCol+1 < m_GI->m_numberCols)
						*newCol = endCol+1;
				}
				else{
					*newCol = startCol;
				}
			}
			else{ 
				if(m_GI->m_dragCol <= endCol){
					if(startCol -1 >0)
						*newCol = startCol-1;
				}
			}
		}
	}

	return UG_SUCCESS;
}

/***************************************************
MoveTopRow
	Moves the grid's top row, calls the OnCanViewMove
	first to see if the process is allowed
	Defines are avialable for this function
	'flag'  0-lineup 1-linedown 2-pageup 
			3-pagedown 4-top 5-bottom

  returns 
	1			out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::MoveTopRow(int flag)
{
	// Prevent this function from processing if grid's view scrolling
	// is not possible (number of rows is equal or less than zero).
	if ( m_GI->m_numberRows <= 0 )
		return 1;

    int i = 0;
	switch(flag)
	{
        // if clause added to LINEUP and LINEDOWN to skip 0 height rows.
        // TD 8/4/99
		case UG_LINEUP:{
            if(m_GI->m_topRow != 0)
            {
                i = -1;
                while(GetRowHeight(m_GI->m_topRow + i) == 0)
                    --i;
            }
			return SetTopRow(m_GI->m_topRow +i);
		}
		case UG_LINEDOWN:{
            if(m_GI->m_topRow != (m_GI->m_numberRows-1))
            {
                i = 1;
                while(GetRowHeight(m_GI->m_topRow + i) == 0)
                    ++i;
            }
			return SetTopRow(m_GI->m_topRow +i);
		}
		case UG_TOP:{
			return SetTopRow(0);
		}
		case UG_BOTTOM:{
			return SetTopRow(m_GI->m_numberRows - 1);
		}
		case UG_PAGEUP:{
			int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
			long row = m_GI->m_topRow;
			while(height >=0){
				if(row >=0)
					height -= GetRowHeight(row);
				else
					height -= m_GI->m_defRowHeight;
				row --;
				if(GetRowHeight(row) > height)
					break;
			}
			return SetTopRow(row);
		}
		case UG_PAGEDOWN:{
			int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
			long row = m_GI->m_topRow;
			while(height >=0){
				if(row < m_GI->m_maxTopRow)
					height -= GetRowHeight(row);
				else
					height -= m_GI->m_defRowHeight;
				row ++;
				if(GetRowHeight(row) > height)
					break;
			}
			return SetTopRow(row);
		}
	}
	return UG_SUCCESS;
}

/***************************************************
AdjustTopRow
	Adjusts the position of the top row by the given
	amount (which can be positive or negative).
	OnCanViewMove is called to see if the process is
	allowed

  returns 
	1			out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::AdjustTopRow(long adjust){
	return SetTopRow(m_GI->m_topRow + adjust);
}

/***************************************************
MoveCurrentRow
	Moves the grid's current row, calls the OnCanMove
	first to see if the process is allowed
	Defines are avialable for this function
	'flag'  0-lineup 1-linedown 2-pageup 
			3-pagedown 4-top 5-bottom

   returns 
	1			out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success

****************************************************/
int CUGCtrl::MoveCurrentRow(int flag){
	
	switch(flag){

		case UG_LINEUP:{
			return GotoRow(m_GI->m_dragRow -1);
		}
		case UG_LINEDOWN:{
			return GotoRow(m_GI->m_dragRow +1);
		}
		case UG_TOP:{
			return GotoRow(0);
		}
		case UG_BOTTOM:{
			return GotoRow(m_GI->m_numberRows - 1);
		}
		case UG_PAGEUP:{
			int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
			long row = m_GI->m_dragRow;
			while(height >=0){
				if(row >=0)
					height -= GetRowHeight(row);
				else
					height -= m_GI->m_defRowHeight;
				row --;
				if(GetRowHeight(row) > height)
					break;
			}
			return GotoRow(row);
		}
		case UG_PAGEDOWN:{
			int height = m_GI->m_gridHeight - m_GI->m_lockRowHeight;
			long row = m_GI->m_dragRow;
			while(height >=0){
				if(row < m_GI->m_numberRows)
					height -= GetRowHeight(row);
				else
					height -= m_GI->m_defRowHeight;
				row ++;
				if(GetRowHeight(row) > height)
					break;
			}
			return GotoRow(row);
		}
	}

	return UG_SUCCESS;
}

/***************************************************
AdjustCurrentRow
	Adjusts the position of the current row by the given
	amount (which can be positive or negative).
	OnCanMove is called to see if the process is
	allowed

  returns 
	1			out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::AdjustCurrentRow(long adjust){
	return GotoRow(m_GI->m_dragRow + adjust);
}

/***************************************************
GotoRow
	Moves the current row to the given row.
	OnCanMove is called to see if the process is
	allowed

  returns 
	UG_ERROR	out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::GotoRow(long row){
	
	if(m_GI->m_moveType == 0){  //keyboard
		if(GetKeyState(VK_SHIFT) >=0 && GetKeyState(VK_CONTROL) >=0)
			m_GI->m_multiSelect->GetOrigCell(&m_GI->m_currentCol,&m_GI->m_currentRow);
	}

	if(row == m_GI->m_dragRow)
		return UG_SUCCESS;

	if( VerifyCurrentRow(&row) != UG_SUCCESS)
		return UG_ERROR;
	
	if(row == m_GI->m_dragRow)
		return UG_SUCCESS;

	if(OnCanMove(m_GI->m_dragCol,m_GI->m_dragRow,m_GI->m_dragCol,row) == FALSE)
		return 2;
	
	if( row >= m_GI->m_bottomRow || row < m_GI->m_topRow )
		if( OnCanViewMove( m_GI->m_leftCol, m_GI->m_topRow, m_GI->m_leftCol, row ) == FALSE )
			return 2;
	
	m_GI->m_dragRow = row;

	AdjustTopRow();
	

	Moved();

	return UG_SUCCESS;
}

int CUGCtrl::GotoRowEx(long row)
{
	m_GI->m_moveType = 1;
	int result = GotoRow(row);
	m_GI->m_moveType = 0;
	return result;
}

/***************************************************
GotoRow
Moves the current row to the given row.
OnCanMove is called to see if the process is allowed.

Row is placed as near as possible to the Y pos (top of cell).

returns
UG_ERROR	out of range
2			OnCanMove did not allow the procedure
UG_SUCCESS	success
****************************************************/
int CUGCtrl::GotoRowAndAdjustYpos(long row, int Y)
{
	SetPaintMode(FALSE);

	int result = GotoRow(row);
	if (result != UG_SUCCESS)
	{
		return result;
	}
	std::vector<std::tuple<long, long, long>> tryList;
	CRect rectCell;
	for (int tries = 0; tries < 25; tries++)
	{
		result = GetCellRect(0, row, &rectCell);
		if (result != UG_SUCCESS)
		{
			break;
		}
		long delta = abs(rectCell.top - Y);
		if (delta < 30)
		{
			break;
		}

		long adjustRow = (rectCell.top < Y) ? -1 : 1;

		tryList.push_back(std::tuple<long, long, long>(delta, adjustRow, m_GI->m_topRow));

		if ((result = AdjustTopRow(adjustRow)) != UG_SUCCESS)
		{
			break;
		}

		if (tryList.size() > 4)
		{
			bool isDithering = false;
			long bestTopRow = -1, minDelta = 10000, last4adj[4] = { 0 }, last4Idx = 0;
			for (std::vector<std::tuple<long, long, long>>::reverse_iterator rit 
				= tryList.rbegin(); rit != tryList.rend(); ++rit)
			{
				long delta = std::get<0>(*rit), adjustRow = std::get<1>(*rit), topRow = std::get<2>(*rit);
				if (delta < minDelta)
				{
					minDelta = delta;
					bestTopRow = topRow;
				}

				if (last4Idx < 4)
				{
					last4adj[last4Idx++] = adjustRow;
				}
				else
				{
					isDithering = (last4adj[0] == last4adj[2] && last4adj[1] == last4adj[3]
						&& last4adj[0] != last4adj[1]);
				}
			}

			if (isDithering)
			{
				SetTopRow(bestTopRow);
				break;
			}
		}
	}

	SetPaintMode(TRUE);
	return result;
}

int CUGCtrl::GotoRowAndAdjustYposEx(long row, int Y)
{
	ClearSelections();
	m_GI->m_moveType = 1;	// L button
	int result = GotoRowAndAdjustYpos(row, Y);
	//if (result == UG_SUCCESS)
	//{
	//	m_CUGGrid->SetFocus();
	//}
	m_GI->m_moveType = 0; // Keyboard
	return result;
}

int CUGCtrl::GotoRowAndAdjustYposVcenterEx(long row)
{
	CRect rcClient;
	GetClientRect(rcClient);
	int Ycenter = rcClient.Height() / 2;
	int rowHeight = GetRowHeight(row);
	int Ydesired = Ycenter - (rowHeight / 2);
	return GotoRowAndAdjustYposEx(row, Ydesired);
}

int CUGCtrl::GotoLastRow()
{
	m_GI->m_moveType = 1;
	int result = GotoRow(m_GI->m_numberRows - 1);	// Goto last row.
	m_GI->m_moveType = 0;
	return result;
}

/***************************************************
SetTopRow
	Moves the top row to the given position.
	OnCanViewMoved is called to see if the procedure
	is allowed
  
  returns 
	UG_ERROR	out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::SetTopRow(long row){

	if(row == m_GI->m_topRow)
		return UG_SUCCESS;

	if( VerifyTopRow(&row) != UG_SUCCESS)
		return UG_ERROR;
	
	if( OnCanViewMove( m_GI->m_leftCol, m_GI->m_topRow, m_GI->m_leftCol, row ) == FALSE )
		return 2;
	
	m_GI->m_topRow = row;

	Moved();

	return UG_SUCCESS;
}

/***************************************************
MoveLeftCol
	Moves the grid's left column, calls the OnCanViewMove
	first to see if the process is allowed
	Defines are avialable for this function
	'flag'  0-lineup 1-linedown 2-pageup 
			3-pagedown 4-top 5-bottom

  returns 
	1			out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::MoveLeftCol(int flag)
{
	// Prevent this function from processing if grid's view scrolling
	// is not possible (number of columns is equal or less than zero).
	if ( m_GI->m_numberCols <= 0 )
		return 1;

	switch(flag)
	{
		case UG_COLLEFT:{
			return SetLeftCol(m_GI->m_leftCol -1);
		}
		case UG_COLRIGHT:{
			return SetLeftCol(m_GI->m_leftCol +1);
		}
		case UG_LEFT:{
			return SetLeftCol(0);
		}
		case UG_RIGHT:{
			return SetLeftCol(m_GI->m_numberCols - 1);
		}
		case UG_PAGELEFT:{
			int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
			int col = m_GI->m_leftCol;
			while(width >=0 && col < m_GI->m_numberCols)
			{
				if(col >=0)
					width -= GetColWidth(col);
				else
					width -= m_GI->m_defColWidth;
				col --;
			}
			return SetLeftCol(col);
		}
		case UG_PAGERIGHT:{
			int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
			int col = m_GI->m_leftCol;
			while(width >=0 && col < m_GI->m_numberCols)
			{
				if(col >=0)
					width -= GetColWidth(col);
				else
					width -= m_GI->m_defColWidth;
				col ++;
			}
			return SetLeftCol(col);
		}
	}
	return UG_SUCCESS;
}
/***************************************************
AdjustLeftCol
	Adjusts the position of the left column by the given
	amount (which can be positive or negative).
	OnCanViewMove is called to see if the process is
	allowed

  returns 
	1			out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::AdjustLeftCol(int adjust){
	return SetLeftCol(m_GI->m_leftCol + adjust);
}
/***************************************************
MoveCurrentCol
	Moves the grid's current column, calls the OnCanMove
	first to see if the process is allowed
	Defines are avialable for this function
	'flag'  0-lineup 1-linedown 2-pageup 
			3-pagedown 4-top 5-bottom

  returns 
	1			out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::MoveCurrentCol(int flag){

	switch(flag){

		case UG_COLLEFT:{
			return GotoCol(m_GI->m_dragCol -1);
		}
		case UG_COLRIGHT:{
			return GotoCol(m_GI->m_dragCol +1);
		}
		case UG_LEFT:{
			return GotoCol(0);
		}
		case UG_RIGHT:{
			return GotoCol(m_GI->m_numberCols - 1);
		}
		case UG_PAGELEFT:{
			int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
			int col = m_GI->m_dragCol;
			while(width >=0){
				if(col >=0)
					width -= GetColWidth(col);
				else
					width -= m_GI->m_defColWidth;
				col --;
			}
			return GotoCol(col);
		}
		case UG_PAGERIGHT:{
			int width = m_GI->m_gridWidth - m_GI->m_lockColWidth;
			int col = m_GI->m_dragCol;
			while(width >=0){
				if(col >=0)
					width -= GetColWidth(col);
				else
					width -= m_GI->m_defColWidth;
				col ++;
			}
			return GotoCol(col);
		}
	}

	return UG_SUCCESS;
}
/***************************************************
AdjustCurrentCol
	Adjusts the position of the current column by the given
	amount (which can be positive or negative).
	OnCanMove is called to see if the process is
	allowed

  returns 
	1			out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::AdjustCurrentCol(int adjust){
	return GotoCol(m_GI->m_dragCol + adjust);
}
/***************************************************
GotoCol
	Moves the current column to the specified postion
	OnCanMove is called to see if the process is allowed

  returns 
	UG_ERROR	out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::GotoCol(int col){

	if(m_GI->m_moveType == 0){  //keyboard
		if(GetKeyState(VK_SHIFT) >=0 && GetKeyState(VK_CONTROL) >=0)
			m_GI->m_multiSelect->GetOrigCell(&m_GI->m_currentCol,&m_GI->m_currentRow);
	}

	if(col == m_GI->m_dragCol)
		return UG_SUCCESS;

	if( VerifyCurrentCol(&col) != UG_SUCCESS)
		return UG_ERROR;
	
	if(col == m_GI->m_dragCol)
		return UG_SUCCESS;

	// TD call the datasource...
	if(m_GI->m_defDataSource->OnCanMove(m_GI->m_dragCol,m_GI->m_dragRow,col,m_GI->m_dragRow) != TRUE)
		return 2;

	if(OnCanMove(m_GI->m_dragCol,m_GI->m_dragRow,col,m_GI->m_dragRow) == FALSE)
		return 2;
	
	if( col >= m_GI->m_rightCol || col < m_GI->m_leftCol )
		if( OnCanViewMove( m_GI->m_leftCol, m_GI->m_topRow, col, m_GI->m_topRow ) == FALSE )
			return 2;
	
	m_GI->m_dragCol = col;

	AdjustLeftCol();

	Moved();

	return UG_SUCCESS;
}
/***************************************************
SetLeftCol
	Moves the left column to the specified postion
	OnCanViewMove is called to see if the process is allowed
  returns 
	1			out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::SetLeftCol(int col){

	if(col == m_GI->m_leftCol)
		return UG_SUCCESS;

	if( VerifyLeftCol(&col) != UG_SUCCESS)
		return UG_ERROR;
	
	if( OnCanViewMove( m_GI->m_leftCol, m_GI->m_topRow, col, m_GI->m_topRow ) == FALSE )
		return 2;
	
	if(m_GI->m_leftCol == col)
		return UG_SUCCESS;


	m_GI->m_leftCol = col;
	Moved();

	return UG_SUCCESS;
}
/***************************************************
GotoCell
	Moves the current row/column to the specified
	position
	OnCanMove is called to see if the process is allowed
  returns 
	UG_ERROR	out of range
	2			OnCanMove did not allow the procedure
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::GotoCell(int col,long row)
{	
	if(col == m_GI->m_dragCol && row == m_GI->m_dragRow)
	{
		// the user clicked on current cell, there is no need to process this message
		// since move action is not required, return with success
		if ( GetKeyState( VK_SHIFT ) < 0 && m_GI->m_multiSelectFlag > 0 )
		{	// in a special case, when user clicks on current cell
			// holding down the SHIFT key with multiselection turned on.
			// This action should clear current block to just one cell
			// therefore the multiselect class needs to be notified.
			m_GI->m_multiSelect->OnLClick( col, row, m_GI->m_moveFlags );
			RedrawAll();
		}

		return UG_SUCCESS;
	}

	if(m_GI->m_enableJoins)
		GetJoinStartCell(&col,&row);
	
	// first the datasource...
	if(m_GI->m_defDataSource->OnCanMove(m_GI->m_currentCol,m_GI->m_currentRow,col,row) != TRUE)
		return 2;

	if(OnCanMove(m_GI->m_currentCol,m_GI->m_currentRow,col,row) == FALSE)
		return 2;
	
	if( col >= m_GI->m_rightCol || col < m_GI->m_leftCol || row >= m_GI->m_bottomRow || row < m_GI->m_topRow )
		if( OnCanViewMove( m_GI->m_leftCol, m_GI->m_topRow, col, row ) == FALSE )
			return 2;
	
	m_GI->m_dragCol = col;
	m_GI->m_dragRow = row;

	AdjustLeftCol();
	AdjustTopRow();

	Moved();

	return UG_SUCCESS;
}
/***************************************************
GotoCell
Moves the current row/column to the specified position
OnCanMove is called to see if the process is allowed
returns
UG_ERROR	out of range
2			OnCanMove did not allow the procedure
UG_SUCCESS	success
****************************************************/
int CUGCtrl::GotoCellEx(int col, long row)
{
	if (m_GI->m_enableJoins)
		GetJoinStartCell(&col, &row);

	// first the datasource...
	if (m_GI->m_defDataSource->OnCanMove(m_GI->m_currentCol, m_GI->m_currentRow, col, row) != TRUE)
		return 2;

	if (OnCanMove(m_GI->m_currentCol, m_GI->m_currentRow, col, row) == FALSE)
		return 2;

	if (col >= m_GI->m_rightCol || col < m_GI->m_leftCol || row >= m_GI->m_bottomRow || row < m_GI->m_topRow)
		if (OnCanViewMove(m_GI->m_leftCol, m_GI->m_topRow, col, row) == FALSE)
			return 2;

	m_GI->m_dragCol = col;
	m_GI->m_dragRow = row;

	AdjustLeftCol();
	AdjustTopRow();

	Moved();

	return UG_SUCCESS;
}

void CUGCtrl::HScroll(UINT nSBCode, UINT nPos)
{
	m_CUGHScroll->HScroll(nSBCode, nPos);
}

/***************************************************
GetCellFromPoint
	Returns the column and row that lies over the
	given x,y co-ordinates. The co-ordinates are
	relative to the top left hand corner of the grid	

  Return
	UG_ERROR	co-ords do not fall on a cell
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::GetCellFromPoint(int x,int y,int *ptcol,long *ptrow){
	
	RECT rect;	
	return GetCellFromPoint(x,y,ptcol,ptrow,&rect);
}
/***************************************************
GetCellFromPoint
	Returns the column and row that lies over the
	given x,y co-ordinates. The co-ordinates are
	relative to the top left hand corner of the grid	
	area (not including the top headings);

	This function also retuns the rectangle of the
	cell that was found at that location

  Return
	UG_ERROR	co-ords do not fall on a cell
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::GetCellFromPoint(int px,int py,int *ptcol,long *ptrow,RECT *rect){

	int ptsFound = 0;

	int x;
	long y;

	if(px < 0 || px > m_GI->m_gridWidth)
		return UG_ERROR;
	if(py < 0 || py > m_GI->m_gridHeight)
		return UG_ERROR;

	int width	= 0;
	int height	= 0;

	//find the col
	for(x=0;x<m_GI->m_numberCols;x++){
		
		if(x == m_GI->m_numLockCols)
			x = m_GI->m_leftCol;
		
		width += GetColWidth(x);

		if(width > px){
			*ptcol = x;
			rect->right = width;
			rect->left = rect->right - GetColWidth(x);
			ptsFound ++;
			break;
		}
	}

	//find the row
	for(y=0;y<m_GI->m_numberRows;y++){
		
		if(y == m_GI->m_numLockRows)
			y = m_GI->m_topRow;
		
		height += GetRowHeight(y);

		if(height > py){
			*ptrow = y;
			rect->bottom = height;
			rect->top = rect->bottom - GetRowHeight(y);
			ptsFound ++;
			break;
		}
	}

	//if the cell was not found then return UG_ERROR
	if(ptsFound != 2)
		return UG_ERROR;

	//if cell joining then check to see if the see is joined
	if(m_GI->m_enableJoins){
		m_GI->m_cell.ClearAll();
		GetCellIndirect(x,y,&m_GI->m_cell);
		if(m_GI->m_cell.IsPropertySet(UGCELL_JOIN_SET)){
			GetCellRect(ptcol,ptrow,rect);
		}
	}
	return UG_SUCCESS;
}
/***************************************************
Does not take cell joining into account
****************************************************/
int CUGCtrl::GetAbsoluteCellFromPoint(int x,int y,int *ptcol,long *ptrow){

	int tempJoinFlag = m_GI->m_enableJoins;

	RECT rect;
	int rt = GetCellFromPoint(x,y,ptcol,ptrow,&rect);

	m_GI->m_enableJoins = tempJoinFlag;

	return rt;
}
/***************************************************
GetCellRect
	Returns the rectangle of the given cell.
	Relative to the top-left corner of the grid
	area (not including the top headings);
  Return
	UG_ERROR	co-ords out of range
	UG_SUCCESS	success
****************************************************/
int CUGCtrl::GetCellRect(int col,long row,RECT *rect){
	return GetCellRect(&col,&row,rect);	
}
/***************************************************
GetCellRect
int CUGCtrl::GetCellRect(int *col,long *row,RECT *rect);

	Purpose
		Returns the rectangle of the given cell relative 
		to the top-left corner of the grid area (not 
		including the top/side headings). This function
		takes cell joining and locked regions into account
	Params
		col	- (input) column to return the rect of
			  (output) the start cell of a joined cell 
			  region if the specified col/row points to
			  a cell within the region
		row	- (input) row to return the rect of
			  (output) the start cell of a joined cell
			  region if the specified col/row points to 
			  a cell witin the region
		rect -  (output) returns the rectange of the specified 
				cell in pixels. Joins and locked regions are
				taken into account
	Return
		UG_SUCCESS	success
		UG_ERROR	cell not visible
****************************************************/
int CUGCtrl::GetCellRect(int *col,long *row,RECT *rect)
{
	if ( *col < 0 )
		return m_CUGSideHdg->GetCellRect( col, row, rect );
	else if ( *row < 0 )
		return m_CUGTopHdg->GetCellRect( col, row, rect );

	int x;
	long y;
	int width =0;
	int height =0;

	int startCol	= *col;
	int endCol		= *col;
	long startRow	= *row;
	long endRow		= *row;

	rect->left	= 0;
	rect->top	= 0;
	rect->right	= 0;
	rect->bottom	= 0;

	//if the specified cell is within a join then find the joined range
	if(m_GI->m_enableJoins)
		if(GetJoinRange(&startCol,&startRow,&endCol,&endRow) == UG_SUCCESS){
			*col = startCol;
			*row = startRow;
		}


	//find the col
	if(startCol >= m_GI->m_numLockCols){//if the col is not within the lock region
		rect->left = m_GI->m_lockColWidth;
		rect->right = m_GI->m_lockColWidth;
	}
	for(x=0;x<m_GI->m_numberCols;x++){
	
		if(x == m_GI->m_numLockCols)
			x = m_GI->m_leftCol;		
		
		if(x == startCol)
			rect->left = width;
		
		width += GetColWidth(x);

		if(x == endCol){
			rect->right = width;
			break;
		}
	}

	//find the row
	if(startRow >= m_GI->m_numLockRows){ //if the row is not within the lock region
		rect->top		= m_GI->m_lockRowHeight;
		rect->bottom	= m_GI->m_lockRowHeight;
	}
	for(y=0;y<m_GI->m_numberRows;y++){
		
		if(y == m_GI->m_numLockRows)
			y = m_GI->m_topRow;
		
		if(y == startRow)
			rect->top = height;

		height += GetRowHeight(y);

		if(y == endRow){
			rect->bottom = height;
			break;
		}
	}

	return UG_SUCCESS;
}

/***************************************************
GetRangeRect
	returns the rectangle for a range of cells
****************************************************/
int CUGCtrl::GetRangeRect(int startCol,long startRow,int endCol,long endRow,RECT *rect){


	int x;
	long y;
	int width =0;
	int height =0;

	rect->left		= - 10;
	rect->top		= - 10;
	rect->right		= m_GI->m_gridWidth + 10;
	rect->bottom	= m_GI->m_gridHeight + 10;

	//find the col
	if(startCol >= m_GI->m_numLockCols && m_GI->m_numLockCols >0){//if the col is not within the lock region
		rect->left = m_GI->m_lockColWidth;
	}
	for(x=0;x<m_GI->m_numberCols;x++){
	
		if(x == m_GI->m_numLockCols)
			x = m_GI->m_leftCol;		
		
		if(x == startCol)
			rect->left = width;
		
		width += GetColWidth(x);

		if(x == endCol){
			rect->right = width;
			break;
		}
		if(width > m_GI->m_gridWidth)
			break;
	}

	//find the row
	if(startRow >= m_GI->m_numLockRows && m_GI->m_numLockRows >0){ //if the row is not within the lock region
		rect->top		= m_GI->m_lockRowHeight;
	}
	for(y=0;y<m_GI->m_numberRows;y++){
		
		if(y == m_GI->m_numLockRows)
			y = m_GI->m_topRow;
		
		if(y == startRow)
			rect->top = height;

		height += GetRowHeight(y);

		if(y == endRow){
			rect->bottom = height;
			break;
		}
		if(height > m_GI->m_gridHeight)
			break;
	}

	return UG_SUCCESS;
}
/***************************************************
returns UG_SUCCESS if the specified cell is part of 
a block, also returns the col and row of the starting
cell of that block
****************************************************/
int CUGCtrl::GetJoinStartCell(int *col,long *row){
	return GetJoinStartCell(col,row,&m_GI->m_cell);
}
/***************************************************
returns UG_SUCCESS if the specified cell is part of 
a block, also returns the col and row of the starting
cell of that block
****************************************************/
int CUGCtrl::GetJoinStartCell(int *col,long *row,CUGCell *cell){
	
	if(m_GI->m_enableJoins == FALSE)
		return UG_ERROR;

	GetCellIndirect(*col,*row,cell);
	if(cell->IsPropertySet(UGCELL_JOIN_SET) == FALSE)
		return 2;

	int startCol;
	long startRow;
	BOOL origin;
	
	cell->GetJoinInfo(&origin,&startCol,&startRow);

	if(!origin){
		*col += startCol;
		*row += startRow;
		GetCellIndirect(*col,*row,cell);
	}

	return UG_SUCCESS;
}
/***************************************************

returns UG_SUCCESS if the specified cell is part of 
a block, also returns the col and row of the starting
and ending cells of that block
col and row point to a cell within the join on input
and contain the start cell on output if successful
****************************************************/
int CUGCtrl::GetJoinRange(int *col,long *row,int *col2,long *row2){

	if(m_GI->m_enableJoins == FALSE)
		return UG_ERROR;

	m_GI->m_cell.ClearAll();
	GetCellIndirect(*col,*row,&m_GI->m_cell);
	if(m_GI->m_cell.IsPropertySet(UGCELL_JOIN_SET) == FALSE)
		return 2;

	int startCol;
	long startRow;
	BOOL origin;
	
	m_GI->m_cell.GetJoinInfo(&origin,&startCol,&startRow);

	if(!origin){
		*col += startCol;
		*row += startRow;
		m_GI->m_cell.ClearAll();
		GetCellIndirect(*col,*row,&m_GI->m_cell);
	}

	m_GI->m_cell.GetJoinInfo(&origin,col2,row2);
	*col2+= *col;
	*row2+= *row;
	
	return UG_SUCCESS;
}
/***************************************************
EnableJoins
	Purpose
	Params
	Return
		UG_SUCCESS(0)	- success
		UG_ERROR(1)		- error 
****************************************************/
int CUGCtrl::EnableJoins(BOOL state){
	if(state)
		m_GI->m_enableJoins = TRUE;
	else
		m_GI->m_enableJoins = FALSE;

	return UG_SUCCESS;
}
/***************************************************
EnableCellOverLap
	Purpose
	Params
	Return
		UG_SUCCESS(0)	- success
		UG_ERROR(1)		- error 
****************************************************/
//int CUGCtrl::EnableCellOverLap(BOOL state){
//	if(state)
//		m_GI->m_enableCellOverLap = TRUE;
//	else
//		m_GI->m_enableCellOverLap = FALSE;
//
//	return UG_SUCCESS;
//}
/***************************************************
EnableColSwapping
	Purpose
	Params
	Return
		UG_SUCCESS(0)	- success
		UG_ERROR(1)		- error 
****************************************************/
int CUGCtrl::EnableColSwapping(BOOL state){
	if(state)
		m_GI->m_enableColSwapping = TRUE;
	else
		m_GI->m_enableColSwapping = FALSE;

	return UG_SUCCESS;
}

/***************************************************
EnableExcelBorders
	Purpose
	Params
	Return
		UG_SUCCESS(0)	- success
		UG_ERROR(1)		- error 
****************************************************/
int CUGCtrl::EnableExcelBorders(BOOL state){
	if(state)
		m_GI->m_enableExcelBorders = TRUE;
	else
		m_GI->m_enableExcelBorders = FALSE;

	return UG_SUCCESS;
}

/***************************************************
EnableScrollOnParialCells
	Purpose
		The 'm_bScrollOnParialCells' flag controls the scrolling behavior
		when the mouse is dragged over partially visible cell.  By default
		we will treat this area same as area 'outside of the grid' meaning
		that the grid will scroll using current ballistics settigs.
		You can set this flag to FALSE if you do not want the grid to
		scroll when user's mouse is over the partially visible cells,
		the grid will only scroll when mouse is dragged outside of the view.
	Params
		state - turn on and off scrolling on the partially
				visible cells.
	Return
		UG_SUCCESS(0)	- this function will never fail
****************************************************/
int CUGCtrl::EnableScrollOnParialCells( BOOL state )
{
	m_GI->m_bScrollOnParialCells = state;

	return UG_SUCCESS;
}

/***************************************************
****************************************************
	EDITING FUNCTIONS
****************************************************
****************************************************/

/***************************************************
StartEdit
	Starts editing mode - if the current cell is
	not readonly and the cell type of the current
	cell allows editing
	An OnStartEdit notification is also sent that
	can allow or dis-allow the editing

  return
	UG_ERROR	editing not allowed
	UG_SUCCESS	success
****************************************************/
int	CUGCtrl::StartEdit(){
	return StartEdit(m_GI->m_currentCol,m_GI->m_currentRow,0);
}
/***************************************************
StartEdit
	Starts editing mode - if the current cell is
	not readonly and the cell type of the current
	cell allows editing
	An OnStartEdit notification is also sent that
	can allow or dis-allow the editing
	'key' allows to have a keystroke stuffed into
	the edit control when is starts, useful for 
	when editing starts because of a OnCharDown
	notification

  return
	UG_ERROR	editing not allowed
	UG_SUCCESS	success
****************************************************/
int	CUGCtrl::StartEdit(int key){

	return StartEdit(m_GI->m_currentCol,m_GI->m_currentRow,key);
}
/***************************************************
StartEdit
	Starts editing mode - if the specified cell is
	not readonly and the cell type of the specified
	cell allows editing
	An OnStartEdit notification is also sent that
	can allow or dis-allow the editing
	'key' allows to have a keystroke stuffed into
	the edit control when is starts, useful for 
	when editing starts because of a OnCharDown
	notification
	'col','row' specifies the cell to edit

  return
	UG_ERROR	editing not allowed
	2			could not move to cell
	UG_SUCCESS	success
****************************************************/
int	CUGCtrl::StartEdit(int col,long row,int key)
{
	// Do not allow to continue if the edit is already started
	if (m_GI->m_editInProgress == TRUE )
		return UG_ERROR;

	m_GI->m_editInProgress = TRUE;

	//clear any multiple sections
	int startCol,endCol;
	long startRow,endRow;

	startCol = m_GI->m_currentCol;
	endCol   = m_GI->m_currentCol;
	startRow = m_GI->m_currentRow;
	endRow	 = m_GI->m_currentRow;

	m_GI->m_editCol = col;
	m_GI->m_editRow = row;
	m_GI->m_editCell.ClearAll();

	m_GI->m_multiSelect->GetTotalRange(&startCol,&startRow,&endCol,&endRow);
	ClearSelections();
	if(startCol!=endCol || startCol!= m_GI->m_editCol || startRow!=endRow || startRow!= m_GI->m_editRow)
		RedrawRange(startCol,startRow,endCol,endRow);

	if(m_GI->m_editCol != m_GI->m_currentCol || m_GI->m_editRow != m_GI->m_currentRow)
	{
		if(GotoCell(m_GI->m_editCol, m_GI->m_editRow) != UG_SUCCESS)
		{
			m_GI->m_editInProgress = FALSE;
		}
	}

	if (m_GI->m_editInProgress == FALSE )
		return 2;
	
	GetCellIndirect(m_GI->m_editCol, m_GI->m_editRow, &m_GI->m_editCell);
	
	//check to see if this cell type is editable
	CUGCellType * ct;
	if(m_GI->m_editCell.IsPropertySet(UGCELL_CELLTYPE_SET))
		ct = GetCellType(m_GI->m_editCell.GetCellType());
	else
		ct = GetCellType(0);
	if(ct->CanTextEdit() == FALSE){
		m_GI->m_editInProgress = FALSE;
		return UG_ERROR;
	}

	CRect rect;
	if(col >= 0 && row >= 0){  //grid editing
		GetCellRect(m_GI->m_editCol, m_GI->m_editRow,&rect);
		m_GI->m_editParent = m_CUGGrid->GetSafeHwnd();
	}
	if(col < 0 && row >= 0){	//side heading editing
		m_CUGSideHdg->GetCellRect(m_GI->m_editCol, m_GI->m_editRow,&rect);
		m_GI->m_editParent = m_CUGSideHdg->GetSafeHwnd();
	}
	if(col >= 0 && row < 0){	//top heading editing
		m_CUGTopHdg->GetCellRect(m_GI->m_editCol, m_GI->m_editRow,&rect);
		m_GI->m_editParent = m_CUGTopHdg->GetSafeHwnd();
	}

	rect.right--;
	rect.bottom--;

	//get the edit area
	ct->GetEditArea(&rect);

	if(m_GI->m_editCell.GetReadOnly()){
		m_GI->m_editInProgress = FALSE;
		return UG_ERROR;
	}

	//set the pointer to the current edit control
	//only use this var from now on as it may point to another
	//control after OnEditStart is called
	//m_GI->m_editCtrl = m_GI->m_editCtrl;

	//check to see which edit control to use
	//if(m_GI->m_editCell.IsPropertySet(UGCELL_MASK_SET) ){

	//	m_GI->m_editCtrl = m_GI->m_maskedEditCtrl;

	//	//if the standard edit control was changed with 
	//	//SetNewEditClass, then use that control
	//	//if(m_GI->m_editCtrl->m_hWnd != m_defEditCtrl.m_hWnd)
	//	//	m_GI->m_editCtrl = m_GI->m_editCtrl;

	//	//if the default masked edit control was changed with
	//	//SetNewMaskedEditClass, then use that control
	//	if(m_GI->m_maskedEditCtrl->m_hWnd != m_defMaskedEditCtrl.m_hWnd)
	//		m_GI->m_editCtrl = m_GI->m_maskedEditCtrl;
	//}
	
	// TD call the datasource first..
	if(m_GI->m_defDataSource->OnEditStart(m_GI->m_editCol, m_GI->m_editRow, m_GI->m_editCtrl) != FALSE)
	{
		if(OnEditStart(m_GI->m_editCol, m_GI->m_editRow, m_GI->m_editCtrl) != FALSE)
		{	
			m_GI->m_editInProgress = TRUE;

			if ( m_GI->m_editCell.GetDataType() == UGCELLDATA_NUMBER && !( m_GI->m_editCell.GetPropertyFlags()&UGCELL_DONOT_LOCALIZE ))
				m_GI->m_editCell.SetPropertyFlags( m_GI->m_editCell.GetPropertyFlags()|UGCELL_DONOT_LOCALIZE );

			std::wstring strFixed = FixLineEndingsW(m_GI->m_editCell.GetText());
			::SetWindowTextW(m_GI->m_editCtrl, strFixed.c_str());

			//HFONT font = m_GI->m_xeUI->GetFontHandle(EXE_FONT::eUI_Font);
			//if (m_GI->m_editCell.IsPropertySet(UGCELL_FONT_SET))
			//	::SendMessageW(m_GI->m_editCtrl, WM_SETFONT, (WPARAM)font, TRUE);
			//	//m_GI->m_editCtrl->SetFont(m_GI->m_xeUI->GetFont(EXE_FONT::eUI_Font));

			::SetParent(m_GI->m_editCtrl, m_GI->m_editParent);
			
			::SetWindowPos(m_GI->m_editCtrl, HWND_TOP, rect.left, rect.top,
				rect.right - rect.left, rect.bottom - rect.top, 0);
			//m_GI->m_editCtrl->SetWindowPos(/*HWND_TOP*/&CWnd::wndTop,rect.left,rect.top,
			//	rect.right-rect.left,rect.bottom-rect.top,0);

			::ShowWindow(m_GI->m_editCtrl, SW_SHOW);

			::SetActiveWindow(m_GI->m_editCtrl);
			::SetFocus(m_GI->m_editCtrl);

			//stuff a key stroke in
			if(key >0){
				::SendMessage(m_GI->m_editCtrl, WM_CHAR, key, 0);
			}

			return UG_SUCCESS;
		}
	}

	m_GI->m_editInProgress = FALSE;
	RedrawCell( col, row );

	return UG_ERROR;
}

/***************************************************
ContinueEdit

	If already in edit mode this function moves the
	editing to another cell using the given adjustments
	if the specified cell is not readonly and the cell 
	type of the specified cell allows editing
	An OnStartEdit notification is also sent that
	can allow or dis-allow the editing

  return
	UG_ERROR	editing not allowed
	UG_SUCCESS	success
****************************************************/
int	CUGCtrl::ContinueEdit(int adjustCol,long adjustRow){
	UNREFERENCED_PARAMETER(adjustCol);
	UNREFERENCED_PARAMETER(adjustRow);
	return UG_SUCCESS;
}

/***************************************************
****************************************************
	GET/SET FUNTIONS    GET/SET FUNCTIONS
****************************************************
****************************************************/

/***************************************************
GetCell
	Fills the given cell class with the information
	from the cell at the given position

  return
		UG_SUCCESS	success
		UG_ERROR	cell not found
****************************************************/
int CUGCtrl::GetCell(int col,long row,CUGCell * cell){

	if(col >= m_GI->m_numberCols)
		return UG_ERROR;

	CUGDataSource * ds = NULL;

	cell->SetPropertyFlags(0);

	//column translation only is performed on cols >= 0
	if(col >=0 && col < m_GI->m_numberCols)
		ds = m_GI->m_colInfo[col].dataSource;
	if(ds == NULL)
		ds = m_GI->m_defDataSource;

	//get the cell from the datasource
	if(col >=0 && col < m_GI->m_numberCols)
		return ds->GetCell(m_GI->m_colInfo[col].colTranslation,row,cell);
	else
		return ds->GetCell(col,row,cell);
}
/***************************************************
GetCellIndirect
	Fills the given cell class with the information
	from the cell at the given position, this function
	starts by filling the cell with the default 
	informaiton for the specified column, then it
	over-writes any information that is 'set' in the
	specified cell.

  return
		UG_SUCCESS	success
		UG_ERROR	cell not found
****************************************************/
int	CUGCtrl::GetCellIndirect(int col,long row,CUGCell *cell){
	
	if (col >= m_GI->m_numberCols)
		return UG_ERROR;

	cell->SetPropertyFlags(0);

	//get the column default
	if(col >= 0 && row >= 0 && col < m_GI->m_numberCols){
		if(m_GI->m_colInfo[col].colDefault != NULL)
			cell->CopyInfoFrom(m_GI->m_colInfo[col].colDefault);
	}
	else{
		cell->CopyInfoFrom(m_GI->m_hdgDefaults);
	}
	
	if(row >= m_GI->m_numberRows)
		return UG_SUCCESS;

	//find the datasource
	CUGDataSource * ds = NULL;
	if(col >=0 && col < m_GI->m_numberCols)
		ds = m_GI->m_colInfo[col].dataSource;
	if(ds == NULL)
		ds = m_GI->m_defDataSource;

	//get the cell from the datasource
	if(col >=0 && col < m_GI->m_numberCols){
		ds->GetCell(m_GI->m_colInfo[col].colTranslation,row,cell);
		OnGetCell(m_GI->m_colInfo[col].colTranslation,row,cell);
	}
	else{
		ds->GetCell(col,row,cell);
		OnGetCell(col,row,cell);
	}

	//use format class
	if(cell->IsPropertySet(UGCELL_FORMAT_SET)){
		cell->GetFormatClass()->ApplyDisplayFormat(cell);
	}
	//use cell style object
	if(cell->IsPropertySet(UGCELL_STYLE_SET)){
		cell->GetCellStyle()->AddInfoTo(cell);
	}

	return UG_SUCCESS;
}

int CUGCtrl::GetRowLineCount(long row)
{
	if (row >= m_GI->m_numberRows)
		return UG_SUCCESS;

	return OnGetRowLineCount(row);
}

/***************************************************
SetCell
	Copies the given cell to the cell at the specified
	position. The information that is saved depends
	on the capabilities of the data source.
	(ex. the memory manager saves all the information,
	 a basic database source will only save the text).

  return
	UG_SUCCESS	success
	UG_ERROR	cell not found
****************************************************/
int	CUGCtrl::SetCell(int col,long row,CUGCell *cell){

	// make sure that the column specified is within range
	if ( col < ( m_GI->m_numberSideHdgCols * -1 ) || col > m_GI->m_numberCols )
		return UG_ERROR;
	// make sure that the row specified is within range
	if ( row < ( m_GI->m_numberTopHdgRows * -1 ) || row > m_GI->m_numberRows )
		return UG_ERROR;

	OnSetCell(col,row,cell);

	//find the datasource
	CUGDataSource * ds = NULL;
	if(col >=0)
		ds = m_GI->m_colInfo[col].dataSource;
	if(ds == NULL)
		ds = m_GI->m_defDataSource;

	//set the cell in the datasource
	//get the cell from the datasource
	if(col >=0)
		ds->SetCell(m_GI->m_colInfo[col].colTranslation,row,cell);
	else
		ds->SetCell(col,row,cell);

	return UG_SUCCESS;
}

/***************************************************
****************************************************/
int	CUGCtrl::GetColTranslation(int col){

	if(col <0 || col >= m_GI->m_numberCols)
		return col;

	return m_GI->m_colInfo[col].colTranslation;
}
/***************************************************
****************************************************/
int	CUGCtrl::SetColTranslation(int col,int transCol){
	
	if(col <0 || col >= m_GI->m_numberCols)
		return UG_ERROR;

	m_GI->m_colInfo[col].colTranslation = transCol;
	
	return UG_SUCCESS;
}

/***************************************************
DeleteCell
	Deletes the specified cell
  Return 
	UG_SUCCESS  success
	UG_ERROR	cell no found
****************************************************/
int	CUGCtrl::DeleteCell(int col,long row){

	//find the datasource
	CUGDataSource * ds = NULL;
	if(col >=0)
		ds = m_GI->m_colInfo[col].dataSource;
	if(ds == NULL)
		ds = m_GI->m_defDataSource;

	return ds->DeleteCell(m_GI->m_colInfo[col].colTranslation,row);
}

/***************************************************
SetColDefault
	Sets the column default values. These values are
	used when the grid draws a cell, the defaults are
	retrieved first then only the informtion that
	the cell being drawn contains is copied overtop
	of the defauls

	return
		UG_SUCCESS success
		UG_ERROR	fail
****************************************************/
int	CUGCtrl::SetColDefault(int col,CUGCell *cell){

	if(col <0 || col >= m_GI->m_numberCols)
		return UG_ERROR;

	cell->CopyInfoTo(m_GI->m_colInfo[col].colDefault);

	return UG_SUCCESS;
}

/***************************************************
SetGridDefault
	Sets the grid default values. These values are
	used when new columns are made (SetNumberCols)
	they are copied to the new column defaults
	(see above)

	return
		UG_SUCCESS success
		UG_ERROR	fail
****************************************************/
int	CUGCtrl::SetGridDefault(CUGCell *cell){

	cell->CopyInfoTo(m_GI->m_gridDefaults);
	
	return UG_SUCCESS;
}

/***************************************************
GetColDefault
	copies the given column defaults into the supplied
	cell

  return
		UG_SUCCESS  success
		UG_ERROR	fail
****************************************************/
int	CUGCtrl::GetColDefault(int col,CUGCell *cell){
	
	if(col <0 || col >= m_GI->m_numberCols)
		return UG_ERROR;

	cell->CopyInfoFrom(m_GI->m_colInfo[col].colDefault);

	return UG_SUCCESS;
}
/***************************************************
GetGridDefault
	copies the grid defaults into the supplied
	cell

  return
		UG_SUCCESS  success
		UG_ERROR	fail
****************************************************/
int	CUGCtrl::GetGridDefault(CUGCell *cell){
	cell->CopyInfoFrom(m_GI->m_gridDefaults);
	return UG_SUCCESS;
}

/***************************************************
SetHeadingDefault
	copies the given cell properties to the heading
	defaults. Heading defaults are used when a
	top and side headings are being drawn ( as well
	as the top corner button). These values are applied
	first, then any properties set for the heading cell
	being drawn are applied overtop

  return
		UG_SUCCESS  success
		UG_ERROR	fail
****************************************************/
int	CUGCtrl::SetHeadingDefault(CUGCell *cell){
	m_GI->m_hdgDefaults->CopyInfoFrom(cell);
	return UG_SUCCESS;
}

/***************************************************
GetHeadingDefault
	Fills the given cell with the heading default
	properties

  return
		UG_SUCCESS  success
		UG_ERROR	fail
****************************************************/
int	CUGCtrl::GetHeadingDefault(CUGCell *cell){
	cell->CopyInfoFrom(m_GI->m_hdgDefaults);
	return UG_SUCCESS;
}

/***************************************************
****************************************************/
int	CUGCtrl::JoinCells(int startCol,long startRow,int endCol,long endRow){

	if(startCol > endCol)
		return UG_ERROR;
	if(startRow > endRow)
		return UG_ERROR;
	if(endCol >= m_GI->m_numberCols)
		return UG_ERROR;
	if(endRow >= m_GI->m_numberRows)
		return UG_ERROR;
	if(startCol < (m_GI->m_numberSideHdgCols *-1))
		return UG_ERROR;
	if(startRow < (m_GI->m_numberTopHdgRows *-1))
		return UG_ERROR;

	int x;
	long y;
	CUGCell cell;

	for(y=startRow;y<=endRow;y++){
		for(x = startCol;x<=endCol;x++){
			GetCell(x,y,&cell);
			cell.SetJoinInfo(FALSE,startCol-x,startRow-y);
			SetCell(x,y,&cell);
		}
	}

	GetCell(startCol,startRow,&cell);
	cell.SetJoinInfo(TRUE,endCol - startCol,endRow - startRow);
	SetCell(startCol,startRow,&cell);

	return UG_SUCCESS;
}

/***************************************************
****************************************************/
int	CUGCtrl::UnJoinCells(int col,long row){
	
	int endCol;
	long endRow;
	CUGCell cell;

	if(GetJoinRange(&col,&row,&endCol,&endRow) == UG_SUCCESS){
		for(long y = row; y <= endRow; y++){
			for(int x = col; x <= endCol; x++){
				 GetCell(x,y,&cell);
				 cell.ClearProperty(UGCELL_JOIN_SET);
				 SetCell(x,y,&cell);
			}
		}
	}
	return UG_SUCCESS;
}

/***************************************************
****************************************************/
int	CUGCtrl::DuplicateCell(int destCol,long destRow, int srcCol, long srcRow){

	CUGCell cell;

	if(GetCell(srcCol,srcRow,&cell) != UG_SUCCESS)
		return UG_ERROR;
	if(SetCell(destCol,destRow,&cell) != UG_SUCCESS)
		return UG_ERROR;

	return UG_SUCCESS;
}

/***************************************************
****************************************************/
int	CUGCtrl::QuickGetText(int col,long row, std::wstring*string){

	XeASSERT(string);

	m_GI->m_cell.ClearAll();
	GetCellIndirect(col,row,&m_GI->m_cell);
	*string = m_GI->m_cell.GetText();
	
	return UG_SUCCESS;
}
/***************************************************
****************************************************/
LPCTSTR	CUGCtrl::QuickGetText(int col,long row)
{
	m_GI->m_cell.ClearAll();
	GetCellIndirect(col,row,&m_GI->m_cell);
	return m_GI->m_cell.GetText();
}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetText(int col,long row,LPCTSTR string)
//{
//	ASSERT(string);
//
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetText(string);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetNumber(int col,long row,double number)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetNumber(number);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetMask(int col,long row,LPCTSTR string)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetMask(string);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetLabelText(int col,long row,LPCTSTR string)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetLabelText(string);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}

/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetTextColor(int col,long row,COLORREF color)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetTextColor(color);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetHTextColor(int col,long row,COLORREF color)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetHTextColor(color);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}

/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetBackColor(int col,long row,COLORREF color)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetBackColor(color);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetHBackColor(int col,long row,COLORREF color)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetHBackColor(color);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetBitmap(int col,long row,CBitmap * bitmap)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetBitmap(bitmap);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetBitmap(int col,long row,int index){
//	return QuickSetBitmap(col,row,GetBitmap(index));
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetFont(int col,long row,CFont * font)
//{
//	ASSERT(font);
//
//	m_cell.ClearAll();
//	GetCell(col,row,&m_cell);
//	m_cell.SetFont(font);
//	SetCell(col,row,&m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetFont(int col,long row,int index)
//{
//	return QuickSetFont(col,row,GetFont(index));
//}

/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetAlignment(int col,long row,short align)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetAlignment(align);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}

/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetBorder(int col,long row,short border)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetBorder(border);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetBorderColor(int col,long row,CPen *pen)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetBorderColor(pen);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetCellType(int col,long row,long type)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetCellType(type);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}

/***************************************************
****************************************************/
//int	CUGCtrl::QuickSetCellTypeEx(int col,long row,long typeEx)
//{
//	m_GI->m_cell.ClearAll();
//	GetCell(col,row,&m_GI->m_cell);
//	m_GI->m_cell.SetCellTypeEx(typeEx);
//	SetCell(col,row,&m_GI->m_cell);
//	return UG_SUCCESS;
//}
/***************************************************
QuickSetRange
	Sets a range of cells to use the same attributes as 
	the given cell
Return
	UG_SUCCESS - success
	otherwise the number of SetCell errors are returned
	this may be caused if part or all of the range is 
	outside of the grids total column/row range
****************************************************/
//int	CUGCtrl::QuickSetRange(int startCol,long startRow,int endCol,long endRow,
//						   CUGCell *cell){
//
//	int error = UG_SUCCESS;
//
//	for(long row = startRow; row <= endRow;row++){
//		for(int col = startCol;col <= endCol;col++){
//			if(SetCell(col,row,cell) != UG_SUCCESS)
//				error++;
//		}
//	}	
//	return error;
//}


/***************************************************
SetNumberRow
	Sets the number of rows that are displayed in the
	grid. Up to 64,000 rows are allowed with non-uniform
	row height. Up to 2 billion rows are allowed with
	uniform row height. If non-uniform row height is set 
	and the number of row exced 65,000 then the grid
	automatically goes to uniform row height

  return
		UG_SUCCESS  success
		UG_ERROR	fail
****************************************************/
int	CUGCtrl::SetNumberRows(long rows,BOOL redraw){
	
	//range checking
	if(rows <0)
		return UG_ERROR;

	if (m_enableUpdate == FALSE )
		redraw = FALSE;
	
	if(rows <= m_GI->m_numLockRows && rows > 0)
		LockRows(rows-1);

	//store the old number of rows
	long oldNumRows = m_GI->m_numberRows;

	//check to see if uniform row height should be used
	//if(rows > 64000 && m_GI->m_uniformRowHeightFlag == FALSE)
	//	SetUniformRowHeight(TRUE);

	//store the number of rows and create the new row height array
	m_GI->m_numberRows = rows;
	//if(m_GI->m_uniformRowHeightFlag == FALSE){
	//	//create the array
	//	int *tempRowHeights = new int[rows];
	//	
	//	//copy the old row heights over
	//	for(long loop = 0;loop < rows ; loop++){
	//		if(loop < oldNumRows)
	//			tempRowHeights[loop] = m_GI->m_rowHeights[loop];
	//		else
	//			tempRowHeights[loop] = m_GI->m_defRowHeight;
	//	}
	//	//delete the old row height array
	//	if(m_GI->m_rowHeights != NULL)
	//		delete[] m_GI->m_rowHeights;
	//	//use the new array
	//	m_GI->m_rowHeights = tempRowHeights;
	//}

	//make sure that the current grid positions are within range
	int oldTop = m_GI->m_topRow;
	CalcTopRow();
	if(oldTop != m_GI->m_topRow)
		Moved();

	if(m_GI->m_currentRow >= rows){
		m_GI->m_currentRow =  rows -1;
		m_GI->m_lastRow = m_GI->m_currentRow;
		m_GI->m_dragRow = m_GI->m_currentRow;
	}
	if(m_GI->m_dragRow >= rows){
		m_GI->m_dragRow = rows - 1;
	}


	SetLockRowHeight();

	if(m_GI->m_currentRow < 0 && rows > 0)
		GotoRow(0);	

	m_CUGVScroll->Moved();

	if(redraw){
		RedrawAll();
	}

	return UG_SUCCESS;
}


/***************************************************
GetNumberRow
	Returns the number of rows
****************************************************/
long CUGCtrl::GetNumberRows(){

	return m_GI->m_numberRows;
}
/***************************************************
SetNumberCols
	Sets the number of columns (0 - 32,000)

  return
		UG_SUCCESS  success
		UG_ERROR	fail
****************************************************/
int CUGCtrl::SetNumberCols(int cols,BOOL redraw){
	
	//range checking
	if(cols <0 || cols > 32000)
		return UG_ERROR;

	if (m_enableUpdate == FALSE )
		redraw = FALSE;

	if(cols <= m_GI->m_numLockCols && cols > 0)
		LockColumns(cols-1);

	//store the old number of cols
	int oldNumCols = m_GI->m_numberCols;

	//store the number of cols and create the new col width array
	m_GI->m_numberCols = cols;

	//create the array
	UGCOLINFO * tempColWidths	= new UGCOLINFO[cols];
		
	//copy the col information over
	int loop;
	for(loop = 0;loop < cols ; loop++){
		if(loop < oldNumCols){
			tempColWidths[loop].width = m_GI->m_colInfo[loop].width;			
			tempColWidths[loop].dataSource = m_GI->m_colInfo[loop].dataSource;
			tempColWidths[loop].colDefault = m_GI->m_colInfo[loop].colDefault;
			tempColWidths[loop].colTranslation = m_GI->m_colInfo[loop].colTranslation;
		}
		else{
			tempColWidths[loop].width = m_GI->m_defColWidth;
			tempColWidths[loop].dataSource = m_GI->m_defDataSource;
			tempColWidths[loop].colDefault = new CUGCell;
			tempColWidths[loop].colDefault->CopyInfoFrom(m_GI->m_gridDefaults);
			tempColWidths[loop].colTranslation = loop;
		}
	}

	//delete the old information
	for(loop = cols;loop <oldNumCols;loop++){
		delete m_GI->m_colInfo[loop].colDefault;
	}
	if(m_GI->m_colInfo != NULL)
	//delete the old col width array
		delete[] m_GI->m_colInfo;
	m_GI->m_colInfo = tempColWidths;

	//make sure that the current grid positions are within range
	CalcLeftCol();
	if(m_GI->m_currentCol >= cols){
		m_GI->m_currentCol =  cols -1;
		m_GI->m_dragCol = m_GI->m_currentCol;
	}

	SetLockColWidth();

	if(m_GI->m_currentCol < 0 && cols > 0)
		GotoCol(0);

	if(redraw)
		RedrawAll();

	return UG_SUCCESS;
}
/***************************************************
GetNumberCols
	Returns the number of columns that are currently
	set
****************************************************/
int CUGCtrl::GetNumberCols(){
	return m_GI->m_numberCols;
}
/***************************************************
SetColWidth
	Sets the width of the given column in pixels
		
	return
		UG_SUCCESS	success
		UG_ERROR	specifed col is out of range
					or a negative width
***************************************************/
int CUGCtrl::SetColWidth(int col,int width, bool notify /*= true*/)
{
	if ( col <0 || col >= m_GI->m_numberCols )
	{
		if( col < 0 && col >= -m_GI->m_numberSideHdgCols )
		{
			return SetSH_ColWidth( col, width );
		}
		return UG_ERROR;
	}
	
	if ( ::IsWindowVisible(Hwnd()) == TRUE && m_GI->m_paintMode == TRUE )
	{
		// Send col sized notifications to cell types in affected column
		// In order to provide acceptable performance this notification will
		// only be sent to the visible cells.
		for( int nIndex = GetTopRow(); nIndex < GetBottomRow(); nIndex++ )
		{
			CUGCellType* pCellType = GetCellType( col, nIndex );
			if ( pCellType != NULL )
			{
				pCellType->OnChangedCellWidth( col, nIndex, &width );
			}
		}

		// Send the col sized notification to CUGCtrl derived classes
		if (notify)
		{
			OnColSized(col, &width);
		}
	}

	// update column width
	m_GI->m_colInfo[col].width = width;

	// check to see if a column within the lock is 
	// being changed, then set the lock width
	if( col < m_GI->m_numLockCols )
	{
		SetLockColWidth();
	}

	return UG_SUCCESS;
}

/***************************************************
GetColWidth
	Retrieves the width of the specied column.

	return
		UG_SUCCESS	success
		UG_ERROR	specifed col is out of range
***************************************************/
int CUGCtrl::GetColWidth(int col,int *width){
	
	if( col >= m_GI->m_numberCols )
		return UG_ERROR;

	if ( col < 0 )
	{	// side heading column
		//translate the col number into a 0 based positive index
		col = (col * -1) -1;

		if ( col >= m_GI->m_numberSideHdgCols )
			return UG_ERROR;

		*width =  m_GI->m_sideHdgWidths[col];
	}
	else
	{	// grid column
		*width =  m_GI->m_colInfo[col].width;
	}

	return UG_SUCCESS;
}
/***************************************************
GetColWidth
***************************************************/
int CUGCtrl::GetColWidth(int col){

	if(col <0 || col >= m_GI->m_numberCols)
		return 0;

	return m_GI->m_colInfo[col].width;
}

/***************************************************
SetDefColWidth
	Sets the default column width. This value is used
	when new columns are created.

  return
	UG_SUCCESS	success
	UG_ERROR	fail
***************************************************/
int CUGCtrl::SetDefColWidth(int width){

	if(width <0)
		return UG_ERROR;

	m_GI->m_defColWidth = width;

	return UG_SUCCESS;
}
/***************************************************
BestFit
	Purpose
		Adjusts the widths of the secified columns
		so that they are the 'best fit' for the information
		being displayed
	Params
		startCol -	the start column in a range of cols
					to be 'best fitted'
		endCol -	the end column in the range of cols
		calcRange - the number of rows to base the calculation
					on. use 0 for all rows
		flag -		options for the best fit
					bit1: include top headings (UG_BESTFIT_TOPHEADINGS)
					bit2: use average width (UG_BESTFIT_AVERAGE)
	Return
****************************************************/
int CUGCtrl::BestFit(int startCol,int endCol,int CalcRange,int flag){

	if(startCol < (m_GI->m_numberSideHdgCols *-1))
		return UG_ERROR;
	if(endCol >= m_GI->m_numberCols)
		return 2;
	if(startCol > endCol)
		return 3;

	CUGCell cell;
	CUGCellType * cellType;
	CSize size;
	int count		= 0;
	int x;
	long y;
	int numCols		= endCol-startCol+1;
	int *bestWidth	= new int[numCols];
	//CDC * dc		= GetDC();
	//HDC hDC = ::GetDC(m_hWnd);
	//CDC* dc = CDC::FromHandle(hDC);

	int startRow = 0;
	if(flag&UG_BESTFIT_TOPHEADINGS)
		startRow -= m_GI->m_numberTopHdgRows;
	
	//set the best width to 3 pixels (3 for min width)
	for(x= 0;x < numCols;x++)
		bestWidth[x] = 3;

	for(y = startRow;y <m_GI->m_numberRows;y++)
	{
		if(y == m_GI->m_numLockRows)
			y = m_GI->m_topRow;

		for(x= startCol;x <= endCol;x++)
		{	
			GetCellIndirect(x,y,&cell);
			
			if(cell.IsPropertySet(UGCELL_JOIN_SET) == FALSE)
			{	// reset the size object to reflect cell's width and height
				size.cx = GetColWidth ( x );
				size.cy = GetRowHeight( y );
				// call GetBestSize for cell type set to this cell
				cellType = GetCellType(cell.GetCellType());
				cellType->GetBestSize(&size,&cell);
				if(size.cx > bestWidth[x-startCol])
					bestWidth[x-startCol] = size.cx;
			}
		}

		count++;
		if(count == CalcRange)
			break;
	}

	//update the col widths - restrict to width of grid window
	int bw;
	RECT rect;
	m_CUGGrid->GetClientRect(&rect);		// window may not be created yet

	int maxw, sideHeadingWidth = 0;

	if(rect.right == 0)
		maxw = 300;
	else
		maxw = rect.right - m_GI->m_vScrollWidth;

	for(x= 0;x < numCols;x++)
	{
		bw = bestWidth[x];
		if(bw > maxw)
			bw = maxw;
		else
			bw+=2;	// just a tweak to give edit ctrl room if
					// AutoSize is turned off 
		if ( startCol + x < 0 )
		{
			sideHeadingWidth += bw;
		}

		// send notifications to cell types.  In order to provide acceptable performance
		// this notification will only be sent to the visible rows.
		for(int nIndex = GetTopRow(); nIndex < GetBottomRow(); nIndex++)
		{
			CUGCellType* pCellType = GetCellType( startCol + x, nIndex );

			if ( pCellType != NULL )
				pCellType->OnChangedCellWidth( startCol + x, nIndex, &bw );
		}

		OnColSized( startCol + x, &bw );
		SetColWidth( startCol + x, bw );
	}
	if ( sideHeadingWidth > 0 )
	{	// adjust width of the side heading
		SetSH_Width( sideHeadingWidth );
	}

	delete[] bestWidth;
	//ReleaseDC(dc);		
	//::ReleaseDC(m_hWnd, hDC);		
	AdjustComponentSizes();

	OnColRowSizeFinished();

	return UG_SUCCESS;
}
/***************************************************
FitToWindow
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::FitToWindow(int startCol,int endCol){

	RECT rect;
	int screenwidth;
	int columnwidths,width;
	int loop;
	float ratio;
	int newwidth;
	// The FitToWindow function can only function when
	// the paint mode is enabled.  Backup current
	// paint mode setting, and turn it on.
	BOOL bOldPaintMode = m_GI->m_paintMode;
	m_GI->m_paintMode = TRUE;
	// when the paint was originally disabled, than force
	// the grid to recalculate the component sizes.
	if ( bOldPaintMode == FALSE )
		AdjustComponentSizes();

	//validation checking
	if(startCol < 0 || endCol < startCol || endCol >= m_GI->m_numberCols)
		return UG_ERROR;
	
	//get the grid screen width
	m_CUGGrid->GetClientRect(&rect);
	screenwidth = rect.right;

	//get the total column widths
	columnwidths = 0;
	for(loop = startCol;loop <=endCol;loop++){
		GetColWidth(loop,&width);
		columnwidths += width;
	}
	
	//get the ratio
	ratio = ((float)screenwidth) / ((float)columnwidths);

	//reset the column widths
	for(loop = startCol;loop <endCol;loop++){
		GetColWidth(loop,&width);
		newwidth = (int)((float)width*ratio);
		SetColWidth(loop,newwidth);
		screenwidth -= newwidth;
	}
	SetColWidth(endCol,screenwidth);

	// Before updating the screen restore original paint mode setting
	m_GI->m_paintMode = bOldPaintMode;
	
	Update();
	return UG_SUCCESS;
}
/***************************************************
SetDefRowHeight
	Sets the default row height. This value is used
	for all rows when uniform row height is set.
	For non-uniform row height, this value is used
	when a new row is created

  return
	UG_SUCCESS	success
	UG_ERROR	fail

***************************************************/
int CUGCtrl::SetDefRowHeight(int height){

	if(height <1 || height > 1024)
		return UG_ERROR;

	m_GI->m_defRowHeight = height;

	return UG_SUCCESS;
}
/***************************************************
SetUniformRowHeight
	Sets the grid in uniform row height or no-uniform
	row height.
	'flag'  TRUE:  then uniform row height is set
	           all rows then become the same height
			   which is set by 'SetDefRowHeight'
			   The height can still be adjusted by the
			   used, but all rows are adjusted at the
			   same time
	'flag' FALSE: then non-uniform row height is set
				if the grid have less than 64001 rows
				A maximum of 64000 rows are allowed 
				with this mode.
				This allows each row to have its own
				height 'SetRowHeight'

  return
	UG_SUCCESS	success
	UG_ERROR	fail
***************************************************/
int CUGCtrl::SetUniformRowHeight(int flag){

	//uniform row height on
	if(flag){
		//if(m_GI->m_rowHeights != NULL){
		//	delete[] m_GI->m_rowHeights;
		//	m_GI->m_rowHeights = NULL;
		//}
		m_GI->m_uniformRowHeightFlag = TRUE;
	}
	//uniform row height off
	else{
		//if(m_GI->m_rowHeights == NULL){
		//	if(m_GI->m_uniformRowHeightFlag && m_GI->m_numberRows >0){
		//		m_GI->m_rowHeights = new int[m_GI->m_numberRows];
		//		for(long row = 0;row <m_GI->m_numberRows;row++){
		//			m_GI->m_rowHeights[row] =m_GI->m_defRowHeight;
		//		}
		//	}
		//}
		m_GI->m_uniformRowHeightFlag = FALSE;
	}

	return UG_SUCCESS;
}
/***************************************************
SetRowHeight
	Sets the height of the specified row. If uniform
	row height is specied then the default row height
	is set

  return
	UG_SUCCESS	success
	UG_ERROR	fail  - out of range
***************************************************/
int CUGCtrl::SetRowHeight( long row,int height )
{
	if( row < 0 || row >= m_GI->m_numberRows )
	{
		if( row < 0 && row >= -m_GI->m_numberTopHdgRows )
		{
			return SetTH_RowHeight( row, height );
		}
		return UG_ERROR;
	}

	if ( ::IsWindowVisible(Hwnd()) == TRUE && m_GI->m_paintMode == TRUE )
	{
		// Send row sized notifications to cell types in affected row
		// In order to provide acceptable performance this notification will
		// only be sent to the visible cells.
		for( int nIndex = GetLeftCol(); nIndex < GetRightCol(); nIndex++ )
		{
			CUGCellType* pCellType = GetCellType( nIndex, row );
			if ( pCellType != NULL )
			{
				pCellType->OnChangedCellHeight( nIndex, row, &height );
			}
		}

		// Send the row sized notification to CUGCtrl derived classes
		OnRowSized( row, &height );
	}

	if(m_GI->m_uniformRowHeightFlag)
		m_GI->m_defRowHeight = height;
	//else
	//	m_GI->m_rowHeights[row] = height;

	// check to see if a row within the lock is 
	// being changed, then set the lock height
	if( row < m_GI->m_numLockRows )
	{
		SetLockRowHeight();
	}

	OnColRowSizeFinished();

	return UG_SUCCESS;
}

/***************************************************
GetRowHeight
	Retrives the height of the specified row.
	If uniform row height is specified then the
	default row height is returned

  return
	UG_SUCCESS	success
	UG_ERROR	fail - out of range
***************************************************/
int CUGCtrl::GetRowHeight(long row,int *height)
{
	if( row >= m_GI->m_numberRows )
		return UG_ERROR;

	if ( row < 0 )
	{	// top heading row
		// translate the row number into a 0 based positive index
		row = (row * -1) -1;

		if ( row >= m_GI->m_numberTopHdgRows )
			return UG_ERROR;

		*height =  m_GI->m_topHdgHeights[row];
	}
	else
	{	// grid row
		*height = GetRowHeight( row );
	}

	return UG_SUCCESS;
}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
int CUGCtrl::GetRowHeight(long row){
	
	if(row < 0 || row >= m_GI->m_numberRows)
		return 0;

	if(m_GI->m_uniformRowHeightFlag)
		return m_GI->m_defRowHeight;

	return GetNonUniformRowHeight(row);
	//return m_GI->m_rowHeights[row];
}

int CUGCtrl::GetNonUniformRowHeight(long row)
{
	XeASSERT(FALSE);	// not supported
	return m_GI->m_defRowHeight;
	//int lineCount = GetRowLineCount(row);
	//int rowHeight = (lineCount == 1) ? s_logGridCharSize.cySingleLineRowHeight  : (lineCount * s_logGridCharSize.cyMultiLineTextHeight) + 4;
	//if (lineCount > 1)
	//{
	//	int cyMinCellHeight = s_logGridCharSize.cySingleLineRowHeight;
	//	int cyMaxCellHeight = m_GI->m_gridHeight < cyMinCellHeight ? cyMinCellHeight : m_GI->m_gridHeight - cyMinCellHeight;
	//	if (rowHeight > cyMaxCellHeight)
	//	{
	//		rowHeight = cyMaxCellHeight;
	//	}
	//}
	//return rowHeight;
}

/***************************************************
	Purpose
	Params
	Return
***************************************************/
int	CUGCtrl::InsertCol(int col){

	//find the datasource
	CUGDataSource * ds = NULL;
	if(col >=0)
		ds = m_GI->m_colInfo[col].dataSource;
	if(ds == NULL)
		ds = m_GI->m_defDataSource;

	int rt = ds->InsertCol(m_GI->m_colInfo[col].colTranslation);
	
	if(rt == UG_SUCCESS){
		
		SetNumberCols(GetNumberCols()+1);

		//adjust the column information structure
		int numCols = GetNumberCols() -1;
		UGCOLINFO temp;
		temp.width			= m_GI->m_colInfo[numCols].width;			
		temp.dataSource		= m_GI->m_colInfo[numCols].dataSource;
		temp.colDefault		= m_GI->m_colInfo[numCols].colDefault;
		temp.colTranslation	= m_GI->m_colInfo[numCols].colTranslation;
		
		for(int loop = numCols ; loop > col; loop--){
			m_GI->m_colInfo[loop].width			= m_GI->m_colInfo[loop-1].width;			
			m_GI->m_colInfo[loop].dataSource	= m_GI->m_colInfo[loop-1].dataSource;
			m_GI->m_colInfo[loop].colDefault	= m_GI->m_colInfo[loop-1].colDefault;		
			m_GI->m_colInfo[loop].colTranslation= m_GI->m_colInfo[loop-1].colTranslation;		
		}
		m_GI->m_colInfo[col].width			= temp.width;			
		m_GI->m_colInfo[col].dataSource		= temp.dataSource;
		m_GI->m_colInfo[col].colDefault		= temp.colDefault;
		m_GI->m_colInfo[col].colTranslation	= col;
	}
	
	return rt;
}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
int	CUGCtrl::AppendCol(){

	//find the datasource
	CUGDataSource * ds = NULL;
	ds = m_GI->m_defDataSource;

	int rt = ds->AppendCol();

	if(rt == UG_SUCCESS)
		SetNumberCols(GetNumberCols()+1);

	return rt;
}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
int	CUGCtrl::DeleteCol(int col){
	
	if(GetNumberCols() <= 0)
		return UG_ERROR;

	//find the datasource
	CUGDataSource * ds = NULL;
	if(col >=0)
		ds = m_GI->m_colInfo[col].dataSource;
	if(ds == NULL)
		ds = m_GI->m_defDataSource;

	int rt = ds->DeleteCol(m_GI->m_colInfo[col].colTranslation);

	if(rt == UG_SUCCESS){

		//adjust the column information structures		
		int numCols = GetNumberCols()-1;
		if(numCols > 0){
			UGCOLINFO temp;
			temp.colDefault		= m_GI->m_colInfo[col].colDefault;
			for(int loop = col ; loop < numCols;loop++){
				m_GI->m_colInfo[loop].width			= m_GI->m_colInfo[loop+1].width;			
				m_GI->m_colInfo[loop].dataSource	= m_GI->m_colInfo[loop+1].dataSource;
				m_GI->m_colInfo[loop].colDefault	= m_GI->m_colInfo[loop+1].colDefault;
				m_GI->m_colInfo[loop].colTranslation= m_GI->m_colInfo[loop+1].colTranslation;
			}
			m_GI->m_colInfo[numCols].colDefault = temp.colDefault;
		}

		//reset the number of cols
		SetNumberCols(GetNumberCols()-1);
	}

	return rt;
}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
int	CUGCtrl::InsertRow(long row){

	if(row > m_GI->m_numberRows)
		return UG_ERROR;

	//find the datasource
	CUGDataSource * ds = NULL;
	ds = m_GI->m_defDataSource;

	int rt = ds->InsertRow(row);
	if(rt == UG_SUCCESS){
		SetNumberRows(m_GI->m_numberRows + 1);
	}
	return rt;
}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
int	CUGCtrl::AppendRow(){
	
	int rt = m_GI->m_defDataSource->AppendRow();
	if(rt == UG_SUCCESS){
		SetNumberRows(m_GI->m_numberRows + 1);
	}
	return rt;
}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
int	CUGCtrl::DeleteRow(long row){

	if(row < 0 || row >= m_GI->m_numberRows)
		return UG_ERROR;

	int rt = m_GI->m_defDataSource->DeleteRow(row);
	if(rt == UG_SUCCESS){
		SetNumberRows(m_GI->m_numberRows - 1);
	}
	return rt;
}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
//int CUGCtrl::FindDialog(){
//	
//	if(m_GI->m_findDialogRunning != FALSE)
//		return UG_ERROR;
//
//	m_GI->m_findDialogRunning = TRUE;
//	m_GI->m_findDialogStarted = TRUE;
//	m_findReplaceDialog = new CFindReplaceDialog;
//	m_findReplaceDialog->Create(TRUE,_T(""),NULL,FR_DOWN,CWnd::FromHandle(m_hWnd));
//	ASSERT(m_findReplaceDialog->m_hWnd);
//
//	CString * strPair = new CString[2];
//	strPair[0] = "&whole word";
//	strPair[1] = "Match &entire cell text";
//
//	EnumChildWindows(m_findReplaceDialog->m_hWnd, ModifyDlgItemText, (LPARAM)strPair);
//
//	delete [] strPair;
//
//	return UG_SUCCESS;
//}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
//int CUGCtrl::ReplaceDialog(){
//	
//	if(m_GI->m_findDialogRunning != FALSE)
//		return UG_ERROR;
//
//	m_GI->m_findDialogRunning = TRUE;
//	m_GI->m_findDialogStarted = TRUE;
//	m_findReplaceDialog = new CFindReplaceDialog;
//	m_findReplaceDialog->Create(FALSE,_T(""),NULL,FR_DOWN, CWnd::FromHandle(m_hWnd));
//
//	CString * strPair = new CString[2];
//	strPair[0] = "&whole word";
//	strPair[1] = "Match &entire cell text";
//
//	EnumChildWindows(m_findReplaceDialog->m_hWnd, ModifyDlgItemText, (LPARAM)strPair);
//
//	delete [] strPair;
//
//	return UG_SUCCESS;
//}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
//int CUGCtrl::FindInAllCols(BOOL state){
//	if(state)
//		m_GI->m_findInAllCols = TRUE;
//	else
//		m_GI->m_findInAllCols = FALSE;
//
//	return UG_SUCCESS;
//}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
//LRESULT CUGCtrl::ProcessFindDialog(WPARAM, LPARAM)
//{
//	BOOL bFoundSomething = FALSE;
//	
//	if(m_findReplaceDialog == NULL)
//		return UG_ERROR;
//	if(m_findReplaceDialog->GetSafeHwnd() == NULL)
//		return UG_ERROR;
//	if(m_findReplaceDialog->IsTerminating())
//	{
//		m_GI->m_findDialogStarted = FALSE;
//		m_GI->m_findDialogRunning = FALSE;
//		// redraw area covered by the find/replace dialog
//		Invalidate();
//		return UG_ERROR;
//	}
//
//	int col = m_GI->m_currentCol;
//	long row = m_GI->m_currentRow;
//
//	CString string = m_findReplaceDialog->GetFindString();
//	CString string2 = m_findReplaceDialog->GetReplaceString();
//	int findFlags = 0;
//	if(!m_findReplaceDialog->MatchCase())
//		findFlags |= UG_FIND_CASEINSENSITIVE;
//	if(!m_findReplaceDialog->MatchWholeWord())
//		findFlags |= UG_FIND_PARTIAL;
//	if(!m_findReplaceDialog->SearchDown())
//		findFlags |= UG_FIND_UP;
//	if(m_GI->m_findInAllCols)
//		findFlags |= UG_FIND_ALLCOLUMNS;
//
//	m_GI->m_findDialogStarted = FALSE;
//
//	if( m_findReplaceDialog->ReplaceCurrent())
//	{
//		QuickSetText(m_GI->m_currentCol,row,string2);
//		RedrawCell(m_GI->m_currentCol,row);
//	}
//
//	if(string2 == string)
//		return UG_SUCCESS;
//	if(findFlags&UG_FIND_PARTIAL) 
//	{
//		if(string2.Find(string) != -1) 
//			return UG_SUCCESS;
//	}
//
//	while(FindNext(&string,&col,&row,findFlags) == UG_SUCCESS ) 
//	{
//		bFoundSomething = TRUE;
//		// find display location of the found column
//		for ( int tempCol = 0; tempCol < m_GI->m_numberCols; tempCol ++ )
//		{
//			if ( GetColTranslation( tempCol ) == col )
//			{
//				col = tempCol;
//				break;
//			}
//		}
//
//		GotoCell( col, row );
//
//		if( m_findReplaceDialog->ReplaceAll())
//		{
//			QuickSetText(col,row,string2);
//			RedrawCell(col,row);
//		}
//		else
//		{
//			break;
//		}
//	}
//
//	if( !bFoundSomething )
//	{
//		::MessageBox(m_findReplaceDialog->m_hWnd, _T("Item not found"),_T("Not Found"),MB_OK);
//		m_findReplaceDialog->SetFocus();
//		return UG_SUCCESS;
//	}
//	
//	return UG_SUCCESS;
//}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
//int CUGCtrl::FindFirst(CString *string,int *col,long *row,long flags){
//
//	*col = m_GI->m_colInfo[*col].colTranslation;
//	return m_GI->m_defDataSource->FindFirst(string,col,row,flags);
//}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
//int CUGCtrl::FindNext(CString *string,int *col,long *row,int flags){
//	*col = m_GI->m_colInfo[*col].colTranslation;
//
//	return m_GI->m_defDataSource->FindNext(string,col,row,flags);
//}

/***************************************************
	Purpose
	Params
	Return
***************************************************/
int CUGCtrl::SortBy(int col,int flag){

	int cols[1];
	cols[0] = col;
	return SortBy(cols,1,flag);

}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
int CUGCtrl::SortBy(int *cols,int num,int flag){
	
	// perform data column translation
	for(int loop = 0;loop < num;loop++)
	{
		// getting column translation on side heading columns will result in an error
		// it is not possible to swap side heading cols.
		if ( cols[loop] >= 0 )
		{
			cols[loop] = m_GI->m_colInfo[cols[loop]].colTranslation;
		}
	}

	return m_GI->m_defDataSource->SortBy(cols,num,flag);
}


/***************************************************
SetTH_Height
	Sets the total height of the top heading
	A value of 0 hides the top heading

  return
	UG_SUCCESS	success
	UG_ERROR	fail - out of range
***************************************************/
int	CUGCtrl::SetTH_Height(int height)
{
	if (m_GI->SetTH_HeightValue(height) == UG_ERROR)
		return UG_ERROR;
	
	AdjustComponentSizes();

	return UG_SUCCESS;
}
/***************************************************
GetTH_Height
	Returns the height of the top heading
***************************************************/
int	CUGCtrl::GetTH_Height(){
	return m_GI->m_topHdgHeight;
}
/***************************************************
SetSH_Width
	Sets the total widht of the side heading
	A value of 0 hides the side heading

  return
	UG_SUCCESS	success
	UG_ERROR	fail - out of range
***************************************************/
int	CUGCtrl::SetSH_Width(int width){

	if(width <0 || width >1024)
		return UG_ERROR;
	
	m_GI->m_sideHdgWidth = width;

	//adjust the width
	int totalWidth = 0;
	int loop;
	double adjust;
	//find the total old height
	for(loop = 0;loop < m_GI->m_numberSideHdgCols;loop++){
		totalWidth += m_GI->m_sideHdgWidths[loop];
	}
	//find the adjustment value
	adjust = (double)width / (double)totalWidth;
	//adjust each col width
	for(loop = 0;loop < m_GI->m_numberSideHdgCols;loop++){
		m_GI->m_sideHdgWidths[loop] = (int)(m_GI->m_sideHdgWidths[loop] * adjust +0.5);
	}

	AdjustComponentSizes();

	return UG_SUCCESS;
}
/***************************************************
GetSH_Width
	Returns the width of the side heading
***************************************************/
int	CUGCtrl::GetSH_Width(){
	return m_GI->m_sideHdgWidth;
}
/***************************************************
SetVS_Width
	Sets the width of the vertical scroll bar

  return
	UG_SUCCESS	success
	UG_ERROR	fail - out of range
***************************************************/
int	CUGCtrl::SetVS_Width(int width){
	
	if(width <0 || width > 1024)
		return UG_ERROR;

	m_GI->m_vScrollWidth = width;

	AdjustComponentSizes();

	return UG_SUCCESS;
}
/***************************************************
GetVS_Width
	returns the width of the vertical scroll bar
***************************************************/
int	CUGCtrl::GetVS_Width(){
	return m_GI->m_vScrollWidth;
}
/***************************************************
SetHVS_Height
	Sets the height of the horizontal scroll bar

  return
	UG_SUCCESS	success
	UG_ERROR	fail - out of range
***************************************************/
int	CUGCtrl::SetHS_Height(int height){

	if(height <0 || height > 1024)
		return UG_ERROR;

	m_GI->m_hScrollHeight = height;

	AdjustComponentSizes();

	return UG_SUCCESS;
}
/***************************************************
GetHS_Height
	returns the height of the horizontal scroll bar
***************************************************/
int	CUGCtrl::GetHS_Height(){
	return m_GI->m_hScrollHeight;
}
/***************************************************
SetCurrentCellMode
	Sets the highlighting and focus mode of the
	current cell
	value of 1 by default

	'mode' if bit 0 ( value of 1) is set then
			the current cell gets a focus rectangle
		   if bit 1 ( value of 2) is set then
		    the current cell is highlighted
		   (a value of three is for both)
***************************************************/
int	CUGCtrl::SetCurrentCellMode(int mode){
	
	if(mode < 0 || mode > 3)
		return UG_ERROR;

	m_GI->m_currentCellMode = mode;

	return UG_SUCCESS;
}
/***************************************************
SetHightlightRow
	Set the grid into rowhighlighting mode.
	FALSE by default
	'mode'  TRUE: row highlighting mode - a whole row
			is highlighted at a time - useful for
			multi-column list boxes

			FALSE: standard cell hightlighting
***************************************************/
int	CUGCtrl::SetHighlightRow(int mode, BOOL bExtend){

	if(mode){
		m_GI->m_highlightRowFlag = TRUE;

		int mode = m_GI->m_multiSelect->GetSelectMode();
		if(mode&UG_MULTISELECT_CELL)
			mode -= UG_MULTISELECT_CELL;
		mode |= UG_MULTISELECT_ROW;

		m_GI->m_multiSelect->SelectMode(mode);

		m_GI->m_bExtend = bExtend;
	}
	else{
		m_GI->m_highlightRowFlag = FALSE;

		int mode = m_GI->m_multiSelect->GetSelectMode();
		if(mode&UG_MULTISELECT_ROW)
			mode -= UG_MULTISELECT_ROW;
		mode |= UG_MULTISELECT_CELL;

		m_GI->m_multiSelect->SelectMode(mode);
	}
	
	return UG_SUCCESS;
}
/***************************************************
SetMultiSelectMode
	Turns multiple selection on and off
	off by default
	'mode'  TRUE or FALSE

***************************************************/
int	CUGCtrl::SetMultiSelectMode(int mode){

	m_GI->m_multiSelectFlag = mode;

	//0: off  1:cell  2:row
	if(mode){
		if(m_GI->m_highlightRowFlag)
			return m_GI->m_multiSelect->SelectMode(2);
		else
			return m_GI->m_multiSelect->SelectMode(1);
	}
	return m_GI->m_multiSelect->SelectMode(mode);	
}
/***************************************************
Set3DHeight
	Sets the height of the 3D border styles
	This value is used every time the border draws
	a rasied or recessed border
	a value of 1 is default

  return
	UG_SUCCESS	success
	UG_ERROR	fail - out of range

***************************************************/
int	CUGCtrl::Set3DHeight(int height){

	if(height <1 || height > 16)
		return UG_ERROR;

	m_GI->m_threeDHeight = height;

	return UG_SUCCESS;
}
/***************************************************
SetPaintMode
	Turns painting on/off.
	Useful for making many changes to the grid's
	properties, with paint mode off the grid will
	not try and update itself
***************************************************/
//#pragma NOTE("SK MOD - added fRedraw parameter")
int	CUGCtrl::SetPaintMode(int mode, BOOL fRedraw /*= TRUE*/){

	if(mode){
		m_GI->m_paintMode = TRUE;
		if( fRedraw )
			AdjustComponentSizes();	// this redraws all except bottom right corner.
	}
	else
		m_GI->m_paintMode = FALSE;

	return UG_SUCCESS;
}

/***************************************************
GetPaintMode
	Returns the current paint mode
***************************************************/
int	CUGCtrl::GetPaintMode(){
	return m_GI->m_paintMode;
}

/***************************************************
	Purpose
	Params
	Return
***************************************************/
int	CUGCtrl::SetVScrollMode(int mode){

	m_GI->m_vScrollMode = mode;
	m_CUGVScroll->Update();

	return UG_SUCCESS;
}
/***************************************************
	Purpose
	Params
	Return
***************************************************/
int	CUGCtrl::SetHScrollMode(int mode){

	m_GI->m_hScrollMode = mode;
	m_CUGHScroll->Update();

	return UG_SUCCESS;
}
/***************************************************
GetCurrentCol
	Returns the current column within the grid
***************************************************/
int	CUGCtrl::GetCurrentCol(){
	
	return m_GI->m_currentCol;
}
/***************************************************
GetCurrentRow
	Returns the curent row within the grid
***************************************************/
long CUGCtrl::GetCurrentRow(){

	return m_GI->m_currentRow;
}
/***************************************************
GetLeftCol
	Returns the left visible column within the grid
***************************************************/
int	CUGCtrl::GetLeftCol(){
	
	return m_GI->m_leftCol;
}
/***************************************************
GetRightCol
	Returns the right visible column within the grid
***************************************************/
int	CUGCtrl::GetRightCol(){
	
	return m_GI->m_rightCol;

}
/***************************************************
GetTopRow
	Returns the top visible row within the grid
***************************************************/
long CUGCtrl::GetTopRow(){
	return m_GI->m_topRow;
}
/***************************************************
GetBottomRow
	Returns the bottom visible row within the grid
***************************************************/
long CUGCtrl::GetBottomRow(){

	return m_GI->m_bottomRow;
}
	
/***************************************************
****************************************************
	REDRAW FUNTIONS 
****************************************************
****************************************************/

/***************************************************
RedrawAll
	Redraws all child windows
***************************************************/
int CUGCtrl::RedrawAll(){
	Update();
	return UG_SUCCESS;
}
/***************************************************
RedrawCell
	Redraws the specified cell within the grid
***************************************************/
int CUGCtrl::RedrawCell(int col,long row){
	if(col < 0){
		m_CUGSideHdg->RedrawAll();
		//m_CUGSideHdg->Invalidate();
		//m_CUGSideHdg->UpdateWindow();
	}
	else if(row < 0){
		m_CUGTopHdg->RedrawDirectly();
	}
	else{
		m_CUGGrid->RedrawCell(col,row);
	}
	return UG_SUCCESS;
}
/***************************************************
RedrawRow
	Redraws the specfied row within the grid
***************************************************/
int CUGCtrl::RedrawRow(long row){
	m_CUGGrid->RedrawRow(row);
	return UG_SUCCESS;
}
/***************************************************
RedrawCol
	Redraws the specified column within the grid
***************************************************/
int CUGCtrl::RedrawCol(int col){
	m_CUGGrid->RedrawCol(col);
	return UG_SUCCESS;
}
/***************************************************
RedrawRange
	Redraws the specified range of cells within the
	grid
***************************************************/
int CUGCtrl::RedrawRange(int startCol,long startRow,int endCol,long endRow){
	m_CUGGrid->RedrawRange(startCol,startRow,endCol,endRow);
	return UG_SUCCESS;
}

/***************************************************
TempDisableFocusRect
	Temporarily disables the drawing of the focus
	rectangle, for the next time the grid is drawn.
	Then it will be reset.
***************************************************/
void CUGCtrl::TempDisableFocusRect(){
	m_CUGGrid->TempDisableFocusRect();
}

/***************************************************
****************************************************
	CELL TYPE
****************************************************
****************************************************/

/***************************************************
AddCellType
	Adds a custom cell type to the grid's cell type
	list. Once added a cell can use this type.
	
	 Returns the ID number of the cell type that 
	was added. This is the value to use when setting
	a cells 'celltype' property

	NOTE: The grid does not delete the cell type when
	the grid is destroyed.

	-1 error
***************************************************/
long CUGCtrl::AddCellType(CUGCellType *celltype){

	if(celltype == NULL)
		return -1;
	
	int index = m_cellTypeList->AddPointer(celltype);
	
	if(index  == -1)
		return -1;
	
	//celltype->SetGridCtrlPtr(this);
	celltype->m_GI = m_GI;
	celltype->SetID(index);

	return index;
}
/****************************************************
RemoveCellType
	Removes the specified cell type from the grid's
	cell type list. It does not delete the cell type
	class
****************************************************/
int CUGCtrl::RemoveCellType(int index){

	CUGCellType* pCellType = (CUGCellType*) m_cellTypeList->GetPointer(index);
	if (NULL != pCellType) delete pCellType;

	m_cellTypeList->DeletePointer(index);

	return UG_SUCCESS;	
}

/***************************************************
****************************************************
	Multiple Selection
****************************************************
****************************************************/

/***************************************************
ClearSelections
	Clears any selections made in the grid
  return
	UG_SUCCESS	success
	UG_ERROR	fail - out of range
****************************************************/
int CUGCtrl::ClearSelections(){

	m_GI->m_multiSelect->ClearAll();

	m_GI->m_dragCol = m_GI->m_currentCol;
	m_GI->m_dragRow = m_GI->m_currentRow;

	return UG_SUCCESS;	
}
/***************************************************
Select
	Selects a specified cell
	Multiple selection must be turned on for this
	function to work
  return
	UG_SUCCESS	success
	UG_ERROR	fail - out of range
****************************************************/
int CUGCtrl::Select(int col,long row){
	m_GI->m_multiSelect->StartBlock(col,row);
	m_GI->m_multiSelect->EndBlock(col,row);
	RedrawCell(col,row);
	return UG_SUCCESS;	
}
/***************************************************
****************************************************/
BOOL CUGCtrl::IsSelected(int col,long row,int *blockNum){
	return m_GI->m_multiSelect->IsSelected(col,row,blockNum);
}

/***************************************************
SelectRange
	Selects a specified range of cells
	Multiple selection must be turned on for this
	function to work
  return
	UG_SUCCESS	success
	UG_ERROR	fail - out of range
****************************************************/
int CUGCtrl::SelectRange(int startCol,long startRow,int endCol,long endRow)
{
	if( m_GI->m_multiSelectFlag & 3 )
	{
		m_GI->m_multiSelect->StartBlock( startCol, startRow );
		m_GI->m_multiSelect->EndBlock( endCol, endRow );
	}

	m_GI->m_dragCol = endCol;
	m_GI->m_dragRow = endRow;

	RedrawRange( startCol, startRow, endCol, endRow );
	return UG_SUCCESS;	
}
/***************************************************
EnumFirstSelected
	Returns the co-ords of the first cell that
	has been selected. the current cell is also included
	in this enumeration
	( if Multiple selection is off then the current cell
	is returned)	

  return
	UG_SUCCESS	success
	UG_ERROR	fail
****************************************************/
int CUGCtrl::EnumFirstSelected(int *col,long *row){

	return m_GI->m_multiSelect->EnumFirstSelected(col,row);
}
/***************************************************
EnumNextSelected
	Returns the co-ords of the next cell that
	has been selected. the current cell is also included
	in this enumeration
	(Multiple selection mode must be set first)
  return
	UG_SUCCESS	success
	UG_ERROR	no more items selected
****************************************************/
int CUGCtrl::EnumNextSelected(int *col,long *row){
	
	return m_GI->m_multiSelect->EnumNextSelected(col,row);
}

/***************************************************
EnumFirstBlock
	Returns the range of the first selected block of cells

  return
	UG_SUCCESS	success
	UG_ERROR	fail
****************************************************/
int CUGCtrl::EnumFirstBlock(int *startCol,long *startRow,int *endCol,long *endRow){

	return m_GI->m_multiSelect->EnumFirstBlock( startCol, startRow, endCol, endRow );
}
/***************************************************
EnumNextBlock
	Returns the range of the next selected block of cells

  return
	UG_SUCCESS	success
	UG_ERROR	no more items selected
****************************************************/
int CUGCtrl::EnumNextBlock(int *startCol,long *startRow,int *endCol,long *endRow){
	
	return m_GI->m_multiSelect->EnumNextBlock( startCol, startRow, endCol, endRow );
}

/***************************************************
CopySelected
	Copies the grid's currently selected items to the
	clipboard

  return
	UG_SUCCESS	success
	UG_ERROR	fail
****************************************************/
int CUGCtrl::CopySelected(){

	return CopySelected(FALSE);
}
/***************************************************
CopySelected
	Copies the grid's currently selected items to the
	clipboard
	'cutFlag' if true then the selected items are cut
		from the grid 
  return
	UG_SUCCESS	success
	UG_ERROR	fail
****************************************************/
int CUGCtrl::CopySelected(int cutFlag){
	
	int rt;
	std::wstring  clipString = L"";

	//get the selected items in a TAB delimited string
	CreateSelectedString(clipString,cutFlag);

	//copy the items to the clipboard
	rt = CopyToClipBoard(&clipString);

	if(rt == UG_SUCCESS){
		ClearSelections();
		RedrawAll();
	}

	return rt;
}
/***************************************************
****************************************************/
void CUGCtrl::CreateSelectedString(std::wstring& string,int cutFlag){
	
	int rt; 
	int col;
	long row,lastrow;
	CUGCell cell;
	
	string = L"";

	//enum selected items and add them to the string
	rt = m_GI->m_multiSelect->EnumFirstSelected(&col,&row);
	lastrow = row;
	while(rt == UG_SUCCESS){
		
		//get the selected cell then copy the string
		GetCellIndirect(col,row,&cell);
		string += cell.GetText();

		//check the cut flag
		if(cutFlag)
		{
			if ( cell.GetReadOnly() != TRUE )
			{
				cell.ClearAll();
				SetCell(col,row,&cell);
			}
		}
		
		//update the last row flag
		lastrow = row;

		//find the next selected item
		rt = m_GI->m_multiSelect->EnumNextSelected(&col,&row);

		//add line feeds between rows
		if(rt == UG_SUCCESS){
			if(row != lastrow)
				string += L"\n";
			//otherwise add tabs between cols
			else
				string += L"\t";
		}
	}
}


/***************************************************
CutSelected
	Copies the grid's currently selected items to the
	clipboard then removes the contents of the selected
	cells
  return
	UG_SUCCESS	success
	UG_ERROR	fail
****************************************************/
int CUGCtrl::CutSelected(){
	
	return CopySelected(TRUE);	
}

/***************************************************
CopyToClipBoard
	Copies the specified string to the clipboard
  return
	UG_SUCCESS	success
	UG_ERROR	fail
****************************************************/
int CUGCtrl::CopyToClipBoard(std::wstring* string){
	
	//std::wstring str(*string->GetString());
	CopyTextToClipboardW(*string);
	return UG_SUCCESS;
}

/***************************************************
CopyFromClipBoard
	Copies the contents from the clipboard to the
	specified string
  return
	UG_SUCCESS	success
	UG_ERROR	fail
****************************************************/
int CUGCtrl::CopyFromClipBoard(std::wstring* string){

	HGLOBAL hg;          //memory handle
	LPWSTR data;          //memory pointer
	//unsigned long size;  //size of clipboard data

	//open clipboard
	::OpenClipboard(Hwnd());

	//get a pointer to the text
	hg=::GetClipboardData(CF_UNICODETEXT);

	//if no text was found then return FALSE
	if(hg==NULL)
		return UG_ERROR;

	//lock the memory and get a pointer to it
	data=(LPWSTR)::GlobalLock(hg);
	//get the size of the text
	//size=(long)::GlobalSize(hg);

	*string = data;

	//unlock the global memory
	::GlobalUnlock(hg);

	//close the clipboard
	::CloseClipboard();

	return UG_SUCCESS;
}
/***************************************************
Paste
	Pastes the contents of the clipboard into the
	grid - starting from the current cell
  return
	UG_SUCCESS	success
	UG_ERROR	fail
****************************************************/
int CUGCtrl::Paste(){
	
	std::wstring string;

	//get the text from the clipboard
	if(CopyFromClipBoard(&string)!= UG_SUCCESS)
		return UG_ERROR;

	Paste(string);

	return UG_SUCCESS;
}
/***************************************************
Paste
	Pastes the contents of the clipboard into the
	grid - starting from the current cell
  return
	UG_SUCCESS	success
	UG_ERROR	fail
****************************************************/
int CUGCtrl::Paste(std::wstring&string){
	
	int		col		= m_GI->m_currentCol;
	long	row		= m_GI->m_currentRow;
	int		pos		=0;
	int     endpos;
	int		len;
	std::wstring	sub;
	LPTSTR	buf;
	CUGCell	cell;
	TCHAR	endchar = 0;
	int		maxcol = col;
	long	maxrow = row;

	buf = string.data();
	len = (int)string.size();

	while(pos < len){
		
		//check to see if the field is blank
		if(buf[pos]==L'\t'|| buf[pos]==L'\r' || buf[pos]==L'\n'){
			endchar = buf[pos];
			endpos = pos;
			if( col <= GetNumberCols() - 1 && row <= GetNumberRows() - 1 )
			{
				//set a blank cell
				GetCell(col,row,&cell);
				if ( cell.GetReadOnly() != TRUE )
				{
					cell.SetText(L"");
					SetCell(col,row,&cell);
				}
			}
		}
		//find the end of the item then copy the item to the cell
		else{
			endpos = pos+1;
			while(endpos < len){
				endchar = buf[endpos];
				if(endchar == L'\n' || endchar == L'\r' || endchar == L'\t')
					break;
				endpos++;
			}

			if( col <= GetNumberCols() - 1 && row <= GetNumberRows() - 1 )
			{
				//copy the item
				GetCell(col,row,&cell);

				if ( cell.GetReadOnly() != TRUE )
				{
					sub = string.substr(pos,endpos-pos);
					cell.SetText(sub.c_str());
					SetCell(col,row,&cell);
				}
			}
			
			//store the max col and row
			if(col > maxcol)
				maxcol = col;
			if(row > maxrow)
				maxrow = row;
		}

		//set up the position for the next field
		if(endchar == L'\t')
			col++;
		if(endchar == L'\r' || endchar == L'\n'){
			col = m_GI->m_currentCol;
			row++;
			if(buf[endpos] == L'\r' && buf[endpos+1]== L'\n')
				endpos++;
		}
		pos = endpos +1;

	}

	ClearSelections();
	SelectRange(m_GI->m_currentCol,m_GI->m_currentRow,maxcol,maxrow);

	RedrawAll();

	return UG_SUCCESS;
}
/***************************************************
return
	-1 failure
	otherwise datasource index number is returned
****************************************************/
int CUGCtrl::AddDataSource(CUGDataSource * ds)
{
	if(ds == NULL)
		return -1;

	int loop;

	//ds->m_ctrl = this;
	ds->m_GI = m_GI;

	if ( m_dataSrcListLength >= 63 )
	{	// do not allow users add more than 64 datasources to the Ultimate Grid
		return -1;
	}

	//alloc a larger DataSource List array if needed
	if( m_dataSrcListLength >= m_dataSrcListMaxLength - 1 )
	{
		m_dataSrcListMaxLength += 16;
		CUGDataSource ** temp = new CUGDataSource*[m_dataSrcListMaxLength];
		for( loop = 0;loop < m_dataSrcListMaxLength; loop++ )
		{
			if(loop < m_dataSrcListLength)
				temp[loop] = m_dataSrcList[loop];
			else
				temp[loop] = NULL;
		}
		delete[] m_dataSrcList;
		m_dataSrcList = temp;
	}
	for( loop = 1; loop < m_dataSrcListMaxLength; loop++ )
	{
		if(m_dataSrcList[loop] == NULL)
		{
			m_dataSrcList[loop] = ds;
			m_dataSrcListLength ++;
			ds->SetID(loop);
			return loop;
		}
	}

	return -1;
}
/***************************************************
GetDataSource
	Purpose
		Returns the pointer to the datasource at the 
		specified 0 based index. Datasources are added
		to the list through the AddDataSource member
		function
	Params
		index - 0 based index of datasource
	Returns
		NULL - failure
		otherwise the pointer to the datasource
****************************************************/
CUGDataSource * CUGCtrl::GetDataSource(int index){

	if(index <0 || index > m_dataSrcListLength)
		return NULL;

	//return the memory manager as the first datasource
	if(index == 0)
		return m_GI->m_CUGMem;
	else
		return m_dataSrcList[index];
}
/***************************************************
SetDefDataSource
	Purpose
		Sets the datasource for the grid to use as
		its default datasource. The datasource must
		first be in the datasource list (which the
		memory manager is by default, index 0).
	Params
		index - index of datasource to use as the default
	Return
		UG_SUCCESS	- success
		UG_ERROR	- failure
****************************************************/
int CUGCtrl::SetDefDataSource(int index){
	
	if(index <0 || index > m_dataSrcListLength)
		return UG_ERROR;

	//if the index is 0 then use the memory manager 
	//that was created in each sheet, otherwise use
	//a datasource from the datasource list
	m_GI->m_defDataSourceIndex = index;

	if(index == 0)
		m_GI->m_defDataSource = m_GI->m_CUGMem;
	else
		m_GI->m_defDataSource = m_dataSrcList[index];

	for(int cols = 0;cols < m_GI->m_numberCols;cols++){
		m_GI->m_colInfo[cols].dataSource = m_GI->m_defDataSource;
	}

	return UG_SUCCESS;
}
/***************************************************
GetDefDataSource
	Purpose
		Returns the index number of the default
		datasource for the grid
	Params
		none
	Return
		default data source index
****************************************************/
int CUGCtrl::GetDefDataSource(){
	return m_GI->m_defDataSourceIndex;
}
/***************************************************
RemoveDataSource 
	Purpose 
		Removes a datasource from the grids datasource
		list. This function DOES NOT DELETE the datasource
		object that was in the list.
	Params
		index -index of datasource to remove
	Return
		UG_SUCCESS	- success
		UG_ERROR	- failure
****************************************************/
int CUGCtrl::RemoveDataSource(int index){
	
	if(index <= 0 || index > m_dataSrcListLength)
		return UG_ERROR;

	if(m_dataSrcList[index] != NULL){
		m_dataSrcList[index] = NULL;
		return UG_SUCCESS;
	}

	return UG_ERROR;
}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetGridUsingDataSource(int index){

	CUGDataSource * ds= GetDataSource(index);

	if(ds == NULL)
		return UG_ERROR;

	int cols = ds->GetNumCols();
	if(cols <1)
		return 2;

	SetNumberCols(cols,FALSE);

	long rows = ds->GetNumRows();
	if(rows >=0)
		SetNumberRows(rows,FALSE);

	AdjustComponentSizes();

	return UG_SUCCESS;
}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
CXeMenu * CUGCtrl::GetPopupMenu()
{
	return m_menu.get();
}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::EmptyMenu(){
	
	m_menu->Clear();
	
	return UG_SUCCESS;
}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::AddMenuItem(int ID,LPCTSTR string){
//
//	if(lstrlen(string) == 0 || ID == -1){
//		if(m_menu->AppendMenu(MF_SEPARATOR,0,_T("")) != FALSE)
//			return UG_SUCCESS;
//	}
//	else{
//		if(m_menu->AppendMenu(MF_STRING,ID,string) != FALSE)
//			return UG_SUCCESS;
//	}
//
//	return UG_ERROR;
//}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::RemoveMenuItem(int ID){
//	
//	if(m_menu->DeleteMenu(ID,MF_BYCOMMAND) != FALSE)
//		return UG_SUCCESS;
//	
//	return UG_ERROR;
//}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::EnableMenu(BOOL state){
	
	if(state)
		m_GI->m_enablePopupMenu = TRUE;
	else
		m_GI->m_enablePopupMenu = FALSE;

	return UG_SUCCESS;
}
/***************************************************
UseHints
	Purpose
		The UseHints function is used to turn on and off
		the ToolTips functionality for the grid and its
		components.
	Params
		BOOL state - TRUE to enable the tooltips and FALSE to disable.
	Return
		UG_SUCCESS, this function will never fail
****************************************************/
int CUGCtrl::UseHints(BOOL state)
{
	if ( state )
	{
		m_GI->m_enableHints = TRUE;
		// enable tool tips on the grid
		//m_CUGGrid->EnableToolTips( TRUE );
		// enable tool tips on the top heading
		//m_CUGTopHdg->EnableToolTips( TRUE );
		// enable tool tips on the side heading
		//m_CUGSideHdg->EnableToolTips( TRUE );
		// enable tool tips on the corner button heading
		//m_CUGCnrBtn->EnableToolTips( TRUE );
	}
	else
	{
		m_GI->m_enableHints = TRUE;
		// disable tool tips on the grid
		//m_CUGGrid->EnableToolTips( FALSE );
		// disable tool tips on the top heading
		//m_CUGTopHdg->EnableToolTips( FALSE );
		// disable tool tips on the side heading
		//m_CUGSideHdg->EnableToolTips( FALSE );
		// disable tool tips on the corner button heading
		//m_CUGCnrBtn->EnableToolTips( FALSE );
	}
	return UG_SUCCESS;
}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::UseVScrollHints(BOOL state){
	if(state){
		m_GI->m_enableVScrollHints = TRUE;
		//m_CUGVScroll->EnableToolTips(TRUE);
	}
	else{
		m_GI->m_enableVScrollHints = FALSE;
		//m_CUGVScroll->EnableToolTips(FALSE);
	}
	return UG_SUCCESS;
}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::UseHScrollHints(BOOL state){
	if(state){
		m_GI->m_enableHScrollHints = TRUE;
		//m_CUGHScroll->EnableToolTips(TRUE);
	}
	else{
		m_GI->m_enableHScrollHints = FALSE;
		//m_CUGHScroll->EnableToolTips(FALSE);
	}
	return UG_SUCCESS;
}

//LRESULT CUGCtrl::OnMouseLeave(WPARAM wparam, LPARAM lparam)
//{
//	m_bTrackMouseLeave = FALSE;
//	m_xtooltip->HideTooltip();
//	return 0;
//}
//
//LRESULT CUGCtrl::OnMouseHover(WPARAM wparam, LPARAM lparam)
//{
//	return 0;
//}
//
//void CUGCtrl::OnPPToolTipNotify(NMHDR * pNotifyStruct, LRESULT * result)
//{
//}
//
//void CUGCtrl::OnPPTTNF_NeedTT(NMHDR * pNotifyStruct, LRESULT * result)
//{
//	NM_PPTOOLTIP_NEED_TT *pNeedTT = (NM_PPTOOLTIP_NEED_TT *)pNotifyStruct;
//	*result = 0;
//	ASSERT(pNeedTT);
//	if (pNeedTT)
//	{
//		SUPER_TOOLTIP_INFO sti;
//		sti.rgbBegin = m_xeUI->GetColor(CID::TT_BgBegin);
//		sti.rgbMid = m_xeUI->GetColor(CID::TT_BgMid);
//		sti.rgbEnd = m_xeUI->GetColor(CID::TT_BgEnd);
//		sti.rgbText = m_xeUI->GetColor(CID::TT_Text);
//		sti.pWnd = this;
//
//		CPoint pt(*pNeedTT->pt);
//		//CXeFileVw *pView = nullptr;
//		//int idx;
//		//CVwInfo *pTabInfo = FindTabAtPoint(pt, idx);
//		//if (pTabInfo && (pView = dynamic_cast<CXeFileVw *>(pTabInfo->m_pView)))
//		//{
//		//	if (pView->GetTooltipForTab(sti))
//		//	{
//		//		CDC *pDC = GetWindowDC();
//		//		CSize sizeTxt = pDC->GetTextExtent(sti.strBody);
//		//		sti.nSizeX = sizeTxt.cx;
//		//		ReleaseDC(pDC);
//
//				m_xtooltip->MakeSuperTip(&sti, *pNeedTT->ti);
//				pNeedTT->ti->hTWnd = GetSafeHwnd();
//				//pNeedTT->ti->rectBounds = pItem->rItem;
//				pNeedTT->ti->ptTipOffset.x = pt.x;
//				*result = 1;
//		//	}
//		//}
//
//		//const stXETB_ITEM *pItem = 0;
//		//FindItemFromPoint(pt, &pItem);
//		//if (pItem)
//		//{
//		//BOOL bMkToolTip = FALSE;
//		//if (pItem->bStyle == XETBS_RH_LOGO)
//		//{
//		//	sti.strBody = "Visit StreamTX website";
//		//	bMkToolTip = TRUE;
//		//}
//		//else if (pItem->bStyle == XETBS_MTX_CLOCK)
//		//{
//		//	sti.strBody = "Current time\nNote - is red when 'House TC'";
//		//	bMkToolTip = TRUE;
//		//}
//		//else if (pItem->bStyle == XETBS_PNG_ICON_BUTTON)
//		//{
//		//	sti.strBody = "";
//		//	sti.nIDText = pItem->uIDcommand;
//		//	bMkToolTip = TRUE;
//		//}
//		//else if (pItem->bStyle == XETBS_MENU_ITEM)
//		//{
//		//	sti.strBody.Format("'%s' menu", pItem->szMenuText);
//		//	bMkToolTip = TRUE;
//		//}
//
//		//sti.strBody = "Snorri test";
//		//bMkToolTip = TRUE;
//
//		//if (!bMkToolTip)
//		//{
//		//	CString strID, strTT;
//		//	strID.Format("%d", pItem->uIDcommand);
//		//	if (m_mapTTstrings.Lookup(strID, strTT))
//		//	{
//		//		sti.strBody = strTT;
//		//		bMkToolTip = TRUE;
//		//	}
//		//}
//
//		//if (bMkToolTip)
//		//{
//		//	m_xtooltip->MakeSuperTip(&sti, *pNeedTT->ti);
//		//	pNeedTT->ti->hTWnd = GetSafeHwnd();
//		//	//pNeedTT->ti->rectBounds = pItem->rItem;
//		//	//pNeedTT->ti->ptTipOffset.x = pItem->rItem.left;
//		//	*result = 1;
//		//}
//		//}
//	}
//}

/***************************************************
AddTab - int AddTab(LPCTSTR label,long ID);
	Purpose
		Adds a new tab to the grid to the end of the
		existing tabs. This function does not resize 
		the visible tab area. Use SetTabWidth for setting
		the size.
	Params
		label	- label to show in the tab
		ID		- ID number returned when the tab is 
				clicked
	Return
		UG_SUCCESS  - success
		UG_ERROR	- error
****************************************************/
//int CUGCtrl::AddTab( CString label, long ID )
//{
//	return m_CUGTab->AddTab(label,ID);
//}
/****************************************************
*****************************************************/
//int CUGCtrl::InsertTab( int pos, CString label, long ID )
//{
//	return m_CUGTab->InsertTab(pos,label,ID);
//}
/****************************************************
*****************************************************/
//int CUGCtrl::DeleteTab( long ID )
//{
//	return m_CUGTab->DeleteTab(ID);
//}
/****************************************************
*****************************************************/
//int CUGCtrl::SetTabWidth( int width )
//{
//	m_GI->m_tabWidth = width;
//	return UG_SUCCESS;
//}
/****************************************************
	Purpose
	Params
	Return
*****************************************************/
//int CUGCtrl::SetCurrentTab( long ID )
//{
//	if( m_CUGTab->SetCurrentTab(ID) == UG_SUCCESS )
//	{
//		return UG_SUCCESS;
//	}
//
//	return UG_ERROR;
//}
/****************************************************
	Purpose
	Params
	Return
*****************************************************/
//int CUGCtrl::GetCurrentTab()
//{
//	return m_CUGTab->GetCurrentTab();
//}
/****************************************************
	Purpose
	Params
	Return
*****************************************************/
//int CUGCtrl::SetTabBackColor( long ID, COLORREF color )
//{
//	return m_CUGTab->SetTabBackColor(ID,color);
//}
/****************************************************
	Purpose
	Params
	Return
*****************************************************/
//int CUGCtrl::SetTabTextColor( long ID, COLORREF color )
//{
//	return m_CUGTab->SetTabTextColor(ID,color);
//}
/****************************************************
	Purpose
	Params
	Return
*****************************************************/
int CUGCtrl::SetNumberSheets(int numSheets)
{
	if(numSheets < 1 || numSheets > 1024)
		return UG_ERROR;

	CUGGridInfo * origGI = m_GI;

	//create the new array
	CUGGridInfo ** temp = new CUGGridInfo *[numSheets];

	//copy the existing gridinfo into the new array
	int loop;
	for(loop = 0;loop < m_numberSheets;loop++){
		if(loop < numSheets)
			temp[loop] = m_GIList[loop];
		else
			delete m_GIList[loop];
	}

	//add new gridinfo elements to the new array
	for(loop = m_numberSheets ; loop < numSheets ;loop++){

		temp[loop] = new CUGGridInfo;

		//set up the default information
		XeASSERT(m_xeUI);
		//temp[loop]->m_multiSelect->m_ctrl	= this;
		temp[loop]->m_multiSelect->m_GI		= temp[loop];
		temp[loop]->m_xeUI					= m_xeUI;
		//temp[loop]->m_CUGMem->m_ctrl		= this;
		temp[loop]->m_CUGMem->m_GI			= temp[loop];
		temp[loop]->m_editCtrl				= m_defEditCtrl.m_hWnd;
		//temp[loop]->m_maskedEditCtrl		= &m_defMaskedEditCtrl;
		// switch all of the grid's components to the new sheet
		m_GI = temp[loop];
		m_CUGGrid->m_GI			= temp[loop];
		m_CUGTopHdg->m_GI		= temp[loop];
		m_CUGSideHdg->m_GI		= temp[loop];
		//m_CUGCnrBtn->m_GI		= temp[loop];
		m_CUGVScroll->m_GI		= temp[loop];
		m_CUGHScroll->m_GI		= temp[loop];
		//m_CUGTab->m_GI			= temp[loop];
		//#ifdef UG_ENABLE_PRINTING
		//	m_CUGPrint->m_GI	= m_GI;
		//#endif
		// allow the user to perform actions on each of the sheets
		OnSheetSetup(loop);
		// Allow drawing after the sheet is initialized
		temp[loop]->m_paintMode = TRUE;
	}
	
	//delete the old array, and use the new array
	if(m_GIList != NULL)
		delete[] m_GIList;
	m_GIList = temp;

	m_numberSheets = numSheets;

	// restore the default sheet
	m_GI = origGI;
	m_CUGGrid->m_GI			= origGI;
	m_CUGTopHdg->m_GI		= origGI;
	m_CUGSideHdg->m_GI		= origGI;
	//m_CUGCnrBtn->m_GI		= origGI;
	m_CUGVScroll->m_GI		= origGI;
	m_CUGHScroll->m_GI		= origGI;
	//m_CUGTab->m_GI			= origGI;
	//#ifdef UG_ENABLE_PRINTING
	//	m_CUGPrint->m_GI	= m_GI;
	//#endif

	if(m_currentSheet >= numSheets)
		SetSheetNumber(numSheets -1);
	
	return UG_SUCCESS;
}

int CUGCtrl::DeleteCurrentSheet( BOOL update )
{
	if( m_numberSheets <= 1 )	// check to see if more than one sheet in grid
		return UG_ERROR;

	CUGGridInfo * origGI = m_GI;	// save pointer to current sheet gridinfo

	// create a new array of pointers to gridinfo class
	int numSheets = m_numberSheets - 1;
	CUGGridInfo ** temp = new CUGGridInfo *[numSheets];

	// copy all but the gridinfo for current sheet into the new array
	for( int i = 0, loop = 0; loop < m_numberSheets; loop++ )
	{
		if( loop != m_currentSheet )
			temp[i++] = m_GIList[loop];
	}
	delete[] m_GIList;	// delete the old array
	m_GIList = temp;	// use the new array

	m_numberSheets = numSheets;

	// calc. index of new "current" sheet
	int newindex = ( m_currentSheet == 0 ) ? 0 : m_currentSheet - 1;
	SetSheetNumber( newindex, update );	// set new "current" sheet

	delete origGI;	// delete gridinfo class of previous "current" sheet

	return UG_SUCCESS;
}

/****************************************************
	Purpose
	Params
	Return
*****************************************************/
int CUGCtrl::GetNumberSheets(){
	return m_numberSheets;
}

/****************************************************
	Purpose
	Params
	Return
*****************************************************/
int CUGCtrl::SetSheetNumber(int index,BOOL update){

	//check to see if the number is valid
	if(index <0 || index >= m_numberSheets)
		return UG_ERROR;

	//check to see if the display should be updated
	BOOL origValue = m_enableUpdate;
	if(!update){
		m_enableUpdate = FALSE;
	}

	m_CUGGrid->SetFocus();

	//switch in the new CUGGridInfo Class
	//copy over the tab width to the new class
	//update all components so that they point to
	//the new CUGGridInfo class
	//update the multiSelect pointer so that it points
	//to the new CUGGridInfo class multiSelect class
	CUGGridInfo* oldInfo	= m_GI;
	m_GI					= m_GIList[index];

	m_GI->m_tabWidth = oldInfo->m_tabWidth;

	m_CUGGrid->m_GI			= m_GI;
	m_CUGTopHdg->m_GI		= m_GI;
	m_CUGSideHdg->m_GI		= m_GI;
	//m_CUGCnrBtn->m_GI		= m_GI;
	m_CUGVScroll->m_GI		= m_GI;
	m_CUGHScroll->m_GI		= m_GI;
	//m_CUGTab->m_GI			= m_GI;
	//#ifdef UG_ENABLE_PRINTING
	//	m_CUGPrint->m_GI	= m_GI;
	//#endif

	//store the current sheet number
	m_currentSheet = index;

	m_GI->m_lastTopRow = -1;
	m_GI->m_lastLeftCol = -1;

	//adjust the child window sizes - this also
	//causes the grid to re-draw
	AdjustComponentSizes();

	m_enableUpdate = origValue;

	return UG_SUCCESS;
}

/****************************************************
	Purpose
	Params
	Return
*****************************************************/
int CUGCtrl::GetSheetNumber(){
	return m_currentSheet;
}

/***************************************************
SetTH_NumberRows - int SetTH_NumberRows(int rows)
	Purpose
		Sets the number of rows available in the top
		heading of the grid. This function will not
		make the top heading disapear if set to 0, use
		the SetTH_Height for this purpose.
	Params
		rows - the number of rows to use in the top heading
	Return
		UG_SUCCESS	success		
		UG_ERROR	invalid number of rows
****************************************************/
int	CUGCtrl::SetTH_NumberRows(int rows){
	
	//range checking
	if(rows < 1 || rows > 64)
		return UG_ERROR;

	//copy the old height information
	if(m_GI->m_topHdgHeights != NULL){
		int * temp = new int[rows];
		for(int loop = 0;loop <rows;loop++){
			if(loop < m_GI->m_numberTopHdgRows)
				temp[loop] = m_GI->m_topHdgHeights[loop];
			else
				temp[loop] = m_GI->m_defRowHeight;		 
		}
		delete[] m_GI->m_topHdgHeights;
		m_GI->m_topHdgHeights = temp;
	}
	else{
		m_GI->m_topHdgHeights = new int[rows];
		for(int loop = 0;loop <rows;loop++)
			m_GI->m_topHdgHeights[loop] = m_GI->m_defRowHeight;
	}

	m_GI->m_numberTopHdgRows = rows;

	return UG_SUCCESS;
}

/***************************************************
SetTH_RowHeight
	Sets the height of a row in the top heading
	Top heading rows start from -1 then -2 and so on
	-1 row is the one closest to the grid it self
****************************************************/
int	CUGCtrl::SetTH_RowHeight(int row,int height)
{	
	//translate the row number into a 0 based positive index
	row = (row * -1) -1;

	if( row < 0 || row >= m_GI->m_numberTopHdgRows )
		return UG_ERROR;

	if( height < 0 || height > 1024 )
		return 2;

	if ( m_GI->m_topHdgHeights[row] == height )
		return UG_SUCCESS;
	
	if ( m_GI->m_numberTopHdgRows <= 1 )
		m_GI->m_topHdgHeight = height;
	
	m_GI->m_topHdgHeights[row] = height;

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int	CUGCtrl::SetSH_NumberCols(int cols){

	//range checking
	if(cols < 1 || cols > 64)
		return UG_ERROR;

	//copy the old width information
	if(m_GI->m_sideHdgWidths != NULL){
		int * temp = new int[cols];
		for(int loop = 0;loop <cols;loop++){
			if(loop < m_GI->m_numberSideHdgCols)
				temp[loop] = m_GI->m_sideHdgWidths[loop];
			else
				temp[loop] = m_GI->m_defColWidth;		 
		}
		delete[] m_GI->m_sideHdgWidths;
		m_GI->m_sideHdgWidths = temp;
	}
	else{
		m_GI->m_sideHdgWidths = new int[cols];
		for(int loop = 0;loop <cols;loop++)
			m_GI->m_sideHdgWidths[loop] = m_GI->m_defColWidth;
	}

	m_GI->m_numberSideHdgCols = cols;

	return UG_SUCCESS;
}

/***************************************************
SetSH_ColWidth
	Sets width of individual column in the grid
	Params
	Return
****************************************************/
int	CUGCtrl::SetSH_ColWidth(int col,int width)
{
	//translate the col number into a 0 based positive index
	col = (col * -1) -1;

	if( col < 0 || col >= m_GI->m_numberSideHdgCols )
		return UG_ERROR;

	if( width < 0 || width > 1024 )
		return 2;
	
	if( m_GI->m_sideHdgWidths[col] == width )
		return UG_SUCCESS;

	if( m_GI->m_numberSideHdgCols <= 1 )
		m_GI->m_sideHdgWidth = width;

	m_GI->m_sideHdgWidths[col] = width;

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
void CUGCtrl::SetLockRowHeight(){

	if(m_GI->m_numLockRows <1)
		return;

	m_GI->m_lockRowHeight = 0;
	for(int loop = 0;loop < m_GI->m_numLockRows;loop++){
		m_GI->m_lockRowHeight += GetRowHeight(loop);
	}

}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
void CUGCtrl::SetLockColWidth(){

	if(m_GI->m_numLockCols <1)
		return;

	m_GI->m_lockColWidth = 0;
	for(int loop = 0;loop < m_GI->m_numLockCols;loop++){
		m_GI->m_lockColWidth += GetColWidth(loop);
	}

}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetArrowCursor(HCURSOR cursor){
	if(cursor == NULL)
		return UG_ERROR;
	//m_GI->m_arrowCursor = cursor;
	XeASSERT(FALSE);	// No longer supported. Use m_xeUI->SetAppCursor().
	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetWESizingCursor(HCURSOR cursor){
	if(cursor == NULL)
		return UG_ERROR;
	m_GI->m_WEResizseCursor = cursor;
	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetNSSizingCursor(HCURSOR cursor){
	if(cursor == NULL)
		return UG_ERROR;
	m_GI->m_NSResizseCursor = cursor;
	return UG_SUCCESS;
}

/***************************************************
OnSetFocus
	Sent when the grid is gaining focus.
Note:
	The grid will loose focus when the edit is started,
	or drop list shown
Params:
	section - Id of the grid section gaining focus, usually UG_GRID
			  possible sections:
					UG_TOPHEADING, UG_SIDEHEADING, UG_GRID
Return:
	<none>
****************************************************/
void CUGCtrl::OnSetFocus(int section){
	int curcol = GetCurrentCol();
	int currow = GetCurrentRow();
	RedrawCell(-1, currow);
	RedrawCell(curcol, -1);
}

/***************************************************
OnKillFocus
	Sent when the grid is loosing focus.
Params:
	section - Id of the grid section loosing focus, usually UG_GRID
			  possible sections:
					UG_TOPHEADING, UG_SIDEHEADING, UG_GRID
Return:
	<none>
****************************************************/
void CUGCtrl::OnKillFocus(int section)
{
	int curcol = GetCurrentCol();
	int currow = GetCurrentRow();
	RedrawCell(-1, currow);
	RedrawCell(curcol, -1);
}

void CUGCtrl::OnKillFocus(int section, HWND hNewWnd)
{
	UNREFERENCED_PARAMETER(hNewWnd);
	// For backwards compatibility call older version
	// of the OnKillFocus notification.
	OnKillFocus(section);
}

/*********************************************
drag and drop
**********************************************/
#ifdef UG_ENABLE_DRAGDROP

/*********************************************
**********************************************/
//int CUGCtrl::StartDragDrop(){
//	
//	ReleaseCapture();
//	
//	CString SelectString;
//	int		len;
//
//	CreateSelectedString(SelectString,FALSE);
//	len = (SelectString.GetLength()+1) * sizeof(TCHAR);
//
//	// copy in both ansi and unicode formats if we 
//	// are running unicode and conversion to MBCS succeeds
//
//////////////////////////////////////////////////
//// non-unicode *********************************
//	HGLOBAL hglobal = GlobalAlloc(GMEM_ZEROINIT,len);
//	int res = 1;
//
//#	ifdef _UNICODE
//	LPSTR string = (LPSTR)GlobalLock(hglobal);
//	
//	// convert to text for copy...
//	res = WideCharToMultiByte(CP_ACP,0,SelectString,len,string,len,NULL,NULL);
//
//	if(!res) {			// use for debug.
//		int err = GetLastError();
//		switch (err) {
//		case ERROR_INSUFFICIENT_BUFFER:
//			break;
//		case ERROR_INVALID_FLAGS:
//			break;
//		case ERROR_INVALID_PARAMETER:
//			break;
//		default:
//			break;
//		}
//	}
//
//#	else 
//		LPTSTR string = (LPTSTR)GlobalLock(hglobal);
//#pragma warning(disable:4996)
//		_tcscpy(string,SelectString);
//#pragma warning(default:4996)
//#	endif
//
//////////////////////////////////////////////////////
//
//	GlobalUnlock(hglobal);
//	if(res)	// conversion ok
//		m_dataSource.CacheGlobalData(CF_TEXT,hglobal,NULL);
//
/////////////////////////////////////////////////////
//// unicode ****************************************
//
//#	ifdef _UNICODE
//	HGLOBAL hglobalu = GlobalAlloc(GMEM_ZEROINIT,len);
//
//	LPTSTR stringu = (LPTSTR)GlobalLock(hglobalu);
//	_tcscpy(stringu,SelectString);
//
//	GlobalUnlock(hglobalu);
//	m_dataSource.CacheGlobalData(CF_UNICODETEXT,hglobalu,NULL);
//#	endif
//
//
//////////////////////////////////////////////////////
//
//	m_dataSource.DoDragDrop(DROPEFFECT_COPY,NULL,NULL);
//
//	m_dataSource.Empty();
//
//	return UG_SUCCESS;
//}
/*********************************************
COleDropTarget
**********************************************/
//int CUGCtrl::DragDropTarget(BOOL state){
//
//	if(state == FALSE){
//		m_dropTarget.Revoke();
//	}
//	else{
//		m_dropTarget.Register(m_CUGGrid);
//	}
//	return UG_SUCCESS;
//}		

/***************************************************
OnDragEnter

Params:
	pDataObject - 
Return:
	DROPEFFECT_NONE - no drag and drop
	DROPEFFECT_COPY - allow drag and drop for copying
****************************************************/
DROPEFFECT  CUGCtrl::OnDragEnter(COleDataObject* pDataObject){
	UNREFERENCED_PARAMETER(*pDataObject);
	return DROPEFFECT_NONE;
}
/***************************************************
//	OnDragOver
//	Params:
//		col, row	-
//		pDataObject - 
//	Return:
//		DROPEFFECT_NONE - no drag and drop
//		DROPEFFECT_COPY - allow drag and drop for copying
****************************************************/
DROPEFFECT  CUGCtrl::OnDragOver(COleDataObject* pDataObject,int col,long row){
	UNREFERENCED_PARAMETER(*pDataObject);
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	return DROPEFFECT_NONE;
}
/***************************************************
//	OnDragDrop
//	Params:
//		col, row	-
//		pDataObject - 
//	Return:
//		DROPEFFECT_NONE - no drag and drop
//		DROPEFFECT_COPY - allow drag and drop for copying
****************************************************/
DROPEFFECT  CUGCtrl::OnDragDrop(COleDataObject* pDataObject,int col,long row){
	UNREFERENCED_PARAMETER(*pDataObject);
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	return DROPEFFECT_NONE;
}
#endif

/***************************************************
OnSetup
	This function is called just after the grid window 
	is created or attached to a dialog item.
	It can be used to initially setup the grid
****************************************************/
void CUGCtrl::OnSetup()
{
	//SetDoubleBufferMode(TRUE);

	SetMultiSelectMode(FALSE);

	//DragDropTarget( TRUE );	// Needed for drag and drop

	EnableJoins(TRUE);

	//****** Add the SortArrow cell type 
	//m_gridSize.m_nSortArrowCell_index = AddCellType(&m_sortarrowcell);

	// Add Url button cell type
	//m_gridSize.m_nUrlBtnCell_index = AddCellType(&m_urlbtncell);

	//OnSheetSetup( 0 ); // setup first sheet

	//SetDefFont(m_xeUI->GetFont(EXE_FONT::eUI_Font));	// Set default font  

	// configure grid layout from data source
	//SetGridFromDataSource(FALSE);
	//SetPaintMode(FALSE);
	SetNumberCols(GetGridNumCols(), FALSE);
	SetNumberRows(GetGridNumRows(), FALSE);
	int nNumTHrows = 1;
	if (GetDataSource()->HasGridHeader())
		nNumTHrows = 2;

	SetTH_NumberRows(nNumTHrows);

	AdjustComponentSizes();

	SetGridSizeToGrid();
}

/***************************************************
OnSheetSetup	
	This notification is called for each additional sheet that the grid
	might contain, here you can customize each sheet in the grid.
Params:
	sheetNumber - idndex of current sheet
Return:
	<none>
****************************************************/
void CUGCtrl::OnSheetSetup(int sheetNumber)
{
	//*** initialize new sheet info. structure
		// Initialize GridSize
	CGridTblInfo& tblInfo = GetDataSource()->GetGridTblInfo();
	std::wstring strRegSectionName{ m_strRegSectionName };
	tblInfo.LoadColWidthsUserSettings(strRegSectionName, L"GridCol");

	UseHints(TRUE); 						// Enable Hints 

	SetUniformRowHeight(TRUE); 			// Set Uniform row height

	//EnableCellOverLap(FALSE); 			// Allow cell overlapping

	const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
	int cyFont = tm.GetHeight() + tm.GetInternalLeading();
	SetDefRowHeight(cyFont); 					// Set the grid's default row height 

	m_GI->SetTH_HeightValue(cyFont + 4); 			// Set the total height of the top heading 

	EnableMenu(TRUE);

	SetVScrollMode(UG_SCROLLTRACKING);
	SetHScrollMode(UG_SCROLLTRACKING);

	SetMultiSelectMode(TRUE);

	//// have the top and side headings appear on the printed output
	//PrintSetOption( UG_PRINT_SIDEHEADING, TRUE );
	//PrintSetOption( UG_PRINT_TOPHEADING, TRUE );
	//PrintSetOption( UG_PRINT_FRAME, FALSE );

	//// have a one inch border on both the top and bottom of each page
	//PrintSetOption( UG_PRINT_TOPMARGIN, 10 );
	//PrintSetOption( UG_PRINT_BOTTOMMARGIN, 10 );
	//PrintSetOption( UG_PRINT_LEFTMARGIN, 5 );
	//PrintSetOption( UG_PRINT_RIGHTMARGIN, 5 );
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
int CUGCtrl::OnCanMove(int oldcol,long oldrow,int newcol,long newrow){
	// show SH for current row as BLUE
	if (oldrow != newrow)
	{
		m_gridsel.m_nGridCurRow = newrow;
		RedrawCell(-1, oldrow);
		RedrawCell(-1, newrow);

		// Always redraw col 0 header - this is because we don't want col 0 header drawn using "current" col bg when cur row is multiline (stacktrace).
		RedrawCell(0, -1);
	}

	// show TH for current column as BLUE
	if (oldcol != newcol)
	{
		m_gridsel.m_nGridCurCol = newcol;
		RedrawCell(oldcol, -1);
		RedrawCell(newcol, -1);
	}
	return TRUE;
}
/***************************************************
OnCanViewMove
	is called when current grid's view is about to be scrolled.
Params:
	oldcol, oldrow - coordinates of orriginal top-left cell
	newcol, newrow - coordinates of top-left cell that is gaining the focus
Return:
	TRUE - to allow for the scroll
	FALSE - to prevent the view from scrolling
****************************************************/
int CUGCtrl::OnCanViewMove(int oldcol,long oldrow,int newcol,long newrow){
	UNREFERENCED_PARAMETER(oldcol);
	UNREFERENCED_PARAMETER(oldrow);
	UNREFERENCED_PARAMETER(newcol);
	UNREFERENCED_PARAMETER(newrow);
	return TRUE;
}

/***************************************************
OnHitBottom
	This notification allows for dynamic row loading, it will be called
	when the grid's drawing function has hit the last row.  It allows the grid
	to ask the datasource/developer if there are additional rows to be displayed.
Params:
	numrows		- known number of rows in the grid
	rowspast	- number of extra rows that the grid is looking for in the datasource
	rowsfound	- number of rows actually found, usually equal to rowspast or zero.
Return:
	<none>
****************************************************/
void CUGCtrl::OnHitBottom(long numrows,long rowspast,long rowsfound){
	UNREFERENCED_PARAMETER(numrows);
	UNREFERENCED_PARAMETER(rowspast);
	UNREFERENCED_PARAMETER(rowsfound);
}

/***************************************************
OnHitTop
	Is called when the user has scrolled all the way to the top of the grid.
Params:
	numrows		- known number of rows in the grid
	rowspast	- number of extra rows that the grid is looking for in the datasource
Return:
	<none>
****************************************************/
void CUGCtrl::OnHitTop(long numrows,long rowspast){
	UNREFERENCED_PARAMETER(numrows);
	UNREFERENCED_PARAMETER(rowspast);
}

/***************************************************
OnCanSizeCol
	is sent when the mouse was moved in the area between
	columns on the top heading, indicating that the user
	might want to resize a column.
Params:
	col - identifies column number that will be sized
Return:
	TRUE - to allow sizing
	FALSE - to prevent sizing
****************************************************/
int CUGCtrl::OnCanSizeCol(int col){
	UNREFERENCED_PARAMETER(col);
	return TRUE;
}

/***************************************************
OnColSizing
	is sent continuously when user is sizing a column.
	This notification is ideal to provide min/max width checks.
Params:
	col - column currently sizing
	width - pointer to a new column width
Return:
	<none>
****************************************************/
void CUGCtrl::OnColSizing(int col,int *width){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(*width);
}

/***************************************************
OnColSized
	This is sent when the user finished sizing the 
	given column.(see above for more information)
Params:
	col - column sized
	width - pointer to new column width
Return:
	<none>
****************************************************/
void CUGCtrl::OnColSized(int col,int *width){
	//SendDataGridColWidthChangedMessage(col, *width);
}

/***************************************************
OnCanSizeRow
	is sent when the mouse was moved in the area between
	rows on the side heading, indicating that the user
	might want to resize a row.
Params:
	row - identifies row number that will be sized
Return:
	TRUE - to allow sizing
	FALSE - to prevent sizing
****************************************************/
int  CUGCtrl::OnCanSizeRow(long row){
	UNREFERENCED_PARAMETER(row);
	return FALSE;
}

/***************************************************
OnRowSizing
	Sent durring user sizing of a row, can provide
	feed back on current height
Params:
	row - row sizing
	height - pointer to current new row height
Return:
	<none>
****************************************************/
void CUGCtrl::OnRowSizing(long row,int *height){
	if (*height < 10)
		*height = 10;
}

/***************************************************
OnRowSized
	This is sent when the user is finished sizing hte
	given row.
Params:
	row - row sized
	height - pointer to new row height
Return:
	<none>
****************************************************/
void CUGCtrl::OnRowSized(long row,int *height){
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*height);
}

/***************************************************
OnCanSizeSideHdg
	Sent when the user is about to start sizing of the side heading
Params:
	<none>
Return:
	TRUE - to allow sizing
	FALSE - to prevent sizing
****************************************************/
int CUGCtrl::OnCanSizeSideHdg(){
	return TRUE;
}

/***************************************************
OnCanSizeTopHdg
	Sent when the user is about to start sizing of the top heading
Params:
	<none>
Return:
	TRUE - to allow sizing
	FALSE - to prevent sizing
****************************************************/
int CUGCtrl::OnCanSizeTopHdg(){
	return FALSE;
}

/***************************************************
OnSideHdgSizing
	Sent while the heading is being sized
Params:
	width - pointer to current new width of the side heading
Return:
	TRUE - to accept current new size
	FALSE - to stop sizing, the size is either too large or too small
****************************************************/
int CUGCtrl::OnSideHdgSizing(int *width){
	if (*width < 10)
		*width = 10;
	return TRUE;
}

/***************************************************
OnTopHdgSizing
	Sent while the top heading is being sized
Params:
	height - pointer to current new height of the top heading
Return:
	TRUE - to accept current new size
	FALSE - to stop sizing, the size is either too large or too small
****************************************************/
int CUGCtrl::OnTopHdgSizing(int *height){
	if (*height < 10)
		*height = 10;
	return TRUE;
}

/***************************************************
OnSideHdgSized
	Sent when the user has completed the sizing of the side heading
Params:
	width - pointer to new width
Return:
	TRUE - to accept new size
	FALSE - to revert to old size
****************************************************/
int CUGCtrl::OnSideHdgSized(int *width){
	//SendDataGridColWidthChangedMessage(-1, *width);
	return 0;
}

/***************************************************
OnTopHdgSized
	Sent when the user has completed the sizing of the top heading
Params:
	height - pointer to new height
Return:
	TRUE - to accept new size
	FALSE - to revert to old size
****************************************************/
int CUGCtrl::OnTopHdgSized(int *height){
	UNREFERENCED_PARAMETER(*height);
	return 0;
}

void CUGCtrl::OnColRowSizeFinished()
{
	GetGridSizeFromGrid();
	CGridTblInfo& tblInfo = GetDataSource()->GetGridTblInfo();
	std::wstring strRegSectionName{ m_strRegSectionName };
	tblInfo.SaveColWidthsUserSettings(strRegSectionName, L"GridCol");
}

/***************************************************
OnColChange
	Sent whenever the current column changes
Params:
	oldcol - column that is loosing the focus
	newcol - column that the user move into
Return:
	<none>
****************************************************/
void CUGCtrl::OnColChange(int oldcol,int newcol){
	UNREFERENCED_PARAMETER(oldcol);
	UNREFERENCED_PARAMETER(newcol);
}

/***************************************************
OnRowChange
	Sent whenever the current row changes
Params:
	oldrow - row that is loosing the locus
	newrow - row that user moved into
Return:
	<none>
****************************************************/
void CUGCtrl::OnRowChange(long oldrow,long newrow){
	UNREFERENCED_PARAMETER(oldrow);
	UNREFERENCED_PARAMETER(newrow);
}

/***************************************************
OnCellChange
	Sent whenever the current cell changes
Params:
	oldcol, oldrow - coordinates of cell that is loosing the focus
	newcol, newrow - coordinates of cell that is gaining the focus
Return:
	<none>
****************************************************/
void CUGCtrl::OnCellChange(int oldcol,int newcol,long oldrow,long newrow){
	UNREFERENCED_PARAMETER(oldcol);
	UNREFERENCED_PARAMETER(newcol);
	UNREFERENCED_PARAMETER(oldrow);
	UNREFERENCED_PARAMETER(newrow);
}

/***************************************************
OnLeftColChange
	Sent whenever the left visible column in the grid changes, indicating
	that the view was scrolled horizontaly
Params:
	oldcol - column that used to be on the left
	newcol - new column that is going to be the first visible column from the left
Return:
	<none>
****************************************************/
void CUGCtrl::OnLeftColChange(int oldcol,int newcol){
	UNREFERENCED_PARAMETER(oldcol);
	UNREFERENCED_PARAMETER(newcol);
}

/***************************************************
OnTopRowChange
	Sent whenever the top visible row in the grid changes, indicating
	that the view was scrolled vertically
Params:
	oldrow - row that used to be on the top
	newrow - new row that is going to be the first visible row from the top
Return:
	<none>
****************************************************/
void CUGCtrl::OnTopRowChange( long oldrow, long newrow )
{
	UNREFERENCED_PARAMETER(oldrow);
	UNREFERENCED_PARAMETER(newrow);
}

/***************************************************
OnViewMoved
	This notification is fired after the current viewing area
	was scrolled.
Params:
	nScrolDir - UG_VSCROLL, UG_HSCROLL

	if the nScrolDir == UG_VSCROLL
		oldPos - row that used to be on the top
		newPos - row that is now the first visible row from the top

	if the nScrolDir == UG_VSCROLL
		oldPos - column that used to be on the left
		newPos - column that is now the first visible column from the left
Return:
	<none>
****************************************************/
void CUGCtrl::OnViewMoved( int nScrolDir, long oldPos, long newPos )
{
	UNREFERENCED_PARAMETER(nScrolDir);
	UNREFERENCED_PARAMETER(oldPos);
	UNREFERENCED_PARAMETER(newPos);
}

/***************************************************
OnLClicked
	Sent whenever the user clicks the left mouse button within the grid
	this message is sent when the button goes down then again when the button goes up
Params:
	col, row	- coordinates of a cell that received mouse click event
	updn		- is TRUE if mouse button is 'down' and FALSE if button is 'up'
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
	if (!updn)	// Left mouse button up? Clear last Lbtn dn. pos.
		m_ptLastLbtnDnMousePos.x = m_ptLastLbtnDnMousePos.y = -1000000;

	BOOL fCtrlKeyDown = (::GetKeyState(VK_CONTROL) & 0x8000) ? TRUE : FALSE;
	BOOL fShiftKeyDown = (::GetKeyState(VK_SHIFT) & 0x8000) ? TRUE : FALSE;
	BOOL fMenuKeyDown = (::GetKeyState(VK_MENU) & 0x8000) ? TRUE : FALSE;
	if (fCtrlKeyDown || fShiftKeyDown || fMenuKeyDown)
		return;
	if (updn)	// Left mouse button down ?
		m_ptLastLbtnDnMousePos = *point;	// Save client coords. of cursor.
}

/***************************************************
OnRClicked
	Sent whenever the user clicks the right mouse button within the grid
	this message is sent when the button goes down then again when the button goes up
Params:
	col, row	- coordinates of a cell that received mouse click event
	updn		- is TRUE if mouse button is 'down' and FALSE if button is 'up'
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(updn);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
}

/***************************************************
OnDClicked
	Sent whenever the user double clicks the left mouse button within the grid
Params:
	col, row	- coordinates of a cell that received mouse click event
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
	StartEdit();
}

/***************************************************
OnMouseMove
	is sent when the user moves mouse over the grid area.
Params:
	col, row	- coordinates of a cell that received mouse click event
	point		- represents the screen point where the mouse event was detected
	nFlags		- 
	processed	- indicates if current event was processed by other control in the grid.
Return:
	<none>
****************************************************/
void CUGCtrl::OnMouseMove(int col,long row,POINT *point,UINT nFlags,BOOL processed){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(nFlags);
	UNREFERENCED_PARAMETER(processed);

	//if (!m_bTrackMouseLeave)
	//{	// We want a WM_MOUSELEAVE message when mouse leaves our client area.
	//	TRACKMOUSEEVENT tme;
	//	tme.cbSize = sizeof(tme);
	//	tme.hwndTrack = m_hWnd;
	//	tme.dwFlags = TME_LEAVE;
	//	tme.dwHoverTime = 0;

	//	// Note - _TrackMouseEvent is from comctl32.dll - emulates TrackMouseEvent
	//	// if it does not exist.
	//	m_bTrackMouseLeave = _TrackMouseEvent(&tme);
	//}
}

/***************************************************
OnTH_LClicked
	Sent whenever the user clicks the left mouse button within the top heading
	this message is sent when the button goes down then again when the button goes up
Params:
	col, row	- coordinates of a cell that received mouse click event
	updn		- is TRUE if mouse button is 'down' and FALSE if button is 'up'
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
	if (updn || row < -1 || !SortingEnabled())
		return;

	//if (m_gridSize.m_nGridSortByCol == col)	// L click on same header col as last time?
	//{
	//	if (m_gridSize.m_nGridSortOrder == UG_SORT_DESCENDING)
	//	{	// Was descending - turn sort off.
	//		m_gridSize.m_nGridSortByCol = -1;
	//		m_gridSize.m_nGridSortOrder = UG_SORT_ASCENDING;
	//	}
	//	else
	//	{
	//		m_gridSize.m_nGridSortOrder = (m_gridSize.m_nGridSortOrder == UG_SORT_ASCENDING)
	//			? UG_SORT_DESCENDING : UG_SORT_ASCENDING;
	//	}
	//}
	//else
	//{
	//	m_gridSize.m_nGridSortByCol = col;
	//	m_gridSize.m_nGridSortOrder = UG_SORT_ASCENDING;
	//}
	//CWaitCursor wait;
	//ClearSelections();
	//if (m_gridSize.m_nGridSortByCol >= 0)
	//{
	//	GetDataSource()->SortBy(m_gridSize.m_nGridSortByCol, m_gridSize.m_nGridSortOrder);
	//}
	//else
	//{	// Reset list sorting to unsorted.
	//	GetDataSource()->SortUnsorted();
	//}
	RedrawAll();
}

/***************************************************
OnTH_RClicked
	Sent whenever the user clicks the right mouse button within the top heading
	this message is sent when the button goes down then again when the button goes up
Params:
	col, row	- coordinates of a cell that received mouse click event
	updn		- is TRUE if mouse button is 'down' and FALSE if button is 'up'
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnTH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(updn);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
}

/***************************************************
OnTH_DClicked
	Sent whenever the user double clicks the left mouse
	button within the top heading
Params:
	col, row	- coordinates of a cell that received mouse click event
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
}

/***************************************************
OnSH_LClicked
	Sent whenever the user clicks the left mouse button within the side heading
	this message is sent when the button goes down then again when the button goes up
Params:
	col, row	- coordinates of a cell that received mouse click event
	updn		- is TRUE if mouse button is 'down' and FALSE if button is 'up'
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnSH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);

	if (!updn || col != -1)
		return;
	SHORT s = ::GetAsyncKeyState(VK_SHIFT);
	BOOL fShift = (s & 0x8000);
	s = ::GetAsyncKeyState(VK_CONTROL);
	BOOL fCtrl = (s & 0x8000);
	s = ::GetAsyncKeyState(VK_MENU);
	BOOL fAlt = (s & 0x8000);

	int endcol = GetNumberCols() - 1;
	if (!GetSelection())
		fShift = FALSE;	// nothing is selected so we ignore shift key state
	else
	{	// check if selection is valid (only full row(s) selection is valid)
		if (m_gridsel.m_nStartCol != 0 || m_gridsel.m_nEndCol != endcol)
			fShift = FALSE;	// selection invalid, ignore shift key state
	}
	if (fShift)
	{
		if (row < m_gridsel.m_nStartRow)
			m_gridsel.m_nStartRow = row;
		else if (row > m_gridsel.m_nEndRow)
			m_gridsel.m_nEndRow = row;
	}
	else
		m_gridsel.m_nStartRow = m_gridsel.m_nEndRow = row;
	if (!(fShift || fCtrl || fAlt))
	{
		GotoRowEx(row);
	}
	SetSelection(FALSE, 0, m_gridsel.m_nStartRow, endcol, m_gridsel.m_nEndRow);
}

/***************************************************
OnSH_RClicked
	Sent whenever the user clicks the right mouse button within the side heading
	this message is sent when the button goes down then again when the button goes up
Params:
	col, row	- coordinates of a cell that received mouse click event
	updn		- is TRUE if mouse button is 'down' and FALSE if button is 'up'
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnSH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(updn);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
}

/***************************************************
OnSH_DClicked
	Sent whenever the user double clicks the left mouse button within the side heading
Params:
	col, row	- coordinates of a cell that received mouse click event
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnSH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
}

/***************************************************
OnCB_LClicked
	Sent whenever the user clicks the left mouse button within the top corner button
	this message is sent when the button goes down then again when the button goes up
Params:
	updn		- is TRUE if mouse button is 'down' and FALSE if button is 'up'
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnCB_LClicked(int updn,RECT *rect,POINT *point,BOOL processed){
	UNREFERENCED_PARAMETER(updn);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
}

/***************************************************
OnCB_RClicked
	Sent whenever the user clicks the right mouse button within the top corner button
	this message is sent when the button goes down then again when the button goes up
Params:
	updn		- is TRUE if mouse button is 'down' and FALSE if button is 'up'
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnCB_RClicked(int updn,RECT *rect,POINT *point,BOOL processed){
	UNREFERENCED_PARAMETER(updn);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
}

/***************************************************
OnCB_DClicked
	Sent whenever the user double clicks the left mouse
	button within the top corner button
Params:
	processed	- indicates if current event was processed by other control in the grid.
	rect		- represents the CDC rectangle of cell in question
	point		- represents the screen point where the mouse event was detected
Return:
	<none>
****************************************************/
void CUGCtrl::OnCB_DClicked(RECT *rect,POINT *point,BOOL processed){
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
}

/***************************************************
OnKeyDown
	Sent when grid received a WM_KEYDOWN message, usually as a result
	of a user pressing any key on the keyboard.
Params:
	vcKey		- virtual key code of the key user has pressed
	processed	- indicates if current event was processed by other control in the grid.
Return:
	<none>
****************************************************/
void CUGCtrl::OnKeyDown(UINT *vcKey,BOOL processed){
	// Allow data source first chance to process the keyboard command
	eGridAction action = eGridAction::NO_ACTION;
	if (GetDataSource()->ProcessKeydownMessage((int)*vcKey, action, m_gridsel))
	{
		processed = true;
		if (action == eGridAction::REDRAW_ROW)
			RedrawRow(GetCurrentRow());
		else if (action == eGridAction::REDRAW_ALL)
			RefreshGrid();
		else if (action == eGridAction::SET_GRID_FROM_DS)
		{
			GetGridSizeFromGrid();
			SetGridFromDataSource(TRUE);
			GetSelection();
		}
		else if (action == eGridAction::UPDATE_ROW_COUNT)
		{
			UpdateRowCount();
		}
		return;
	}

	BOOL fCtrlKeyDown = (::GetKeyState(VK_CONTROL) & 0x8000) ? TRUE : FALSE;
	BOOL fShiftKeyDown = (::GetKeyState(VK_SHIFT) & 0x8000) ? TRUE : FALSE;
	BOOL fMenuKeyDown = (::GetKeyState(VK_MENU) & 0x8000) ? TRUE : FALSE;
	if (!(fShiftKeyDown || fCtrlKeyDown || fMenuKeyDown))
	{
		if (*vcKey == VK_F2 && !m_GI->m_editInProgress)
		{
			processed = true;
			StartEdit();
		}
	}
	else if (fCtrlKeyDown)
	{
		if (!(fShiftKeyDown || fMenuKeyDown))	// NOT Shift or Alt key down?
		{
			switch (*vcKey)
			{
			case 0x41:	// Ctrl-A (select all)
				processed = ClipboardOperation(UG_EDIT_SELECTALL);
				break;
			case 0x58:	// Ctrl-X (cut)
				processed = ClipboardOperation(UG_EDIT_CUT);
				break;
			case 0x43:	// Ctrl-C (copy)
				processed = ClipboardOperation(UG_EDIT_COPY);
				break;
			case 0x56:	// Ctrl-V (paste)
				processed = ClipboardOperation(UG_EDIT_PASTE);
				break;
			case 0x5A:	// Ctrl-Z (undo)
				processed = ClipboardOperation(UG_EDIT_UNDO);
				break;
				// Note - FindBar UI belongs to XeFileVw
				//case 0x46:	// Ctrl-F (find dlg)
				//	OnEditFind();
				//	processed = true;
				//	break;
				//case 0x50:	// Ctrl-P (print dlg)
				//	PrintGrid();
				//	processed = true;
				//	break;
			}
		}
	}
}

void CUGCtrl::OnEditFind()
{
	//_NotifyParentView(_GetNotifyData(eGridNfShowFind));
}

void CUGCtrl::_UpdateMenuCallback(CXeMenu* pMenu, size_t top_level_index)
{
	XeASSERT(pMenu);
	if (pMenu)
	{
		CXeSubMenu* pSubMenu = pMenu->GetTopLevelMenuItem(top_level_index);
		for (ListBoxExItem& menu_item : pSubMenu->m_items)
		{
			switch ((UINT)menu_item.m_extra_data)
			{
			case ID_EDIT_FIND:
				menu_item.EnableItem(GetNumberRows() > 0);
				break;
			}
		}
	}
}

stGridNotifyData CUGCtrl::_GetNotifyData(EGRIDOP eOp, int col, long row, int section)
{
	stGridNotifyData nfData(eOp, col, col, row, section, 0, m_gridsel, ::GetParent(Hwnd()));
	return nfData;
}

stGridNotifyData CUGCtrl::_GetNotifyDataFlag(EGRIDOP eOp, bool bFlag)
{
	stGridNotifyData nfData = _GetNotifyData(eOp);
	nfData.m_bFlag = bFlag;
	return nfData;
}

void CUGCtrl::SetGridFromDataSource(BOOL fRedraw /*= TRUE*/)
{
	SetPaintMode(FALSE);
	SetNumberCols(GetGridNumCols(), FALSE);
	SetNumberRows(GetGridNumRows(), FALSE);
	int nNumTHrows = 1;
	if (GetDataSource()->HasGridHeader())
		nNumTHrows = 2;

	SetTH_NumberRows(nNumTHrows);

	AdjustComponentSizes();

	SetGridSizeToGrid();
	SetPaintMode(TRUE, fRedraw);
	if (fRedraw)
		_RedrawDirectly();
}

void CUGCtrl::GetGridSizeFromGrid()
{
	int nNumTHrows = 1;
	if (GetDataSource()->HasGridHeader())
		nNumTHrows++;

	int nH = GetTH_Height();	// total height of all TH rows

	CGridTblInfo& tblInfo = GetDataSource()->GetGridTblInfo();
	tblInfo.SetSHwidth(GetSH_Width());

	int nNumCols = GetGridNumCols();
	for (int col = 0; col < nNumCols; col++)
	{
		tblInfo.SetColWidth(col, GetColWidth(col));
	}
}

void CUGCtrl::SetGridSizeToGrid()
{
	//******* Set the total height of the top heading 
	int nTHrowHeight = m_xeUI->GetValue(UIV::s_cyGridTH_RowHeight);
	int nNumTHrows = 1;
	if (GetDataSource()->HasGridHeader())
		nNumTHrows++;
	if (nNumTHrows > 1)
	{
		SetTH_Height((nTHrowHeight * nNumTHrows));

		//******* Set the heights of the individual heading rows 
		SetTH_RowHeight(-1, nTHrowHeight);
		SetTH_RowHeight(-2, nTHrowHeight);
		if (nNumTHrows > 2)
			SetTH_RowHeight(-3, nTHrowHeight);
	}
	else
		SetTH_Height(nTHrowHeight);

	CGridTblInfo& tblInfo = GetDataSource()->GetGridTblInfo();
	SetSH_Width(tblInfo.GetSHwidth());
	const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
	SetDefRowHeight(tm.GetHeight() + tm.GetInternalLeading());
	for (int col = 0; col < GetNumberCols(); col++)
	{
		SetColWidth(col, tblInfo.GetColWidth(col));
	}
	RedrawAll();
}

BOOL CUGCtrl::ClipboardOperation(UINT uOperation)
{	// clipboard operations relating to the cell currently in edit mode 
	//   or the entire grid if no cell is currently in edit mode
	switch (uOperation)
	{
	case UG_EDIT_COPY:
		if (m_GI->m_editInProgress)
		{
			::SendMessageW(m_GI->m_editCtrl, WM_COPY, 0, 0);
			//((CEdit*)m_GI->m_editCtrl)->Copy();
		}
		else
			CopySelectionToClipboard();
		break;
	case UG_EDIT_UNDO:
		if (m_GI->m_editInProgress)
		{
			::SendMessageW(m_GI->m_editCtrl, EM_UNDO, 0, 0);
			//((CEdit*)m_GI->m_editCtrl)->Undo();
		}
		break;
	case UG_EDIT_SELECTALL:
		if (m_GI->m_editInProgress)
		{
			::SendMessageW(m_GI->m_editCtrl, EM_SETSEL, 0, -1);
			//((CEdit*)m_GI->m_editCtrl)->SetSel(0, -1);
		}
		else
			SetSelection();
		break;
	}
	return TRUE;
}

/***************************************************
OnKeyUp
	Sent when grid received a WM_KEYUP message, usually as a result
	of a user releasing a key.
Params:
	vcKey		- virtual key code of the key user has pressed
	processed	- indicates if current event was processed by other control in the grid.
Return:
	<none>
****************************************************/
void CUGCtrl::OnKeyUp(UINT *vcKey,BOOL processed){
	UNREFERENCED_PARAMETER(*vcKey);
	UNREFERENCED_PARAMETER(processed);
}

/***************************************************
OnCharDown
	Sent when grid received a WM_CHAR message, usually as a result
	of a user pressing any key that represents a printable characrer.
Params:
	vcKey		- virtual key code of the key user has pressed
	processed	- indicates if current event was processed by other control in the grid.
Return:
	<none>
****************************************************/
void CUGCtrl::OnCharDown(UINT *vcKey,BOOL processed){
	UNREFERENCED_PARAMETER(*vcKey);
	UNREFERENCED_PARAMETER(processed);
}
	
/***************************************************
OnGetCell
	This message is sent everytime the grid needs to
	draw a cell in the grid. At this point the cell
	object has been populated with all of the information
	that will be used to draw the cell. This information
	can now be changed before it is used for drawing.
Warning:
	This notification is called for each cell that needs to be painted
	Placing complicated calculations here will slowdown the refresh speed.
Params:
	col, row	- coordinates of cell currently drawing
	cell		- pointer to the cell object that is being drawn
Return:
	<none>
****************************************************/
void CUGCtrl::OnGetCell(int col,long row,CUGCell *cell){
	GetDataSource()->GetCellForGrid(col, row, cell, m_gridsel);
}

/***************************************************
OnSetCell
	This notification is sent everytime a cell is set,
	here you have a chance to make final modification
	to the cell's content before it is saved to the data source.
Params:
	col, row	- coordinates of cell currently saving
	cell		- pointer to the cell object that is being saved
Return:
	<none>
****************************************************/
void CUGCtrl::OnSetCell(int col,long row,CUGCell *cell){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*cell);
}

/***************************************************
OnDataSourceNotify
	This message is sent from a data source and this message
	depends on the data source - check the information
	on the data source(s) being used
	- The ID of the Data source is also returned
Params:
	ID		- datasource ID
	msg		- message ID to identify current process
	param	- additional iformation or object that might be needed
Return:
	<none>
****************************************************/
void CUGCtrl::OnDataSourceNotify(int ID,long msg,long param){
	UNREFERENCED_PARAMETER(ID);
	UNREFERENCED_PARAMETER(msg);
	UNREFERENCED_PARAMETER(param);
}

/***************************************************
OnCellTypeNotify
	This notification is sent by the celltype and it is different from cell-type
	to celltype and even from notification to notification.  It is usually used to
	provide the developer with some feed back on the cell events and sometimes to
	ask the developer if given event is to be accepted or not
Params:
	ID			- celltype ID
	col, row	- co-ordinates cell that is processing the message
	msg			- message ID to identify current process
	param		- additional iformation or object that might be needed
Return:
	TRUE - to allow celltype event
	FALSE - to disallow the celltype event
****************************************************/
int CUGCtrl::OnCellTypeNotify(long ID,int col,long row,long msg,long long param){
	switch (ID)
	{
	case UGCT_CHECKBOX:
		return OnCheckbox(ID, col, row, msg, param);
	case UGCT_DROPLIST:
		return OnDropList(ID, col, row, msg, param);
	}
	//if( ID == m_nUrlBtnCell_index )
	//	return OnUrlButton( ID, col, row, msg, param );
	//if(ID == m_nRadioIndex){
	//}
	//if(ID == m_nDateTimeIndex){
	//	return OnDateTime(ID, col, row, msg, param);		
	//}
	//if(ID == m_nButtonIndex){
	//	return OnPushButton( ID, col, row, msg, param);
	//}
	//if(ID == m_nEllipsisIndex){
	//	return OnEllipsisButton( ID, col, row, msg, param);
	//}
	//if(ID == m_nSpinIndex){
	//	return OnSpinButton( ID, col, row, msg, param);
	//}
	//if(ID == m_nSliderIndex){
	//	return OnSlider( ID, col, row, msg, param);
	//}

	return TRUE;
}

int CUGCtrl::OnCheckbox(long ID, int col, long row, long msg, long long param)
{
	CSetGridEditInProgressFlagInDataSource geipfids(this);// Destructor clears flag.

	CUGCell cell;
	GetCell(col, row, &cell);
	int nCellTypeIndex = cell.GetCellType();
	int nParam = cell.GetParam();

	if (msg == UGCT_CHECKBOXSET)
	{
		int nFieldType, nMaxLen;
		if (GetDataSource()->FieldEditOK(col, row, nFieldType, nMaxLen))
		{
			std::wstring str = (param) ? L"x" : L"";
			eGridAction action = eGridAction::REDRAW_ROW;
			BOOL fEditOK = GetDataSource()->ProcessEdit(col, row, str.c_str(), action, Hwnd());
			ProcessAfterEditAction(fEditOK, action, col, row);
			return TRUE;
		}
		return FALSE;
	}
	return TRUE;
}

int CUGCtrl::OnDropList(long ID, int col, long row, long msg, long long param)
{
	CUGCell cell;
	GetCell(col, row, &cell);
	int nCellTypeIndex = cell.GetCellType();
	int nParam = cell.GetParam();

	if (msg == UGCT_DROPLISTSTART)
	{
		std::vector<ListBoxExItem>* pList = (std::vector<ListBoxExItem>*)param;
		CRect cellRect;
		GetCellRect(col, row, &cellRect);
		BOOL result = GetDataSource()->GetListBoxData(col, row, *pList, cellRect, Hwnd());
		return result;
	}
	else if (msg == UGCT_DROPLISTSELECT)
	{
		std::wstring* pString = (std::wstring*)param;
		CRect rect;
		if (GetCellRect(col, row, &rect) == UG_ERROR)
			return FALSE;
		//m_GI->m_gridWnd->ClientToScreen(&rect);
		::ClientToScreen(m_GI->m_gridWnd, (LPPOINT)&rect);
		::ClientToScreen(m_GI->m_gridWnd, ((LPPOINT)&rect) + 1);
		//if (*pString == "{add new...}")
		//{
		//	BOOL fAddNew = pDSrc->AddNewLBitem(fldidx, rect, this, *pString);
		//	m_CUGGrid->SetFocus();
		//	if (!fAddNew)
		//		return FALSE;
		//}
		//else if (*pString == "{edit list...}")
		//{
		//	pDSrc->EditLBitems(fldidx, rect, this);
		//	m_CUGGrid->SetFocus();
		//	return FALSE;
		//}
		eGridAction action = eGridAction::REDRAW_ROW;
		//pDSrc->ModifySelectedItem(nDay, fldidx, row, pString, action, rect, this);
		BOOL fEditOK = GetDataSource()->ProcessEdit(col, row, pString->c_str(), action, Hwnd());
		ProcessAfterEditAction(fEditOK, action, col, row);
		return TRUE;
	}
	else if (msg == UGCT_DROPLIST_CHECKBOX_CHECKED)
	{
		ListBoxExItem* pItem = (ListBoxExItem*)param;
		GetDataSource()->ListBoxCheckBoxChecked(col, row, pItem);
	}
	return TRUE;
}

/***************************************************
OnEditStart
	This message is sent whenever the grid is ready to start editing a cell
Params:
	col, row - location of the cell that edit was requested over
	edit -	pointer to a pointer to the edit control, allows for swap of edit control
			if edit control is swapped permanently (for the whole grid) is it better
			to use 'SetNewEditClass' function.
Return:
	TRUE - to allow the edit to start
	FALSE - to prevent the edit from starting
****************************************************/
int CUGCtrl::OnEditStart(int col, long row,HWND edit){
	CXeGridDataSource* pDSrc = GetDataSource();
	CGridTblInfo& tblinfo = pDSrc->GetGridTblInfo();

	pDSrc->SetGridEditInProgressFlag();	// Flag will be cleared in OnEditFinish.

	CUGCell cell;
	int isCellOk = GetCell(col, row, &cell);
	if (isCellOk != UG_SUCCESS)
	{
		pDSrc->GetCellForGrid(col, row, &cell, m_gridsel);
	}
	if (cell.GetCellType() == UGCT_DROPLIST && !pDSrc->IsDropListEditType(col, row))
	{
		pDSrc->SetGridEditInProgressFlag(FALSE);
		return FALSE;
	}

	// Prevent user from editing a cell that is "drop-list",
	//   if DRPDNFLG_DROP_LIST flag is set for that field.
	//if( tblinfo.IsDrpDnList( fldidx ) )
	//{
	//	UINT uDrpDnFlags = tblinfo.GetDrpDnListFlags( fldidx );
	//	if( uDrpDnFlags & DRPDNFLG_DROP_LIST )
	//	{
	//		pDSrc->SetGridEditInProgressFlag( FALSE );
	//		return FALSE;
	//	}
	//}
	int flags = 0;
	if (pDSrc->FieldEditOK(col, row, flags, m_nMaxLen))
	{
		// Allow data source to bypass normal cell editing if needed.
		CRect rectCell;
		if (GetCellRect(col, row, &rectCell) == UG_ERROR)
			return FALSE;
		//m_GI->m_gridWnd->ClientToScreen(&rectCell);
		eGridAction action = eGridAction::NO_ACTION;
		if (pDSrc->EditField(col, row, rectCell, action, GetSafeHwnd()))
		{
			//m_GI->m_gridWnd->SetFocus();
			::SetFocus(m_GI->m_gridWnd);
			ProcessAfterEditAction(TRUE, action, col, row);
			RefreshGrid(TRUE);
			pDSrc->SetGridEditInProgressFlag(FALSE);	// Clear edit in progress flag.
			return FALSE;	// cell editing already done so return FALSE
		}

		// special processing when field type is color value
		if (flags & GGC_COLORBTN)
		{	// m_nMaxLen = the color value to edit
			CRect rScr(rectCell);
			m_CUGGrid->ClientToScreen(&rScr);
			bool isShowPickerFull = s_xeLastUsedUIsettings.GetBool_or_Val(L"GRID_CLRBTN_SHOW_FULL", false);
			ColorPickerMode pkr_mode = isShowPickerFull ? ColorPickerMode::Full : ColorPickerMode::Simple;
			CXeColorPickerDropDown pcr(m_xeUI, m_nMaxLen /* color */);
			pcr.ShowDropDown(m_CUGGrid->GetSafeHwnd(), rScr, pkr_mode);
			if (pcr.HasUserSelectedColor())
			{
				bool isShowPickerFull = pcr.GetColorPickerLayoutMode() == ColorPickerMode::Full;
				s_xeLastUsedUIsettings.Set(L"GRID_CLRBTN_SHOW_FULL", isShowPickerFull);
				COLORREF clr = pcr.GetSelectedColor();
				std::wstring str = std::to_wstring(clr);
				action = eGridAction::REDRAW_ROW;
				BOOL fEditOK = pDSrc->ProcessEdit(col, row, str.c_str(), action, Hwnd());
				ProcessAfterEditAction(fEditOK, action, col, row);
				RefreshGrid(TRUE);
			}
			pDSrc->SetGridEditInProgressFlag(FALSE);
			return FALSE;	// color editing already done so return FALSE
		}
		//else
		//{
		//	CRect rcCell;
		//	CSize sizeText;
		//	CString strText;
		//	bool isTextFit = true; // GetCellTextFit(col, row, rcCell, sizeText, cellFlags, strText);
		//	if (flags & GGC_MULTILINE_CELL && rcCell.Height() > (3 * m_xeUI->GetLogGridCharSize().cyMultiLineTextHeight) && !isTextFit)
		//	{	// show scrollbars
		//		(*edit)->ModifyStyle(0, WS_VSCROLL | WS_HSCROLL, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		//	}
		//	else
		//	{	// don't show scrollbars
		//		(*edit)->ModifyStyle(WS_VSCROLL | WS_HSCROLL, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		//	}
		//}
		return TRUE;
	}
	pDSrc->SetGridEditInProgressFlag(FALSE);
	return FALSE;
}

/***************************************************
OnEditVerify
	This notification is sent every time the user hits a key while in edit mode.
	It is mostly used to create custom behavior of the edit contol, because it is
	so eazy to allow or disallow keys hit.
Params:
	col, row	- location of the edit cell
	edit		-	pointer to the edit control
	vcKey		- virtual key code of the pressed key
Return:
	TRUE - to accept pressed key
	FALSE - to do not accept the key
****************************************************/
int CUGCtrl::OnEditVerify(int col,long row,HWND edit,UINT *vcKey){
	if (m_nMaxLen > 0 && ::GetWindowTextLengthW(m_GI->m_editCtrl) >= m_nMaxLen)
	{
		if (*vcKey == VK_DELETE || *vcKey == VK_BACK || *vcKey == 0x7F)
			return TRUE;
		return FALSE;
	}
	return TRUE;
}

/***************************************************
OnEditKeyDown and OnEditKeyUp
	are not ued by the Ultimate Grid, but are provided
	if there is a need for the CUGCtrl derived class
	to be notified about these events in the edit control.
	A custom edit control will need to be developed
	that sends these notifications.
****************************************************/
int CUGCtrl::OnEditKeyDown(int col,long row,HWND edit,UINT *vcKey){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*edit);
	UNREFERENCED_PARAMETER(*vcKey);
	return TRUE;
}

int CUGCtrl::OnEditKeyUp(int col,long row,HWND edit,UINT *vcKey){
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
int CUGCtrl::OnEditFinish(int col, long row,HWND edit,LPCTSTR string,BOOL cancelFlag)
{
	BOOL fRet;
	while (TRUE)
	{
		if (cancelFlag)
		{
			fRet = TRUE;
			break;
		}

		// check if user has actually made some changes
		LPCTSTR pStr = QuickGetText(col, row);
		std::wstring orgstring = pStr ? pStr : L"";
		if (orgstring == string)
		{
			fRet = TRUE;	// NO CHANGES HAVE BEEN MADE TO THE CELL !
			break;
		}

		eGridAction action = eGridAction::REDRAW_ROW;
		BOOL fEditOK = GetDataSource()->ProcessEdit(col, row, string, action, Hwnd());
		ProcessAfterEditAction(fEditOK, action, col, row);
		RefreshGrid(TRUE);
		fRet = fEditOK;
		break;
	}

	GetDataSource()->SetGridEditInProgressFlag(FALSE);	// Clear flag set by OnEditStart.
	return fRet;
}

void CUGCtrl::RefreshGrid(BOOL fHeaderAndTabOnly /*= FALSE*/)
{
	if (!fHeaderAndTabOnly)
		RedrawAll();

	if (GetDataSource()->HasGridHeader())
		RedrawCell(0, -2);
}

void CUGCtrl::ProcessAfterEditAction(BOOL fEditOK, eGridAction& action, int col, long row)
{
	if (action == eGridAction::REDRAW_ROW && fEditOK)
		RedrawRow(GetCurrentRow());
	else if (action == eGridAction::REDRAW_ALL)
		RedrawAll();
	else if (action == eGridAction::SET_GRID_FROM_DS)
	{
		SetGridFromDataSource(TRUE);
		m_fCancelNext_OnEditContinue = TRUE; // MUST cancel next OnEditContinue
	}
	else if (action == eGridAction::UPDATE_ROW_COUNT)
	{
		UpdateRowCount();
	}
	if (fEditOK && m_onEditFinishedCallback != nullptr)
	{
		m_onEditFinishedCallback(col, row);
	}
}

void CUGCtrl::UpdateRowCount()
{
	SetNumberRows(GetGridNumRows(), TRUE);
}

/***************************************************
OnEditContinue
	This notification is called when the user pressed 'tab' or 'enter' keys
	Here you have a chance to modify the destination cell
Params:
	oldcol, oldrow - edit cell that is loosing edit focus
	newcol, newrow - cell that the edit is going into, by changing their
						values you are able to change where to edit next
Return:
	TRUE - allow the edit to continue
	FALSE - to prevent the move, the edit will be stopped
****************************************************/
int CUGCtrl::OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow){
	if (m_fCancelNext_OnEditContinue)
	{
		m_fCancelNext_OnEditContinue = FALSE;
		return FALSE;
	}
	//GotoCell( *newcol, *newrow );
	return TRUE;
}

/***************************************************
OnMenuCommand
	This notification is called when the user has selected a menu item
	in the pop-up menu.
Params:
	col, row - the cell coordinates of where the menu originated from
	setcion - identify for which portion of the gird the menu is for.
			  possible sections:
					UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
					UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
	item - ID of the menu item selected
Return:
	<none>
****************************************************/
void CUGCtrl::OnMenuCommand(int col,long row,int section,int item){
	eGridAction action = eGridAction::NO_ACTION;
	stGridNotifyData nfData = _GetMenuNotifyData(EGRIDOP::eGridNfMenuCmd, col, row, section, item);
	switch (section)
	{
	case UG_HSCROLL:
	case UG_VSCROLL:
		break;
	case UG_CORNERBUTTON:
	case UG_SIDEHEADING:
	case UG_TOPHEADING:
	case UG_GRID:
		if (_NotifyParentView(nfData))	// Allow parent view a chance to process the menu command.
		{
			break;	// Menu command processed.
		}
		// Allow data source second crack at the menu command.
		if (GetDataSource()->OnGridMenuCommand(item, col, row, section,
			nfData.m_selection, Hwnd(), action))
		{	// Menu item was processed by the data source.
			if (action != eGridAction::NO_ACTION)
				ProcessAfterEditAction(TRUE, action, col, row);
			break;
		}

		switch (item)
		{
		case IDMG_SELECTALL:
			SetSelection();
			break;
		case IDMG_EDITCOPY:
			ClipboardOperation(UG_EDIT_COPY);
			break;
		case IDMG_FIND:
			OnEditFind();
			break;
			//case IDMG_PRINT:
			//	PrintGrid();
			//	break;
		case IDMG_RESETSIZE: {
			CGridTblInfo& tblInfo = GetDataSource()->GetGridTblInfo();
			tblInfo.ResetColWidths();
			std::wstring strRegSectionName{ m_strRegSectionName };
			tblInfo.SaveColWidthsUserSettings(strRegSectionName, L"GridCol");
			SetGridFromDataSource();
			} break;
		}
		break;
	}
}

stGridNotifyData CUGCtrl::_GetMenuNotifyData(EGRIDOP eOp, int col, long row, int section, int menuCmdId)
{
	stGridNotifyData nfData = _GetNotifyData(eOp, col, row, section);
	nfData.m_menuCmdId = menuCmdId;
	return nfData;
}

/***************************************************
OnMenuStart
	Is called when the pop up menu is about to be shown
Params:
	col, row	- the cell coordinates of where the menu originated from
	setcion		- identify for which portion of the gird the menu is for.
				possible sections:
					UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
					UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
Return:
	TRUE - to allow menu to show
	FALSE - to prevent the menu from poping up
****************************************************/
int CUGCtrl::OnMenuStart(int col,long row,int section){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);

	EmptyMenu();
	CXeMenu* pgridMenu = GetPopupMenu();

	switch (section)
	{
	case UG_HSCROLL:
	case UG_VSCROLL:
		return FALSE;
	case UG_TAB:	// note col = tab index or -1 when cursor was not over a tab
		return FALSE;
	case UG_CORNERBUTTON:
	case UG_SIDEHEADING:
	case UG_TOPHEADING:
	case UG_GRID:
		BOOL fEntireRowsSelected;
		int nNumCellsSelected = GetSelection(&fEntireRowsSelected);
		UINT uFlagsCut = (fEntireRowsSelected) ? 0 : MF_GRAYED;
		UINT uFlagsCopy = (nNumCellsSelected) ? 0 : MF_GRAYED;
		pgridMenu->AppendMenu(0, ListBoxExItem(IDMG_EDITCOPY, L"Copy\tCtrl-C", IsSeparator::no,
				IsChecked::no, nNumCellsSelected ? IsEnabled::yes: IsEnabled::no));
		pgridMenu->AppendMenu(0, ListBoxExItem(IDMG_SELECTALL, L"Select all\tCtrl-A", IsSeparator::yes));

		// Allow data source to add menu items or cancel menu popup.
		// Note - GetSelection(...) called already (see above).
		if (!GetDataSource()->OnGridMenuStart(pgridMenu, col, row, section, m_gridsel))
			return FALSE;

		pgridMenu->AppendMenu(0, ListBoxExItem(IDMG_RESETSIZE, L"Reset grid column-row size", IsSeparator::yes));

		break;
	}

	return TRUE;
}

int CUGCtrl::GetSelection(BOOL* pfEntireRowsSelected /*= 0*/)
{
	// return number of cells selected and selection in member variables when selection is valid
	// return 0 when invalid or no selection

	m_gridsel.m_nStartCol = -1;
	m_gridsel.m_nEndCol = -1;
	m_gridsel.m_nStartRow = -1;
	m_gridsel.m_nEndRow = -1;
	m_gridsel.m_fSelectionValid = FALSE;
	m_gridsel.m_nNumCellsSelected = 0;
	m_gridsel.m_nNumRowsSelected = 0;
	m_gridsel.m_fEntireRowsSelected = FALSE;
	m_gridsel.m_nGridCurRow = GetCurrentRow();
	m_gridsel.m_nGridCurCol = GetCurrentCol();
	m_gridsel.m_nGridCurColFldIdx = GetCurrentCol();

	if (EnumFirstBlock(&m_gridsel.m_nStartCol, &m_gridsel.m_nStartRow,
		&m_gridsel.m_nEndCol, &m_gridsel.m_nEndRow) == UG_ERROR)
		return 0;

	if (m_gridsel.m_nStartRow == (GetNumberRows() - 1))	// last row ? (is always emty)
	{	// selection is invalid !
		ClearSelections();
		return 0;
	}

	if (m_gridsel.m_nEndRow == (GetNumberRows() - 1))	// last row ?
	{	// last row can not be a part of the selection, modify the selection
		m_gridsel.m_nEndRow = (GetNumberRows() - 2);
		SetSelection(FALSE, m_gridsel.m_nStartCol, m_gridsel.m_nStartRow,
			m_gridsel.m_nEndCol, m_gridsel.m_nEndRow);
	}
	m_gridsel.m_fSelectionValid = TRUE;

	m_gridsel.m_nNumCellsSelected = ((m_gridsel.m_nEndCol - m_gridsel.m_nStartCol) + 1)
		* ((m_gridsel.m_nEndRow - m_gridsel.m_nStartRow) + 1);

	// check if selection is entire row(s)
	if (m_gridsel.m_nStartCol == 0
		&& m_gridsel.m_nEndCol == (GetNumberCols() - 1))
	{
		m_gridsel.m_fEntireRowsSelected = TRUE;
		if (pfEntireRowsSelected)
			*pfEntireRowsSelected = m_gridsel.m_fEntireRowsSelected;
	}

	m_gridsel.m_nNumRowsSelected = ((m_gridsel.m_nEndRow - m_gridsel.m_nStartRow) + 1);

	return m_gridsel.m_nNumCellsSelected;
}

void CUGCtrl::SetSelection(BOOL fSelectAll /*= TRUE*/, int nStartCol /*= -1*/,
	long nStartRow /*= -1*/, int nEndCol /*= -1*/, long nEndRow /*= -1*/)
{
	ClearSelections();
	int lastcol = (GetNumberCols() - 1), lastrow = (GetNumberRows() - 2);
	if (fSelectAll)
	{
		nStartCol = 0;
		nStartRow = 0;
		nEndCol = lastcol;
		nEndRow = lastrow;
	}
	if (nStartCol < 0)
		nStartCol = 0;
	if (nStartRow < 0)
		nStartRow = 0;
	if (nEndCol < 0)
		nEndCol = 0;
	if (nEndCol > lastcol)
		nEndCol = lastcol;
	if (nEndRow < 0)
		nEndRow = 0;
	if (nEndRow > lastrow)
		nEndRow = lastrow;
	if (nStartCol > nEndCol)
		nStartCol = nEndCol;
	if (nStartRow > nEndRow)
		nStartRow = nEndRow;

	SelectRange(nStartCol, nStartRow, nEndCol, nEndRow);
	_RedrawDirectly();

	m_gridsel.m_nStartCol = nStartCol;
	m_gridsel.m_nStartRow = nStartRow;
	m_gridsel.m_nEndCol = nEndCol;
	m_gridsel.m_nEndRow = nEndRow;
}

BOOL CUGCtrl::CopySelectionToClipboard(BOOL fTH /*= TRUE*/, BOOL fSH /*= FALSE*/)
{
	CXeWaitCursor wait(m_xeUI);
	//CWaitCursor wait;

	std::wstring strGrid;
	if (!CopySelectionToString(strGrid, fTH, fSH))
		return FALSE;

	if (!::OpenClipboard(Hwnd()))
		return FALSE;
	::EmptyClipboard();
	size_t mlen = strGrid.size() + 1;
	HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, mlen);
	wchar_t* psz = (wchar_t*)::GlobalLock(hGlobal);
	wcscpy_s(psz, mlen, strGrid.c_str());
	::GlobalUnlock(hGlobal);
	::SetClipboardData(CF_UNICODETEXT, hGlobal);

	::CloseClipboard();
	return TRUE;
}

BOOL CUGCtrl::CopySelectionToString(std::wstring& strSel, BOOL fTH /*= TRUE*/,
	BOOL fSH /*= FALSE*/)
{
	strSel = L"";
	int temprow, numcells;
	BOOL fEntireRowsSelected;
	if (!(numcells = GetSelection(&fEntireRowsSelected)))
		return FALSE;
	std::wstring strText;
	BOOL fDoingTH = FALSE;
	if (numcells == 1)	// is selection single cell ?
		fTH = FALSE;	// no top heading when only single cell selected
	if (fTH)	// include top heading ?
	{
		fDoingTH = TRUE;
		temprow = m_gridsel.m_nStartRow;
		m_gridsel.m_nStartRow = -1;
	}
	for (int row = m_gridsel.m_nStartRow; row <= m_gridsel.m_nEndRow; row++)
	{
		if (fSH)	// include side heading ?
		{
			QuickGetText(-1, row, &strText);
			strSel += strText;
		}
		for (int col = m_gridsel.m_nStartCol; col <= m_gridsel.m_nEndCol; col++)
		{
			QuickGetText(col, row, &strText);
			if (col < m_gridsel.m_nEndCol)
				strSel += strText + L"\t";
			else
				strSel += strText;
		}
		strSel += L"\r\n";
		if (fDoingTH)
		{
			fDoingTH = FALSE;
			m_gridsel.m_nStartRow = temprow;	// restore start row after doing top heading
			row = temprow - 1;
		}
	}
	return TRUE;
}

/***************************************************
HideCurrentCell
	function is used to 'hide' currently selected cell.
	As the result the grid will appear without the 
	focus rectangle on any of the cells.
Params
	<none>
Return
	<none>
****************************************************/
int CUGCtrl::HideCurrentCell()
{	
	ClearSelections();

	m_GI->m_currentCol = -1;
	m_GI->m_currentRow = -1;

	m_GI->m_dragCol = -1;
	m_GI->m_dragRow = -1;

	m_GI->m_lastCol = -1;
	m_GI->m_lastRow = -1;

	return UG_SUCCESS;
}

/***************************************************
OnScreenDCSetup
	Is called when each of the components are painted.
Params:
	dc		- pointer to the current CDC object
	section	- section of the grid painted.
				possible sections:
					UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
					UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
Return:
	<none>
****************************************************/
//void CUGCtrl::OnScreenDCSetup(CDC *dc,CDC *db_dc,int section){
//	UNREFERENCED_PARAMETER(*dc);
//	UNREFERENCED_PARAMETER(*db_dc);
//	UNREFERENCED_PARAMETER(section);
//}

/***************************************************
OnSortEvaluate
	Sent as a result of the 'SortBy' call, this is called for each cell
	comparison and allows for customization of the sorting routines.
	We provide following code as example of what could be done here,
	you migh have to modify it to give your application customized sorting.
Params:
	cell1, cell2	- pointers to cells that are compared
	flags			- identifies sort direction
Return:
	value less than zero to identify that the cell1 comes before cell2
	value equal to zero to identify that the cell1 and cell2 are equal
	value greater than zero to identify that the cell1 comes after cell2
****************************************************/
int CUGCtrl::OnSortEvaluate(CUGCell *cell1,CUGCell *cell2,int flags){

	// if one of the cells is NULL, do not compare its text
	if ( cell1 == NULL && cell2 == NULL )
		return 0;
	else if ( cell1 == NULL )
		return 1;
	else if ( cell2 == NULL )
		return -1;

	if(flags&UG_SORT_DESCENDING)
	{
		CUGCell *ptr = cell1;
		cell1 = cell2;
		cell2 = ptr;
	}

	// initialize variables for numeric check
	//double num1, num2;
	// compare the cells, with respect to the cell's datatype
	switch(cell1->GetDataType())
	{
		//case UGCELLDATA_NUMBER:
		//case UGCELLDATA_BOOL:
		//case UGCELLDATA_CURRENCY:
		//	num1 = cell1->GetNumber();
		//	num2 = cell2->GetNumber();
		//	if(num1 < num2)
		//		return -1;
		//	if(num1 > num2)
		//		return 1;
		//	return 0;
		//	break;
		case UGCELLDATA_STRING:
		default:
			// if datatype is not recognized, compare cell's text
			if(NULL == cell1->GetText())
				return 1;
			if(NULL == cell2->GetText())
				return -1;
	}

	return _tcscmp(cell1->GetText(),cell2->GetText());
}

/***************************************************
	Purpose
	Params
	Return
0:off  1:on  2:on-squared 3:on-cubed
****************************************************/
int CUGCtrl::SetBallisticMode(int mode){	

	if(mode <0 || mode >3)
		return UG_ERROR;

	m_GI->m_ballisticMode = mode;

	return UG_SUCCESS;

}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetBallisticDelay(int milisec){

	if(milisec < 0 || milisec > 32000)
		return UG_ERROR;

	m_GI->m_ballisticDelay = milisec;

	return UG_SUCCESS;
}

/***************************************************
SetKeyBallisticMode
	Purpose
		Sets the ballistic mode for scrolling with
		the keyboard. The mode value is the number
		of repeats the key must make before an
		increment in speed is made. Every time 
		a multiple of the repeat number is reached
		the speed is doubled.
	Params
		mode	0:off  n:number of key repeats
				(0 - 1000 are valid)
	Return
		UG_ERROR - invalid mode number
		UG_SUCCESS - success
****************************************************/
int CUGCtrl::SetBallisticKeyMode(int mode){

	if(mode <0 || mode > 1000)
		return UG_ERROR;

	m_GI->m_ballisticKeyMode = mode;

	return UG_SUCCESS;
}

/***************************************************
SetKeyBallisticDelay
	Purpose
		Sets the initial delay for ballistic keys
		(if ballistic keys are on)
		the delay is only used when the key repeat
		count is lower than the mode set
	Params
		milisec - delay time
	Params
		UG_ERROR - invalid time
		UG_SUCCESS - success
****************************************************/
int CUGCtrl::SetBallisticKeyDelay(int milisec){

	if(milisec < 0)
		return UG_ERROR;

	m_GI->m_ballisticKeyDelay = milisec;

	return UG_SUCCESS;
}

/***************************************************
SetDoubleBufferMode
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::SetDoubleBufferMode(int mode)
//{
//	m_CUGGrid->SetDoubleBufferMode(mode);
//
//	return UG_SUCCESS;
//}

/***************************************************
SetGridLayout
	Purpose
		The SetGridLayout function is used to toggle LTR
		and RTL layout modes.  With this function you
		can alternate grid's layout between left-to-right
		and right-to-left.
	Params
		layoutMode - can be:
			0: Left-to-Right (default), or
			1: Right-to-Left
	Return
		UG_SUCCESS - upon success, or
		UG_ERROR - when layout mode cannot be changed
****************************************************/
//int CUGCtrl::SetGridLayout( int layoutMode )
//{
//	// Determine if the layout specified is same
//	// as current layout
//	LONG lExStyles = GetWindowLong( GetSafeHwnd(), GWL_EXSTYLE );
//
//	if ( lExStyles&WS_EX_LAYOUTRTL && layoutMode == 1 )
//		return UG_ERROR;
//	else if (!( lExStyles&WS_EX_LAYOUTRTL ) && layoutMode == 0 )
//		return UG_ERROR;
//
//	// change layout for all of grid's child windows
//	ToggleLayout( this );
//	ToggleLayout( m_CUGGrid );
//	ToggleLayout( m_CUGTopHdg );
//	ToggleLayout( m_CUGSideHdg );
//	ToggleLayout( m_CUGCnrBtn );
//	ToggleLayout( m_CUGTab );
//	//ToggleLayout( m_CUGVScroll );
//	//ToggleLayout( m_CUGHScroll );
//#ifdef UG_ENABLE_SCROLLHINTS
//	ToggleLayout( m_CUGHint );
//#endif
//	ToggleLayout( &m_defEditCtrl );
//	ToggleLayout( &m_defMaskedEditCtrl );
//
//	// force the tab control to recalculate position and size
//	// of the scroll buttons
//	m_CUGTab->SendMessage( WM_SIZE );
//
//	// force the grid to repaint
//	Invalidate();
//	UpdateWindow();
//
//	return UG_SUCCESS;
//}

/***************************************************
ToggleLayout
	is a helper function to the SetGridLayout
****************************************************/
//void CUGCtrl::ToggleLayout( CWnd *pWnd )
//{
//	if ( pWnd != NULL )
//	{
//		LONG lExStyles = GetWindowLong( pWnd->GetSafeHwnd(), GWL_EXSTYLE );
//
//		// The following lines update the application layout to 
//		// be right-to-left or left-to-right, as appropriate.
//		lExStyles ^= WS_EX_LAYOUTRTL; // Toggle layout.
//
//		SetWindowLong( pWnd->GetSafeHwnd(), GWL_EXSTYLE, lExStyles );
//
//		pWnd->Invalidate();
//	}
//}

/***************************************************
LockColumns
	Purpose
		Sets the number of columns to lock in place
		on the left side of the grid (so they do not
		scroll). 
		The following variable are set:
			m_GI->m_numLockCols
			m_GI->m_lockColWidth
	Params
		numCols - the number of columns to lock
	Return
		UG_SUCCESS	- success
		UG_ERROR	- numCols was out of range
****************************************************/
int CUGCtrl::LockColumns(int numCols){
	
	//do range checking
	if(numCols <0 || numCols > m_GI->m_numberCols)
		return UG_ERROR;

	//if the new value is the same as the old then just return
	if(numCols == m_GI->m_numLockCols)
		return UG_SUCCESS;

	//set the number of locked columns
	m_GI->m_numLockCols = numCols;

	//set the lock width
	m_GI->m_lockColWidth = 0;
	for(int col = 0;col < numCols;col++){
		m_GI->m_lockColWidth += GetColWidth(col);
	}

	m_GI->m_leftCol = numCols;
	m_GI->m_lastLeftCol = numCols;

	CalcLeftCol();
	
	return UG_SUCCESS;
}

/***************************************************
LockRows
	Purpose
		Sets the number of rows to lock in place
		on the top of the grid (so they do not
		scroll). 
		The following variable are set:
			m_GI->m_numLockRows
			m_GI->m_lockRowHeight
	Params
		numRows - the number of rows to lock
	Return
		UG_SUCCESS	- success
		UG_ERROR	- numRows was out of range
****************************************************/
int CUGCtrl::LockRows(int numRows){
	
	//do range checking
	if(numRows <0 || numRows > m_GI->m_numberRows)
		return UG_ERROR;

	//if the new value is the same as the old then just return
	if(numRows == m_GI->m_numLockRows)
		return UG_SUCCESS;

	//set the number of locked rows
	m_GI->m_numLockRows = numRows;

	//set the lock height
	m_GI->m_lockRowHeight = 0;
	for(int row = 0;row < numRows;row++){
		m_GI->m_lockRowHeight += GetRowHeight(row);
	}

	m_GI->m_topRow = numRows;
	m_GI->m_lastTopRow = numRows;

	CalcTopRow();

	return UG_SUCCESS;
}

/***************************************************
OnAdjustComponentSizes
	Called when the grid components are being created,
	sized, and arranged.  This notification provides
	us with ability to further customize the way
	the grid will be presented to the user.
Params:
	grid, topHdg, sideHdg, cnrBtn, vScroll, hScroll, 
	tabs	- sizes and location of each of the grid's
			  windowed components.
Return:
	<none>
****************************************************/
void CUGCtrl::OnAdjustComponentSizes(RECT *grid,RECT *topHdg,RECT *sideHdg,
		RECT *cnrBtn,RECT *vScroll,RECT *hScroll,RECT *tabs){
	UNREFERENCED_PARAMETER(*grid);
	UNREFERENCED_PARAMETER(*topHdg);
	UNREFERENCED_PARAMETER(*sideHdg);
	UNREFERENCED_PARAMETER(*cnrBtn);
	UNREFERENCED_PARAMETER(*vScroll);
	UNREFERENCED_PARAMETER(*hScroll);
	UNREFERENCED_PARAMETER(*tabs);
} 

/***************************************************
OnDrawFocusRect
	Called when the focus rect needs to be painted.
Params:
	dc		- pointer to the current CDC object
	rect	- rect of the cell that requires the focus rect
Return:
	<none>
****************************************************/
//void CUGCtrl::OnDrawFocusRect(CDC *dc,RECT *rect){
//
//	int col=GetCurrentCol();
//	int row=GetCurrentRow();
//	CUGCellType* pCellType=GetCellType(col,row);
//	if(pCellType!=NULL)
//	{
//		if(pCellType->OnDrawFocusRect(dc,rect,col,row))
//			return;
//	}
//
//	rect->bottom --;
//	rect->right --;
//	dc->DrawFocusRect(rect);
//}

/***************************************************
OnGetDefBackColor
	Sent when the area behind the grid needs to be painted.
Params:
	section - Id of the grid section that requested this color
			  possible sections:
					UG_TOPHEADING, UG_SIDEHEADING, UG_GRID
Return:
	RGB value representing the color of choice
****************************************************/
//COLORREF CUGCtrl::OnGetDefBackColor(int section){
////#pragma NOTE("SKMOD to support custom colors")
//	if(section == UG_GRID)
//		return m_xeUI->GetColor(CID::GrdPaperBg);
//	else
//		return m_xeUI->GetColor(CID::GrdHdrFillBg);
//}

/***************************************************
DrawExcelFocusRect
	function is called when Excel style focus rectangle
	was selected.
Params:
	dc		- pointer to the current CDC object
	rect	- rect of the cell that requires the focus rect
Return:
	<none>
****************************************************/
//void CUGCtrl::DrawExcelFocusRect(CDC *dc,RECT *rect){
//	
//	BOOL innerRectOnly = FALSE;
//	BOOL hideTop = FALSE;
//	BOOL hideLeft = FALSE;
//	CPen pen,wPen;
//	int dcID = dc->SaveDC();
//
//	rect->right--;
//	rect->bottom--;
//
//	//find the rect to draw - may be a range of cells
//	//if multi-select is on
//	if(m_GI->m_multiSelectFlag&3){
//		if(m_GI->m_multiSelect->GetNumberBlocks() == 1){
//			int startCol,endCol;
//			long startRow,endRow;
//			m_GI->m_multiSelect->GetCurrentBlock(&startCol,&startRow,&endCol,&endRow);
//
//			if(startCol != endCol || startRow != endRow){
//				GetRangeRect(startCol,startRow,endCol,endRow,rect);
//			}
//			if(startCol >= m_GI->m_numLockCols && startCol < m_GI->m_leftCol){
//				//hide the left
//				hideLeft = TRUE;
//			}
//			if(startRow >= m_GI->m_numLockRows && startRow < m_GI->m_topRow){
//				//hide the top
//				hideTop = TRUE;
//			}
//			if(rect->bottom <= rect->top)
//				GetRangeRect(startCol,startRow,endCol,endRow,rect);
//			if(rect->right <= rect->left)
//				GetRangeRect(startCol,startRow,endCol,endRow,rect);
//			
//			//add draw hints for the grid so it will clear up the
//			//focus rect next time
//			//m_CUGGrid->m_drawHint.AddHint(startCol-1,startRow-1,endCol+1,endRow+1);
//		}
//		else if(m_GI->m_multiSelect->GetNumberBlocks() > 1){
//			innerRectOnly = TRUE;
//		}
//		else{
//			//add draw hints for the grid so it will clear up the
//			//focus rect next time
//			//m_CUGGrid->m_drawHint.AddHint(m_GI->m_currentCol-1,m_GI->m_currentRow-1,
//			//	m_GI->m_currentCol+1,m_GI->m_currentRow+1);
//		}
//	}
//	else{
//		//add draw hints for the grid so it will clear up the
//		//focus rect next time
//		//m_CUGGrid->m_drawHint.AddHint(m_GI->m_currentCol-1,m_GI->m_currentRow-1,
//		//	m_GI->m_currentCol+1,m_GI->m_currentRow+1);
//	}
//
//	int top = rect->top;
//	int left = rect->left;
//
//
//	//inner black rect
//	dc->SelectObject(GetStockObject(BLACK_PEN));
//	if(rect->top == 0)
//		rect->top ++;
//	if(rect->left == 0)
//		rect->left ++;
//	if(!hideTop){
//		dc->MoveTo(rect->left,rect->top);
//		dc->LineTo(rect->right-1,rect->top);
//	}
//	else{
//		dc->MoveTo(rect->right-1,rect->top);
//	}
//	dc->LineTo(rect->right-1,rect->bottom-1);
//	dc->LineTo(rect->left,rect->bottom-1);
//	if(!hideLeft){
//		dc->LineTo(rect->left,rect->top-1);
//	}
//	rect->top = top;
//	rect->left = left;
//
//
//	//inner white rect
//	dc->SelectObject(GetStockObject(WHITE_PEN));
//	if(rect->top == 0)
//		rect->top ++;
//	if(rect->left == 0)
//		rect->left ++;
//	if(!hideTop){
//		dc->MoveTo(rect->left+1,rect->top+1);
//		dc->LineTo(rect->right-2,rect->top+1);
//	}
//	else{
//		dc->MoveTo(rect->right-2,rect->top+1);
//	}
//	dc->LineTo(rect->right-2,rect->bottom-2);
//	dc->LineTo(rect->left+1,rect->bottom-2);
//	if(!hideLeft){
//		dc->LineTo(rect->left+1,rect->top);
//	}
//	rect->top = top;
//	rect->left = left;
//
//
//
//	//draw the rest if innerRectOnly is false
//	if(!innerRectOnly){
//		
//		dc->SelectObject(GetStockObject(BLACK_PEN));
//
//
//		//outer rect
//		if(rect->top == 0)
//			rect->top +=2;
//		if(rect->left == 0)
//			rect->left +=2;
//		if(hideLeft)
//			rect->left++;
//		if(!hideTop){
//			dc->MoveTo(rect->left-2,rect->top-2);
//			dc->LineTo(rect->right+1,rect->top-2);			
//		}
//		else{
//			rect->top++;
//			dc->MoveTo(rect->right+1,rect->top-2);
//		}
//		dc->LineTo(rect->right+1,rect->bottom+1);
//		dc->LineTo(rect->left-2,rect->bottom+1);
//		if(!hideLeft){
//			dc->LineTo(rect->left-2,rect->top -3);
//		}
//
//		rect->top = top;
//		rect->left = left;
//
//
//		//bottom right square
//		dc->MoveTo(rect->right-2,rect->bottom-2);
//		dc->LineTo(rect->right,rect->bottom-2);
//		dc->MoveTo(rect->right+2,rect->bottom-2);
//		dc->LineTo(rect->right+2,rect->bottom+1);
//		dc->MoveTo(rect->right+2,rect->bottom+1);
//		dc->LineTo(rect->right+2,rect->bottom+2);
//		dc->LineTo(rect->right,rect->bottom+2);
//		dc->MoveTo(rect->right-2,rect->bottom+2);
//		dc->LineTo(rect->right,rect->bottom+2);
//
//		wPen.CreatePen(PS_SOLID,1,RGB(255,255,255));
//		dc->SelectObject(wPen);
//		dc->MoveTo(rect->right+3,rect->bottom-3);
//		dc->LineTo(rect->right-3,rect->bottom-3);
//		dc->LineTo(rect->right-3,rect->bottom+3);
//	
//
//		pen.CreatePen(PS_SOLID,1,RGB(120,120,120));
//		dc->SelectObject(pen);
//
//
//		//dark gray middle rect
//		if(rect->top >0 && !hideTop){
//			dc->MoveTo(rect->left-2,rect->top-1);
//			dc->LineTo(rect->right+2,rect->top-1);
//		}
//		else
//			rect->top +=2;
//
//		dc->MoveTo(rect->right,rect->top-1);
//		dc->LineTo(rect->right,rect->bottom+3);
//		if(rect->left >0 && !hideLeft){
//			dc->MoveTo(rect->left-1,rect->bottom+1);
//			dc->LineTo(rect->left-1,rect->top-2);
//		}
//		else
//			rect->left +=3;
//		dc->MoveTo(rect->right+2,rect->bottom);
//		dc->LineTo(rect->left-3,rect->bottom);
//
//	}
//
//	rect->top = top;
//	rect->left = left;
//
//
//	dc->RestoreDC(dcID);
//
//	pen.DeleteObject();
//	wPen.DeleteObject();
//
//}

/***************************************************
StartMenu
	starts the pop-up menu if enabled

	col - col to start the menu for
	row - row to start the menu for
	point - point in screen co-ords
	seciton - UG_GRID,UG_TOPHEADING,UG_SIDEHEADING
			  UG_CORNERBUTTON,UG_TAB,UG_VSCROLL,UG_HSCROLL
Return
	UG_SUCCESS	- success
	UG_ERROR	- menu not enabled
	2			- OnStartMenu did not allow the menu to appear
	3			- menu failed
****************************************************/
int CUGCtrl::StartMenu(int col,long row,POINT *point,int section){

	if(!m_GI->m_enablePopupMenu)
		return UG_ERROR;

	GetJoinStartCell(&col,&row);

	if(OnMenuStart(col,row,section) == FALSE)
		return 2;

	m_menuCol = col;
	m_menuRow = row;
	m_menuSection = section;

	m_menu->ShowMenu(Hwnd(), CPoint(*point), 0);

	// SK MOD - needed NF when menu dismissed to dly paint the grid.
	//OnMenuEnd( col, row, section );

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//BOOL CUGCtrl::OnCommand(WPARAM wParam, LPARAM lParam) 
LRESULT CUGCtrl::_OnWmCommand(WORD wSource, WORD wID, HWND sender)
{
	//int ID = LOWORD(wParam);

	//send a notification for menu items
	//if(ID >= 1000){
	if(wID >= 1000){
		OnMenuCommand(m_menuCol,m_menuRow,m_menuSection,wID);
	}

	//return CWnd::OnCommand(wParam, lParam);
	return 0;
}

/***************************************************
SetMargin
	Purpose
		Sets the left/right margins that cell types 
		will use when drawing cells.
	Params
		pixels - pixels wide to set the margin to
	Return
		UG_SUCCESS(0)	- success
		UG_ERROR(1)		- error 
****************************************************/
int CUGCtrl::SetMargin(int pixels){
	
	if(pixels < 0)
		return UG_ERROR;

	m_GI->m_margin = pixels;

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::AddFont(LPCTSTR fontName,int height,int weight){
//
//	return AddFont(height,0,0,0,weight,0,0,0,0,0,0,0,0,fontName);
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::AddFont(int height,int width,int escapement,int orientation, 
//			int weight,BYTE italic,BYTE underLine,BYTE strikeOut, 
//			BYTE charSet,BYTE outputPrecision,BYTE clipPrecision, 
//			BYTE quality,BYTE pitchAndFamily,LPCTSTR fontName){
//
//	LOGFONT lf;	
//
//	//create a log font structure from the params
//	lf.lfHeight			= height;
//	lf.lfWidth			= width;
//	lf.lfEscapement		= escapement;
//	lf.lfOrientation	= orientation;
//	lf.lfWeight			= weight;
//	lf.lfItalic			= italic;
//	lf.lfUnderline		= underLine;
//	lf.lfStrikeOut		= strikeOut;
//	lf.lfCharSet		= charSet;
//	lf.lfOutPrecision	= outputPrecision;
//	lf.lfClipPrecision	= clipPrecision;
//	lf.lfQuality		= quality;
//	lf.lfPitchAndFamily	= pitchAndFamily;
//#pragma warning(disable:4996)
//	if(_tcslen(fontName) < LF_FACESIZE)
//		_tcscpy(lf.lfFaceName,fontName);
//	else
//		_tcscpy(lf.lfFaceName,_T(""));
//#pragma warning(default:4996)
//
//	return AddFontIndirect( lf );
//}

/***************************************************
AddFontIndirect
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::AddFontIndirect( LOGFONT lgFont )
//{
//	int lfBaseSize = sizeof(LOGFONT) - LF_FACESIZE;
//	//int count,loop;
//	//LOGFONT lf2;
//
//	// ! ! ! this section disabled so sheets in grid can have separate fonts
//	////check to see if a font with the same attribs already exists
//	////else add the font
//	//count = m_fontList->GetCount();
//	//for( loop = 0; loop < count; loop++ )
//	//{
//	//	//get the LOGFONT for an existing font
//	//	CFont * font = (CFont*)m_fontList->GetPointer( loop );
//	//	font->GetLogFont( &lf2 );
//
//	//	//compare the LOGFONTs
//	//	if( memcmp( &lgFont, &lf2, lfBaseSize ) == 0 )
//	//	{
//	//		if( lstrcmp( lgFont.lfFaceName, lf2.lfFaceName ) == 0 )
//	//		{
//	//			//if the LOGFONTs are the same then just
//	//			//increment the use count and return a pointer 
//	//			//to the existing font
//	//			m_fontList->UpdateParam( loop, m_fontList->GetParam(loop) + 1 );
//	//			return loop;
//	//		}
//	//	}
//	//}
//
//	//if a matching font was not found above then create it
//	CFont * font = new CFont();
//	font->CreateFontIndirect( &lgFont );
//	return m_fontList->AddPointer( font );
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::RemoveFont(int index){
//
//	//decrement the use count
//	int count = m_fontList->GetParam(index) - 1;
//	m_fontList->UpdateParam(index,count);	
//
//	//if the use count is 0 then delete it
//	if(count <=0){
//		CFont* pFont = (CFont *) (m_fontList->GetPointer(index));
//		if (NULL != pFont) delete pFont;
//		return m_fontList->DeletePointer(index);
//	}
//
//	return UG_SUCCESS;
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::ClearAllFonts(){
//
//	m_fontList->EmptyList();
//
//	return UG_SUCCESS;
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//CFont * CUGCtrl::GetFont(int index){
//	
//	return (CFont *)m_fontList->GetPointer(index);
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::SetDefFont(CFont *font){
//
//	if(font == NULL)
//		return UG_ERROR;
//
//	//m_GI->m_defFont = font;
//
//	return UG_SUCCESS;
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::SetDefFont(int index){
//
//	CFont * font = (CFont *)m_fontList->GetPointer(index);
//	if(font == NULL)
//		return UG_ERROR;
//
//	m_GI->m_defFont =  font;
//
//	return UG_SUCCESS;
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::AddBitmap( UINT resourceID,LPCTSTR resourceName ){
//
//	CBitmap * bitmap = new CBitmap();
//	if(resourceName == NULL)
//		bitmap->LoadBitmap(resourceID);
//	else
//		bitmap->LoadBitmap(resourceName);
//
//	return m_bitmapList->AddPointer(bitmap);
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::AddBitmap( LPCTSTR fileName){
//
//	BITMAPFILEHEADER	m_bmfh;
//	BITMAPINFOHEADER	m_bmih;
//	BITMAPINFO *		m_lpbmi;
//	void *				m_lpvBits;
//
//	FILE * fptr;
//
//	// open the file
//#pragma warning(disable:4996)
//	fptr = _tfopen(fileName,_T("rb"));
//#pragma warning(default:4996)
//
//	//return if the open file failed
//	if(fptr==NULL){
//		return -1;
//	}
//
//	//Retrieve the BITMAPFILEHEADER structure
//	fread(&m_bmfh,1,sizeof(BITMAPFILEHEADER),fptr);
//
//	//Retrieve the BITMAPINFOHEADER structure
//	fread(&m_bmih,1,sizeof(BITMAPINFOHEADER),fptr);
//
//#pragma warning(disable:4334)
//	//Allocate memory for the BITMAPINFO structure
//	if(m_bmih.biBitCount <16)
//		m_lpbmi = (BITMAPINFO *)new char[sizeof(BITMAPINFOHEADER)+((1<<m_bmih.biBitCount) * sizeof(RGBQUAD))];
//	else
//		m_lpbmi = (BITMAPINFO *)new char[sizeof(BITMAPINFOHEADER)+ sizeof(RGBQUAD)];
//
//	//Load BITMAPINFOHEADER into the BITMAPINFO structure
//	memcpy(m_lpbmi,&m_bmih,sizeof(BITMAPINFO));
//
//	//Retrieve the color table
//	if(m_bmih.biBitCount <16)
//		fread(m_lpbmi->bmiColors,1,((1<<m_bmih.biBitCount) * sizeof(RGBQUAD)),fptr);
//#pragma warning(default:4334)
//
//	//Allocate memory for the required number of bytes
//	m_lpvBits = new char[m_bmfh.bfSize - m_bmfh.bfOffBits];
//
//	//Retrieve the bitmap data
//	fread(m_lpvBits,1,(m_bmfh.bfSize - m_bmfh.bfOffBits),fptr);
//
//	//close the file
//	fclose(fptr);
//
//
//	//check to see if a DIB was already loaded
//	//if it was then delete the old information
//
//	CBitmap* bitmap = new CBitmap();
//	
//	CDC *dc = m_CUGGrid->GetDC();
//	bitmap->Attach(CreateDIBitmap(dc->m_hDC,&m_bmih,CBM_INIT,m_lpvBits,m_lpbmi, DIB_RGB_COLORS));
//	m_CUGGrid->ReleaseDC(dc);
//
//	delete[] m_lpbmi;
//	delete[] m_lpvBits;
//
//	return m_bitmapList->AddPointer(bitmap);
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::RemoveBitmap(int index){
//
//	CBitmap* bitmap = (CBitmap*) m_bitmapList->GetPointer(index);
//	if (NULL != bitmap) delete bitmap;
//
//	return m_bitmapList->DeletePointer(index);
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::ClearAllBitmaps(){
//
//	m_bitmapList->EmptyList();
//
//	return UG_SUCCESS;
//}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
//CBitmap* CUGCtrl::GetBitmap(int index){
//
//	return (CBitmap *)m_bitmapList->GetPointer(index);
//
//}

/***************************************************
OnColSwapStart
	Called to inform the grid that the col swap was started on given col.
Params:
	col - identifies the column
Return:
	TRUE - to allow for the swap to take place
	FALSE - to disallow the swap
****************************************************/
BOOL CUGCtrl::OnColSwapStart(int col){

	UNREFERENCED_PARAMETER(col);
	return TRUE;
}

/***************************************************
OnCanColSwap
	Called when col swap is about to take place, here you can 'lock' certain
	cols in place so that are not swapable.
Params:
	fromCol - where the col orriginated from
	toCol	- where the col will be located if the swap is allowed
Return:
	TRUE - to allow for the swap to take place
	FALSE - to disallow the swap
****************************************************/
BOOL CUGCtrl::OnCanColSwap(int fromCol,int toCol){
	
	UNREFERENCED_PARAMETER(fromCol);
	UNREFERENCED_PARAMETER(toCol);
	return TRUE;
}

/***************************************************
OnColSwapped
	Called just after column-swap operation was completed.
Params:
	fromCol - where the col orriginated from
	toCol	- where the col will be located if the swap is allowed
Return:
	<none>
****************************************************/
void CUGCtrl::OnColSwapped(int fromCol,int toCol){
	UNREFERENCED_PARAMETER(fromCol);
	UNREFERENCED_PARAMETER(toCol);
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::MoveColPosition(int fromCol,int toCol,BOOL insertBefore){

	//check the instert before flag		
	if(insertBefore == FALSE)
		toCol++;

	//range checking
	if(fromCol <0 || fromCol >= m_GI->m_numberCols)
		return UG_ERROR;	
	if(toCol <0 || toCol > m_GI->m_numberCols)
		return UG_ERROR;

	//check to see if the col position needs to be changed
	if(fromCol == toCol || fromCol+1 == toCol)
		return UG_SUCCESS;

	if(fromCol < toCol)
		toCol --;

	//store the column info for the fromCol - since it will be over-written
	int fTransCol				= m_GI->m_colInfo[fromCol].colTranslation;
	int fWidth					= m_GI->m_colInfo[fromCol].width;
	CUGDataSource * fDataSource = m_GI->m_colInfo[fromCol].dataSource;
	CUGCell	* fColDefault		= m_GI->m_colInfo[fromCol].colDefault;

	//shift the column information over to the new position
	if(fromCol < toCol){
		//shift everything bwtween the to and from cols down one column
		for(int loop = fromCol;loop < toCol;loop++){
			m_GI->m_colInfo[loop].colTranslation = m_GI->m_colInfo[loop+1].colTranslation;
			m_GI->m_colInfo[loop].width			= m_GI->m_colInfo[loop+1].width;
			m_GI->m_colInfo[loop].dataSource	= m_GI->m_colInfo[loop+1].dataSource;
			m_GI->m_colInfo[loop].colDefault	= m_GI->m_colInfo[loop+1].colDefault;
		}
	}
	else{
		//shift everything bwtween the to and from cols up one column
		for(int loop = fromCol;loop > toCol;loop--){
			m_GI->m_colInfo[loop].colTranslation = m_GI->m_colInfo[loop-1].colTranslation;
			m_GI->m_colInfo[loop].width			= m_GI->m_colInfo[loop-1].width;
			m_GI->m_colInfo[loop].dataSource	= m_GI->m_colInfo[loop-1].dataSource;
			m_GI->m_colInfo[loop].colDefault	= m_GI->m_colInfo[loop-1].colDefault;
		}
	}

	//copy the prev. saved col informtion into the to column
	m_GI->m_colInfo[toCol].colTranslation = fTransCol;
	m_GI->m_colInfo[toCol].width		= fWidth;
	m_GI->m_colInfo[toCol].dataSource	= fDataSource;
	m_GI->m_colInfo[toCol].colDefault	= fColDefault;

	SetLockColWidth();

	return UG_SUCCESS;
}



//#ifdef UG_ENABLE_PRINTING
//
//
///***************************************************
//	Purpose
//	Params
//	Return
//this function returns the number of pages needed
//to print the entire range, using the current
//print scale and options
//****************************************************/
//int CUGCtrl::PrintInit(CDC * pDC, CPrintDialog* pPD, int startCol,long startRow,
//			  int endCol,long endRow){
//
//	return m_CUGPrint->PrintInit(pDC,pPD,startCol,startRow,endCol,endRow);
//}
//
///***************************************************
//	Purpose
//	Params
//	Return
//****************************************************/
//int CUGCtrl::PrintPage(CDC * pDC, int pageNum){
//
//	return m_CUGPrint->PrintPage(pDC,pageNum);
//}
//
///***************************************************
//PrintSetMargin
//	Sets the margin in mm (milli-meters)
//Params
//	whichMargin	- one of the following
//					UG_PRINT_LEFTMARGIN
//					UG_PRINT_RIGHTMARGIN
//					UG_PRINT_TOPMARGIN
//					UG_PRINT_BOTTOMMARGIN
//	size - size of the margin in mm
//return
//	UG_SUCCESS - success
//	UG_ERROR - error
//****************************************************/
//int CUGCtrl::PrintSetMargin(int whichMargin,int size){
//	return m_CUGPrint->PrintSetMargin(whichMargin,size);
//}
//
///***************************************************
//	Purpose
//	Params
//	Return
//****************************************************/
//int CUGCtrl::PrintSetScale(int scale){
//	
//	return m_CUGPrint->PrintSetScale(scale);
//}
//
///***************************************************
//PrintSetOption
//	Sets the up the printing options.
//	
//	 Note:When setting margins the param value is 
//	 in mm (milli-meters)
//Params
//	option	- one of the following
//				UG_PRINT_LEFTMARGIN
//				UG_PRINT_RIGHTMARGIN
//				UG_PRINT_TOPMARGIN
//				UG_PRINT_BOTTOMMARGIN
//				UG_PRINT_FRAME
//				UG_PRINT_TOPHEADING
//				UG_PRINT_SIDEHEADING
//	param - if setting a margin:  size in mm
//			if setting the frame: TRUE or FALSE
//			if setting a heading: TRUE or FALSE
//return
//	UG_SUCCESS - success
//	UG_ERROR - error
//****************************************************/
//int CUGCtrl::PrintSetOption(int option,long param){
//	
//	return m_CUGPrint->PrintSetOption(option,param);
//}
//
///***************************************************
//	Purpose
//	Params
//	Return
//****************************************************/
//int CUGCtrl::PrintGetOption(int option,long *param){
//	
//	return m_CUGPrint->PrintGetOption(option,param);
//}
//
//#endif

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetNewTopHeadingClass(CUGTopHdg * topHeading){

	if(m_CUGTopHdg != NULL)
		delete m_CUGTopHdg;

	m_CUGTopHdg = topHeading;
	//m_CUGTopHdg->m_ctrl = this;
	m_CUGTopHdg->m_GI	= m_GI;

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetNewSideHeadingClass(CUGSideHdg * sideHeading){

	if(m_CUGSideHdg!= NULL)
		delete m_CUGSideHdg;

	m_CUGSideHdg = sideHeading;
	//m_CUGSideHdg->m_ctrl = this;
	m_CUGSideHdg->m_GI	= m_GI;

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetNewGridClass(CUGGrid * grid){

	if(m_CUGGrid != NULL)
		delete m_CUGGrid;


	m_CUGGrid = grid;
	//m_CUGGrid->m_ctrl = this;
	m_CUGGrid->m_GI	= m_GI;

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetNewMultiSelectClass(CUGMultiSelect * multiSelect){

	if(m_GI->m_multiSelect != NULL)
		delete m_GI->m_multiSelect;

	m_GI->m_multiSelect = multiSelect;
	//m_GI->m_multiSelect->m_ctrl = this;
	m_GI->m_multiSelect->m_GI = m_GI;

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::SetNewTabClass(CUGTab * tab){
//
//	if(m_CUGTab != NULL)
//		delete m_CUGTab;
//
//	m_CUGTab = tab;
//	//m_CUGTab->m_ctrl = this;
//	m_CUGTab->m_GI	= m_GI;
//
//	return UG_SUCCESS;
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetNewVScrollClass(CUGVScroll * scroll){

	if(m_CUGVScroll != NULL)
		delete m_CUGVScroll;

	m_CUGVScroll = scroll;
	//m_CUGVScroll->m_ctrl = this;
	m_CUGVScroll->m_GI	= m_GI;

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::SetNewHScrollClass(CUGHScroll * scroll){

	if(m_CUGHScroll != NULL)
		delete m_CUGHScroll;

	m_CUGHScroll = scroll;
	//m_CUGHScroll->m_ctrl = this;
	m_CUGHScroll->m_GI	= m_GI;

	return UG_SUCCESS;
}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::SetNewEditClass(CWnd * edit){
//
//	m_GI->m_editCtrl = edit;
//
//	return UG_SUCCESS;
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//CWnd * CUGCtrl::GetEditClass(){
//	
//	return m_GI->m_editCtrl;
//
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::SetNewMaskedEditClass(CWnd * edit){
//
//	m_GI->m_maskedEditCtrl = edit;
//
//	return UG_SUCCESS;
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//CWnd * CUGCtrl::GetMaskedEditClass(){
//	
//	return m_GI->m_maskedEditCtrl;
//
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
//int CUGCtrl::SetTrackingWindow(CWnd *wnd){
//	
//	if(wnd == NULL)
//		return UG_ERROR;
//
//	m_trackingWnd = wnd;
//	m_trackingWnd->SetWindowPos(&CWnd::wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
//
//	return UG_SUCCESS;
//}
/***************************************************
SetTrackingWindowMode	

Params
	mode - 0 normal, 1 stay close
Return
****************************************************/
//int CUGCtrl::SetTrackingWindowMode(int mode){
//	
//	m_GI->m_trackingWndMode = mode;
//	return UG_SUCCESS;
//}
/***************************************************
	Purpose
	Params
	Return
****************************************************/
//void CUGCtrl::MoveTrackingWindow(){
//
//	if(m_trackingWnd == NULL)
//		return;
//
//	RECT tRect,origTRect;
//	RECT rect;
//
//	BOOL moveWindow = FALSE;
//
//	int sWidth = GetSystemMetrics(SM_CXSCREEN);
//	int sHeight = GetSystemMetrics(SM_CYSCREEN);
//
//	m_trackingWnd->GetWindowRect(&origTRect);
//	m_trackingWnd->GetWindowRect(&tRect);
//	m_CUGGrid->ScreenToClient(&tRect);
//
//	GetCellRect(m_GI->m_dragCol,m_GI->m_dragRow,&rect);
//
//	if(m_GI->m_trackingWndMode == 1){  //stay close mode
//		int tWidth = tRect.right - tRect.left;
//		int tHeight = tRect.bottom - tRect.top;
//		
//		tRect.left = rect.right +30;
//		tRect.right = tRect.left + tWidth;
//		tRect.top = rect.top;
//		tRect.bottom = rect.top + tHeight;
//
//		m_CUGGrid->ClientToScreen(&tRect);
//		moveWindow = TRUE;
//	}
//
//	if(tRect.top < rect.bottom && tRect.bottom > rect.top && 
//		tRect.left < rect.right && tRect.right > rect.left){
//		
//		int rightInc = rect.right - tRect.left + 5;
//		int downInc = rect.bottom - tRect.top + 5;
//
//		//move the tracking rect over to the right if possible
//		if(origTRect.right+rightInc < sWidth){
//			m_trackingWnd->GetWindowRect(&tRect);
//			tRect.left += rightInc;
//			tRect.right += rightInc;
//			moveWindow = TRUE;
//		}
//		//else move the tracking rect down
//		else if(origTRect.bottom + downInc < sHeight){
//			m_trackingWnd->GetWindowRect(&tRect);
//			tRect.top += downInc;
//			tRect.bottom += downInc;
//			moveWindow = TRUE;
//		}
//		else{
//			rightInc = rect.left - tRect.right - 5;
//			downInc = rect.top  - tRect.bottom - 5;
//
//			//move the tracking rect to the left
//			if(origTRect.left+rightInc >0){
//				m_trackingWnd->GetWindowRect(&tRect);
//				tRect.left += rightInc;
//				tRect.right += rightInc;
//				moveWindow = TRUE;
//			}
//			//move the tracking rect up
//			else if(origTRect.bottom + downInc < sHeight){
//				m_trackingWnd->GetWindowRect(&tRect);
//				tRect.top += downInc;
//				tRect.bottom += downInc;
//				moveWindow = TRUE;
//			}
//		}
//	}
//
//	if(moveWindow){
//		OnTrackingWindowMoved(&origTRect,&tRect);
//		m_trackingWnd->MoveWindow(&tRect,TRUE);
//		m_trackingWnd->UpdateWindow();
//		RedrawAll();
//	}
//}

/***************************************************
OnTrackingWindowMoved
	Called to notify the grid that the tracking window was moved
Params:
	origRect	- from
	newRect		- to location and size of the track window
Return:
	<none>
****************************************************/
//void CUGCtrl::OnTrackingWindowMoved(RECT *origRect,RECT *newRect){
//	UNREFERENCED_PARAMETER(*origRect);
//	UNREFERENCED_PARAMETER(*newRect);
//}

/***************************************************
	Purpose
	Params
	Return
****************************************************/
int CUGCtrl::EnableUpdate(BOOL state){
	
	m_enableUpdate = state;

	return UG_SUCCESS;
}
/***************************************************
OnHint
	Is called whent the hint is about to be displayed on the main grid area,
	here you have a chance to set the text that will be shown
Params:
	col, row	- the cell coordinates of where the menu originated from
	string		- pointer to the string that will be shown
Return:
	TRUE - to show the hint
	FALSE - to prevent the hint from showing
****************************************************/
int CUGCtrl::OnHint(int col,long row,int section, TOOLTIP_SETTINGS &ttSettings){
	if (section == UG_SIDEHEADING)
	{
		return FALSE;
	}

	// Set width in pixels for multiline tooltip 
	//  - without this message tooltip is single line (i.e. line breaks ignored)
	if (section != UG_GRID)
	{
		return FALSE;
	}

	CRect rcCell;
	CSize sizeText;
	//UINT cellFlags;
	BOOL bResult = GetDataSource()->GetHintForCell(col, row, ttSettings);

	if (bResult)
	{
		ttSettings.m_strTooltipText = PrepareTooltipText(ttSettings.m_strTooltipText);
	}
	return bResult;
}
/***************************************************
OnVScrollHint
	Is called whent the hint is about to be displayed on the vertical scroll bar,
	here you have a chance to set the text that will be shown
Params:
	row		- current top row
	string	- pointer to the string that will be shown
Return:
	TRUE - to show the hint
	FALSE - to prevent the hint from showing
****************************************************/
int CUGCtrl::OnVScrollHint(long row, std::wstring*string){
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*string);
	return TRUE;
}
/***************************************************
OnHScrollHint
	Is called whent the hint is about to be displayed on the horizontal scroll bar,
	here you have a chance to set the text that will be shown
Params:
	col		- current left col
	string	- pointer to the string that will be shown
Return:
	TRUE - to show the hint
	FALSE - to prevent the hint from showing
****************************************************/
int CUGCtrl::OnHScrollHint(int col, std::wstring*string){
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(*string);
	return TRUE;
}

LRESULT CUGCtrl::MakeSuperTooltip(NM_PPTOOLTIP_NEED_TT * pNeedTT, 
	CXeTooltipIF* xtooltip, HWND hWnd, int section)
{
	int col;
	long row;
	// get mouse coordinates
	POINT point;
	GetCursorPos(&point);
	::ScreenToClient(hWnd, &point);

	// check if the mouse pointer is over a cell
	if (GetCellFromPoint(point.x, point.y, &col, &row) == UG_SUCCESS)
	{
		TOOLTIP_SETTINGS ttSettings;
		if (OnHint(col, row, section, ttSettings) == TRUE)
		{
			pNeedTT->ti->hTWnd = hWnd;
			pNeedTT->ti->sTooltip = ttSettings.m_strTooltipText.c_str();
			//SUPER_TOOLTIP_INFO sti;
			//sti.m_sBody = ttSettings.m_strTooltipText.c_str();
			//sti.rgbBg = m_xeUI->GetColor(CID::TT_Bg);
			//sti.rgbText = m_xeUI->GetColor(CID::TT_Text);
			//sti.hWnd = GetSafeHwnd();

			//CSize sizeTxt = GetMultilineTextExtentW(this, pNeedTT->ti->sTooltip.c_str());
			CSize sizeTxt = m_xeUI->GetTextSizeW(EXE_FONT::eUI_Font, pNeedTT->ti->sTooltip.c_str());
			//CSize sizeTxt = GetMultilineTextExtentW(this, sti.m_sBody.c_str());
			//sti.nSizeX = sizeTxt.cx;

			//xtooltip.MakeSuperTip(&sti, *pNeedTT->ti);
			//pNeedTT->ti->hTWnd = hWnd;

			CRect rectCell, rectClient;
			::GetClientRect(hWnd, &rectClient);
			GetCellRect(col, row, &rectCell);

			pNeedTT->ti->rectBounds = rectCell;	// if the mouse moves outside the bounds the tooltip is removed.

			// Normal Tooltip window position is below cell, 8 pixels to the right.
			LONG ttOffsetX = ttSettings.m_bCoverCell ? rectCell.left : rectCell.left + 8;
			LONG ttOffsetY = ttSettings.m_bCoverCell ? rectCell.top - rectClient.bottom : rectCell.bottom - rectClient.bottom;

			if (ttSettings.m_bCanChangeXpos && (ttOffsetX + sizeTxt.cx) > rectClient.right)
			{
				ttOffsetX = rectClient.right - sizeTxt.cx;
				if (ttOffsetX < 0)
				{
					ttOffsetX = 0;
				}
			}

			pNeedTT->ti->ptTipOffset.x = ttOffsetX;
			pNeedTT->ti->ptTipOffset.y = ttOffsetY > 0 ? 0 : ttOffsetY;

			return 1;
		}
	}
	return 0;
}

void CUGCtrl::HideTooltip()
{
	//m_xtooltip->HideTooltip();
	m_CUGGrid->HideTooltip();
	//m_CUGTopHdg->HideTooltip();
	m_CUGSideHdg->HideTooltip();
	//m_CUGCnrBtn->HideToolTip();
	//m_CUGVScroll->HideToolTip();
	//m_CUGHScroll->HideToolTip();
}

/***************************************************
Params
	mode -  0, no user adjusted sizing allowed
			1, user sizing is allowed, but updated don't 
				happen until the sizing is finished
			2, user sizing is allowed, and updates are
				performed in real-time
****************************************************/
int CUGCtrl::SetUserSizingMode(int mode){
	
	m_GI->m_userSizingMode = mode;

	return UG_SUCCESS;
}
/***************************************************
****************************************************/
int CUGCtrl::SetColDataSource(int col,int dataSrcIndex){

	if(dataSrcIndex < 0 || dataSrcIndex > m_dataSrcListLength)
		return UG_ERROR;

	if(dataSrcIndex == 0)
		return SetColDataSource(col,m_GI->m_CUGMem);
	else 
		return SetColDataSource(col,m_dataSrcList[dataSrcIndex]);		
}

/***************************************************
****************************************************/
int CUGCtrl::SetColDataSource(int col,CUGDataSource * dataSrc){
	
	if(col < 0 || col >= m_GI->m_numberCols)
		return UG_ERROR;
	if(dataSrc == NULL)
		return UG_ERROR;

	m_GI->m_colInfo[col].dataSource = dataSrc;
	return UG_SUCCESS;
}

/***************************************************
	ModifyDialogItemText - used in conjunction with EnumChildWindows
	to let us change text of a control on a common dialog
	(e.g. m_findReplaceDialog)
****************************************************/
//BOOL CALLBACK CUGCtrl::ModifyDlgItemText(HWND hWnd, LPARAM lParam){
//	// if the text of the window passed in matches the
//	// text of the first CSring in the array pair pointed
//	// to by lParam, set the text to the 2nd CString 
//	// and return FALSE;
//	CString strTemp;
//	CString * strP = (CString*) lParam;
//	
//	// get the existing text
//	CWnd * wnd = CWnd::FromHandle(hWnd);
//	wnd->GetWindowText(strTemp);
//
//	if(strTemp.Find((LPCTSTR)strP[0]) != -1) {
//		wnd->SetWindowText(strP[1]);
//		return FALSE;
//	}
//	return TRUE;
//}

/***************************************************
	SetCancelMode - used to set the internal flag
	to determine if the contents of the edit control
	are discarded or saved, when the edit control 
	loses focus.
****************************************************/
int CUGCtrl::SetCancelMode( BOOL bCancel )
{
	m_GI->m_bCancelMode = bCancel ? TRUE : FALSE;

	return UG_SUCCESS;
}

/***************************************************
	GetCancelMode - returns the state of the internal
	flag that determines if the contents of the edit
	control are discraded when the edit control
	loses the focus to another window
****************************************************/
BOOL CUGCtrl::GetCancelMode()
{
	return m_GI->m_bCancelMode;
}

//CXeScrollBarMarkers* CUGCtrl::GetScrollBarCtrl(int nBar) const
//{
////#pragma NOTE("SKMOD to support CXeScrollBar")
//	if(nBar==SB_HORZ)
//		return (CXeScrollBarMarkers*)m_CUGHScroll;
//	else
//		return (CXeScrollBarMarkers*)m_CUGVScroll;
//}

//CSize CUGCtrl::CalcTextSize(CString& strText)
//{
//	CSize szTxt;
//	CFont* pCellFont = m_xeUI->GetFont(EXE_FONT::eUI_Font);
//	if (pCellFont == nullptr)
//	{
//		return szTxt;
//	}
//	CDC* pDC = GetDC();
//	CFont* pOldFont = pDC->SelectObject(pCellFont);
//	CRect rcTxt;
//	rcTxt.SetRectEmpty();
//	pDC->DrawText(strText, &rcTxt, DT_CALCRECT);
//	szTxt.SetSize(rcTxt.Width(), rcTxt.Height());
//	pDC->SelectObject(pOldFont);
//	ReleaseDC(pDC);
//	return szTxt;
//}

