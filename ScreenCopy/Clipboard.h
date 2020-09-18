#pragma once
#include "stdafx.h"

class Clipboard
{
public:
    //-------------------------------------------------------------------------
    static void Write(CString sPuzzle)
    {
        if (::OpenClipboard(NULL))
        {
            EmptyClipboard();

            int cch = sPuzzle.GetLength();
            // Allocate a global memory object for the text. 

            HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (cch + 1) * sizeof(TCHAR));
            if (hglbCopy)
            {
                // Lock the handle and copy the text to the buffer. 
                LPTSTR lptstrCopy = reinterpret_cast<TCHAR*>(GlobalLock(hglbCopy));
                if (lptstrCopy)
                {
                    memcpy(lptstrCopy, sPuzzle, cch * sizeof(TCHAR));
                    lptstrCopy[cch] = (TCHAR)0;    // null character 
                }
                GlobalUnlock(hglbCopy);

                // Place the handle on the clipboard. 
                SetClipboardData(CF_UNICODETEXT, hglbCopy);

                CloseClipboard();
            }

        }
    }

    //-------------------------------------------------------------------------
    static CString Read()
    {
        CString sPuzzle;

        if (::IsClipboardFormatAvailable(CF_TEXT) && ::OpenClipboard(NULL))
        {
            HANDLE hClipData = ::GetClipboardData(CF_UNICODETEXT);
            if (hClipData)
            {
                TCHAR* pClipText = reinterpret_cast<TCHAR*>(GlobalLock(hClipData));
                if (pClipText != NULL)
                {
                    sPuzzle = pClipText;
                    GlobalUnlock(hClipData);
                }
            }
            ::CloseClipboard();
        }
        return sPuzzle;
    }
};

