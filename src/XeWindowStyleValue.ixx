module;

#include "os_minimal.h"

export module Xe.WindowStyleValue;

export class XeWindowStyleValue
{
protected:
	DWORD m_dwStyle = 0, m_dwExStyle = 0;

public:
	XeWindowStyleValue() = default;
	XeWindowStyleValue(DWORD dwStyle, DWORD dwExStyle) : m_dwStyle(dwStyle), m_dwExStyle(dwExStyle) {}

	void Set(DWORD dwStyle, DWORD dwExStyle)
	{
		m_dwStyle = dwStyle, m_dwExStyle = dwExStyle;
	}

	bool Update(DWORD dwStyle, DWORD dwExStyle)
	{
		bool isChanged = m_dwStyle != dwStyle || m_dwExStyle != dwExStyle;
		m_dwStyle = dwStyle, m_dwExStyle = dwExStyle;
		return isChanged;
	}

#pragma region WindowStyleProperties
public:
	bool hasThickBorder() const { return m_dwStyle & WS_THICKFRAME; }

	bool hasBorder() const
	{
		return m_dwStyle & WS_BORDER || m_dwStyle & WS_DLGFRAME || m_dwStyle & WS_THICKFRAME
			|| m_dwExStyle & WS_EX_CLIENTEDGE || m_dwExStyle & WS_EX_DLGMODALFRAME
			|| m_dwExStyle & WS_EX_STATICEDGE || m_dwExStyle & WS_EX_WINDOWEDGE;
	}

	bool hasCaption() const { return (m_dwStyle & WS_CAPTION) == WS_CAPTION; }	// Window has caption?

	bool hasSysMenu() const { return m_dwStyle & WS_SYSMENU; }			// Window has system menu?

	bool isMaximized() const { return m_dwStyle & WS_MAXIMIZE; }		// Window is maximized?
	bool isMinimized() const { return m_dwStyle & WS_MINIMIZE; }		// Window is minimized?

	bool hasMinimizeBtn() const { return m_dwStyle & WS_MINIMIZEBOX; }	// Window has minimize button?
	bool hasMaximizeBtn() const { return m_dwStyle & WS_MAXIMIZEBOX; }	// Window has maximize button?

	// Window has Icon in the top left corner?
	bool hasIcon() const { return hasCaption() && hasThickBorder() && hasSysMenu(); }

	bool isNormalWithCaption() const
	{
		return !isMaximized() && !isMinimized() && hasCaption();
	}

	bool hasTabStop() const { return m_dwStyle & WS_TABSTOP; }
#pragma endregion WindowStyleProperties

#pragma region ListBoxStyles
public:
	bool hasLBS_COMBOBOX() const { return m_dwStyle & LBS_COMBOBOX; }
	bool hasLBS_DISABLENOSCROLL() const { return m_dwStyle & LBS_DISABLENOSCROLL; }
	bool hasLBS_EXTENDEDSEL() const { return m_dwStyle & LBS_EXTENDEDSEL; }
	bool hasLBS_HASSTRINGS() const { return m_dwStyle & LBS_HASSTRINGS; }
	bool hasLBS_MULTICOLUMN() const { return m_dwStyle & LBS_MULTICOLUMN; }
	bool hasLBS_MULTIPLESEL() const { return m_dwStyle & LBS_MULTIPLESEL; }
	bool hasLBS_NODATA() const { return m_dwStyle & LBS_NODATA; }
	bool hasLBS_NOINTEGRALHEIGHT() const { return m_dwStyle & LBS_NOINTEGRALHEIGHT; }
	bool hasLBS_NOREDRAW() const { return m_dwStyle & LBS_NOREDRAW; }
	bool hasLBS_NOSEL() const { return m_dwStyle & LBS_NOSEL; }
	bool hasLBS_NOTIFY() const { return m_dwStyle & LBS_NOTIFY; }
	bool hasLBS_OWNERDRAWFIXED() const { return m_dwStyle & LBS_OWNERDRAWFIXED; }
	bool hasLBS_OWNERDRAWVARIABLE() const { return m_dwStyle & LBS_OWNERDRAWVARIABLE; }
	bool hasLBS_SORT() const { return m_dwStyle & LBS_SORT; }
	bool hasLBS_USETABSTOPS() const { return m_dwStyle & LBS_USETABSTOPS; }
	bool hasLBS_WANTKEYBOARDINPUT() const { return m_dwStyle & LBS_WANTKEYBOARDINPUT; }

