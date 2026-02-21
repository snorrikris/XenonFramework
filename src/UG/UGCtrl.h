/*************************************************************************
				Class Declaration : CUGCtrl
**************************************************************************
	Source file : UGCtrl.cpp
	Header file : UGCtrl.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		This is the main grid's class.  It contains and
		controls all of the windows and classes that
		make up Ultimate Grid for MFC.

		This class is very important and the first class
		that any development with Ultimate Grid will encounter.
		
	Details
		- Generaly a class derived from this class
		  is used in applications. A derived class
		  is neccessary in order to handle any of the
		  notifications that the grid sends.
		- Being derived from CWnd allows the grid to be
		  used anywhere a CWnd can be used.
		- When this class is created, using CreateGrid or AttachGrid,
		  it also:
			- creates all grid's child windows
			  (CUGGrid, CUGTopHdg, CUGSideHdg, etc.)
			- creates a default memory manager datasource
			  and registers grid's standard cell types
*************************************************************************/
#ifndef _UGCtrl_H_
#define _UGCtrl_H_

// grid feature enable defines - rem out one or more of these defines 
// to remove one or more features
#define UG_ENABLE_MOUSEWHEEL
//#define UG_ENABLE_PRINTING
//#define UG_ENABLE_FINDDIALOG
//#define UG_ENABLE_SCROLLHINTS

#ifdef __AFXOLE_H__  //OLE must be included
	#define UG_ENABLE_DRAGDROP
#endif

#ifndef WS_EX_LAYOUTRTL
	#define WS_EX_LAYOUTRTL		0x00400000L
#endif // WS_EX_LAYOUTRTL


import Xe.Menu;
import Xe.GridDataSource;
import Xe.UIcolorsIF;
import Xe.D2DWndBase;
//import Xe.LogDefs;
import Xe.DefData;

/////////////////////////////////////////////////
class CUGCtrl : public CXeD2DCtrlBase
{
protected:
	structGridSelection m_gridsel;	// grid selection data

// Construction
public:
	CUGCtrl(CXeGridDataSource* pDS, const wchar_t* strRegSectionName,
		GridNotifyCallbackFunc gridNotifyCallback = nullptr);

// Attributes
public:
	//CXeUIcolorsIF* m_xeUI = nullptr;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUGCtrl)
protected:
	//virtual BOOL PreTranslateMessage(MSG* pMsg) override;

	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override;

	//virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender) override;
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUGCtrl();

	// Generated message map functions
protected:
	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	//{{AFX_MSG(CUGCtrl)
	//afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual LRESULT _OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam) override;
	//afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//afx_msg void OnDestroy();
	//afx_msg void OnPaint();
	//afx_msg void OnNcPaint();
	//afx_msg void OnSetFocus(CWnd* pOldWnd);
	virtual LRESULT _OnSetFocus(HWND hOldWnd) override;
	//afx_msg void OnSysColorChange();
	//afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual LRESULT _OnVScroll(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT _OnHScroll(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT _OnGetDlgCode(WPARAM wParam, LPARAM lParam) override;
	LRESULT OnCellTypeMessage(WPARAM, LPARAM);
	//afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	//}}AFX_MSG
	//DECLARE_MESSAGE_MAP()

public:

	int m_contructorResults;

protected:
	std::wstring m_strRegSectionName;

public:
	int GetGridNumCols() { return GetDataSource()->GetGridTblInfo().GetNumColumns(); }
	int GetGridNumRows() { return GetDataSource()->GetNumRows(); }
	void GetGridSizeFromGrid();
	void SetGridSizeToGrid();

public:
	// data source - note pointer is always valid - see ctor.
	CXeGridDataSource* GetDataSource() { XeASSERT(m_pInitialDataSource); return m_pInitialDataSource; }
	//DSType GetDataSourceType() { return GetDataSource()->GetDataSourceType(); }
	dsid_t GetDataSourceId() { return GetDataSource()->GetDataSourceId(); }
	structGridSelection& GetGridSelData() { return m_gridsel; };

protected:
	CXeGridDataSource* m_pInitialDataSource = nullptr;

