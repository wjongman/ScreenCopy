#pragma once
// #include <atlctrls.h>
// #include "Settings.h"
// #include <vector>
// #include <strsafe.h>
#include "PresetsListCtl.h"

///////////////////////////////////////////////////////////////////////////////
//
// Dialog for adding and editing a grabber-preset
//
class CEditPresetDlg : public CDialogImpl<CEditPresetDlg>
{
    std::wstring m_title = L"Edit Preset";
    GrabberPreset m_preset;

public:
    enum { IDD = IDD_EDITPRESETDLG };

    BEGIN_MSG_MAP(CEditPresetDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()

    //---------------------------------------------------------------------------
    CEditPresetDlg() = default;

    //---------------------------------------------------------------------------
    CEditPresetDlg(GrabberPreset const& preset)
    {
        m_preset = preset;
    }
    
    //---------------------------------------------------------------------------
    void SetCaption(std::wstring const& title)
    {
        m_title = title;
    }

    //---------------------------------------------------------------------------
    void SetPreset(GrabberPreset preset)
    {
        m_preset = preset;
    }

    //---------------------------------------------------------------------------
    GrabberPreset GetPreset()
    {
        return m_preset;
    }

private:
    //---------------------------------------------------------------------------
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        SetWindowText(m_title.c_str());
        CenterWindow(GetParent());

        ::SetWindowText(GetDlgItem(IDC_PRESET_NAME), m_preset.description.c_str());
        ::SetWindowText(GetDlgItem(IDC_PRESET_X), std::to_wstring(m_preset.x).c_str());
        ::SetWindowText(GetDlgItem(IDC_PRESET_Y), std::to_wstring(m_preset.y).c_str());
        ::SetWindowText(GetDlgItem(IDC_PRESET_W), std::to_wstring(m_preset.w).c_str());
        ::SetWindowText(GetDlgItem(IDC_PRESET_H), std::to_wstring(m_preset.h).c_str());
        return TRUE;
    }

    //---------------------------------------------------------------------------
    LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        wchar_t buffer[MAX_PATH];
        GetDlgItemText(IDC_PRESET_NAME, buffer, sizeof(buffer));
        m_preset.description = buffer;
        m_preset.x = GetDlgItemInt(IDC_PRESET_X);
        m_preset.y = GetDlgItemInt(IDC_PRESET_Y);
        m_preset.w = GetDlgItemInt(IDC_PRESET_W);
        m_preset.h = GetDlgItemInt(IDC_PRESET_H);
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

///////////////////////////////////////////////////////////////////////////////
//
// Dialog for managing grabber-presets
//
class CManagePresetsDlg : public CDialogImpl<CManagePresetsDlg>
{
    CPresetsListCtrl m_listView;
    CButton m_okButton;
    CButton m_addButton;
    CButton m_editButton;
    CButton m_deleteButton;

    // We use m_presetsList as the underlying container, all
    // operations in the listbox will be carried out on this
    // container, from which the listbox is then repopulated.
    PresetsList m_presetsList;

public:
    enum { IDD = IDD_MANAGEPRESETSDLG };

    BEGIN_MSG_MAP(CManagePresetsDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_PRESET_ADD, OnAdd)
        COMMAND_ID_HANDLER(IDC_PRESET_EDIT, OnEdit)
        COMMAND_ID_HANDLER(IDC_PRESET_DELETE, OnDelete)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        NOTIFY_HANDLER(IDC_LISTVIEW, NM_CLICK, OnListSelChange)
    END_MSG_MAP()

    //---------------------------------------------------------------------------
    CManagePresetsDlg(PresetsList const& presetsList)
    {
        m_presetsList = presetsList;
    }

    //---------------------------------------------------------------------------
    PresetsList GetPresets()
    {
        return m_presetsList;
    }

    //---------------------------------------------------------------------------
    void AddPreset(GrabberPreset preset)
    {
        CEditPresetDlg dlg;
        dlg.SetCaption(L"Add Preset");
        if (dlg.DoModal() == IDOK)
        {
            m_presetsList.push_back(dlg.GetPreset());
            m_listView.Populate(m_presetsList);
        }
    }

private:
    //---------------------------------------------------------------------------
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CenterWindow(GetParent());
        m_okButton.Attach(GetDlgItem(IDOK));
        m_addButton.Attach(GetDlgItem(IDC_PRESET_ADD));
        m_editButton.Attach(GetDlgItem(IDC_PRESET_EDIT));
        m_deleteButton.Attach(GetDlgItem(IDC_PRESET_DELETE));
        m_listView.Attach(GetDlgItem(IDC_LISTVIEW));

        m_editButton.EnableWindow(false);
        m_deleteButton.EnableWindow(false);
        m_listView.Populate(m_presetsList);
        return TRUE;
    }

    //---------------------------------------------------------------------------
    LRESULT OnAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        CEditPresetDlg dlg;
        dlg.SetCaption(L"Add Preset");
        if (dlg.DoModal() == IDOK)
        {
            m_presetsList.push_back(dlg.GetPreset());
            m_listView.Populate(m_presetsList);
        }
        return 0;
    }

    //---------------------------------------------------------------------------
    LRESULT OnEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        int index = m_listView.GetSelectedIndex();
        GrabberPreset preset = m_presetsList[index];
        CEditPresetDlg dlg;
        dlg.SetPreset(preset);
        dlg.SetCaption(L"Edit Preset");
        if (dlg.DoModal() == IDOK)
        {
            m_presetsList[index] = dlg.GetPreset();
            m_listView.Populate(m_presetsList);
        }
        return 0;
    }

    //---------------------------------------------------------------------------
    LRESULT OnDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        int index = m_listView.GetSelectedIndex();
        auto tail = m_presetsList.erase(m_presetsList.begin() + index);
        m_listView.Populate(m_presetsList);

        return 0;
    }

    //---------------------------------------------------------------------------
    LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        EndDialog(wID);
        return 0;
    }
    //---------------------------------------------------------------------------
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        EndDialog(wID);
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnListSelChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
    {
        m_editButton.EnableWindow(true);
        m_deleteButton.EnableWindow(true);
        auto lpnmia = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
        int newIndex = lpnmia->iItem;
        int oldIndex = m_listView.GetSelectedIndex();
        //UpdateLabels(index);
        return 0;
    }

};

