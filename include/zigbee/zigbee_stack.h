#pragma once

#include "ieee802154.h"
#include "security_manager.h"
#include "zdo_layer.h"
#include "core/config.h"
#include "core/logger.h"
#include "core/event.h"
#include "core/timer.h"
#include "drivers/hal.h"
#include <memory>
#include <map>
#include <vector>
#include <functional>

namespace zigbee_mesh::zigbee {

enum class StackState : uint8_t {
    Uninitialized = 0,
    Initializing = 1,
    Idle = 2,
    Scanning = 3,
    Joining = 4,
    Joined = 5,
    Rejoining = 6,
    Leaving = 7,
    Error = 0xFF,
};

struct DeviceTableEntry {
    core::ExtendedAddress ieee_addr{0};
    core::ShortAddress nwk_addr{0};
    DeviceRole role{DeviceRole::Unknown};
    uint8_t capability{0};
    uint8_t depth{0};
    bool authenticated{false};
    uint8_t lqi{0};
    int8_t rssi{0};
    uint32_t last_seen{0};
    core::DeviceStatus status{core::DeviceStatus::Unknown};
};

struct EndpointDesc {
    uint8_t endpoint{0};
    uint16_t profile_id{0};
    std::vector<uint16_t> input_clusters;
    std::vector<uint16_t> output_clusters;
};

struct BindingTableEntry {
    core::ExtendedAddress src_addr{0};
    uint8_t src_endpoint{0};
    uint16_t cluster_id{0};
    enum class DstType : uint8_t { Group = 0, Addr = 1 } dst_type;
    core::ExtendedAddress dst_addr{0};
    uint8_t dst_endpoint{0};
};

class ZigbeeStack {
public:
    using PacketHandler = std::function<void(const ApsHeader&, const core::ByteBuffer&, core::ShortAddress)>;
    using StateChangeHandler = std::function<void(StackState old_state, StackState new_state)>;

    ZigbeeStack();
    ~ZigbeeStack();

    bool init(drivers::RadioDriver* radio, const core::Config& config);
    bool start();
    void stop();

    bool formNetwork(uint8_t channel, PanId pan_id);
    bool joinNetwork(PanId pan_id, uint8_t channel, const uint8_t* nwk_key = nullptr);
    bool rejoinNetwork();
    bool leaveNetwork();

    bool sendPayload(core::ShortAddress dest, uint16_t cluster_id, uint8_t endpoint,
                     const core::ByteBuffer& payload, bool ack = true);
    bool sendGroupPayload(uint16_t group_id, uint16_t cluster_id,
                          const core::ByteBuffer& payload);

    bool registerEndpoint(uint8_t endpoint, uint16_t profile_id,
                          const std::vector<uint16_t>& input_clusters,
                          const std::vector<uint16_t>& output_clusters);
    bool deregisterEndpoint(uint8_t endpoint);

    bool bind(const core::ExtendedAddress& src_addr, uint8_t src_endpoint,
              uint16_t cluster_id, const core::ExtendedAddress& dst_addr,
              uint8_t dst_endpoint);
    bool unbind(const core::ExtendedAddress& src_addr, uint8_t src_endpoint,
                uint16_t cluster_id, const core::ExtendedAddress& dst_addr,
                uint8_t dst_endpoint);

    bool permitJoin(uint8_t duration);
    bool permitJoinOnRouter(core::ShortAddress router_addr, uint8_t duration);

    StackState getState() const { return state_; }
    core::NetworkInfo getNetworkInfo() const { return nwk_info_; }
    DeviceRole getRole() const { return role_; }

    std::vector<DeviceTableEntry> getDeviceTable() const;
    std::vector<BindingTableEntry> getBindingTable() const;
    std::vector<EndpointDesc> getEndpoints() const;

    void setPacketHandler(PacketHandler handler) { packet_handler_ = std::move(handler); }
    void setStateChangeHandler(StateChangeHandler handler) { state_handler_ = std::move(handler); }

    SecurityManager& securityManager() { return security_; }
    ZdoLayer& zdoLayer() { return zdo_; }

    bool isCoordinator() const { return role_ == DeviceRole::Coordinator; }
    bool isRouter() const { return role_ == DeviceRole::Router; }
    bool isEndDevice() const { return role_ == DeviceRole::EndDevice; }

    void setHeartbeatInterval(uint32_t ms) { heartbeat_interval_ms_ = ms; }

    core::ShortAddress getShortAddress() const { return nwk_info_.short_addr; }
    core::ExtendedAddress getExtendedAddress() const { return nwk_info_.extended_addr; }
    PanId getPanId() const { return nwk_info_.pan_id; }
    uint8_t getChannel() const { return nwk_info_.channel; }

private:
    void processFrame(const Ieee802154Frame& frame, int8_t rssi, uint8_t lqi);
    void processNwkData(const NwkHeader& header, const core::ByteBuffer& payload);
    void processNwkCommand(const NwkCommand& cmd);
    void processApsData(const ApsHeader& header, const core::ByteBuffer& payload);
    void handleDeviceAnnounce(const ZdoFrame& frame);

    void setState(StackState new_state);
    void startHeartbeat();
    void sendDeviceAnnounce();

    drivers::RadioDriver* radio_{nullptr};
    std::unique_ptr<Ieee802154Layer> ieee802154_;
    SecurityManager security_;
    ZdoLayer zdo_;
    core::EventEmitter emitter_;
    TimerManager timer_mgr_;
    core::NetworkInfo nwk_info_;
    StackState state_{StackState::Uninitialized};
    DeviceRole role_{DeviceRole::Unknown};

    std::map<uint8_t, EndpointDesc> endpoints_;
    std::vector<DeviceTableEntry> device_table_;
    std::vector<BindingTableEntry> binding_table_;

    PacketHandler packet_handler_;
    StateChangeHandler state_handler_;

    uint32_t heartbeat_interval_ms_{15000};
    bool initialized_{false};
};

} // namespace zigbee_mesh::zigbee
