/*************************************************************************
				Class Declaration : CUGGrid
**************************************************************************
	Source file : UGGrid.cpp
	Header file : UGGrid.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		The CUGGrid takes care of the main grid area,
		this is where all of the cells are drawn and
		most user interaction takes place.
	Details
		- This class handles drawing of all cells,
		  excluding cells on headings.
		- All of the mouse and keyboard events
		  are routed to the CUGCtrl derived class
		  in form of virtual functions (notifications).
*************************************************************************/
#ifndef _UGGrid_H_
#define _UGGrid_H_

#ifndef WM_MOUSEWHEEL
#define ON_WM_MOUSEWHEEL
#endif

#include <memory>
#include "ugdrwhnt.h"

import Xe.D2DWndBase;

//import Xe.XSuperTooltip;
class CUGGridInfo;

class  CUGGrid : public CXeD2DWndBase
{
// Construction
public:
	CUGGrid(CXeUIcolorsIF* pUIcolors);

	bool CreateGrid(DWORD dwStyle, const CRect& rect, HWND hParentWnd, UINT nID);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUGGrid)
protected:
	virtual LRESULT _OnOtherMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	//virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	public:
	//virtual CScrollBar* GetScrollBarCtrl(int nBar) const;
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUGGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CUGGrid)
	//afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//afx_msg void OnDestroy();
	//afx_msg void OnPaint();
	//afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//afx_msg void OnKillFocus(CWnd* pNewWnd);
	//afx_msg void OnSetFocus(CWnd* pOldWnd);
	//afx_msg int  OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	//afx_msg void OnSize(UINT nType, int cx, int cy);
	//afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//afx_msg UINT OnGetDlgCode();
	//afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	//afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
	//afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	
	//#ifdef WM_MOUSEWHEEL
	//	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//#endif
	//afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	//DECLARE_MESSAGE_MAP()

	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rcClient) override;
	virtual LRESULT _OnMouseActivate(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override;
	virtual LRESULT _OnMouseLeave(WPARAM wparam, LPARAM lparam) override;
	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnRightDown(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnRightUp(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint pt) override;
	virtual LRESULT _OnSetCursor(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint pt) override;
	virtual LRESULT _OnMouseHWheel(WORD fwKeys, short zDelta, CPoint pt) override;
	virtual LRESULT _OnSetFocus(HWND hOldWnd) override;
	virtual LRESULT _OnKillFocus(HWND hNewWnd) override;

	virtual LRESULT _OnKeyDown(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT _OnKeyUp(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT _OnChar(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT _OnVScroll(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT _OnHScroll(WPARAM wParam, LPARAM lParam) override;
	virtual LRESULT _OnGetDlgCode(WPARAM wParam, LPARAM lParam) override;

	//afx_msg LRESULT OnMouseLeave(WPARAM wparam, LPARAM lparam);

	virtual LRESULT _OnNotify_NeedTooltip(NM_PPTOOLTIP_NEED_TT* pNeedTT) override;

	//void DrawCellsIntern(CDC *dc,CDC *db_dc);
	void DrawCellsIntern(CXeD2DRenderContext* pRctx);

	// This protected member is used to prevent the grid from un-matched
	// mouse button events.  This member was added in attempt to work around
	// a Windows bug, when user double clicks within a window 4 messages are
	// sent: WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK, WM_LBUTTONUP
	// On many occasions popup windows are closed on the double click event
	// causing the last mouse up message be sent to any window under current
	// mouse cursor.
	BOOL m_bHandledMouseDown;

public:
	CUGGridInfo *	m_GI;			//pointer to the grid information

	//CBitmap *		m_bitmap;		//double buffering
	//int				m_doubleBufferMode;

	CUGCell			m_cell;			//general purpose cell object

	long			m_keyRepeatCount; //key ballistics repeat counter

	int				m_hasFocus;

	BOOL			m_cellTypeCapture;

	BOOL			m_tempDisableFocusRect;

public:

	void Update();
	void Moved();

	void RedrawCell(int col,long row);
	void RedrawRow(long row);
	void RedrawCol(int col);
	void RedrawRange(int startCol,long startRow,int endCol,long endRow);
	void TempDisableFocusRect();

	void HideTooltip() { _HideTooltip(); }
};

#endif // _UGGrid_H_