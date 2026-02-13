module;

#include "os_minimal.h"
#include <memory>
#include <string>
#include "logging.h"

export module Xe.UIWorkThread;

import Xe.Utils;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class LGWTState { Idle, ExecuteWorkThreadWork };
export class CXeUIWorkThread
{
protected:
	VSRL::Logger& logger() { return VSRL::s_pVSRL->GetInstance("CXeUIWorkThread"); }

	HANDLE m_hWorkThread = nullptr;
	int m_nWorkThreadId = 0;
	HANDLE m_hEventStop = nullptr;			// Work thread stop event. ManualReset=TRUE.
	HANDLE m_hEventNeedsWork = nullptr;		// Watch list changed event. ManualReset=TRUE.

	CXeCancelEvent m_cancelEvent;

	LGWTState m_state = LGWTState::Idle;

	WorkThreadWorkCallbackFunc m_wt_callback = nullptr;

public:
	void StartThread()
	{
		logger().debug("StartThread");

		m_hEventStop = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		m_hEventNeedsWork = ::CreateEvent(NULL, TRUE, FALSE, NULL);

		m_hWorkThread = ::CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)CXeUIWorkThread::_SWorkThread, this, 0,
			(LPDWORD) & (m_nWorkThreadId));
	}

	void StopThread()
	{
		logger().debug("StopThread");

		::SetEvent(m_hEventStop);
		if (::WaitForSingleObject(m_hWorkThread, 1000) != WAIT_OBJECT_0)	// Wait for worker thread to end.
		{
			::TerminateThread(m_hWorkThread, 0xFFFFFFFF);
		}

		::CloseHandle(m_hEventNeedsWork);
		::CloseHandle(m_hEventStop);
		::CloseHandle(m_hWorkThread);
	}

	CXeCancelEvent* DoWorkThreadWork(WorkThreadWorkCallbackFunc wt_callback)
	{
		XeASSERT(m_state == LGWTState::Idle && !m_wt_callback);
		m_wt_callback = wt_callback;
		_SetNewWorkState(LGWTState::ExecuteWorkThreadWork);
		m_cancelEvent.ResetCancelEvent();
		::SetEvent(m_hEventNeedsWork);
		return &m_cancelEvent;
	}

	void CancelWork()
	{
		m_cancelEvent.CancelWork();
	}

	CXeCancelEvent* GetCancelEvent()
	{
		return &m_cancelEvent;
	}

protected:
	void _SetNewWorkState(LGWTState state)
	{
		m_state = state;
	}

	static DWORD WINAPI _SWorkThread(LPVOID* pParam)
	{
		CXeUIWorkThread* pThis = (CXeUIWorkThread*)pParam;
		return pThis->_WorkThread();
	}

	DWORD _WorkThread()
	{
		logger().debug("Work thread started");
		//::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
		std::vector<HANDLE> waitHandles({ m_hEventStop, m_hEventNeedsWork });
		int nWaitHandlesCount = (int)waitHandles.size();
		while (true)
		{
			DWORD dw = WaitForMultipleObjects(nWaitHandlesCount, &waitHandles[0], FALSE, INFINITE);
			dw -= WAIT_OBJECT_0;
			if (dw > (DWORD)(nWaitHandlesCount - 1))
				break;	// internal error - should never happen.

			//XeTRACE("LogGridWorkThread :: %s Thread: %d - Wait obj: %d\r\n",
			//	FILETIMEX::UTCnow().ToStr(DATEFMT::ISOplus_mS).c_str(),
			//	GetCurrentThreadId(), dw);

			if (dw == 0)
			{	// Stop event is in signaled state : thread is terminating
				break;
			}
			else if (dw == 1)
			{	// Need work event is in signaled state
				::ResetEvent(m_hEventNeedsWork);
				_OnWorkIsNeeded();
			}
			else
			{
				_ASSERT(FALSE);
			}
		}
		logger().debug("Work thread ends");
		return 0;
	}

	void _OnWorkIsNeeded()
	{
		if (m_state == LGWTState::ExecuteWorkThreadWork)
		{
			XeASSERT(m_wt_callback);
			if (m_wt_callback)
			{
				m_wt_callback();
				m_wt_callback = nullptr;
			}
		}
		m_state = LGWTState::Idle;
	}
};