	int m_nMaxLen;		// maximum text length accepted from user input (0 = unlimited)
	//CPen* m_ppenWvBorder;
	CUGSortArrowType m_sortarrowcell;
	CUGUrlBtnType m_urlbtncell;
	POINT m_ptLastLbtnDnMousePos;	// Last position in client coords. when L button down.
									//  Set to -1000000, -1000000 when L btn goes up.
									//  Used to implement drag start operation.

public:
	// sorting support
	BOOL SortingEnabled() { return FALSE; /*m_gridSize.m_fSortingEnabled;*/ }
	//void EnableSorting(BOOL fSortingEnabled = TRUE) { m_gridSize.m_fSortingEnabled = fSortingEnabled; }

	// selection support
	int GetSelection(BOOL* pfEntireRowsSelected = 0);
	void SetSelection(BOOL fSelectAll = TRUE, int nStartCol = -1, long nStartRow = -1,
		int nEndCol = -1, long nEndRow = -1);
	BOOL CopySelectionToClipboard(BOOL fTH = TRUE, BOOL fSH = FALSE);
	BOOL CopySelectionToString(std::wstring& strSel, BOOL fTH = TRUE, BOOL fSH = FALSE);
	BOOL ClipboardOperation(UINT uOperation);

	void SetGridFromDataSource(BOOL fRedraw = TRUE);
	void UpdateRowCount();
	void RefreshGrid(BOOL fHeaderAndTabOnly = FALSE);	// RedrawAll, update tab/sheet name

protected:
	GridNotifyCallbackFunc m_GridNotifyCallback = nullptr;
	bool _NotifyParentView(stGridNotifyData& nfData)
	{
		if (m_GridNotifyCallback) { return m_GridNotifyCallback(nfData); }
		return false;
	}

	stGridNotifyData _GetMenuNotifyData(EGRIDOP eOp, int col, long row, int section, int menuCmdId);
	stGridNotifyData _GetNotifyData(EGRIDOP eOp, int col = -1, long row = -1, int section = UG_GRID);
	stGridNotifyData _GetNotifyDataFlag(EGRIDOP eOp, bool bFlag);

public:
	// Calc. text size using current grid cell font.
	//CSize CalcTextSize(CString& strText);

	void OnEditFind();
	void _UpdateMenuCallback(CXeMenu* pMenu, size_t top_level_index);

	//***** internal classes *****
	
	//data source list
	CUGDataSource		**m_dataSrcList;
	int					m_dataSrcListLength;
	int					m_dataSrcListMaxLength;

	//CUGPtrList			*m_fontList;
	//CUGPtrList			*m_bitmapList;
	CUGPtrList			*m_cellTypeList;
	CUGPtrList			*m_cellStylesList;
	CUGPtrList			*m_validateList;
	CUGPtrList			*m_displayFormatList;

	//standard cell types
	CUGCellType			m_normalCellType;
	std::unique_ptr<CUGDropListType> m_dropListType;
	CUGCheckBoxType		m_checkBoxType;
	CUGArrowType		m_arrowType;
	CUGProgressType		m_progressType;


	//#ifdef UG_ENABLE_PRINTING
	//CUGPrint*			m_CUGPrint;
	//#endif

	//popup menu
	std::unique_ptr<CXeMenu> m_menu;			
	int					m_menuCol;
	long				m_menuRow;
	int					m_menuSection;


	//grid info list / sheet variables
	CUGGridInfo ** m_GIList;
	int m_currentSheet;
	int m_numberSheets;


	//update enable/disable flag
	BOOL m_enableUpdate;


public:

	//current sheet 
	CUGGridInfo * m_GI;

	//child window classes
	CUGGrid				*m_CUGGrid;
	CUGTopHdg			*m_CUGTopHdg;
	CUGSideHdg			*m_CUGSideHdg;
	//CUGCnrBtn			*m_CUGCnrBtn;

