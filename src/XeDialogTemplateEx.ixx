module;
#include <windows.h>
#include <string>
#include <vector>

export module Xe.DialogTemplateEx;

// See: https://learn.microsoft.com/en-us/windows/win32/dlgbox/dlgtemplateex
// and: https://devblogs.microsoft.com/oldnewthing/20040623-00/?p=38753

#pragma pack(push)
#pragma pack(1)
struct DLGTEMPLATEEX_1
{
	WORD      dlgVer;		// version number - always 1
	WORD      signature;	// always 0xFFFF
	DWORD     helpID;		// help ID. The help context identifier for the dialog box window.
							// When the system sends a WM_HELP message, it passes this value
							// in the wContextId member of the HELPINFO structure.
	DWORD     exStyle;		// window extended style. This member is not used when creating
							// dialog boxes, but applications that use dialog box templates
							// can use it to create other types of windows.
	DWORD     style;		// dialog style. This member can be a combination of window style values and dialog
							// box style values.
							// If style includes the DS_SETFONT or DS_SHELLFONT dialog box style, the DLGTEMPLATEEX
							// header of the extended dialog box template contains four additional members 
							// (pointsize, weight, italic, and typeface) that describe the font to use for the
							// text in the client area and controls of the dialog box. If possible, the system 
							// creates a font according to the values specified in these members. Then the 
							// system sends a WM_SETFONT message to the dialog box and to each control to provide
							// a handle to the font.
	WORD      cDlgItems;	// number of controls in this dialog
							// How many DLGITEMTEMPLATEEX structs follow 'this' header.

							// x, y, cx, cy, in dialog box units, position and size the upper-left corner of the dialog box.
	short     x;			// x-coordinate.
	short     y;			// y-coordinate
	short     cx;			// width
	short     cy;			// height

	//sz_Or_Ord menu;
	// A variable-length array of 16-bit elements that identifies a menu resource for the dialog
	// box. If the first element of this array is 0x0000, the dialog box has no menu and the
	// array has no other elements. If the first element is 0xFFFF, the array has one additional
	// element that specifies the ordinal value of a menu resource in an executable file. If the
	// first element has any other value, the system treats the array as a null-terminated
	// Unicode string that specifies the name of a menu resource in an executable file.

	//sz_Or_Ord windowClass;
	// A variable-length array of 16-bit elements that identifies the window class of the dialog
	// box. If the first element of the array is 0x0000, the system uses the predefined dialog
	// box class for the dialog box and the array has no other elements. If the first element is
	// 0xFFFF, the array has one additional element that specifies the ordinal value of a
	// predefined system window class. If the first element has any other value, the system
	// treats the array as a null-terminated Unicode string that specifies the name of a
	// registered window class.

	//WCHAR title[titleLen];
	// The title of the dialog box. If the first element of this array is 0x0000, the dialog
	// box has no title and the array has no other elements.

	// DLGTEMPLATEEX_2 starts here
	//WORD      pointsize;
	//WORD      weight;
	//BYTE      italic;
	//BYTE      charset;
	//WCHAR     typeface[stringLen];

	bool Has_DS_fontStyle() const
	{
		return (style & DS_SETFONT) != 0 || (style & DS_SHELLFONT) != 0;
	}
};

// This struct is present only if the style member specifies DS_SETFONT or DS_SHELLFONT.
struct DLGTEMPLATEEX_2
{
	WORD      pointsize;	// The point size of the font to use for the text in the dialog
							// box and its controls.
	WORD      weight;		// The weight of the font. Note that, although this can be any
							// of the values listed for the lfWeight member of the LOGFONT
							// structure, any value that is used will be automatically
							// changed to FW_NORMAL. (0x0000 = FW_DONTCARE)
	BYTE      italic;		// Indicates whether the font is italic. If this value is TRUE,
							// the font is italic.
	BYTE      charset;		// The character set to be used. For more information, see the
							// lfcharset member of LOGFONT. (0x01 = DEFAULT_CHARSET)

	//WCHAR typeface[stringLen];	// The name of the typeface for the font.

	// Note - the point, weight, italic and character set are all passed to the CreateFont function.
};

// After the header (DLGTEMPLATEEX) come the dialog item templates, each of which must be
// aligned on a DWORD boundary.

struct DLGITEMTEMPLATEEX 
{
	DWORD     helpID;		// The help context identifier for the control. When the system
							// sends a WM_HELP message, it passes the helpID value in the
							// dwContextId member of the HELPINFO structure.
	DWORD     exStyle;		// The extended styles for a window. This member is not used to 
							// create controls in dialog boxes, but applications that use
							// dialog box templates can use it to create other types of windows.
	DWORD     style;		// The style of the control. This member can be a combination of
							// window style values (such as WS_BORDER) and one or more of the
							// control style values (such as BS_PUSHBUTTON and ES_LEFT).

