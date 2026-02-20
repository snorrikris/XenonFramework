module;

#include "os_minimal.h"
#include <string>
#include <algorithm>
#include <wincodec.h>
#include <D2d1_1.h>
//#include "CustomWndMsgs.h"

export module Xe.ImageCtrl;

import Xe.UIcolorsIF;
import Xe.D2DWndBase;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export constexpr UINT WMU_NOTIFY_ZOOM_CHANGED = (WM_USER + 835);	// Sent from CImageCtrl to parent to notify of zoom level change

typedef enum tagImgUIbtn { eImgBtnNone = 0, eImgBtnReset, eImgBtnZoomIn, eImgBtnZoomOut, eImgBtnRotateLeft, eImgBtnRotateRight } EImgUIbtn;

export class CXeImageCtrl : public CXeD2DWndBase
{
protected:
	float m_angle = 0.0f;		// Rotation angle.

	Microsoft::WRL::ComPtr<IWICFormatConverter> m_wicImg;

	bool m_isMouseCapture = false;

	CRect m_rcReset, m_rcZoomIn, m_rcZoomOut, m_rcRotateLeft, m_rcRotateRight;
	EImgUIbtn m_eMouseOverBtn = eImgBtnNone;

	int m_sizeType;             // Size type: SIZE_ORIGINAL, SIZE_SCALETOFILL, SIZE_CUSTOM.
	double m_left;              // x-coordinate of the image top-left point.
	double m_top;               // y-coordinate of the image top-left point.
	double m_width;             // Image width (pixels).
	double m_height;            // Image height (pixels).
	double m_widthOriginal;     // Image original width (pixels).
	double m_heightOriginal;    // Image original height (pixels).
	bool m_maintainAspectRatio; // Maintain aspect ratio or not.
	double m_aspectRatio;       // Aspect ratio factor:  Case "no image": CStatic->ClientRectangle (height / width), Case "image": m_height / m_width.
	int m_allignmentType;       // Image alignment type inside control: ALLIGN_TOPLEFT, ALLIGN_TOPCENTER,... 
	bool m_isPanMode;           // Enabled/disabled PAN mode.
	bool m_isZoomMode;          // Enabled/disabled ZOOM mode.
	double m_zoomMin;           // Minimal rectangle side value (in pixels) on ZOOM action.
	double m_zoomMax;           // Maximal rectangle side value (in pixels) on ZOOM action.
	BOOL m_isImageShown;        // Is image shown in the control?

	//BOOL m_isInitialShow;       // Is initial image show? TRUE: if not derived from PAN/ZOOM mode action, FALSE: otherwise.
	CPoint m_panAtPt;           // Origin point of PAN action.
	CPoint m_panOffset;         // Offset distances at PAN action.
	CPoint m_zoomAtPt;          // Point at zoom event, which is triggered by mouse wheel scrolling ON image.
	double m_zoomFactor;        // Zoom factor: Case > 1: zoom in, Case < 1: zoom out.

public:
	CXeImageCtrl(CXeUIcolorsIF* pUIcolors)
		: CXeD2DWndBase(pUIcolors), m_sizeType(sizeType::SIZE_SCALETOFIT100),
		m_maintainAspectRatio(true), m_aspectRatio(1), m_allignmentType(allignmentType::ALLIGN_TOPLEFT),
		m_isPanMode(TRUE), m_isZoomMode(TRUE)
	{
		m_isImageShown = FALSE;
		m_panAtPt.SetPoint(-1, -1);
		m_panOffset.SetPoint(0, 0);
		_rstZoomAtPt();
		m_zoomFactor = 1.0;
		m_zoomMin = 1;
		m_zoomMax = 99999;

		int x = 3, y = 5, cx = 24, cy = 24, cyNext = 28;
		m_rcReset.SetRect(x, y, x + cx, y + cy);
		y += cyNext;
		m_rcZoomIn.SetRect(x, y, x + cx, y + cy);
		y += cyNext;
		m_rcZoomOut.SetRect(x, y, x + cx, y + cy);
		y += cyNext;
		m_rcRotateLeft.SetRect(x, y, x + cx, y + cy);
		y += cyNext;
		m_rcRotateRight.SetRect(x, y, x + cx, y + cy);
	}