	CUGVScroll			*m_CUGVScroll;
	CUGHScroll			*m_CUGHScroll;

	bool m_isComponentsCreated = false;

	//scroll hint window
	//#ifdef UG_ENABLE_SCROLLHINTS
	//CUGHint				*m_CUGHint;
	//#endif
	
	//tabs
	//CUGTab				*m_CUGTab;

	//tracking topmost window
	//CWnd				*m_trackingWnd;

	//default edit control
	CUGEdit			m_defEditCtrl;
	//CUGMaskedEdit	m_defMaskedEditCtrl;

protected:

	//***** internal functions *****
	void CalcTopRow();			//Calcs the max top row then adjusts the top row if needed
	void CalcLeftCol();			//Calcs the max left col then adjusts the left col if needed
	void AdjustTopRow();		//moves top row so that the current row is in view
	void AdjustLeftCol();		//move the left col so that the current col is in view
	void Update();				//updates all windows - also performs recalculations
	void Moved();				//this function is called whenever a grid movement is made
								//this function then notifies the child windows
	
	BOOL CreateChildWindows();	//creates the child windows
	//void ToggleLayout( CWnd *pWnd );

	void SetLockRowHeight();
	void SetLockColWidth();

	//void MoveTrackingWindow();

public:

	//check to see if the new position is valid
	int VerifyTopRow(long* newRow);
	int VerifyCurrentRow(long* newRow);
	int VerifyLeftCol(int* newCol);
	int VerifyCurrentCol(int* newCol);


	//*************** creation/setup *****************
	//window creation
	BOOL CreateGrid(DWORD dwStyle, DWORD dwExStyle, const CRect& rect, HWND hParentWnd, UINT nID);
	//dialog resource functions
	//BOOL AttachGrid(CWnd * wnd,UINT ID);

	void AdjustComponentSizes();	//adjusts and positions the child windows

	//*************** editing *****************
	int		StartEdit();
	int		StartEdit(int key);
	int		StartEdit(int col,long row,int key);
	int		ContinueEdit(int adjustCol,long adjustRow);

	//*************** row and column *****************
	int		SetNumberRows(long rows,BOOL redraw = TRUE);
	long	GetNumberRows();
	
	int		SetNumberCols(int cols,BOOL redraw = TRUE);
	int		GetNumberCols();

	int		SetDefColWidth(int width);
	int		SetColWidth(int col,int width, bool notify = true);
	int		GetColWidth(int col,int *width);
	int		GetColWidth(int col);

	int		SetDefRowHeight(int height);
	int		SetUniformRowHeight(int flag);
	int		SetRowHeight(long row,int height);
	int		GetRowHeight(long row,int *height);
	int		GetRowHeight(long row);

	int GetNonUniformRowHeight(long row);

	int		GetCurrentCol();
	long	GetCurrentRow();
	int		GetLeftCol();
	long	GetTopRow();
	int		GetRightCol();
	long	GetBottomRow();

	int		InsertCol(int col);
	int		AppendCol();
	int		DeleteCol(int col);
	int		InsertRow(long row);
	int		AppendRow();
	int		DeleteRow(long row);

	//************ heading functions *************
	int		SetTH_NumberRows(int rows);
	int		SetTH_RowHeight(int row,int height);
	int		SetSH_NumberCols(int cols);
	int		SetSH_ColWidth(int col,int width);


	//************* find/replace/sort *********************
	//int FindFirst(CString *string,int *col,long *row,long flags);
	//int FindNext(CString *string,int *col,long *row,int flags);

	//int FindDialog();
	//int ReplaceDialog();
	//LRESULT ProcessFindDialog(WPARAM, LPARAM);
	//int FindInAllCols(BOOL state);
	//CFindReplaceDialog *m_findReplaceDialog;


