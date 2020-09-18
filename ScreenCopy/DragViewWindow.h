// View.h : interface of the CDragViewWindow class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "DragSource.h"

#include <gdiplus.h>
#pragma comment(lib, "Gdiplus.lib")
#include "ImageSaver.h"

class CDragViewWindow : public CWindowImpl<CDragViewWindow, CWindow,
                            CWinTraits<WS_BORDER | WS_SYSMENU | WS_THICKFRAME, WS_EX_TOOLWINDOW>>
{
    ULONG_PTR m_gdiplusToken;
    bool m_bDragging = false;
    std::wstring m_dragFilePath;

public:
    DECLARE_WND_CLASS(NULL)

    //-------------------------------------------------------------------------
    void SetDragFilePath(std::wstring const& filePath) 
    { 
        m_dragFilePath = filePath; 
    }

private:
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
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    END_MSG_MAP()

    //-------------------------------------------------------------------------
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        SetWindowText(L"drag view");

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

        popupMenu.AppendMenu(MF_STRING, ID_VIEW_CLOSE, L"Close");
        popupMenu.AppendMenu(MF_SEPARATOR);
        popupMenu.AppendMenu(MF_STRING, ID_SCREEN_SAVE, L"Save Rescaled");
        popupMenu.SetMenuDefaultItem(ID_VIEW_CLOSE, FALSE);

        // Pop-up where the right mouse button was pressed
        UINT cmd = popupMenu.TrackPopupMenu(
            TPM_CENTERALIGN | TPM_RETURNCMD, ptMouse.x, ptMouse.y, m_hWnd);

        switch (cmd)
        {
        case ID_VIEW_CLOSE:
            ShowWindow(SW_HIDE);
            break;
        case ID_SCREEN_SAVE:
            SaveScaledDragImage();
            break;
        default:
            break;
        }
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        m_bDragging = true;
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
            ShowWindow(SW_HIDE);
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

    //-------------------------------------------------------------------------
    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        CPaintDC dc(m_hWnd);
        CRect rcClient;
        GetClientRect(&rcClient);
        dc.FillRect(rcClient, COLOR_APPWORKSPACE);

        Gdiplus::Bitmap* scaledBmp = GetScaledDragImage();

        int offsetX = (rcClient.Width() - scaledBmp->GetWidth()) / 2;
        int offsetY = (rcClient.Height() - scaledBmp->GetHeight()) / 2;

        Gdiplus::Graphics g(dc);
        g.DrawImage(scaledBmp, offsetX, offsetY);

        return 0;
    }

    //-------------------------------------------------------------------------
    Gdiplus::Bitmap* GetScaledDragImage()
    {
        CRect rcClient;
        GetClientRect(&rcClient);

        int clientW = rcClient.Width();
        int clientH = rcClient.Height();
        int destSize = min(clientW, clientH);

        Gdiplus::Bitmap srcBmp(m_dragFilePath.c_str());
        int srcW = srcBmp.GetWidth();
        int srcH = srcBmp.GetHeight();
        int srcSize = max(srcW, srcH);

        float scale = (float)destSize / (float)srcSize;

        int resultW = (int)(srcW * scale);
        int resultH = (int)(srcH * scale);

        Gdiplus::Bitmap resultBmp(resultW, resultH);
        Gdiplus::Graphics g(&resultBmp);
        g.ScaleTransform(scale, scale);
        g.DrawImage(&srcBmp, 0, 0);

        return resultBmp.Clone(0, 0, resultW, resultH, PixelFormat32bppARGB);
    }

    //-------------------------------------------------------------------------
    void SaveScaledDragImage()
    {
        Gdiplus::Bitmap* scaledBmp = GetScaledDragImage();
        HBITMAP hDestBmp;
        Gdiplus::Status status = scaledBmp->GetHBITMAP(RGB(0, 0, 0), &hDestBmp);
        if (status == Gdiplus::Ok)
        {
            ImageSaver saver;
            saver.SaveDragImage(hDestBmp);
        }
    }
};