	~CXeImageCtrl()
	{
		release(false);
	}

	bool Create(HWND hParentWnd)
	{
		std::wstring class_name = L"XeImageCtrlWndClass";
		m_xeUI->RegisterWindowClass(class_name, D2DCtrl_WndProc);
		HWND hWnd = CreateD2DWindow(0, class_name.c_str(), nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
				CRect(), hParentWnd, 0);
		// Note - OnSize has been called.
		return hWnd != 0;
	}

	enum sizeType { SIZE_ORIGINAL, SIZE_SCALETOFIT, SIZE_SCALETOFIT100, SIZE_CUSTOM };
	enum allignmentType { ALLIGN_TOPLEFT, ALLIGN_TOPCENTER, ALLIGN_TOPRIGHT, ALLIGN_MIDDLELEFT, ALLIGN_MIDDLECENTER, ALLIGN_MIDDLERIGHT, ALLIGN_BOTTOMLEFT, ALLIGN_BOTTOMCENTER, ALLIGN_BOTTOMRIGHT };

	void setSizeType(int sizeType) { m_sizeType = sizeType; }                          // Size type: SIZE_ORIGINAL, SIZE_SCALETOFILL, SIZE_CUSTOM.
	double getLeft() { return m_left; }                                                // x-coordinate of the image top-left point.
	double getTop() { return m_top; }                                                  // y-coordinate of the image top-left point.
	void setWidth(double width) { m_width = width; }                                   // Set image width (pixels).
	double getWidth() { return m_width; }                                              // Image width (pixels).
	double getWidthOriginal() { return m_widthOriginal; }                              // Image original width (pixels).
	void setHeight(double height) { m_height = height; }                               // Set image height (pixels).
	double getHeight() { return m_height; }                                            // Image height (pixels).
	double getHeightOriginal() { return m_heightOriginal; }                            // Image original height (pixels).

	void setMaintainAspectRatio(bool maintainAspectRatio) { m_maintainAspectRatio = maintainAspectRatio; }
	double setAspectRatio(double aspectRatio) { return m_aspectRatio = aspectRatio; }
	double getAspectRatio() { return m_aspectRatio; }                                  // Case "no image": CStatic->ClientRectangle (height / width), Case "image": m_height / m_width.
	void setAllignmentType(int allignmentType) { m_allignmentType = allignmentType; }  // Image alignment type inside control: ALLIGN_TOPLEFT, ALLIGN_TOPCENTER,... 
	void setPanMode(bool isPanMode) { m_isPanMode = isPanMode; }                       // Enable/disable PAN mode.
	void setZoomMode(bool isZoomMode) { m_isZoomMode = isZoomMode; }                   // Enable/disable ZOOM mode.
	double getZoomFactor() { return m_zoomFactor; }
	double getZoomPercent()
	{
		double curW = getWidth();
		double orgW = getWidthOriginal();
		double zoomPercent = curW / orgW;	// 1 is 100%
		return zoomPercent;
	}

	bool isImageShown() { return m_isImageShown > 0; }                                 // Is image shown in the control?

	void update()                                                                     // Update image in the control.
	{
		CalculateImage(true);
		_RedrawDirectly();
	}

	void erase()                                                                      // Erase image from the control.
	{
		release();
		update();
	}

	BOOL load(BYTE* pData, size_t nSize)                                              // Loads an image from BYTE array.
	{
		release();
		_loadImageFromRawDataBuffer(pData, nSize);
		return m_isImageShown;
	}

private:
	void release(bool calcFrameAspectRatio = true)                                    // Release allocated memory and initialize.
	{
		CRect rect;
		m_panAtPt.SetPoint(-1, -1);
		m_panOffset.SetPoint(0, 0);
		_rstZoomAtPt();
		m_zoomFactor = 1;

		if (calcFrameAspectRatio)
		{
			GetClientRect(rect);
			m_aspectRatio = (double)rect.Height() / rect.Width();
		}
	}

