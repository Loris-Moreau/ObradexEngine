#pragma once

// ============================================================
//  InventorySystem.h  —  Player Inventory
// ============================================================
//  Stores the player's collected items as a flat list of
//  InventoryEntry values (item + stack count).
//
//  The UI follows the Deus Ex: Mankind Divided aesthetic:
//  a dark panel with amber accents, drawn as a full-screen
//  overlay when the player presses I.
//
//  Items arrive here via:
//    - Container grab (EditorUI::DrawContainerPopup calls AddItem)
//    - Standalone pickup (Interaction::SpawnPickup callback)
//
//  The inventory does NOT enforce a size limit at this stage;
//  that constraint will be re-evaluated once item categories
//  (weapons, ammo, consumables) are designed.
// ============================================================

#include "World.h"   // for Item
#include <vector>
#include <string>

// ── InventoryEntry ────────────────────────────────────────────
/// An item stack in the player's inventory.
struct InventoryEntry
{
    Item item;
    int  stackCount = 1;  ///< Total carried across all stacks
};

// ── InventorySystem ───────────────────────────────────────────
class InventorySystem
{
public:
    InventorySystem() = default;

    // ── Item management ───────────────────────────────────────

    /// Add an item (stacks with existing entries by name).
    void AddItem(const Item& item);

    /// Remove `quantity` units of the named item.
    /// Returns true if the full amount was available.
    bool RemoveItem(const std::string& name, int quantity = 1);

    /// Total quantity of the named item currently held.
    int  GetQuantity(const std::string& name) const;

    /// True if at least `quantity` units of the item are held.
    bool HasItem(const std::string& name, int quantity = 1) const;

    /// All current inventory entries (read-only).
    const std::vector<InventoryEntry>& GetEntries() const { return m_entries; }

    // ── UI ────────────────────────────────────────────────────

    /// Toggle the inventory panel open/closed (bound to I).
    void Toggle() { m_open = !m_open; }
    bool IsOpen() const { return m_open; }

    /// Draw the inventory UI.  Call from EditorUI::Render each frame.
    /// @param displayW / displayH  Current window pixel dimensions.
    void DrawUI(float displayW, float displayH);

private:
    std::vector<InventoryEntry> m_entries;
    bool                        m_open = false;
};