	// The x, y, cx, cy, in dialog box units, position and size of the upper-left corner of
	// the control. This coordinate is always relative to the upper-left corner of the
	// dialog box's client area.
	short     x;
	short     y;
	short     cx;
	short     cy;
	DWORD     id;			// The control identifier.
	//sz_Or_Ord windowClass;// A variable-length array of 16-bit elements that specifies the
							// window class of the control. If the first element of this array
							// is any value other than 0xFFFF, the system treats the array as
							// a null-terminated Unicode string that specifies the name of a 
							// registered window class. If the first element is 0xFFFF, the 
							// array has one additional element that specifies the ordinal 
							// value of a predefined system class.The ordinal can be one of 
							// the following atom values:
							// 0x0080 Button, 0x0081 Edit, 0x0082 Static, 0x0083 List box,
							// 0x0084 Scroll bar, 0x0085 Combo box
	//sz_Or_Ord title;		// A variable-length array of 16-bit elements that contains the
							// initial text or resource identifier of the control. If the
							// first element of this array is 0xFFFF, the array has one
							// additional element that specifies the ordinal value of a 
							// resource, such as an icon, in an executable file. You can use 
							// a resource identifier for controls, such as static icon 
							// controls, that load and display an icon or other resource
							// rather than text. If the first element is any value other than
							// 0xFFFF, the system treats the array as a null-terminated
							// Unicode string that specifies the initial text.
	//WORD      extraCount;	// The number of bytes of creation data that follow this member.
							// If this value is greater than zero, the creation data begins
							// at the next WORD boundary. This creation data can be of any
							// size and format. The control's window procedure must be able to
							// interpret the data. When the system creates the control, it
							// passes a pointer to this data in the lParam parameter of the 
							// WM_CREATE message that it sends to the control.
};
#pragma pack(pop)

export struct CXeDlgTemplateHeader
{
	DLGTEMPLATEEX_1		s1{ 0 };

	std::wstring		menu_name;
	WORD				menu_ordinal = 0;

	std::wstring		windowClass;
	WORD				class_ordinal;

	std::wstring		title;

	DLGTEMPLATEEX_2		font{ 0 };

	std::wstring		typeface;
};

export struct CXeDlgTemplateItem
{
	DLGITEMTEMPLATEEX		s{ 0 };
	std::wstring			windowClass;
	WORD					class_ordinal;

	std::wstring			title;
	WORD					resource_ordinal; // ICON?

	WORD					extraCount;
	std::vector<uint8_t>	extraData;
};

export class CXeDialogTemplateEx
{
public:
	CXeDlgTemplateHeader			m_hdr{ 0 };
	std::vector<CXeDlgTemplateItem>	m_items;

	std::wstring m_lastError;

	/// <summary>
	/// Load DLGITEMTEMPLATEEX from resources. Returns true if parse ok.
	/// </summary>
	bool LoadTemplateFromResources(HINSTANCE hInst, const wchar_t* pszTemplate)
	{
		bool isOk = false;
		HRSRC hrsrc = ::FindResource(hInst, pszTemplate, RT_DIALOG);
		if (hrsrc)
		{
			HGLOBAL hglob = ::LoadResource(hInst, hrsrc);
			if (hglob)
			{
				void* pTemplate = ::LockResource(hglob);
				DWORD dwSize = ::SizeofResource(hInst, hrsrc);
				if (pTemplate && dwSize)
				{
					isOk = _ParseTemplate(pTemplate, dwSize);
				}
				else
				{
					m_lastError = L"Failed to lock resource or size = 0";
				}
				FreeResource(hglob);
			}
			else
			{
				m_lastError = L"Failed to load resource";
			}
		}
		else
		{
			m_lastError = L"Resource not found";
		}
		return isOk;
	}

