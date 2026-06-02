#pragma once

// InventorySystem.h - Player inventory.
//
// Stores collected items as a flat list of stacks keyed by name.
// Items arrive via container grabs (EditorUI::DrawContainerPopup) and
// standalone pickups (Interaction::SpawnPickup callback).
//
// The UI renders a Deus Ex Mankind Divided-style overlay toggled by I.
// No size limit is enforced yet; that will follow weapon/ammo/consumable design.

#include "World.h"
#include <vector>
#include <string>

struct InventoryEntry
{
    Item item;
    int  stackCount = 1;
};

class InventorySystem
{
public:
    InventorySystem() = default;

    // Add item, stacking with an existing entry of the same name.
    void AddItem(const Item& item);

    // Remove quantity units of the named item. Returns false if not enough.
    bool RemoveItem(const std::string& name, int quantity = 1);

    int  GetQuantity(const std::string& name) const;
    bool HasItem    (const std::string& name, int quantity = 1) const;

    const std::vector<InventoryEntry>& GetEntries() const { return m_entries; }

    void Toggle()         { m_open = !m_open; }
    bool IsOpen()   const { return m_open;     }

    // Draw the full-screen inventory overlay. Call from EditorUI::Render each frame.
    void DrawUI(float displayW, float displayH);

private:
    std::vector<InventoryEntry> m_entries;
    bool                        m_open = false;
};
