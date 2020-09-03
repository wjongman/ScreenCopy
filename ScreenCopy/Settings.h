#pragma once

#include <vector>
#include <string>

///////////////////////////////////////////////////////////////////////////////
// Abstract base class for persisting settings
//
// Offers a uniform interface for accessing stored settings
//
// Inherit this to implement various ways of storing data
//
class CSettingsStore
{
public:
    virtual void SetSection(std::wstring const& sectionName) = 0;
    virtual std::wstring ReadString(std::wstring const& entry, std::wstring const& defval = L"") = 0;
    virtual int ReadInt(std::wstring const& entry, int defval = 0) = 0;
    virtual void WriteString(std::wstring const& entry, std::wstring const& value) = 0;
    virtual void WriteInt(std::wstring const& entry, int value) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation of CSettingsStore that stores settings in an ini-file
//
class CIniFile : public CSettingsStore
{
    std::wstring m_filePath;
    std::wstring m_currentSection;

public:
    //-------------------------------------------------------------------------
    CIniFile(std::wstring const& iniFilePath) : m_filePath(iniFilePath)
    {
    }

    //-------------------------------------------------------------------------
    ~CIniFile()
    {
        // Flush
        //WritePrivateProfileString(NULL, NULL, NULL, m_filePath.c_str());
    }

    //-------------------------------------------------------------------------
    void SetSection(std::wstring const& sectionName) override
    {
        m_currentSection = sectionName;
    }

    //-------------------------------------------------------------------------
    std::wstring ReadString(std::wstring const& entry, std::wstring const& defval = L"") override
    {
        wchar_t szBuffer[MAX_PATH + 1] = { 0 };
        GetPrivateProfileString(m_currentSection.c_str(), entry.c_str(), defval.c_str(), szBuffer, MAX_PATH, m_filePath.c_str());
        return szBuffer;
    }

    //-------------------------------------------------------------------------
    int ReadInt(std::wstring const& entry, int defval = 0) override
    {
        return GetPrivateProfileInt(m_currentSection.c_str(), entry.c_str(), defval, m_filePath.c_str());
    }

    //-------------------------------------------------------------------------
    void WriteString(std::wstring const& entry, std::wstring const& value) override
    {
        WritePrivateProfileString(m_currentSection.c_str(), entry.c_str(), value.c_str(), m_filePath.c_str());
    }

    //-------------------------------------------------------------------------
    void WriteInt(std::wstring const& entry, int value) override
    {
        WriteString(entry, std::to_wstring(value));
    }

};

///////////////////////////////////////////////////////////////////////////////
// Implementation of CSettingsStore that uses the registry
//
class CRegFile : public CSettingsStore
{
    const std::wstring m_rootKeyPath;
    std::wstring m_currentSection;

public:
    //-------------------------------------------------------------------------
    CRegFile(std::wstring const& rootPath) : m_rootKeyPath(rootPath)
    {
    }

    //-------------------------------------------------------------------------
    void SetSection(std::wstring const& sectionName) override
    {
        if (!sectionName.empty())
        {
            m_currentSection = m_rootKeyPath + L"\\" + sectionName;
        }
        else
        {
            m_currentSection = m_rootKeyPath;
        }
    }

    //-------------------------------------------------------------------------
    std::wstring ReadString(std::wstring const& entry, std::wstring const& defval = L"") override
    {
        std::wstring resultVal = defval;

        CRegKeyEx regkey;
        if (regkey.Open(HKEY_CURRENT_USER, m_currentSection.c_str(), KEY_READ) == ERROR_SUCCESS)
        {
            wchar_t buffer[MAX_PATH];
            DWORD nbytes = MAX_PATH;
            if (regkey.QueryStringValue(entry.c_str(), buffer, &nbytes) == ERROR_SUCCESS)
            {
                resultVal = buffer;
            }
        }
        return resultVal;
    }

    //-------------------------------------------------------------------------
    int ReadInt(std::wstring const& entry, int defval = 0) override
    {
        int result = defval;

        CRegKeyEx regkey;
        if (regkey.Open(HKEY_CURRENT_USER, m_currentSection.c_str(), KEY_READ) == ERROR_SUCCESS)
        {
            DWORD value;
            if (regkey.QueryDWORDValue(entry.c_str(), value) == ERROR_SUCCESS)
            {
                result = value;
            }
        }
        return result;
    }

    //-------------------------------------------------------------------------
    void WriteString(std::wstring const& entry, std::wstring const& value) override
    {
        CRegKeyEx regkey;
        if (regkey.Create(HKEY_CURRENT_USER, m_currentSection.c_str(), REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE) == ERROR_SUCCESS)
        {
            regkey.SetStringValue(entry.c_str(), value.c_str());
        }
    }

