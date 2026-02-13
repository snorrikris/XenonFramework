/*************************************************************************
				Class Implementation : CUGDropListType
**************************************************************************
	Source file : UGDLType.cpp
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved
*************************************************************************/


#include "../os_minimal.h"
#include <string>
#include <d2d1.h>
#include <dwrite.h>

import Xe.UIcolorsIF;
#include "ugdefine.h"
#include "uggdinfo.h"
#include "UGDLType.h"

import Xe.PopupCtrl;
import Xe.StringTools;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef _T
#define _T(quote) TEXT(quote) 
#endif

/***************************************************
CUGDropListType - Constructor
	Initialize all member variables
***************************************************/
CUGDropListType::CUGDropListType(CXeUIcolorsIF* pUIcolors)
{
	m_xeUI = pUIcolors;
	//set up the variables
	m_btnWidth = GetSystemMetrics(SM_CXVSCROLL);
	UIScaleFactor sf = m_xeUI->GetUIScaleFactor();
	m_btnWidth = sf.ScaleX(m_btnWidth);

	m_btnDown = FALSE;

	//TRACE("ctor\n");
	m_btnCol = -1;
	m_btnRow = -1;

	// make sure that this cell types do not overlap
	m_canOverLap = FALSE;
}

/***************************************************
GetName  - overloaded CUGCellType::GetName
	Returns a readable name for the cell type.
	Returned value is used to help end-users
	to see what cell type are available.

    **See CUGCellType::GetName for more details
	about this function

Params:
	<none>
Return:
	cell type name
****************************************************/
LPCTSTR CUGDropListType::GetName()
{
	return _T("DropList");
}

/***************************************************
GetUGID  - overloaded CUGCellType::GetUGID
	Returns a GUID for the cell type, this number
	is unique for each cell type and never changes.
	This number can be used to find the cell types
	added to this instance of the Ultimate Grid.

    **See CUGCellType::GetUGID for more details
	about this function

Params:
	<none>
Returns:
	UGID (which is actually a GUID)
****************************************************/
LPCUGID CUGDropListType::GetUGID()
{
	static const UGID ugid = { 0x1667a6a0, 0xf746, 
							0x11d0, 0x9c, 0x7f, 
							0x0, 0x80, 0xc8, 0x3f, 
							0x71, 0x2f };

	return &ugid;
}

/***************************************************
GetEditArea  - overloaded CUGCellType::GetEditArea
	Returns the editable area of a cell type.
	Some celltypes (i.e. drop list) require to
	have certain portion of the cell not covered
	by the edit control.  In case of the drop list
	the drop button should not be covered.

    **See CUGCellType::GetEditArea for more details
	about this function

Params:
	rect - pointer to the cell rectangle, it can
		   be used to modify the edit area
Return:
	UG_SUCCESS or UG_ERROR, currently the Utltimate
	Grid does not check the return value.
****************************************************/
int CUGDropListType::GetEditArea(RECT *rect)
{	
	rect->right -=(m_btnWidth+1);

	return UG_SUCCESS;
}

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
void CUGDropListType::GetBestSize(CSize *size,CUGCell *cell)
{
	CUGCellType::GetBestSize(size, cell );
	// in the case of the drop list celltype we will take the normal
	// width of the text selected, and if the drop button is vible,
	// we will increate the width by the width of the button.
	if (!( cell->GetCellTypeEx()&UGCT_DROPLISTHIDEBUTTON ))
	{
		size->cx += m_btnWidth;
	}
}

/***************************************************
OnDClicked - overloaded CUGCellType::OnDClicked
	This function is called when the left mouse 
	button is double clicked over a cell.
	The drop list cell type determines if the 
	user double clicks on the area that is covered
	by the drop list button, if this is the case than
	this event is passed on to the OnLClicked.

    **See CUGCellType::OnDClicked for more details
	about this function

Params:
	col - column that was clicked in
	row - row that was clicked in
	rect - rectangle of the cell that was clicked in
	point - point where the mouse was clicked
Return:
	TRUE - if the event was processed
	FALSE - if the event was not
****************************************************/
BOOL CUGDropListType::OnDClicked(int col,long row,RECT *rect,POINT *point)
{
	CUGCell cell;
	m_GI->GetCellIndirect(col, row, &cell);
	int style = 0;
	if (cell.IsPropertySet(UGCELL_CELLTYPEEX_SET))
	{
		style = cell.GetCellTypeEx();
	}

	if (point->x > (rect->right - m_btnWidth))
	{
		return OnLClicked(col,row,1,rect,point);
	}
	
	if (style & UGCT_DROPLIST_DBLCLICK_STARTDROP)
	{
		m_btnDown = TRUE;
		m_btnCol = col;
		m_btnRow = row;
		StartDropList();
		return TRUE;
	}
	return FALSE;
}

