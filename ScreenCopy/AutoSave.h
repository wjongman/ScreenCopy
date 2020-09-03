#pragma once
//---------------------------------------------------------------------------
#include "Settings.h"
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

// Utility functions
// ---------------------------------------------------------------------------
std::wstring GetDesktopPath()
{
    wchar_t path[MAX_PATH];
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, path);
    return std::wstring(path);
}

// -------------------------------------------------------------------------
// Return true if file 'filePath' exists
bool FileExists(CString const& FilePath)
{
    DWORD attrib = ::GetFileAttributes(FilePath);
    return (attrib != INVALID_FILE_ATTRIBUTES);
}

//-------------------------------------------------------------------------
//Return true if 'path' exists and is a directory
bool DirectoryExists(CString const& path)
{
    DWORD attrib = ::GetFileAttributes(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
}

enum OverwriteAction {oaPrompt=1, oaReplace, oaRename };

//---------------------------------------------------------------------------
class AutoSaver
{
    CString m_directory;
    CString m_prefix;
    int m_digits;
    int m_nextValue;
    int m_imageType;
    int m_existAction;

public:
    AutoSaver()
    {
        LoadOptions();
    }

    //---------------------------------------------------------------------------
    ~AutoSaver()
    {
        SaveOptions();
    }

    //---------------------------------------------------------------------------
    void SaveImage(HBITMAP hBmp)
    {
        SaveBitmap(hBmp);
        DeleteObject(hBmp);
    }

private:
    //---------------------------------------------------------------------------
    void __fastcall LoadOptions()
    {
        CSettings settings;

        m_directory = settings.GetString(L"autosave", L"directory", GetDesktopPath()).c_str();
        m_prefix = settings.GetString(L"autosave", L"filename", L"Snapshot").c_str();
        m_digits = settings.GetInt(L"autosave", L"digits", 2);
        m_nextValue = settings.GetInt(L"autosave", L"nextvalue", 1);
        m_imageType = settings.GetInt(L"autosave", L"imagetype", 0);
        m_existAction = settings.GetInt(L"autosave", L"existaction", 0);
    }

    //---------------------------------------------------------------------------
    void __fastcall SaveOptions()
    {
        CSettings settings;

        settings.SetString(L"autosave", L"directory", (LPCWSTR)m_directory);
        settings.SetString(L"autosave", L"prefix", (LPCWSTR)m_prefix);
        settings.SetInt(L"autosave", L"digits", m_digits);
        settings.SetInt(L"autosave", L"nextvalue", m_nextValue);
        settings.SetInt(L"autosave", L"imagetype", m_imageType);
        settings.SetInt(L"autosave", L"existaction", m_existAction);
    }

    //---------------------------------------------------------------------------
    CString GetNextPathName()
    {
        return m_directory + L"\\" + m_prefix + GetSequenceString() + GetExtension(m_imageType);
    }

    //---------------------------------------------------------------------------
    CString GetSequenceString()
    {
        // Todo: make sure NextValue has no more then Digits digits
        CString seqNum = "";

        if (m_digits != 0)
            seqNum.Format(L"%0*d", m_digits, m_nextValue);

        return seqNum;
    }

    //---------------------------------------------------------------------------
    CString GetExtension(int type)
    {
        wchar_t* extensions[] = { L".jpg", L".png", L".bmp", L".gif" };
        if (type >= 0 && type < 4)
        {
            return extensions[type];
        }
        return L"";
    }

    //---------------------------------------------------------------------------
    CString GetMimeType(int type)
    {
        wchar_t* mimeTypes[] = { L"image/jpeg", L"image/png", L"image/bmp", L"image/gif" };
        if (type >= 0 && type < 4)
        {
            return mimeTypes[type];
        }
        return L"";
    }

    //---------------------------------------------------------------------------
    void SaveBitmap(HBITMAP hBitmap)
    {
        // First check if the target directory exists
        if (!DirectoryExists(m_directory))
        {
            // TODO: offer to change or create directory here..
            // TODO: stuff all static strings in a resource file
            CString message = L"Your autosave settings refer to a directory that doesn't exist:\n" + m_directory + L"\n";
            MessageBox(NULL, message, L"Auto save", MB_OK);
            return;
        }

        CString SavePath = GetNextPathName();
        if (FileExists(SavePath))
        {
            switch (m_existAction)
            {
            case oaPrompt: // Ask user
            {
                CString message = GetNextPathName() + L":\n\nOverwrite existing file?";
                if (::MessageBox(NULL, message, L"Auto save", MB_OKCANCEL) == IDCANCEL)
                {
                    // Canceled, do nothing
                    return;
                }
            }
            break;

            case oaReplace: // Overwrite
                break;

            case oaRename: // Auto rename
                while (FileExists(GetNextPathName()))
                {
                    // Find first available filename
                    // (might be slow on huge directories..)
                    m_nextValue++;
                }
                break;
            }
        }

        DoSave(hBitmap, GetNextPathName(), GetMimeType(m_imageType));
        m_nextValue++;
    }

    //-------------------------------------------------------------------------
    // Save a bitmap using GDI+
    void DoSave(HBITMAP hBitmap, CString const& filename, CString const& mimetype)
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        {
            // Before we call Image::Save, we must initialize an
            // EncoderParameters object. The EncoderParameters object
            // has an array of EncoderParameter objects. In this
            // case, there is only one EncoderParameter object in the array.
            // The one EncoderParameter object has an array of values.
            // In this case, there is only one value (of type ULONG)
            // in the array. This value can vary from 0 to 100.
            Gdiplus::EncoderParameters encoderParameters;
            encoderParameters.Count = 1;
            encoderParameters.Parameter[0].Guid = Gdiplus::EncoderQuality;
            encoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
            encoderParameters.Parameter[0].NumberOfValues = 1;
            // Save the image with quality level between
            // 0 (best compression) and 100 (best quality).
            // It seems only JPEG images honor these parameters..
            ULONG quality = 90;
            encoderParameters.Parameter[0].Value = &quality;

            CLSID clsid;
            GetEncoderClsid(mimetype, &clsid);

            Gdiplus::Bitmap bitmap(hBitmap, NULL);
            bitmap.Save(filename, &clsid, &encoderParameters);
        }
        Gdiplus::GdiplusShutdown(gdiplusToken);
    }

    //-------------------------------------------------------------------------
    // Helper function
    // Receives the MIME type of an encoder and returns the
    // class identifier (CLSID) of that encoder.
    // See: https://docs.microsoft.com/en-us/windows/desktop/gdiplus/-gdiplus-retrieving-the-class-identifier-for-an-encoder-use
    int GetEncoderClsid(const WCHAR* mimetype, CLSID* pClsid)

    {
        using namespace Gdiplus;
        UINT num = 0;          // number of image encoders
        UINT size = 0;         // size of the image encoder array in bytes

        GetImageEncodersSize(&num, &size);
        if (size == 0)
            return -1;  // No encoders at all?

        ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
        if (pImageCodecInfo == NULL)
            return -1;  // Out of memory?

        GetImageEncoders(num, size, pImageCodecInfo);

        for (UINT i = 0; i < num; ++i)
        {
            if (wcscmp(pImageCodecInfo[i].MimeType, mimetype) == 0)
            {
                *pClsid = pImageCodecInfo[i].Clsid;
                free(pImageCodecInfo);
                return i;  // Found it
            }
        }

        free(pImageCodecInfo);
        return -1; // No such MIME type
    }

};

