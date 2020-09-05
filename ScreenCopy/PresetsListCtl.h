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
    GrabberPreset() = default;
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
        int maxSize = description.size() + 4 * 9;
        std::vector<wchar_t> commaStr(maxSize);
        ::StringCchPrintfW(commaStr.data(), maxSize, L"%s,%d,%d,%d,%d", description.c_str(), x, y, w, h);
        return commaStr.data();
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

using PresetsList = std::vector<GrabberPreset>;

/////////////////////////////////////////////////////////////////////////////
class CPresetsListCtrl : public CListViewCtrl
{
//     BEGIN_MSG_MAP(CPresetsListCtrl)
//     CHAIN_MSG_MAP(CPresetsListCtrl)
//     END_MSG_MAP()

    const int m_cellWidth = 45;

public:
    //-----------------------------------------------------------------------
    void Populate(PresetsList const& presets)
    {
        // Set the control styles here so we don't have
        // to remember to set them in the dialog template.
        DWORD dwRemoveStyles = LVS_TYPEMASK /*| LVS_SORTASCENDING | LVS_SORTDESCENDING*/
            | LVS_OWNERDRAWFIXED;
        DWORD dwNewStyles = LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER;
        ModifyStyle(dwRemoveStyles, dwNewStyles);
        ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_FLATSB);

        // Empty the control, and if there are no columns, insert columns.
        DeleteAllItems();
        LVCOLUMN rCol = { LVCF_WIDTH };
        if (!GetColumn(0, &rCol))
        {
            InsertColumn(0, L"Name", LVCFMT_LEFT, 135);
            InsertColumn(1, L"X", LVCFMT_RIGHT, m_cellWidth);
            InsertColumn(2, L"Y", LVCFMT_RIGHT, m_cellWidth);
            InsertColumn(3, L"W", LVCFMT_RIGHT, m_cellWidth);
            InsertColumn(4, L"H", LVCFMT_RIGHT, m_cellWidth);
        }

        // Add the items
        int index = 0;
        for (auto &preset : presets)
        {
            SetRow(index, preset);
            ++index;
        }
        AdjustColumns();
    }

    //-------------------------------------------------------------------------
    void SetRow(int index, GrabberPreset preset)
    {
        AddItem(index, 0, preset.description.c_str());
        AddItem(index, 1, std::to_wstring(preset.x).c_str());
        AddItem(index, 2, std::to_wstring(preset.y).c_str());
        AddItem(index, 3, std::to_wstring(preset.w).c_str());
        AddItem(index, 4, std::to_wstring(preset.h).c_str());
    }

    //-------------------------------------------------------------------------
    void AdjustColumns()
    {
        // To avoid showing horizontal scrollbar we adjust width of column 0
        CRect rcClient;
        GetClientRect(&rcClient);
        int descrWidth = rcClient.Width() - 4 * m_cellWidth;
        ListView_SetColumnWidth(m_hWnd, 0, descrWidth);
    }
};