#pragma once

//----------------------------------------------------------------------------
// Dragging a shell object, part 1: Getting the IDataObject 
// Raymond Chen - The Old New Thing
// https://devblogs.microsoft.com/oldnewthing/20041206-00/?p=37133
//
#include <oleidl.h>
#include <shtypes.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <atlcomcli.h>

class CDragSource : public IDropSource
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IDropSource ***
    STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHODIMP GiveFeedback(DWORD dwEffect);

    CDragSource()
        : m_cRef(1)
    {
    }

private:
    ULONG m_cRef;
};

HRESULT CDragSource::QueryInterface(REFIID riid, void** ppv)
{
    IUnknown* punk = NULL;
    if (riid == IID_IUnknown)
    {
        punk = static_cast<IUnknown*>(this);
    }
    else if (riid == IID_IDropSource)
    {
        punk = static_cast<IDropSource*>(this);
    }

    *ppv = punk;
    if (punk)
    {
        punk->AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

ULONG CDragSource::AddRef() { return ++m_cRef; }

#pragma warning(push)
#pragma warning(disable : 4146) // unary minus operator applied to unsigned type, result still unsigned
ULONG CDragSource::Release()
{
    ULONG cRef = -m_cRef;
    if (cRef == 0)
        delete this;
    return cRef;
}
#pragma warning(pop)

HRESULT CDragSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    if (fEscapePressed)
        return DRAGDROP_S_CANCEL;

    // [Update: missing paren repaired, 7 Dec]
    if (!(grfKeyState & (MK_LBUTTON | MK_RBUTTON)))
        return DRAGDROP_S_DROP;

    return S_OK;
}

HRESULT CDragSource::GiveFeedback(DWORD dwEffect) 
{ 
    return DRAGDROP_S_USEDEFAULTCURSORS; 
}

HRESULT GetUIObjectOfFile(HWND hwnd, LPCWSTR pszPath, REFIID riid, void** ppv)
// https://blogs.msdn.microsoft.com/oldnewthing/20040920-00/?p=37823
{
    *ppv = NULL;
    HRESULT hr;
    LPITEMIDLIST pidl;
    SFGAOF sfgao;
    if (SUCCEEDED(hr = SHParseDisplayName(pszPath, NULL, &pidl, 0, &sfgao)))
    {
        CComPtr<IShellFolder> psf;
        LPCITEMIDLIST pidlChild;
        if (SUCCEEDED(hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&psf, &pidlChild)))
        {
            hr = psf->GetUIObjectOf(hwnd, 1, &pidlChild, riid, NULL, ppv);
        }
        CoTaskMemFree(pidl);
    }
    return hr;
}
