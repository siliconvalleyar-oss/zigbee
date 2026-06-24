#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <cstring>
#include <functional>
#include <limits>

namespace zigbee_mesh::core {

using ShortAddress = uint16_t;
using ExtendedAddress = uint64_t;
using PanId = uint16_t;
using Channel = uint8_t;
using Endpoint = uint8_t;
using ClusterId = uint16_t;
using ProfileId = uint16_t;
using AttributeId = uint16_t;
using SequenceNumber = uint8_t;

constexpr ShortAddress kBroadcastAddress = 0xFFFF;
constexpr ShortAddress kCoordinatorAddress = 0x0000;
constexpr ExtendedAddress kExtendedBroadcastAddress = 0xFFFFFFFFFFFFFFFFULL;
constexpr uint8_t kMinChannel = 11;
constexpr uint8_t kMaxChannel = 26;
constexpr size_t kMaxPayloadSize = 127;
constexpr size_t kMaxNwkPayloadSize = 72;
constexpr size_t kMaxAddrMapDepth = 30;

enum class DeviceRole : uint8_t {
    Coordinator = 0x00,
    Router = 0x01,
    EndDevice = 0x02,
    Unknown = 0xFF,
};

enum class NwkState : uint8_t {
    OffNetwork = 0x00,
    Joining = 0x01,
    Joined = 0x02,
    Rejoining = 0x03,
    Leaving = 0x04,
    Error = 0xFF,
};

enum class DeviceStatus : uint8_t {
    Offline = 0x00,
    Online = 0x01,
    Sleepy = 0x02,
    Pending = 0x03,
    Unknown = 0xFF,
};

enum class ErrorCode : int32_t {
    Success = 0,
    GenericError = -1,
    InvalidParameter = -2,
    NotFound = -3,
    AlreadyExists = -4,
    NoMemory = -5,
    NotInitialized = -6,
    Timeout = -7,
    NoRoute = -8,
    BufferOverflow = -9,
    InvalidAddress = -10,
    ChannelBusy = -11,
    HardwareError = -12,
    SecurityError = -13,
    NotImplemented = -14,
    AuthFailed = -15,
    KeyNotFound = -16,
    CRCError = -17,
    InsufficientResources = -18,
    NotPermitted = -19,
};

enum class PacketPriority : uint8_t {
    Low = 0x00,
    Normal = 0x01,
    High = 0x02,
    Critical = 0x03,
};

enum class SecurityLevel : uint8_t {
    None = 0x00,
    MIC32 = 0x01,
    MIC64 = 0x02,
    MIC128 = 0x03,
    EncMIC32 = 0x04,
    EncMIC64 = 0x05,
    EncMIC128 = 0x06,
};

enum class FrameType : uint8_t {
    Beacon = 0x00,
    Data = 0x01,
    Ack = 0x02,
    Command = 0x03,
};

enum class NwkFrameType : uint8_t {
    Data = 0x00,
    Command = 0x01,
};

enum class NwkCommandId : uint8_t {
    RouteRequest = 0x01,
    RouteReply = 0x02,
    RouteError = 0x03,
    NetworkStatus = 0x04,
    Leave = 0x05,
    LeaveAck = 0x06,
    RequestKey = 0x08,
    SwitchKey = 0x09,
    KeyRequest = 0x0A,
    DeviceAnnounce = 0x0B,
    NetworkReport = 0x0C,
    NetworkUpdate = 0x0D,
};

enum class ApsFrameType : uint8_t {
    Data = 0x00,
    Command = 0x01,
    InterPan = 0x03,
};

enum class ApsCommandId : uint8_t {
    DataRequest = 0x00,
    DataConfirm = 0x01,
    DataIndication = 0x02,
    InterPanDataConfirm = 0x03,
    InterPanDataIndication = 0x04,
    Bind = 0x05,
    Unbind = 0x06,
    BindConfirm = 0x07,
    UnbindConfirm = 0x08,
    GetAttribute = 0x09,
    SetAttribute = 0x10,
};

struct MacAddress {
    ShortAddress short_addr{kBroadcastAddress};
    ExtendedAddress extended_addr{kExtendedBroadcastAddress};

    bool isShort() const {
        return extended_addr == kExtendedBroadcastAddress;
    }

