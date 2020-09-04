#pragma once
#include <atlctrls.h>
#include "Settings.h"
#include <vector>
#include <strsafe.h>
#include "PresetsListCtl.h"

///////////////////////////////////////////////////////////////////////////////
//
// Dialog for managing grabber presets
//
class CPresetsDlg : public CDialogImpl<CPresetsDlg>
{
    CPresetsListCtrl m_listView;
    CButton m_okButton;
    std::vector<GrabberPreset> m_presetList;
public:
    enum { IDD = IDD_PRESETSDLG };

    BEGIN_MSG_MAP(CPresetsDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_PRESET_ADD, OnAdd)
        COMMAND_ID_HANDLER(IDC_PRESET_ADD, OnEdit)
        COMMAND_ID_HANDLER(IDC_PRESET_ADD, OnDelete)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()


private:
    //---------------------------------------------------------------------------
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CenterWindow(GetParent());
        m_okButton.Attach(GetDlgItem(IDOK));

        m_listView.Attach(GetDlgItem(IDC_LISTVIEW));
        m_presetList = LoadPresets();
//         m_presetList.push_back(GrabberPreset(L"Monitor 1", 0, 0, 1920, 1080));
//         m_presetList.push_back(GrabberPreset(L"Monitor 2", 1920, 0, 1600, 900));
        m_listView.Populate(m_presetList);
        return TRUE;
    }

    //---------------------------------------------------------------------------
    LRESULT OnAdd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        return 0;
    }
    //---------------------------------------------------------------------------
    LRESULT OnEdit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        return 0;
    }
    //---------------------------------------------------------------------------
    LRESULT OnDelete(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        return 0;
    }
    //---------------------------------------------------------------------------
    LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        SavePresets();
        EndDialog(wID);
        return 0;
    }
    //---------------------------------------------------------------------------
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        EndDialog(wID);
        return 0;
    }

    //---------------------------------------------------------------------------
    std::vector<GrabberPreset> LoadPresets()
    {
        std::vector<GrabberPreset> presets;

        CSettings settings;
        std::wstring keyName = L"capture\\presets";
        m_listView.DeleteAllItems();
        for (int i = 1; i < 99; i++)
        {
            std::wstring commatext = settings.GetString(keyName, std::to_wstring(i), L"");
            if (commatext.empty())
                return presets;

            presets.push_back(GrabberPreset(commatext));
        }
        return presets;
    }

    //---------------------------------------------------------------------------
    void SavePresets()
    {
        CSettings settings;
        std::wstring keyName = L"capture\\presets";
        settings.DeleteSection(keyName);
        for (size_t i = 1; i <= m_presetList.size(); i++)
        {
            settings.SetString(keyName, std::to_wstring(i), m_presetList[i - 1].GetCommaText());
        }
    }

};