	int SortBy(int col,int flag = UG_SORT_ASCENDING);
	int SortBy(int *cols,int num,int flag = UG_SORT_ASCENDING);


	//************* child windows *********************
	int		SetTH_Height(int height);
	int		GetTH_Height();
	int		SetSH_Width(int width);
	int		GetSH_Width();
	int		SetVS_Width(int width);
	int		GetVS_Width();
	int		SetHS_Height(int height);
	int		GetHS_Height();

	//************* default/cell information *********************
	int		GetCell(int col,long row,CUGCell *cell);
	int		GetCellIndirect(int col,long row,CUGCell *cell);
	int		SetCell(int col,long row,CUGCell *cell);
	int		DeleteCell(int col,long row);

	int GetRowLineCount(long row);

	int		SetColDefault(int col,CUGCell *cell);
	int		GetColDefault(int col,CUGCell *cell);
	int		SetGridDefault(CUGCell *cell);
	int		GetGridDefault(CUGCell *cell);
	int		SetHeadingDefault(CUGCell *cell);
	int		GetHeadingDefault(CUGCell *cell);

	//column information and translation
	int		GetColTranslation(int col);
	int		SetColTranslation(int col,int transCol);


	//cell joining
	int		JoinCells(int startcol,long startrow,int endcol,long endrow);
	int		UnJoinCells(int col,long row);
	int		EnableJoins(BOOL state);

	//cell functions
	//int		QuickSetText(int col,long row,LPCTSTR string);
	//int		QuickSetNumber(int col,long row,double number);
	//int		QuickSetMask(int col,long row,LPCTSTR string);
	//int		QuickSetLabelText(int col,long row,LPCTSTR string);
	//int		QuickSetTextColor(int col,long row,COLORREF color);
	//int		QuickSetBackColor(int col,long row,COLORREF color);
	//int		QuickSetAlignment(int col,long row,short align);
	//int		QuickSetBorder(int col,long row,short border);
	//int		QuickSetBorderColor(int col,long row,HPEN pen);
	//int		QuickSetFont(int col,long row,CFont * font);
	//int		QuickSetFont(int col,long row,int index);
	//int		QuickSetBitmap(int col,long row,CBitmap * bitmap);
	//int		QuickSetBitmap(int col,long row,int index);
	//int		QuickSetCellType(int col,long row,long type);
	//int		QuickSetCellTypeEx(int col,long row,long typeEx);
	//int		QuickSetHTextColor(int col,long row,COLORREF color);
	//int		QuickSetHBackColor(int col,long row,COLORREF color);

	//int		QuickSetRange(int startCol,long startRow,int endCol,long endRow,
	//			CUGCell* cell);

	int			QuickGetText(int col,long row, std::wstring*string);
	LPCTSTR		QuickGetText(int col,long row);

	int		DuplicateCell(int destCol,long destRow, int srcCol, long srcRow);


	//************* general modes and settings *********************
	int	SetCurrentCellMode(int mode);	//focus and highlighting
											//bit 1:focus 2:hightlight
	int	SetHighlightRow(int mode, BOOL bExtend = TRUE);		//on,off
	int	SetMultiSelectMode(int mode);	//on,off
	int	Set3DHeight(int height);		//in pixels
	int	SetPaintMode(int mode, BOOL fRedraw = TRUE );			//on,off
	int	GetPaintMode();
	int	SetVScrollMode(int mode);		// 0-normal 2- tracking 3-joystick
	int	SetHScrollMode(int mode);		// 0-normal 2- tracking
	
	int HideCurrentCell();

	int SetBallisticMode(int mode);	// 0:off  1:on  2:on-squared 3:on-cubed
	int SetBallisticDelay(int milisec);

	int SetBallisticKeyMode(int mode); //0-off n-number of key repeats before increase
	int SetBallisticKeyDelay(int milisec);
	
	//int SetDoubleBufferMode(int mode);	// 0:off 1:on
	//int SetGridLayout( int layoutMode );	// 0: Left-to-Right (default),
											// 1: Right-to-Left

