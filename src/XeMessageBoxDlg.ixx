module;

#include "os_minimal.h"
#include <string>
#include "XeResource.h"

export module Xe.MessageBoxDlg;

import Xe.BaseDlg;
import Xe.StringTools;
import Xe.ViewManagerIF;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
#define MB_OK                       0x00000000L
#define MB_OKCANCEL                 0x00000001L
#define MB_ABORTRETRYIGNORE         0x00000002L
#define MB_YESNOCANCEL              0x00000003L
#define MB_YESNO                    0x00000004L
#define MB_RETRYCANCEL              0x00000005L
#define MB_CANCELTRYCONTINUE        0x00000006L

#define MB_ICONHAND                 0x00000010L
#define MB_ICONQUESTION             0x00000020L
#define MB_ICONEXCLAMATION          0x00000030L
#define MB_ICONASTERISK             0x00000040L

#define MB_USERICON                 0x00000080L
#define MB_ICONWARNING              MB_ICONEXCLAMATION
#define MB_ICONERROR                MB_ICONHAND

#define MB_ICONINFORMATION          MB_ICONASTERISK
#define MB_ICONSTOP                 MB_ICONHAND

#define MB_DEFBUTTON1               0x00000000L
#define MB_DEFBUTTON2               0x00000100L
#define MB_DEFBUTTON3               0x00000200L
#define MB_DEFBUTTON4               0x00000300L

#define MB_APPLMODAL                0x00000000L
#define MB_SYSTEMMODAL              0x00001000L
#define MB_TASKMODAL                0x00002000L
#define MB_HELP                     0x00004000L // Help Button

#define MB_NOFOCUS                  0x00008000L
#define MB_SETFOREGROUND            0x00010000L
#define MB_DEFAULT_DESKTOP_ONLY     0x00020000L

#define MB_TOPMOST                  0x00040000L
#define MB_RIGHT                    0x00080000L
#define MB_RTLREADING               0x00100000L

#define MB_SERVICE_NOTIFICATION     0x00200000L

#define MB_TYPEMASK                 0x0000000FL
#define MB_ICONMASK                 0x000000F0L
#define MB_DEFMASK                  0x00000F00L
#define MB_MODEMASK                 0x00003000L
#define MB_MISCMASK                 0x0000C000L


#define IDI_APPLICATION     MAKEINTRESOURCE(32512)
#define IDI_HAND            MAKEINTRESOURCE(32513)
#define IDI_QUESTION        MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION     MAKEINTRESOURCE(32515)
#define IDI_ASTERISK        MAKEINTRESOURCE(32516)
#define IDI_WINLOGO         MAKEINTRESOURCE(32517)
#define IDI_SHIELD          MAKEINTRESOURCE(32518)

#define IDI_WARNING     IDI_EXCLAMATION
#define IDI_ERROR       IDI_HAND
#define IDI_INFORMATION IDI_ASTERISK
*/

export class CXeMessageBoxDlg : public CXeBaseDlg
{
public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MSGBOX };
#endif

	const wchar_t*	m_pMsg;
	const wchar_t*	m_pTitle;
	UINT			m_type;
	int				m_b1_code = 0, m_b2_code = 0, m_b3_code = 0;

	CXeMessageBoxDlg(CXeViewManagerIF* pVwMgr, UINT uType, const wchar_t* pMsg, const wchar_t* pTitle,
			HWND hParent = nullptr, CRect* pRectCell = nullptr)
			: CXeBaseDlg(IDD_MSGBOX, pVwMgr, hParent, pRectCell), m_type(uType), m_pMsg(pMsg), m_pTitle(pTitle)
	{
	}
	//virtual ~CXeMessageBoxDlg();