	bool hasLBS_UnsupportedStyle() const
	{
		return hasLBS_EXTENDEDSEL() || hasLBS_MULTICOLUMN()
			|| hasLBS_NODATA() || hasLBS_NOREDRAW() || hasLBS_NOSEL()
			|| hasLBS_OWNERDRAWFIXED() || hasLBS_OWNERDRAWVARIABLE()
			|| hasLBS_WANTKEYBOARDINPUT();
	}
	bool hasLBS_OwnerDrawStyle() const { return hasLBS_OWNERDRAWFIXED() || hasLBS_OWNERDRAWVARIABLE(); }
	/* Listbox styles
	LBS_COMBOBOX  0x8000L
	Notifies a list box that it is part of a combo box. This allows coordination between the two controls
	so that they present a unified UI. The combo box itself must set this style.
	If the style is set by anything but the combo box, the list box will regard itself incorrectly as a
	child of a combo box and a failure will result.

	LBS_DISABLENOSCROLL 0x1000L
	Shows a disabled horizontal or vertical scroll bar when the list box does not contain enough items to
	scroll. If you do not specify this style, the scroll bar is hidden when the list box does not contain
	enough items. This style must be used with the WS_VSCROLL or WS_HSCROLL style.

	LBS_EXTENDEDSEL 0x0800L
	Allows multiple items to be selected by using the SHIFT key and the mouse or special key combinations.

	LBS_HASSTRINGS 0x0040L
	Specifies that a list box contains items consisting of strings. The list box maintains the memory and
	addresses for the strings so that the application can use the LB_GETTEXT message to retrieve the text
	for a particular item. By default, all list boxes except owner-drawn list boxes have this style.
	You can create an owner-drawn list box either with or without this style.
	For owner-drawn list boxes without this style, the LB_GETTEXT message retrieves the value associated
	with an item (the item data).

	LBS_MULTICOLUMN 0x0200L
	Specifies a multi-column list box that is scrolled horizontally. The list box automatically calculates
	the width of the columns, or an application can set the width by using the LB_SETCOLUMNWIDTH message.
	If a list box has the LBS_OWNERDRAWFIXED style, an application can set the width when the list box
	sends the WM_MEASUREITEM message.
	A list box with the LBS_MULTICOLUMN style cannot scroll vertically it ignores any WM_VSCROLL messages
	it receives.
	The LBS_MULTICOLUMN and LBS_OWNERDRAWVARIABLE styles cannot be combined. If both are specified,
	LBS_OWNERDRAWVARIABLE is ignored.

	LBS_MULTIPLESEL 0x0008L
	Turns string selection on or off each time the user clicks or double-clicks a string in the list box.
	The user can select any number of strings.

	LBS_NODATA 0x2000L
	Specifies a no-data list box. Specify this style when the count of items in the list box will exceed
	one thousand. A no-data list box must also have the LBS_OWNERDRAWFIXED style, but must not have the
	LBS_SORT or LBS_HASSTRINGS style.
	A no-data list box resembles an owner-drawn list box except that it contains no string or bitmap data
	for an item. Commands to add, insert, or delete an item always ignore any specified item data;
	requests to find a string within the list box always fail. The system sends the WM_DRAWITEM message
	to the owner window when an item must be drawn. The itemID member of the DRAWITEMSTRUCT structure
	passed with the WM_DRAWITEM message specifies the line number of the item to be drawn. A no-data list
	box does not send a WM_DELETEITEM message.

	LBS_NOINTEGRALHEIGHT 0x0100L
	Specifies that the size of the list box is exactly the size specified by the application when it created
	the list box. Normally, the system sizes a list box so that the list box does not display partial items.
	For list boxes with the LBS_OWNERDRAWVARIABLE style, the LBS_NOINTEGRALHEIGHT style is always enforced.

	LBS_NOREDRAW 0x0004L
	Specifies that the list box's appearance is not updated when changes are made.
	To change the redraw state of the control, use the WM_SETREDRAW message.

	LBS_NOSEL 0x4000L
	Specifies that the list box contains items that can be viewed but not selected.

	LBS_NOTIFY 0x0001L
	Causes the list box to send a notification code to the parent window whenever the user clicks a list
	box item (LBN_SELCHANGE), double-clicks an item (LBN_DBLCLK), or cancels the selection (LBN_SELCANCEL).

	LBS_OWNERDRAWFIXED  0x0010L
	Specifies that the owner of the list box is responsible for drawing its contents and that the items
	in the list box are the same height. The owner window receives a WM_MEASUREITEM message when the list
	box is created and a WM_DRAWITEM message when a visual aspect of the list box has changed.

	LBS_OWNERDRAWVARIABLE 0x0020L
	Specifies that the owner of the list box is responsible for drawing its contents and that the items
	in the list box are variable in height. The owner window receives a WM_MEASUREITEM message for each
	item in the box when the list box is created and a WM_DRAWITEM message when a visual aspect of the
	list box has changed.
	This style causes the LBS_NOINTEGRALHEIGHT style to be enabled.
	This style is ignored if the LBS_MULTICOLUMN style is specified.

	LBS_SORT 0x0002L
	Sorts strings in the list box alphabetically.

	LBS_STANDARD (This style combines the LBS_NOTIFY, LBS_SORT, WS_VSCROLL, and WS_BORDER styles)
	Sorts strings in the list box alphabetically. The parent window receives a notification code whenever
	the user clicks a list box item, double-clicks an item, or or cancels the selection. The list box has
	a vertical scroll bar, and it has borders on all sides.

	LBS_USETABSTOPS 0x0080L
	Enables a list box to recognize and expand tab characters when drawing its strings. You can use the
	LB_SETTABSTOPS message to specify tab stop positions. The default tab positions are 32 dialog template
	units apart. Dialog template units are the device-independent units used in dialog box templates.
	To convert measurements from dialog template units to screen units (pixels), use the MapDialogRect
	function.

	LBS_WANTKEYBOARDINPUT 0x0400L
	Specifies that the owner of the list box receives WM_VKEYTOITEM messages whenever the user presses a
	key and the list box has the input focus. This enables an application to perform special processing
	on the keyboard input.
	*/
#pragma endregion ListBoxStyles

#pragma region ComboBoxStyle
public:
	bool hasCBS_SIMPLE() const { return (m_dwStyle & 0x3) == CBS_SIMPLE; }
	bool hasCBS_DROPDOWN() const { return (m_dwStyle & 0x3) == CBS_DROPDOWN; }
	bool hasCBS_DROPDOWNLIST() const { return (m_dwStyle & 0x3) == CBS_DROPDOWNLIST; }
	bool hasCBS_OWNERDRAWFIXED() const { return m_dwStyle & CBS_OWNERDRAWFIXED; }
	bool hasCBS_OWNERDRAWVARIABLE() const { return m_dwStyle & CBS_OWNERDRAWVARIABLE; }
	bool hasCBS_AUTOHSCROLL() const { return m_dwStyle & CBS_AUTOHSCROLL; }
	bool hasCBS_OEMCONVERT() const { return m_dwStyle & CBS_OEMCONVERT; }
	bool hasCBS_SORT() const { return m_dwStyle & CBS_SORT; }
	bool hasCBS_HASSTRINGS() const { return m_dwStyle & CBS_HASSTRINGS; }
	bool hasCBS_NOINTEGRALHEIGHT() const { return m_dwStyle & CBS_NOINTEGRALHEIGHT; }
	bool hasCBS_DISABLENOSCROLL() const { return m_dwStyle & CBS_DISABLENOSCROLL; }
	bool hasCBS_UPPERCASE() const { return m_dwStyle & CBS_UPPERCASE; }
	bool hasCBS_LOWERCASE() const { return m_dwStyle & CBS_LOWERCASE; }

