module;

#include "os_minimal.h"
//#include <afxext.h>         // MFC extensions
//#include <afxole.h>
#include <string>
#include <shtypes.h>
#include <Shlobj_core.h>
#include "logging.h"

export module Xe.OpenFileExplorerAsync;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export class CXeOpenFileExplorerAsync
{
protected:
	VSRL::Logger& logger() { return VSRL::s_pVSRL->GetInstance("CXeOpenFileExplorerAsync"); }
	HANDLE m_hWorkThread = nullptr;
	int m_nWorkThreadId = 0;

	std::wstring m_pathname_to_select;

public:
	void OpenFileExplorer_SelectFile(const std::wstring& pathname)
	{
		m_pathname_to_select = pathname;
		_CreateWorkThread();
	}

protected:
	void _CreateWorkThread()
	{
		if (m_hWorkThread)
		{
			logger().error("Work thread needs killing");
			::TerminateThread(m_hWorkThread, 0);
		}
		m_hWorkThread = ::CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)CXeOpenFileExplorerAsync::_SWorkThread, this, 0,
			(LPDWORD)&(m_nWorkThreadId));
	}

	static DWORD WINAPI _SWorkThread(LPVOID* pParam)
	{
		CXeOpenFileExplorerAsync* pThis = (CXeOpenFileExplorerAsync*)pParam;
		return pThis->_WorkThread();
	}

	DWORD _WorkThread()
	{
		logger().debug("Work thread started");
		HRESULT result = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		XeASSERT(result == S_OK);

		PIDLIST_ABSOLUTE pIdL = NULL;
		SFGAOF out;
		result = ::SHParseDisplayName(m_pathname_to_select.c_str(), NULL, &pIdL, SFGAO_FILESYSTEM, &out);
		if (SUCCEEDED(result))
		{
			result = ::SHOpenFolderAndSelectItems(pIdL, 0, 0, 0);
			ILFree(pIdL);
		}

		logger().debug("Work thread ends");
		::CoUninitialize();
		m_hWorkThread = nullptr;
		m_nWorkThreadId = 0;
		return 0;
	}
};

