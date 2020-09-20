// View.h : interface of the CDragViewWindow class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "DragSource.h"

#include <gdiplus.h>
#pragma comment(lib, "Gdiplus.lib")

#include <memory>
#include "ImageSaver.h"
#include "Clipboard.h"

///////////////////////////////////////////////////////////////////////////////
//
//  Window that holds a draggable bitmap
//
class CDragViewWindow : public CWindowImpl<CDragViewWindow, CWindow,
                            CWinTraits<WS_BORDER | WS_SYSMENU | WS_THICKFRAME, WS_EX_TOOLWINDOW>>
{
    ULONG_PTR m_gdiplusToken;
    bool m_bDragging = false;
    std::wstring m_dragFilePath;
    HCURSOR m_cursor;

public:
    DECLARE_WND_CLASS(NULL)

    //-------------------------------------------------------------------------
    void SetDragFilePath(std::wstring const& filePath) 
    { 
        m_dragFilePath = filePath;
        Invalidate();
    }

private:
    //-------------------------------------------------------------------------
    BOOL PreTranslateMessage(MSG* pMsg)
    {
        pMsg;
        return FALSE;
    }

    BEGIN_MSG_MAP(CDragViewWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_CLOSE, OnClose)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDoubleClick)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    END_MSG_MAP()

    //-------------------------------------------------------------------------
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        SetWindowText(L"Drag Area");
        m_cursor = ::LoadCursor(_Module.GetResourceInstance(), (LPCWSTR)IDC_DRAG);
        ::SetClassLongPtr(m_hWnd, GCL_HCURSOR, (LONG)m_cursor);
        ::SetClassLongPtr(m_hWnd, GCL_HBRBACKGROUND, COLOR_APPWORKSPACE+1);
        // Initialize GDI+.
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
        ::DeleteFile(m_dragFilePath.c_str());
        bHandled = FALSE;
        return 1;
    }

    //-------------------------------------------------------------------------
    LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        ShowWindow(SW_HIDE);
        bHandled = true;
        return 1;
    }

    //-------------------------------------------------------------------------
    LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        CPoint ptMouse;
        ptMouse.x = GET_X_LPARAM(lParam);
        ptMouse.y = GET_Y_LPARAM(lParam);

        CMenu popupMenu;
        popupMenu.CreatePopupMenu();

        popupMenu.AppendMenu(MF_STRING, ID_VIEW_RESTORE, L"ScreenCopy\tDbl-Click");
        popupMenu.AppendMenu(MF_SEPARATOR);
        popupMenu.AppendMenu(MF_STRING, ID_SCREEN_SAVEAS, L"Save As...");
        popupMenu.AppendMenu(MF_STRING, ID_SCREEN_SAVE, L"Save Scaled");
        popupMenu.AppendMenu(MF_STRING, ID_SCREEN_COPY, L"Copy File Path");
        popupMenu.AppendMenu(MF_SEPARATOR);
        popupMenu.AppendMenu(MF_STRING, ID_VIEW_CLOSE, L"Close\tEsc");
        popupMenu.SetMenuDefaultItem(ID_VIEW_RESTORE, FALSE);

        // Pop-up where the right mouse button was pressed
        UINT cmd = popupMenu.TrackPopupMenu(
            TPM_CENTERALIGN | TPM_RETURNCMD, ptMouse.x, ptMouse.y, m_hWnd);

        switch (cmd)
        {
        case ID_VIEW_RESTORE:
            RestoreCopyWindow();
            break;
        case ID_SCREEN_SAVEAS:
        {
            SaveImageAs(m_dragFilePath);
            break;
        }
        case ID_SCREEN_SAVE:
            SaveScaledDragImage(m_dragFilePath);
            break;
        case ID_SCREEN_COPY:
            Clipboard::Write(m_dragFilePath);
            break;
        case ID_VIEW_CLOSE:
            ShowWindow(SW_HIDE);
            break;
        default:
            break;
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnLButtonDoubleClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        RestoreCopyWindow();
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        m_bDragging = true;
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
        if (wParam == VK_ESCAPE)
        {
            ShowWindow(SW_HIDE);
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
//         SetCursor();
        bHandled = true;
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        if (m_bDragging)
        {
            SetCapture();
            BeginDrag(m_dragFilePath);
            ReleaseCapture();
            m_bDragging = false;
            // Hide if button released outside client area
            CPoint ptMouse{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            CRect rcClient;
            GetClientRect(&rcClient);
            if (!PtInRect(&rcClient, ptMouse))
            {
                ShowWindow(SW_HIDE);
            }
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT BeginDrag(std::wstring const& filePath)
    {
        CComPtr<IDataObject> spDataObject;
        if (SUCCEEDED(GetUIObjectOfFile(m_hWnd, filePath.c_str(), IID_PPV_ARGS(&spDataObject))))
        {
            IDropSource* pDropSource = new CDropSource();
            if (pDropSource)
            {
                DWORD dwEffect;
                DoDragDrop(spDataObject, pDropSource, DROPEFFECT_COPY | DROPEFFECT_LINK, &dwEffect);
                // pDropSource will delete itself when refcount becomes 0
                pDropSource->Release();
            }
        }
        return 0;
    }

    using BitmapPtr = std::unique_ptr<Gdiplus::Bitmap>;
    //-------------------------------------------------------------------------
    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        CPaintDC dc(m_hWnd);
        CRect rcClient;
        GetClientRect(&rcClient);
        //dc.FillRect(rcClient, COLOR_APPWORKSPACE);
        BitmapPtr pScaledBmp = GetScaledBitmap(m_dragFilePath, rcClient);

        int bmpW = pScaledBmp->GetWidth();
        int bmpH = pScaledBmp->GetHeight();
        int offsetX = (rcClient.Width() - bmpW) / 2;
        int offsetY = (rcClient.Height() - bmpH) / 2;
//         CRect rcFrame{ -1, -1, bmpW, bmpH };
//         rcFrame.OffsetRect(offsetX, offsetY);
//         dc.Rectangle(rcFrame);

        Gdiplus::Graphics g(dc);
        g.DrawImage(pScaledBmp.get(), offsetX, offsetY);
        Gdiplus::Pen blackpen(Gdiplus::Color(0, 0, 0));
        Gdiplus::Rect rcFrame(offsetX, offsetY, bmpW, bmpH);
        g.DrawRectangle(&blackpen, rcFrame);

        return 0;
    }

    //-------------------------------------------------------------------------
    void SaveScaledDragImage(std::wstring const& filePath)
    {
        CRect rcClient;
        GetClientRect(&rcClient);
        BitmapPtr scaledBmp = GetScaledBitmap(filePath, rcClient);
        HBITMAP hDestBmp;
        Gdiplus::Status status = scaledBmp->GetHBITMAP(RGB(0, 0, 0), &hDestBmp);
        if (status == Gdiplus::Ok)
        {
            ImageSaver saver;
            saver.SaveDragImage(hDestBmp);
        }
    }

    //-------------------------------------------------------------------------
    void SaveImageAs(std::wstring const& filePath)
    {
        Gdiplus::Bitmap bitmap(filePath.c_str());
        HBITMAP hBitmap;
        Gdiplus::Status status = bitmap.GetHBITMAP(RGB(0, 0, 0), &hBitmap);
        if (status == Gdiplus::Ok)
        {
            ImageSaver saver;
            saver.SaveImageAs(hBitmap);
        }
    }

    //-------------------------------------------------------------------------
    BitmapPtr GetScaledBitmap(std::wstring const& filePath, CRect const& rcTarget)
    {
        ATLASSERT(!filePath.empty());

        int targetW = rcTarget.Width();
        int targetH = rcTarget.Height();
        int targetSize = min(targetW, targetH);

        Gdiplus::Bitmap sourceBmp(filePath.c_str());
        int sourceW = sourceBmp.GetWidth();
        int sourceH = sourceBmp.GetHeight();
        int sourceSize = 0;// = max(sourceW, sourceH);

        float scaleW = ((float)targetW / (float)sourceW);
        float scaleH = ((float)targetH / (float)sourceH);
        float scale = (scaleW < scaleH) ? scaleW : scaleH;

        int resultW = (int)(sourceW * scale);
        int resultH = (int)(sourceH * scale);

        Gdiplus::Bitmap resultBmp(resultW, resultH);
        Gdiplus::Graphics g(&resultBmp);
        g.ScaleTransform(scale, scale);
        g.DrawImage(&sourceBmp, 0, 0);

        return BitmapPtr(resultBmp.Clone(0, 0, resultW, resultH, PixelFormat32bppARGB));
    }
    
    //-------------------------------------------------------------------------
    void RestoreCopyWindow()
    {
        HWND hwnd = ::FindWindow(L"ScreenCopyWindowClass", 0);
        if (hwnd)
        {
            ::ShowWindow(hwnd, SW_SHOW);
            ::SetForegroundWindow(hwnd);
            ::BringWindowToTop(hwnd);
        }
    }

};
