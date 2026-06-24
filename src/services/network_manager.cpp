#include "services/network_manager.h"
#include "core/logger.h"

namespace zigbee_mesh::services {

NetworkManager::NetworkManager() = default;
NetworkManager::~NetworkManager() { stop(); }

bool NetworkManager::init(const core::Config& config) {
    config_ = config;
    stack_ = std::make_unique<zigbee::ZigbeeStack>();
    return true;
}

bool NetworkManager::start() {
    if (!stack_) return false;
    stack_->setPacketHandler(
        [this](const zigbee::ApsHeader& h, const core::ByteBuffer& p, core::ShortAddress s) {
            handlePacket(h, p, s);
        });
    running_ = true;
    ZIGBEE_LOG_INFO("Network manager started, role=%s", core::deviceRoleToString(role_));
    return true;
}

void NetworkManager::stop() {
    running_ = false;
    if (stack_) stack_->stop();
}

bool NetworkManager::createNetwork(uint8_t channel, core::PanId pan_id) {
    if (!stack_) return false;
    role_ = core::DeviceRole::Coordinator;
    stack_->setRole(role_);
    return stack_->formNetwork(channel, pan_id);
}

bool NetworkManager::joinNetwork(core::PanId pan_id, uint8_t channel) {
    if (!stack_) return false;
    role_ = core::DeviceRole::Router;
    stack_->setRole(role_);
    return stack_->joinNetwork(pan_id, channel);
}

bool NetworkManager::leaveNetwork() {
    return stack_ ? stack_->leaveNetwork() : false;
}

bool NetworkManager::permitJoin(uint8_t duration) {
    return stack_ ? stack_->permitJoin(duration) : false;
}

core::NetworkInfo NetworkManager::getNetworkInfo() const {
    return stack_ ? stack_->getNetworkInfo() : core::NetworkInfo{};
}

std::vector<zigbee::DeviceTableEntry> NetworkManager::getDevices() const {
    return stack_ ? stack_->getDeviceTable() : std::vector<zigbee::DeviceTableEntry>{};
}

void NetworkManager::handlePacket(const zigbee::ApsHeader& header,
                                   const core::ByteBuffer& payload, core::ShortAddress src) {
    (void)header;
    (void)payload;
    (void)src;
}

MeshService::MeshService() = default;

bool MeshService::init(routing::RoutingTable* routing) {
    return manager_.init(routing);
}

bool MeshService::start() { running_ = true; return true; }
void MeshService::stop() { running_ = false; }

bool MeshService::broadcastHeartbeat() { return true; }
bool MeshService::sendPing(core::ShortAddress target) {
    (void)target;
    return true;
}

mesh::TopologySnapshot MeshService::getTopology() const { return manager_.getTopology(); }
std::string MeshService::getAsciiTopology() const { return manager_.generateAsciiTopology(); }
bool MeshService::isNodeReachable(core::ShortAddress addr) const { return manager_.isNodeReachable(addr); }
void MeshService::updateNodeMetrics(core::ShortAddress addr, uint8_t lqi, int8_t rssi) {
    manager_.updateNode(addr, lqi, rssi);
}

bool DiagnosticsManager::init(NetworkManager* network, MeshService* mesh) {
    network_ = network;
    mesh_ = mesh;
    setUptimeStart();
    return true;
}

void DiagnosticsManager::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    packets_sent_ = packets_received_ = packets_failed_ = 0;
    route_discoveries_ = join_count_ = leave_count_ = 0;
}

void DiagnosticsManager::recordPacketSent() { std::lock_guard<std::mutex> lock(mutex_); packets_sent_++; }
void DiagnosticsManager::recordPacketReceived() { std::lock_guard<std::mutex> lock(mutex_); packets_received_++; }
void DiagnosticsManager::recordPacketFailed() { std::lock_guard<std::mutex> lock(mutex_); packets_failed_++; }
void DiagnosticsManager::recordJoin() { std::lock_guard<std::mutex> lock(mutex_); join_count_++; }
void DiagnosticsManager::recordLeave() { std::lock_guard<std::mutex> lock(mutex_); leave_count_++; }
void DiagnosticsManager::recordRouteDiscovery() { std::lock_guard<std::mutex> lock(mutex_); route_discoveries_++; }

