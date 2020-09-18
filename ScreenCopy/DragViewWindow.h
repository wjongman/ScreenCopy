// View.h : interface of the CDragViewWindow class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "DragSource.h"

#include <gdiplus.h>
#pragma comment(lib, "Gdiplus.lib")

class CDragViewWindow : public CWindowImpl<CDragViewWindow, CWindow,
    CWinTraits<WS_BORDER | WS_SYSMENU | WS_THICKFRAME, WS_EX_TOOLWINDOW> >
{
    ULONG_PTR m_gdiplusToken;
    bool m_bDragging = false;
    std::wstring m_dragFilePath;

public:
    DECLARE_WND_CLASS(NULL)

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        pMsg;
        return FALSE;
    }

    BEGIN_MSG_MAP(CDragViewWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    END_MSG_MAP()

    //-------------------------------------------------------------------------
    void SetDragFilePath(std::wstring const& filePath)
    {
        m_dragFilePath = filePath;
    }

    //-------------------------------------------------------------------------
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        CRect rcWindow = { 0, 0, 64, 64 };
        SetWindowPos(HWND_TOP, &rcWindow, SW_HIDE);
        CenterWindow();
        SetWindowText(L"drag");

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
            IDropSource* pDropSource = new CDropSource(); // Will delete itself when refcount becomes 0
            if (pDropSource)
            {
                DWORD dwEffect;
                DoDragDrop(spDataObject, pDropSource, DROPEFFECT_COPY | DROPEFFECT_LINK, &dwEffect);
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
        Gdiplus::Bitmap bitmap(m_dragFilePath.c_str());

        int cx = bitmap.GetWidth();
        int cy = bitmap.GetHeight();


        CDC memDC = CreateCompatibleDC(dc);
        HBITMAP hBmp;
        bitmap.GetHBITMAP(RGB(0, 0, 0), &hBmp);
        CBitmapHandle bmpOld = memDC.SelectBitmap(hBmp);

        CRect rcPage{ 0, 0, 64, 64 };
        // calc scaling factor, so that image is not too small
        // (based on the width only, max 3/4 width)
        int nScale = ::MulDiv(rcPage.right, 3, 4) / cx;
        if (nScale == 0) // too big already
            nScale = 1;
        // calc margins to center bitmap
        int xOff = (rcPage.right - nScale * cx) / 2;
        if (xOff < 0)
            xOff = 0;
        int yOff = (rcPage.bottom - nScale * cy) / 2;
        if (yOff < 0)
            yOff = 0;
        // ensure that preview doesn't go outside of the page
        int cxBlt = nScale * cx;
        if (xOff + cxBlt > rcPage.right)
            cxBlt = rcPage.right - xOff;
        int cyBlt = nScale * cy;
        if (yOff + cyBlt > rcPage.bottom)
            cyBlt = rcPage.bottom - yOff;

        // paint bitmap image
        ::StretchBlt(dc, xOff, yOff, cxBlt, cyBlt, memDC, 0, 0, cx, cy, SRCCOPY);

        memDC.SelectBitmap(bmpOld);
        return 0;
    }
    

};
