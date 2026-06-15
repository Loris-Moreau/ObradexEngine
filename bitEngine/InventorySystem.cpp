// InventorySystem.cpp - DXMD-style grid inventory with drag/drop.

#include "InventorySystem.h"
#include "Engine.h"
#include "TextureManager.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <cstdio>

// ── Grid helpers ───────────────────────────────────────────────────────────

bool InventorySystem::RegionFree(int col, int row, int w, int h) const
{
    if (col < 0 || row < 0 || col + w > kInvCols || row + h > kInvRows)
        return false;
    for (int r = row; r < row + h; ++r)
        for (int c = col; c < col + w; ++c)
            if (m_grid[r * kInvCols + c]) return false;
    return true;
}

void InventorySystem::SetRegion(int col, int row, int w, int h, bool occupied)
{
    for (int r = row; r < row + h; ++r)
        for (int c = col; c < col + w; ++c)
            m_grid[r * kInvCols + c] = occupied;
}

bool InventorySystem::AutoPlace(PlacedItem& pi)
{
    int w = std::max(1, pi.item.gridW);
    int h = std::max(1, pi.item.gridH);
    for (int r = 0; r <= kInvRows - h; ++r)
        for (int c = 0; c <= kInvCols - w; ++c)
            if (RegionFree(c, r, w, h))
            {
                pi.col = c; pi.row = r;
                SetRegion(c, r, w, h, true);
                return true;
            }
    return false;
}

void InventorySystem::Organise()
{
    // Rebuild the grid from scratch using the same greedy strategy.
    m_grid.fill(false);
    for (auto& pi : m_placed)
    {
        pi.col = 0; pi.row = 0;
        AutoPlace(pi);
    }
}

// ── Public API ─────────────────────────────────────────────────────────────

bool InventorySystem::AddItem(const Item& item)
{
    // Stack with an existing item of the same name.
    for (auto& pi : m_placed)
    {
        if (pi.item.name == item.name)
        {
            pi.item.quantity += item.quantity;
            return true;
        }
    }

    PlacedItem pi;
    pi.item = item;
    if (!AutoPlace(pi)) return false;
    m_placed.push_back(pi);
    return true;
}

bool InventorySystem::RemoveItem(const std::string& name, int quantity)
{
    for (auto it = m_placed.begin(); it != m_placed.end(); ++it)
    {
        if (it->item.name == name)
        {
            if (it->item.quantity < quantity) return false;
            it->item.quantity -= quantity;
            if (it->item.quantity == 0)
            {
                SetRegion(it->col, it->row,
                          it->item.gridW, it->item.gridH, false);
                m_placed.erase(it);
            }
            return true;
        }
    }
    return false;
}

int InventorySystem::GetQuantity(const std::string& name) const
{
    for (const auto& pi : m_placed)
        if (pi.item.name == name) return pi.item.quantity;
    return 0;
}

bool InventorySystem::HasItem(const std::string& name, int qty) const
{
    return GetQuantity(name) >= qty;
}

// ── UI ─────────────────────────────────────────────────────────────────────