/***************************************************
OnLClicked - overloaded CUGCellType::OnLClicked
	This function is called when the left mouse 
	button is clicked over a cell.
	The drop list cell type uses this event to
	show the drop list control when the user
	clicks on the drop list button.
	The list control will also be hiden when
	user clicks on the same cell while it is
	showing.
   	
	**See CUGCellType::OnLClicked for more details
	about this function

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
BOOL CUGDropListType::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point)
{
	//TRACE("OnLClicked - col %d, row %d, l %d, t %d, r %d, b %d, x %d, y %d\n",
	//	col, row, rect->left, rect->top, rect->right, rect->bottom, point->x, point->y);
	if( m_GI->m_multiSelectFlag > 0 && ( GetKeyState( VK_SHIFT ) < 0 || GetKeyState( VK_CONTROL ) < 0 ))
		// The drop list should not process mouse click events when
		// user selects cells.
		return TRUE;

	if(updn)
	{
		//TRACE("updn TRUE - m_btnCol=%d, m_btnRow=%d\n", m_btnCol, m_btnRow);
		if(point->x > (rect->right - m_btnWidth))
		{			
			//TRACE("in drp dn btn\n");
			if(col == m_btnCol && row == m_btnRow)
			{
				//TRACE("col,row == m_btnCol.Row\n");
				m_btnCol = -1;
				m_btnRow = -1;			
			}
			else
			{
				//TRACE("col,row != m_btnCol.Row\n");
				if (m_GI->GetCurrentCol() != col || m_GI->GetCurrentRow() != row)
				{
					return FALSE;	// Don't show droplist if not current cell.
				}
				//copy the droplist button co-ords
				CopyRect(&m_btnRect,rect);
				m_btnRect.left = rect->right - m_btnWidth;
			
				//redraw the button
				m_btnDown = TRUE;
				m_btnCol = col;
				m_btnRow = row;
				m_GI->RedrawCell(m_btnCol,m_btnRow);

				//start the drop list
				StartDropList();
				return TRUE;
			}
		}
		else if(m_btnCol ==-2)
		{
			//TRACE("m_btnCol ==-2\n");
			m_btnCol = -1;
			m_btnRow = -1;			
			return FALSE;
		}
	}
	else if(m_btnDown)
	{
		//TRACE("updn FALSE\n");
		m_btnDown = FALSE;
		m_GI->RedrawCell(col,row);
		return TRUE;
	}

	return FALSE;
}

/***************************************************
OnMouseMove - overloaded CUGCellType::OnMouseMove
	This function is called when the mouse  is over
	a cell of this celltype.
	The Drop list cell type uses this notification
	to visually represent to the user that the left
	mouse button was pressed over the button area,
	but it now has been moved on and the list will
	not be shown if the button is released.

   	**See CUGCellType::OnMouseMove for more details
	about this function

Params:
	col - column that the mouse is over
	row - row that the mouse is over
	point - point where the mouse is
	flags - mouse move flags (see WM_MOUSEMOVE)
Return:
	TRUE - if the event was processed
	FALSE - if the event was not
***************************************************/
BOOL CUGDropListType::OnMouseMove(int col,long row,POINT *point,UINT flags)
{	
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);

	if((flags&MK_LBUTTON) == FALSE)
		return TRUE;

	if(point->x >= m_btnRect.left && point->x <= m_btnRect.right)
	{
		if(point->y >= m_btnRect.top && point->y <= m_btnRect.bottom)
		{
			if(!m_btnDown)
			{
				m_btnDown = TRUE;
				m_GI->RedrawCell(m_btnCol,m_btnRow);
			}
		}
		else if(m_btnDown)
		{
			m_btnDown = FALSE;
			m_GI->RedrawCell(m_btnCol,m_btnRow);
		}
	}
	else if(m_btnDown)
	{
		m_btnDown = FALSE;
		m_GI->RedrawCell(m_btnCol,m_btnRow);
	}

	return FALSE;
}

