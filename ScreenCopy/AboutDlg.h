// aboutdlg.h : interface of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include <time.h>
#include "about.h"

///////////////////////////////////////////////////////////////////////////////
class CAboutDlg : public CDialogImpl<CAboutDlg>
{
public:
    enum
    {
        IDD = IDD_ABOUTBOX
    };

    BEGIN_MSG_MAP(CAboutDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    NOTIFY_HANDLER(IDC_SYSLINK, NM_CLICK, OnNMClickSyslink)
    END_MSG_MAP()

    //-------------------------------------------------------------------------
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        AtlInitCommonControls(ICC_LINK_CLASS);
        CenterWindow(GetParent());

        std::string sBuildDate = GetBuildDate();

        std::wstring wsBuildDate(sBuildDate.begin(), sBuildDate.end());
        SetDlgItemText(IDC_VERSIONSTAMP, wsBuildDate.c_str());
        CString appName;
        appName.LoadString(IDS_APPNAME);
        SetDlgItemText(IDC_APPNAME, appName);

        return TRUE;
    }

    //-------------------------------------------------------------------------
    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        EndDialog(wID);
        return 0;
    }

    //-------------------------------------------------------------------------
    LRESULT OnNMClickSyslink(int idCtrl, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
    {
        CString sUrl;
        sUrl.LoadString(IDS_URL_HOMEPAGE);

        // Open URL in default web-browser
        ::ShellExecute(0, _T("open"), sUrl, NULL, NULL, SW_SHOWNORMAL);

        EndDialog(idCtrl);
        return 0;
    }

    //-------------------------------------------------------------------------
    // Convert string from __DATE__ and __TIME__ into a formatted date string
    // http://stackoverflow.com/questions/1765014/convert-string-from-date-into-a-time-t
    //
    std::string GetBuildDate()
    {
        char s_month[4];
        struct tm t = {0};
        static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

        int month, day, year;
        sscanf_s(g_szBuildDate, "%s %d %d", s_month, 4, &day, &year);

        month = (strstr(month_names, s_month) - month_names) / 3;

        t.tm_mon = month;
        t.tm_mday = day;
        t.tm_year = year - 1900;
        t.tm_isdst = -1;

        int hour, min, sec;
        sscanf_s(g_szBuildTime, "%d:%d:%d", &hour, &min, &sec);
        t.tm_hour = hour;
        t.tm_min = min;
        t.tm_sec = sec;

        char szBuilddate[128];
        // Format date like: 2013-07-30   12:23:34
        strftime(szBuilddate, 128, "Version of  %Y-%m-%d   %H:%M:%S", &t);

        return std::string(szBuilddate);
    }

};