	bool hasCBS_UnsupportedStyle() const
	{
		return hasCBS_SIMPLE() || hasCBS_OWNERDRAWFIXED() || hasCBS_OWNERDRAWVARIABLE()
			|| hasCBS_OEMCONVERT() || hasCBS_NOINTEGRALHEIGHT() || hasCBS_UPPERCASE()
			|| hasCBS_LOWERCASE();
	}
	/*
	CBS_SIMPLE            0x0001L
	Displays the list box at all times. The current selection in the list box is displayed in
	the edit control.

	CBS_DROPDOWN          0x0002L
	Similar to CBS_SIMPLE, except that the list box is not displayed unless the user selects
	an icon next to the edit control.

	CBS_DROPDOWNLIST      0x0003L
	Similar to CBS_DROPDOWN, except that the edit control is replaced by a static text item
	that displays the current selection in the list box.

	CBS_OWNERDRAWFIXED    0x0010L
	Specifies that the owner of the list box is responsible for drawing its contents and that
	the items in the list box are all the same height. The owner window receives a
	WM_MEASUREITEM message when the combo box is created and a WM_DRAWITEM message when a
	visual aspect of the combo box has changed.

	CBS_OWNERDRAWVARIABLE 0x0020L
	Specifies that the owner of the list box is responsible for drawing its contents and that
	the items in the list box are variable in height. The owner window receives a
	WM_MEASUREITEM message for each item in the combo box when you create the combo box and a
	WM_DRAWITEM message when a visual aspect of the combo box has changed.

	CBS_AUTOHSCROLL       0x0040L
	Automatically scrolls the text in an edit control to the right when the user types a
	character at the end of the line. If this style is not set, only text that fits within the
	rectangular boundary is allowed.

	CBS_OEMCONVERT        0x0080L
	Converts text entered in the combo box edit control from the Windows character set to the
	OEM character set and then back to the Windows character set. This ensures proper character
	conversion when the application calls the CharToOem function to convert a Windows string in
	the combo box to OEM characters. This style is most useful for combo boxes that contain file
	names and applies only to combo boxes created with the CBS_SIMPLE or CBS_DROPDOWN style.

	CBS_SORT              0x0100L
	Automatically sorts strings added to the list box.

	CBS_HASSTRINGS        0x0200L
	Specifies that an owner-drawn combo box contains items consisting of strings. The combo box
	maintains the memory and address for the strings so the application can use the CB_GETLBTEXT
	message to retrieve the text for a particular item.
	For accessibility issues, see Exposing Owner-Drawn Combo Box Items

	CBS_NOINTEGRALHEIGHT  0x0400L
	Specifies that the size of the combo box is exactly the size specified by the application
	when it created the combo box. Normally, the system sizes a combo box so that it does not
	display partial items.

	CBS_DISABLENOSCROLL   0x0800L
	Shows a disabled vertical scroll bar in the list box when the box does not contain enough
	items to scroll. Without this style, the scroll bar is hidden when the list box does not
	contain enough items.

	CBS_UPPERCASE         0x2000L
	Converts to uppercase all text in both the selection field and the list.

	CBS_LOWERCASE         0x4000L
	Converts to lowercase all text in both the selection field and the list.
	*/
#pragma endregion ComboBoxStyle

#pragma region ButtonControlStyles
public:
	// Return false if any style not supported.
	bool IsValidButtonStyle() const
	{
		DWORD type = m_dwStyle & BS_TYPEMASK;
		bool isAnyInvalid = (type == BS_USERBUTTON) || (type == BS_OWNERDRAW) || (m_dwStyle & BS_ICON);
		return !isAnyInvalid;
	}

