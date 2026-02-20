module;

#include "os_minimal.h"
#include <memory>
#include "logging.h"

export module Demo.MainWnd;

import Xe.MainFrameBase;
import ViewManagerDemo;

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
};