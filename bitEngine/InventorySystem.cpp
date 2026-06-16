// InventorySystem.cpp - Player inventory implementation.

#include "InventorySystem.h"
#include <imgui.h>
#include <algorithm>
#include <iostream>

void InventorySystem::AddItem(const Item& item)
{
    for (auto& e : m_entries)
    {
        if (e.item.name == item.name)
        {
            e.stackCount += item.quantity;
            std::cout << "[Inventory] +" << item.quantity
                      << " " << item.name
                      << "  (total: " << e.stackCount << ")\n";
            return;
        }
    }

    InventoryEntry entry;
    entry.item       = item;
    entry.stackCount = item.quantity;
    m_entries.push_back(entry);
    std::cout << "[Inventory] Added: " << item.name
              << " x" << item.quantity << "\n";
}

bool InventorySystem::RemoveItem(const std::string& name, int quantity)
{
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        if (it->item.name == name)
        {
            if (it->stackCount < quantity) return false;
            it->stackCount -= quantity;
            if (it->stackCount == 0) m_entries.erase(it);
            return true;
        }
    }
    return false;
}

int InventorySystem::GetQuantity(const std::string& name) const
{
    for (const auto& e : m_entries)
        if (e.item.name == name) return e.stackCount;
    return 0;
}

bool InventorySystem::HasItem(const std::string& name, int quantity) const
{
    return GetQuantity(name) >= quantity;
}

void InventorySystem::DrawUI(float displayW, float displayH)
{
    if (!m_open) return;

    // Full-screen dim behind the panel.
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        {0.f, 0.f}, {displayW, displayH},
        IM_COL32(0, 0, 0, 180));

    const float kPanelW = std::min(displayW * 0.72f, 800.f);
    const float kPanelH = std::min(displayH * 0.78f, 560.f);
    const float kPx     = (displayW - kPanelW) * 0.5f;
    const float kPy     = (displayH - kPanelH) * 0.5f;

    // Deus Ex Mankind Divided-style: dark background, amber accents.
    ImGui::PushStyleColor(ImGuiCol_WindowBg,      ImVec4(0.06f,0.05f,0.04f,0.97f));
    ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0.55f,0.40f,0.10f,0.85f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg,       ImVec4(0.10f,0.08f,0.03f,1.00f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.20f,0.14f,0.03f,1.00f));
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.25f,0.18f,0.04f,0.80f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.40f,0.28f,0.06f,0.90f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(16.f,14.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(8.f,7.f));

    ImGui::SetNextWindowPos (ImVec2(kPx, kPy),           ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelW, kPanelH),   ImGuiCond_Always);

    bool open = m_open;
    if (ImGui::Begin("INVENTORY  [I]", &open,
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove   |
                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f,0.60f,0.18f,1.f));
        ImGui::Text("ALL ITEMS  (%d)", (int)m_entries.size());
        ImGui::PopStyleColor();
        ImGui::SameLine(kPanelW - 80.f);
        ImGui::TextDisabled("[I] close");
        ImGui::Separator();
        ImGui::Spacing();

        if (m_entries.empty())
        {
            ImGui::TextColored(ImVec4(0.45f,0.42f,0.38f,1.f), "Your inventory is empty.");
        }
        else
        {
            ImGui::BeginChild("InvScroll", ImVec2(0.f,0.f), false);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f,0.52f,0.20f,1.f));
            ImGui::Text("%-28s %6s   %s", "ITEM", "QTY", "DESCRIPTION");
            ImGui::PopStyleColor();
            ImGui::Separator();

            for (int i = 0; i < (int)m_entries.size(); ++i)
            {
                const auto& e = m_entries[i];
                ImGui::PushID(i);

                // Subtle alternating row highlight.
                if (i % 2 == 0)
                {
                    ImVec2 rowMin = ImGui::GetCursorScreenPos();
                    ImVec2 rowMax = { rowMin.x + kPanelW - 32.f,
                                     rowMin.y + ImGui::GetTextLineHeightWithSpacing() };
                    ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax,
                        IM_COL32(255, 200, 60, 12));
                }

                ImGui::TextColored(ImVec4(0.88f,0.78f,0.50f,1.f),
                                   "%-28s", e.item.name.c_str());
                ImGui::SameLine(0.f, 8.f);
                ImGui::TextColored(ImVec4(0.65f,0.65f,0.65f,1.f),
                                   "%4d", e.stackCount);
                if (!e.item.description.empty())
                {
                    ImGui::SameLine(0.f, 12.f);
                    ImGui::TextDisabled("%s", e.item.description.c_str());
                }

                ImGui::PopID();
            }

            ImGui::EndChild();
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(6);

    if (!open) m_open = false;
}