	bool HasImageStyle() const { return m_dwStyle & BS_BITMAP; }

#define BS_DEFSPLITBUTTON       0x0000000DL
	bool IsDefaultButton() const
	{
		DWORD bs = m_dwStyle & BS_TYPEMASK;
		return bs == BS_DEFPUSHBUTTON || bs == BS_DEFSPLITBUTTON || bs == BS_DEFCOMMANDLINK;
	}

	bool IsCheckOrRadioBox() const { return IsCheckBox() || IsRadioButton(); }
	bool IsAutoCheckOrRadioBox() const { return IsAutoCheckBox() || IsAutoRadioButton(); }

	bool IsCheckBox() const { DWORD bs = m_dwStyle & BS_TYPEMASK; return bs == BS_CHECKBOX || bs == BS_AUTOCHECKBOX || bs == BS_3STATE || bs == BS_AUTO3STATE; }
	bool IsAutoCheckBox() const { DWORD bs = m_dwStyle & BS_TYPEMASK; return bs == BS_AUTOCHECKBOX || bs == BS_AUTO3STATE; }
	bool Is3stateCheckBox() const { DWORD bs = m_dwStyle & BS_TYPEMASK; return bs == BS_3STATE || bs == BS_AUTO3STATE; }
	bool IsRadioButton() const { DWORD bs = m_dwStyle & BS_TYPEMASK; return bs == BS_RADIOBUTTON || bs == BS_AUTORADIOBUTTON; }
	bool IsAutoRadioButton() const { DWORD bs = m_dwStyle & BS_TYPEMASK; return bs == BS_AUTORADIOBUTTON; }

	bool IsGroupBox() const { DWORD bs = m_dwStyle & BS_TYPEMASK; return bs == BS_GROUPBOX; }

	bool IsCheck_Radio_Url_or_GroupBox() const { return IsCheckOrRadioBox() || IsUrlButton() || IsGroupBox(); }

	bool IsSplitButton() const { DWORD bs = m_dwStyle & BS_TYPEMASK; return bs == BS_SPLITBUTTON || bs == BS_DEFSPLITBUTTON; }
	bool IsUrlButton() const { DWORD bs = m_dwStyle & BS_TYPEMASK; return bs == BS_COMMANDLINK || bs == BS_DEFCOMMANDLINK; }

