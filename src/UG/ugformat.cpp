/***********************************************
	Ultimate Grid
	Copyright 1994 - 2002 Dundas Software Ltd.


	class CUGFormat
************************************************/



#include "../os_minimal.h"
#include "UGFormat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/***********************************************
************************************************/
CUGCellFormat::CUGCellFormat(){
}

/***********************************************
************************************************/
CUGCellFormat::~CUGCellFormat(){
}

/***********************************************
************************************************/
void CUGCellFormat::ApplyDisplayFormat(CUGCell *cell){
	UNREFERENCED_PARAMETER(cell);
}
	
/***********************************************
return 
	0 - information valid
	1 - information invalid
************************************************/
int CUGCellFormat::ValidateCellInfo(CUGCell *cell){
	UNREFERENCED_PARAMETER(cell);
	return 0;
}
