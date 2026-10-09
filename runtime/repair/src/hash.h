#pragma once
#include <cstdint>
#include <iosfwd>
#include <span>
#include <string>

namespace crimson::repair {
std::string hash(std::span<const std::uint8_t> data);
// Hashes through the same open stream subsequently used for the PE reads.
std::string hash_stream(std::istream& stream);
}