protected:
	virtual BOOL _OnInitDialog() override
	{
		_SetDefaultButton();
		_SetButtons();
		::SetWindowText(Hwnd(), m_pTitle ? m_pTitle : L"Error");
		_SetIcon();

		BOOL result = _InitDialog({});

		CRect rcCli, rcWnd;
		GetClientRect(rcCli);
		GetWindowRect(rcWnd);

		CXeScintillaEditControl* pTxtCtrl = _GetEditControl(IDC_MESSAGE);
		if (m_pMsg && pTxtCtrl)
		{
			CRect rcTxtCtrl = GetControlRect(IDC_MESSAGE);

			// Calculate minimum dialog cx size.
			CRect rcB2 = GetControlRect(IDC_MSG_BTN2);
			CRect rcB3 = GetControlRect(IDC_MSG_BTN3);
			int cxDlgMargin = rcB3.left - rcB2.right;
			int cxDlgMin = cxDlgMargin * 2 + rcB3.Width() * 3;

			// Calculate text size.
			const XeFontMetrics& tm = m_xeUI->GetFontMetric(EXE_FONT::eUI_Font);
			std::vector<std::wstring> lines = splitW(std::wstring(m_pMsg), L"\n");
			int cx_txt = m_xeUI->GetMaxTextWidthUsingFontW(lines, EXE_FONT::eUI_Font, Hwnd())
					+ (4 * tm.m_aveCharWidth);// Add a little margin to text size.
			int cy_txt = ((int)lines.size() + 1) * tm.m_height;
			
			// Calculate new dialog size (if it needs to grow).
			int cx_txtctrl = rcTxtCtrl.Width(), cy_txtctrl = rcTxtCtrl.Height();
			// Allow cx to grow by max 500%, Allow cy to grow by max 500%
			int cx_grow = cx_txt > cx_txtctrl ? std::min((5 * cx_txtctrl), cx_txt) - cx_txtctrl : 0;
			int cy_grow = cy_txt > cy_txtctrl ? std::min((5 * cy_txtctrl), cy_txt) - cy_txtctrl : 0;
			if (cx_grow || cy_grow)
			{
				UINT flags = SWP_NOMOVE | SWP_NOCOPYBITS | SWP_NOZORDER;
				int cxNew = std::max(cxDlgMin, (rcWnd.Width() + cx_grow));
				int cyNew = rcWnd.Height() + cy_grow;
				SetWindowPos(::GetParent(Hwnd()), 0, 0, cxNew, cyNew, flags);

				int x_btn_offset = cxNew - rcWnd.Width();
				int y_btn_offset = cyNew - rcWnd.Height();
				_MoveButtons(x_btn_offset, y_btn_offset);
			}
			pTxtCtrl->SetWindowText(m_pMsg);
		}
		return result;
	}

	virtual LRESULT _OnWmCommand(WORD wSource, WORD wID, HWND sender) override
	{
		if (wID == IDC_MSG_BTN1 && wSource == BN_CLICKED)
		{
			OnBtn1();
			return 0;
		}
		else if (wID == IDC_MSG_BTN2 && wSource == BN_CLICKED)
		{
			OnBtn2();
			return 0;
		}
		else if (wID == IDC_MSG_BTN3 && wSource == BN_CLICKED)
		{
			OnBtn3();
			return 0;
		}
		return CXeBaseDlg::_OnWmCommand(wSource, wID, sender);
	}

	void OnBtn1() { EndDialog(m_b1_code); }
	void OnBtn2() { EndDialog(m_b2_code); }
	void OnBtn3() { EndDialog(m_b3_code); }

	void _SetIcon()
	{
		CXeD2DStatic* pIconCtrl = _GetStaticControl(IDC_MSG_ICON);
		HICON hIcon = _GetIconHandle();
		if (pIconCtrl)
		{
			if (hIcon)
			{
				pIconCtrl->SetIcon(hIcon);
			}
			else
			{
				// When no icon - expand the text control to the left.
				CRect rcIconCtrl = GetControlRect(IDC_MSG_ICON);
				pIconCtrl->ShowWindow(SW_HIDE);
				CRect rcTxtCtrl = GetControlRect(IDC_MESSAGE);
				rcTxtCtrl.left = rcIconCtrl.left;
				CXeScintillaEditControl* pTxtCtrl = _GetEditControl(IDC_MESSAGE);
				if (pTxtCtrl)
				{
					pTxtCtrl->SetWindowPos(nullptr, rcTxtCtrl.left, rcTxtCtrl.top, rcTxtCtrl.Width(),
							rcTxtCtrl.Height(), SWP_NOCOPYBITS | SWP_NOZORDER);
				}
			}
		}
	}

	HICON _GetIconHandle() const
	{
		UINT uIcn = m_type & MB_ICONMASK;
		if (uIcn == MB_ICONHAND       ) { return ::LoadIcon(NULL, IDI_HAND); }
		if (uIcn == MB_ICONQUESTION   ) { return ::LoadIcon(NULL, IDI_QUESTION); }
		if (uIcn == MB_ICONEXCLAMATION) { return ::LoadIcon(NULL, IDI_EXCLAMATION); }
		if (uIcn == MB_ICONASTERISK   ) { return ::LoadIcon(NULL, IDI_ASTERISK); }
		return 0;
	}

	void _SetButtons()
	{
		UINT uBtn = m_type & 0x0000000FL;
		CXeD2DButton* pBtn1 = _GetButtonControl(IDC_MSG_BTN1);
		CXeD2DButton* pBtn2 = _GetButtonControl(IDC_MSG_BTN2);
		CXeD2DButton* pBtn3 = _GetButtonControl(IDC_MSG_BTN3);
		if (!(pBtn1 && pBtn2 && pBtn3))
		{
			XeASSERT(FALSE);
			return;
		}
		CRect rcB1 = GetControlRect(IDC_MSG_BTN1);
		CRect rcB2 = GetControlRect(IDC_MSG_BTN2);
		CRect rcB3 = GetControlRect(IDC_MSG_BTN3);
		if (uBtn == MB_OK)
		{
			pBtn1->SetWindowText(L"Ok");
			pBtn2->ShowWindow(SW_HIDE);
			pBtn3->ShowWindow(SW_HIDE);
			m_b1_code = IDOK;
			pBtn1->SetWindowPos(nullptr, rcB3.left, rcB3.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_NOZORDER);
		}
		else if (uBtn == MB_OKCANCEL)
		{
			pBtn1->SetWindowText(L"Ok");
			pBtn2->SetWindowText(L"Cancel");
			pBtn3->ShowWindow(SW_HIDE);
			m_b1_code = IDOK;
			m_b2_code = IDCANCEL;
			pBtn1->SetWindowPos(nullptr, rcB2.left, rcB2.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_NOZORDER);
			pBtn2->SetWindowPos(nullptr, rcB3.left, rcB3.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_NOZORDER);
		}
		else if (uBtn == MB_ABORTRETRYIGNORE)
		{
			pBtn1->SetWindowText(L"Abort");
			pBtn2->SetWindowText(L"Retry");
			pBtn3->SetWindowText(L"Ignore");
			m_b1_code = IDABORT;
			m_b2_code = IDRETRY;
			m_b3_code = IDIGNORE;
		}
		else if (uBtn == MB_YESNOCANCEL)
		{
			pBtn1->SetWindowText(L"Yes");
			pBtn2->SetWindowText(L"No");
			pBtn3->SetWindowText(L"Cancel");
			m_b1_code = IDYES;
			m_b2_code = IDNO;
			m_b3_code = IDCANCEL;
		}
		else if (uBtn == MB_YESNO)
		{
			pBtn1->SetWindowText(L"Yes");
			pBtn2->SetWindowText(L"No");
			pBtn3->ShowWindow(SW_HIDE);
			m_b1_code = IDYES;
			m_b2_code = IDNO;
			pBtn1->SetWindowPos(nullptr, rcB2.left, rcB2.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_NOZORDER);
			pBtn2->SetWindowPos(nullptr, rcB3.left, rcB3.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_NOZORDER);
		}
		else if (uBtn == MB_RETRYCANCEL)
		{
			pBtn1->SetWindowText(L"Retry");
			pBtn2->SetWindowText(L"Cancel");
			pBtn3->ShowWindow(SW_HIDE);
			m_b1_code = IDRETRY;
			m_b2_code = IDCANCEL;
			pBtn1->SetWindowPos(nullptr, rcB2.left, rcB2.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_NOZORDER);
			pBtn2->SetWindowPos(nullptr, rcB3.left, rcB3.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_NOZORDER);
		}
		else if (uBtn == MB_CANCELTRYCONTINUE)
		{
			pBtn1->SetWindowText(L"Cancel");
			pBtn2->SetWindowText(L"Try again");
			pBtn3->SetWindowText(L"Continue");
			m_b1_code = IDCANCEL;
			m_b2_code = IDTRYAGAIN;
			m_b3_code = IDCONTINUE;
		}
	}

	void _SetDefaultButton()
	{
		CXeD2DButton* pBtn1 = _GetButtonControl(IDC_MSG_BTN1);
		CXeD2DButton* pBtn2 = _GetButtonControl(IDC_MSG_BTN2);
		CXeD2DButton* pBtn3 = _GetButtonControl(IDC_MSG_BTN3);
		if (!(pBtn1 && pBtn2 && pBtn3))
		{
			XeASSERT(FALSE);
			return;
		}
		if (m_type & MB_DEFBUTTON2)
		{
			pBtn1->ModifyStyle(BS_DEFPUSHBUTTON, 0);
			pBtn2->ModifyStyle(0, BS_DEFPUSHBUTTON);
			pBtn1->RedrawWindow();
			pBtn2->RedrawWindow();
		}
		else if (m_type & MB_DEFBUTTON3)
		{
			pBtn1->ModifyStyle(BS_DEFPUSHBUTTON, 0);
			pBtn3->ModifyStyle(0, BS_DEFPUSHBUTTON);
			pBtn1->RedrawWindow();
			pBtn3->RedrawWindow();
		}
	}

	void _MoveButtons(int x_btn_offset, int y_btn_offset)
	{
		CXeD2DButton* pBtn1 = _GetButtonControl(IDC_MSG_BTN1);
		CXeD2DButton* pBtn2 = _GetButtonControl(IDC_MSG_BTN2);
		CXeD2DButton* pBtn3 = _GetButtonControl(IDC_MSG_BTN3);
		if (!(pBtn1 && pBtn2 && pBtn3))
		{
			XeASSERT(FALSE);
			return;
		}
		CRect rcB1 = GetControlRect(IDC_MSG_BTN1);
		rcB1.OffsetRect(x_btn_offset, y_btn_offset);
		CRect rcB2 = GetControlRect(IDC_MSG_BTN2);
		rcB2.OffsetRect(x_btn_offset, y_btn_offset);
		CRect rcB3 = GetControlRect(IDC_MSG_BTN3);
		rcB3.OffsetRect(x_btn_offset, y_btn_offset);
		pBtn1->SetWindowPos(nullptr, rcB1.left, rcB1.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_NOZORDER);
		pBtn2->SetWindowPos(nullptr, rcB2.left, rcB2.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_NOZORDER);
		pBtn3->SetWindowPos(nullptr, rcB3.left, rcB3.top, 0, 0, SWP_NOSIZE | SWP_NOCOPYBITS | SWP_NOZORDER);
	}
};

export int XeMessageBox(CXeViewManagerIF* pVwMgr, HWND hParent,
		const wchar_t* pMsg, const wchar_t* pTitle = nullptr, UINT uType = MB_OK, CRect * pRectCell = nullptr)
{
	CXeMessageBoxDlg msgbox(pVwMgr, uType, pMsg, pTitle, hParent, pRectCell);
	INT_PTR dlgResult = msgbox.DoModal();
	return (int)dlgResult;
}

