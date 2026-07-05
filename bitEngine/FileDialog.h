#pragma once

// FileDialog.h - Native OS file-open dialog, cross-platform.
//
// Windows: GetOpenFileNameA.
// Linux/macOS: zenity (falls back silently to false if not installed).
//
// The selected path is returned relative to the current working directory
// so it can be stored directly in level files and passed to loaders that
// expect working-directory-relative paths (TextureManager::Load, etc).
// If the selection is outside the working directory tree, the absolute
// path is returned instead.

#include <string>

// filterName:    label shown in the dialog, e.g. "Image Files"
// filterPattern: semicolon-free single pattern list, e.g. "*.png;*.jpg;*.jpeg"
// initialDir:    directory the dialog opens in (working-directory-relative)
// outPath:       receives the selected path on success
// Returns false if the user cancelled or no dialog backend is available.
bool BrowseForFile(const char* dialogTitle,
                   const char* filterName,
                   const char* filterPattern,
                   const char* initialDir,
                   std::string& outPath);