	// Note - for group box, check box and radio button - H LEFT is assumed if horizontal text alignment not specified.
	// Note - for others - H CENTER is assumed if horizontal text alignment not specified.
	bool IsBS_LEFT() const { DWORD bs = m_dwStyle & BS_CENTER; return ((IsGroupBox() || IsCheckOrRadioBox()) && bs == 0) || bs == BS_LEFT; }
	bool IsBS_RIGHT() const { DWORD bs = m_dwStyle & BS_CENTER; return bs == BS_RIGHT; }
	bool IsBS_CENTER() const { DWORD bs = m_dwStyle & BS_CENTER; return (!IsGroupBox() && bs == 0) || bs == BS_CENTER; }

	// Note - V CENTER  is assumed if veritcal text alignment not specified.
	bool IsBS_TOP() const { DWORD bs = m_dwStyle & BS_VCENTER; return bs == BS_TOP; }
	bool IsBS_BOTTOM() const { DWORD bs = m_dwStyle & BS_VCENTER; return bs == BS_BOTTOM; }

	bool IsBS_MULTILINE() const { return m_dwStyle & BS_MULTILINE; }

	bool IsBS_LEFTTEXT() const { return m_dwStyle & BS_LEFTTEXT; }

	/*
	 * Button Control Styles
	BS_PUSHBUTTON       0x00000000L
	BS_DEFPUSHBUTTON    0x00000001L
	BS_CHECKBOX         0x00000002L
	BS_AUTOCHECKBOX     0x00000003L
	BS_RADIOBUTTON      0x00000004L
	BS_3STATE           0x00000005L
	BS_AUTO3STATE       0x00000006L
	BS_GROUPBOX         0x00000007L
	BS_USERBUTTON       0x00000008L Not supported (debug version ASSERTs).
	BS_AUTORADIOBUTTON  0x00000009L
	BS_PUSHBOX          0x0000000AL Ignored (not supported).
	BS_OWNERDRAW        0x0000000BL Not supported (debug version ASSERTs).
	BS_TYPEMASK         0x0000000FL Not a style but a bit mask.

	BS_LEFTTEXT         0x00000020L	Places text on the left side of the radio button or check box when combined
									with a radio button or check box style. Same as the BS_RIGHTBUTTON style.
	BS_RIGHTBUTTON      BS_LEFTTEXT

	BS_TEXT             0x00000000L
	BS_ICON             0x00000040L Not supported (debug version ASSERTs).
	BS_BITMAP           0x00000080L Supported (see comment regarding BM_SETIMAGE)

	// Text alignment styles (note - when none of the alignment styles specified - BS_CENTER | BS_VCENTER - is assumed).
	BS_LEFT             0x00000100L
	BS_RIGHT            0x00000200L
	BS_CENTER           0x00000300L
	BS_TOP              0x00000400L
	BS_BOTTOM           0x00000800L
	BS_VCENTER          0x00000C00L

	BS_PUSHLIKE         0x00001000L Ignored (not supported).
	BS_MULTILINE        0x00002000L
	BS_NOTIFY           0x00004000L
	BS_FLAT             0x00008000L Ignored (not supported).

	BS_SPLITBUTTON      0x0000000CL
	BS_DEFSPLITBUTTON   0x0000000DL
	BS_COMMANDLINK      0x0000000EL
	BS_DEFCOMMANDLINK   0x0000000FL


	BM_SETIMAGE message
	wParam = IMAGE_BITMAP (Note - IMAGE_ICON not supported)
	lParam = PID of the image (debug version ASSERTs if PID is not valid).

	Note - Microsoft specs are:
	The appearance of text and image or both on a button control depends on the BS_ICON and BS_BITMAP styles,
	and whether the BM_SETIMAGE message is called. The possible results are as follows:

	BS_ICON or BS_BITMAP Set?	BM_SETIMAGE Called?	Result
	Yes							Yes					Show icon only.
	No							Yes					Show icon and text.
	Yes							No					Show text only.
	No							No					Show text only


	Note - groupbox should have the WS_CLIPSIBLINGS style - otherwise the groupbox will obscure the controls inside it.

	*/
#pragma endregion ButtonControlStyles

#pragma region StaticControlStyles
public:
	bool HasSS_ImageStyle() const { DWORD bs = m_dwStyle & SS_TYPEMASK; return bs == SS_BITMAP || bs == SS_ICON; }
	bool HasSS_IconStyle() const { DWORD bs = m_dwStyle & SS_TYPEMASK; return bs == SS_ICON; }