	void _loadImageFromRawDataBuffer(BYTE* pData, size_t nSize)
	{
		Microsoft::WRL::ComPtr<IWICStream> stream;
		Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
		Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
		HRESULT hr = m_xeUI->D2D_GetWICFactory()->CreateStream(stream.GetAddressOf());
		XeASSERT(hr == S_OK);
		if (SUCCEEDED(hr))
		{
			// Initialize the stream with the memory pointer and size.
			hr = stream->InitializeFromMemory(pData, (DWORD)nSize);
			XeASSERT(hr == S_OK);
		}
		if (SUCCEEDED(hr))
		{
			// Create a decoder
			hr = m_xeUI->D2D_GetWICFactory()->CreateDecoderFromStream(stream.Get(), NULL,
				WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
			XeASSERT(hr == S_OK);

			// Retrieve the first frame of the image from the decoder
			if (SUCCEEDED(hr))
			{
				hr = decoder->GetFrame(0, frame.GetAddressOf());
				XeASSERT(hr == S_OK);
				if (SUCCEEDED(hr))
				{
					UINT width, height;
					hr = frame->GetSize(&width, &height);
					XeASSERT(hr == S_OK);
					m_widthOriginal = width;
					m_heightOriginal = height;
				}
			}

			//Step 3: Format convert the frame to 32bppPBGRA
			if (SUCCEEDED(hr))
			{
				hr = m_xeUI->D2D_GetWICFactory()->CreateFormatConverter(m_wicImg.GetAddressOf());
				XeASSERT(hr == S_OK);
			}
			if (SUCCEEDED(hr))
			{
				hr = m_wicImg->Initialize(
					frame.Get(),					 // Input bitmap to convert
					GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format
					WICBitmapDitherTypeNone,         // Specified dither pattern
					NULL,                            // Specify a particular palette 
					0.f,                             // Alpha threshold
					WICBitmapPaletteTypeCustom       // Palette translation type
				);
				XeASSERT(hr == S_OK);
			}
		}
		CalculateImage(true);
		_RedrawDirectly();
	}

	void _rstZoomAtPt() { m_zoomAtPt.SetPoint(-1000000, -1000000); }
	bool _isZoomAtPtRst() { return m_zoomAtPt.x == -1000000; }
	bool _Zoom(double zoomFactor, CPoint ptZoomAt)
	{
		if (m_isImageShown && m_isZoomMode)
		{
			double zoomPercent = getZoomPercent();
			bool canZoom =
				(zoomPercent > 0.1 && zoomFactor < 1)	// Zoom in possible
				|| (zoomPercent < 5 && zoomFactor > 1);	// Zoom out possible

			if (canZoom)
			{
				m_zoomAtPt = ptZoomAt;
				m_zoomFactor = zoomFactor;

				//TRACE("zDelta=%d\n", zDelta);
				CalculateImage(false);
				RecalcPos();
				_RedrawDirectly();
			}
			return true;
		}
		return false;
	}

	EImgUIbtn _GetUIbuttonAtPoint(CPoint point)
	{
		if (m_rcReset.PtInRect(point))
		{
			return eImgBtnReset;
		}
		else if (m_rcZoomIn.PtInRect(point))
		{
			return eImgBtnZoomIn;
		}
		else if (m_rcZoomOut.PtInRect(point))
		{
			return eImgBtnZoomOut;
		}
		else if (m_rcRotateLeft.PtInRect(point))
		{
			return eImgBtnRotateLeft;
		}
		else if (m_rcRotateRight.PtInRect(point))
		{
			return eImgBtnRotateRight;
		}
		return eImgBtnNone;
	}

	void _DoUIbuttonAction(EImgUIbtn eButton)
	{
		switch (eButton)
		{
		case eImgBtnReset:
			m_panAtPt.SetPoint(-1, -1);
			m_panOffset.SetPoint(0, 0);
			_rstZoomAtPt();
			m_zoomFactor = 1;
			m_angle = 0.0f;
			CalculateImage(true);
			_RedrawDirectly();
			::SendMessage(GetParent(Hwnd()), WMU_NOTIFY_ZOOM_CHANGED, 0, 0); // Send message to parent window to signalize ZOOM % changed.
			break;
		case eImgBtnZoomIn:
			_Zoom(1.25, CPoint(0, 0));
			break;
		case eImgBtnZoomOut:
			_Zoom(0.75, CPoint(0, 0));
			break;
		case eImgBtnRotateLeft:
			if (m_angle <= 0.0f)
			{
				m_angle = 270.0f;
			}
			else
			{
				m_angle -= 90.0f;
			}
			_RedrawDirectly();
			break;
		case eImgBtnRotateRight:
			m_angle += 90.0f;
			if (m_angle >= 360.0f)
			{
				m_angle = 0.0f;
			}
			_RedrawDirectly();
			break;
		}
	}

protected:
	virtual void _PaintF(ID2D1RenderTarget* pRT, D2D1_RECT_F rc) override
	{
		pRT->Clear(m_xeUI->GetColorF(CID::CtrlBg));

		if (m_wicImg.Get())
		{
			_DrawImg(pRT);
			m_isImageShown = true;
		}
		else
		{
			m_isImageShown = false;
		}
		_DrawButtonImage(pRT, PID::eIMGUI_RESET_PNG, RectFfromRect(m_rcReset), true, true, m_eMouseOverBtn == eImgBtnReset);
		_DrawButtonImage(pRT, PID::eIMGUI_ZOOM_IN_PNG, RectFfromRect(m_rcZoomIn), true, true, m_eMouseOverBtn == eImgBtnZoomIn);
		_DrawButtonImage(pRT, PID::eIMGUI_ZOOM_OUT_PNG, RectFfromRect(m_rcZoomOut), true, true, m_eMouseOverBtn == eImgBtnZoomOut);
		_DrawButtonImage(pRT, PID::eIMGUI_ROTATE_LEFT_PNG, RectFfromRect(m_rcRotateLeft), true, true, m_eMouseOverBtn == eImgBtnRotateLeft);
		_DrawButtonImage(pRT, PID::eIMGUI_ROTATE_RIGHT_PNG, RectFfromRect(m_rcRotateRight), true, true, m_eMouseOverBtn == eImgBtnRotateRight);
	}

	void _DrawImg(ID2D1RenderTarget* pRT)
	{
		int cxUI = 30;
		bool isSwapXY = m_angle == 90.0f || m_angle == 270.0f;
		int imgWidth = isSwapXY ? (int)(m_height + 1e-6) : (int)(m_width + 1e-6);
		int imgHeight = isSwapXY ? (int)(m_width + 1e-6) : (int)(m_height + 1e-6);
		D2D1_RECT_F rcBox{ (float)m_left, (float)m_top, (float)m_left + (float)imgWidth, (float)m_top + (float)imgHeight };
		rcBox = OffsetRectF(rcBox, (float)cxUI, 0.0f);

		bool isImgOnLeftSide = true, isEnabled = true, isHovering = false;
		D2D1_RECT_F rcImg{ rcBox.left, rcBox.top, rcBox.left, rcBox.bottom }; // cx = 0.
		Microsoft::WRL::ComPtr<ID2D1Bitmap> pD2DBitmap;
		HRESULT hr = pRT->CreateBitmapFromWicBitmap(m_wicImg.Get(), NULL, pD2DBitmap.GetAddressOf());
		if (!SUCCEEDED(hr))
		{
			return;
		}
		auto img_size = pD2DBitmap->GetSize();
		if (img_size.width <= 0 || img_size.height <= 0)
		{
			return;
		}
		Microsoft::WRL::ComPtr<ID2D1DeviceContext> pDeviceContext;
		pRT->QueryInterface(__uuidof(ID2D1DeviceContext), (void**)pDeviceContext.GetAddressOf());
		XeASSERT(pDeviceContext);

		// Calculate the image box - size and position (image will be scaled to fit)
		rcImg = CalculateImageSizeAndPosition(isImgOnLeftSide, rcBox, img_size);

		// Scale image to fit the image box.
		float scaleXY = 1.0f;
		float cxBox = WidthOf(rcImg);
		float cyBox = HeightOf(rcImg);
		if (cxBox > 0 && cyBox > 0)
		{
			float scaleX = cxBox / img_size.width;
			float scaleY = cyBox / img_size.height;
			scaleXY = std::min(scaleX, scaleY);
		}
		D2D_SIZE_F img_scale(scaleXY, scaleXY);
		D2D1::Matrix3x2F mtx = D2D1::Matrix3x2F::Scale(img_scale, D2D1::Point2F(0, 0));

		Microsoft::WRL::ComPtr<ID2D1Effect> scaleEffect;
		pDeviceContext->CreateEffect(CLSID_D2D12DAffineTransform, scaleEffect.GetAddressOf());

		D2D1::Matrix3x2F mtxRot = D2D1::Matrix3x2F::Rotation(m_angle, D2D1::Point2F(img_size.width /2.0f, img_size.height / 2.0f));
		mtxRot = mtxRot * D2D1::Matrix3x2F::Scale(img_scale, D2D1::Point2F(0, 0));
		bool isEffectNeeded = !isEnabled || isHovering;
		scaleEffect->SetInput(0, pD2DBitmap.Get());

		scaleEffect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, mtxRot);

		D2D1_POINT_2F targetOffset(rcImg.left, rcImg.top);
		pDeviceContext->DrawImage(scaleEffect.Get(), targetOffset);
	}