DiagnosticsInfo DiagnosticsManager::getDiagnostics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    DiagnosticsInfo info;
    info.uptime_ms = getCurrentTimeMs() - start_time_ms_;
    info.packets_sent = packets_sent_;
    info.packets_received = packets_received_;
    info.packets_failed = packets_failed_;
    info.route_discoveries = route_discoveries_;
    info.join_count = join_count_;
    info.leave_count = leave_count_;
    if (mesh_) {
        info.device_count = mesh_->manager().getNodeCount();
        info.route_count = mesh_->manager().getLinkCount();
    }
    uint32_t total = info.packets_sent + info.packets_received;
    info.packet_delivery_ratio = total > 0 ?
        static_cast<float>(info.packets_received) / static_cast<float>(total) * 100.0f : 0.0f;
    return info;
}

std::string DiagnosticsManager::getDiagnosticsSummary() const {
    auto info = getDiagnostics();
    char buf[512];
    snprintf(buf, sizeof(buf),
        "Uptime: %us | Sent: %u | Recv: %u | Failed: %u | PDR: %.1f%% | Devices: %zu | Routes: %zu",
        info.uptime_ms / 1000, info.packets_sent, info.packets_received,
        info.packets_failed, info.packet_delivery_ratio,
        info.device_count, info.route_count);
    return buf;
}

std::string DiagnosticsManager::getDetailedReport() const {
    auto info = getDiagnostics();
    std::string r;
    r += "=== Diagnostics Report ===\n";
    char buf[256];
    snprintf(buf, sizeof(buf), "Uptime: %u seconds\n", info.uptime_ms / 1000);
    r += buf;
    snprintf(buf, sizeof(buf), "Packets Sent: %u\n", info.packets_sent);
    r += buf;
    snprintf(buf, sizeof(buf), "Packets Received: %u\n", info.packets_received);
    r += buf;
    snprintf(buf, sizeof(buf), "Packets Failed: %u\n", info.packets_failed);
    r += buf;
    snprintf(buf, sizeof(buf), "PDR: %.1f%%\n", info.packet_delivery_ratio);
    r += buf;
    snprintf(buf, sizeof(buf), "Route Discoveries: %u\n", info.route_discoveries);
    r += buf;
    snprintf(buf, sizeof(buf), "Join Count: %u\n", info.join_count);
    r += buf;
    snprintf(buf, sizeof(buf), "Leave Count: %u\n", info.leave_count);
    r += buf;
    snprintf(buf, sizeof(buf), "Devices: %zu\n", info.device_count);
    r += buf;
    snprintf(buf, sizeof(buf), "Routes: %zu\n", info.route_count);
    r += buf;
    return r;
}

uint32_t DiagnosticsManager::getCurrentTimeMs() {
    return static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
}

bool OtaUpdateManager::init(NetworkManager* network) {
    network_ = network;
    return true;
}

bool OtaUpdateManager::startUpdate(const std::string& firmware_url, const std::string& target_version) {
    std::lock_guard<std::mutex> lock(mutex_);
    status_.status = OtaUpdateInfo::Status::Downloading;
    status_.firmware_url = firmware_url;
    status_.target_version = target_version;
    status_.progress = 0;
    ZIGBEE_LOG_INFO("OTA update started: %s -> %s", status_.current_version.c_str(), target_version.c_str());
    return true;
}

bool OtaUpdateManager::cancelUpdate() {
    std::lock_guard<std::mutex> lock(mutex_);
    status_.status = OtaUpdateInfo::Status::Idle;
    return true;
}

OtaUpdateInfo OtaUpdateManager::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

std::string OtaUpdateManager::getCurrentVersion() const {
    return status_.current_version;
}

void OtaUpdateManager::downloadFirmware(const std::string& url) {
    (void)url;
}

void OtaUpdateManager::applyFirmware() {}

bool OtaUpdateManager::verifyFirmware() { return true; }

bool OtaUpdateManager::writeFirmwareBlock(const std::vector<uint8_t>& data, uint32_t offset) {
    (void)data;
    (void)offset;
    return true;
}

} // namespace zigbee_mesh::services
