#pragma once
#include <atlctrls.h>
#include <string>
#include <strsafe.h>
#include <vector>

/////////////////////////////////////////////////////////////////////////////
struct GrabberPreset
{
    std::wstring description;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    //-----------------------------------------------------------------------
    GrabberPreset(std::wstring const& name, int x, int y, int w, int h)
        : description(name)
        , x(x)
        , y(y)
        , w(w)
        , h(h)
    {
    }

    //-----------------------------------------------------------------------
    GrabberPreset(std::wstring const& commatext) { SetCommaText(commatext); }

    //-----------------------------------------------------------------------
    bool SetCommaText(std::wstring const& commaText)
    {
        bool success = false;
        auto presets = SplitAt(commaText, L',');
        // FIXME: this assumes that "description"  doesn't contain commas..
        if (presets.size() == 5)
        {
            description = presets[0];
            // TODO: add decent error checking
            x = std::stoi(presets[1]);
            y = std::stoi(presets[2]);
            w = std::stoi(presets[3]);
            h = std::stoi(presets[4]);
            return true;
        }
        return false;
    }

    //-----------------------------------------------------------------------
    std::wstring GetCommaText()
    {
        // std::wstring descr = L"\"" + description + L"\"";
        wchar_t commaStr[100] = {};
        ::StringCchPrintfW(commaStr, 100, L"%s,%d,%d,%d,%d", description.c_str(), x, y, w, h);
        return commaStr;
    }
    //-------------------------------------------------------------------------
    std::vector<std::wstring> SplitAt(std::wstring const& input, const wchar_t delimiter)
    // Return vector of parts in 'input' separated by 'delimiter'
    {
        std::vector<std::wstring> parts;
        int startpos = 0;
        int delimpos = 0;
        while (delimpos != std::string::npos)
        {
            delimpos = input.find_first_of(delimiter, startpos);
            parts.push_back(input.substr(startpos, delimpos - startpos));
            startpos = delimpos + 1;
        }
        return parts;
    }
};

/////////////////////////////////////////////////////////////////////////////
class CPresetsListCtrl : public CListViewCtrl
{
    std::vector<GrabberPreset> m_presetList;

public:
    BEGIN_MSG_MAP(CPresetsListCtrl)
    CHAIN_MSG_MAP(CPresetsListCtrl)
    END_MSG_MAP()

    //-----------------------------------------------------------------------
    void Populate(std::vector<GrabberPreset> const& presetList)
    {
        // Tweak the control styles so we don't have to remember to set them
        // in the dialog template.
        DWORD dwRemoveStyles = LVS_TYPEMASK | LVS_SORTASCENDING | LVS_SORTDESCENDING
            | LVS_OWNERDRAWFIXED;
        DWORD dwNewStyles = LVS_REPORT | LVS_SINGLESEL;
        ModifyStyle(dwRemoveStyles, dwNewStyles);
        ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        // Empty the control, and if there are no columns, insert columns.
        DeleteAllItems();
        LVCOLUMN rCol = { LVCF_WIDTH };
        if (!GetColumn(0, &rCol))
        {
            InsertColumn(0, L"Name", LVCFMT_LEFT, 135);
            InsertColumn(1, L"X", LVCFMT_LEFT, 40);
            InsertColumn(2, L"Y", LVCFMT_LEFT, 40);
            InsertColumn(3, L"W", LVCFMT_LEFT, 40);
            InsertColumn(4, L"H", LVCFMT_LEFT, 40);
        }

        // Initialize LVITEM members that are common to all items.
        LVITEM lvi = {};
//         lvi.iItem = MAXLONG;
        lvi.stateMask = 0;
        lvi.iSubItem = 0;
        lvi.state = 0;
        for (auto &preset : presetList)
        {
            int iItem = InsertItem(&lvi);
            SetItemText(iItem, 0, preset.description.c_str());
            SetItemText(iItem, 1, std::to_wstring(preset.x).c_str());
            SetItemText(iItem, 2, std::to_wstring(preset.y).c_str());
            SetItemText(iItem, 3, std::to_wstring(preset.w).c_str());
            SetItemText(iItem, 4, std::to_wstring(preset.h).c_str());
        }
    }

};