/*************************************************************************
				Class Declaration : CUGSideHdg
**************************************************************************
	Source file : ugsidehd.cpp
	Header file : ugsidehd.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		The side heading (CUGSideHdg) object/window
		is responsible to draw cells and handle
		user's actions on the rows heading.

	Keay features:
		- This class provides ability to resize
		  rows height with the mouse.
		- as well as the width of the entire
		  side heading and columns it contains
		- mouse and keyboard messages are forwarded
		  to the CUGCtrl class as notifications.
		  OnSH_...

*************************************************************************/
#ifndef _ugsidehd_H_
#define _ugsidehd_H_
#include <memory>
#include "UGCell.h"
#include "UGDrwHnt.h"

import Xe.D2DWndBase;

class CUGGridInfo;

class  CUGSideHdg : public CXeD2DWndBase
{
public:
	CUGGridInfo* m_GI;			//pointer to the grid information

	CUGSideHdg(CXeUIcolorsIF* pUIcolors);

	bool CreateSideHdg(DWORD dwStyle, const CRect& rect, HWND hParentWnd, UINT nID);

public:
	virtual ~CUGSideHdg();

	void RedrawAll();

protected:

	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override;
	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override;
	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override;
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnRightDown(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnRightUp(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam) override;

	virtual LRESULT _OnNotify_NeedTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT) override;

protected:
	CUGCell			m_cell;			//general purpose cell class

	long			m_bottomRow;	//the row drawn at the bottom

	int				m_isSizing;			//sizing flag
	int				m_canSize;			//sizing flag
	int				m_colOrRowSizing;	// 0 - col 1- row
	long			m_sizingColRow;		// column or row number being sized
	int				m_sizingStartSize;	// original col/row width/height
	int				m_sizingStartPos;	// original starting mouse position
	int				m_sizingStartWidth;//original side heading total width
	int				m_sizingNumRowsDown; //number of rows from top when size started

	RECT			m_focusRect;		//focus rect for column sizing option

	void DrawCellsIntern(CXeD2DRenderContext* pRctx, D2D1_RECT_F rcClient);

public:
	int GetCellRect(int *col,long *row,RECT *rect);
	int GetCellRect(int col,long row,RECT *rect);
	int GetCellFromPoint(CPoint *point,int *col,long *row,RECT *rect);

	int GetSHColWidth(int col);

	void Update();
	void Moved();
	void HideTooltip() { _HideTooltip(); }
};

#endif // _ugsidehd_H_