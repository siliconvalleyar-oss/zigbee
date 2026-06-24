#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace zigbee_mesh::core {

class ByteBuffer {
public:
    ByteBuffer() : data_(0) {}

    explicit ByteBuffer(size_t capacity) : data_(capacity, 0) {}

    ByteBuffer(const uint8_t* data, size_t len) : data_(data, data + len) {}

    ByteBuffer(std::vector<uint8_t> data) : data_(std::move(data)) {}

    static ByteBuffer fromHex(const std::string& hex) {
        ByteBuffer buf;
        std::string clean = hex;
        clean.erase(std::remove_if(clean.begin(), clean.end(), ::isspace), clean.end());
        if (clean.size() % 2 != 0) {
            throw std::invalid_argument("Invalid hex string length");
        }
        buf.data_.reserve(clean.size() / 2);
        for (size_t i = 0; i < clean.size(); i += 2) {
            uint8_t byte = static_cast<uint8_t>(
                std::stoul(clean.substr(i, 2), nullptr, 16));
            buf.data_.push_back(byte);
        }
        return buf;
    }

    std::string toHex() const {
        std::ostringstream ss;
        for (uint8_t byte : data_) {
            ss << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(byte);
        }
        return ss.str();
    }

    void appendU8(uint8_t val) { data_.push_back(val); }

    void appendU16LE(uint16_t val) {
        data_.push_back(static_cast<uint8_t>(val & 0xFF));
        data_.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    }

    void appendU16BE(uint16_t val) {
        data_.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        data_.push_back(static_cast<uint8_t>(val & 0xFF));
    }

    void appendU32LE(uint32_t val) {
        data_.push_back(static_cast<uint8_t>(val & 0xFF));
        data_.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        data_.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        data_.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    }

    void appendU32BE(uint32_t val) {
        data_.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
        data_.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        data_.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        data_.push_back(static_cast<uint8_t>(val & 0xFF));
    }

    void appendU64LE(uint64_t val) {
        for (int i = 0; i < 8; ++i) {
            data_.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
        }
    }

    void append(const uint8_t* data, size_t len) {
        data_.insert(data_.end(), data, data + len);
    }

    void append(const ByteBuffer& other) {
        data_.insert(data_.end(), other.data_.begin(), other.data_.end());
    }

    uint8_t readU8(size_t offset) const {
        checkBounds(offset, 1);
        return data_[offset];
    }

    uint16_t readU16LE(size_t offset) const {
        checkBounds(offset, 2);
        return static_cast<uint16_t>(data_[offset]) |
               (static_cast<uint16_t>(data_[offset + 1]) << 8);
    }

    uint16_t readU16BE(size_t offset) const {
        checkBounds(offset, 2);
        return (static_cast<uint16_t>(data_[offset]) << 8) |
               static_cast<uint16_t>(data_[offset + 1]);
    }

    uint32_t readU32LE(size_t offset) const {
        checkBounds(offset, 4);
        return static_cast<uint32_t>(data_[offset]) |
               (static_cast<uint32_t>(data_[offset + 1]) << 8) |
               (static_cast<uint32_t>(data_[offset + 2]) << 16) |
               (static_cast<uint32_t>(data_[offset + 3]) << 24);
    }

    uint32_t readU32BE(size_t offset) const {
        checkBounds(offset, 4);
        return (static_cast<uint32_t>(data_[offset]) << 24) |
               (static_cast<uint32_t>(data_[offset + 1]) << 16) |
               (static_cast<uint32_t>(data_[offset + 2]) << 8) |
               static_cast<uint32_t>(data_[offset + 3]);
    }

    uint64_t readU64LE(size_t offset) const {
        checkBounds(offset, 8);
        uint64_t val = 0;
        for (int i = 0; i < 8; ++i) {
            val |= static_cast<uint64_t>(data_[offset + i]) << (i * 8);
        }
        return val;
    }

    ByteBuffer readBytes(size_t offset, size_t count) const {
        checkBounds(offset, count);
        return ByteBuffer(data_.data() + offset, count);
    }

    uint8_t operator[](size_t idx) const { return data_[idx]; }
    uint8_t& operator[](size_t idx) { return data_[idx]; }

    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    const uint8_t* data() const { return data_.data(); }
    uint8_t* data() { return data_.data(); }

    void resize(size_t new_size) { data_.resize(new_size); }
    void clear() { data_.clear(); }
    void reserve(size_t capacity) { data_.reserve(capacity); }

    void push_back(uint8_t val) { data_.push_back(val); }

    ByteBuffer subbuffer(size_t offset, size_t count = SIZE_MAX) const {
        checkBounds(offset, 1);
        if (count == SIZE_MAX) count = data_.size() - offset;
        checkBounds(offset, count);
        return ByteBuffer(data_.data() + offset, count);
    }

    std::vector<uint8_t> toVector() const { return data_; }

    std::string toString() const {
        return std::string(reinterpret_cast<const char*>(data_.data()), data_.size());
    }

    bool operator==(const ByteBuffer& other) const { return data_ == other.data_; }
    bool operator!=(const ByteBuffer& other) const { return data_ != other.data_; }

    ByteBuffer& operator+=(const ByteBuffer& other) {
        append(other);
        return *this;
    }

    ByteBuffer operator+(const ByteBuffer& other) const {
        ByteBuffer result = *this;
        result += other;
        return result;
    }

private:
    void checkBounds(size_t offset, size_t count) const {
        if (offset + count > data_.size()) {
            throw std::out_of_range("ByteBuffer access out of bounds: offset=" +
                std::to_string(offset) + " count=" + std::to_string(count) +
                " size=" + std::to_string(data_.size()));
        }
    }

    std::vector<uint8_t> data_;
};

} // namespace zigbee_mesh::core
