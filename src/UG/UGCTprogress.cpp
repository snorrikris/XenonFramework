
#include "../os_minimal.h"
#include <string>
#include <d2d1.h>

import Xe.UIcolorsIF;
#include "ugdefine.h"
#include "uggdinfo.h"
#include "UGCTprogress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef _T
#define _T(quote) TEXT(quote) 
#endif

CUGProgressType::CUGProgressType()
{	
	// make sure that user cannot start edit this cell type
	m_canTextEdit =	FALSE;
}

CUGProgressType::~CUGProgressType()
{
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
LPCTSTR CUGProgressType::GetName()
{
	return _T("Progress indicator");
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
LPCUGID CUGProgressType::GetUGID()
{
	static const UGID ugid = { 0xf5c12593, 0xd930, 0x11d6, 
	{ 0x9b, 0x37, 0x0, 0x51, 0xba, 0xd5, 0x4b, 0xcc } };

	return &ugid;
}


/***************************************************
OnDraw - overloaded CUGCellType::OnDraw
	The arrow cell type uses this function to
	properly draw itself within cell's area
	respecting the extended cell type styles
	specified, using text color as the color
	which should be used to draw arrows.

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
	<none>
****************************************************/
void CUGProgressType::OnDraw(CXeD2DRenderContext* pRctx, EXE_FONT eFont, RECT *rect,int col,long row,CUGCell *cell,int selected,int current)
{
//#ifdef UG_ENABLE_PRINTING
//	// If it is printing, call OnDrawPrint to draw the cell 
//	if (dc->IsPrinting()) {
//		OnDrawPrint(dc, rect, col, row, cell, selected, current);
//		return;
//	}
//#endif

	DrawBorder(pRctx, rect, rect, cell);
	CUGCellType::DrawBackground(pRctx, rect, cell->GetBackColor());

	long progress = cell->GetParam();
	if (progress < 0)
	{
		progress = 0;
	}
	else if (progress > 100)
	{
		progress = 100;
	}

	COLORREF rgbProgressBg = cell->GetHBackColor();

	CRect rc = rect;
	int nWidth = rc.Width();
	int nProgressWidth = progress >= 100 ? nWidth : (nWidth / 100) * progress;
	rc.right = rc.left + nProgressWidth;

	ID2D1RenderTarget* pRT = pRctx->m_pCurrentRT;
	//CBrush brush(rgbProgressBg);
	//dc->FillRect(rc, &brush);
	pRT->FillRectangle(RectFfromRect(rc), pRctx->GetBrush(CID::LogFilterHdrBg));

	//ASSERT(!m_GI->m_enableCellOverLap);	// Not supported in this cell type.

	//dc->SetBkMode(TRANSPARENT);
	//DrawText(dc, rect, 0, col, row, cell, selected, current);
	DrawText(pRctx, eFont, rect, 0, col, row, cell, selected, current);
}

void CUGProgressType::DrawBackground(CXeD2DRenderContext* pRctx, RECT* rect, COLORREF backcolor)
{
}

