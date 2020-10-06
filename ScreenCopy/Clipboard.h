#pragma once
#include "stdafx.h"

class Clipboard
{
public:
    //-------------------------------------------------------------------------
    static void Write(std::wstring text)
    {
        if (::OpenClipboard(NULL))
        {
            // Allocate a global memory object for the text. 
            int bufSize = text.size();
            HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, (bufSize + 1) * sizeof(wchar_t));
            if (hGlobal)
            {
                // Lock the handle and copy the text to the buffer. 
                wchar_t* pTextCopy = reinterpret_cast<wchar_t*>(::GlobalLock(hGlobal));
                if (pTextCopy)
                {
                    memcpy(pTextCopy, text.c_str(), bufSize * sizeof(wchar_t));
                    pTextCopy[bufSize] = L'\0';
                }
                ::GlobalUnlock(hGlobal);

                ::EmptyClipboard();
                ::SetClipboardData(CF_UNICODETEXT, hGlobal);
                ::CloseClipboard();
            }
        }
    }

    //-------------------------------------------------------------------------
    static std::wstring Read()
    {
        std::wstring result;
        if (::IsClipboardFormatAvailable(CF_TEXT) && ::OpenClipboard(NULL))
        {
            HANDLE hClipData = ::GetClipboardData(CF_UNICODETEXT);
            if (hClipData)
            {
                wchar_t* pClipText = reinterpret_cast<wchar_t*>(::GlobalLock(hClipData));
                if (pClipText != NULL)
                {
                    result = pClipText;
                    ::GlobalUnlock(hClipData);
                }
            }
            ::CloseClipboard();
        }
        return result;
    }
};

