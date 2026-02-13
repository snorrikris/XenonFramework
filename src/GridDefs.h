#pragma once
//#include "os_minimal.h"
//#include <afxole.h>			// Needed for drag and drop support in Grid

// The following defines are the same as UG_xxx defines in ugdefine.h - repeated here so
// AdoDataSource is not dependent on UltiMate Grid.
//sorting
#define GRID_SORT_ASCENDING		1
#define GRID_SORT_DESCENDING	2
//border style defines
#define GRID_BDR_LTHIN  		0x000000001
#define GRID_BDR_TTHIN  		0x000000002
#define GRID_BDR_RTHIN 			0x000000004 
#define GRID_BDR_BTHIN			0x000000008 //BIT3
#define GRID_BDR_LMEDIUM 		0x000000010
#define GRID_BDR_TMEDIUM  		0x000000020
#define GRID_BDR_RMEDIUM  		0x000000040
#define GRID_BDR_BMEDIUM  		0x000000080
#define GRID_BDR_LTHICK  		0x000000100
#define GRID_BDR_TTHICK  		0x000000200
#define GRID_BDR_RTHICK  		0x000000400
#define GRID_BDR_BTHICK  		0x000000800
#define GRID_BDR_RECESSED  		0x000001000
#define GRID_BDR_RAISED 		0x000002000
//Grid Sections - used by menu commands, etc
#define GRID_SECTION_GRID				1
#define GRID_SECTION_TOPHEADING			2
#define GRID_SECTION_SIDEHEADING		3
#define GRID_SECTION_CORNERBUTTON		4
#define GRID_SECTION_HSCROLL  			5
#define GRID_SECTION_VSCROLL  			6
#define GRID_SECTION_TAB				7


// Grid menu item identifiers
#define IDMG_RESETSIZE	    1008
#define IDMG_FIND		    1009
#define IDMG_SELECTALL	    1012
#define IDMG_EDITCOPY	    1013
#define IDMG_PRINT		    1015
#define IDMG_SELECTFONT		1021
#define IDMG_SET_FROM_FILTER 1022
#define IDMG_SET_TO_FILTER   1023
#define IDMG_GOTO_PREV_TIME_JUMP 1024
#define IDMG_GOTO_NEXT_TIME_JUMP 1025

#define IDMG_FIRST_CUSTOM	1100

// GetGridCell flags
#define GGC_BOLD			0x02
#define GGC_OWNED_CELL		0x04
#define GGC_MULTILINE_CELL	0x08
#define GGC_CHECKBOX		0x10
//#define GGC_COLORBTN		0x20

// used by grid
#define UG_EDIT_CUT			1
#define UG_EDIT_COPY        2
#define UG_EDIT_PASTE       3
#define UG_EDIT_UNDO        4
#define UG_EDIT_SELECTALL   5

//class CXeOleDropSource : public COleDropSource
//{
//public:
//	CXeOleDropSource();
//	virtual SCODE GiveFeedback(DROPEFFECT dropEffect);
//
//	//protected:
//	//	HCURSOR m_hCopyBottomCursor;	// Used when XE_DROPEFFECT_BOTTOM flag in dropEffect.
//	//	HCURSOR m_hMoveBottomCursor;	// Used when XE_DROPEFFECT_BOTTOM flag in dropEffect.
//	//	HCURSOR m_hUIlockedCursor;		// Used when XE_DROPEFFECT_UI_LOCKED flag in dropEffect.
//};

