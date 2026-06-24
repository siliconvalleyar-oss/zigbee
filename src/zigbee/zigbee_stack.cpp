#include "zigbee/zigbee_stack.h"
#include "core/logger.h"
#include <algorithm>

namespace zigbee_mesh::zigbee {

ZigbeeStack::ZigbeeStack() = default;
ZigbeeStack::~ZigbeeStack() { stop(); }

bool ZigbeeStack::init(drivers::RadioDriver* radio, const core::Config& config) {
    radio_ = radio;
    ieee802154_ = std::make_unique<Ieee802154Layer>(radio);
    security_.init(config);

    nwk_info_.extended_addr = config.get<uint64_t>("device", "ieee_address", 0x0000000000000000ULL);
    nwk_info_.pan_id = config.get<uint16_t>("network", "pan_id", 0x1234);
    nwk_info_.channel = config.get<uint8_t>("network", "channel", 11);
    role_ = static_cast<core::DeviceRole>(config.get<int>("device", "role", 2));

    ieee802154_->setFrameHandler(
        [this](const Ieee802154Frame& frame, int8_t rssi, uint8_t lqi) {
            processFrame(frame, rssi, lqi);
        });

    initialized_ = true;
    setState(StackState::Idle);
    ZIGBEE_LOG_INFO("ZigbeeStack initialized, role=%s, PAN=0x%04X",
                     core::deviceRoleToString(role_), nwk_info_.pan_id);
    return true;
}

bool ZigbeeStack::start() {
    if (!initialized_) return false;
    core::NetworkInfo info = nwk_info_;
    ieee802154_->init(info);
    zdo_.init();
    startHeartbeat();
    sendDeviceAnnounce();
    setState(StackState::Joined);
    return true;
}

void ZigbeeStack::stop() {
    timer_mgr_.cancelAll();
    setState(StackState::Uninitialized);
    initialized_ = false;
}

bool ZigbeeStack::formNetwork(uint8_t channel, core::PanId pan_id) {
    role_ = core::DeviceRole::Coordinator;
    nwk_info_.channel = channel;
    nwk_info_.pan_id = pan_id;
    nwk_info_.role = role_;
    nwk_info_.short_addr = 0x0000;
    radio_->setChannel(channel);
    security_.generateNetworkKey();
    setState(StackState::Joined);
    ZIGBEE_LOG_INFO("Network formed: PAN=0x%04X, CH=%d", pan_id, channel);
    return true;
}

bool ZigbeeStack::joinNetwork(core::PanId pan_id, uint8_t channel, const uint8_t* nwk_key) {
    role_ = core::DeviceRole::Router;
    nwk_info_.channel = channel;
    nwk_info_.pan_id = pan_id;
    nwk_info_.role = role_;
    radio_->setChannel(channel);
    if (nwk_key) security_.setNetworkKey(nwk_key);
    setState(StackState::Joined);
    ZIGBEE_LOG_INFO("Joined network: PAN=0x%04X, CH=%d", pan_id, channel);
    return true;
}

bool ZigbeeStack::rejoinNetwork() {
    setState(StackState::Rejoining);
    setState(StackState::Joined);
    return true;
}

bool ZigbeeStack::leaveNetwork() {
    setState(StackState::Leaving);
    device_table_.clear();
    binding_table_.clear();
    setState(StackState::Idle);
    ZIGBEE_LOG_INFO("Left network");
    return true;
}

bool ZigbeeStack::sendPayload(core::ShortAddress dest, uint16_t cluster_id, uint8_t endpoint,
                               const core::ByteBuffer& payload, bool ack) {
    (void)ack;
    ApsHeader header;
    header.frame_type = ApsFrameType::Data;
    header.endpoint = endpoint;
    header.cluster_id = cluster_id;
    header.profile_id = 0x0104;
    header.src_addr = nwk_info_.short_addr;
    header.counter = 0;

    core::ByteBuffer aps_buf = header.serialize();
    core::ByteBuffer full_payload;
    full_payload.append(aps_buf);
    full_payload.append(payload);

    Ieee802154Frame frame = ieee802154_->createDataFrame(
        dest, full_payload.data(), full_payload.size());
    return ieee802154_->sendFrame(frame);
}

bool ZigbeeStack::sendGroupPayload(uint16_t group_id, uint16_t cluster_id,
                                    const core::ByteBuffer& payload) {
    ApsHeader header;
    header.frame_type = ApsFrameType::Data;
    header.group_id = group_id;
    header.cluster_id = cluster_id;
    header.profile_id = 0x0104;
    header.src_addr = nwk_info_.short_addr;

    core::ByteBuffer aps_buf = header.serialize();
    core::ByteBuffer full_payload;
    full_payload.append(aps_buf);
    full_payload.append(payload);

    Ieee802154Frame frame = ieee802154_->createDataFrame(
        0xFFFC, full_payload.data(), full_payload.size());
    return ieee802154_->sendFrame(frame);
}

bool ZigbeeStack::registerEndpoint(uint8_t endpoint, uint16_t profile_id,
                                    const std::vector<uint16_t>& input_clusters,
                                    const std::vector<uint16_t>& output_clusters) {
    EndpointDesc desc;
    desc.endpoint = endpoint;
    desc.profile_id = profile_id;
    desc.input_clusters = input_clusters;
    desc.output_clusters = output_clusters;
    endpoints_[endpoint] = std::move(desc);
    return true;
}

bool ZigbeeStack::deregisterEndpoint(uint8_t endpoint) {
    return endpoints_.erase(endpoint) > 0;
}

bool ZigbeeStack::bind(const core::ExtendedAddress& src_addr, uint8_t src_endpoint,
                        uint16_t cluster_id, const core::ExtendedAddress& dst_addr,
                        uint8_t dst_endpoint) {
    BindingTableEntry entry;
    entry.src_addr = src_addr;
    entry.src_endpoint = src_endpoint;
    entry.cluster_id = cluster_id;
    entry.dst_type = BindingTableEntry::DstType::Addr;
    entry.dst_addr = dst_addr;
    entry.dst_endpoint = dst_endpoint;
    binding_table_.push_back(entry);
    return true;
}

bool ZigbeeStack::unbind(const core::ExtendedAddress& src_addr, uint8_t src_endpoint,
                          uint16_t cluster_id, const core::ExtendedAddress& dst_addr,
                          uint8_t dst_endpoint) {
    auto it = std::remove_if(binding_table_.begin(), binding_table_.end(),
        [&](const BindingTableEntry& e) {
            return e.src_addr == src_addr && e.src_endpoint == src_endpoint &&
                   e.cluster_id == cluster_id && e.dst_addr == dst_addr &&
                   e.dst_endpoint == dst_endpoint;
        });
    binding_table_.erase(it, binding_table_.end());
    return true;
}

bool ZigbeeStack::permitJoin(uint8_t duration) {
    return zdo_.mgmtPermitJoinRequest(0x0000, duration, false);
}

bool ZigbeeStack::permitJoinOnRouter(core::ShortAddress router_addr, uint8_t duration) {
    return zdo_.mgmtPermitJoinRequest(router_addr, duration, false);
}

std::vector<DeviceTableEntry> ZigbeeStack::getDeviceTable() const {
    return device_table_;
}

std::vector<BindingTableEntry> ZigbeeStack::getBindingTable() const {
    return binding_table_;
}

std::vector<EndpointDesc> ZigbeeStack::getEndpoints() const {
    std::vector<EndpointDesc> result;
    for (const auto& [ep, desc] : endpoints_) {
        result.push_back(desc);
    }
    return result;
}

void ZigbeeStack::processFrame(const Ieee802154Frame& frame, int8_t rssi, uint8_t lqi) {
    NwkHeader nwk_header;
    core::ByteBuffer nwk_payload(frame.payload.data(), frame.payload.size());

    if (!NwkHeader::deserialize(nwk_payload, nwk_header)) return;

    if (frame.frame_type == core::FrameType::Command) {
        NwkCommand cmd;
        if (NwkCommand::deserialize(nwk_payload, cmd)) {
            processNwkCommand(cmd);
        }
        return;
    }

    auto it = device_table_.begin();
    for (; it != device_table_.end(); ++it) {
        if (it->nwk_addr == frame.src_addr) {
            it->lqi = lqi;
            it->rssi = rssi;
            it->last_seen = static_cast<uint32_t>(
                std::chrono::steady_clock::now().time_since_epoch().count());
            it->status = core::DeviceStatus::Online;
            break;
        }
    }

    processNwkData(nwk_header, nwk_payload);
}

void ZigbeeStack::processNwkData(const NwkHeader& header, const core::ByteBuffer& payload) {
    size_t nwk_header_size = 9;
    if (payload.size() <= nwk_header_size) return;

    ApsHeader aps_header;
    core::ByteBuffer aps_payload(payload.data() + nwk_header_size, payload.size() - nwk_header_size);

    if (ApsHeader::deserialize(aps_payload, aps_header)) {
        if (packet_handler_) {
            size_t aps_header_size = 10;
            if (aps_payload.size() > aps_header_size) {
                core::ByteBuffer data(aps_payload.data() + aps_header_size,
                                       aps_payload.size() - aps_header_size);
                packet_handler_(aps_header, data, header.src_addr);
            }
        }
    }
}

void ZigbeeStack::processNwkCommand(const NwkCommand& cmd) {
    switch (cmd.command_id) {
        case NwkCommandId::DeviceAnnounce:
            handleDeviceAnnounce(ZdoFrame{});
            break;
        default:
            break;
    }
}

void ZigbeeStack::handleDeviceAnnounce(const ZdoFrame& frame) {
    DeviceTableEntry entry;
    entry.nwk_addr = frame.src_addr;
    entry.status = core::DeviceStatus::Online;
    entry.last_seen = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());

