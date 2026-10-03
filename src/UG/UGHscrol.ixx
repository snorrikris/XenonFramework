module;

/*************************************************************************
				Class Implementation : CUGHScroll
**************************************************************************
	Source file : UGHScrol.cpp
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved
*************************************************************************/
/*************************************************************************
				Class Declaration : CUGHScroll
**************************************************************************
	Source file : ughscrol.cpp
	Header file : ughscrol.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	This class is grid's horizontal scrollbar.

*************************************************************************/

#include "../os_minimal.h"
//#include <afxext.h>         // MFC extensions
#include <string>

#include "ugdefine.h"
//#include "uggdinfo.h"
//#include "ughscrol.h"
// define WM_HELPHITTEST messages
//#include <afxpriv.h>

export module Xe.UGHScrol;

import Xe.UGGridInfoIF;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#pragma NOTE("SKMOD to support CXeScrollBar")
// Base class of 'this' class changed to CXeScrollBar.

import Xe.ScrollBar;

//class CUGGridInfo;

export class CUGHScroll : public CXeScrollBar
{
public:
	CUGGridInfoIF* m_GI;			//pointer to the grid information

protected:
	int	m_lastMaxLeftCol;
	int m_lastNumLockCols;

	int m_trackColPos;

public:
	/***************************************************
		Standard construction/desrtuction
	***************************************************/
	CUGHScroll(CXeUIcolorsIF* pUIcolors) : CXeScrollBar(pUIcolors)
	{
		m_lastMaxLeftCol = -1;
		m_lastNumLockCols = -1;
		m_trackColPos = 0;
	}

	virtual ~CUGHScroll()
	{}

	/********************************************
		Message handlers
	*********************************************/
	//BEGIN_MESSAGE_MAP(CUGHScroll, CXeScrollBar)
	//	//{{AFX_MSG_MAP(CUGHScroll)
	//	ON_WM_RBUTTONDOWN()
	//	ON_WM_CREATE()
	//	ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
	//	ON_WM_HELPINFO()
	//	//}}AFX_MSG_MAP
	//END_MESSAGE_MAP()

	/***************************************************
	Update
		function forces window update.
	Params:
		<none>
	Returns:
		<none>
	*****************************************************/
	void Update()
	{
		Moved();
	}

	/***************************************************
	Moved
		Scroll bar position changed, redraw the grid.
	Params:
		<none>
	Returns:
		<none>
	*****************************************************/
	void Moved()
	{
		if (!m_GI->PaintMode())
			return;

		//update the range if the max left col has changed
		//or if the number of locked columns has changed
		if (m_lastMaxLeftCol != m_GI->MaxLeftCol() || m_lastNumLockCols != m_GI->NumLockCols())
		{
			m_lastMaxLeftCol = m_GI->MaxLeftCol();
			m_lastNumLockCols = m_GI->NumLockCols();

			//set the scroll range
			SCROLLINFO ScrollInfo;
			ScrollInfo.cbSize = sizeof(SCROLLINFO);
			ScrollInfo.fMask = SIF_PAGE | SIF_RANGE;
			ScrollInfo.nPage = (m_GI->GridWidth() - m_GI->LockColWidth()) / m_GI->DefColWidth();
			ScrollInfo.nMin = 0;
			ScrollInfo.nMax = (m_GI->MaxLeftCol() - m_GI->NumLockCols()) + ScrollInfo.nPage - 1;
			SetScrollInfo(&ScrollInfo, FALSE);

			if (m_GI->HScrollRect().top == m_GI->HScrollRect().bottom)
				m_GI->AdjustComponentSizes();
		}

		//set the scroll pos
		if (m_GI->LastLeftCol() != m_GI->LeftCol())
		{
			SetScrollPos(m_GI->LeftCol() - m_GI->NumLockCols(), TRUE);
			m_GI->OnViewMoved(UG_HSCROLL, (long)m_GI->LastLeftCol(), (long)m_GI->LeftCol());
		}

		_RedrawDirectly();
	}

