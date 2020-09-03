#pragma once

///////////////////////////////////////////////////////////////////////////////
//  Encapsulation of Hotkey API
//
enum HotkeyStatus { HK_REGISTRATION_FAILED, HK_REGISTERED, HK_ALREADY_REGISTERED };

struct Hotkey
{
    WORD modifiers = 0;
    WORD keycode = 0;
    void Load()
    {
        CSettings settings;
        keycode = settings.GetInt(L"hotkey", L"keycode", 0);
        modifiers = settings.GetInt(L"hotkey", L"modifiers", 0);
    }
    void Save()
    {
        CSettings settings;
        settings.SetInt(L"hotkey", L"keycode", keycode);
        settings.SetInt(L"hotkey", L"modifiers", modifiers);
    }
    bool CanRegister()
    {
        if (keycode != 0 && Register(NULL, 1) == HK_REGISTERED)
        {
            UnRegister(NULL, 1);
            return true;
        } 
        return false;
    }
    HotkeyStatus Register(HWND hwnd, int id)
    {
        int result = ::RegisterHotKey(hwnd, id, modifiers, keycode);
        if (result == 0)
        {
            DWORD error = GetLastError();
            if (error == ERROR_HOTKEY_ALREADY_REGISTERED)
            {
                return HK_ALREADY_REGISTERED;
            }
            return HK_REGISTRATION_FAILED;
        }
        return HK_REGISTERED;
    }
    void UnRegister(HWND hwnd, int id)
    { 
        ::UnregisterHotKey(hwnd, id);
    }
};

///////////////////////////////////////////////////////////////////////////////
//                                                                                        !m_h
// Dialog for selecting a hotkey combination
//
class CHotkeyDlg : public CDialogImpl<CHotkeyDlg>
{
    CStatic m_warnLabel;
    CHotKeyCtrl m_hotkeyCtrl;
    CButton m_okButton;
    Hotkey m_hotkeyOld;
    Hotkey m_hotkey;

public:
    enum { IDD = IDD_HOTKEYDLG };

    BEGIN_MSG_MAP(CAutoSaveDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDC_HOTKEY, EN_CHANGE, OnEditChange)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC , OnCtlColor)
    END_MSG_MAP()

    CHotkeyDlg(Hotkey const& hotkey) : m_hotkey(hotkey) {}
    Hotkey GetHotkey() { return m_hotkey; }

private:
    //---------------------------------------------------------------------------
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CenterWindow(GetParent());
        m_warnLabel.Attach(GetDlgItem(IDC_WARNING));
        m_warnLabel.ShowWindow(SW_HIDE);
        m_okButton.Attach(GetDlgItem(IDOK));

        // Disable current hotkey for duration of this dialog
        m_hotkeyOld = m_hotkey;
        m_hotkeyOld.UnRegister(GetParent(), 1);

        m_hotkeyCtrl.Attach(GetDlgItem(IDC_HOTKEY));
        m_hotkeyCtrl.SetHotKey(m_hotkey.keycode, m_hotkey.modifiers);
        if (!m_hotkey.CanRegister())
        {
            if (m_hotkey.keycode != 0)
            {
                m_warnLabel.ShowWindow(SW_SHOW);
            }
            m_okButton.EnableWindow(false);
        }
        return TRUE;
    }

    //-------------------------------------------------------------------------
    LRESULT OnEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        m_warnLabel.ShowWindow(SW_HIDE);
        m_hotkeyCtrl.GetHotKey(m_hotkey.keycode, m_hotkey.modifiers);
        if (m_hotkey.keycode == 0)
            return 0;

        if (m_hotkey.CanRegister())
        {
            m_okButton.EnableWindow(true);
        }
        else
        {
            m_warnLabel.ShowWindow(SW_SHOW);
            m_okButton.EnableWindow(false);
        }
        return 0;
    }

    //---------------------------------------------------------------------------
    LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        m_hotkeyOld.Register(GetParent(), 1);
        EndDialog(wID);
        return 0;
    }
    //---------------------------------------------------------------------------
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        m_hotkeyOld.Register(GetParent(), 1);
        m_hotkey = m_hotkeyOld;
        EndDialog(wID);
        return 0;
    }

    //---------------------------------------------------------------------------
    LRESULT OnCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
    {
        HWND hwnd = reinterpret_cast<HWND>(lParam);
        if (hwnd == m_warnLabel.m_hWnd)
        {
            // we're about to draw the warning label
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(0, 0, 255));
            return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_MENU));
        }
        return FALSE;
    }

};

