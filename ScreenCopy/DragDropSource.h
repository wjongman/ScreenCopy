// DragDropSource.h: interface for the CDragDropSource class.
//////////////////////////////////////////////////////////////////////
#pragma once

// Drag/drop source implementation, IDataObject impl is taken from a
// technical article in MSDN:
// http://msdn.microsoft.com/library/en-us/dnwui/html/ddhelp_pt2.asp

// Abstracted from:
// WTL for MFC Programmers, Part X - Implementing a Drag and Drop Source
// https://www.codeproject.com/articles/14482/webcontrols/
//
// Also see:
// The Shell Drag/Drop Helper Object Part 1: IDropTargetHelper
// https://msdn.microsoft.com/en-us/library/ms997500.aspx
// The Shell Drag/Drop Helper Object Part 2: IDropSourceHelper
// https://msdn.microsoft.com/en-us/library/ms997502.aspx
//

#include <vector>
#include <atlmisc.h>

/////////////////////////////////////////////////////////////////////////////////
//struct CDraggedFileInfo
//{
//    // Data set at the beginning of a drag/drop:
//    CString sFilename;      // name of the file as stored in the CAB
//    CString sTempFilePath;  // path to the file we extract from the CAB
//    int nListIdx;           // index of this item in the list ctrl
//
//    // Data set while extracting files:
//    bool bPartialFile;      // true if this file is continued in another cab
//    CString sCabName;       // name of the CAB file
//    bool bCabMissing;       // true if the file is partially in this cab and
//    // the CAB it's continued in isn't found, meaning
//    // the file can't be extracted
//
//    CDraggedFileInfo(const CString& s, int n) :
//        sFilename(s), nListIdx(n), bPartialFile(false), bCabMissing(false)
//    { }
//};

