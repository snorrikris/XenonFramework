module;

#include "os_minimal.h"
#include <list>
#include <string>

export module Xe.FileVwIF;

//import Xe.FileContainerUI_Base;
import Xe.UserSettings;
//import Xe.LogFileFindDefs;
import Xe.mfc_types;
//import Xe.LogDefs;
//import Xe.LogFilterDefs;
//import Xe.FileContainerIF_DataDefs;
import Xe.DefData;
import Xe.StringTools;
import Xe.UItypesPID;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

export enum class ECLIPBRDOP { eFILENAME = 0, eFULLPATH };

export class CXeFileVwIF
{
public:
	// Ensure any derived class will have its dtor called when object deleted.
	virtual ~CXeFileVwIF() = default;

#pragma region CommonForAllFileVws
	virtual dsid_t GetDataSourceId() const = 0; // { return GetConstFileContainerUI_IF()->GetDataSourceId(); }
	//DSType GetDataSourceType() const { return GetConstFileContainerUI_IF()->GetDataSourceType(); }
	//virtual CXeFileContainerUI_IF* GetFileContainerUI_IF() = 0;
	//virtual const CXeFileContainerUI_IF* GetConstFileContainerUI_IF() const = 0;

	virtual const std::wstring& GetViewName() const = 0;
	virtual const std::wstring& GetPathName() const = 0;
	virtual PID GetViewPID() const = 0;
	virtual COLORREF GetViewTitleTextColor() const = 0;

	virtual HWND GetHwndOfView() const = 0;

	// Return true if point (in screen coords.) is in 'this' view else false.
	virtual bool IsPointInThisView(POINT& pt) = 0;

	virtual void SetFocusToView() = 0;

	//virtual std::wstring GetViewTitle() = 0;

	virtual std::wstring GetTooltipForTab() = 0;

	virtual void OnCopyInfoToClipboard(ECLIPBRDOP eClpBrdOp) = 0;
	virtual bool CanCopyInfoToClipboard(ECLIPBRDOP eClpBrdOp) = 0;
	//virtual bool CanOpenInAnotherInstance() = 0;
	//virtual void SetContainerParameter(const FCP& param) = 0;
	//virtual FCP GetContainerParameter(FCParam type) const = 0;
#pragma endregion CommonForAllFileVws

#pragma region LogFileVwAndMergeVwSpecific
	//virtual void RedrawGrid() = 0;

	//virtual void GotoLastRowInGrid() = 0;
	//virtual bool IsOnLastRowInGrid() = 0;

	//virtual long GetGridRowCount() = 0;

	virtual void OnChangedSettings(const ChangedSettings& chg_settings) = 0;

	// (Log/Merge view) goto date/time.
	// ftGoto = date/time to go to.
	//virtual void GotoViewDateTime(FILETIMEX ftGoto) = 0;

	//virtual void TriggerSetFilterOnWT(ParamsForSetFilter params, uint8_t uPriority) = 0;

	//virtual void TriggerChangeMergeOnWT(SimpleMergeList& mergeList, ParamsForSetFilter* pSfp = nullptr) = 0;

	//virtual void RebuildGridRowIndex(bool isMultilineLogMsgWanted) = 0;

	//// Set grid state after filtering. Note - the parameters for this are set in SetFilter.
	//virtual void PostSetFilter() = 0;

	//virtual void GotoGridRow(long row) = 0;

	//virtual void UpdateBookmarksList() = 0;

	//virtual void OnBookmarksModified() = 0;

	//virtual bool ApplyFindParam(const CLogFileFindParam& params) = 0;
#pragma endregion LogFileVwAndMergeVwSpecific

#pragma region LogFileVwSpecific
	// Get date/time (no offset) of current log view grid row.
	// Returns zero if invalid row number or view not of type log view.
	//virtual FILETIMEX GetCurRowDateTime() = 0;

	//virtual void OnLogFileRenamed() = 0;

	//virtual bool LoadMoreLogFileData() = 0;

	//virtual void SetLogFileOffset(FILETIMESPAN offset) = 0;
#pragma endregion LogFileVwSpecific

#pragma region MergeVwSpecific
	//virtual bool NotifyMerge_LogChanging(dsid_t dwDataSourceId, ELOGCHANGING eChange) = 0;

	//virtual void SetMergeListTitle(const std::wstring& strMergeListTitle) = 0;

	//// Handle Merge result (merge rebuild)
	//virtual void OnMergeResult() = 0;
#pragma endregion MergeVwSpecific

#pragma region ImageVwSpecific
	//virtual void SetImageTitle() = 0;
#pragma endregion ImageVwSpecific
};

export class CVwInfo
{
public:
	dsid_t			m_dsid;
	CXeFileVwIF*	m_pView = nullptr;
	bool			m_isPinned = false;

	// Calculated or UI vars for Tabs.
	int				m_cxTabNeeded = 0;	// Width of tab needed - valid even if not visible.
	CRect			m_rcTab;			// Calculated when painting, is empty if tab not visible in UI.
	int				m_nRow = 0;			// Row number in tab view
	bool			m_bIsMouseOver = false;
	CRect			m_rcCloseBtn;
	bool			m_bIsMouseOverCloseBtn = false;
	CRect			m_rcPinBtn;
	bool			m_bIsMouseOverPinBtn = false;

	// View list windows UI vars.
	int				m_cxViewName = 0;
	CRect			m_rcPos;			// Item position in client coords.

	std::wstring	m_empty_string;

	CVwInfo()
	{
		ClearUIvars();
	}
	CVwInfo(CXeFileVwIF* pView) : m_pView(pView)
	{
		m_dsid = pView ? pView->GetDataSourceId() : dsid_t();
		ClearUIvars();
	}
	//~CVwInfo() {}

	void ClearUIvars()
	{
		m_rcTab.SetRectEmpty();
		m_rcCloseBtn.SetRectEmpty();
		m_rcPinBtn.SetRectEmpty();
		m_bIsMouseOver = m_bIsMouseOverCloseBtn = m_bIsMouseOverPinBtn = false;
		m_nRow = 0;
	}

	//DSType GetDataSourceType() const
	//{
	//	XeASSERT(m_pView);
	//	if (m_pView) { return m_pView->GetDataSourceType(); }
	//	return DSType::UNKNOWN;
	//}

	const std::wstring& GetViewName() const
	{
		XeASSERT(m_pView);
		if (!m_pView) { return m_empty_string; }
		//return m_pView->GetFileContainerUI_IF()->GetMetadata().m_strLogFilename;
		return m_pView->GetViewName();
	}

	const std::wstring& GetPathName() const
	{
		XeASSERT(m_pView);
		if (!m_pView) { return m_empty_string; }
		return m_pView->GetPathName();
	}
};

export typedef std::list<CVwInfo> CVwInfoList;
export typedef std::list<CVwInfo>::iterator Iterator_VwInfoList;

