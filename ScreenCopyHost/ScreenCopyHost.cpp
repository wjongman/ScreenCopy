#include <stdio.h>
#include <windows.h>

int main(int argc, char* argv[])
{
//     if (argc > 1)
//     {
//     }
    HWND hwndScreenCopy = ::FindWindow(L"ScreenCopyWindowClass", 0);
    if (hwndScreenCopy)
    {
        printf("Found ScreenCopy (0x%08X)\nClosing window\nDone\n", (UINT)hwndScreenCopy);
        ::PostMessage(hwndScreenCopy, WM_CLOSE, 0, 0);
    }
    else
    {
        puts("ScreenCopyWindowClass not found!\nDone.");
//         // Start ScreenCopy
//         std::wstring screenCopyPath = GetScreenCopyPath();
//         if (!screenCopyPath.empty())
//         {
//             ShellExecute(0, L"open", L"ScreenCopy.exe", 0, 0, SW_SHOW);
//         }
    }
}
