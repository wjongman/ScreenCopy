#pragma once
#include "Settings.h"
#include "AutoSave.h"
#include "SnapRect.h"
#include "AboutDlg.h"
#include "AutoSaveDlg.h"
#include "HotkeyDlg.h"
#include "PresetsDlg.h"

/////////////////////////////////////////////////////////////////////////////
/// Array of connected monitors
struct MonitorRects
{
    std::vector<RECT> rcMonitors;
    static BOOL CALLBACK MonitorEnum(
        HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData)
    {
        MonitorRects* pThis = reinterpret_cast<MonitorRects*>(pData);
        pThis->rcMonitors.push_back(*lprcMonitor);
        return TRUE;
    }

    MonitorRects() { EnumDisplayMonitors(0, 0, MonitorEnum, (LPARAM)this); }
};

/////////////////////////////////////////////////////////////////////////////
/// A half-transparent window with no caption that can
/// capture the screen below it
class CScreenWindow : public CWindowImpl<CScreenWindow, CWindow,
          CWinTraits<WS_VISIBLE | WS_POPUP, WS_EX_LAYERED | WS_EX_TOPMOST>>
{
    bool m_bHasFocus;
    Hotkey m_hotkey;
   
public:
    DECLARE_WND_CLASS_EX(L"ScreenCopyWindowClass", 0, COLOR_WINDOW)

private:
    BEGIN_MSG_MAP(CScreenWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnWindowPosChanging)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_SYSKEYDOWN, OnSysKeyDown)
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

        // Window
        CSettings settings;
        CRect rcWindow = { 0, 0, 400, 200 };
        settings.RestoreWindowPlacement(m_hWnd, L"main", rcWindow);
        // Set the window to 50% opacity, max = 255
        SetLayeredWindowAttributes(m_hWnd, 0, 128, LWA_ALPHA);
        SetWindowText(L"ScreenCopy");
        
        // Hotkey
        m_hotkey.Load();
        if (m_hotkey.CanRegister())
        {
            m_hotkey.Register(m_hWnd, 1);
        }
        else
        {
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
    bool SelectAndRegisterHotkey()
    {
        CHotkeyDlg dlg(m_hotkey);
        if (dlg.DoModal() == IDOK)
        {
            m_hotkey = dlg.GetHotkey();
            m_hotkey.Register(m_hWnd, 1);
            return true;
        }
        else
        {
            return false;
        }
    }

    //-------------------------------------------------------------------------
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        m_hotkey.UnRegister(m_hWnd, 1);
        m_hotkey.Save();

        CSettings settings;
        settings.SaveWindowPlacement(m_hWnd, L"main");

        bHandled = FALSE;
        return 1;
    }

    //-------------------------------------------------------------------------
    LRESULT OnWindowPosChanging(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
    {
        WINDOWPOS* pwp = reinterpret_cast<WINDOWPOS*>(lParam);

        if (pwp->flags & SWP_SHOWWINDOW)
        {
            // window_was_shown();
        }
        if (pwp->flags & SWP_HIDEWINDOW)
        {
            // window_was_hidden();
        }
        if (!(pwp->flags & SWP_NOMOVE))
        {
            // window_moved_to(pwp->x, pwp->y);
            bool controlkeydown = GetKeyState(VK_CONTROL) & 0x8000;
            if (!controlkeydown)
            {
                // Snap to edges of work area
                int EdgeSnapGap = 15;
                CSnapRect sr(pwp);
                sr.SnapToInsideOf(GetMonitorRect(), EdgeSnapGap);
                pwp->x = sr.left;
                pwp->y = sr.top;
            }
            Invalidate();
        }
        if (!(pwp->flags & SWP_NOSIZE))
        {
            // window_resized_to(pwp->cx, pwp->cy);
            bool controlkeydown = GetKeyState(VK_CONTROL) & 0x8000;
            if (!controlkeydown)
            {
                // Snap to edges of work area
                int EdgeSnapGap = 15;
                CSnapRect sr(pwp);
                sr.SnapToInsideOf(GetMonitorRect(), EdgeSnapGap);
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
    LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
    {
        CPoint ptMouse;
        ptMouse.x = GET_X_LPARAM(lParam);
        ptMouse.y = GET_Y_LPARAM(lParam);

        CMenu PopupMenu;
        PopupMenu.CreatePopupMenu();

        PopupMenu.AppendMenu(MF_STRING, ID_VIEW_CLOSE, L"Close\t&Esc");
        PopupMenu.AppendMenu(MF_SEPARATOR);
        PopupMenu.AppendMenu(MF_STRING, ID_SCREEN_COPY, L"Copy\t&C");
        PopupMenu.AppendMenu(MF_STRING, ID_SCREEN_SAVE, L"Save\t&S");
        PopupMenu.AppendMenu(MF_SEPARATOR);
        PopupMenu.AppendMenu(MF_STRING, ID_VIEW_HOTKEY, L"Hotkey...");
        PopupMenu.AppendMenu(MF_STRING, ID_VIEW_OPTIONS, L"Autosave...");
        PopupMenu.AppendMenu(MF_STRING, ID_VIEW_PRESETS, L"Presets...");
        PopupMenu.AppendMenu(MF_STRING, ID_APP_ABOUT, L"About");
        PopupMenu.AppendMenu(MF_SEPARATOR);
        PopupMenu.AppendMenu(MF_STRING, ID_APP_EXIT, L"Exit");
        PopupMenu.SetMenuDefaultItem(ID_VIEW_CLOSE, FALSE);

        // Pop-up where the right mouse button was pressed
        UINT cmd = PopupMenu.TrackPopupMenu(
            TPM_LEFTALIGN | TPM_RETURNCMD, ptMouse.x, ptMouse.y, m_hWnd);

        switch (cmd)
        {
        case ID_APP_EXIT:
            PostMessage(WM_CLOSE);
            break;
        case ID_VIEW_CLOSE:
            ShowWindow(SW_HIDE);
            break;
        case ID_APP_ABOUT:
        {
            CAboutDlg dlg;
            dlg.DoModal();
            break;
        }
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
        case ID_VIEW_HOTKEY:
        {
            SelectAndRegisterHotkey();
            break;
        }
        case ID_VIEW_OPTIONS:
        {
            CAutoSaveDlg dlg;
            dlg.DoModal();
            break;
        }
        case ID_VIEW_PRESETS:
        {
            CPresetsDlg dlg;
            dlg.DoModal();
            break;
        }
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

        case 'S':
            ShowWindow(SW_HIDE);
            SaveScreen();
            break;

        case VK_ESCAPE:
            ShowWindow(SW_HIDE);
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
            // Alt-<num> selects monitor
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
                // '1' means monitor[0]
                UINT num = wParam - '0' - 1;
                MonitorRects monitors;
                if (num < monitors.rcMonitors.size())
                {
                    SetWindowPos(HWND_TOP, &monitors.rcMonitors[num], SWP_SHOWWINDOW);
                }
            }
        }
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
        return hit;
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
        
        CPen activePen = CreatePen(PS_SOLID, 1, COLOR_ACTIVECAPTION);
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
        AutoSaver saver;
        saver.SaveImage(hBmp);
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

};
