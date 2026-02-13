/*************************************************************************
				Class Declaration : CUGUrlBtnType
**************************************************************************
	Purpose
		Draws the cells value as text. Plus adds a "e" button on the
		right hand side of the cell. This button when clicked will fire a 
		notifiction (UGCT_URLBUTTONCLICK).

	Details
		Cell type extended style
			UGCT_URLHIDEBUTTON - when set the ellipsis button will not
				be shown unless the cell has focus

		Notifications (OnCellTypeNotify)
			UGCT_URLBUTTONCLICK - sent when the ellipsis button is clicked

*************************************************************************/
#pragma once

//celltype notification
#define UGCT_URLBUTTONCLICK	100

#define UGCT_URLHIDEBUTTON		512
#define UGCT_URLISEMAIL			1024

#include "UGCelTyp.h"

//CUGUrlBtnType declaration
class  CUGUrlBtnType: public CUGCellType
{
protected:

	int		m_btnWidth;
	int		m_btnDown;
	long	m_btnRow;
	int		m_btnCol;
	RECT	m_btnRect;

	HPEN	m_pen;
	HBRUSH	m_brush;

public:

	CUGUrlBtnType();
	~CUGUrlBtnType();
	
	//overloaded CUGCellType functions
	virtual LPCTSTR GetName();
	virtual LPCUGID GetUGID();
	virtual int GetEditArea(RECT *rect);
	virtual BOOL OnLClicked(int col,long row,int updn,RECT *rect,POINT *point);
	virtual BOOL OnDClicked(int col,long row,RECT *rect,POINT *point);
	virtual BOOL OnMouseMove(int col,long row,POINT *point,UINT flags);
	virtual void OnDraw(CXeD2DRenderContext* pRctx, EXE_FONT eFont, RECT *rect,int col,long row,CUGCell *cell,int selected,int current) override;
	virtual void GetBestSize(CSize *size,CUGCell *cell);
};