	// Note - for group box - H LEFT is assumed if horizontal text alignment not specified.
	// Note - for others - H CENTER is assumed if horizontal text alignment not specified.
	bool IsSS_LEFT() const { DWORD bs = m_dwStyle & SS_TYPEMASK; return bs == SS_LEFT; }
	bool IsSS_RIGHT() const { DWORD bs = m_dwStyle & SS_TYPEMASK; return bs == SS_RIGHT; }
	bool IsSS_CENTER() const { DWORD bs = m_dwStyle & SS_TYPEMASK; return bs == SS_CENTER; }

	/*
	 * Static Control Styles

	SS_LEFT             0x00000000L
	SS_CENTER           0x00000001L
	SS_RIGHT            0x00000002L
	SS_ICON             0x00000003L
	SS_BLACKRECT        0x00000004L
	SS_GRAYRECT         0x00000005L
	SS_WHITERECT        0x00000006L
	SS_BLACKFRAME       0x00000007L
	SS_GRAYFRAME        0x00000008L
	SS_WHITEFRAME       0x00000009L
	SS_USERITEM         0x0000000AL
	SS_SIMPLE           0x0000000BL
	SS_LEFTNOWORDWRAP   0x0000000CL
	SS_OWNERDRAW        0x0000000DL
	SS_BITMAP           0x0000000EL
	SS_ENHMETAFILE      0x0000000FL
	SS_ETCHEDHORZ       0x00000010L
	SS_ETCHEDVERT       0x00000011L
	SS_ETCHEDFRAME      0x00000012L
	SS_TYPEMASK         0x0000001FL
	SS_REALSIZECONTROL  0x00000040L
	SS_NOPREFIX         0x00000080L Don't do "&" character translation
	SS_NOTIFY           0x00000100L
	SS_CENTERIMAGE      0x00000200L
	SS_RIGHTJUST        0x00000400L
	SS_REALSIZEIMAGE    0x00000800L
	SS_SUNKEN           0x00001000L
	SS_EDITCONTROL      0x00002000L
	SS_ENDELLIPSIS      0x00004000L
	SS_PATHELLIPSIS     0x00008000L
	SS_WORDELLIPSIS     0x0000C000L
	SS_ELLIPSISMASK     0x0000C000L
	*/
#pragma endregion StaticControlStyles

#pragma region EditControlStyles
public:
	bool isES_MultiLine() const { return (m_dwStyle & ES_MULTILINE) != 0; }
	bool isES_wantReturn() const { return (m_dwStyle & ES_WANTRETURN) != 0; }
	bool isES_readOnly() const { return (m_dwStyle & ES_READONLY) != 0; }