	/// <summary>
	/// Load DLGITEMTEMPLATEEX from resources. Returns true if parse ok.
	/// </summary>
	bool LoadTemplateFromResources(HINSTANCE hInst, uint64_t dlg_id)
	{
		return LoadTemplateFromResources(hInst, MAKEINTRESOURCE(dlg_id));
	}

protected:
	bool _ParseTemplate(void* pTemplate, DWORD dwSize)
	{
		WORD* pEnd = (WORD*)((uint8_t*)pTemplate + dwSize);

		DLGTEMPLATEEX_1* pS1 = (DLGTEMPLATEEX_1*)pTemplate;
		if (pS1->dlgVer != 1 || pS1->signature != 0xFFFF)
		{
			m_lastError = L"Signature error";
			return false;
		}

		m_hdr.s1 = *pS1;

		WORD* pNext = (WORD*)(pS1 + 1);

		if (*pNext != 0)	// Has menu?
		{
			if (*pNext == 0xFFFF)	// Has menu ordinal?
			{
				++pNext;
				m_hdr.menu_ordinal = *pNext++;
			}
			else	// Has menu name
			{
				if (!_GetString((wchar_t*)pNext, pTemplate, dwSize, m_hdr.menu_name, &pNext))
				{
					m_lastError = L"Parse menu name failed";
					return false;
				}
			}
		}
		else
		{
			++pNext;
		}

		if (*pNext != 0)	// Has windowClass?
		{
			if (*pNext == 0xFFFF)	// Has class ordinal?
			{
				++pNext;
				m_hdr.class_ordinal = *pNext++;
			}
			else	// Has class name
			{
				if (!_GetString((wchar_t*)pNext, pTemplate, dwSize, m_hdr.windowClass, &pNext))
				{
					m_lastError = L"Parse class name failed";
					return false;
				}
			}
		}
		else
		{
			++pNext;
		}

		if (*pNext != 0)	// Has title?
		{
			if (!_GetString((wchar_t*)pNext, pTemplate, dwSize, m_hdr.title, &pNext))
			{
				m_lastError = L"Parse title failed";
				return false;
			}
		}
		else
		{
			++pNext;
		}

		// Has font specs?
		if (m_hdr.s1.Has_DS_fontStyle())
		{
			DLGTEMPLATEEX_2* pS2 = (DLGTEMPLATEEX_2*)pNext;
			m_hdr.font = *pS2;

			pNext = (WORD*)(pS2 + 1);
			if (!_GetString((wchar_t*)pNext, pTemplate, dwSize, m_hdr.typeface, &pNext))
			{
				m_lastError = L"Parse typeface name failed";
				return false;
			}
		}

		WORD cItems = m_hdr.s1.cDlgItems;
		while (cItems)	// Has items?
		{
			// Align on DWORD boundary.
			if (((uint64_t)pNext & 0x3) != 0)
			{
				++pNext;
			}
			if (pNext >= pEnd)
			{
				m_lastError = L"Dialog items missing from struct";
				return false;
			}
			CXeDlgTemplateItem item{ 0 };
			DLGITEMTEMPLATEEX* pItm = (DLGITEMTEMPLATEEX*)pNext;
			item.s = *pItm;

			pNext = (WORD*)(pItm + 1);

			if (*pNext != 0)	// Has windowClass?
			{
				if (*pNext == 0xFFFF)	// Has class ordinal?
				{
					++pNext;
					item.class_ordinal = *pNext++;
				}
				else	// Has class name
				{
					if (!_GetString((wchar_t*)pNext, pTemplate, dwSize, item.windowClass, &pNext))
					{
						m_lastError = L"Parse item class name failed";
						return false;
					}
				}
			}
			else
			{
				++pNext;
			}

			if (*pNext != 0)	// Has title?
			{
				if (*pNext == 0xFFFF)	// Has resource ordinal (icon)?
				{
					++pNext;
					item.resource_ordinal = *pNext++;
				}
				else	// Has string
				{
					if (!_GetString((wchar_t*)pNext, pTemplate, dwSize, item.title, &pNext))
					{
						m_lastError = L"Parse item title failed";
						return false;
					}
				}
			}
			else
			{
				++pNext;
			}

			if (*pNext != 0)	// Has extra data?
			{
				item.extraCount = *pNext++;
				if (!_CopyExtraData((uint8_t*)pNext, item.extraCount, pTemplate, dwSize,
						item.extraData, &pNext))
				{
					m_lastError = L"Copy item extra data failed";
					return false;
				}
			}
			else
			{
				++pNext;
			}

			m_items.push_back(item);

			--cItems;
		}
		return true;
	}

	bool _GetString(wchar_t* pWstring, void* pTemplate, DWORD dwSize,
			std::wstring& str, WORD** pNext)
	{
		wchar_t* pEnd = (wchar_t*)((uint8_t*)pTemplate + dwSize);
		if (pWstring >= pEnd)
		{
			return false;
		}
		wchar_t* pStrBeg = pWstring;
		while (*pWstring != 0)
		{
			if (pWstring >= pEnd)
			{
				return false;
			}
			++pWstring;
		}
		size_t str_len = pWstring - pStrBeg;
		if (str_len == 0)
		{
			return false;
		}
		++pWstring;
		*pNext = (WORD*)pWstring;
		str.assign(pStrBeg);
		return true;
	}

	bool _CopyExtraData(uint8_t* pData, size_t len, void* pTemplate, DWORD dwSize,
		std::vector<uint8_t>& data, WORD** pNext)
	{
		uint8_t* pEnd = (uint8_t*)((uint8_t*)pTemplate + dwSize);
		if (pData >= pEnd)
		{
			return false;
		}
		uint8_t* pDataBeg = pData;
		while (len > 0)
		{
			if (pData >= pEnd)
			{
				return false;
			}
			data.push_back(*pData++);
			--len;
		}

		// Align on next DWORD boundary.
		while (((uint64_t)pData & 0x3) != 0)
		{
			++pData;
		}
		*pNext = (WORD*)pData;
		return true;
	}
};

