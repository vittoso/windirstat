// TypeView.cpp - Implementation of CExtensionListControl and CTypeView
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2005 Bernhard Seifert
// Copyright (C) 2004-2024 WinDirStat Team (windirstat.net)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "stdafx.h"
#include "WinDirStat.h"
#include "Item.h"
#include "MainFrame.h"
#include "DirStatDoc.h"
#include <common/CommonHelpers.h>
#include "TypeView.h"
#include "GlobalHelpers.h"
#include "Localization.h"

/////////////////////////////////////////////////////////////////////////////

CExtensionListControl::CListItem::CListItem(CExtensionListControl* list, LPCWSTR extension, const SExtensionRecord& r)
{
    m_list      = list;
    m_extension = extension;
    m_record    = r;
    m_image     = -1;
}

bool CExtensionListControl::CListItem::DrawSubitem(int subitem, CDC* pdc, CRect rc, UINT state, int* width, int* focusLeft) const
{
    if (subitem == COL_EXTENSION)
    {
        DrawLabel(m_list, GetMyImageList(), pdc, rc, state, width, focusLeft);
    }
    else if (subitem == COL_COLOR)
    {
        DrawColor(pdc, rc, state, width);
    }
    else
    {
        return false;
    }

    return true;
}

void CExtensionListControl::CListItem::DrawColor(CDC* pdc, CRect rc, UINT state, int* width) const
{
    if (width != nullptr)
    {
        *width = 40;
        return;
    }

    DrawSelection(m_list, pdc, rc, state);

    rc.DeflateRect(2, 3);

    if (rc.right <= rc.left || rc.bottom <= rc.top)
    {
        return;
    }

    CTreemap treemap;
    treemap.DrawColorPreview(pdc, rc, m_record.color, &COptions::TreemapOptions);
}

CStringW CExtensionListControl::CListItem::GetText(int subitem) const
{
    switch (subitem)
    {
    case COL_EXTENSION:
        {
            return GetExtension();
        }

    case COL_COLOR:
        {
            return L"(color)";
        }

    case COL_BYTES:
        {
            return FormatBytes(m_record.bytes);
        }

    case COL_FILES:
        {
            return FormatCount(m_record.files);
        }

    case COL_DESCRIPTION:
        {
            return GetDescription();
        }

    case COL_BYTESPERCENT:
        {
            return GetBytesPercent();
        }

    default:
        {
            ASSERT(0);
            return wds::strEmpty;
        }
    }
}

CStringW CExtensionListControl::CListItem::GetExtension() const
{
    return m_extension;
}

int CExtensionListControl::CListItem::GetImage() const
{
    if (m_image == -1)
    {
        m_image = GetMyImageList()->getExtImageAndDescription(m_extension, m_description);
    }
    return m_image;
}

CStringW CExtensionListControl::CListItem::GetDescription() const
{
    if (m_description.IsEmpty())
    {
        m_image = GetMyImageList()->getExtImageAndDescription(m_extension, m_description);
    }
    return m_description;
}

CStringW CExtensionListControl::CListItem::GetBytesPercent() const
{
    CStringW s;
    s.Format(L"%s%%", FormatDouble(GetBytesFraction() * 100).GetString());
    return s;
}

double CExtensionListControl::CListItem::GetBytesFraction() const
{
    if (m_list->GetRootSize() == 0)
    {
        return 0;
    }

    return static_cast<double>(m_record.bytes) /
        static_cast<double>(m_list->GetRootSize());
}

int CExtensionListControl::CListItem::Compare(const CSortingListItem* baseOther, int subitem) const
{
    int r = 0;

    auto other = static_cast<const CListItem*>(baseOther);

    switch (subitem)
    {
    case COL_EXTENSION:
        {
            r = signum(GetExtension().CompareNoCase(other->GetExtension()));
        }
        break;

    case COL_COLOR:
    case COL_BYTES:
        {
            r = usignum(m_record.bytes, other->m_record.bytes);
        }
        break;

    case COL_FILES:
        {
            r = usignum(m_record.files, other->m_record.files);
        }
        break;

    case COL_DESCRIPTION:
        {
            r = signum(GetDescription().CompareNoCase(other->GetDescription()));
        }
        break;

    case COL_BYTESPERCENT:
        {
            r = signum(GetBytesFraction() - other->GetBytesFraction());
        }
        break;

    default:
        {
            ASSERT(0);
        }
    }

    return r;
}

/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CExtensionListControl, COwnerDrawnListControl)
    ON_WM_MEASUREITEM_REFLECT()
    ON_WM_DESTROY()
