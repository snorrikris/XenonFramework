module;

#include "os_minimal.h"
#include <shobjidl.h> 
#include <atlbase.h>
#include <vector>
#include <string>
#include "logging.h"
#include "XeAssert.h"

export module Xe.FileDlgHelpers;

VSRL::Logger& logger_filehelper() { return VSRL::s_pVSRL->GetInstance("FileDlgHelpers"); }

void _SetFileDlgTitle(IFileDialog* pDlg, const std::wstring& title)
{
	HRESULT hr = pDlg->SetTitle(title.c_str());
	XeASSERT(hr == S_OK);
}

std::wstring _GetFileDlgResult(IFileDialog* pDlg)
{
	std::wstring result;
	Microsoft::WRL::ComPtr<IShellItem> item;
	HRESULT hr = pDlg->GetResult(&item);
	XeASSERT(hr == S_OK);
	if (SUCCEEDED(hr) && item)
	{
		PWSTR pszFilePath;
		hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
		XeASSERT(hr == S_OK);
		if (SUCCEEDED(hr))
		{
			result = pszFilePath;
			CoTaskMemFree(pszFilePath); // Free the memory
		}
	}
	return result;
}

std::vector<std::wstring> _GetFileOpenDlgResults(IFileOpenDialog* pDlg)
{
	std::vector<std::wstring> files;

	Microsoft::WRL::ComPtr<IShellItemArray> items;
	HRESULT hr = pDlg->GetResults(&items);
	XeASSERT(hr == S_OK);
	if (SUCCEEDED(hr))
	{
		DWORD cItems = 0;
		hr = items->GetCount(&cItems); // Get the number of items
		XeASSERT(hr == S_OK);
		if (SUCCEEDED(hr))
		{
			for (DWORD i = 0; i < cItems; i++)
			{
				Microsoft::WRL::ComPtr<IShellItem> item;
				hr = items->GetItemAt(i, &item); // Get the item at index i
				XeASSERT(hr == S_OK);
				if (SUCCEEDED(hr) && item)
				{
					PWSTR pszFilePath;
					hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					XeASSERT(hr == S_OK);
					if (SUCCEEDED(hr))
					{
						files.push_back(pszFilePath);
					}
				}
			}
		}
	}
	return files;
}

void _AddFileDlgOptions(IFileDialog* pDlg, FILEOPENDIALOGOPTIONS opt)
{
	FILEOPENDIALOGOPTIONS fos;
	pDlg->GetOptions(&fos);
	fos |= opt;
	HRESULT hr = pDlg->SetOptions(fos);
	XeASSERT(hr == S_OK);
}

void _SetFileDlgFolder(IFileDialog* pDlg, const std::wstring& folder)
{
	Microsoft::WRL::ComPtr<IShellItem> curfolder;
	HRESULT hr = SHCreateItemFromParsingName(folder.c_str(), NULL, IID_PPV_ARGS(&curfolder));
	if (SUCCEEDED(hr))
	{
		hr = pDlg->SetFolder(curfolder.Get());
		XeASSERT(hr == S_OK);
	}
}

std::vector<COMDLG_FILTERSPEC> _CreateFILTERSPEC(const std::vector<std::pair<std::wstring, std::wstring>>& file_types)
{
	std::vector<COMDLG_FILTERSPEC> filterSpecs(file_types.size());
	COMDLG_FILTERSPEC* filterSpec = filterSpecs.data();

	for (const auto& filter : file_types)
	{
		filterSpec->pszName = filter.first.c_str();
		filterSpec->pszSpec = filter.second.c_str();
		++filterSpec;
	}
	return filterSpecs;
}

void _SetFileDlgFileTypes(IFileDialog* pDlg, const std::vector<std::pair<std::wstring, std::wstring>>& file_types)
{
	std::vector<COMDLG_FILTERSPEC> filterSpecs = _CreateFILTERSPEC(file_types);
	HRESULT hr = pDlg->SetFileTypes((UINT)filterSpecs.size(), &(filterSpecs[0]));
	XeASSERT(hr == S_OK);
}

void _SetFileDlgFileName(IFileDialog* pDlg, const std::wstring& file_name)
{
	HRESULT hr = pDlg->SetFileName(file_name.c_str());
	XeASSERT(hr == S_OK);
}

void _AddFileDlgCheckBox(IFileDialog* pDlg, DWORD dwCB_ID, const std::wstring& label, BOOL state)
{
	Microsoft::WRL::ComPtr<IFileDialogCustomize> pfdc;
	HRESULT hr = pDlg->QueryInterface(IID_PPV_ARGS(&pfdc));
	XeASSERT(hr == S_OK);
	if (SUCCEEDED(hr))
	{
		hr = pfdc->AddCheckButton(dwCB_ID, label.c_str(), state);
		XeASSERT(hr == S_OK);
	}
}

BOOL _GetFileDlgCheckBoxState(IFileDialog* pDlg, DWORD dwCB_ID)
{
	BOOL result = FALSE;
	Microsoft::WRL::ComPtr<IFileDialogCustomize> pfdc;
	HRESULT hr = pDlg->QueryInterface(IID_PPV_ARGS(&pfdc));
	XeASSERT(hr == S_OK);
	if (SUCCEEDED(hr))
	{
		hr = pfdc->GetCheckButtonState(dwCB_ID, &result);
		XeASSERT(hr == S_OK);
	}
	return result;
}