	virtual LRESULT _OnSetFocus(HWND hOldWnd)
	{
		::SendMessage(GetParent(Hwnd()), WMU_NOTIFY_GOT_FOCUS, (WPARAM)GetSafeHwnd(), (LPARAM)hOldWnd);
		return 0;
	}

	virtual LRESULT _OnKillFocus(HWND hNewWnd) override
	{
		::SendMessage(GetParent(Hwnd()), WMU_NOTIFY_LOST_FOCUS, (WPARAM)GetSafeHwnd(), (LPARAM)hNewWnd);
		return 0;
	}

	virtual LRESULT _OnLeftDown(UINT nFlags, CPoint point) override
	{
		SetFocus();
		BOOL fCtrlKeyDown = (::GetKeyState(VK_CONTROL) & 0x8000) ? TRUE : FALSE;
		BOOL fShiftKeyDown = (::GetKeyState(VK_SHIFT) & 0x8000) ? TRUE : FALSE;
		BOOL fMenuKeyDown = (::GetKeyState(VK_MENU) & 0x8000) ? TRUE : FALSE;
		if (fCtrlKeyDown || fShiftKeyDown || fMenuKeyDown)
			return 0;
		EImgUIbtn eUIbutton = _GetUIbuttonAtPoint(point);
		if (eUIbutton > eImgBtnNone)
		{
			_DoUIbuttonAction(eUIbutton);
		}
		else if (m_isImageShown && m_isPanMode)
		{
			SetCapture();
			m_isMouseCapture = true;
			m_panAtPt = point;
			m_panOffset.SetPoint(0, 0);
		}
		return 0;
	}