	/*
	// Edit Control Styles
	#define ES_LEFT             0x0000L
	#define ES_CENTER           0x0001L
	#define ES_RIGHT            0x0002L
	#define ES_MULTILINE        0x0004L
	#define ES_UPPERCASE        0x0008L
	#define ES_LOWERCASE        0x0010L
	#define ES_PASSWORD         0x0020L
	#define ES_AUTOVSCROLL      0x0040L
	#define ES_AUTOHSCROLL      0x0080L
	#define ES_NOHIDESEL        0x0100L
	#define ES_OEMCONVERT       0x0400L
	#define ES_READONLY         0x0800L
	#define ES_WANTRETURN       0x1000L
	#define ES_NUMBER           0x2000L

	// Edit Control Notification Codes
	#define EN_SETFOCUS         0x0100
	#define EN_KILLFOCUS        0x0200
	#define EN_CHANGE           0x0300
	#define EN_UPDATE           0x0400
	#define EN_ERRSPACE         0x0500
	#define EN_MAXTEXT          0x0501
	#define EN_HSCROLL          0x0601
	#define EN_VSCROLL          0x0602
	#define EN_ALIGN_LTR_EC     0x0700
	#define EN_ALIGN_RTL_EC     0x0701
	#define EN_BEFORE_PASTE     0x0800
	#define EN_AFTER_PASTE      0x0801

	// Edit control EM_SETMARGIN parameters
	#define EC_LEFTMARGIN       0x0001
	#define EC_RIGHTMARGIN      0x0002
	#define EC_USEFONTINFO      0xffff

	// wParam of EM_GET/SETIMESTATUS
	#define EMSIS_COMPOSITIONSTRING        0x0001

	// lParam for EMSIS_COMPOSITIONSTRING
	#define EIMES_GETCOMPSTRATONCE         0x0001
	#define EIMES_CANCELCOMPSTRINFOCUS     0x0002
	#define EIMES_COMPLETECOMPSTRKILLFOCUS 0x0004


	// Edit Control Messages
	#define EM_GETSEL               0x00B0
	#define EM_SETSEL               0x00B1
	#define EM_GETRECT              0x00B2
	#define EM_SETRECT              0x00B3
	#define EM_SETRECTNP            0x00B4
	#define EM_SCROLL               0x00B5
	#define EM_LINESCROLL           0x00B6
	#define EM_SCROLLCARET          0x00B7
	#define EM_GETMODIFY            0x00B8
	#define EM_SETMODIFY            0x00B9
	#define EM_GETLINECOUNT         0x00BA
	#define EM_LINEINDEX            0x00BB
	#define EM_SETHANDLE            0x00BC
	#define EM_GETHANDLE            0x00BD
	#define EM_GETTHUMB             0x00BE
	#define EM_LINELENGTH           0x00C1
	#define EM_REPLACESEL           0x00C2
	#define EM_GETLINE              0x00C4
	#define EM_LIMITTEXT            0x00C5
	#define EM_CANUNDO              0x00C6
	#define EM_UNDO                 0x00C7
	#define EM_FMTLINES             0x00C8
	#define EM_LINEFROMCHAR         0x00C9
	#define EM_SETTABSTOPS          0x00CB
	#define EM_SETPASSWORDCHAR      0x00CC
	#define EM_EMPTYUNDOBUFFER      0x00CD
	#define EM_GETFIRSTVISIBLELINE  0x00CE
	#define EM_SETREADONLY          0x00CF
	#define EM_SETWORDBREAKPROC     0x00D0
	#define EM_GETWORDBREAKPROC     0x00D1
	#define EM_GETPASSWORDCHAR      0x00D2
	#define EM_SETMARGINS           0x00D3
	#define EM_GETMARGINS           0x00D4
	#define EM_SETLIMITTEXT         EM_LIMITTEXT   // win40 Name change
	#define EM_GETLIMITTEXT         0x00D5
	#define EM_POSFROMCHAR          0x00D6
	#define EM_CHARFROMPOS          0x00D7
	#define EM_SETIMESTATUS         0x00D8
	#define EM_GETIMESTATUS         0x00D9
	#define EM_ENABLEFEATURE        0x00DA

	// EM_ENABLEFEATURE options
	typedef enum {
		EDIT_CONTROL_FEATURE_ENTERPRISE_DATA_PROTECTION_PASTE_SUPPORT = 0,
		EDIT_CONTROL_FEATURE_PASTE_NOTIFICATIONS = 1,
	} EDIT_CONTROL_FEATURE;

	// EDITWORDBREAKPROC code values
	#define WB_LEFT            0
	#define WB_RIGHT           1
	#define WB_ISDELIMITER     2
	*/
#pragma endregion EditControlStyles

#pragma region TreeControlStyles
public:
	bool hasTVS_HASBUTTONS() const { return m_dwStyle & TVS_HASBUTTONS; }
	bool hasTVS_HASLINES() const { return m_dwStyle & TVS_HASLINES; }
	bool hasTVS_LINESATROOT() const { return m_dwStyle & TVS_LINESATROOT; }
	bool hasTVS_EDITLABELS() const { return m_dwStyle & TVS_EDITLABELS; }
	bool hasTVS_DISABLEDRAGDROP() const { return m_dwStyle & TVS_DISABLEDRAGDROP; }
	bool hasTVS_SHOWSELALWAYS() const { return m_dwStyle & TVS_SHOWSELALWAYS; }
	bool hasTVS_RTLREADING() const { return m_dwStyle & TVS_RTLREADING; }
	bool hasTVS_NOTOOLTIPS() const { return m_dwStyle & TVS_NOTOOLTIPS; }
	bool hasTVS_CHECKBOXES() const { return m_dwStyle & TVS_CHECKBOXES; }
	bool hasTVS_TRACKSELECT() const { return m_dwStyle & TVS_TRACKSELECT; }
	bool hasTVS_SINGLEEXPAND() const { return m_dwStyle & TVS_SINGLEEXPAND; }
	bool hasTVS_INFOTIP() const { return m_dwStyle & TVS_INFOTIP; }
	bool hasTVS_FULLROWSELECT() const { return m_dwStyle & TVS_FULLROWSELECT; }
	bool hasTVS_NOSCROLL() const { return m_dwStyle & TVS_NOSCROLL; }
	bool hasTVS_NONEVENHEIGHT() const { return m_dwStyle & TVS_NONEVENHEIGHT; }
	bool hasTVS_NOHSCROLL() const { return m_dwStyle & TVS_NOHSCROLL; }