	int LockColumns(int numCols);		
	int	LockRows(int numRows);

	//int	EnableCellOverLap(BOOL state);
	int EnableColSwapping(BOOL state);
	int EnableExcelBorders(BOOL state);
	int EnableScrollOnParialCells( BOOL state );


	//************* movement *********************
	int MoveTopRow(int flag);	//0-lineup 1-linedown 2-pageup 3-pagedown 4-top 5-bottom
	int AdjustTopRow(long adjust);
	int MoveCurrentRow(int flag);
	int AdjustCurrentRow(long adjust);
	int GotoRow(long row);
	int GotoRowEx(long row);
	int GotoRowAndAdjustYpos(long row, int Y);
	int GotoRowAndAdjustYposEx(long row, int Y);
	int GotoRowAndAdjustYposVcenterEx(long row);
	int GotoLastRow();
	int	SetTopRow(long row);

	int MoveLeftCol(int flag);
	int AdjustLeftCol(int adjust);
	int MoveCurrentCol(int flag);
	int AdjustCurrentCol(int adjust);
	int GotoCol(int col);
	int	SetLeftCol(int col);

	int GotoCell(int col,long row);
	int GotoCellEx(int col,long row);

	void HScroll(UINT nSBCode, UINT nPos);

	//************* finding cells *********************
	int GetCellFromPoint(int x,int y,int *col,long *row);
	int GetCellFromPoint(int x,int y,int *ptcol,long *ptrow,RECT *rect);
	int GetAbsoluteCellFromPoint(int x,int y,int *ptcol,long *ptrow);

	int GetCellRect(int col,long row,RECT *rect);
	int GetCellRect(int *col,long *row,RECT *rect);
	int GetRangeRect(int startCol,long startRow,int endCol,long endRow,RECT *rect);
	
	int GetJoinStartCell(int *col,long *row);
	int GetJoinStartCell(int *col,long *row,CUGCell *cell);
	int GetJoinRange(int *col,long *row,int *col2,long *row2);


	//************* cell types *********************
	long AddCellType(CUGCellType *);   //returns the cell type ID
	int RemoveCellType(int ID);
	CUGCellType * GetCellType(int type);	//returns the pointer to a cell type
	int GetCellType(CUGCellType * type);

	//************* data sources *********************
	int AddDataSource(CUGDataSource * ds);
	CUGDataSource * GetDataSource(int index);
	int RemoveDataSource(int index);
	int SetDefDataSource(int index);
	int GetDefDataSource();
	int SetGridUsingDataSource(int index);

	//************* fonts *********************
	//int AddFont(LPCTSTR fontName,int height,int weight);
	//int AddFont(int height,int width,int escapement,int orientation, 
	//		int weight,BYTE italic,BYTE underline,BYTE strikeOut, 
	//		BYTE charSet,BYTE outputPrecision,BYTE clipPrecision, 
	//		BYTE quality,BYTE pitchAndFamily,LPCTSTR fontName);
	//int AddFontIndirect( LOGFONT lgFont );
	//int RemoveFont(int index);
	//int ClearAllFonts();
	//CFont * GetFont(int index);
	//int SetDefFont(CFont *font);
	//int SetDefFont(int index);

	//************* bitmaps *********************
	//int AddBitmap( UINT resourceID,LPCTSTR resourceName= NULL);
	//int AddBitmap( LPCTSTR fileName);
	//int RemoveBitmap(int index);
	//int ClearAllBitmaps();
	//CBitmap* GetBitmap(int index);


	//************* redrawing *********************
	int RedrawAll();
	int RedrawCell(int col,long row);
	int RedrawRow(long row);
	int RedrawCol(int col);
	int RedrawRange(int startCol,long startRow,int endCol,long endRow);
	void TempDisableFocusRect();