    bool isExtended() const {
        return short_addr == kBroadcastAddress;
    }

    bool operator==(const MacAddress& other) const {
        return short_addr == other.short_addr &&
               extended_addr == other.extended_addr;
    }

    bool operator!=(const MacAddress& other) const {
        return !(*this == other);
    }

    std::string toString() const {
        if (isShort()) {
            return "0x" + shortAddrToString(short_addr);
        }
        return "0x" + extAddrToString(extended_addr);
    }

    static std::string shortAddrToString(ShortAddress addr) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%04X", addr);
        return buf;
    }

    static std::string extAddrToString(ExtendedAddress addr) {
        char buf[20];
        std::snprintf(buf, sizeof(buf), "%016llX",
                      static_cast<unsigned long long>(addr));
        return buf;
    }
};

struct DeviceInfo {
    MacAddress address;
    DeviceRole role{DeviceRole::Unknown};
    DeviceStatus status{DeviceStatus::Unknown};
    uint8_t lqi{0};
    int8_t rssi{0};
    uint8_t depth{0};
    uint16_t parent_addr{0};
    uint32_t last_seen_ms{0};
    bool is_relay{false};
};

struct NetworkInfo {
    PanId pan_id{0};
    Channel channel{kMinChannel};
    ShortAddress short_addr{0};
    ExtendedAddress extended_addr{0};
    uint8_t nwk_update_id{0};
    uint8_t nwk_frame_counter{0};
    DeviceRole role{DeviceRole::Unknown};
    NwkState state{NwkState::OffNetwork};
    uint8_t tx_power{0};
    int8_t max_rssi{-128};
};

struct RouteEntry {
    ShortAddress destination{0};
    ShortAddress next_hop{0};
    uint8_t metric{0};
    uint8_t age{0};
    bool active{false};
    bool concentrator{false};
};

struct NeighborEntry {
    MacAddress address;
    uint8_t lqi{0};
    int8_t rssi{0};
    uint8_t depth{0};
    bool incoming_fc{false};
    bool outgoing_fc{false};
    uint32_t frame_counter{0};
    uint8_t age{0};
};

constexpr inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success: return "Success";
        case ErrorCode::GenericError: return "GenericError";
        case ErrorCode::InvalidParameter: return "InvalidParameter";
        case ErrorCode::NotFound: return "NotFound";
        case ErrorCode::AlreadyExists: return "AlreadyExists";
        case ErrorCode::NoMemory: return "NoMemory";
        case ErrorCode::NotInitialized: return "NotInitialized";
        case ErrorCode::Timeout: return "Timeout";
        case ErrorCode::NoRoute: return "NoRoute";
        case ErrorCode::BufferOverflow: return "BufferOverflow";
        case ErrorCode::InvalidAddress: return "InvalidAddress";
        case ErrorCode::ChannelBusy: return "ChannelBusy";
        case ErrorCode::HardwareError: return "HardwareError";
        case ErrorCode::SecurityError: return "SecurityError";
        case ErrorCode::NotImplemented: return "NotImplemented";
        case ErrorCode::AuthFailed: return "AuthFailed";
        case ErrorCode::KeyNotFound: return "KeyNotFound";
        case ErrorCode::CRCError: return "CRCError";
        case ErrorCode::InsufficientResources: return "InsufficientResources";
        case ErrorCode::NotPermitted: return "NotPermitted";
        default: return "Unknown";
    }
}

constexpr inline const char* deviceRoleToString(DeviceRole role) {
    switch (role) {
        case DeviceRole::Coordinator: return "Coordinator";
        case DeviceRole::Router: return "Router";
        case DeviceRole::EndDevice: return "EndDevice";
        default: return "Unknown";
    }
}

constexpr inline const char* nwkStateToString(NwkState state) {
    switch (state) {
        case NwkState::OffNetwork: return "OffNetwork";
        case NwkState::Joining: return "Joining";
        case NwkState::Joined: return "Joined";
        case NwkState::Rejoining: return "Rejoining";
        case NwkState::Leaving: return "Leaving";
        case NwkState::Error: return "Error";
        default: return "Unknown";
    }
}

} // namespace zigbee_mesh::core