	virtual LRESULT _OnLeftUp(UINT nFlags, CPoint point) override
	{
		if (m_isMouseCapture)
		{
			CRect rect;
			GetClientRect(rect);
			if (m_isImageShown && m_isPanMode)
			{
				RecalcPos();
				_RedrawDirectly();
			}
			ReleaseCapture();
			m_isMouseCapture = false;
		}
		return 0;
	}

	virtual LRESULT _OnLeftDoubleClick(UINT nFlags, CPoint point) override
	{
		SetFocus();
		BOOL fCtrlKeyDown = (::GetKeyState(VK_CONTROL) & 0x8000) ? TRUE : FALSE;
		BOOL fShiftKeyDown = (::GetKeyState(VK_SHIFT) & 0x8000) ? TRUE : FALSE;
		BOOL fMenuKeyDown = (::GetKeyState(VK_MENU) & 0x8000) ? TRUE : FALSE;
		if (fCtrlKeyDown || fShiftKeyDown || fMenuKeyDown)
			return 0;
		EImgUIbtn eUIbutton = _GetUIbuttonAtPoint(point);
		if (eUIbutton > eImgBtnNone)
		{
			_DoUIbuttonAction(eUIbutton);
		}
		return 0;
	}

	virtual LRESULT _OnMouseMove(UINT nFlags, CPoint point) override
	{
		if (m_isMouseCapture && m_isImageShown && m_isPanMode && nFlags & MK_LBUTTON)
		{
			m_panOffset = point - m_panAtPt;
			m_panAtPt = point;
			//CalculateImage(false);
			m_left += m_panOffset.x;
			m_top += m_panOffset.y;
			_RedrawDirectly();
		}
		else
		{
			EImgUIbtn eMouseOverBtn = _GetUIbuttonAtPoint(point);
			if (eMouseOverBtn != m_eMouseOverBtn)
			{
				m_eMouseOverBtn = eMouseOverBtn;
				_RedrawDirectly();
			}
		}
		return CXeD2DWndBase::_OnMouseMove(nFlags, point);
	}

