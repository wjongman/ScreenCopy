#pragma once
#include "Settings.h"
#include "ImageSaver.h"
#include "SnapRect.h"
#include "AboutDlg.h"
#include "AutoSaveDlg.h"
#include "HotkeyDlg.h"
#include "PresetsDlg.h"
#include "TrayIcon.h"
#include "DragViewWindow.h"

/////////////////////////////////////////////////////////////////////////////
/// Array of connected monitors
///
struct MonitorRects
{
    std::vector<RECT> rects;
    MonitorRects() { EnumDisplayMonitors(0, 0, MonitorEnum, (LPARAM)this); }
    static BOOL CALLBACK MonitorEnum(
        HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData)
    {
        MonitorRects* pThis = reinterpret_cast<MonitorRects*>(pData);
        pThis->rects.push_back(*lprcMonitor);
        return TRUE;
    }

};

/////////////////////////////////////////////////////////////////////////////
/// Main window, a half-transparent window with no caption that can
/// capture the screen below it
///
class CScreenWindow : public CWindowImpl<CScreenWindow, CWindow,
          CWinTraits<WS_VISIBLE | WS_POPUP, WS_EX_LAYERED | WS_EX_TOPMOST>>
{
    bool m_bHasFocus;
    CTrayIcon m_trayIcon;
    Hotkey m_hotkey;
    PresetsList m_presetsList;
    UINT WM_TASKBARCREATED;
    CDragViewWindow m_dragWindow;

public:
    DECLARE_WND_CLASS_EX(L"ScreenCopyWindowClass", 0, COLOR_WINDOW)

private:
    BEGIN_MSG_MAP(CScreenWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnWindowPosChanging)
    MESSAGE_HANDLER(WM_SIZING, OnSizing)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_SYSKEYDOWN, OnSysKeyDown)
    MESSAGE_HANDLER(WM_NCLBUTTONDBLCLK, OnLButtonDoubleClick)
    MESSAGE_HANDLER(CTrayIcon::TRAYICONNOTIFY, OnTrayNotify)
    MESSAGE_HANDLER(WM_TASKBARCREATED, OnTaskbarCreated)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_HOTKEY, OnHotKey)
    MESSAGE_HANDLER(WM_NCHITTEST, OnHitTest)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_CLOSE, OnClose)
    END_MSG_MAP()

    //-------------------------------------------------------------------------
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        // Cursor
        HCURSOR hCursor = LoadCursor(NULL, IDC_SIZEALL);
        ::SetClassLong(m_hWnd, GCL_HCURSOR, (LONG)hCursor);

        // Icons
        HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(),
            MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON,
            ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON),
            LR_DEFAULTCOLOR);
        SetIcon(hIcon, TRUE);

        HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(),
            MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON,
            ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON),
            LR_DEFAULTCOLOR);
        SetIcon(hIconSmall, FALSE);

        // Tray-icon
        std::wstring sHintText = L"ScreenCopy";
        m_trayIcon.Init(m_hWnd, hIconSmall, sHintText.c_str());

        // We want to be notified when the task-bar restarts
        // so we can make our trayicon show after a crash of explorer
        // MSDN Home -> MSDN Library -> User Interface Design and Development ->
        // Shell Programmers Guide ->  Intermediate Shell Techniques ->
        // Programming the Shell -> The Taskbar
        UINT WM_TASKBARCREATED = ::RegisterWindowMessage(L"TaskbarCreated");

        // Window
        CSettings settings;
        CRect rcWindow = { 0, 0, 400, 200 };
        settings.RestoreWindowPlacement(m_hWnd, L"main", rcWindow);
        // Set the window to 50% opacity, max = 255
        SetLayeredWindowAttributes(m_hWnd, 0, 128, LWA_ALPHA);
        SetWindowText(L"ScreenCopy");

        // Presets
        m_presetsList = LoadPresets();
        // If presetsList is empty, add monitor rect(s)
        if (m_presetsList.empty())
        {
            MonitorRects mons;
            for (size_t i = 0; i < mons.rects.size(); ++i)
            {
                auto name = L"Monitor " + std::to_wstring(i + 1);
                m_presetsList.push_back({ name, mons.rects[i] });
            }
        }

        // Hotkey
        m_hotkey.Load();
        if (m_hotkey.CanRegister())
        {
            m_hotkey.Register(m_hWnd, 1);
        }
        else
        {
            CenterWindow();
            bool ok = SelectAndRegisterHotkey();
            if (!ok)
            {
                // No valid hotkey defined, bail out.
                return -1;
            }
        }


        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        CSettings settings;
        settings.SaveWindowPlacement(m_hWnd, L"main");
        m_hotkey.UnRegister(m_hWnd, 1);
        m_hotkey.Save();
        m_trayIcon.Remove();
        SavePresets();

        bHandled = FALSE;
        return 1;
    }

    //-------------------------------------------------------------------------
    LRESULT OnWindowPosChanging(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
    {
        WINDOWPOS* pwp = reinterpret_cast<WINDOWPOS*>(lParam);

        if (pwp->flags & SWP_SHOWWINDOW)
        {
            // window_was_shown
        }
        if (pwp->flags & SWP_HIDEWINDOW)
        {
            // window_was_hidden
        }
        if (!(pwp->flags & SWP_NOMOVE))
        {
            // window_moved_to(pwp->x, pwp->y);
            bool controlkeydown = GetKeyState(VK_CONTROL) & 0x8000;
            if (!controlkeydown)
            {
                // Snap to edges of work area
                int edgeSnapGap = 15;
                CSnapRect sr(pwp);
                sr.SnapToInsideOf(GetMonitorRect(), edgeSnapGap);
                pwp->x = sr.left;
                pwp->y = sr.top;
            }
            Invalidate();
        }
        if (!(pwp->flags & SWP_NOSIZE))
        {
            // window_resized_to(pwp->cx, pwp->cy);
            // Minimum size is 16x16 px.
            pwp->cx = pwp->cx < 16 ? 16 : pwp->cx;
            pwp->cy = pwp->cy < 16 ? 16 : pwp->cy;
            
            bool controlkeydown = GetKeyState(VK_CONTROL) & 0x8000;
            if (!controlkeydown)
            {
                // Snap to edges of work area
                int edgeSnapGap = 15;
                CSnapRect sr(pwp);
                sr.SnapToInsideOf(GetMonitorRect(), edgeSnapGap);
                pwp->cx = sr.Width();
                pwp->cy = sr.Height();
            }
            Invalidate();
        }

        // Don't forget to set bHandled to false !!
        bHandled = false;
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnSizing(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
    {
        CRect rcCurrent;
        GetWindowRect(&rcCurrent);
        
        // Constrain dimensions to square when shift key is down
        bool shift = ::GetKeyState(VK_SHIFT) & 0x8000;
        if (shift)
        {
            // https://stackoverflow.com/questions/20682975/resize-form-and-maintain-aspect-ratio
            CRect* prcNew = reinterpret_cast<CRect*>(lParam);
            int FAspectRatio = 1;
            switch (wParam)
            {
            case WMSZ_LEFT:
            case WMSZ_RIGHT:
                prcNew->bottom = prcNew->top + (prcNew->Width() / FAspectRatio);
                break;
            case WMSZ_TOP:
            case WMSZ_BOTTOM:
                prcNew->right = prcNew->left + (prcNew->Height() * FAspectRatio);
                break;

            case WMSZ_TOPLEFT:
            case WMSZ_TOPRIGHT:
            case WMSZ_BOTTOMLEFT:
            case WMSZ_BOTTOMRIGHT:
            {
                bool SizeBasedOnWidth;
                if (prcNew->Width() > rcCurrent.Width())
                {
                    SizeBasedOnWidth = prcNew->Height()
                        < MulDiv(rcCurrent.Height(), prcNew->Width(), rcCurrent.Width());
                }
                else
                {
                    SizeBasedOnWidth = prcNew->Width()
                        > MulDiv(rcCurrent.Width(), prcNew->Height(), rcCurrent.Height());
                }
                if (SizeBasedOnWidth)
                {
                    int NewHeight = prcNew->Width() / FAspectRatio;
                    switch (wParam)
                    {
                    case WMSZ_TOPLEFT:
                    case WMSZ_TOPRIGHT:
                        prcNew->top = prcNew->bottom - NewHeight;
                        break;
                    case WMSZ_BOTTOMLEFT:
                    case WMSZ_BOTTOMRIGHT:
                        prcNew->bottom = prcNew->top + NewHeight;
                        break;
                    }
                }
                else
                {
                    int NewWidth = prcNew->Height() * FAspectRatio;
                    switch (wParam)
                    {
                    case WMSZ_TOPLEFT:
                    case WMSZ_BOTTOMLEFT:
                        prcNew->left = prcNew->right - NewWidth;
                        break;
                    case WMSZ_TOPRIGHT:
                    case WMSZ_BOTTOMRIGHT:
                        prcNew->right = prcNew->left + NewWidth;
                        break;
                    }
                }
            }
            }
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
    {
        CPoint ptMouse;
        ptMouse.x = GET_X_LPARAM(lParam);
        ptMouse.y = GET_Y_LPARAM(lParam);

        CMenu popupMenu;
        popupMenu.CreatePopupMenu();

        popupMenu.AppendMenu(MF_STRING, ID_VIEW_CLOSE, L"Close\t&Esc");
        popupMenu.AppendMenu(MF_SEPARATOR);
        popupMenu.AppendMenu(MF_STRING, ID_SCREEN_COPY, L"Copy\t&C");
        popupMenu.AppendMenu(MF_STRING, ID_SCREEN_SAVE, L"Save\t&S");
        popupMenu.AppendMenu(MF_STRING, ID_SCREEN_DRAG, L"Drag Area\tDbl-Click");
        popupMenu.AppendMenu(MF_STRING, ID_SCREEN_SAVEAS, L"Save As...\tShift+&S");
        popupMenu.AppendMenu(MF_SEPARATOR);
        AppendPresetMenu(popupMenu);
        popupMenu.AppendMenu(MF_STRING, ID_VIEW_HOTKEY, L"Hotkey...");
        popupMenu.AppendMenu(MF_STRING, ID_VIEW_AUTOSAVE, L"Autosave...");
        popupMenu.AppendMenu(MF_SEPARATOR);
        popupMenu.AppendMenu(MF_STRING, ID_APP_ABOUT, L"About");
        popupMenu.AppendMenu(MF_STRING, ID_APP_EXIT, L"Exit\t&X");
        popupMenu.SetMenuDefaultItem(ID_VIEW_CLOSE, FALSE);

        // Pop-up where the right mouse button was pressed
        UINT cmd = popupMenu.TrackPopupMenu(
            TPM_CENTERALIGN | TPM_RETURNCMD, ptMouse.x, ptMouse.y, m_hWnd);

        return DispatchMenuCommand(cmd);
    }

    //-------------------------------------------------------------------------
    void AppendPresetMenu(CMenu &popupMenu)
    {
        CMenu subMenu;
        subMenu.CreatePopupMenu();
        subMenu.AppendMenu(MF_STRING, IDC_PRESET_ADD, L"Add...\t&A");
        subMenu.AppendMenu(MF_STRING, ID_VIEW_PRESETS, L"Manage...\t&M");
        subMenu.AppendMenu(MF_SEPARATOR);
        int i = 1;
        for (auto& preset : m_presetsList)
        {
            std::wstring title = preset.description + L"\t&Alt+" + std::to_wstring(i);
            subMenu.AppendMenu(MF_STRING, ID_PRESET_FIRST + i, title.c_str());
            if (i++ > 9)
                break;
        }
        popupMenu.AppendMenu(MF_POPUP, subMenu, L"Presets");
    }

    //-------------------------------------------------------------------------
    LRESULT DispatchMenuCommand(UINT cmd)
    {
        if (cmd > ID_PRESET_FIRST && cmd < ID_PRESET_FIRST + 9)
        {
            RestorePreset(cmd - ID_PRESET_FIRST - 1);
            return 0;
        }

        switch (cmd)
        {
        case ID_VIEW_CLOSE:
            ShowWindow(SW_HIDE);
            break;
        case ID_SCREEN_COPY:
        {
            ShowWindow(SW_HIDE);
            CopyScreen();
            break;
        }
        case ID_SCREEN_SAVE:
        {
            ShowWindow(SW_HIDE);
            SaveScreen();
            break;
        }
        case ID_SCREEN_DRAG:
        {
            ShowWindow(SW_HIDE);
            DragScreen();
            break;
        }
        case ID_SCREEN_SAVEAS:
        {
            ShowWindow(SW_HIDE);
            SaveScreenAs();
            break;
        }
        case ID_VIEW_HOTKEY:
        {
            SelectAndRegisterHotkey();
            break;
        }
        case ID_VIEW_AUTOSAVE:
        {
            CAutoSaveDlg dlg;
            dlg.DoModal();
            break;
        }
        case IDC_PRESET_ADD:
        {
            AddPreset();
            break;
        }
        case ID_VIEW_PRESETS:
        {
            ManagePresets();
            break;
        }
        case ID_APP_ABOUT:
        {
            CAboutDlg dlg;
            dlg.DoModal();
            break;
        }
        case ID_APP_EXIT:
            PostMessage(WM_CLOSE);
            break;
        default:
            ::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(cmd, 0), NULL);
            break;
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
        // Default arrow action is to move the form
        bool resize = false;

        // Resize form when control key is down
        if (::GetKeyState(VK_CONTROL) & 0x8000)
            resize = true;

        // Default increment is 1
        int delta = 1;

        // Increment is 10 when shift key is down
        if (::GetKeyState(VK_SHIFT) & 0x8000)
            delta = 10;

        CRect rcClient;
        GetWindowRect(&rcClient);
        switch (wParam)
        {
        case VK_LEFT:
            resize ? rcClient.right -= delta : rcClient.OffsetRect(-delta, 0);
            break;
        case VK_RIGHT:
            resize ? rcClient.right += delta : rcClient.OffsetRect(delta, 0);
            break;
        case VK_UP:
            resize ? rcClient.bottom -= delta : rcClient.OffsetRect(0, -delta);
            break;
        case VK_DOWN:
            resize ? rcClient.bottom += delta : rcClient.OffsetRect(0, delta);
            break;
        case VK_RETURN:
            break;

        case 'C':
            ShowWindow(SW_HIDE);
            CopyScreen();
            break;

        case 'D':
            ShowWindow(SW_HIDE);
            DragScreen();
            break;

        case 'S':
            ShowWindow(SW_HIDE);
            if (::GetKeyState(VK_SHIFT) & 0x8000)
            {
                SaveScreenAs();
            } 
            else
            {
                SaveScreen();
            }
            break;

        case 'A':
            AddPreset();
            break;

        case 'M':
            ManagePresets();
            break;

        case VK_ESCAPE:
            ShowWindow(SW_HIDE);

        case 'X':
            PostMessage(WM_CLOSE);
            break;

        }
        MoveWindow(rcClient, false);

        bHandled = FALSE;
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnSysKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        // Bit 29 of lParam is state of Alt key
        if (lParam & 0x20000000)
        {
            // Alt-<num> selects preset
            switch (wParam)
            {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                // 'Alt+1' means preset[0]
                RestorePreset(wParam - '0' - 1);
            }
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnLButtonDoubleClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        DragScreen();
        ShowWindow(SW_HIDE);
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnHotKey(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
        SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        ShowWindow(SW_SHOW);
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        // See if we need to resize or move
        LRESULT hit = ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
        if (hit == HTCLIENT)
        {
            // We want plenty of space for showing resize cursors.
            // Minimum space is 4 (for now).
            CRect rcClient;
            GetClientRect(&rcClient);
            int x_margin = 4 + rcClient.Width() / 10;
            int y_margin = 4 + rcClient.Height() / 10;

            CPoint pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ScreenToClient(&pt);

            CRect rcTopLeft(rcClient);
            rcTopLeft.right = rcTopLeft.left + x_margin;
            rcTopLeft.bottom = rcTopLeft.top + y_margin;
            if (PtInRect(&rcTopLeft, pt))
            {
                return HTTOPLEFT;
            }

            CRect rcTopRight(rcClient);
            rcTopRight.left = rcTopRight.right - x_margin;
            rcTopRight.bottom = rcTopRight.top + y_margin;
            if (PtInRect(&rcTopRight, pt))
            {
                return HTTOPRIGHT;
            }

            CRect rcBottomRight(rcClient);
            rcBottomRight.left = rcBottomRight.right - x_margin;
            rcBottomRight.top = rcBottomRight.bottom - y_margin;
            if (PtInRect(&rcBottomRight, pt))
            {
                return HTBOTTOMRIGHT;
            }

            CRect rcBottomLeft(rcClient);
            rcBottomLeft.right = rcBottomLeft.left + x_margin;
            rcBottomLeft.top = rcBottomLeft.bottom - y_margin;
            if (PtInRect(&rcBottomLeft, pt))
            {
                return HTBOTTOMLEFT;
            }

            CRect rcTop(rcClient);
            rcTop.bottom = rcTop.top + y_margin;
            if (PtInRect(&rcTop, pt))
            {
                return HTTOP;
            }

            CRect rcLeft(rcClient);
            rcLeft.right = rcLeft.left + x_margin;
            if (PtInRect(&rcLeft, pt))
            {
                return HTLEFT;
            }

            CRect rcBottom(rcClient);
            rcBottom.top = rcBottom.bottom - y_margin;
            if (PtInRect(&rcBottom, pt))
            {
                return HTBOTTOM;
            }

            CRect rcRight(rcClient);
            rcRight.left = rcRight.right - x_margin;
            if (PtInRect(&rcRight, pt))
            {
                return HTRIGHT;
            }

            if (::GetAsyncKeyState(MK_LBUTTON) < 0)
            {
                // We are being dragged
                return HTCAPTION;
            }
        }
        bHandled = false;
        return hit;
    }

    //-------------------------------------------------------------------------
    LRESULT OnTaskbarCreated(
        UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        // Windows Explorer has restarted
        m_trayIcon.Restore();
        return 0;
    }

    //----------------------------------------------------------------------------
    LRESULT OnTrayNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
    {
        switch (lParam)
        {
        case WM_RBUTTONUP:
            HandleTrayMenu();
            break;
        case WM_LBUTTONUP:
            SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            ShowWindow(SW_SHOW);
            break;
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    void HandleTrayMenu()
    {
        CMenu trayMenu;
        trayMenu.CreatePopupMenu();

        trayMenu.AppendMenu(MF_STRING, ID_VIEW_RESTORE, L"Restore");
        trayMenu.AppendMenu(MF_SEPARATOR);
        trayMenu.AppendMenu(MF_STRING, ID_VIEW_HOTKEY, L"Hotkey...");
        trayMenu.AppendMenu(MF_STRING, ID_VIEW_AUTOSAVE, L"Autosave...");
        trayMenu.AppendMenu(MF_STRING, ID_VIEW_PRESETS, L"Presets...");
        trayMenu.AppendMenu(MF_SEPARATOR);
        trayMenu.AppendMenu(MF_STRING, ID_APP_ABOUT, L"About");
        trayMenu.AppendMenu(MF_STRING, ID_APP_EXIT, L"Exit");
        trayMenu.SetMenuDefaultItem(ID_VIEW_RESTORE, FALSE);

        // Pop-up where the mouse button was pressed
        CPoint ptMouse;
        ::GetCursorPos(&ptMouse);
        int cmd = trayMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, 
            ptMouse.x, ptMouse.y, m_hWnd);

        DispatchMenuCommand(cmd);
    }

    //-------------------------------------------------------------------------
    LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        m_bHasFocus = true;
        Invalidate();
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        m_bHasFocus = false;
        Invalidate();
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CPaintDC dc(m_hWnd);
        CRect rc;
        GetClientRect(&rc);
        
        CPen inactivePen = CreatePen(PS_DOT, 1, COLOR_INACTIVECAPTION);
        CPen hilitePen = CreatePen(PS_SOLID, 1, RGB(255, 0, 255));
        if (m_bHasFocus)
        {
            dc.SelectPen(hilitePen);
        }
        else
        {
            dc.SelectPen(inactivePen);
        }
        
        // Borders
        dc.MoveTo(rc.left, rc.top);
        dc.LineTo(rc.right - 1, rc.top);
        dc.LineTo(rc.right - 1, rc.bottom - 1);
        dc.LineTo(rc.left, rc.bottom - 1);
        dc.LineTo(rc.left, rc.top);

        // Determine size of sizegrip
        int gap = 4;
        int x_incr = GetSystemMetrics(SM_CXVSCROLL) / gap;
        int y_incr = GetSystemMetrics(SM_CXHSCROLL) / gap;

        // top-left sizegrip
        for (int i = 1; i <= gap; i++)
        {
            dc.MoveTo(rc.left, rc.top + i * y_incr);
            dc.LineTo(rc.left + i * x_incr, rc.top);
        }

        for (int i = 1; i <= gap; i++)
        {
            // top-right sizegrip
            dc.MoveTo(rc.right, rc.top + i * y_incr);
            dc.LineTo(rc.right - i * x_incr, rc.top);
        }

        for (int i = 1; i <= gap; i++)
        {
            // bottom-right sizegrip
            dc.MoveTo(rc.right, rc.bottom - i * y_incr);
            dc.LineTo(rc.right - i * x_incr, rc.bottom);
        }

        for (int i = 1; i <= gap; i++)
        {
            // bottom-left sizegrip
            dc.MoveTo(rc.left, rc.bottom - i * y_incr);
            dc.LineTo(rc.left + i * x_incr, rc.bottom);
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        PostQuitMessage(0);
        bHandled = FALSE;
        return 1;
    }

    //-------------------------------------------------------------------------
    LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
        BOOL& /*bHandled*/)
    {
        PostMessage(WM_CLOSE);
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,
        BOOL& /*bHandled*/)
    {
        CAboutDlg dlg;
        dlg.DoModal();
        return 0;
    }

    //-------------------------------------------------------------------------
    bool SelectAndRegisterHotkey()
    {
        CHotkeyDlg dlg(m_hotkey);
        if (dlg.DoModal() == IDOK)
        {
            m_hotkey = dlg.GetHotkey();
            m_hotkey.Register(m_hWnd, 1);
            return true;
        }
        return false;
    }

    //-------------------------------------------------------------------------
    void AddPreset()
    {
        CRect rc;
        GetWindowRect(&rc);
        CEditPresetDlg dlg({ L"", rc });
        dlg.SetCaption(L"Add Preset");
        if (dlg.DoModal() == IDOK)
        {
            m_presetsList.push_back(dlg.GetPreset());
        }
    }

    //-------------------------------------------------------------------------
    void ManagePresets()
    {
        CManagePresetsDlg dlg(m_presetsList);
        int result = dlg.DoModal();
        if (result >= ID_PRESET_FIRST && result < ID_PRESET_FIRST + 9)
        {
            m_presetsList = dlg.GetPresets();
            int selIndex = result - ID_PRESET_FIRST;
            RestorePreset(selIndex);
        }
        else if (result == IDOK)
        {
            m_presetsList = dlg.GetPresets();
        }
    }

    //---------------------------------------------------------------------------
    HBITMAP GrabScreen(CRect const& rc)
    {
        static CBitmap m_bitmap = 0;

        CDC dcScreen = CreateDC(L"DISPLAY", NULL, NULL, NULL);
        CDC dcMem = CreateCompatibleDC(dcScreen);
        m_bitmap = CreateCompatibleBitmap(dcScreen, rc.Width(), rc.Height());
        HBITMAP hOldBitmap = dcMem.SelectBitmap(m_bitmap);
        BitBlt(dcMem, 0, 0, rc.Width(), rc.Height(), dcScreen, rc.left,
            rc.top, SRCCOPY);
        m_bitmap = dcMem.SelectBitmap(hOldBitmap);

        return m_bitmap;
    }

    //---------------------------------------------------------------------------
    void CopyToClipboard(HBITMAP hBitmap)
    {
        OpenClipboard();
        EmptyClipboard();
        HBITMAP hBitmapCopy
            = (HBITMAP)::CopyImage(hBitmap, IMAGE_BITMAP, 0, 0, 0);
        SetClipboardData(CF_BITMAP, hBitmapCopy);
        CloseClipboard();
        DeleteObject(hBitmap);
    }

    //-------------------------------------------------------------------------
    void CopyScreen()
    {
        CRect rcWindow;
        GetWindowRect(&rcWindow);
        HBITMAP hBmp = GrabScreen(rcWindow);
        CopyToClipboard(hBmp);
    }

    //-------------------------------------------------------------------------
    void SaveScreen()
    {
        CRect rcWindow;
        GetWindowRect(&rcWindow);
        HBITMAP hBmp = GrabScreen(rcWindow);
        ImageSaver saver;
        saver.AutoSaveImage(hBmp);
    }

    //-------------------------------------------------------------------------
    void DragScreen()
    {
        CRect rcWindow;
        GetWindowRect(&rcWindow);
        HBITMAP hBmp = GrabScreen(rcWindow);
        ImageSaver saver;
        saver.SaveDragImage(hBmp);
        CPoint ptMouse;
        GetCursorPos(&ptMouse);
        ShowDragViewWindow(ptMouse);
    }

    //-------------------------------------------------------------------------
    void SaveScreenAs()
    {
        CRect rcWindow;
        GetWindowRect(&rcWindow);
        HBITMAP hBmp = GrabScreen(rcWindow);
        ImageSaver saver;
        saver.SaveImageAs(hBmp);
    }

    //-------------------------------------------------------------------------
    void ShowDragViewWindow(CPoint const& ptShow)
    {
        CSettings settings;
        std::wstring savePath = settings.GetString(L"autosave", L"directory", GetDesktopPath()).c_str();

        CRect rcWindow;
        GetWindowRect(&rcWindow);
        if (!m_dragWindow.IsWindow())
        {
            m_dragWindow.Create(m_hWnd);
        }
        m_dragWindow.SetDragFilePath(savePath + L"\\ScreenCopy.png");
        if (!m_dragWindow.IsWindowVisible())
        {
            CRect rcDrag{ 0, 0, 120, 120 };
            rcDrag.OffsetRect(ptShow.x - 60, ptShow.y - 60);
            m_dragWindow.SetWindowPos(HWND_TOP, &rcDrag, SWP_SHOWWINDOW);
        }
    }

    //-------------------------------------------------------------------------
    // Return bounding rectangle of virtual desktop
    CRect GetScreenDimensions()
    {
        int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        
        return CRect(x, y, x + cx, y + cy);
    }

    //---------------------------------------------------------------------------
    CRect GetMonitorRect()
    {
        HMONITOR hMon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi;
        mi.cbSize = sizeof MONITORINFO;
        GetMonitorInfo(hMon, &mi);
        return mi.rcMonitor;
    }

    //---------------------------------------------------------------------------
    void RestorePreset(int index)
    {
        SetWindowPos(HWND_TOP, m_presetsList[index].rect, SWP_SHOWWINDOW);
    }

    //---------------------------------------------------------------------------
    PresetsList LoadPresets()
    {
        PresetsList presetsList;

        CSettings settings;
        std::wstring keyName = L"presets";
        for (int i = 1; i < 99; ++i) // 99 presets should be enough for everyone :-)
        {
            std::wstring commatext = settings.GetString(keyName, std::to_wstring(i), L"");
            if (commatext.empty())
                return presetsList;

            presetsList.push_back(GrabberPreset(commatext));
        }
        return presetsList;
    }

    //---------------------------------------------------------------------------
    void SavePresets()
    {
        CSettings settings;
        std::wstring section = L"presets";
        settings.DeleteSection(section);
        for (size_t i = 0; i < m_presetsList.size(); ++i)
        {
            settings.SetString(section, std::to_wstring(i + 1), m_presetsList[i].GetCommaText());
        }
    }

};
