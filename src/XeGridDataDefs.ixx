module;

#include <functional>

export module Xe.GridDataDefs;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class eGridAction
{
	NO_ACTION = 0,
	REDRAW_ROW = 1,	// Redraw current row.
	REDRAW_ALL = 2,
	SET_GRID_FROM_DS = 3,
	UPDATE_ROW_COUNT = 4
};

export enum class EGRIDOP
{
	eGridNfMenuCmd,
	//eGridNfShowFind,	// Show Find UI (in XeFileView)
	//eGridNfFindResult,	// Last search cmd (next/prev) result. (m_bFlag has status - found/not found)
};

export typedef std::function<void(int col, long row)> OnEditFinishedCallbackFunc;

