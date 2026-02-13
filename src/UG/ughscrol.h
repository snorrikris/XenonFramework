/*************************************************************************
				Class Declaration : CUGHScroll
**************************************************************************
	Source file : ughscrol.cpp
	Header file : ughscrol.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	This class is grid's horizontal scrollbar.

*************************************************************************/
#ifndef _ughscrol_H_
#define _ughscrol_H_

//#include "..\XeScrollBar.h"

import Xe.ScrollBar;
class CUGGridInfo;

class  CUGHScroll : public CXeScrollBar
{
// Construction
public:
	CUGGridInfo* m_GI;			//pointer to the grid information

	CUGHScroll(CXeUIcolorsIF* pUIcolors);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUGHScroll)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUGHScroll();

	// Generated message map functions
protected:
	virtual LRESULT _OnRightDown(UINT nFlags, CPoint point) override;
	//{{AFX_MSG(CUGHScroll)
	//afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
	//afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG

	//DECLARE_MESSAGE_MAP()


protected:
	int	m_lastMaxLeftCol;
	int m_lastNumLockCols;

	int m_trackColPos;

public:

	//internal functions
	void Update();
	void Moved();

	void HScroll(UINT nSBCode, UINT nPos);
};

#endif // _ughscrol_H_
