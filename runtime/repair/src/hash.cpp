#include "hash.h"
#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <istream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace crimson::repair {
namespace {
void checked(NTSTATUS status) {
    if (status < 0) throw std::runtime_error("BCrypt SHA-256 failed");
}
class Digest {
    BCRYPT_ALG_HANDLE algorithm_ = nullptr;
    BCRYPT_HASH_HANDLE state_ = nullptr;
    std::vector<std::uint8_t> object_;
public:
    Digest() {
        checked(BCryptOpenAlgorithmProvider(&algorithm_, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
        try {
            ULONG bytes = 0, written = 0;
            checked(BCryptGetProperty(algorithm_, BCRYPT_OBJECT_LENGTH,
                reinterpret_cast<PUCHAR>(&bytes), sizeof(bytes), &written, 0));
            object_.resize(bytes);
            checked(BCryptCreateHash(algorithm_, &state_, object_.data(), bytes, nullptr, 0, 0));
        } catch (...) {
            BCryptCloseAlgorithmProvider(algorithm_, 0);
            throw;
        }
    }
    Digest(const Digest&) = delete;
    Digest& operator=(const Digest&) = delete;
    ~Digest() {
        if (state_) BCryptDestroyHash(state_);
        BCryptCloseAlgorithmProvider(algorithm_, 0);
    }
    void add(std::span<const std::uint8_t> bytes) {
        if (bytes.size() > std::numeric_limits<ULONG>::max())
            throw std::runtime_error("SHA chunk too large");
        checked(BCryptHashData(state_, const_cast<PUCHAR>(bytes.data()),
            static_cast<ULONG>(bytes.size()), 0));
    }
    std::string finish() {
        std::array<std::uint8_t, 32> bytes{};
        checked(BCryptFinishHash(state_, bytes.data(), static_cast<ULONG>(bytes.size()), 0));
        constexpr char hex[] = "0123456789abcdef";
        std::string result;
        for (auto b : bytes) {
            result.push_back(hex[b >> 4]);
            result.push_back(hex[b & 15]);
        }
        return result;
    }
};
}
std::string hash(std::span<const std::uint8_t> data) {
    Digest digest;
    digest.add(data);
    return digest.finish();
}
std::string hash_stream(std::istream& stream) {
    stream.clear();
    stream.seekg(0);
    Digest digest;
    std::vector<std::uint8_t> buffer(1024 * 1024);
    while (stream.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || stream.gcount())
        digest.add(std::span(buffer.data(), static_cast<std::size_t>(stream.gcount())));
    if (!stream.eof()) throw std::runtime_error("Cannot read source EXE");
    stream.clear();
    stream.seekg(0);
    return digest.finish();
}
}