	//************* multi-select *********************
	int ClearSelections();
	int Select(int col,long row);
	int SelectRange(int startCol,long startRow,int endCol,long endRow);
	int EnumFirstSelected(int *col,long *row);
	int EnumNextSelected(int *col,long *row);
	int EnumFirstBlock(int *startCol,long *startRow,int *endCol,long *endRow);
	int EnumNextBlock(int *startCol,long *startRow,int *endCol,long *endRow);
	BOOL IsSelected(int col,long row,int *blockNum = NULL);

	//************* clipboard ********************
	int CopySelected();
	int CutSelected();
	int CopySelected(int cutflag);  //TRUE,FALSE
	int Paste();
	int Paste(std::wstring&string);
	int CopyToClipBoard(std::wstring* string);
	int CopyFromClipBoard(std::wstring* string);
	void CreateSelectedString(std::wstring& string,int cutFlag);


	//************* column sizing ********************
	int FitToWindow(int startCol,int endCol);
	int BestFit(int startCol,int endCol,int CalcRange,int flag);


	//************* printing ********************
	//#ifdef UG_ENABLE_PRINTING
	//	int PrintInit(CDC * pDC, CPrintDialog* pPD, int startCol,long startRow,
	//		int endCol,long endRow);
	//	int PrintPage(CDC * pDC, int pageNum);
	//	int PrintSetMargin(int whichMargin,int size);
	//	int PrintSetScale(int scalePercent);
	//	int PrintSetOption(int option,long param);
	//	int PrintGetOption(int option,long *param);
	//#endif


	//************* hints ********************
	int UseHints(BOOL state);
	int UseVScrollHints(BOOL state);
	int UseHScrollHints(BOOL state);


	//************* pop-up menu ********************
	CXeMenu * GetPopupMenu();
	int EmptyMenu();
	//int AddMenuItem(int ID,LPCTSTR string);
	//int RemoveMenuItem(int ID);
	int EnableMenu(BOOL state);


	//************* drag and drop ********************
	//#ifdef UG_ENABLE_DRAGDROP
	//	COleDataSource	m_dataSource;
	//	CUGDropTarget	m_dropTarget;

	//	int StartDragDrop();
	//	int DragDropTarget(BOOL state);
	//#endif


	//************* tabs ********************
	//int AddTab( CString label, long ID );
	//int InsertTab( int pos, CString label, long ID );
	//int DeleteTab( long ID );
	//int SetTabBackColor( long ID, COLORREF color );
	//int SetTabTextColor( long ID, COLORREF color );
	//int SetTabWidth( int width );
	//int SetCurrentTab( long ID );
	//int GetCurrentTab();


	//************* sheets ********************
	int SetNumberSheets(int numSheets);
	int DeleteCurrentSheet( BOOL update = TRUE );
	int GetNumberSheets();
	int SetSheetNumber(int index,BOOL update = TRUE);
	int GetSheetNumber();

	//****************************************************
	//*********** Over-ridable Notify Functions **********
	//****************************************************
	void OnSetup();
	void OnSheetSetup(int sheetNumber);

	//movement and sizing
	int  OnCanMove(int oldcol,long oldrow,int newcol,long newrow);
	int  OnCanViewMove(int oldcol,long oldrow,int newcol,long newrow);
	void OnHitBottom(long numrows,long rowspast,long rowsfound);
	void OnHitTop(long numrows,long rowspast);
	
	int  OnCanSizeCol(int col);
	void OnColSizing(int col,int *width);
	void OnColSized(int col,int *width);
	int  OnCanSizeRow(long row);
	void OnRowSizing(long row,int *height);
	void OnRowSized(long row,int *height);

	int  OnCanSizeTopHdg();
	int  OnCanSizeSideHdg();
	int  OnTopHdgSizing(int *height);
	int  OnSideHdgSizing(int *width);
	int  OnTopHdgSized(int *height);
	int  OnSideHdgSized(int *width);

	void OnColRowSizeFinished();
	
