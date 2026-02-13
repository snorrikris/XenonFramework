/*************************************************************************
				Class Declaration : CUGArrowType
**************************************************************************
	Source file : UGCTArrw.cpp
	Header file : UGCTArrw.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

  	Purpose
		The CUGArrowType is a basic cell type that allows to display
		left/right arrows and double arrows.

		To use this cell type all you have to do is set any cell's cell type
		property to UGCT_ARROW (or 3).

		Extended styles
			UGCT_ARROWRIGHT		- show a single arrow to the right
			UGCT_ARROWLEFT		- show a single arrow to the left
			UGCT_ARROWDRIGHT	- show a double arrow to the right
			UGCT_ARROWDLEFT		- show a double arrow to the left

**************************************************************************/
#ifndef _UGCTarrw_H_
#define _UGCTarrw_H_

// define cell type extensions
#define UGCT_ARROWRIGHT		BIT4
#define UGCT_ARROWLEFT		BIT5
#define	UGCT_ARROWDRIGHT	BIT6
#define	UGCT_ARROWDLEFT		BIT7

// define size of each triangle
#define UG_TRIANGLE_WIDTH	4
#define UG_TRIANGLE_HEIGHT	7

#include "UGCelTyp.h"

//CUGArrowType class declaration
class  CUGArrowType: public CUGCellType
{
public:
	CUGArrowType();
	~CUGArrowType();

	virtual LPCTSTR GetName();
	virtual LPCUGID GetUGID();
	virtual void OnDraw(CXeD2DRenderContext* pRctx, EXE_FONT eFont, RECT *rect,int col,long row,CUGCell *cell,int selected,int current) override;
	virtual void GetBestSize(CSize *size,CUGCell *cell);
};

#endif //_UGCTarrw_H_