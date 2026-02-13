module;

#include "os_minimal.h"
#include <string>
#include "XeResource.h"

export module Xe.AskStringDlg;

import Xe.BaseDlg;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export class CXeAskStringDlg : public CXeBaseDlg
{
public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ASK_STRING };
#endif

	std::wstring m_strDialogTitle;
	std::wstring m_str2ask;

	//CAskStringDlg(CXeViewManagerIF* pVwMgr, CWnd* pParent = nullptr, CRect* pRectCell = nullptr)
	//	: CXeBaseDlg(IDD_BOOKMARK_COMMENT, pVwMgr, pRectCell, pParent)
	//{
	//}

	CXeAskStringDlg(CXeViewManagerIF* pVwMgr, const std::wstring& title, const std::wstring& comment,
		HWND hParent = nullptr, CRect* pRectCell = nullptr)
		: CXeBaseDlg(IDD_ASK_STRING, pVwMgr, hParent, pRectCell)
	{
		m_strDialogTitle = title;
		m_str2ask = comment;
	}
	//virtual ~CXeAskStringDlg();

protected:
	virtual BOOL _OnInitDialog() override
	{
		_InitDialog({});
		::SetWindowText(Hwnd(), m_strDialogTitle.c_str());
		_SetControlString(IDC_ASK_STRING, m_str2ask);
		return TRUE;
	}

	virtual void OnOK() override
	{
		m_str2ask = _GetControlString(IDC_ASK_STRING);
		CXeBaseDlg::OnOK();
	}
};
