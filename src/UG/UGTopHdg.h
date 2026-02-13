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
#ifndef _UGTopHdg_H_
#define _UGTopHdg_H_
#include "ugcell.h"
#include "UgDrwHnt.h"

import Xe.D2DWndBase;
import Xe.FileTimeX;

class CUGGridInfo;

class  CUGTopHdg : public CXeD2DWndBase
{
// Construction
public:
	CUGGridInfo* m_GI;		//pointer to the grid information

	CUGTopHdg(CXeUIcolorsIF* pUIcolors);

	bool CreateTopHdg(DWORD dwStyle, const CRect& rect, HWND hParentWnd, UINT nID);

	// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUGTopHdg)
	protected:
	//virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUGTopHdg();

	// Generated message map functions
protected:
	//{{AFX_MSG(CUGTopHdg)
	//afx_msg void OnPaint();
	//afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
	//afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	//DECLARE_MESSAGE_MAP()

	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override;
	//virtual LRESULT _OnCreate(CREATESTRUCTW* pCS) override;
	//virtual LRESULT _OnDestroy() override;
	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override;
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnRightDown(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnRightUp(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam) override;

	BOOL ToolTipNeedText( UINT id, NMHDR* pTTTStruct, LRESULT* pResult );
	//virtual INT_PTR OnToolHitTest( CPoint point, TOOLINFO *pTI ) const;

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

	//void DrawCellsIntern(CDC *dc);
	void DrawCellsIntern(CXeD2DRenderContext* pRctx);

	void CheckForUserResize(CPoint *point);

public:
	int GetCellRect(int *col,long *row,RECT *rect);
	int GetCellRect(int col,long row,RECT *rect);
	int GetCellFromPoint(CPoint *point,int *col,int *row,RECT *rect);
	int GetJoinRange(int *col,int *row,int *endCol,int *endRow);

	int GetTHRowHeight(int row);

	void Update();
	void Moved();
};

#endif // _UGTopHdg_H_