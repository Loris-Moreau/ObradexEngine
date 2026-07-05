// FileDialog.cpp - Native file-open dialog implementation.

#include "FileDialog.h"
#include <filesystem>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")
#endif

namespace fs = std::filesystem;

// Builds the Windows OPENFILENAMEA lpstrFilter double-null-terminated string:
// "Image Files\0*.png;*.jpg;*.jpeg\0" -- both parts null-terminated, whole
// thing double-null-terminated.
static std::string BuildWinFilter(const char* name, const char* pattern)
{
    std::string f;
    f += name;   f.push_back('\0');
    f += pattern; f.push_back('\0');
    f.push_back('\0');
    return f;
}

// Converts an absolute path to one relative to the current working
// directory when possible; falls back to the absolute path otherwise
// (e.g. selection on a different drive on Windows).
static std::string ToRelative(const std::string& absPath)
{
    std::error_code ec;
    fs::path rel = fs::relative(fs::path(absPath), fs::current_path(), ec);
    if (ec || rel.empty() || rel.string().rfind("..", 0) == 0)
        return absPath;
    // Normalise to forward slashes so paths are portable in saved level files.
    std::string s = rel.string();
    for (char& c : s) if (c == '\\') c = '/';
    return s;
}

bool BrowseForFile(const char* dialogTitle,
                   const char* filterName,
                   const char* filterPattern,
                   const char* initialDir,
                   std::string& outPath)
{
#ifdef _WIN32
    char path[MAX_PATH] = {};
    std::string filter = BuildWinFilter(filterName, filterPattern);

    OPENFILENAMEA ofn = {};
    ofn.lStructSize  = sizeof(ofn);
    ofn.lpstrFilter  = filter.c_str();
    ofn.lpstrFile    = path;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrTitle   = dialogTitle;
    ofn.lpstrInitialDir = initialDir;
    // OFN_NOCHANGEDIR: the dialog can change CWD as a side effect on some
    // Windows versions; this flag prevents that so relative asset paths
    // elsewhere in the engine keep working after the dialog closes.
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&ofn)) return false;
    outPath = ToRelative(path);
    return true;
#else
    // zenity pattern syntax: "Image files | *.png *.jpg *.jpeg" (space-separated,
    // no semicolons). Convert the semicolon-separated pattern we were given.
    std::string zenPattern = filterPattern;
    for (char& c : zenPattern) if (c == ';') c = ' ';

    std::string cmd = "zenity --file-selection --title='" + std::string(dialogTitle) +
                      "' --file-filter='" + filterName + " | " + zenPattern +
                      "' --filename='" + std::string(initialDir) + "/' 2>/dev/null";

    FILE* f = popen(cmd.c_str(), "r");
    if (!f) return false;

    char buf[1024] = {};
    bool got = (fgets(buf, sizeof(buf), f) != nullptr);
    pclose(f);
    if (!got) return false;

    size_t len = strlen(buf);
    if (len && buf[len-1] == '\n') buf[len-1] = '\0';
    if (buf[0] == '\0') return false;

    outPath = ToRelative(buf);
    return true;
#endif
}
