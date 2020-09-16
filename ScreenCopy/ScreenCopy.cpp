// ScreenCopy.cpp : main source file for ScreenCopy.exe
//

#include "stdafx.h"

#include <atlctrls.h>
#include <atldlgs.h>
#include <atlframe.h>

#include "resource.h"

#include "DragViewWindow.h"
#include "ScreenCopyWindow.h"
#include "about.h"

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
    // If an instance is already running,
    // center it, activate it and bail out.
    HWND hwndOther = ::FindWindow(L"ScreenCopyWindowClass", 0);
    if (hwndOther)
    {
        CWindow(hwndOther).CenterWindow();
        ::ShowWindow(hwndOther, SW_SHOW);
        ::SetForegroundWindow(hwndOther);
        ::BringWindowToTop(hwndOther);
        return 0;
    }

    CMessageLoop theLoop;
    _Module.AddMessageLoop(&theLoop);

    // CScreenWindow wndMain;
    CDragViewWindow wndMain;

    if (wndMain.Create(0) == NULL)
    {
        ATLTRACE(_T("Main window creation failed!\n"));
        return 0;
    }

    //   wndMain.ShowWindow(nCmdShow);

    int nRet = theLoop.Run();

    _Module.RemoveMessageLoop();
    return nRet;
}

int WINAPI _tWinMain(
    HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
    HRESULT hRes = ::OleInitialize(NULL);
    ATLASSERT(SUCCEEDED(hRes));

    // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
    ::DefWindowProc(NULL, 0, 0, 0L);

    AtlInitCommonControls(ICC_BAR_CLASSES); // add flags to support other controls

    hRes = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hRes));

    int nRet = Run(lpstrCmdLine, nCmdShow);

    _Module.Term();
    ::OleUninitialize();

    return nRet;
}