/***************************************************
OnKillFocus - overloaded CUGCellType::OnKillFocus
	This function is called when a the drop list cell type
	looses focus.

   	**See CUGCellType::OnKillFocus for more details
	about this function

Params:
	col - column that just lost focus
	row - row that just lost focus
	cell - pointer to the cell object located at col/row
Return:
	<none>
****************************************************/
void CUGDropListType::OnKillFocus(int col,long row,CUGCell *cell)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(cell);

	//TRACE("OnKillFocus\n");
	m_btnCol = -1;
	m_btnRow = -1;
}

/***************************************************
OnKeyDown - overloaded CUGCellType::OnKeyDown
	This function is called when a cell of this type
	has focus and a key is pressed.(See WM_KEYDOWN)
	The drop list cell type allows the user to show
	the list with either an Enter key or the CTRL-Down
	arrow combination.

  	**See CUGCellType::OnKeyDown for more details
	about this function

Params:
	col - column that has focus
	row - row that has focus
	vcKey - pointer to the virtual key code, of the key that was pressed
Return:
	TRUE - if the event was processed
	FALSE - if the event was not
****************************************************/
BOOL CUGDropListType::OnKeyDown(int col,long row,UINT *vcKey)
{	
	if((*vcKey==VK_RETURN) || ((*vcKey==VK_DOWN)&&(GetKeyState(VK_CONTROL) < 0)))
	{
		//TRACE("OnKeyDown\n");
		m_btnCol = col;
		m_btnRow = row;
		StartDropList();
		*vcKey =0;
		return TRUE;
	}

	return FALSE;
}

/***************************************************
OnCharDown  - overloaded CUGCellType::OnCharDown
	The drop list only handles this notification to inform the 
	grid that the enter and down arrow keys were already handled.

	**See CUGCellType::OnCharDown for more details
	about this function

Params:
	col - column that has focus
	row - row that has focus
	vcKey - pointer to the virtual key code, of the key that was pressed
Return:
	TRUE - if the event was processed
	FALSE - if the event was not
****************************************************/
BOOL CUGDropListType::OnCharDown(int col,long row,UINT *vcKey)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);

	if(*vcKey==VK_RETURN)
		return TRUE;
	if(*vcKey==VK_DOWN)
		return TRUE;
	return FALSE;
}

/***************************************************
OnDraw - overloaded CUGCellType::OnDraw
	The drop list cell type draws its text using the
	base classes DrawText routine, plus draws the
	drop down button

    **See CUGCellType::OnDraw for more details
	about this function
Params
	dc		- device context to draw the cell with
	rect	- rectangle to draw the cell in
	col		- column that is being drawn
	row		- row that is being drawn
	cell	- cell that is being drawn
	selected- TRUE if the cell is selected, otherwise FALSE
	current - TRUE if the cell is the current cell, otherwise FALSE
Return
	none
****************************************************/
void CUGDropListType::OnDraw(CXeD2DRenderContext* pRctx, EXE_FONT eFont, RECT *rect,int col,long row,CUGCell *cell,int selected,int current)
{
	ID2D1RenderTarget* pRT = pRctx->m_pCurrentRT;

	int left = rect->left;

	int style = 0;
	if(cell->IsPropertySet(UGCELL_CELLTYPEEX_SET))
		style = cell->GetCellTypeEx();

	//if the cell is not current and hide button is on
	//then dont draw the button
	if(style&UGCT_DROPLISTHIDEBUTTON && !current)
	{
		CUGCellType::OnDraw(pRctx, eFont, rect,col,row,cell,selected,current);
		return;
	}

	rect->left = rect->right - m_btnWidth;

	//CXeUIhelper uiHlp(m_xeUI, *dc, true);
	//uiHlp.DrawComboBoxButton(CRect(rect), CID::GrdCellDefTxt, CID::GrdCellDefBg);
	pRctx->_DrawComboBoxButton(pRT, RectFfromRect(CRect(rect)), CID::GrdCellDefTxt, CID::GrdCellDefBg);

	//draw the text in using the default drawing routine
	rect->left = left;
	rect->right -= (m_btnWidth);

	DrawText(pRctx, eFont, rect,0,col,row,cell,selected,current);
}