	/***************************************************
	OnHScroll
		The framework calls this member function when the user clicks a window's
		horizontal scroll bar.
	Params:
		nSBCode		- please see MSDN for more information on the parameters.
		nPos
		pScrollBar
	Returns:
		<none>
	*****************************************************/
	void HScroll(UINT nSBCode, UINT nPos)
	{
		if (::GetFocus() != m_GI->GridWnd())
			::SetFocus(m_GI->GridWnd());

		m_GI->SetMoveType(4);

		switch (nSBCode)
		{
		case SB_LINEDOWN:
			m_GI->MoveLeftCol(UG_COLRIGHT);
			break;
		case SB_LINEUP:
			m_GI->MoveLeftCol(UG_COLLEFT);
			break;
		case SB_PAGEUP:
			m_GI->MoveLeftCol(UG_PAGELEFT);
			break;
		case SB_PAGEDOWN:
			m_GI->MoveLeftCol(UG_PAGERIGHT);
			break;
		case SB_TOP:
			m_GI->MoveLeftCol(UG_LEFT);
			break;
		case SB_BOTTOM:
			m_GI->MoveLeftCol(UG_RIGHT);
			break;
		case SB_THUMBTRACK:
			if (m_GI->HScrollMode() == UG_SCROLLTRACKING)	//tracking
				m_GI->SetLeftCol(nPos + m_GI->NumLockCols());

			m_trackColPos = nPos + m_GI->NumLockCols();

			//if enabled then show scroll hints
			//#ifdef UG_ENABLE_SCROLLHINTS
			//	if(m_GI->m_enableHScrollHints)
			//	{
			//		CString string;
			//		RECT rect;
			//		GetWindowRect(&rect);
			//		rect.left = LOWORD(GetMessagePos());
			//		m_ctrl->ScreenToClient(&rect);
			//		m_ctrl->m_CUGHint->SetWindowAlign(UG_ALIGNCENTER|UG_ALIGNBOTTOM);
			//		m_ctrl->m_CUGHint->SetTextAlign(UG_ALIGNCENTER);

			//		m_ctrl->OnHScrollHint(m_trackColPos,&string);
			//		// set text before move window...
			//		m_ctrl->m_CUGHint->SetText(string,FALSE);

			//		m_ctrl->m_CUGHint->MoveHintWindow(rect.left,rect.top-1,20);
			//		m_ctrl->m_CUGHint->Show();				
			//	}
			//#endif // UG_ENABLE_SCROLLHINTS
			break;
		case SB_ENDSCROLL:
			break;
		case SB_THUMBPOSITION:
			//#ifdef UG_ENABLE_SCROLLHINTS
			//if(m_GI->m_enableHScrollHints)
			//{
			//	m_ctrl->m_CUGHint->Hide();				
			//}
			//#endif

			m_GI->SetLeftCol(nPos + m_GI->NumLockCols());

			break;
		}
	}

	/***************************************************
	OnRButtonDown
		checks if the popup menus are enabled, if so that it starts process to
		show popup menu.
	Params:
		nFlags		- please see MSDN for more information on the parameters.
		point
	Returns:
		<none>
	*****************************************************/
	//void OnRButtonDown(UINT nFlags, CPoint point) 
	virtual LRESULT _OnRightDown(UINT nFlags, CPoint point) override
	{
		if (m_GI->IsEnablePopupMenu())
		{
			ClientToScreen(&point);
			m_GI->StartMenu(0, 0, point, UG_HSCROLL);
		}

		//CXeScrollBar::OnRButtonDown(nFlags, point);
		return CXeScrollBar::_OnRightDown(nFlags, point);
	}

	/***************************************************
	OnCreate
		Please see MSDN for more details on this event handler.
		The CUGHScroll control takes this opportunity to check if the scroll
		hints are enabled, if so then it enables its tool tips.
	Params:
		lpCreateStruct	- Points to a CREATESTRUCT structure
						  that contains information about the
						  CWnd object being created.
	Returns:
		OnCreate must return 0 to continue the creation of the
		CWnd object. If the application returns –1, the window
		will be destroyed.
	*****************************************************/
	//int OnCreate(LPCREATESTRUCT lpCreateStruct) 
	//{
	//	if (CXeScrollBar::OnCreate(lpCreateStruct) == -1)
	//		return -1;
	//	
	//	if(m_GI->m_enableHScrollHints)
	//		EnableToolTips(TRUE);
	//	
	//	return 0;
	//}

	/************************************************
	OnHelpHitTest
		Sent as a result of context sensitive help
		being activated (with mouse) over horizontal scroll
	Params:
		WPARAM - not used
		LPARAM - x, y coordinates of the mouse event
	Returns:
		Context help ID to be displayed
	*************************************************/
	//LRESULT OnHelpHitTest(WPARAM, LPARAM)
	//{
	//	return 0;
	//}

	/************************************************
	OnHelpInfo
		Sent as a result of context sensitive help
		being activated (with mouse) over horizontal scroll
		if the grid is on the dialog
	Params:
		HELPINFO - structure that contains information on selected help topic
	Returns:
		TRUE or FALSE to allow further processing of this message
	*************************************************/
	//BOOL OnHelpInfo(HELPINFO* pHelpInfo) 
	//{
	//	return FALSE;
	//}
};