    //-------------------------------------------------------------------------
    void WriteInt(std::wstring const& entry, int value) override
    {
        CRegKeyEx regkey;
        if (regkey.Create(HKEY_CURRENT_USER, m_currentSection.c_str(), REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE) == ERROR_SUCCESS)
        {
            regkey.SetDWORDValue(entry.c_str(), value);
        }
    }

};

///////////////////////////////////////////////////////////////////////////////
// Simple frontend for storing settings.
// The constructor of this class determines whether to use
// an ini-file or the registry.
//
class CSettings
{
    CSettingsStore* m_pStore;

    // Infer paths from name of current program
    const std::wstring m_regpath = GetRegistryPath();
    const std::wstring m_iniFilePath = GetIniFilePath();

public:

    //-------------------------------------------------------------------------
    CSettings()
    {
        // See what kind of storage we need
        if (FileExists(m_iniFilePath))
        {
            // Use inifile when found
            m_pStore = new CIniFile(m_iniFilePath);
        }
        else
        {
            // Use the registry
            m_pStore = new CRegFile(m_regpath);
        }
    }

    //-------------------------------------------------------------------------
    ~CSettings()
    {
        delete m_pStore;
    }

    //-------------------------------------------------------------------------
    void SetInt(std::wstring const& section, std::wstring const& key, int value)
    {
        m_pStore->SetSection(section);
        m_pStore->WriteInt(key, value);
    }

    //-------------------------------------------------------------------------
    void SetString(std::wstring const& section, std::wstring const& key, std::wstring const& value)
    {
        m_pStore->SetSection(section);
        m_pStore->WriteString(key, value);
    }

    //-------------------------------------------------------------------------
    int GetInt(std::wstring const& section, std::wstring const& key, int default)
    {
        m_pStore->SetSection(section);
        return m_pStore->ReadInt(key, default);
    }

    //-------------------------------------------------------------------------
    std::wstring GetString(std::wstring const& section, std::wstring const& key, std::wstring const& default)
    {
        m_pStore->SetSection(section);
        return m_pStore->ReadString(key, default);
    }

    //-------------------------------------------------------------------------
    void RestoreWindowPlacement(HWND hWnd, CRect const& rcDefault)
    {
        m_pStore->SetSection(L"main");
        CRect rcMain;
        rcMain.left = m_pStore->ReadInt(L"left", rcDefault.left);
        rcMain.top = m_pStore->ReadInt(L"top", rcDefault.top);
        rcMain.right = m_pStore->ReadInt(L"right", rcDefault.right);
        rcMain.bottom = m_pStore->ReadInt(L"bottom", rcDefault.bottom);

        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);
        wp.flags = 0;
        wp.showCmd = SW_RESTORE;
        wp.rcNormalPosition = rcMain;

        SetWindowPlacement(hWnd, &wp);
    }

    //-------------------------------------------------------------------------
    void SaveWindowPlacement(HWND hWnd)
    {
        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hWnd, &wp);
        CRect rcMain = wp.rcNormalPosition;

        SaveWindowPlacement(rcMain);
    }

    //-------------------------------------------------------------------------
    void SaveWindowPlacement(CRect const& rcMain)
    {
        m_pStore->SetSection(L"main");

        m_pStore->WriteInt(L"left", rcMain.left);
        m_pStore->WriteInt(L"top", rcMain.top);
        m_pStore->WriteInt(L"right", rcMain.right);
        m_pStore->WriteInt(L"bottom", rcMain.bottom);
    }

private:
    //-------------------------------------------------------------------------
    // Return path to ini-file for this program
    std::wstring GetIniFilePath()
    {
        // Replace .exe by .ini
        wchar_t path[MAX_PATH] = {};
        GetModuleFileName(0, path, MAX_PATH);
        PathRemoveExtension(path);
        PathAddExtension(path, L".ini");

        return std::wstring(path);
    }

    //-------------------------------------------------------------------------
    // Return path to registry key for this program
    std::wstring GetRegistryPath()
    {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileName(0, path, MAX_PATH);
        wchar_t* filename = PathFindFileName(path);
        PathRemoveExtension(filename);

        std::wstring regPath(L"Software\\toptools.org\\");
        return regPath + std::wstring(filename);
    }

    //-------------------------------------------------------------------------
    // Return path to where this program was started
    std::wstring GetProgramDirectory()
    {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileName(0, path, MAX_PATH);
        PathRemoveFileSpec(path);

        return std::wstring(path);
    }

    //-------------------------------------------------------------------------
    // Return true if file 'filePath' exists
    bool FileExists(std::wstring const& FilePath)
    {
        DWORD attrib = ::GetFileAttributes(FilePath.c_str());
        return (attrib != INVALID_FILE_ATTRIBUTES);
    }

};


