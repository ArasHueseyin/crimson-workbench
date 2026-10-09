#include "pe.h"
#include "hash.h"
#include "repair_plan.h"
#include <cstring>
#include <stdexcept>
#include <array>

namespace crimson::repair {
namespace {
template<class T> T object(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() != sizeof(T)) throw std::runtime_error("Invalid PE object size");
    T result;
    std::memcpy(&result, bytes.data(), sizeof(T));
    return result;
}
}
std::vector<std::uint8_t> Pe::read(std::uint64_t offset, std::size_t size) {
    if (offset > length_ || size > length_ - offset || size > 16 * 1024 * 1024)
        throw std::runtime_error("Invalid bounded PE read");
    stream_.clear();
    stream_.seekg(static_cast<std::streamoff>(offset));
    std::vector<std::uint8_t> result(size);
    if (!stream_.read(reinterpret_cast<char*>(result.data()), size))
        throw std::runtime_error("Truncated PE");
    return result;
}
Pe::Pe(const std::filesystem::path& path, NativeBuild build) : build_(build), stream_(path, std::ios::binary) {
    if (!stream_) throw std::runtime_error("Cannot open source EXE read-only");
    // No PE data is trusted/executed until the entire file matches this build.
    verify_unchanged();
    stream_.seekg(0, std::ios::end);
    length_ = static_cast<std::uint64_t>(stream_.tellg());
    const auto dos = object<IMAGE_DOS_HEADER>(read(0, sizeof(IMAGE_DOS_HEADER)));
    if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0)
        throw std::runtime_error("Invalid DOS header");
    const auto nt = object<IMAGE_NT_HEADERS64>(read(dos.e_lfanew, sizeof(IMAGE_NT_HEADERS64)));
    if (nt.Signature != IMAGE_NT_SIGNATURE || nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt.FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER64) ||
        nt.FileHeader.NumberOfSections > 96 || nt.OptionalHeader.ImageBase != 0x140000000ull)
        throw std::runtime_error("Unsupported PE layout");
    auto offset = static_cast<std::uint64_t>(dos.e_lfanew) + sizeof(nt);
    for (unsigned i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
        sections_.push_back(object<IMAGE_SECTION_HEADER>(read(offset, sizeof(IMAGE_SECTION_HEADER))));
        offset += sizeof(IMAGE_SECTION_HEADER);
    }
    exceptions_ = nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
}
std::vector<std::uint8_t> Pe::rva(std::uint32_t address, std::size_t size) {
    for (const auto& s : sections_) {
        if (address >= s.VirtualAddress) {
            const std::uint64_t offset = address - s.VirtualAddress;
            if (offset <= s.SizeOfRawData && size <= s.SizeOfRawData - offset)
                return read(static_cast<std::uint64_t>(s.PointerToRawData) + offset, size);
        }
    }
    throw std::runtime_error("RVA is not fully backed by a PE section");
}
RUNTIME_FUNCTION Pe::function(std::uint32_t begin, std::uint32_t size) {
    if (exceptions_.Size % sizeof(RUNTIME_FUNCTION)) throw std::runtime_error("Invalid pdata");
    const auto bytes = rva(exceptions_.VirtualAddress, exceptions_.Size);
    for (std::size_t i = 0; i < bytes.size(); i += sizeof(RUNTIME_FUNCTION)) {
        RUNTIME_FUNCTION entry;
        std::memcpy(&entry, bytes.data() + i, sizeof(entry));
        if (entry.BeginAddress == begin && entry.EndAddress == begin + size) return entry;
    }
    throw std::runtime_error("Expected function boundary absent from pdata");
}
std::vector<std::uint8_t> Pe::unwind(const RUNTIME_FUNCTION& entry) {
    const auto header = rva(entry.UnwindData, 4);
    // Fixture dependencies are nonthrowing; the 2949 registry fixture uses real
    // Windows SRW locks. Retain stack unwind codes but never register the game's
    // language-specific cleanup handlers in the private arena.
    const bool legacy_cleanup = build_ == NativeBuild::legacy_2944 && (entry.BeginAddress == 0x27b05e0 || entry.BeginAddress == 0x2a82730 ||
        entry.BeginAddress == 0x98fb70 || entry.BeginAddress == 0x1371c20 ||
        entry.BeginAddress == 0x1434900 || entry.BeginAddress == 0x1436690 || entry.BeginAddress == 0x1436bf0) &&
        header[0] == 0x11;
    const bool registry_cleanup = build_ == NativeBuild::registry_2949 &&
        (entry.BeginAddress == 0x8ae140 || entry.BeginAddress == 0x2a82300 ||
         entry.BeginAddress == 0x1434910 || entry.BeginAddress == 0x14366a0 ||
         entry.BeginAddress == 0x1436b20 || entry.BeginAddress == 0x1436c00 ||
         entry.BeginAddress == 0x1371b60 || entry.BeginAddress == 0x1371c30 ||
         entry.BeginAddress == 0x8b4480 || entry.BeginAddress == 0x20c90f0 ||
         entry.BeginAddress == 0x98fb70 || entry.BeginAddress == 0x2adfda0 ||
         entry.BeginAddress == 0x2409750 || entry.BeginAddress == 0xf2fe4a0 ||
         entry.BeginAddress == 0xf300c80 || entry.BeginAddress == 0x3858c0 ||
         entry.BeginAddress == 0x20cdc70 || entry.BeginAddress == 0x2ad1d70 ||
         entry.BeginAddress == 0xa11ca0 || entry.BeginAddress == 0x2c1da50 ||
         entry.BeginAddress == 0x188ccc0 || entry.BeginAddress == 0x188c440 ||
         entry.BeginAddress == 0x188b8f0 || entry.BeginAddress == 0x188bcb0 ||
         entry.BeginAddress == 0x19ed0a0 || entry.BeginAddress == 0x19ed130 ||
         entry.BeginAddress == 0x19ed1f0 || entry.BeginAddress == 0xcdb0b80 ||
         entry.BeginAddress == 0x1a829d0 || entry.BeginAddress == 0x1a82d60 ||
         entry.BeginAddress == 0x13825a0 || entry.BeginAddress == 0x44b220 ||
         entry.BeginAddress == 0x240a040 || entry.BeginAddress == 0x2358700 ||
         entry.BeginAddress == 0x235c750 || entry.BeginAddress == 0x1326490 ||
         entry.BeginAddress == 0x29ec2e0) && header[0] == 0x11;
    const bool fixture_cleanup = legacy_cleanup || registry_cleanup;
    const bool inventory_chain = header[0] == 0x21 &&
        ((build_ == NativeBuild::legacy_2944 &&
          (entry.BeginAddress == 0x212f4e4 || entry.BeginAddress == 0x212f55e || entry.BeginAddress == 0x212f56f)) ||
         (build_ == NativeBuild::registry_2949 &&
          (entry.BeginAddress == 0x212f4f4 || entry.BeginAddress == 0x212f56e || entry.BeginAddress == 0x212f57f)));
    const bool socket_chain = header[0] == 0x21 &&
        ((build_ == NativeBuild::legacy_2944 &&
          (entry.BeginAddress == 0x240e408 || entry.BeginAddress == 0x240e5a6)) ||
         (build_ == NativeBuild::registry_2949 &&
          (entry.BeginAddress == 0x240e418 || entry.BeginAddress == 0x240e5b6)));
    struct Chain { std::uint32_t child, parent, length; };
    constexpr std::array item_chains{
        Chain{0x241185c, 0x2411840, 0x1c}, Chain{0x24118a7, 0x2411840, 0x1c},
        Chain{0x24118c8, 0x2411840, 0x1c}, Chain{0x2411913, 0x24118c8, 0x4b},
        Chain{0x2411999, 0x24118c8, 0x4b}, Chain{0x24119a1, 0x2411840, 0x1c},
        Chain{0xf32a0f0, 0xf32a0d0, 0x20}, Chain{0xf32a104, 0xf32a0f0, 0x14},
        Chain{0xf32a182, 0xf32a0f0, 0x14}, Chain{0xf32a187, 0xf32a0d0, 0x20},
        Chain{0x240fcec, 0x240fcc0, 0x2c}, Chain{0x240fd72, 0x240fcc0, 0x2c},
        Chain{0xe1dba26, 0xe1db910, 0x116}, Chain{0xe1dbb05, 0xe1db910, 0x116},
        Chain{0x20cdb82, 0x20cd9e0, 0x1a2}, Chain{0x20cdc60, 0x20cd9e0, 0x1a2},
        Chain{0x40b7c2, 0x40b7b0, 0x12}, Chain{0x40b7dc, 0x40b7c2, 0x1a},
        Chain{0x40b870, 0x40b7c2, 0x1a}, Chain{0x40b889, 0x40b7b0, 0x12},
        Chain{0x4105a1, 0x410590, 0x11}, Chain{0x4105b7, 0x4105a1, 0x16},
        Chain{0x410608, 0x4105a1, 0x16}, Chain{0x410620, 0x410590, 0x11},
        Chain{0x2776d37, 0x2776d00, 0x37}, Chain{0x2776d5e, 0x2776d37, 0x27},
        Chain{0x2776d9f, 0x2776d37, 0x27}, Chain{0x2776dbd, 0x2776d00, 0x37},
        Chain{0x96facc, 0x96fab0, 0x1c}, Chain{0x96fb25, 0x96fab0, 0x1c},
        Chain{0x96fb3d, 0x96fab0, 0x1c}, Chain{0x96fb4a, 0x96fab0, 0x1c},
        Chain{0x244c472, 0x244c460, 0x12}, Chain{0x244c4e1, 0x244c460, 0x12},
        Chain{0x240901c, 0x2409000, 0x1c}, Chain{0x24090b2, 0x2409000, 0x1c},
        Chain{0x240910a, 0x2409000, 0x1c}, Chain{0x240911d, 0x2409000, 0x1c},
        Chain{0xee2a23f, 0xee2a210, 0x2f}, Chain{0xee2a280, 0xee2a23f, 0x41},
        Chain{0xee2a326, 0xee2a23f, 0x41}, Chain{0xee2a330, 0xee2a210, 0x2f}};
    const Chain* item_chain = nullptr;
    if (build_ == NativeBuild::registry_2949 && header[0] == 0x21)
        for (const auto& c : item_chains) if (c.child == entry.BeginAddress) item_chain = &c;
    const bool chained = inventory_chain || socket_chain || item_chain;
    if (header[0] != 1 && !fixture_cleanup && !chained)
        throw std::runtime_error("Unexpected unwind handler/chain/version");
    const auto prefix = 4 + ((static_cast<std::size_t>(header[2]) + 1) & ~1ull) * 2;
    auto result = rva(entry.UnwindData, prefix + (chained ? sizeof(RUNTIME_FUNCTION) : 0));
    if (chained) {
        RUNTIME_FUNCTION parent;
        std::memcpy(&parent, result.data() + prefix, sizeof(parent));
        const auto expected = item_chain ? function(item_chain->parent, item_chain->length) : inventory_chain ?
            function(build_ == NativeBuild::legacy_2944 ? 0x212f4a0 : 0x212f4b0, 0x44) :
            function(build_ == NativeBuild::legacy_2944 ? 0x240e3e0 : 0x240e3f0, 0x28);
        // This relocated helper retains a chain to a byte-identical original
        // unwind record. Normalize ONLY this pinned alias to the registered copy.
        if (item_chain && parent.BeginAddress == 0xf32a0d0 && parent.EndAddress == 0xf32a0f0 &&
            parent.UnwindData == 0x60b08f0 && expected.UnwindData == 0x17155638 &&
            rva(parent.UnwindData, 12) == rva(expected.UnwindData, 12)) {
            parent.UnwindData = expected.UnwindData;
            std::memcpy(result.data() + prefix, &parent, sizeof(parent));
        }
        if (item_chain && parent.BeginAddress == 0xe1db910 && parent.EndAddress == 0xe1dba26 &&
            parent.UnwindData == 0x6106998 && expected.UnwindData == 0x17147c54 &&
            rva(parent.UnwindData, 24) == rva(expected.UnwindData, 24)) {
            parent.UnwindData = expected.UnwindData;
            std::memcpy(result.data() + prefix, &parent, sizeof(parent));
        }
        if (item_chain && parent.BeginAddress == 0xee2a210 && parent.EndAddress == 0xee2a23f &&
            parent.UnwindData == 0x62bd504 && expected.UnwindData == 0x171512d8 &&
            rva(parent.UnwindData, 12) == rva(expected.UnwindData, 12)) {
            parent.UnwindData = expected.UnwindData;
            std::memcpy(result.data() + prefix, &parent, sizeof(parent));
        }
        if (std::memcmp(&parent, &expected, sizeof(parent)) != 0 ||
            (!item_chain && rva(parent.UnwindData, 1)[0] != 1))
            throw std::runtime_error("Unexpected inventory unwind chain");
        // The chain stores RVAs; its parent is also registered in the arena.
    } else result[0] = 1;
    return result;
}
void Pe::verify_unchanged() {
    const char* expected = nullptr;
    switch (build_) {
        case NativeBuild::legacy_2944: expected = exe_sha256; break;
        case NativeBuild::registry_2949:
            expected = "a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a";
            break;
        default: throw std::runtime_error("Unknown native build selector");
    }
    if (hash_stream(stream_) != expected) throw std::runtime_error("Unknown or changed EXE; refused");
}
}
