module;

/*************************************************************************
				Class Implementation : CUGDataSource
**************************************************************************
	Source file : UGDtaSrc.cpp
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved
*************************************************************************/
/*************************************************************************
				Class Declaration : CUGDataSource
**************************************************************************
	Source file : UGDtaSrc.cpp
	Header file : UGDtaSrc.h
	Copyright © Dundas Software Ltd. 1994 - 2002, All Rights Reserved

	Purpose
		The CUGDataSource class is used by the grid
		as standard interface between the grid
		and its data.  The Ultimate Grid relies
		on CUGDataSource derived class to provide
		it with all of the information that needs
		to be displayed.

		Datasources can be practically anything
		i.e.	arrays
				linked lists
				databases
				flat files
				real-time feeds (sensors)
				calculations

	Details
		This is a base class which all other datasources
		must be derived from. By defining a standard
		interface to the data, an abstract layer is
		created which allows the uderlying data to
		come from any source, plus allows the datasource
		to be changed without any code re-write.

		At the minimum only ONE virtual function must
		be overwitten it is the GetCell. GetCell is
		called by the grid when it needs information
		about a particular cell.

		Even though the grid generally works on a cell by
		cell basis, many datasource (such as databases)
		tend to work on a row by row basis. To allow data
		to be written to the datasource in this manner
		transaction writing can be used within a datasource
		by overwritting the transaction functions.

		If a derived datasource cannot return the number
		of rows that is contains, then overwrite
		the OnHitBottom virtual function. This allows for
		the grid to ask the datasource for new rows on the fly.

		Stanard return values from a datasource are
			UG_NA		- not implemented (-1)
			UG_SUCCESS	- success (0)
			1 and up	- error codes
*************************************************************************/

#include "../os_minimal.h"
#include <string>

#include "ugdefine.h"
//#include "UGDtaSrc.h"

export module Xe.UGDtaSrc;

//import Xe.UGGridInfoIF;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//defines for the IsSupported function
//#define UGDS_SUPPORT_INSERTROW		BIT0
//#define UGDS_SUPPORT_APPENDROW		BIT1
//#define UGDS_SUPPORT_INSERTCOL		BIT2
//#define UGDS_SUPPORT_APPENDCOL		BIT3
//#define UGDS_SUPPORT_DELETEROW		BIT4
//#define UGDS_SUPPORT_DELETECOL		BIT5
//#define UGDS_SUPPORT_EMPTY			BIT6
//#define UGDS_SUPPORT_CLOSE			BIT7
//#define UGDS_SUPPORT_SAVE			BIT8
//#define UGDS_SUPPORT_OPEN			BIT9
//#define UGDS_SUPPORT_SETPASSWORD	BIT10
//#define UGDS_SUPPORT_FIND			BIT11
//#define UGDS_SUPPORT_SORT			BIT12
//#define UGDS_SUPPORT_GETNUMROWS		BIT13
//#define UGDS_SUPPORT_GETNUMCOLS		BIT14
//#define UGDS_SUPPORT_GETCOLNAME		BIT15
//#define UGDS_SUPPORT_GETCOLTYPE		BIT16
//#define UGDS_SUPPORT_SETCELL		BIT17
//#define UGDS_SUPPORT_GETCELL		BIT18
//#define UGDS_SUPPORT_TRANSACTIONS	BIT19
//#define UGDS_SUPPORT_HITBOTTON		BIT20
//#define UGDS_SUPPORT_HITTOP			BIT21
//#define UGDS_SUPPORT_GETPREVCOL		BIT22

//class CUGCell;
//class CUGGridInfoIF;