    auto it = std::find_if(device_table_.begin(), device_table_.end(),
        [&](const DeviceTableEntry& e) { return e.nwk_addr == frame.src_addr; });

    if (it != device_table_.end()) {
        *it = entry;
    } else {
        device_table_.push_back(entry);
        core::DeviceEvent event;
        event.type = core::EventType::DeviceJoined;
        event.device_address.short_addr = frame.src_addr;
        event.source_addr = frame.src_addr;
        emitter_.emitAsync(std::make_unique<core::DeviceEvent>(event));
    }
}

void ZigbeeStack::setState(StackState new_state) {
    StackState old = state_;
    state_ = new_state;
    nwk_info_.state = static_cast<core::NwkState>(static_cast<uint8_t>(new_state));
    if (state_handler_) {
        state_handler_(old, new_state);
    }
    ZIGBEE_LOG_INFO("Stack state: %d -> %d", static_cast<int>(old), static_cast<int>(new_state));
}

void ZigbeeStack::startHeartbeat() {
    timer_mgr_.scheduleInterval(
        std::chrono::milliseconds(heartbeat_interval_ms_),
        [this]() {
            sendDeviceAnnounce();
        });
}

void ZigbeeStack::sendDeviceAnnounce() {
    zdo_.deviceAnnce(nwk_info_.short_addr, nwk_info_.extended_addr, 0x00);
}

} // namespace zigbee_mesh::zigbee
