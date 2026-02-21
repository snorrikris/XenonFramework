module;

#include "os_minimal.h"
#include <memory>
#include "logging.h"

export module Demo.MainWnd;

import Xe.MainFrameBase;
import Demo.ViewManager;
import Demo.Form;

export class CDemoMainWnd : public CXeMainFrameBase
{
private:
	VSRL::Logger& logger() { return VSRL::s_pVSRL->GetInstance("CDemoMainWnd"); }

	std::unique_ptr<CViewManagerDemo> m_viewManager;

public:
	CDemoMainWnd(CXeUIcolorsIF* pUIcolors) : CXeMainFrameBase(pUIcolors)
	{
		m_mainWndClassName = L"XenonDemo_MainWndClass";

		m_viewManager = std::make_unique<CViewManagerDemo>(m_xeUI);
		m_pVwMgr = dynamic_cast<CXeViewManagerIF*>(m_viewManager.get());
	}

protected:
	//virtual int _GetViewProp(int view_id, XeViewProp view_property_id, int param = 0) override
	//{
	//	return 0;
	//}

	virtual void _OnCreateClient() override
	{
		dsid_t dsId = m_viewManager->GetNextNewDataSourceId();
		std::unique_ptr<CDemoForm> view = std::make_unique<CDemoForm>(m_viewManager.get(), dsId, Hwnd());
		view->CreateView();
		CreateViewParams viewParams(ETABVIEWID::eAnyTabVw);
		viewParams.makeThisCurrentView = true;
		m_viewManager->AttachView(std::move(view), viewParams);
	}
};