void InventorySystem::DrawUI(float displayW, float displayH)
{
    if (!m_open) return;

    // Full-screen dim
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        {0,0}, {displayW, displayH}, IM_COL32(0,0,0,190));

    // Panel size: grid + left sidebar for selected-item details
    const float kSideW   = 220.f;
    const float kPadding = 14.f;
    const float kGridW   = kInvCols * kCellPx;
    const float kGridH   = kInvRows * kCellPx;
    const float kPanelW  = kSideW + kGridW + kPadding * 3.f;
    const float kPanelH  = kGridH + kPadding * 4.f + 50.f;
    const float kPX      = (displayW - kPanelW) * 0.5f;
    const float kPY      = (displayH - kPanelH) * 0.5f;

    // DXMD amber-on-dark style
    ImGui::PushStyleColor(ImGuiCol_WindowBg,       ImVec4(0.04f,0.04f,0.04f,0.97f));
    ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(0.55f,0.40f,0.10f,0.85f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg,        ImVec4(0.08f,0.06f,0.02f,1.00f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,  ImVec4(0.16f,0.12f,0.03f,1.00f));
    ImGui::PushStyleColor(ImGuiCol_Button,         ImVec4(0.18f,0.13f,0.03f,1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.38f,0.26f,0.06f,1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(kPadding, kPadding));

    ImGui::SetNextWindowPos (ImVec2(kPX, kPY),       ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelW,kPanelH), ImGuiCond_Always);

    bool open = m_open;
    if (!ImGui::Begin("INVENTORY  [I]", &open,
                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(6);
        if (!open) m_open = false;
        return;
    }

    ImDrawList* dl     = ImGui::GetWindowDrawList();
    ImVec2      winPos = ImGui::GetWindowPos();

    // ── Left sidebar (selected item detail) ──────────────────────────────
    ImGui::BeginChild("##inv_side", ImVec2(kSideW, kGridH), false);
    if (m_ctxIdx >= 0 && m_ctxIdx < (int)m_placed.size())
    {
        const Item& item = m_placed[m_ctxIdx].item;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f,0.62f,0.18f,1.f));
        ImGui::TextWrapped("%s", item.name.c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextWrapped("%s", item.description.c_str());
        ImGui::Spacing();
        ImGui::Text("Qty: %d", item.quantity);
        ImGui::Text("Size: %dx%d", item.gridW, item.gridH);
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        if (ImGui::Button("Use",     ImVec2(-1,0))) { /* TODO: use item */ }
        if (ImGui::Button("Drop",    ImVec2(-1,0)))
        {
            RemoveItem(item.name, 1);
            m_ctxIdx = -1;
        }
        if (ImGui::Button("Examine", ImVec2(-1,0))) {}  // TODO: examine popup
        if (ImGui::Button("Discard all", ImVec2(-1,0)))
        {
            RemoveItem(item.name, item.quantity);
            m_ctxIdx = -1;
        }
    }
    else
    {
        ImGui::TextDisabled("Select an item");
    }
    ImGui::EndChild();

    ImGui::SameLine(0.f, kPadding);

    // ── Grid area ─────────────────────────────────────────────────────────
    ImGui::BeginChild("##inv_grid", ImVec2(kGridW, kGridH + 40.f), false);

    ImVec2 gridOrigin = ImGui::GetCursorScreenPos();

    // Draw grid lines
    for (int r = 0; r <= kInvRows; ++r)
        dl->AddLine(ImVec2(gridOrigin.x, gridOrigin.y + r * kCellPx),
                    ImVec2(gridOrigin.x + kGridW, gridOrigin.y + r * kCellPx),
                    IM_COL32(60,50,20,180));
    for (int c = 0; c <= kInvCols; ++c)
        dl->AddLine(ImVec2(gridOrigin.x + c * kCellPx, gridOrigin.y),
                    ImVec2(gridOrigin.x + c * kCellPx, gridOrigin.y + kGridH),
                    IM_COL32(60,50,20,180));

    // Draw placed items
    for (int i = 0; i < (int)m_placed.size(); ++i)
    {
        if (i == m_dragIdx) continue;  // drawn separately while dragging

        const PlacedItem& pi  = m_placed[i];
        int w = std::max(1, pi.item.gridW);
        int h = std::max(1, pi.item.gridH);

        ImVec2 tl(gridOrigin.x + pi.col * kCellPx,
                  gridOrigin.y + pi.row * kCellPx);
        ImVec2 br(tl.x + w * kCellPx, tl.y + h * kCellPx);

        bool hovered = ImGui::IsMouseHoveringRect(tl, br);

        // Item cell background
        ImU32 bg = (m_ctxIdx == i) ? IM_COL32(80,60,10,230)
                 : hovered         ? IM_COL32(60,45,8,210)
                                   : IM_COL32(30,25,5,200);
        dl->AddRectFilled(tl, br, bg, 3.f);

        // Icon or coloured square
        if (pi.item.iconTexID != 0)
        {
            ImVec2 ip(tl.x+4, tl.y+4), ib(br.x-4, br.y-20);
            dl->AddImage(reinterpret_cast<ImTextureID>(
                             static_cast<intptr_t>(pi.item.iconTexID)),
                         ip, ib);
        }

        // Item name (truncated)
        char lbl[32];
        std::snprintf(lbl, sizeof(lbl), "%.14s", pi.item.name.c_str());
        ImVec2 txtPos(tl.x + 4.f, br.y - 18.f);
        dl->AddText(txtPos, IM_COL32(220,185,80,255), lbl);

        // Quantity badge (bottom-right)
        if (pi.item.quantity > 1)
        {
            char qty[8];
            std::snprintf(qty, sizeof(qty), "x%d", pi.item.quantity);
            ImVec2 qSize = ImGui::CalcTextSize(qty);
            dl->AddText(ImVec2(br.x - qSize.x - 3.f, tl.y + 3.f),
                        IM_COL32(160,160,160,200), qty);
        }

        // Border
        dl->AddRect(tl, br,
                    hovered ? IM_COL32(200,160,40,255) : IM_COL32(100,75,15,180),
                    3.f, 0, 1.5f);

        // Left-click: select
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_ctxIdx  = i;
            m_dragIdx = i;
            // Record drag offset in cells
            ImVec2 mouse = ImGui::GetMousePos();
            m_dragOffCol = (int)((mouse.x - gridOrigin.x) / kCellPx) - pi.col;
            m_dragOffRow = (int)((mouse.y - gridOrigin.y) / kCellPx) - pi.row;
        }

        // Right-click: select (context menu handled at panel level)
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            m_ctxIdx = i;
    }

    // Drag: draw item ghost under mouse and drop on release
    if (m_dragIdx >= 0 && m_dragIdx < (int)m_placed.size()
        && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        PlacedItem& pi = m_placed[m_dragIdx];
        ImVec2 mouse   = ImGui::GetMousePos();
        int dropCol    = (int)((mouse.x - gridOrigin.x) / kCellPx) - m_dragOffCol;
        int dropRow    = (int)((mouse.y - gridOrigin.y) / kCellPx) - m_dragOffRow;
        int w          = std::max(1, pi.item.gridW);
        int h          = std::max(1, pi.item.gridH);

        ImVec2 tl(gridOrigin.x + dropCol * kCellPx,
                  gridOrigin.y + dropRow * kCellPx);
        ImVec2 br(tl.x + w * kCellPx, tl.y + h * kCellPx);

        bool canDrop = RegionFree(dropCol, dropRow, w, h) ||
                       (dropCol == pi.col && dropRow == pi.row);
        dl->AddRectFilled(tl, br,
                          canDrop ? IM_COL32(80,60,10,180)
                                  : IM_COL32(120,20,20,180), 3.f);
        dl->AddRect(tl, br,
                    canDrop ? IM_COL32(200,160,40,200)
                            : IM_COL32(200,40,40,200), 3.f, 0, 2.f);
    }
    else if (m_dragIdx >= 0)
    {
        // Mouse released - attempt to drop
        PlacedItem& pi = m_placed[m_dragIdx];
        ImVec2 mouse   = ImGui::GetMousePos();
        int dropCol    = (int)((mouse.x - gridOrigin.x) / kCellPx) - m_dragOffCol;
        int dropRow    = (int)((mouse.y - gridOrigin.y) / kCellPx) - m_dragOffRow;
        int w          = std::max(1, pi.item.gridW);
        int h          = std::max(1, pi.item.gridH);

        // Temporarily free the item's current cells so it doesn't block itself
        SetRegion(pi.col, pi.row, w, h, false);
        if (RegionFree(dropCol, dropRow, w, h))
        {
            pi.col = dropCol;
            pi.row = dropRow;
        }
        // Re-occupy (new or original position)
        SetRegion(pi.col, pi.row, w, h, true);
        m_dragIdx = -1;
    }

    ImGui::Dummy(ImVec2(kGridW, kGridH));  // Reserve space

    // Auto-organise button
    ImGui::Spacing();
    if (ImGui::Button("Auto-Organise [A]", ImVec2(-1, 0)))
        Organise();

    ImGui::EndChild();

    // Context menu (right-click)
    if (m_ctxIdx >= 0 && m_ctxIdx < (int)m_placed.size()
        && ImGui::BeginPopupContextWindow("##inv_ctx"))
    {
        const Item& item = m_placed[m_ctxIdx].item;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f,0.62f,0.18f,1.f));
        ImGui::Text("%s", item.name.c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();
        if (ImGui::MenuItem("Use"))    { /* TODO */ }
        if (ImGui::MenuItem("Examine")){ /* TODO */ }
        if (ImGui::MenuItem("Drop"))   { RemoveItem(item.name, 1); m_ctxIdx = -1; }
        if (ImGui::MenuItem("Discard all")) { RemoveItem(item.name, item.quantity); m_ctxIdx = -1; }
        ImGui::EndPopup();
    }

    // Auto-organise keyboard shortcut
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
        && ImGui::IsKeyPressed(ImGuiKey_A))
        Organise();

    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(6);

    if (!open) m_open = false;
}