#pragma warning(suppress: 26454)
    ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnLvnDeleteitem)
    ON_WM_SETFOCUS()
#pragma warning(suppress: 26454)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnLvnItemchanged)
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

CExtensionListControl::CExtensionListControl(CTypeView* typeView)
    : COwnerDrawnListControl(19, COptions::TypesColumnOrder.Ptr(), COptions::TypesColumnWidths.Ptr()) // FIXME: Harcoded value
      , m_typeView(typeView)
{
    m_rootSize = 0;
}

bool CExtensionListControl::GetAscendingDefault(int column)
{
    switch (column)
    {
    case COL_EXTENSION:
    case COL_DESCRIPTION:
        {
            return true;
        }
    case COL_COLOR:
    case COL_BYTES:
    case COL_FILES:
    case COL_BYTESPERCENT:
        {
            return false;
        }
    default:
        {
            ASSERT(0);
            return true;
        }
    }
}

// As we will not receive WM_CREATE, we must do initialization
// in this extra method. The counterpart is OnDestroy().
void CExtensionListControl::Initialize()
{
    SetSorting(COL_BYTES, false);

    InsertColumn(COL_EXTENSION, Localization::Lookup(IDS_EXTCOL_EXTENSION), LVCFMT_LEFT, 60, COL_EXTENSION);
    InsertColumn(COL_COLOR, Localization::Lookup(IDS_EXTCOL_COLOR), LVCFMT_LEFT, 40, COL_COLOR);
    InsertColumn(COL_BYTES, Localization::Lookup(IDS_EXTCOL_BYTES), LVCFMT_RIGHT, 60, COL_BYTES);
    InsertColumn(COL_BYTESPERCENT, L"% " + Localization::Lookup(IDS_EXTCOL_BYTES), LVCFMT_RIGHT, 50, COL_BYTESPERCENT);
    InsertColumn(COL_FILES, Localization::Lookup(IDS_EXTCOL_FILES), LVCFMT_RIGHT, 50, COL_FILES);
    InsertColumn(COL_DESCRIPTION, Localization::Lookup(IDS_EXTCOL_DESCRIPTION), LVCFMT_LEFT, 170, COL_DESCRIPTION);

    OnColumnsInserted();

    // We don't use the list control's image list, but attaching an image list
    // to the control ensures a proper line height.
    SetImageList(GetMyImageList(), LVSIL_SMALL);
}

void CExtensionListControl::OnDestroy()
{
    COwnerDrawnListControl::OnDestroy();
}

void CExtensionListControl::SetExtensionData(const CExtensionData* ed)
{
    DeleteAllItems();

    int i        = 0;
    POSITION pos = ed->GetStartPosition();
    while (pos != nullptr)
    {
        CStringW ext;
        SExtensionRecord r;
        ed->GetNextAssoc(pos, ext, r);

        auto item = new CListItem(this, ext, r);
        InsertListItem(i++, item);
    }

    SortItems();
}

void CExtensionListControl::SetRootSize(ULONGLONG totalBytes)
{
    m_rootSize = totalBytes;
}

ULONGLONG CExtensionListControl::GetRootSize() const
{
    return m_rootSize;
}

void CExtensionListControl::SelectExtension(LPCWSTR ext)
{
    int i = 0;
    for (i = 0; i < GetItemCount(); i++)
    {
        if (GetListItem(i)->GetExtension().CompareNoCase(ext) == 0)
        {
            break;
        }
    }
    if (i < GetItemCount())
    {
        SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        EnsureVisible(i, false);
    }
}

CStringW CExtensionListControl::GetSelectedExtension()
{
    POSITION pos = GetFirstSelectedItemPosition();
    if (pos == nullptr)
    {
        return wds::strEmpty;
    }

    const int i = GetNextSelectedItem(pos);
    const CListItem* item = GetListItem(i);
    return item->GetExtension();
}

CExtensionListControl::CListItem* CExtensionListControl::GetListItem(int i) const
{
    return (CListItem*)GetItemData(i);
}

void CExtensionListControl::OnLvnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult)
{
    const auto lv = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    delete (CListItem*)lv->lParam;
    *pResult = 0;
}

void CExtensionListControl::MeasureItem(LPMEASUREITEMSTRUCT mis)
{
    mis->itemHeight = GetRowHeight();
}

void CExtensionListControl::OnSetFocus(CWnd* pOldWnd)
{
    COwnerDrawnListControl::OnSetFocus(pOldWnd);
    GetMainFrame()->SetLogicalFocus(LF_EXTENSIONLIST);
}

