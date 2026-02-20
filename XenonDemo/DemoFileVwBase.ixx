module;

#include "os_minimal.h"
#include <string>

export module Demo.FileVwBase;

export import Xe.FileVwIF;
export import ViewManagerDemo_IF;
export import Xe.D2DWndBase;
import Xe.Helpers;

export class CDemoFileVwBase : public CXeD2DWndBase, public CXeFileVwIF
{
protected:
	CViewManagerDemoIF* m_pVwMgr = nullptr;
	dsid_t m_dsId;

	std::wstring m_filename, m_pathname;

public:
	CDemoFileVwBase(CViewManagerDemoIF* pVwMgr, dsid_t dsId)
			: CXeD2DWndBase(pVwMgr->GetUIcolors()), m_pVwMgr(pVwMgr), m_dsId(dsId)
	{
	}

	virtual bool LoadFile(const std::wstring& pathname) = 0;

	virtual dsid_t GetDataSourceId() const override { return m_dsId; }

	virtual const std::wstring& GetViewName() const override { return m_filename; }
	virtual const std::wstring& GetPathName() const override { return m_pathname; }

	virtual HWND GetHwndOfView() const override { return Hwnd(); }

	// Return true if point (in screen coords.) is in 'this' view else false.
	virtual bool IsPointInThisView(POINT& pt) override
	{
		CRect rc = _GetClientRect();
		ClientToScreen(rc);
		return rc.PtInRect(pt) != 0;
	}

	virtual PID GetViewPID() const override { return PID::None; }

	virtual COLORREF GetViewTitleTextColor() const override
	{
		return m_pVwMgr->GetUIcolors()->GetColor(CID::CtrlTxt);
	}

	virtual void OnCopyInfoToClipboard(ECLIPBRDOP eClpBrdOp) override
	{
		switch (eClpBrdOp)
		{
		case ECLIPBRDOP::eFILENAME:
			CopyTextToClipboardW(m_filename);
			break;
		case ECLIPBRDOP::eFULLPATH:
			CopyTextToClipboardW(m_pathname);
			break;
		}
	}
	virtual bool CanCopyInfoToClipboard(ECLIPBRDOP eClpBrdOp) override
	{
		return true;
	}

	virtual std::wstring GetTooltipForTab() override
	{
		return m_pathname;
	}

protected:
	void _SetPathname(const std::wstring& pathname)
	{
		m_filename = GetFilenameWithExt(pathname);
		m_pathname = pathname;
	}
};