/***************************************************
StartDropList
	This internal function is used to parse out 
	the label string, which contains the items to 
	show in the list.  Also this function will
	show the list box in the proper location on the
	screen.
Params:
	<none>
Return:
	UG_ERROR	- the list is already visible.
	UG_SUCCESS	- everything went smoothly, the drop
				  list was shown
***************************************************/
int CUGDropListType::StartDropList()
{
	CUGCell cell;
	m_GI->GetCellIndirect(m_btnCol,m_btnRow,&cell);

	// make sure that the read only cells do not show droplist
	if ( cell.GetReadOnly() == TRUE )
		return UG_SUCCESS;

	//get the current row and col
	//m_btnCol  = m_ctrl->GetCurrentCol();
	//m_btnRow = m_ctrl->GetCurrentRow();
	//TRACE("StartDropList - m_btnCol=%d, m_btnRow=%d\n", m_btnCol, m_btnRow);

	//notify the user of the list, so it can be modified if needed
	std::vector<ListBoxExItem> list;
	if ( OnCellTypeNotify(m_ID,m_btnCol,m_btnRow,UGCT_DROPLISTSTART,(long long)&list) == FALSE )
		// if FALSE was returned from OnCellTypeNotify call, than the developer does not
		// wish to show the drop list at the moment.
		return UG_ERROR;

	LOGFONT		lf;
	// set default font height
	lf.lfHeight = m_GI->m_xeUI->GetFontMetric(EXE_FONT::eUI_Font).GetHeight();

	//get the font
	EXE_FONT font = EXE_FONT::eUI_Font;
	if(cell.IsPropertySet(UGCELL_FONT_SET))
		font = cell.GetFont();

	//if(font != NULL)
	//{
	//	//find the height of the font
	//	HFONT cfont = font;
	//	cfont->GetLogFont(&lf); 	// lf.lfHeight

	//	if( lf.lfHeight < 0 ){
	//		HDC hDC = CreateCompatibleDC(NULL);
	//		lf.lfHeight = -MulDiv(lf.lfHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	//		DeleteDC(hDC);
	//	}
	//}

	//get the rectangle for the listbox
	CRect rect;
	m_GI->GetCellRect(m_btnCol,m_btnRow,rect);
	CRect rectCell(rect);
	rect.top = rect.bottom;
	rect.left+=10;
	rect.right+=10;
	int len = (int)list.size();
	if(len >15)
		len = 15;
	rect.bottom += lf.lfHeight * len + 6;

	// Increase H width if needed - but not by more that 50% of host cell width.
	std::vector<std::wstring> texts;
	bool hasIcons = false;
	for (const ListBoxExItem& item : list)
	{
		if (item.m_item_data.m_hasIcon) { hasIcons = true; }
		texts.push_back(item.m_string);
	}
	uint32_t cxMax = m_xeUI->GetMaxTextWidthUsingFontW(texts);
	// Add a little extra H space when list has icons.
	if (hasIcons) { cxMax += m_xeUI->GetUIScaleFactor().ScaleX(24); }
	cxMax += m_xeUI->GetUIScaleFactor().ScaleX(12);	// H margin
	if (rect.Width() > 0 && rect.Width() < (int)cxMax)
	{
		rect.right = rect.left + cxMax;
	}

	// create the list control
	std::unique_ptr<CXeListBoxExCommon> listBox = std::make_unique<CXeListBoxExCommon>(m_xeUI);
	listBox->Initialize(LBType::DropDown,
			[this](WORD nfCode, int item_idx) { OnLBnotify(nfCode, item_idx); });
	m_listBox = listBox.get();

	// add the items to the list
	listBox->SetItemsList(list);

	if (list.size() && list[0].m_item_data.m_isCheckbox)
	{
		listBox->SetCheckBoxesMode(true, false, false);
	}

	CRect rcListbox(rect);
	//m_GI->m_gridWnd->ClientToScreen(rcListbox);
	::ClientToScreen(m_GI->m_gridWnd, (LPPOINT)&rcListbox);
	::ClientToScreen(m_GI->m_gridWnd, ((LPPOINT)&rcListbox) + 1);
	//m_GI->m_gridWnd->ClientToScreen(rectCell);
	::ClientToScreen(m_GI->m_gridWnd, (LPPOINT)&rectCell);
	::ClientToScreen(m_GI->m_gridWnd, ((LPPOINT)&rectCell) + 1);
	CXePopupCtrl popup(m_xeUI, XeShowPopup::FadeIn80);
	popup.SetPopupPointer(listBox.get());
	popup.ShowPopup(m_GI->m_gridWnd, rcListbox, rectCell);
	m_listBox = nullptr;

	m_btnDown = FALSE;
	m_GI->RedrawCell(m_btnCol,m_btnRow);

	return UG_SUCCESS;
}