void CExtensionListControl::OnLvnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    const auto pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if ((pNMLV->uNewState & LVIS_SELECTED) != 0)
    {
        m_typeView->SetHighlightExtension(GetSelectedExtension());
    }
    *pResult = 0;
}

void CExtensionListControl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_TAB)
    {
        GetMainFrame()->MoveFocus(LF_DIRECTORYLIST);
    }
    else if (nChar == VK_ESCAPE)
    {
        GetMainFrame()->MoveFocus(LF_NONE);
    }
    COwnerDrawnListControl::OnKeyDown(nChar, nRepCnt, nFlags);
}

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CTypeView, CView)

BEGIN_MESSAGE_MAP(CTypeView, CView)
    ON_WM_CREATE()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_SETFOCUS()
END_MESSAGE_MAP()

CTypeView::CTypeView()
    : m_extensionListControl(this)
{
    m_showTypes = true;
}

void CTypeView::SysColorChanged()
{
    m_extensionListControl.SysColorChanged();
}

bool CTypeView::IsShowTypes() const
{
    return m_showTypes;
}

void CTypeView::ShowTypes(bool show)
{
    m_showTypes = show;
    OnUpdate(nullptr, 0, nullptr);
}

void CTypeView::SetHighlightExtension(LPCWSTR ext)
{
    GetDocument()->SetHighlightExtension(ext);
    if (GetFocus() == &m_extensionListControl)
    {
        GetDocument()->UpdateAllViews(this, HINT_EXTENSIONSELECTIONCHANGED);
    }
}

int CTypeView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    constexpr RECT rect = {0, 0, 0, 0};
    VERIFY(m_extensionListControl.Create(LVS_SINGLESEL | LVS_OWNERDRAWFIXED | LVS_SHOWSELALWAYS | WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_WDS_CONTROL));
    m_extensionListControl.SetExtendedStyle(m_extensionListControl.GetExtendedStyle() | LVS_EX_HEADERDRAGDROP);

    m_extensionListControl.ShowGrid(COptions::ListGrid);
    m_extensionListControl.ShowStripes(COptions::ListStripes);
    m_extensionListControl.ShowFullRowSelection(COptions::ListFullRowSelection);

    m_extensionListControl.Initialize();
    return 0;
}

void CTypeView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject*)
{
    switch (lHint)
    {
    case HINT_NEWROOT:
    case HINT_NULL:
        if (IsShowTypes() && GetDocument()->IsRootDone())
        {
            m_extensionListControl.SetRootSize(GetDocument()->GetRootSize());
            m_extensionListControl.SetExtensionData(GetDocument()->GetExtensionData());

            // If there is no vertical scroll bar, the header control doen't repaint
            // correctly. Don't know why. But this helps:
            m_extensionListControl.GetHeaderCtrl()->InvalidateRect(nullptr);
        }
        else
        {
            m_extensionListControl.DeleteAllItems();
        }

        [[fallthrough]];
    case HINT_SELECTIONREFRESH:
        if (IsShowTypes())
        {
            SetSelection();
        }
        break;

    case HINT_ZOOMCHANGED:
        break;

    case HINT_TREEMAPSTYLECHANGED:
        {
            InvalidateRect(nullptr);
            m_extensionListControl.InvalidateRect(nullptr);
            m_extensionListControl.GetHeaderCtrl()->InvalidateRect(nullptr);
        }
        break;

    case HINT_LISTSTYLECHANGED:
        {
            m_extensionListControl.ShowGrid(COptions::ListGrid);
            m_extensionListControl.ShowStripes(COptions::ListStripes);
            m_extensionListControl.ShowFullRowSelection(COptions::ListFullRowSelection);
        }
        break;
    default:
        break;
    }
}

void CTypeView::SetSelection()
{
    const auto & items = CTreeListControl::GetTheTreeListControl()->GetAllSelected<CItem>();
    for (const auto & item : items)
    {
        if (item->GetType() != IT_FILE)
        {
            m_extensionListControl.EnsureVisible(0, false);
        }
        else
        {
            m_extensionListControl.SelectExtension(item->GetExtension());
        }
    }
}

void CTypeView::OnDraw(CDC* pDC)
{
    CView::OnDraw(pDC);
}

void CTypeView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    if (::IsWindow(m_extensionListControl.m_hWnd))
    {
        CRect rc(0, 0, cx, cy);
        m_extensionListControl.MoveWindow(rc);
    }
}

void CTypeView::OnSetFocus(CWnd* /*pOldWnd*/)
{
    m_extensionListControl.SetFocus();
}
