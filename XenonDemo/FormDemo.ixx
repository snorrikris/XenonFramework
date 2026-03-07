module;

#include "os_minimal.h"
#include <string>
#include "XeResource.h"

export module Demo.Form;

import Xe.FormBase;
import Demo.ViewManager_IF;

export class CDemoForm : public CXeFormBase
{
protected:
	CViewManagerDemoIF* m_pDemoVwMgr = nullptr;

	std::wstring m_viewname = L"Form demo", m_description = L"Form demo - view with controls (dialog)";

public:
	CDemoForm(CViewManagerDemoIF* pVwMgr, dsid_t dsId, HWND hWndParent)
			: CXeFormBase(IDD_FORMVIEW, pVwMgr, dsId, hWndParent), m_pDemoVwMgr(pVwMgr)
	{
	}

#pragma region CXeFileVwIF_impl
	virtual std::wstring GetViewName() const override { return m_viewname; }
	virtual std::wstring GetPathName() const override { return m_description; }
	virtual std::wstring GetTooltipForTab() override { return m_description; }
#pragma endregion CXeFileVwIF_impl
};

