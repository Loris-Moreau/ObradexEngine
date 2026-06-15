#pragma once

// InventorySystem.h - Deus Ex: Mankind Divided-style grid inventory.
//
// Items occupy a rectangular region on a 12x8 grid (cells are 46x46 px).
// Each Item carries gridW x gridH, indicating how many cells it fills.
// Items can be dragged to any free region, or auto-placed by pressing A.
//
// Context menu (right-click) shows: Use, Drop, Examine, Move, Discard.
// Auto-organise packs items top-left via a greedy shelf-packing algorithm.

#include "World.h"
#include <vector>
#include <optional>
#include <string>

// Grid dimensions
static constexpr int kInvCols   = 12;
static constexpr int kInvRows   = 8;
static constexpr float kCellPx  = 46.f;   // Cell size in screen pixels

struct PlacedItem
{
    Item item;
    int  col = 0;   // Top-left grid column (0-based)
    int  row = 0;   // Top-left grid row    (0-based)
};

class InventorySystem
{
public:
    InventorySystem() = default;

    // Add an item, auto-placing it in the first available region.
    // Returns false if the grid is full.
    bool AddItem(const Item& item);

    // Remove one stack unit of the named item. Returns false if not found.
    bool RemoveItem(const std::string& name, int quantity = 1);

    int  GetQuantity(const std::string& name) const;
    bool HasItem    (const std::string& name, int quantity = 1) const;

    const std::vector<PlacedItem>& GetPlaced() const { return m_placed; }

    void Toggle()       { m_open = !m_open; }
    bool IsOpen() const { return m_open; }
    void Clear()        { m_placed.clear(); m_grid.fill(false); }

    // Draw the full inventory overlay. Call once per frame from EditorUI::Render.
    void DrawUI(float displayW, float displayH);

private:
    // Returns true if the region (col,row,w,h) is free.
    bool RegionFree(int col, int row, int w, int h) const;

    // Marks/clears cells in the grid for the given region.
    void SetRegion(int col, int row, int w, int h, bool occupied);

    // Greedy top-left auto-placement; returns false if no space.
    bool AutoPlace(PlacedItem& pi);

    // Pack all placed items top-left (auto-organise).
    void Organise();

    std::array<bool, kInvCols * kInvRows> m_grid = {};
    std::vector<PlacedItem> m_placed;
    bool m_open = false;

    // Drag state
    int  m_dragIdx    = -1;    // Index into m_placed, or -1 if not dragging
    int  m_dragOffCol = 0;
    int  m_dragOffRow = 0;

    // Context menu state
    int  m_ctxIdx = -1;        // Item right-clicked, or -1
};