/// <summary>
/// Show open file dialog.
/// </summary>
/// <param name="lastUsedDir">Folder to set as first choice, or empty to use default.</param>
/// <param name="isAllowMultiSelect">true if user can select more than one file, false if only single select.</param>
/// <param name="file_types">List of file types, e.g.: {{L"Log files","*.log"},{L"All files",L"*.*"}}, or empty.</param>
/// <returns>List of files selected.</returns>
export std::vector<std::wstring> ShowOpenFileDlg(const std::wstring& lastUsedDir, bool isAllowMultiSelect,
		const std::vector<std::pair<std::wstring, std::wstring>>& file_types, std::wstring* pTitle = nullptr)
{
	std::vector<std::wstring> files;
	Microsoft::WRL::ComPtr<IFileOpenDialog> fod = 0;
	//HRESULT hr = fod.CoCreateInstance(__uuidof(FileOpenDialog));
	HRESULT hr = ::CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_ALL,
		__uuidof(IFileOpenDialog),
		reinterpret_cast<void**>(fod.GetAddressOf()));

	if (SUCCEEDED(hr))
	{
		if (pTitle)
		{
			_SetFileDlgTitle(fod.Get(), *pTitle);
		}
		if (lastUsedDir.size())
		{
			_SetFileDlgFolder(fod.Get(), lastUsedDir);
		}
		if (isAllowMultiSelect)
		{
			_AddFileDlgOptions(fod.Get(), FOS_ALLOWMULTISELECT);
		}
		if (file_types.size())
		{
			_SetFileDlgFileTypes(fod.Get(), file_types);
		}

		// Show the Open File dialog box.
		hr = fod->Show(NULL);	// Returns S_OK if user selected something.

		// Get the file names from the dialog box.
		if (SUCCEEDED(hr))
		{
			files = _GetFileOpenDlgResults(fod.Get());
		}
	}
	return files;
}

/// <summary>
/// Show SaveAs dialog.
/// </summary>
/// <param name="suggested_filename">The filename that is shown in the edit box of the dialog.</param>
/// <param name="folder">Folder to set as first choice, or empty to use default.</param>
/// <param name="file_types">List of file types, e.g.: {{L"Log files","*.log"},{L"All files",L"*.*"}}, or empty.</param>
/// <param name="pCheckBoxBool">Pointer to BOOL if checkbox wanted in dialog.</param>
/// <param name="pCheckBoxLabel">Label of the checkbox.</param>
/// <returns>The selected filename.</returns>
export std::wstring ShowSaveAsDlg(const std::wstring& suggested_filename, const std::wstring& folder,
		const std::vector<std::pair<std::wstring, std::wstring>>& file_types, std::wstring* pTitle = nullptr,
		BOOL *pCheckBoxBool = nullptr, const std::wstring* pCheckBoxLabel = nullptr)
{
	std::wstring save_file;
	Microsoft::WRL::ComPtr<IFileSaveDialog> fsd = 0;
	//HRESULT hr = fsd.CoCreateInstance(__uuidof(FileSaveDialog));
	HRESULT hr = ::CoCreateInstance(
		CLSID_FileSaveDialog,
		nullptr,
		CLSCTX_ALL,
		__uuidof(IFileSaveDialog),
		reinterpret_cast<void**>(fsd.GetAddressOf()));
	if (SUCCEEDED(hr))
	{
		if (pTitle)
		{
			_SetFileDlgTitle(fsd.Get(), *pTitle);
		}
		if (folder.size())
		{
			_SetFileDlgFolder(fsd.Get(), folder);
		}
		if (suggested_filename.size())
		{
			_SetFileDlgFileName(fsd.Get(), suggested_filename);
		}
		if (file_types.size())
		{
			_SetFileDlgFileTypes(fsd.Get(), file_types);
		}

		DWORD dwCB_ID = 1001;
		_AddFileDlgCheckBox(fsd.Get(), dwCB_ID, *pCheckBoxLabel, *pCheckBoxBool);

		// Show the SaveAs dialog box.
		hr = fsd->Show(NULL);	// Returns S_OK if user selected something.

		if (SUCCEEDED(hr))		// Get the file name from the dialog box.
		{
			save_file = _GetFileDlgResult(fsd.Get());

			*pCheckBoxBool = _GetFileDlgCheckBoxState(fsd.Get(), dwCB_ID);
		}
	}
	return save_file;
}

export std::wstring ShowFolderPickerDlg(const std::wstring& folder, std::wstring* pTitle = nullptr)
{
	std::wstring selected_folder;
	Microsoft::WRL::ComPtr<IFileOpenDialog> fpd = 0;
	//HRESULT hr = fpd.CoCreateInstance(__uuidof(FileOpenDialog));
	HRESULT hr = ::CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_ALL,
		__uuidof(IFileOpenDialog),
		reinterpret_cast<void**>(fpd.GetAddressOf())
	);

	if (SUCCEEDED(hr))
	{
		if (pTitle)
		{
			_SetFileDlgTitle(fpd.Get(), *pTitle);
		}
		_AddFileDlgOptions(fpd.Get(), FOS_PICKFOLDERS);
		if (folder.size())
		{
			_SetFileDlgFolder(fpd.Get(), folder);
		}

		// Show the File Open dialog in folder picker mode.
		hr = fpd->Show(NULL);	// Returns S_OK if user selected something.

		if (SUCCEEDED(hr))		// Get the folder name from the dialog box.
		{
			selected_folder = _GetFileDlgResult(fpd.Get());
		}
	}
	return selected_folder;
}

