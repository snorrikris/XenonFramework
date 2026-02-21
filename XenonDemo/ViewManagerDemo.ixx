module;

#include "os_minimal.h"
#include <mutex>
#include "logging.h"
#include "XeResource.h"

export module Demo.ViewManager;

export import Demo.ViewManager_IF;
import Xe.ViewManager;
import Xe.UserSettings;
import Xe.FileDlgHelpers;
import Xe.FileHelpers;
import Demo.ImageVw;
import Demo.TextVw;

export class CViewManagerDemo : public CXeViewManager, public CViewManagerDemoIF
{
private:
	VSRL::Logger& logger() { return VSRL::s_pVSRL->GetInstance("CViewManagerDemo"); }

	std::mutex m_mutex_next_datasource_id;
	uint32_t m_next_datasource_id = 1;

public:
	CViewManagerDemo(CXeUIcolorsIF* pUIcolors) : CXeViewManager(pUIcolors)
	{
	}
	virtual ~CViewManagerDemo() {}

	virtual dsid_t GetNextNewDataSourceId() override
	{
		std::lock_guard<std::mutex> lock(m_mutex_next_datasource_id);
		return dsid_t::MakeDSID(m_next_datasource_id++);
	}

	// Delete assignment operator and copy constructor to prevent object copy.
	//CViewManagerDemo(const CViewManagerDemo&) = delete;
	//CViewManagerDemo& operator=(const CViewManagerDemo&) = delete;

	virtual void Destroy() override
	{
		CXeViewManager::Destroy();
	}

	virtual void ProcessCommand(UINT uCmdId, UINT param) override
	{
		switch (uCmdId)
		{
		case ID__FILE_OPEN:
			_OpenFileDlg(ETABVIEWID::eAnyTabVw);
			break;
		default:
			CXeViewManager::ProcessCommand(uCmdId, param);
			break;
		}
	}

	virtual void OnDroppedFiles(CPoint ptScreenCoords, const std::vector<std::wstring>& dropped_files) override
	{
		_OpenFiles(dropped_files, CreateViewParams(ETABVIEWID::eAnyTabVw));
		//std::wstring strNewWindowTitle;
		//std::vector<std::wstring> listFiles;
		//ETABVIEWID eTabVwId = CXeViewManager::GetTabVwFromPoint(ptScreenCoords);

		//for (const std::wstring& file : dropped_files)
		//{
		//	DWORD dwAttr = GetFileAttributes(file.c_str());
		//	if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
		//	{
		//		std::vector<std::wstring> files = GetFileListFromFolder(file, L"*", true);
		//		listFiles.insert(listFiles.end(), files.begin(), files.end());

		//		if (strNewWindowTitle.size() == 0)
		//		{
		//			strNewWindowTitle = file;
		//		}
		//	}
		//	else
		//	{
		//		listFiles.push_back(file);

		//		if (dropped_files.size() == 1 && wcscmp(GetFileExtension(file).c_str(), L"zip") == 0)
		//		{
		//			strNewWindowTitle = file;
		//		}
		//	}
		//}

		//OpenFilesAsync(OF_ShowProgress | OF_AddToMRU, m_uNextListId++,
		//	L"", listFiles, CreateViewParams(eTabVwId), &m_defaultFilterSettings);

		//if (strNewWindowTitle.size() > 0)
		//{
		//	std::wstring strCurTitle = m_pFilesMgr->GetUserDefinedMainWindowTitle();
		//	if (strCurTitle.size() == 0)
		//	{
		//		m_pFilesMgr->SetUserDefinedMainWindowTitle(strNewWindowTitle);
		//		OnFileManagerNotifyGUI(FMNF::UpdateWindowTitle, 0, EWCLStatus::None);
		//	}
		//}
	}

