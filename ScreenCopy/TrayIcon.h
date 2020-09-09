#ifndef TrayIconH
#define TrayIconH


///////////////////////////////////////////////////////////////////////////////
class CTrayIcon
{
private:
    const UINT m_uID;
    bool m_isInitialized;
    HWND m_hWnd;
    HICON m_hIcon;

public:
    static const int TRAYICONNOTIFY = WM_APP + 100;

    //-------------------------------------------------------------------------
    CTrayIcon() : m_isInitialized(false), m_uID(1600) 
    {
    }

    //-------------------------------------------------------------------------
    ~CTrayIcon()
    {
    }

public:
    //-------------------------------------------------------------------------
    void Init(HWND hWindow, HICON hIcon, std::wstring const& sTip = L"")
    {
        // get a local copy of parent window handle
        m_hWnd = hWindow;
        // get a local copy of icon handle
        m_hIcon = hIcon;

        m_isInitialized = true;
        TrayMessage(m_hWnd, NIM_ADD, hIcon, sTip);
    }

    //-------------------------------------------------------------------------
    void SetIcon(HICON hIcon, std::wstring const& sTip = L"")
    {
        TrayMessage(m_hWnd, NIM_MODIFY, hIcon, sTip);
    }

    //-------------------------------------------------------------------------
    void SetTooltipText(std::wstring const& sTip)
    {
        TrayMessage(m_hWnd, NIM_MODIFY, NULL, sTip);
    }

    //-------------------------------------------------------------------------
    void Add()
    {
        TrayMessage(m_hWnd, NIM_ADD, m_hIcon);
    }

    //-------------------------------------------------------------------------
    void Remove()
    {
        TrayMessage(m_hWnd, NIM_DELETE);
    }

    //-------------------------------------------------------------------------
    void Change()
    {
        TrayMessage(m_hWnd, NIM_MODIFY);
    }

    //-------------------------------------------------------------------------
    void Restore() 
    { 
        Remove(); 
        Add(); 
    }

private:
    //-------------------------------------------------------------------------
    BOOL TrayMessage(HWND hWnd, DWORD dwMessage,
        HICON hIcon = 0, std::wstring const& sTip = L"")
    {
        if (m_isInitialized)
        {
            NOTIFYICONDATA nid; 
            ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
            nid.cbSize = NOTIFYICONDATA_V1_SIZE;
            nid.uID = m_uID;
            nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
            nid.hIcon = hIcon;
            nid.hWnd = hWnd;
            nid.uCallbackMessage = TRAYICONNOTIFY;
            
            if (!sTip.empty())
            {
                lstrcpyn(nid.szTip, sTip.c_str(), sizeof(nid.szTip));
            }

            return Shell_NotifyIcon(dwMessage, &nid);
        }
        return false;
    }
};

#endif
