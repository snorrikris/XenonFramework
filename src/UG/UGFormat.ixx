module;

/***********************************************
	Ultimate Grid
	Copyright 1994 - 2002 Dundas Software Ltd.


	class CUGFormat
************************************************/
/*************************************************************************
				Class Declaration : CUGCellFormat
**************************************************************************
	Source file : ugformat.cpp
	Header file : ugformat.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		This class is used for the formating the
		cells data for display and/or for editing.
*************************************************************************/



#include "../os_minimal.h"

export module Xe.UGFormat;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CUGCell;

export class CUGCellFormat //: public CObject
{
public:

	/***********************************************
	************************************************/
	CUGCellFormat() {}

	/***********************************************
	************************************************/
	virtual ~CUGCellFormat() {}

	/***********************************************
	************************************************/
	virtual void ApplyDisplayFormat(CUGCell* cell) {
		UNREFERENCED_PARAMETER(cell);
	}

	/***********************************************
	return
		0 - information valid
		1 - information invalid
	************************************************/
	virtual int ValidateCellInfo(CUGCell* cell) {
		UNREFERENCED_PARAMETER(cell);
		return 0;
	}
};

