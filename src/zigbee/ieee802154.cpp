#include "zigbee/ieee802154.h"
#include "core/logger.h"

namespace zigbee_mesh::zigbee {

core::ByteBuffer Ieee802154Frame::serialize() const {
    core::ByteBuffer buf;
    uint8_t frame_control_0 = 0;
    frame_control_0 |= (static_cast<uint8_t>(frame_type) & 0x03);
    frame_control_0 |= (ack_requested ? (1 << 5) : 0);
    frame_control_0 |= (security_enabled ? (1 << 3) : 0);

    uint8_t frame_control_1 = 0;
    frame_control_1 |= (security_enabled ? (1 << 6) : 0);

    buf.appendU8(frame_control_0);
    buf.appendU8(frame_control_1);
    buf.appendU8(sequence_number);

    if (frame_type == core::FrameType::Data || frame_type == core::FrameType::Command) {
        if (dest_pan_id != core::kBroadcastAddress) {
            buf.appendU16LE(dest_pan_id);
        }
        buf.appendU16LE(dest_addr);

        if (!security_enabled || src_pan_id == dest_pan_id) {
            buf.appendU16LE(src_addr);
        } else {
            buf.appendU16LE(src_pan_id);
            buf.appendU16LE(src_addr);
        }
    }

    if (security_enabled) {
        buf.appendU32LE(frame_counter);
    }

    buf.append(payload.data(), payload.size());
    return buf;
}

bool Ieee802154Frame::deserialize(const core::ByteBuffer& data, Ieee802154Frame& frame) {
    if (data.size() < 3) return false;

    size_t offset = 0;
    uint8_t fc0 = data.readU8(offset++);
    uint8_t fc1 = data.readU8(offset++);

    frame.frame_type = static_cast<core::FrameType>(fc0 & 0x03);
    frame.ack_requested = (fc0 >> 5) & 0x01;
    frame.security_enabled = (fc0 >> 3) & 0x01;

    frame.sequence_number = data.readU8(offset++);

    if (offset + 2 <= data.size()) {
        frame.dest_pan_id = data.readU16LE(offset);
        offset += 2;
    }
    if (offset + 2 <= data.size()) {
        frame.dest_addr = data.readU16LE(offset);
        offset += 2;
    }
    if (offset + 2 <= data.size()) {
        frame.src_addr = data.readU16LE(offset);
        offset += 2;
    }

    if (data.size() > offset) {
        frame.payload.assign(data.data() + offset, data.data() + data.size());
    }

    return true;
}

size_t Ieee802154Frame::getSerializedSize() const {
    return 3 + 2 + 2 + 2 + payload.size() + (security_enabled ? 4 : 0);
}

core::ByteBuffer NwkHeader::serialize() const {
    core::ByteBuffer buf;
    uint8_t fc = 0;
    fc |= (static_cast<uint8_t>(frame_type) & 0x01);
    fc |= (protocol_version & 0x0F) << 2;
    fc |= (discover_route ? (1 << 6) : 0);
    fc |= (security ? (1 << 7) : 0);
    buf.appendU8(fc);

    uint8_t fc2 = 0;
    fc2 |= (multicast ? 0x01 : 0);
    fc2 |= (src_route ? 0x02 : 0);
    buf.appendU8(fc2);

    buf.appendU16LE(dest_addr);
    buf.appendU16LE(src_addr);
    buf.appendU8(radius);
    buf.appendU8(sequence_number);

    return buf;
}

bool NwkHeader::deserialize(const core::ByteBuffer& data, NwkHeader& header) {
    if (data.size() < 9) return false;
    size_t offset = 0;
    uint8_t fc = data.readU8(offset++);
    header.frame_type = static_cast<core::NwkFrameType>(fc & 0x01);
    header.protocol_version = (fc >> 2) & 0x0F;
    header.discover_route = (fc >> 6) & 0x01;
    header.security = (fc >> 7) & 0x01;

    uint8_t fc2 = data.readU8(offset++);
    header.multicast = fc2 & 0x01;
    header.src_route = (fc2 >> 1) & 0x01;

    header.dest_addr = data.readU16LE(offset); offset += 2;
    header.src_addr = data.readU16LE(offset); offset += 2;
    header.radius = data.readU8(offset++);
    header.sequence_number = data.readU8(offset++);
    return true;
}

core::ByteBuffer NwkCommand::serialize() const {
    core::ByteBuffer buf;
    buf.appendU8(static_cast<uint8_t>(command_id));
    buf.append(payload.data(), payload.size());
    return buf;
}

bool NwkCommand::deserialize(const core::ByteBuffer& data, NwkCommand& cmd) {
    if (data.size() < 1) return false;
    cmd.command_id = static_cast<core::NwkCommandId>(data.readU8(0));
    if (data.size() > 1) {
        cmd.payload.assign(data.data() + 1, data.data() + data.size());
    }
    return true;
}

core::ByteBuffer ApsHeader::serialize() const {
    core::ByteBuffer buf;
    uint8_t fc = 0;
    fc |= (static_cast<uint8_t>(frame_type) & 0x03);
    fc |= (delivery_mode ? (1 << 2) : 0);
    fc |= (security ? (1 << 3) : 0);
    fc |= (acknowledged ? (1 << 5) : 0);
    buf.appendU8(fc);
    buf.appendU8(endpoint);
    buf.appendU16LE(cluster_id);
    buf.appendU16LE(profile_id);
    buf.appendU16LE(group_id);
    buf.appendU16LE(src_addr);
    buf.appendU8(counter);
    return buf;
}

bool ApsHeader::deserialize(const core::ByteBuffer& data, ApsHeader& header) {
    if (data.size() < 10) return false;
    size_t offset = 0;
    uint8_t fc = data.readU8(offset++);
    header.frame_type = static_cast<core::ApsFrameType>(fc & 0x03);
    header.delivery_mode = (fc >> 2) & 0x01;
    header.security = (fc >> 3) & 0x01;
    header.acknowledged = (fc >> 5) & 0x01;

    header.endpoint = data.readU8(offset++);
    header.cluster_id = data.readU16LE(offset); offset += 2;
    header.profile_id = data.readU16LE(offset); offset += 2;
    header.group_id = data.readU16LE(offset); offset += 2;
    header.src_addr = data.readU16LE(offset); offset += 2;
    header.counter = data.readU8(offset++);
    return true;
}

Ieee802154Layer::Ieee802154Layer(drivers::RadioDriver* radio) : radio_(radio) {}

bool Ieee802154Layer::init(const core::NetworkInfo& info) {
    nwk_info_ = info;
    initialized_ = true;
    ZIGBEE_LOG_INFO("IEEE 802.15.4 layer initialized, PAN: 0x%04X, CH: %d",
                     nwk_info_.pan_id, nwk_info_.channel);
    return true;
}

void Ieee802154Layer::setFrameHandler(FrameHandler handler) {
    frame_handler_ = std::move(handler);
}

bool Ieee802154Layer::sendFrame(const Ieee802154Frame& frame) {
    if (!radio_ || !radio_->isOpen()) return false;
    core::ByteBuffer serialized = frame.serialize();
    return radio_->send(serialized);
}

Ieee802154Frame Ieee802154Layer::createDataFrame(
    core::ShortAddress dest_addr, const uint8_t* payload, size_t len) {
    Ieee802154Frame frame;
    frame.frame_type = core::FrameType::Data;
    frame.sequence_number = nextSequenceNumber();
    frame.dest_pan_id = nwk_info_.pan_id;
    frame.dest_addr = dest_addr;
    frame.src_pan_id = nwk_info_.pan_id;
    frame.src_addr = nwk_info_.short_addr;
    frame.src_extended_addr = nwk_info_.extended_addr;
    frame.payload.assign(payload, payload + len);
    return frame;
}

Ieee802154Frame Ieee802154Layer::createCommandFrame(
    core::NwkCommandId cmd_id, core::ShortAddress dest_addr,
    const uint8_t* payload, size_t len) {
    Ieee802154Frame frame;
    frame.frame_type = core::FrameType::Command;
    frame.sequence_number = nextSequenceNumber();
    frame.dest_pan_id = nwk_info_.pan_id;
    frame.dest_addr = dest_addr;
    frame.src_pan_id = nwk_info_.pan_id;
    frame.src_addr = nwk_info_.short_addr;
    NwkCommand cmd;
    cmd.command_id = cmd_id;
    cmd.src_addr = nwk_info_.short_addr;
    cmd.dest_addr = dest_addr;
    if (payload && len > 0) {
        cmd.payload.assign(payload, payload + len);
    }
    core::ByteBuffer cmd_buf = cmd.serialize();
    frame.payload.assign(cmd_buf.data(), cmd_buf.data() + cmd_buf.size());
    return frame;
}

void Ieee802154Layer::onRadioReceive(const core::ByteBuffer& frame, int8_t rssi, uint8_t lqi) {
    Ieee802154Frame parsed;
    if (processIncoming(frame, parsed) && frame_handler_) {
        frame_handler_(parsed, rssi, lqi);
    }
}

bool Ieee802154Layer::processIncoming(const core::ByteBuffer& raw, Ieee802154Frame& parsed) {
    return Ieee802154Frame::deserialize(raw, parsed);
}

uint8_t Ieee802154Layer::nextSequenceNumber() {
    return seq_num_++;
}

bool Ieee802154Layer::scanChannels(uint16_t channel_mask, std::vector<ChannelScanResult>& results) {
    if (!radio_) return false;
    for (uint8_t ch = 11; ch <= 26; ++ch) {
        if (!(channel_mask & (1 << (ch - 11)))) continue;
        radio_->setChannel(ch);
        ChannelScanResult result;
        result.channel = ch;
        result.energy_detected = false;
        result.pan_found = false;
        result.pan_id = 0;
        result.lqi = 0;
        results.push_back(result);
    }
    return true;
}

bool Ieee802154Layer::associate(core::PanId pan_id, uint8_t channel) {
    radio_->setChannel(channel);
    radio_->setPanId(pan_id);
    return true;
}

bool Ieee802154Layer::disassociate() {
    return true;
}

void Ieee802154Layer::setRxOnWhenIdle(bool enabled) {
    (void)enabled;
}

} // namespace zigbee_mesh::zigbee