void CUGDropListType::OnLBnotify(WORD nfCode, int item_idx)
{
	XeASSERT(m_listBox);
	if (!m_listBox) { return; }
	bool hasUserSelected = false;
	if (nfCode == LBN_SELCHANGE)
	{
		hasUserSelected = true;
		//TRACE("LBN_SELCHANGE\n");
	}
	else if (nfCode == LBN_DBLCLK)
	{
		hasUserSelected = true;
		//TRACE("LBN_DBLCLK\n");
	}
	else if (nfCode == CBN_DROPDOWN || nfCode == CBN_CLOSEUP)
	{
		if (nfCode == CBN_CLOSEUP && m_listBox->m_isSelEndOk)
		{
			hasUserSelected = true;
		}
		//TRACE("CBN_DROPDOWN || CBN_CLOSEUP\n");
	}
	else if (nfCode == CBN_SELENDCANCEL)
	{
		//TRACE("CBN_SELENDCANCEL\n");
	}
	else if (nfCode == LB_EX_NOTIFY_ITEM_CHECKED)
	{
		//TRACE("LB_EX_NOTIFY_ITEM_CHECKED\n");
		const ListBoxExItem* pItem = m_listBox->GetItemDataAtIndexConst(item_idx);
		OnCellTypeNotify(m_ID, m_btnCol, m_btnRow,
			UGCT_DROPLIST_CHECKBOX_CHECKED, (long long)pItem);
		m_GI->RedrawCell(m_btnCol, m_btnRow);
	}

	CXeListBoxExCommon& cmn = *m_listBox;
	LRESULT curSel = cmn.OnGetCurSelMsg(0, 0);
	LRESULT txtLen = cmn.OnGetTextLenMsg(curSel, 0);
	if (!hasUserSelected || curSel == LB_ERR || txtLen == LB_ERR)
	{
		return;
	}

	CUGCell cell;
	// make sure that the cell is not read only before making
	// user's selection permanent
	m_GI->GetCellIndirect(m_btnCol, m_btnRow, &cell);
	if (cell.GetReadOnly() == FALSE)
	{
		//get the string selected
		std::wstring string;
		string.resize(txtLen);
		LRESULT res = cmn.OnGetTextMsg(curSel, (LPARAM)string.data());

		// send notification with index of the selected item
		OnCellTypeNotify(m_ID, m_btnCol, m_btnRow,
			UGCT_DROPLISTSELECTEX, (long long)curSel);

		//notify the user of the selection
		if (OnCellTypeNotify(m_ID, m_btnCol, m_btnRow, //set the id
			UGCT_DROPLISTSELECT, (long long)&string) != FALSE) {

			m_GI->GetCellIndirect(m_btnCol, m_btnRow, &cell);
			cell.SetText(string.c_str());
			m_GI->SetCell(m_btnCol, m_btnRow, &cell);

			// notify the user that the selection was set
			m_GI->OnCellTypeNotify(m_ID, m_btnCol, m_btnRow, UGCT_DROPLISTPOSTSELECT, (long long)&string);

			m_GI->RedrawCell(m_btnCol, m_btnRow);
		}
	}
	m_btnCol = -1;
	m_btnRow = -1;

	::SetFocus(m_GI->m_gridWnd);
}