//////////////////////////////////////////////////////////////////////////////
class CDragDropSource :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDragDropSource>,
    public IDataObject,
    public IDropSource
{
private:
    bool  m_bInitialized;
    DWORD m_dwLastEffect;

    std::vector<std::wstring> m_vecDraggedFiles;
//    CString m_sCabFilePath;
//    CString m_sFileBeingExtracted;
//    std::vector<CDraggedFileInfo>::iterator m_it;   // iter on file currently being extracted
//    bool m_bAbortingDueToMissingCab;

    // List of FORMATETCs for which we have data, used in EnumFormatEtc
    std::vector<FORMATETC> m_vecSupportedFormats;

    typedef struct
    {
        FORMATETC fe;
        STGMEDIUM stgm;
    } DATAENTRY, *LPDATAENTRY;  // Each active FORMATETC gets one of these

    LPDATAENTRY m_rgde;  // Array of active DATAENTRY entries
    int m_cde;           // Size of m_rgde

public:
    //-------------------------------------------------------------------------
    CDragDropSource() :
        m_bInitialized(false), 
        m_dwLastEffect(DROPEFFECT_NONE), 
        m_cde(0), 
        m_rgde(NULL)
    {
    }

    //-------------------------------------------------------------------------
    ~CDragDropSource()
    {
        for (int i = 0; i < m_cde; i++)
        {
            CoTaskMemFree(m_rgde[i].fe.ptd);
            ReleaseStgMedium(&m_rgde[i].stgm);
        }

        CoTaskMemFree(m_rgde);
    }

    //-------------------------------------------------------------------------
    BEGIN_COM_MAP(CDragDropSource)
        COM_INTERFACE_ENTRY(IDataObject)
        COM_INTERFACE_ENTRY(IDropSource)
    END_COM_MAP()

    //-------------------------------------------------------------------------
    // Operations
    bool Init(std::wstring const& fileName)
    //bool Init(LPCTSTR szCabFilePath, std::vector<CDraggedFileInfo>& vec)
    {
        FORMATETC fmtetc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM medium = { TYMED_HGLOBAL };
        size_t bytesNeeded = sizeof(DROPFILES);

        // Init member data
        m_vecDraggedFiles.push_back(fileName);

        // Calc how much space is needed to hold all the filenames
        for (auto& filePath : m_vecDraggedFiles)
        {
            bytesNeeded += sizeof(wchar_t) * (1 + filePath.size());
        }

        // One more wchar_t for the final null char
        bytesNeeded += sizeof(wchar_t);

        // Alloc a buffer to hold the DROPFILES data.
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bytesNeeded);

        if (!hGlobal)
            return false;

        DROPFILES* pDrop = (DROPFILES*)GlobalLock(hGlobal);

        if (!pDrop)
        {
            GlobalFree(hGlobal);
            return false;
        }

        pDrop->pFiles = sizeof(DROPFILES);
        pDrop->fWide = 1;

        // Copy the filenames into the buffer.
        LPTSTR pszFilename = (LPTSTR)(pDrop + 1);
        _tcscpy(pszFilename, fileName.c_str());
        pszFilename += fileName.size() + 1;
        //for (it = m_vecDraggedFiles.begin(); it != m_vecDraggedFiles.end(); it++)
        //{
        //    sTempFilePath = sTempDir + it->sFilename;
        //    int fd = _tcreat(sTempFilePath, _S_IREAD | _S_IWRITE);

        //    if (-1 != fd)
        //        _close(fd);

        //    _tcscpy(pszFilename, sTempFilePath);

        //    it->sTempFilePath = sTempFilePath;

        //    // Keep track of this temp file so we can clean up when the app exits.
        //    g_vecsTempFiles.push_back(sTempFilePath);
        //}

        GlobalUnlock(hGlobal);
        medium.hGlobal = hGlobal;

        if (FAILED(SetData(&fmtetc, &medium, TRUE)))
        {
            GlobalFree(hGlobal);
            return false;
        }

        // We're done.
        m_bInitialized = true;
        return true;
    }

    //-------------------------------------------------------------------------
    HRESULT DoDragDrop(DWORD dwOKEffects, DWORD* pdwEffect)
    {
        if (!m_bInitialized)
            return E_FAIL;

        CComQIPtr<IDropSource> pDropSrc = this;
        CComQIPtr<IDataObject> pDataObj = this;

        ATLASSERT(pDropSrc && pDataObj);

        return ::DoDragDrop(pDataObj, pDropSrc, dwOKEffects, pdwEffect);
    }

    //-------------------------------------------------------------------------
    const std::vector<std::wstring> const& GetDragResults() const
    {
        return m_vecDraggedFiles;
    }

    ///////////////////////////////////////////////////////////////////////////
    // IDataObject
    // taken from a technical article in MSDN:
    // http://msdn.microsoft.com/library/en-us/dnwui/html/ddhelp_pt2.asp
    //-------------------------------------------------------------------------
    STDMETHODIMP SetData(FORMATETC* pfe, STGMEDIUM* pstgm, BOOL fRelease)
    {
        if (!fRelease)
            return E_NOTIMPL;

        LPDATAENTRY pde;
        HRESULT hres = FindFORMATETC(pfe, &pde, TRUE);

        if (SUCCEEDED(hres))
        {
            if (pde->stgm.tymed)
            {
                ReleaseStgMedium(&pde->stgm);
                ZeroMemory(&pde->stgm, sizeof(STGMEDIUM));
            }

            if (fRelease)
            {
                pde->stgm = *pstgm;
                hres = S_OK;
            }
            else
            {
                hres = AddRefStgMedium(pstgm, &pde->stgm, TRUE);
            }

            pde->fe.tymed = pde->stgm.tymed;    /* Keep in sync */

//             /* Subtlety!  Break circular reference loop */
//             if (GetCanonicalIUnknown(pde->stgm.pUnkForRelease) ==
//                 GetCanonicalIUnknown(static_cast<IDataObject*>(this)))
//             {
//                 pde->stgm.pUnkForRelease->Release();
//                 pde->stgm.pUnkForRelease = NULL;
//             }
        }

        return hres;
    }


    //-------------------------------------------------------------------------
    STDMETHODIMP GetData(FORMATETC* pfe, STGMEDIUM* pstgm)
    {
        LPDATAENTRY pde;
        HRESULT hres;

        hres = FindFORMATETC(pfe, &pde, FALSE);

        if (SUCCEEDED(hres))
            hres = AddRefStgMedium(&pde->stgm, pstgm, FALSE);

        return hres;
    }

    //-------------------------------------------------------------------------
    // This typedef creates an IEnumFORMATETC enumerator that the drag source
    // uses in EnumFormatEtc().
    typedef CComEnumOnSTL<
        IEnumFORMATETC,           // name of enumerator interface
        &IID_IEnumFORMATETC,      // IID of enumerator interface
        FORMATETC,                // type of object to return
        _Copy<FORMATETC>,         // copy policy class
        std::vector<FORMATETC>    // type of collection holding the data
    > 
    CEnumFORMATETCImpl;

    //-------------------------------------------------------------------------
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc)
    {
        HRESULT hr;
        CComObject<CEnumFORMATETCImpl>* pImpl;

        // Create an enumerator object.
        hr = CComObject<CEnumFORMATETCImpl>::CreateInstance(&pImpl);

        if (FAILED(hr))
            return hr;

        pImpl->AddRef();

        // Fill in m_vecSupportedFormats with the formats that the caller has
        // put in this object.
        m_vecSupportedFormats.clear();

        for (int i = 0; i < m_cde; i++)
            m_vecSupportedFormats.push_back(m_rgde[i].fe);

        // Init the enumerator, passing it our vector of supported FORMATETCs.
        hr = pImpl->Init(GetUnknown(), m_vecSupportedFormats);

        if (FAILED(hr))
        {
            pImpl->Release();
            return E_UNEXPECTED;
        }

        // Return the requested interface to the caller.
        hr = pImpl->QueryInterface(ppenumFormatEtc);

        pImpl->Release();
        return hr;
    }

    //-------------------------------------------------------------------------
    STDMETHODIMP QueryGetData(FORMATETC* pfe)
    {
        LPDATAENTRY pde;

        return FindFORMATETC(pfe, &pde, FALSE);
    }
    
    //-------------------------------------------------------------------------
    STDMETHODIMP GetDataHere(FORMATETC* pformatetc, STGMEDIUM *pmedium)
    {
        return E_NOTIMPL;
    }
    
    //-------------------------------------------------------------------------
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* pformatectIn,
                                       FORMATETC* pformatetcOut)
    {
        return E_NOTIMPL;
    }
    
    //-------------------------------------------------------------------------
    STDMETHODIMP DAdvise(FORMATETC* pformatetc, DWORD advf,
                         IAdviseSink* pAdvSink, DWORD* pdwConnection)
    {
        return E_NOTIMPL;
    }
    
    //-------------------------------------------------------------------------
    STDMETHODIMP DUnadvise(DWORD dwConnection)
    {
        return E_NOTIMPL;
    }
    
    //-------------------------------------------------------------------------
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA** ppenumAdvise)
    {
        return E_NOTIMPL;
    }

    ///////////////////////////////////////////////////////////////////////////
    // IDropSource
    //-------------------------------------------------------------------------
    STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
    {
        // If ESC was pressed, cancel the drag. If the left button was released,
        // do the drop.
        if (fEscapePressed)
            return DRAGDROP_S_CANCEL;
        else if (!(grfKeyState & MK_LBUTTON))
        {
            if (DROPEFFECT_NONE == m_dwLastEffect)
                return DRAGDROP_S_CANCEL;

            // If the drop was accepted, do the extracting here, so that when we
            // return, the files are in the temp dir & ready for Explorer to copy.
            // If ExtractFilesFromCab() failed and m_bAbortingDueToMissingCab is true,
            // then return success anyway, because we don't want to fail the whole
            // shebang in the case of a missing CAB file (some files may still be
            // extractable).
            //if (ExtractFilesFromCab() || m_bAbortingDueToMissingCab)
            //    return DRAGDROP_S_DROP;
            //else
            return E_UNEXPECTED;
        }
        else
            return S_OK;
    }

    //-------------------------------------------------------------------------
    STDMETHODIMP GiveFeedback(DWORD dwEffect)
    {
        m_dwLastEffect = dwEffect;
        return DRAGDROP_S_USEDEFAULTCURSORS;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Helper functions used by IDataObject methods
    //-------------------------------------------------------------------------
    HRESULT FindFORMATETC(FORMATETC* pfe, LPDATAENTRY* ppde, BOOL fAdd)
    {
        *ppde = NULL;

        /* Comparing two DVTARGETDEVICE structures is hard, so we don't even try */
        if (pfe->ptd != NULL)
            return DV_E_DVTARGETDEVICE;

        /* See if it's in our list */
        int ide = 0;
        for (ide = 0; ide < m_cde; ide++)
        {
            if (m_rgde[ide].fe.cfFormat == pfe->cfFormat &&
                m_rgde[ide].fe.dwAspect == pfe->dwAspect &&
                m_rgde[ide].fe.lindex == pfe->lindex)
            {
                if (fAdd || (m_rgde[ide].fe.tymed & pfe->tymed))
                {
                    *ppde = &m_rgde[ide];
                    return S_OK;
                }
                else
                {
                    return DV_E_TYMED;
                }
            }
        }

        if (!fAdd)
            return DV_E_FORMATETC;

        LPDATAENTRY pdeT = (LPDATAENTRY)CoTaskMemRealloc(m_rgde,
            sizeof(DATAENTRY) * (m_cde + 1));

        if (pdeT)
        {
            m_rgde = pdeT;
            m_cde++;
            m_rgde[ide].fe = *pfe;

            ZeroMemory(&pdeT[ide].stgm, sizeof(STGMEDIUM));
            *ppde = &m_rgde[ide];

            return S_OK;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }

    //-------------------------------------------------------------------------
    HRESULT AddRefStgMedium(STGMEDIUM* pstgmIn, STGMEDIUM* pstgmOut, BOOL fCopyIn)
    {
        HRESULT hres = S_OK;
        STGMEDIUM stgmOut = *pstgmIn;

        if (pstgmIn->pUnkForRelease == NULL &&
            !(pstgmIn->tymed & (TYMED_ISTREAM | TYMED_ISTORAGE)))
        {
            if (fCopyIn)
            {
                /* Object needs to be cloned */
                if (pstgmIn->tymed == TYMED_HGLOBAL)
                {
                    stgmOut.hGlobal = GlobalClone(pstgmIn->hGlobal);

                    if (!stgmOut.hGlobal)
                        hres = E_OUTOFMEMORY;
                }
                else
                    hres = DV_E_TYMED;      /* Don't know how to clone GDI objects */
            }
            else
                stgmOut.pUnkForRelease = static_cast<IDataObject*>(this);
        }

        if (SUCCEEDED(hres))
        {
            switch (stgmOut.tymed)
            {
            case TYMED_ISTREAM:
                stgmOut.pstm->AddRef();
                break;

            case TYMED_ISTORAGE:
                stgmOut.pstg->AddRef();
                break;
            }

            if (stgmOut.pUnkForRelease)
                stgmOut.pUnkForRelease->AddRef();

            *pstgmOut = stgmOut;
        }

        return hres;
    }

    //-------------------------------------------------------------------------
    HGLOBAL GlobalClone(HGLOBAL hglobIn)
    {
        HGLOBAL hglobOut = NULL;
        LPVOID pvIn = GlobalLock(hglobIn);

        if (pvIn)
        {
            SIZE_T cb = GlobalSize(hglobIn);

            HGLOBAL hglobOut = GlobalAlloc(GMEM_FIXED, cb);

            if (hglobOut)
                CopyMemory(hglobOut, pvIn, cb);

            GlobalUnlock(hglobIn);
        }

        return hglobOut;
    }

    //-------------------------------------------------------------------------
    IUnknown* GetCanonicalIUnknown(IUnknown* punk)
    {
        IUnknown *punkCanonical;

        if (punk && SUCCEEDED(punk->QueryInterface(IID_IUnknown,
            (void**)&punkCanonical)))
        {
            punkCanonical->Release();
        }
        else
        {
            punkCanonical = punk;
        }

        return punkCanonical;
    }
};