	virtual void ProcessCommandLine(const std::wstring& cmd_line) override
	{
		//OpenFilesAsync(OF_ShowProgress, m_uNextListId++,
		//	cmd_line, { cmd_line }, CreateViewParams(), &m_defaultFilterSettings);
	}

#pragma region CXeViewManagerIF_impl
	virtual void OnMainWindowCreate() override { CXeViewManager::OnMainWindowCreate(); }
	virtual void OnMainWindowDestroy() override { CXeViewManager::OnMainWindowDestroy(); }
	//virtual void Destroy() override { CXeViewManager::Destroy(); }
	virtual void CreateTabViews(HWND hMainWnd, CXeD2DToolbarIF* pMainWndToolBar) override { CXeViewManager::CreateTabViews(hMainWnd, pMainWndToolBar); }
	virtual bool AttachView(std::unique_ptr<CXeFileVwIF> view, CreateViewParams viewParams) override { return CXeViewManager::AttachView(std::move(view), viewParams); }
	virtual void On_Timer_1S() override { CXeViewManager::On_Timer_1S(); }
	virtual int GetTabViewHeight(int idx) override { return CXeViewManager::GetTabViewHeight(idx); }
	virtual int RecalculateTabViewHeight(int idx, int cxAvailable) override { return CXeViewManager::RecalculateTabViewHeight(idx, cxAvailable); }
	virtual void SetFocusToCurrentView() override { CXeViewManager::SetFocusToCurrentView(); }
	virtual void OnViewClosing(dsid_t dwDataSourceId) override { CXeViewManager::OnViewClosing(dwDataSourceId); }
	virtual void OnViewClosed(dsid_t dwDataSourceId) override { CXeViewManager::OnViewClosed(dwDataSourceId); }
	virtual void OnViewRenamed(CXeFileVwIF* pView, const std::wstring& strNewTitle) override { CXeViewManager::OnViewRenamed(pView, strNewTitle); }
	virtual void DeleteAllViews() override { CXeViewManager::DeleteAllViews(); }
	virtual void FindAndDeleteViewWithDataSourceId(dsid_t dwDataSourceId) override { CXeViewManager::FindAndDeleteViewWithDataSourceId(dwDataSourceId); }
	virtual void OnChangedSettings(const ChangedSettings& chg_settings) override { CXeViewManager::OnChangedSettings(chg_settings); }
	virtual CXeUIcolorsIF* GetUIcolors() override { return CXeViewManager::GetUIcolors(); }
	virtual bool SwitchToView(CXeFileVwIF* pView, bool setFocusToView) override { return CXeViewManager::SwitchToView(pView, setFocusToView); }
	virtual bool SwitchToViewWithDataSourceId(dsid_t dwDataSourceId, bool isSetFocus) override { return CXeViewManager::SwitchToViewWithDataSourceId(dwDataSourceId, isSetFocus); }
	virtual bool SwitchToViewAtIndex(ETABVIEWID tabVwId, UINT uIndex) override { return CXeViewManager::SwitchToViewAtIndex(tabVwId, uIndex); }
	virtual bool SwitchToView(ViewNavigate navigate, dsid_t dwDataSourceIdOfViewWithFocus) override { return CXeViewManager::SwitchToView(navigate, dwDataSourceIdOfViewWithFocus); }
	virtual ETABVIEWID GetTabViewIdOfView(dsid_t dwDataSourceId) const override { return CXeViewManager::GetTabViewIdOfView(dwDataSourceId); }
	virtual size_t GetTabViewCount(ETABVIEWID vwId) const override { return CXeViewManager::GetTabViewCount(vwId); }
	virtual size_t GetTabViewCurrentIndex(ETABVIEWID vwId) const override { return CXeViewManager::GetTabViewCurrentIndex(vwId); }
	virtual UINT GetTotalViewCount() override { return CXeViewManager::GetTotalViewCount(); }
	virtual ETABVIEWID GetTabVwFromPoint(POINT& pt) override { return CXeViewManager::GetTabVwFromPoint(pt); }
	virtual CXeFileVwIF* GetViewFromDataSourceId(dsid_t dwDataSourceId) override { return CXeViewManager::GetViewFromDataSourceId(dwDataSourceId); }
	virtual const CXeFileVwIF* GetViewFromDataSourceIdConst(dsid_t dwDataSourceId) const override { return CXeViewManager::GetViewFromDataSourceIdConst(dwDataSourceId); }
	virtual std::vector<CXeFileVwIF*> GetAllViews() override { return CXeViewManager::GetAllViews(); }
	virtual std::vector<CXeFileVwIF*> GetAllForegroundViews() override { return CXeViewManager::GetAllForegroundViews(); }
	virtual std::vector<CXeFileVwIF*> GetAllNonForegroundViews() override { return CXeViewManager::GetAllNonForegroundViews(); }
	virtual std::vector<CXeFileVwIF*> GetAllViewsFromDataSourceIds(const std::vector<dsid_t>& dataSourceIds) override { return CXeViewManager::GetAllViewsFromDataSourceIds(dataSourceIds); }
	virtual void SendMessageToAllViews(UINT uMessage, WPARAM wParam, LPARAM lParam) override { CXeViewManager::SendMessageToAllViews(uMessage, wParam, lParam); }
	virtual CRect GetTabViewWindow(ETABVIEWID tabVwId) override { return CXeViewManager::GetTabViewWindow(tabVwId); }
	virtual void ViewWithFocusChanged(dsid_t dwDataSourceId) override { CXeViewManager::ViewWithFocusChanged(dwDataSourceId); }
	virtual void SetFocusToView(dsid_t dwDataSourceId) override { CXeViewManager::SetFocusToView(dwDataSourceId); }
	virtual bool IsViewVisible(dsid_t dwDataSourceId) override { return CXeViewManager::IsViewVisible(dwDataSourceId); }
	virtual bool IsLastViewWithFocus(dsid_t dwDataSourceId) override { return CXeViewManager::IsLastViewWithFocus(dwDataSourceId); }
	virtual void OnTabOrderChanged() override { CXeViewManager::OnTabOrderChanged(); }
	virtual bool OnViewGotFocus(dsid_t dwDataSourceId) override { return CXeViewManager::OnViewGotFocus(dwDataSourceId); }
	virtual void OnViewLostFocus(dsid_t dwDataSourceId) override { CXeViewManager::OnViewLostFocus(dwDataSourceId); }
	//virtual void ProcessCommand(UINT uCmdId, UINT param) override { CXeViewManager::ProcessCommand(uCmdId, param); }
	virtual void OnTabCtxMenuCmd(UINT uCmdId, ETABVIEWID tabId, dsid_t datasourceId) override { CXeViewManager::OnTabCtxMenuCmd(uCmdId, tabId, datasourceId); }
	virtual CXeCancelEvent* DoWorkThreadWork(WorkThreadWorkCallbackFunc func) override { return CXeViewManager::DoWorkThreadWork(func); }
	virtual CXeCancelEvent* GetWorkThreadWorkCancelEvent() override { return CXeViewManager::GetWorkThreadWorkCancelEvent(); }
	virtual bool CanOpenContainingFolder(dsid_t dsId) override { return CXeViewManager::CanOpenContainingFolder(dsId); }
	virtual void OpenContainingFolder(dsid_t dsId) override { CXeViewManager::OpenContainingFolder(dsId); }
	virtual void CopyInfoToClipboard(ECLIPBRDOP operation, dsid_t dsId) override { CXeViewManager::CopyInfoToClipboard(operation, dsId); }
	virtual int ShowMessageBox(HWND hParent, const wchar_t* pMsg, const wchar_t* pTitle = nullptr, UINT uType = MB_OK, CRect* pRectCell = nullptr) override { return CXeViewManager::ShowMessageBox(hParent, pMsg, pTitle, uType, pRectCell); }
	virtual std::optional<std::wstring> AskStringDlg(HWND hParent, const std::wstring& title, const std::wstring& string, CRect* pRectCell = nullptr) override { return CXeViewManager::AskStringDlg(hParent, title, string, pRectCell); }
	virtual void OnNotifyChanges(WPARAM wParam, LPARAM lParam) override { CXeViewManager::OnNotifyChanges(wParam, lParam); }
	virtual void OnUIcreateComplete() override { CXeViewManager::OnUIcreateComplete(); }
	//virtual void OnDroppedFiles(CPoint ptScreenCoords, const std::vector<std::wstring>& dropped_files) override { CXeViewManager::OnDroppedFiles(ptScreenCoords, dropped_files); }
	virtual void OpenFileFromMRUlist(size_t idx) override { CXeViewManager::OpenFileFromMRUlist(idx); }
	//virtual void ProcessCommandLine(const std::wstring& cmd_line) override { CXeViewManager::ProcessCommandLine(cmd_line); }
#pragma endregion CXeViewManagerIF_impl

#pragma region OpenFiles
protected:
	void _OpenFileDlg(ETABVIEWID eTabVwId)
	{
		std::wstring lastUsedDir = s_xeLastUsedUIsettings.GetString_or_Val(L"OpenFileDlgLastUsedDir", L"");
		// Note - try to verify dir is accessible - but only wait 300mS.
		if (lastUsedDir.size() && !m_verifyDirAccessible.IsAccessible(lastUsedDir.c_str()))
		{
			lastUsedDir = L"";
		}

		std::wstring title = L"Open one or more files";
		std::vector<std::pair<std::wstring, std::wstring>> file_types{ {L"All files", L"*.*"} };
		std::vector<std::wstring> files = ShowOpenFileDlg(lastUsedDir, true, file_types, &title);
		if (files.size() == 0)
		{
			return;
		}

		// File list name is empty when many files selected.
		//std::wstring strFileListName = files.size() == 1 ? GetFilenameWithExt(files[0]) : L"";

		//OpenFilesAsync(OF_ShowProgress | OF_AddToMRU, m_uNextListId++,
		//	strFileListName, files, CreateViewParams(eTabVwId), &m_defaultFilterSettings);

		_OpenFiles(files, CreateViewParams(eTabVwId));

		s_xeLastUsedUIsettings.Set(L"OpenFileDlgLastUsedDir", GetDirectory(files[0]));
	}

