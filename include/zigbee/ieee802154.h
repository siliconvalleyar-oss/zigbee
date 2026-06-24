#pragma once

#include "core/types.h"
#include "core/bytebuffer.h"
#include "core/event.h"
#include "drivers/hal.h"
#include <memory>
#include <vector>
#include <map>
#include <functional>

namespace zigbee_mesh::zigbee {

struct Ieee802154Frame {
    core::FrameType frame_type;
    uint8_t sequence_number{0};
    core::PanId dest_pan_id{core::kBroadcastAddress};
    core::ShortAddress dest_addr{core::kBroadcastAddress};
    core::PanId src_pan_id{0};
    core::ShortAddress src_addr{0};
    core::ExtendedAddress src_extended_addr{0};
    bool ack_requested{false};
    bool security_enabled{false};
    uint8_t security_level{0};
    uint32_t frame_counter{0};
    std::vector<uint8_t> payload;
    uint8_t fcs{0};

    core::ByteBuffer serialize() const;
    static bool deserialize(const core::ByteBuffer& data, Ieee802154Frame& frame);
    size_t getSerializedSize() const;
};

struct NwkHeader {
    core::NwkFrameType frame_type{core::NwkFrameType::Data};
    uint8_t protocol_version{2};
    bool discover_route{false};
    bool multicast{false};
    bool security{false};
    bool src_route{false};
    uint8_t reserved{0};
    core::ShortAddress dest_addr{0};
    core::ShortAddress src_addr{0};
    uint8_t radius{0};
    uint8_t sequence_number{0};
    uint32_t src_extended_addr{0};

    core::ByteBuffer serialize() const;
    static bool deserialize(const core::ByteBuffer& data, NwkHeader& header);
};

struct NwkCommand {
    core::NwkCommandId command_id;
    core::ShortAddress src_addr{0};
    core::ShortAddress dest_addr{0};
    std::vector<uint8_t> payload;

    core::ByteBuffer serialize() const;
    static bool deserialize(const core::ByteBuffer& data, NwkCommand& cmd);
};

struct ApsHeader {
    core::ApsFrameType frame_type{core::ApsFrameType::Data};
    bool delivery_mode{false};
    bool security{false};
    bool acknowledged{false};
    uint8_t endpoint{0};
    uint16_t cluster_id{0};
    uint16_t profile_id{0};
    core::ShortAddress group_id{0};
    core::ShortAddress src_addr{0};
    uint8_t counter{0};

    core::ByteBuffer serialize() const;
    static bool deserialize(const core::ByteBuffer& data, ApsHeader& header);
};

struct BeaconPayload {
    uint8_t protocol_id{0};
    uint8_t stack_profile{2};
    uint8_t nwk_update_id{0};
    bool beacon_type{false};
    uint8_t routing_cost{0};
    bool permit_joining{false};
    bool has_parent{false};
    bool has_child{false};
};

struct ChannelScanResult {
    uint8_t channel{0};
    bool energy_detected{false};
    bool pan_found{false};
    core::PanId pan_id{0};
    uint8_t lqi{0};
};

class Ieee802154Layer {
public:
    using FrameHandler = std::function<void(const Ieee802154Frame&, int8_t rssi, uint8_t lqi)>;

    explicit Ieee802154Layer(drivers::RadioDriver* radio);

    bool init(const core::NetworkInfo& info);
    bool sendFrame(const Ieee802154Frame& frame);
    void setFrameHandler(FrameHandler handler);

    bool scanChannels(uint16_t channel_mask, std::vector<ChannelScanResult>& results);
    bool associate(core::PanId pan_id, uint8_t channel);
    bool disassociate();

    void setRxOnWhenIdle(bool enabled);

    Ieee802154Frame createDataFrame(
        core::ShortAddress dest_addr,
        const uint8_t* payload, size_t len);

    Ieee802154Frame createCommandFrame(
        core::NwkCommandId cmd_id,
        core::ShortAddress dest_addr,
        const uint8_t* payload, size_t len);

private:
    void onRadioReceive(const core::ByteBuffer& frame, int8_t rssi, uint8_t lqi);
    bool processIncoming(const core::ByteBuffer& raw, Ieee802154Frame& parsed);
    uint8_t nextSequenceNumber();

    drivers::RadioDriver* radio_{nullptr};
    FrameHandler frame_handler_;
    core::NetworkInfo nwk_info_;
    uint8_t seq_num_{0};
    bool initialized_{false};
};

} // namespace zigbee_mesh::zigbee
