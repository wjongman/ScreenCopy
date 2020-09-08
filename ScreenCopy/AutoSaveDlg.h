#pragma once

#include "ImageSaver.h"
#include "resource.h"
///////////////////////////////////////////////////////////////////////////////
//
// Dialog for configuring auto-save options
//
class CAutoSaveDlg : public CDialogImpl<CAutoSaveDlg>
{
    CButton m_bnBrowse;
    CComboBox m_cbType;
    bool m_initialized = false;

public:
    enum { IDD = IDD_AUTOSAVEDLG };

    BEGIN_MSG_MAP(CAutoSaveDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_HANDLER(IDC_BROWSE, BN_CLICKED, OnBrowse)
        COMMAND_HANDLER(IDC_EDIT_DIRECTORY, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER(IDC_EDIT_PREFIX, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER(IDC_EDIT_DIGITS, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER(IDC_EDIT_NEXTVALUE, EN_CHANGE, OnEditChange)
    END_MSG_MAP()

    //---------------------------------------------------------------------------
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CenterWindow(GetParent());

        CSettings settings;

        SetDlgItemText(IDC_EDIT_DIRECTORY, settings.GetString(L"autosave", L"directory", GetDesktopPath()).c_str());
        SetDlgItemText(IDC_EDIT_PREFIX, settings.GetString(L"autosave", L"prefix", L"Snapshot").c_str());
        SetDlgItemInt(IDC_EDIT_DIGITS, settings.GetInt(L"autosave", L"digits", 2));
        SetDlgItemInt(IDC_EDIT_NEXTVALUE, settings.GetInt(L"autosave", L"nextvalue", 1));

        CUpDownCtrl spinDigits;
        spinDigits.Attach(GetDlgItem(IDC_SPIN1));
        spinDigits.SetRange(0, 8);

        m_cbType.Attach(GetDlgItem(IDC_IMAGETYPE));
        m_cbType.AddString(L".jpg");
        m_cbType.AddString(L".png");
        m_cbType.AddString(L".bmp");
        m_cbType.AddString(L".gif");
        m_cbType.SetCurSel(settings.GetInt(L"autosave", L"imagetype", 0));

        int action = settings.GetInt(L"autosave", L"existaction", 1);
        switch (action)
        {
        case 1:
            ::SendMessage(GetDlgItem(IDC_PROMPT), BM_SETCHECK, 1, 0);
            break;
        case 2:
            ::SendMessage(GetDlgItem(IDC_REPLACE), BM_SETCHECK, 1, 0);
            break;
        case 3:
            ::SendMessage(GetDlgItem(IDC_RENAME), BM_SETCHECK, 1, 0);
            break;
        }

        m_initialized = true;
        SetDlgItemText(IDC_PREVIEW, GetNextPathName());

        return TRUE;
    }

    //-------------------------------------------------------------------------
    LRESULT OnBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        // Show Select Folder dialog
        std::wstring selected = SelectFolder(m_hWnd);
        if (!selected.empty())
        {
            SetDlgItemText(IDC_EDIT_DIRECTORY, selected.c_str());
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        if (m_initialized)
        {
            SetDlgItemText(IDC_PREVIEW, GetNextPathName());
        }
        return 0;
    }

    //---------------------------------------------------------------------------
    CString GetItemText(int itemId)
    {
        wchar_t szText[MAX_PATH];
        GetDlgItemText(itemId, szText, sizeof(szText));
        return szText;
    }

    //---------------------------------------------------------------------------
    CString GetNextPathName()
    {
        CString pathname = GetItemText(IDC_EDIT_DIRECTORY) + L"\\"
            + GetItemText(IDC_EDIT_PREFIX) + GetSequenceString() + GetExtension();

        return pathname;
    }

    //---------------------------------------------------------------------------
    CString GetSequenceString()
    {
        // Todo: make sure NextValue has no more then Digits digits
        CString SeqNum = "";
        int digits = GetDlgItemInt(IDC_EDIT_DIGITS);
        int nextvalue = GetDlgItemInt(IDC_EDIT_NEXTVALUE);
        if (digits != 0)
            SeqNum.Format(L"%0*d", digits, nextvalue);

        return SeqNum;
    }

    //---------------------------------------------------------------------------
    CString GetExtension()
    {
        int index = m_cbType.GetCurSel();
        wchar_t* Extension[] = { L".jpg", L".png", L".bmp" };
        if (index >= 0 && index < 3)
        {
            return Extension[index];
        }
        return L"";
    }

    //---------------------------------------------------------------------------
    int GetExistAction()
    {
        int action = 0;
        CButton button;
        if (IsDlgButtonChecked(IDC_PROMPT))
        {
            action = 1;
        }
        if (IsDlgButtonChecked(IDC_REPLACE))
        {
            action = 2;
        }
        if (IsDlgButtonChecked(IDC_RENAME))
        {
            action = 3;
        }
        return action;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Wrapper for IFileOpenDialog with the FOS_PICKFOLDERS option
    // https://stackoverflow.com/questions/52617193/what-is-the-easiest-way-to-create-a-folder-picker-dialog-box-in-c
    //-------------------------------------------------------------------------
    class CCoInitialize
    // https://devblogs.microsoft.com/oldnewthing/?p=39243
    {
        HRESULT m_hr;

    public:
        CCoInitialize()
            : m_hr(CoInitialize(NULL))
        {
        }
        ~CCoInitialize()
        {
            if (SUCCEEDED(m_hr))
                CoUninitialize();
        }
        operator HRESULT() const { return m_hr; }
    };

    //-------------------------------------------------------------------------
    std::wstring SelectFolder(HWND hwndOwner)
    {
        std::wstring selectedPath = L"";
        // IFileOpenDialog needs initialized COM
        CCoInitialize init;
        if (SUCCEEDED(init))
        {
            // Create an instance of IFileOpenDialog.
            CComPtr<IFileOpenDialog> pFolderDlg;
            HRESULT hr = pFolderDlg.CoCreateInstance(CLSID_FileOpenDialog);
            if (SUCCEEDED(hr))
            {
                // Set options for a filesystem folder picker dialog.
                FILEOPENDIALOGOPTIONS opt;
                pFolderDlg->GetOptions(&opt);
                pFolderDlg->SetOptions(opt | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST
                    | FOS_FORCEFILESYSTEM);

                // Show the dialog modally.
                if (SUCCEEDED(pFolderDlg->Show(hwndOwner)))
                {
                    // Get the path of the selected folder
                    CComPtr<IShellItem> pSelectedItem;
                    pFolderDlg->GetResult(&pSelectedItem);

                    if (pSelectedItem)
                    {
                        CComHeapPtr<wchar_t> pPath;
                        hr = pSelectedItem->GetDisplayName(
                            SIGDN_FILESYSPATH, &pPath);
                        if (SUCCEEDED(hr))
                        {
                            selectedPath = pPath.m_pData;
                        }
                    }
                }
            }
        }
        return selectedPath;
    }

    //---------------------------------------------------------------------------
    LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        CString directory = GetItemText(IDC_EDIT_DIRECTORY);
        if (!DirectoryExists(directory))
        {
            CString sMsg = directory + L":\n\nDirectory doesn't exist!";
            MessageBox(sMsg, L"Oops..", MB_OK);
            return 0;
        }
        CSettings settings;

        settings.SetString(L"autosave", L"directory", (LPCWSTR)GetItemText(IDC_EDIT_DIRECTORY));
        settings.SetString(L"autosave", L"prefix", (LPCWSTR)GetItemText(IDC_EDIT_PREFIX));
        settings.SetInt(L"autosave", L"digits", GetDlgItemInt(IDC_EDIT_DIGITS));
        settings.SetInt(L"autosave", L"nextvalue", GetDlgItemInt(IDC_EDIT_NEXTVALUE));
        settings.SetInt(L"autosave", L"imagetype",  m_cbType.GetCurSel());
        settings.SetInt(L"autosave", L"existaction", GetExistAction());

        EndDialog(wID);
        return 0;
    }
    
    //---------------------------------------------------------------------------
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        EndDialog(wID);
        return 0;
    }
};