	//void SendDataGridColWidthChangedMessage(int col, int width);
	//afx_msg LRESULT OnColWidthChangedMessage(WPARAM wParam, LPARAM lParam);

	void OnColChange(int oldcol,int newcol);
	void OnRowChange(long oldrow,long newrow);
	void OnCellChange(int oldcol,int newcol,long oldrow,long newrow);
	void OnLeftColChange(int oldcol,int newcol);
	void OnTopRowChange(long oldrow,long newrow);
	void OnViewMoved( int nScrolDir, long oldPos, long newPos );

	//mouse and key strokes
	void OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed);
	void OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed);
	void OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);
	void OnMouseMove(int col,long row,POINT *point,UINT nFlags,BOOL processed=0);
	void OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
	void OnTH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
	void OnTH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed=0);
	void OnSH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
	void OnSH_RClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);
	void OnSH_DClicked(int col,long row,RECT *rect,POINT *point,BOOL processed=0);
	void OnCB_LClicked(int updn,RECT *rect,POINT *point,BOOL processed=0);
	void OnCB_RClicked(int updn,RECT *rect,POINT *point,BOOL processed=0);
	void OnCB_DClicked(RECT *rect,POINT *point,BOOL processed=0);
	
	void OnKeyDown(UINT *vcKey,BOOL processed);
	void OnKeyUp(UINT *vcKey,BOOL processed);
	void OnCharDown(UINT *vcKey,BOOL processed);
	
	//GetCellIndirect notification
	void OnGetCell(int col,long row,CUGCell *cell);

	int OnGetRowLineCount(long row) { return 1; };

	//SetCell notification
	void OnSetCell(int col,long row,CUGCell *cell);
	
	//data source notifications
	void OnDataSourceNotify(int ID,long msg,long param);

	//cell type notifications
	int OnCellTypeNotify(long ID,int col,long row,long msg,long long param);
	int OnCheckbox(long ID, int col, long row, long msg, long long param);
	int OnDropList(long ID, int col, long row, long msg, long long param);

	//editing
	int OnEditStart(int col, long row,HWND edit);
	int OnEditVerify(int col,long row,HWND edit,UINT *vcKey);
	int OnEditFinish(int col, long row,HWND edit,LPCTSTR string,BOOL cancelFlag);
	int OnEditContinue(int oldcol,long oldrow,int* newcol,long* newrow);
	int OnEditKeyDown(int col,long row,HWND edit,UINT *vcKey);
	int OnEditKeyUp(int col,long row,HWND edit,UINT *vcKey);

	BOOL m_fCancelNext_OnEditContinue;
	void ProcessAfterEditAction(BOOL fEditOK, eGridAction& action, int col, long row);

	// Set callback function that is called after user has edited a cell.
	void SetOnEditFinishedCallback(OnEditFinishedCallbackFunc callback) { m_onEditFinishedCallback = callback; }
	OnEditFinishedCallbackFunc m_onEditFinishedCallback = nullptr;

	//menu notifications
	void OnMenuCommand(int col,long row,int section,int item);
	int  OnMenuStart(int col,long row,int section);

	// SK MOD - needed NF when menu dismissed to dly paint the grid.
	void OnMenuEnd(int col,long row,int section) {};

	//hints
	int OnHint(int col,long row,int section, TOOLTIP_SETTINGS &ttSettings);
	int OnVScrollHint(long row, std::wstring*string);
	int OnHScrollHint(int col, std::wstring*string);

	LRESULT MakeSuperTooltip(NM_PPTOOLTIP_NEED_TT *pNeedTT, 
		CXeTooltipIF* xtooltip, HWND hWnd, int section);

	void HideTooltip();

	//drag and drop
	#ifdef UG_ENABLE_DRAGDROP
		DROPEFFECT OnDragEnter(COleDataObject* pDataObject);
		DROPEFFECT OnDragOver(COleDataObject* pDataObject,int col,long row);
		DROPEFFECT OnDragDrop(COleDataObject* pDataObject,int col,long row);
	#endif

	//sorting
	int  OnSortEvaluate(CUGCell *cell1,CUGCell *cell2,int flags);
	
	//DC setup
	//void OnScreenDCSetup(CDC *dc,CDC *db_dc,int section);
	
	void OnAdjustComponentSizes(RECT *grid,RECT *topHdg,RECT *sideHdg,
		RECT *cnrBtn,RECT *vScroll,RECT *hScroll,RECT *tabs);

	//focus rect setup
	//void OnDrawFocusRect(CDC *dc,RECT *rect);

	void OnSetFocus(int section);
	void OnKillFocus(int section);
	void OnKillFocus(int section, HWND hNewWnd);
	
	//void DrawExcelFocusRect(CDC *dc,RECT *rect);
		
	int StartMenu(int col,long row,POINT *point,int section);

	int SetArrowCursor(HCURSOR cursor);
	int SetWESizingCursor(HCURSOR cursor);
	int SetNSSizingCursor(HCURSOR cursor);

	int SetMargin(int pixels);

	//functions to be called by grid edit controls
	int EditCtrlFinished(LPCTSTR string,BOOL cancelFlag,
		BOOL continueFlag,int continueCol,long continueRow);

	int EditCancel();

	int SetCancelMode( BOOL bCancel );
	BOOL GetCancelMode();

	CUGCellType * GetCellType(int col,long row);

	BOOL OnColSwapStart(int col);
	BOOL OnCanColSwap(int fromCol,int toCol);
	void OnColSwapped(int fromCol,int toCol);

	int MoveColPosition(int fromCol,int toCol,BOOL insertBefore);

	int SetNewTopHeadingClass(CUGTopHdg * topHeading);
	int SetNewSideHeadingClass(CUGSideHdg * sideHeading);
	int SetNewGridClass(CUGGrid * grid);
	int SetNewMultiSelectClass(CUGMultiSelect * multiSelect);
	//int SetNewTabClass(CUGTab * tab);
	int SetNewVScrollClass(CUGVScroll * scroll);
	int SetNewHScrollClass(CUGHScroll * scroll);


	//int SetNewEditClass(CWnd * edit);
	//int SetNewMaskedEditClass(CWnd * edit);
	//CWnd * GetEditClass();
	//CWnd * GetMaskedEditClass();

	//tracking window
	//int SetTrackingWindow(CWnd *wnd);
	//int SetTrackingWindowMode(int mode);
	//void OnTrackingWindowMoved(RECT *origRect,RECT *newRect);

	int EnableUpdate(BOOL state);

	int SetUserSizingMode(int state);

	int SetColDataSource(int col,int dataSrcIndex);
	int SetColDataSource(int col,CUGDataSource * dataSrc);

	//static BOOL CALLBACK ModifyDlgItemText(HWND hWnd, LPARAM lParam);

	// This function returns the m_CUGPrint.
	// It is used in printing cells.
	//#ifdef UG_ENABLE_PRINTING
	//inline	CUGPrint* GetUGPrint (void) {
	//	return m_CUGPrint;
	//}
	//#endif
};

// Helper class to set grid edit cell in progress flag in current data source.
class CSetGridEditInProgressFlagInDataSource
{
public:
	// Constructor sets flag.
	CSetGridEditInProgressFlagInDataSource(CUGCtrl* pGrid)
	{
		XeASSERT(pGrid);
		m_pGrid = pGrid;
		m_pDSrc = pGrid->GetDataSource();
		XeASSERT(m_pDSrc);
		if (m_pDSrc)
			m_pDSrc->SetGridEditInProgressFlag();
	}

	// Destructor clears flag.
	~CSetGridEditInProgressFlagInDataSource()
	{
		if (m_pDSrc)
			m_pDSrc->SetGridEditInProgressFlag(FALSE);
	}

private:
	CUGCtrl* m_pGrid;
	CXeGridDataSource* m_pDSrc;
};

#endif // _UGCtrl_H_
