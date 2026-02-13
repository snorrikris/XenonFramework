/*************************************************************************
				Class Declaration : CUGEdit
**************************************************************************
	Source file : UGEdit.cpp
	Header file : UGEdit.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		The Ultimate grid uses the CUGEdit class
		as the default edit control.  An instance
		of this class is created every time an instance
		of the Ultimate Grid is created.

		It is possible to specify new edit control
		for the Ultimate Grid to use, you can do this
		either with the use of the OnEditStart notification
		or SetNewEditClass.  Each of these methods has
		its benefits and please refer to the documentation
		for more information.

	Key features
		-This control automatically expands to the
		 right as text is entered, once the control
		 reaches the right side of the grid then it
		 expands downward until it reaches the bottom.
		 Once it reaches the bottom then it will start
		 scrolling text as it is entered.
		-When editing first starts the control also
		 automatically expands to fit the inital text.
*************************************************************************/
#ifndef _UGEdit_H_
#define _UGEdit_H_
#include <tuple>

class CUGGridInfo;

class  CUGEdit
{
public:
	CUGGridInfo* m_GI = nullptr;
	HWND m_hWnd = 0;
	
protected:
	// based on user action destination cell needs to be set 
	int m_continueCol;
	long m_continueRow;
	BOOL m_cancel;
	BOOL m_continueFlag;

	// member variable that keeps track of the last pressed key
	UINT m_lastKey;

	clock_t m_lastDblClickTicks = 0;

	bool m_isUseScintilla = false;

public:
	CUGEdit();
	virtual ~CUGEdit();

	bool CreateUGEdit(const CRect& rc, HWND hParentWnd, UINT uID, bool isUseScintilla);

	// The last key user pressed - usefult to know if ESC cancelled the edit.
	UINT GetLastKey() const { return m_lastKey; }

	BOOL SetAutoSize(BOOL state);

	std::tuple<bool, LRESULT> _OnMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT _OnSetText(WPARAM wParam, LPARAM lParam);
	void _SetText(const void* pText, size_t text_len, bool isReadOnly);
	void _UpdateScintillaColors();
	void _UpdateScintillaFont();
	void _SetScintillaFont(const wchar_t* pFontName, int size);


};

#endif //#ifndef _UGEdit_H_