	bool hasTVS_UnsupportedStyle() const
	{
		return hasTVS_EDITLABELS() || hasTVS_RTLREADING() || hasTVS_CHECKBOXES()
			|| hasTVS_TRACKSELECT() || hasTVS_SINGLEEXPAND() || hasTVS_INFOTIP()
			|| hasTVS_FULLROWSELECT() || hasTVS_NOSCROLL() || hasTVS_NONEVENHEIGHT()
			|| hasTVS_NOHSCROLL();
	}
	/* Tree control styles
	TVS_HASBUTTONS          0x0001
	Displays plus (+) and minus (-) buttons next to parent items. The user clicks the buttons to
	expand or collapse a parent item's list of child items. To include buttons with items at the
	root of the tree view, TVS_LINESATROOT must also be specified.
	TVS_HASLINES            0x0002
	Uses lines to show the hierarchy of items.
	TVS_LINESATROOT         0x0004
	Uses lines to link items at the root of the tree-view control. This value is ignored if
	TVS_HASLINES is not also specified.
	TVS_EDITLABELS          0x0008
	Allows the user to edit the labels of tree-view items.
	TVS_DISABLEDRAGDROP     0x0010
	Prevents the tree-view control from sending TVN_BEGINDRAG notification codes.
	TVS_SHOWSELALWAYS       0x0020
	Causes a selected item to remain selected when the tree-view control loses focus.
	TVS_RTLREADING          0x0040
	Version 4.70. Causes text to be displayed from right-to-left (RTL). Usually, windows display
	text left-to-right (LTR). Windows can be mirrored to display languages such as Hebrew or
	Arabic that read RTL. Typically, tree-view text is displayed in the same direction as the
	text in its parent window. If TVS_RTLREADING is set, tree-view text reads in the opposite
	direction from the text in the parent window.
	TVS_NOTOOLTIPS          0x0080
	Version 4.70. Disables tooltips.
	TVS_CHECKBOXES          0x0100
	Version 4.70. Enables check boxes for items in a tree-view control. A check box is displayed
	only if an image is associated with the item. When set to this style, the control effectively
	uses DrawFrameControl to create and set a state image list containing two images. State image
	1 is the unchecked box and state image 2 is the checked box. Setting the state image to zero
	removes the check box altogether. For more information, see Working with state image indexes.
	Version 5.80. Displays a check box even if no image is associated with the item.
	Once a tree-view control is created with this style, the style cannot be removed. Instead,
	you must destroy the control and create a new one in its place. Destroying the tree-view
	control does not destroy the check box state image list. You must destroy it explicitly.
	Get the handle to the state image list by sending the tree-view control a TVM_GETIMAGELIST
	message. Then destroy the image list with ImageList_Destroy.
	If you want to use this style, you must set the TVS_CHECKBOXES style with SetWindowLong after
	you create the treeview control, and before you populate the tree. Otherwise, the checkboxes
	might appear unchecked, depending on timing issues.
	TVS_TRACKSELECT         0x0200
	Version 4.70. Enables hot tracking in a tree-view control.
	TVS_SINGLEEXPAND        0x0400
	Version 4.71. Causes the item being selected to expand and the item being unselected to
	collapse upon selection in the tree view. If the mouse is used to single-click the selected
	item and that item is closed, it will be expanded. If the user holds down the CTRL key while
	selecting an item, the item being unselected will not be collapsed.
	Version 5.80. Causes the item being selected to expand and the item being unselected to
	collapse upon selection in the tree view. If the user holds down the CTRL key while selecting
	an item, the item being unselected will not be collapsed.
	TVS_INFOTIP             0x0800
	Version 4.71. Obtains tooltip information by sending the TVN_GETINFOTIP notification.
	TVS_FULLROWSELECT       0x1000
	Version 4.71. Enables full-row selection in the tree view. The entire row of the selected item
	is highlighted, and clicking anywhere on an item's row causes it to be selected. This style
	cannot be used in conjunction with the TVS_HASLINES style.
	TVS_NOSCROLL            0x2000
	Version 5.80. Disables horizontal scrolling in the control. The control will not display any
	horizontal scroll bars.
	TVS_NONEVENHEIGHT       0x4000
	Version 4.71 Sets the height of the items to an odd height with the TVM_SETITEMHEIGHT message.
	By default, the height of items must be an even value.
	TVS_NOHSCROLL           0x8000  // TVS_NOSCROLL overrides this
	Version 4.71. Disables both horizontal and vertical scrolling in the control. The control will
	not display any scroll bars.
	*/
#pragma endregion TreeControlStyles
};

