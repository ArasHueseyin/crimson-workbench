#pragma once
#include <Windows.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace crimson::repair {
enum class NativeBuild { legacy_2944, registry_2949 };
// Read-only, hash-pinned file reader. Never LoadLibrary / launch the game EXE.
class Pe {
    NativeBuild build_;
    std::ifstream stream_;
    std::uint64_t length_ = 0;
    std::vector<IMAGE_SECTION_HEADER> sections_;
    IMAGE_DATA_DIRECTORY exceptions_{};
    std::vector<std::uint8_t> read(std::uint64_t offset, std::size_t size);
public:
    explicit Pe(const std::filesystem::path& path, NativeBuild build = NativeBuild::legacy_2944);
    std::vector<std::uint8_t> rva(std::uint32_t address, std::size_t size);
    RUNTIME_FUNCTION function(std::uint32_t begin, std::uint32_t size);
    std::vector<std::uint8_t> unwind(const RUNTIME_FUNCTION& function);
    void verify_unchanged();
};
}