	void _OpenFiles(const std::vector<std::wstring>& strPathNames, CreateViewParams viewParams)
	{
		CXeTabsView* pTabVw = _GetTabView(viewParams.eTabVwId);
		XeASSERT(pTabVw);
		if (!pTabVw)
		{
			return;
		}
		std::vector<std::wstring> files = strPathNames;	// Copy list.
		//RemovePathnamesWithExtension(files, L"log_aux");	// Ignore aux data files
		//std::vector<CWorkContext> workList;
		//int deleteRolling_SSH_LogFilesGreaterThan
		//	= (int)s_xeUIsettings[L"RollingLogFilesSettings"].Get(L"AutoCloseRolling_SSH_LogFilesGreterThan").getU32();
		//std::wstring ssh_DestFolderBase = s_xeLastUsedUIsettings.GetString_or_Val(L"SSHdestFolder",
		//	GetKnownFolder(L"%temp%"));


		// Switch to view for items from list that are already opened - add the rest to worklist.
		for (auto it = files.begin(); it != files.end(); )
		{
			//CWorkContext workContext = CWorkContext::MkLoad(uListId, m_hMainWnd,
			//	pFilterParams, viewParams);

			//SSHfile ssh_file(*it);	// Parse as SSH file string - in case it is SSH file.
			//if (ssh_file.HasSSHparams())
			//{
			//	*it = ssh_file.GetAs_SSH_file_string();	// Remove password (if any)
			//	m_sshServerHistory.UpdateSSHfileInfoFromFileHistory(ssh_file);
			//	workContext.m_sshParameters = ssh_file;
			//	workContext.m_sshParameters.m_strSSHdest_path_root = ssh_DestFolderBase;
			//	workContext.m_sshParameters.m_deleteRolling_SSH_LogFilesGreaterThan = deleteRolling_SSH_LogFilesGreaterThan;
			//}
			//else if (workContext.m_evtlogParams.Parse(xet::toUTF8(*it)))	// Is the file Event Log channel?
			//{

			//}

			//if (_FindAndSwitchToViewWithFile(*it, viewParams.makeThisCurrentView))
			//{
			//	it = files.erase(it);	// Remove already open file from list.
			//}
			//else
			//{
				// Add to work list items that are not already open
				//workContext.m_strPathName = *it;
				//workContext.m_dwDataSourceId = m_pFilesMgr->GetNextNewDataSourceId();
				//workContext.m_pLogFormats = m_pLogFormats->GetLogFormatsForFile(*it);
				//if ((of_flags & OF_SuppressErrorMessageBox) != 0)
				//{
				//	workContext.m_bSuppressErrorMessageBox = true;
				//}
				//if ((of_flags & OF_RetryOpen) != 0)
				//{
				//	workContext.m_bRetryOpen = true;
				//}
				//workList.push_back(workContext);

				//CreateNewView(m_hMainWnd, pContainer, context.m_VwParams);

				if (IsImageFileExtension(*it))
				{
					std::unique_ptr<CDemoImageVw> view = std::make_unique<CDemoImageVw>(this, GetNextNewDataSourceId());
					if (view->LoadFile(*it))
					{
						AttachView(std::move(view), viewParams);	// Note - view object has been moved and is no longer usable here.
						m_MRUList.Add(*it);
					}
				}
				else if (IsFileExtension(L"txt", *it))
				{
					std::unique_ptr<CDemoTextVw> view = std::make_unique<CDemoTextVw>(this, GetNextNewDataSourceId());
					if (view->LoadFile(*it))
					{
						AttachView(std::move(view), viewParams);	// Note - view object has been moved and is no longer usable here.
						m_MRUList.Add(*it);
					}
				}
				else
				{
					XeASSERT(false);	// File type not supported.
				}

				++it;
			//}
		}

		//if ((of_flags & OF_AddToMRU) != 0)	// Add to Most Recently Used file list?
		//{
		//	m_MRUList.Add(files);
		//}

		//if (workList.size() == 0)
		//{
		//	return;
		//}

		//m_pFilesMgr->ScheduleWorkAsync(listname, workList, false);

		//if ((of_flags & OF_ShowProgress) != 0)	// Show progress dialog?
		//{
		//	_ShowOpenStatusDialog(uListId, false);
		//}
	}
#pragma endregion OpenFiles
};

