#pragma once

#include "PresetsListCtl.h"

////////////////////////////////////////////////////////////////////////////////
///
///  Dialog for adding and editing a grabber-preset
///
class CEditPresetDlg : public CDialogImpl<CEditPresetDlg>
{
    CEdit m_nameEdit;
    CButton m_okButton;
    std::wstring m_title = L"Edit Preset";
    GrabberPreset m_preset;

public:
    enum { IDD = IDD_EDITPRESETDLG };

    BEGIN_MSG_MAP(CEditPresetDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_HANDLER(IDC_PRESET_NAME, EN_CHANGE, OnNameEditChange)
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
        m_nameEdit.Attach(GetDlgItem(IDC_PRESET_NAME));
        m_okButton.Attach(GetDlgItem(IDOK));

        SetWindowText(m_title.c_str());
        CenterWindow(GetParent());

        ::SetWindowText(GetDlgItem(IDC_PRESET_NAME), m_preset.description.c_str());
        ::SetWindowText(GetDlgItem(IDC_PRESET_X), std::to_wstring(m_preset.rect.left).c_str());
        ::SetWindowText(GetDlgItem(IDC_PRESET_Y), std::to_wstring(m_preset.rect.top).c_str());
        ::SetWindowText(GetDlgItem(IDC_PRESET_W), std::to_wstring(m_preset.rect.Width()).c_str());
        ::SetWindowText(GetDlgItem(IDC_PRESET_H), std::to_wstring(m_preset.rect.Height()).c_str());
        
        bool empty = (m_nameEdit.GetWindowTextLength() == 0);
        m_okButton.EnableWindow(!empty);
        
        return TRUE;
    }

    //-------------------------------------------------------------------------
    LRESULT OnNameEditChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        
        bool empty = (m_nameEdit.GetWindowTextLength() == 0);
        m_okButton.EnableWindow(!empty);
        return 0;
    }

    //---------------------------------------------------------------------------
    LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        std::vector<wchar_t> buffer(m_nameEdit.GetWindowTextLength());
        m_nameEdit.GetWindowText(buffer.data(), buffer.size());
        m_preset.description = buffer.data();

        int x = GetDlgItemInt(IDC_PRESET_X);
        int y = GetDlgItemInt(IDC_PRESET_Y);
        int w = GetDlgItemInt(IDC_PRESET_W);
        int h = GetDlgItemInt(IDC_PRESET_H);
        m_preset.rect = { x, y, x + w, y + h };

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

////////////////////////////////////////////////////////////////////////////////
///
///  Dialog for managing grabber-presets
///
class CManagePresetsDlg : public CDialogImpl<CManagePresetsDlg>
{
    CPresetsListCtrl m_listView;

    // We use m_presetsList as the underlying container, all
    // operations in the listview will be carried out on this
    // container, from which the listview is then repopulated.
    PresetsList m_presetsList;

public:
    enum { IDD = IDD_MANAGEPRESETSDLG };

    BEGIN_MSG_MAP(CManagePresetsDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_PRESET_ADD, OnAdd)
        COMMAND_ID_HANDLER(IDC_PRESET_EDIT, OnEdit)
        COMMAND_ID_HANDLER(IDC_PRESET_DELETE, OnDelete)
        COMMAND_ID_HANDLER(IDC_PRESET_MOVEUP, OnMoveUp)
        COMMAND_ID_HANDLER(IDC_PRESET_MOVEDOWN, OnMoveDown)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        NOTIFY_HANDLER(IDC_LISTVIEW, LVN_ITEMCHANGED, OnListSelChange)
        NOTIFY_HANDLER(IDC_LISTVIEW, LVN_BEGINDRAG, OnListSelChange)
        NOTIFY_HANDLER(IDC_LISTVIEW, LVN_ITEMCHANGED, OnListSelChange)
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
        m_listView.Attach(GetDlgItem(IDC_LISTVIEW));

        ::EnableWindow(GetDlgItem(IDC_PRESET_EDIT), false);
        ::EnableWindow(GetDlgItem(IDC_PRESET_DELETE), false);
        m_listView.Populate(m_presetsList);
        m_listView.SelectItem(0);
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
            m_listView.SelectItem(m_presetsList.size() - 1);
        }
        return 0;
    }

    //---------------------------------------------------------------------------
    LRESULT OnEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        int selIndex = m_listView.GetSelectedIndex();
        if (selIndex != -1)
        {
            GrabberPreset preset = m_presetsList[selIndex];
            CEditPresetDlg dlg;
            dlg.SetPreset(preset);
            dlg.SetCaption(L"Edit Preset");
            if (dlg.DoModal() == IDOK)
            {
                m_presetsList[selIndex] = dlg.GetPreset();
                m_listView.Populate(m_presetsList);
                m_listView.SelectItem(selIndex);
            }
        }

        return 0;
    }

    //---------------------------------------------------------------------------
    LRESULT OnDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        size_t selIndex = m_listView.GetSelectedIndex();
        if (selIndex != -1)
        {
            m_presetsList.erase(m_presetsList.begin() + selIndex);
            m_listView.Populate(m_presetsList);
            if (m_presetsList.size() > 0 && selIndex < m_presetsList.size())
            {
                m_listView.SelectItem(selIndex);
            }
            else
            {
                BOOL handled = false;
                OnListSelChange(0, 0, handled);
            }
        }
        return 0;
    }

    //---------------------------------------------------------------------------
    LRESULT OnMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        size_t selIndex = m_listView.GetSelectedIndex();
        if (selIndex > 0)
        {
            MovePresetItem(selIndex, selIndex - 1);
            m_listView.Populate(m_presetsList);
            m_listView.SelectItem(selIndex - 1);
        }
        m_listView.SetFocus();
        return 0;
    }

    //---------------------------------------------------------------------------
    LRESULT OnMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        size_t selIndex = m_listView.GetSelectedIndex();
        if (selIndex < m_listView.GetItemCount() - 1)
        {
            MovePresetItem(selIndex, selIndex + 1);
            m_listView.Populate(m_presetsList);
            m_listView.SelectItem(selIndex + 1);
        }
        m_listView.SetFocus();
        return 0;
    }

//---------------------------------------------------------------------------
    void MovePresetItem(size_t from, size_t to)
    {
        if (to > m_presetsList.size())
            to = m_presetsList.size();

        GrabberPreset tmp = m_presetsList[from];
        m_presetsList.erase(m_presetsList.begin() + from);
        m_presetsList.insert(m_presetsList.begin() + to, tmp);
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
    LRESULT OnListSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
    {
        size_t selIndex = m_listView.GetSelectedIndex();
        bool enable = !m_presetsList.empty() && selIndex != -1;
        ::EnableWindow(GetDlgItem(IDC_PRESET_EDIT), enable);
        ::EnableWindow(GetDlgItem(IDC_PRESET_DELETE), enable);
        
        ::EnableWindow(GetDlgItem(IDC_PRESET_MOVEUP), selIndex > 0);
        ::EnableWindow(GetDlgItem(IDC_PRESET_MOVEDOWN), selIndex < m_listView.GetItemCount() - 1);
        return 0;
    }

};

