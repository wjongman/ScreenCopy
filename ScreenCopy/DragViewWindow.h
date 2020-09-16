// View.h : interface of the CDragViewWindow class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include <atlscrl.h>
// #include "DragDropSource.h"
#include "DropSource.h"

class CDragViewWindow : public CWindowImpl<CDragViewWindow, CWindow,
    CWinTraits<WS_BORDER | WS_SYSMENU | WS_THICKFRAME/*, WS_EX_TOOLWINDOW*/> >
{
    HDC m_memDC = NULL; // Memory DC for buffered drawing of our bitmap
    bool m_bDragging = false;
    CPoint m_ptLastMouse = { 0 };

public:
    DECLARE_WND_CLASS(NULL)

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        pMsg;
        return FALSE;
    }

    void DoPaint(CDCHandle dc)
    {
        //TODO: Add your drawing code here
    }

    BEGIN_MSG_MAP(CDragViewWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//  LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//  LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//  LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

    //-------------------------------------------------------------------------
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        // At this stage we have a valid HWND so now we can get
        // our device context and use it to create a memory DC
        HDC dc = ::GetDC(m_hWnd);
        m_memDC = ::CreateCompatibleDC(dc);
        ::ReleaseDC(m_hWnd, dc);

        CRect rcWindow = { 0, 0, 64, 64 };
        SetWindowPos(HWND_TOP, &rcWindow, SW_HIDE);
        CenterWindow();
        ShowWindow(SW_SHOW);
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        ::DeleteDC(m_memDC);
        // Avoid losing foreground focus
        bHandled = FALSE;
        return 1;
    }

     //-------------------------------------------------------------------------
    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CPaintDC dc(m_hWnd);

        // Draw buffered bitmap
//         CRect rcROI = GetROI();
//         ::BitBlt(dc, 0, 0, rcROI.Width(), rcROI.Height(), m_memDC, 0, 0, SRCCOPY);

        return 0;
    }

     //-------------------------------------------------------------------------
    LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        m_bDragging = true;
        m_ptLastMouse = CPoint(LOWORD(lParam), HIWORD(lParam));
//         SetCapture();
        return 0;
    }
    
     //-------------------------------------------------------------------------
    LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
    {
        if (m_bDragging)
        {
            CPoint ptMouse(LOWORD(lParam), HIWORD(lParam));
            int deltaX = ptMouse.x - m_ptLastMouse.x;
            int deltaY = ptMouse.y - m_ptLastMouse.y;

//             // Adjust ROI
//             CRect rcROI = GetROI();
//             rcROI.OffsetRect(deltaX, deltaY);
//             SetROI(rcROI);

            m_ptLastMouse = ptMouse;
            SetCapture();
            BeginDrag();
            ReleaseCapture();
            m_bDragging = false;
        }
        return 0;
    }

    LRESULT BeginDrag()
    {
        IDataObject* pdto;
        if (SUCCEEDED(GetUIObjectOfFile(m_hWnd, L"C:\\etc\\_Test\\thumbnails\\test2.png", IID_IDataObject, (void**)&pdto)))
        {
            IDropSource* pds = new CDropSource();
            if (pds)
            {
                DWORD dwEffect;
                DoDragDrop(pdto, pds, DROPEFFECT_COPY | DROPEFFECT_LINK, &dwEffect);
                pds->Release();
            }
            pdto->Release();
        }
        return 0;
    }

//     LRESULT BeginDrag2()
//     {
//         //         NMLISTVIEW* pnmlv = (NMLISTVIEW*)phdr;
//         //        std::vector<CDraggedFileInfo> vec;
// 
//         std::wstring testFilePath{ L"C:/etc/_Test/thumbnails/test2.png" };
//         std::vector<std::wstring> vec = { L"C:/etc/_Test/thumbnails/test2.png" };
// 
//         CComObjectStack<CDragDropSource> dropsrc;
//         DWORD dwEffect = 0;
//         HRESULT hr;
//         CComPtr<IDragSourceHelper> pdsh;
// 
//         // Init the drag/drop data object.
//         if (!dropsrc.Init(testFilePath))
//         {
//             ATLTRACE("Error: Couldn't init drop source object\n");
//             return 0; // do nothing
//         }
// 
//         // On 2K+, init a drag source helper object that will do the fancy drag
//         // image when the user drags into Explorer (or another target that supports
//         // the drag/drop helper).
//         hr = pdsh.CoCreateInstance(CLSID_DragDropHelper);
// 
//         if (SUCCEEDED(hr))
//         {
//             CComQIPtr<IDataObject> pdo;
//             CPoint ptStart{ 32, 32 };
//             if (pdo = dropsrc.GetUnknown())
//                 pdsh->InitializeFromWindow(m_hWnd, &ptStart, pdo);
//         }
// 
//         // Start the drag/drop!
//         hr = dropsrc.DoDragDrop(DROPEFFECT_COPY, &dwEffect);
// 
//         if (FAILED(hr))
//             ATLTRACE("Error: DoDragDrop() failed, error: 0x%08X\n", hr);
// 
//         return 0;
//     }

};
