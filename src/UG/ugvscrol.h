/*************************************************************************
				Class Declaration : CUGVScroll
**************************************************************************
	Source file : ugvscrol.cpp
	Header file : ugvscrol.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	This class is grid's vertical scrollbar.

*************************************************************************/
#ifndef _ugvscrol_H_
#define _ugvscrol_H_

#include <list>
import Xe.ScrollBar;
class CUGGridInfo;

class  CUGVScroll : public CXeScrollBar
{
// Construction
public:
	CUGGridInfo* m_GI;			//pointer to the grid information

	CUGVScroll(CXeUIcolorsIF* pUIcolors);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUGVScroll)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUGVScroll();

	// Generated message map functions
protected:
	virtual LRESULT _OnRightDown(UINT nFlags, CPoint point) override;
	//{{AFX_MSG(CUGVScroll)
	//afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
	//afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG

	//DECLARE_MESSAGE_MAP()

protected:

	//double			m_multiRange;	//scroll bar multiplication factor
	//								//for setting the scroll range
	//double			m_multiPos;		//multiplication factor for setting the 
	//								//top row during a thumb track
	long			m_lastMaxTopRow;//last max top row

	int				m_lastScrollMode;

	int				m_lastNumLockRow;

	long			m_trackRowPos;

public:

	//internal functions
	void Update();
	void Moved();
	void VScroll(UINT nSBCode, UINT nPos);

};

#endif // _ugvscrol_H_