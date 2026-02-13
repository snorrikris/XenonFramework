module;

//////////////////////////////////////////////////
// CMonitor - wrapper to Win32 multi-monitor API
//
// Author: Donald Kackman
// Email:  don@itsEngineering.com
// Copyright 2002, Donald Kackman
//
// You may freely use or modify this code provided this
// Copyright is included in all derived versions.
//
///////////////////////////////////////////////////

//
//David Campbell's article
//How to Exploit Multiple Monitor Support in Memphis and Windows NT 5.0
//is very helpful for multimonitor api calls
//http://www.microsoft.com/msj/defaultframe.asp?page=/msj/0697/monitor/monitor.htm&nav=/msj/0697/newnav.htm
//

#include "os_minimal.h"
#include <vector>
#include <string>
#include <algorithm>

export module Xe.Monitors;

import Xe.mfc_types;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export class CMonitor
{
public:
	HMONITOR m_hMonitor;

	std::wstring GetName() const
	{
		XeASSERT(IsMonitor());

		MONITORINFOEX mi;
		mi.cbSize = sizeof(mi);
		::GetMonitorInfo(m_hMonitor, &mi);

		std::wstring s(mi.szDevice);
		return s;
	}

	// these methods return true if any part of the item intersects the monitor rect
	BOOL IsOnMonitor(const POINT pt) const
	{
		CRect rect;
		GetMonitorRect(rect);

		return rect.PtInRect(pt);
	}

	BOOL IsOnMonitor(HWND hWnd) const
	{
		CRect rect;
		GetMonitorRect(rect);

		XeASSERT(::IsWindow(hWnd));
		CRect wndRect;
		::GetWindowRect(hWnd, &wndRect);

		return rect.IntersectRect(rect, wndRect);
	}

	BOOL IsOnMonitor(const LPRECT lprc) const
	{
		CRect rect;
		GetMonitorRect(rect);

		return rect.IntersectRect(rect, lprc);
	}


	void GetMonitorRect(LPRECT lprc) const
	{
		XeASSERT(IsMonitor());

		MONITORINFO mi;
		RECT        rc;

		mi.cbSize = sizeof(mi);
		::GetMonitorInfo(m_hMonitor, &mi);
		rc = mi.rcMonitor;

		::SetRect(lprc, rc.left, rc.top, rc.right, rc.bottom);
	}

	// the work area does not include the start bar
	void GetWorkAreaRect(LPRECT lprc) const
	{
		XeASSERT(IsMonitor());

		MONITORINFO mi;
		RECT        rc;

		mi.cbSize = sizeof(mi);
		::GetMonitorInfo(m_hMonitor, &mi);
		rc = mi.rcWork;

		::SetRect(lprc, rc.left, rc.top, rc.right, rc.bottom);
	}

	//these two center methods are adapted from David Campbell's
	//MSJ article (see comment at the top of the header file)
	void CenterRectToMonitor(LPRECT lprc, const BOOL UseWorkAreaRect) const
	{
		int  w = lprc->right - lprc->left;
		int  h = lprc->bottom - lprc->top;

		CRect rect;
		if (UseWorkAreaRect)
			GetWorkAreaRect(&rect);
		else
			GetMonitorRect(&rect);

		lprc->left = rect.left + (rect.Width() - w) / 2;
		lprc->top = rect.top + (rect.Height() - h) / 2;
		lprc->right = lprc->left + w;
		lprc->bottom = lprc->top + h;
	}

	void CenterWindowToMonitor(HWND hWnd, const BOOL UseWorkAreaRect) const
	{
		XeASSERT(IsMonitor());
		XeASSERT(hWnd);
		XeASSERT(::IsWindow(hWnd));

		CRect rect;
		::GetWindowRect(hWnd, &rect);
		CenterRectToMonitor(&rect, UseWorkAreaRect);
		::SetWindowPos(hWnd, NULL, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	void ClipRectToMonitor(LPRECT lprc, const BOOL UseWorkAreaRect) const
	{
		int w = lprc->right - lprc->left;
		int h = lprc->bottom - lprc->top;

		CRect rect;
		if (UseWorkAreaRect)
			GetWorkAreaRect(&rect);
		else
			GetMonitorRect(&rect);

		lprc->left = std::max(rect.left, std::min(rect.right - w, lprc->left));
		lprc->top = std::max(rect.top, std::min(rect.bottom - h, lprc->top));
		lprc->right = lprc->left + w;
		lprc->bottom = lprc->top + h;
	}

	//
	// is the instance the primary monitor
	BOOL IsPrimaryMonitor() const
	{
		XeASSERT(IsMonitor());

		MONITORINFO mi;

		mi.cbSize = sizeof(mi);
		::GetMonitorInfo(m_hMonitor, &mi);

		return mi.dwFlags == MONITORINFOF_PRIMARY;
	}

	//
	// is the instance currently attached to a valid monitor handle
	BOOL IsMonitor() const;
};

export class CMonitors
{
public:
	std::vector<CMonitor> m_MonitorArray;

	typedef struct tagMATCHMONITOR
	{
		HMONITOR target;
		BOOL foundMatch;
	} MATCHMONITOR, * LPMATCHMONITOR;

	static BOOL CALLBACK FindMatchingMonitorHandle(
		HMONITOR hMonitor,  // handle to display monitor
		HDC hdcMonitor,     // handle to monitor DC
		LPRECT lprcMonitor, // monitor intersection rectangle
		LPARAM dwData       // data
	);



#if 0  // -----------------------------------------------------------
	static BOOL CALLBACK AddMonitorsCallBack(
		HMONITOR hMonitor,  // handle to display monitor
		HDC hdcMonitor,     // handle to monitor DC
		LPRECT lprcMonitor, // monitor intersection rectangle
		LPARAM dwData       // data
	);
#endif // -----------------------------------------------------------

	static BOOL CALLBACK /*CMonitors::*/ AddMonitorsCallBack(HMONITOR hMonitor,
		HDC /*hdcMonitor*/,
		LPRECT /*lprcMonitor*/,
		LPARAM dwData)
	{
		//LPADDMONITOR pAddMonitor = (LPADDMONITOR)dwData;
		CMonitors* pThis = (CMonitors*)dwData;

		CMonitor mon;
		mon.m_hMonitor = hMonitor;
		pThis->m_MonitorArray.push_back(mon);
		//CMonitor* pMonitor = new CMonitor;
		//pMonitor->Attach( hMonitor );

		//CString str;
		//str.Format("currentIndex=%d  GetSize=%d", pAddMonitor->currentIndex, pAddMonitor->pMonitors->GetSize());
		//AfxMessageBox(str);
		//XeASSERT((pAddMonitor->currentIndex >= 0) && (pAddMonitor->currentIndex < pAddMonitor->pMonitors->GetSize()));

		//if (pAddMonitor->currentIndex < pAddMonitor->pMonitors->GetSize())
		//{
		//	pAddMonitor->pMonitors->SetAt( pAddMonitor->currentIndex, pMonitor );
		//	pAddMonitor->currentIndex++;
		//}

		return TRUE;
	}

	CMonitors()
	{
		//m_MonitorArray.SetSize( ::GetSystemMetrics(SM_CMONITORS) );
		//CString str;
		//str.Format("GetSize=%d  mons=%d", m_MonitorArray.GetSize(), ::GetSystemMetrics(SM_CMONITORS));
		//AfxMessageBox(str);

		//ADDMONITOR addMonitor;
		//addMonitor.pMonitors = &m_MonitorArray;
		//addMonitor.currentIndex = 0;

		::EnumDisplayMonitors(NULL, NULL, AddMonitorsCallBack, (LPARAM)this);
	}

	int GetCount() const { return (int)m_MonitorArray.size(); }

	//CMonitors::~CMonitors()
	//{
	//	for ( int i = 0; i < m_MonitorArray.GetSize(); i++ )
	//		delete m_MonitorArray.GetAt( i );
	//}


	// CMonitors member functions

	//
	// returns the primary monitor
	static CMonitor GetPrimaryMonitor()
	{
		//the primary monitor always has its origin at 0,0
		HMONITOR hMonitor = ::MonitorFromPoint(CPoint(0, 0), MONITOR_DEFAULTTOPRIMARY);
		XeASSERT(IsMonitor(hMonitor));

		CMonitor monitor;
		monitor.m_hMonitor = hMonitor;
		//monitor.Attach( hMonitor );
		XeASSERT(monitor.IsPrimaryMonitor());

		return monitor;
	}

	//
	// is the given handle a valid monitor handle
	static BOOL IsMonitor(const HMONITOR hMonitor)
	{
		if (hMonitor == NULL)
			return FALSE;

		MATCHMONITOR match;
		match.target = hMonitor;
		match.foundMatch = FALSE;

		::EnumDisplayMonitors(NULL, NULL, FindMatchingMonitorHandle, (LPARAM)&match);

		return match.foundMatch;
	}

	static BOOL AllMonitorsShareDisplayFormat()
	{
		return ::GetSystemMetrics(SM_SAMEDISPLAYFORMAT);
	}

	//
	// the number of monitors on the system
	static int GetMonitorCount()
	{
		return ::GetSystemMetrics(SM_CMONITORS);
	}


	//CMonitor CMonitors::GetMonitor( const int index ) const
	//{
	//#if _MFC_VER >= 0x0700
	//	XeASSERT( index >= 0 && index < m_MonitorArray.GetCount() ); 
	//#else
	//	XeASSERT( index >= 0 && index < m_MonitorArray.GetSize() );
	//#endif
	//
	//	CMonitor* pMonitor = (CMonitor*)m_MonitorArray.GetAt( index );
	//
	//	return *pMonitor;
	//}

	//
	// returns the rectangle that is the union of all active monitors
	static void GetVirtualDesktopRect(LPRECT lprc)
	{
		::SetRect(lprc,
			::GetSystemMetrics(SM_XVIRTUALSCREEN),
			::GetSystemMetrics(SM_YVIRTUALSCREEN),
			::GetSystemMetrics(SM_CXVIRTUALSCREEN),
			::GetSystemMetrics(SM_CYVIRTUALSCREEN));

	}

	//
	// these methods determine wheter the given item is
	// visible on any monitor
	static BOOL IsOnScreen(const LPRECT lprc)
	{
		return ::MonitorFromRect(lprc, MONITOR_DEFAULTTONULL) != NULL;
	}

	static BOOL IsOnScreen(const POINT pt)
	{
		return ::MonitorFromPoint(pt, MONITOR_DEFAULTTONULL) != NULL;
	}

	static BOOL IsOnScreen(HWND hWnd)
	{
		return ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL) != NULL;
	}

	static CMonitor GetNearestMonitor(const LPRECT lprc)
	{
		CMonitor monitor;
		monitor.m_hMonitor = ::MonitorFromRect(lprc, MONITOR_DEFAULTTONEAREST);
		return monitor;
	}

	static CMonitor GetNearestMonitor(const POINT pt)
	{
		CMonitor monitor;
		monitor.m_hMonitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
		return monitor;
	}

	static CMonitor GetNearestMonitor(HWND hWnd)
	{
		XeASSERT(hWnd);
		XeASSERT(::IsWindow(hWnd));

		CMonitor monitor;
		monitor.m_hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		return monitor;
	}
};

BOOL CMonitor::IsMonitor() const
{
	return CMonitors::IsMonitor(m_hMonitor);
}

//this is the callback method that gets called via IsMontior
BOOL CALLBACK CMonitors::FindMatchingMonitorHandle(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	LPMATCHMONITOR pMatch = (LPMATCHMONITOR)dwData;

	if (hMonitor == pMatch->target)
	{
		//found a monitor with the same handle we are looking for		
		pMatch->foundMatch = TRUE;
		return FALSE; //stop enumerating
	}

	//haven't found a match yet
	pMatch->foundMatch = FALSE;
	return TRUE;	//keep enumerating
}

