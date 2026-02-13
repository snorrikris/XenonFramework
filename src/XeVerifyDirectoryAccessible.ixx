module;

#include "os_minimal.h"
#include <string>
#include <vector>

export module Xe.VerifyDirectoryAccessible;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CVDA_WT
{
protected:
	std::wstring m_strDirectory;

	HANDLE m_hWorkThread = nullptr;
	int m_nWorkThreadId = 0;
	HANDLE m_hEventDirIsOk = nullptr;

public:
	CVDA_WT() = default;
	//~CVDA_WT();

	bool IsDirAccessible(const std::wstring& strDirectory)
	{
		m_strDirectory = strDirectory;

		m_hEventDirIsOk = CreateEvent(NULL, TRUE, FALSE, NULL);

		m_hWorkThread = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)CVDA_WT::_SWorkThread, this, 0,
			(LPDWORD) & (m_nWorkThreadId));

		DWORD dwWaitResult = WaitForSingleObject(m_hEventDirIsOk, 300);
		if (dwWaitResult == WAIT_OBJECT_0)
		{
			// Directory was accessible. Worker thread has ended.
			CloseHandle(m_hEventDirIsOk);
			m_hEventDirIsOk = nullptr;
			CloseHandle(m_hWorkThread);
			m_hWorkThread = nullptr;
			return true;
		}
		return false;
	}

	bool DoCleanUp(bool killThread)
	{
		bool isClosed = false;
		if (m_hWorkThread)
		{
			DWORD dwExitCode;
			GetExitCodeThread(m_hWorkThread, &dwExitCode);
			if (dwExitCode != STILL_ACTIVE)
			{
				isClosed = true;
			}
			else
			{
				if (killThread)
				{
					TerminateThread(m_hWorkThread, 1);
					isClosed = true;
				}
			}

			if (isClosed)
			{
				CloseHandle(m_hEventDirIsOk);
				m_hEventDirIsOk = nullptr;
				CloseHandle(m_hWorkThread);
				m_hWorkThread = nullptr;
			}
		}
		else
		{
			isClosed = true;
		}
		return isClosed;
	}

private:
	static DWORD __stdcall _SWorkThread(LPVOID* pParam)
	{
		CVDA_WT* pThis = (CVDA_WT*)pParam;
		return pThis->_WorkThread();
	}

	DWORD _WorkThread()
	{
		DWORD dwAttrib = ::GetFileAttributesW(m_strDirectory.c_str());

		if (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
		{
			SetEvent(m_hEventDirIsOk);
			return 0;	// Set no error exit code.
		}
		return 1;	// Set error exit code.
	}
};

export class CVerifyDirectoryAccessible
{
protected:
	std::vector<CVDA_WT> m_threadList;

public:
	CVerifyDirectoryAccessible() = default;
	~CVerifyDirectoryAccessible()
	{
		// kill any remaining threads
		_DoCleanUp(true);
	}

	bool IsAccessible(const std::wstring& strDirectory)
	{
		_DoCleanUp(false);	// Do a quick cleanup of old threads that have ended.

		CVDA_WT vda;
		bool result = vda.IsDirAccessible(strDirectory);
		if (!result)
		{
			// Add to thread list - cleanup later (dtor)
			m_threadList.push_back(vda);
		}
		return result;
	}

private:
	void _DoCleanUp(bool killThreads)
	{
		for (auto it = m_threadList.begin(); it != m_threadList.end(); )
		{
			if (it->DoCleanUp(killThreads))
			{
				it = m_threadList.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
};