	virtual LRESULT _OnMouseWheel(WORD fwKeys, short zDelta, CPoint pt) override
	{
		if (_Zoom(1 + (zDelta / 1200.0), pt))
		{
			return TRUE;
		}
		return 1;
	}

	void CalculateImage(bool isInitialShow)
	{
		//if (!m_pBmp)
		//{
		//	return;
		//}

		double w0, h0, sx, sy, s, dx, dy;
		CRect rect;
		GetClientRect(rect);

		if (isInitialShow)
		{
			m_aspectRatio = m_heightOriginal / m_widthOriginal;

			if (!m_maintainAspectRatio)
			{
				if (m_sizeType == sizeType::SIZE_SCALETOFIT)
				{
					m_width = rect.Width();
					m_height = rect.Height();
				}
				else if (m_sizeType == sizeType::SIZE_ORIGINAL)
				{
					m_width = m_widthOriginal;
					m_height = m_heightOriginal;
				}
				m_left = m_top = 0;
			}
			else
			{
				if (m_sizeType == sizeType::SIZE_SCALETOFIT)
				{
					sx = rect.Width() / m_widthOriginal;
					sy = rect.Height() / m_heightOriginal;
					s = (sx > sy) ? sy : sx;
					m_height = m_aspectRatio * (m_width = s * m_widthOriginal);
				}
				if (m_sizeType == sizeType::SIZE_SCALETOFIT100)
				{
					sx = rect.Width() / m_widthOriginal;
					sy = rect.Height() / m_heightOriginal;
					s = (sx > sy) ? sy : sx;
					s = s > 1 ? 1 : s;
					m_height = m_aspectRatio * (m_width = s * m_widthOriginal);
				}
				else if (m_sizeType == sizeType::SIZE_CUSTOM)
				{
					sx = m_width / m_widthOriginal;
					sy = m_height / m_heightOriginal;
					s = (sx > sy) ? sy : sx;
					m_height = m_aspectRatio * (m_width = s * m_widthOriginal);
				}
				else if (m_sizeType == sizeType::SIZE_ORIGINAL)
				{
					m_width = m_widthOriginal;
					m_height = m_heightOriginal;
				}
			}

			if (m_allignmentType == allignmentType::ALLIGN_TOPLEFT)
			{
				m_left = m_top = 0;
			}
			else if (m_allignmentType == allignmentType::ALLIGN_TOPCENTER)
			{
				m_left = (rect.Width() - m_width) / 2;
				m_top = 0;
			}
			else if (m_allignmentType == allignmentType::ALLIGN_TOPRIGHT)
			{
				m_left = rect.Width() - m_width;
				m_top = 0;
			}
			else if (m_allignmentType == allignmentType::ALLIGN_MIDDLELEFT)
			{
				m_left = 0;
				m_top = (rect.Height() - m_height) / 2;
			}
			else if (m_allignmentType == allignmentType::ALLIGN_MIDDLECENTER)
			{
				m_left = (rect.Width() - m_width) / 2;
				m_top = (rect.Height() - m_height) / 2;
			}
			else if (m_allignmentType == allignmentType::ALLIGN_MIDDLERIGHT)
			{
				m_left = rect.Width() - m_width;
				m_top = (rect.Height() - m_height) / 2;
			}
			else if (m_allignmentType == allignmentType::ALLIGN_BOTTOMLEFT)
			{
				m_left = 0;
				m_top = rect.Height() - m_height;
			}
			else if (m_allignmentType == allignmentType::ALLIGN_BOTTOMCENTER)
			{
				m_left = (rect.Width() - m_width) / 2;
				m_top = rect.Height() - m_height;
			}
			else if (m_allignmentType == allignmentType::ALLIGN_BOTTOMRIGHT)
			{
				m_left = rect.Width() - m_width;
				m_top = rect.Height() - m_height;
			}
		}
		else if (_isZoomAtPtRst())  // PAN action.
		{
			m_left += m_panOffset.x;
			m_top += m_panOffset.y;
		}
		else if (m_zoomFactor > 1e-6)	// ZOOM action.
		{
			ScreenToClient(&m_zoomAtPt);
			if ((dx = (m_zoomAtPt.x - m_left)) < 1e-6)
			{
				m_zoomAtPt.x = (LONG)m_left;
				dx = 0;
			}
			else if (m_zoomAtPt.x > m_left + m_width - 1e-6)
			{
				m_zoomAtPt.x = (LONG)(m_left + (dx = m_width));
			}
			if ((dy = (m_zoomAtPt.y - m_top)) < 1e-6)
			{
				m_zoomAtPt.y = (LONG)m_top; dy = 0;
			}
			else if (m_zoomAtPt.y > m_top + m_height - 1e-6)
			{
				m_zoomAtPt.y = (LONG)(m_top + (dy = m_height));
			}

			w0 = m_width * m_zoomFactor; h0 = m_height * m_zoomFactor;
			if (w0 >= m_zoomMin && w0 <= m_zoomMax && h0 >= m_zoomMin && h0 <= m_zoomMax) // "ZOOM OUT" rectangle can't have a side smaller than m_ZoomMin pixels, "ZOOM iN" can't have a side greater than m_zoomMax pixels.
			{
				dx *= (w0 / m_width); dy *= (h0 / m_height);
				m_left = m_zoomAtPt.x - dx; m_top = m_zoomAtPt.y - dy; m_height = m_aspectRatio * (m_width = w0);
				::SendMessage(GetParent(Hwnd()), WMU_NOTIFY_ZOOM_CHANGED, 0, 0); // Send message to parent window to signalize ZOOM action event.
			}
		}
	}

	void RecalcPos()
	{
		CRect rect;
		GetClientRect(rect);
		double right = m_left + m_width, bottom = m_top + m_height;
		if (m_left > 0 || (m_left < 0 && m_width < rect.Width()))
		{
			m_left = 0;
		}
		else if (m_left < 0 && right < rect.right)
		{
			m_left = (double)rect.Width() - m_width;
		}
		if (m_top > 0 || (m_top < 0 && m_height < rect.Height()))
		{
			m_top = 0;
		}
		else if (m_top < 0 && bottom < rect.bottom)
		{
			m_top = (double)rect.Height() - m_height;
		}
	}
};


