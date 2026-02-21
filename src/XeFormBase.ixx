module;

#include "os_minimal.h"

export module Xe.FormBase;

export import Xe.FileVwIF;
export import Xe.BaseDlg;

export class CXeFormBase : public CXeBaseDlg, public CXeFileVwIF
{
protected:
	dsid_t m_dsId;

public:
	CXeFormBase(UINT nIDTemplate, CXeViewManagerIF* pVwMgr, dsid_t dsId, HWND hWndParent)
		: CXeBaseDlg(nIDTemplate, pVwMgr, hWndParent), m_dsId(dsId)
	{
	}

	bool CreateView()
	{
		return _CreateDialog();
	}

#pragma region CXeFileVwIF_impl
	virtual dsid_t GetDataSourceId() const override { return m_dsId; }
	virtual PID GetViewPID() const override { return PID::None; }
	virtual COLORREF GetViewTitleTextColor() const override
	{
		return m_pVwMgr->GetUIcolors()->GetColor(CID::CtrlTxt);
	}
	virtual HWND GetHwndOfView() const override { return Hwnd(); }
	virtual bool IsPointInThisView(POINT& pt) override
	{
		CRect rc = _GetClientRect();
		ClientToScreen(rc);
		return rc.PtInRect(pt) != 0;
	}
	virtual void SetFocusToView() override {}
	virtual void OnCopyInfoToClipboard(ECLIPBRDOP eClpBrdOp) override {}
	virtual bool CanCopyInfoToClipboard(ECLIPBRDOP eClpBrdOp) override { return false; }
	virtual void OnChangedSettings(const ChangedSettings& chg_settings) override
	{
		bool isFontsChanged = m_xeUI->IsFontSettingsChanged(chg_settings);
		bool isColorsChanged = chg_settings.IsChanged(L"Colors", L"Color");
		std::vector<CXeD2DCtrlBase*> controls = _GetAllControls();
		for (CXeD2DCtrlBase* ctrl : controls)
		{
			ctrl->OnChangedUIsettings(isFontsChanged, isColorsChanged);
		}
	}
#pragma endregion CXeFileVwIF_impl
};

