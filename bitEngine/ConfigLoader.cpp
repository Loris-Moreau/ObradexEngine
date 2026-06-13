// ConfigLoader.cpp - INI config file parser.

#include "ConfigLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

static std::string Trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

bool ConfigLoader::Load(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cout << "[Config] " << filepath << " not found, using defaults.\n";
        return false;
    }

    std::string section;
    std::string line;
    while (std::getline(file, line))
    {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        if (line.front() == '[' && line.back() == ']')
        {
            section = ToLower(line.substr(1, line.size() - 2));
            continue;
        }

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key   = ToLower(Trim(line.substr(0, eq)));
        std::string value = Trim(line.substr(eq + 1));
        m_data[section][key] = value;
    }

    std::cout << "[Config] Loaded " << filepath << "\n";
    return true;
}

bool ConfigLoader::Save(const std::string& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    for (auto& [section, pairs] : m_data)
    {
        file << "[" << section << "]\n";
        for (auto& [key, val] : pairs)
            file << key << " = " << val << "\n";
        file << "\n";
    }
    return true;
}

std::string ConfigLoader::GetRaw(const std::string& section,
                                  const std::string& key,
                                  const std::string& def) const
{
    auto sit = m_data.find(ToLower(section));
    if (sit == m_data.end()) return def;
    auto kit = sit->second.find(ToLower(key));
    if (kit == sit->second.end()) return def;
    return kit->second;
}

void ConfigLoader::SetRaw(const std::string& s,
                           const std::string& k,
                           const std::string& v)
{
    m_data[ToLower(s)][ToLower(k)] = v;
}

int ConfigLoader::GetInt(const std::string& s, const std::string& k, int def) const
{
    std::string r = GetRaw(s, k, "");
    if (r.empty()) return def;
    try { return std::stoi(r); } catch (...) { return def; }
}

float ConfigLoader::GetFloat(const std::string& s, const std::string& k, float def) const
{
    std::string r = GetRaw(s, k, "");
    if (r.empty()) return def;
    try { return std::stof(r); } catch (...) { return def; }
}

bool ConfigLoader::GetBool(const std::string& s, const std::string& k, bool def) const
{
    std::string r = ToLower(GetRaw(s, k, ""));
    if (r.empty()) return def;
    return r == "true" || r == "1" || r == "yes";
}

std::string ConfigLoader::GetString(const std::string& s,
                                     const std::string& k,
                                     std::string def) const
{
    return GetRaw(s, k, def);
}

void ConfigLoader::SetInt   (const std::string& s, const std::string& k, int v)
    { SetRaw(s, k, std::to_string(v)); }

void ConfigLoader::SetFloat (const std::string& s, const std::string& k, float v)
    { SetRaw(s, k, std::to_string(v)); }

void ConfigLoader::SetBool  (const std::string& s, const std::string& k, bool v)
    { SetRaw(s, k, v ? "true" : "false"); }

void ConfigLoader::SetString(const std::string& s,
                              const std::string& k,
                              const std::string& v)
    { SetRaw(s, k, v); }
