// ScreenCopy.cpp : main source file for ScreenCopy.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>

#include "resource.h"

#include "about.h"
#include "ScreenCopyWindow.h"

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

	CScreenWindow wndMain;

	if(wndMain.Create(0) == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

// 	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
