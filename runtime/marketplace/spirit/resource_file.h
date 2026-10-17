#pragma once
#include <Windows.h>
#include <string>
#include "player_resource_settings.h"

namespace crimson::marketplace::resources {
inline std::uint32_t load_setting_from_directory(const std::wstring& directory) {
    const auto attributes = GetFileAttributesW(directory.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const auto error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? kLegacyDefault : 0;
    }
    if (!(attributes & FILE_ATTRIBUTE_DIRECTORY) || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return 0;
    const std::wstring path = directory + L"\\player-resources.dat";
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                             OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        const auto error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? kLegacyDefault : 0;
    }
    BY_HANDLE_FILE_INFORMATION info{};
    unsigned char bytes[9]{};
    DWORD count{};
    const bool valid = GetFileInformationByHandle(file, &info) &&
        !(info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) &&
        info.nFileSizeHigh == 0 && info.nFileSizeLow == 8 && info.nNumberOfLinks == 1 &&
        ReadFile(file, bytes, sizeof(bytes), &count, nullptr);
    CloseHandle(file);
    return valid ? setting(bytes, count) : 0;
}
} // namespace crimson::marketplace::resources
