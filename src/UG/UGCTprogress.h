/*************************************************************************
				Class Declaration : CUGProgressType
**************************************************************************
  	Purpose
		The CUGProgressType is a basic cell type that allows to display
		progress (0-100) as a background color in cell.

		The progress value is stored in the cell's m_param member.

		To use this cell type all you have to do is set any cell's cell type
		property to UGCT_PROGRESS.

**************************************************************************/
#pragma once

#include "UGCelTyp.h"

class  CUGProgressType: public CUGCellType
{
public:
	CUGProgressType();
	~CUGProgressType();

	virtual LPCTSTR GetName();
	virtual LPCUGID GetUGID();
	virtual void OnDraw(CXeD2DRenderContext* pRctx, EXE_FONT eFont, RECT *rect,int col,long row,CUGCell *cell,int selected,int current) override;
	virtual void DrawBackground(CXeD2DRenderContext* pRctx, RECT* rect, COLORREF backcolor);
};

