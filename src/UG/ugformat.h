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
#ifndef _ugformat_H_
#define _ugformat_H_

class CUGCell;

class  CUGCellFormat //: public CObject
{
public:
	CUGCellFormat();
	virtual ~CUGCellFormat();

	virtual void ApplyDisplayFormat(CUGCell *cell);
	
	virtual int  ValidateCellInfo(CUGCell *cell);
};

#endif	// _ugformat_H_