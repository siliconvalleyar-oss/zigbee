#pragma once

#include "core/types.h"
#include "core/config.h"
#include "core/logger.h"
#include "core/event.h"
#include "zigbee/zigbee_stack.h"
#include "mesh/mesh_manager.h"
#include "network/routing.h"
#include "storage/storage_manager.h"
#include "security/security_manager.h"
#include <memory>
#include <vector>

namespace zigbee_mesh::services {

struct DiagnosticsInfo {
    uint32_t uptime_ms{0};
    uint32_t packets_sent{0};
    uint32_t packets_received{0};
    uint32_t packets_failed{0};
    uint32_t route_discoveries{0};
    uint32_t join_count{0};
    uint32_t leave_count{0};
    int8_t avg_rssi{0};
    uint8_t avg_lqi{0};
    size_t neighbor_count{0};
    size_t route_count{0};
    size_t device_count{0};
    size_t mesh_depth{0};
    float packet_delivery_ratio{0.0f};
};

struct OtaUpdateInfo {
    enum class Status { Idle, Downloading, Applying, Verifying, Completed, Failed };
    Status status{Status::Idle};
    std::string firmware_url;
    std::string current_version;
    std::string target_version;
    uint32_t progress{0};
    uint32_t total_size{0};
    std::string error_message;
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool init(const core::Config& config);
    bool start();
    void stop();

    bool createNetwork(uint8_t channel = 11, core::PanId pan_id = 0x1234);
    bool joinNetwork(core::PanId pan_id, uint8_t channel);
    bool leaveNetwork();
    bool scanChannels(uint16_t channel_mask = 0x07FFF800);
    bool scanNetworks();

    bool permitJoin(uint8_t duration = 60);

    core::NetworkInfo getNetworkInfo() const;
    std::vector<zigbee::DeviceTableEntry> getDevices() const;

    void setRole(core::DeviceRole role) { role_ = role; }
    core::DeviceRole getRole() const { return role_; }

    zigbee::ZigbeeStack& stack() { return *stack_; }

    bool isRunning() const { return running_; }

private:
    void handlePacket(const zigbee::ApsHeader& header,
                      const core::ByteBuffer& payload, core::ShortAddress src);

    std::unique_ptr<zigbee::ZigbeeStack> stack_;
    core::Config config_;
    core::DeviceRole role_{core::DeviceRole::Router};
    bool running_{false};
};

class MeshService {
public:
    MeshService() = default;

    bool init(routing::RoutingTable* routing);
    bool start();
    void stop();

    bool broadcastHeartbeat();
    bool sendPing(core::ShortAddress target);

    mesh::TopologySnapshot getTopology() const;
    std::string getAsciiTopology() const;
    bool isNodeReachable(core::ShortAddress addr) const;

    void updateNodeMetrics(core::ShortAddress addr, uint8_t lqi, int8_t rssi);

    mesh::MeshManager& manager() { return manager_; }

private:
    mesh::MeshManager manager_;
    bool running_{false};
};

class DiagnosticsManager {
public:
    DiagnosticsManager() = default;

    bool init(NetworkManager* network, MeshService* mesh);
    void reset();

    void recordPacketSent();
    void recordPacketReceived();
    void recordPacketFailed();
    void recordJoin();
    void recordLeave();
    void recordRouteDiscovery();

    DiagnosticsInfo getDiagnostics() const;
    std::string getDiagnosticsSummary() const;
    std::string getDetailedReport() const;

    void setUptimeStart() { start_time_ms_ = getCurrentTimeMs(); }

private:
    static uint32_t getCurrentTimeMs();

    NetworkManager* network_{nullptr};
    MeshService* mesh_{nullptr};
    uint32_t start_time_ms_{0};
    uint32_t packets_sent_{0};
    uint32_t packets_received_{0};
    uint32_t packets_failed_{0};
    uint32_t route_discoveries_{0};
    uint32_t join_count_{0};
    uint32_t leave_count_{0};
    mutable std::mutex mutex_;
};

class OtaUpdateManager {
public:
    OtaUpdateManager() = default;

    bool init(NetworkManager* network);
    bool startUpdate(const std::string& firmware_url, const std::string& target_version);
    bool cancelUpdate();
    OtaUpdateInfo getStatus() const;
    std::string getCurrentVersion() const;

private:
    void downloadFirmware(const std::string& url);
    void applyFirmware();
    bool verifyFirmware();
    bool writeFirmwareBlock(const std::vector<uint8_t>& data, uint32_t offset);

    NetworkManager* network_{nullptr};
    OtaUpdateInfo status_;
    mutable std::mutex mutex_;
    std::thread worker_thread_;
};

} // namespace zigbee_mesh::services
