#pragma once

#include "core/types.h"
#include "core/config.h"
#include "core/logger.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

namespace zigbee_mesh::storage {

struct StoredDeviceInfo {
    core::ExtendedAddress ieee_addr{0};
    core::ShortAddress nwk_addr{0};
    core::DeviceRole role{core::DeviceRole::Unknown};
    uint8_t depth{0};
    core::ShortAddress parent_addr{0};
    bool authenticated{false};
    std::string name;
    std::string description;
    uint32_t first_seen{0};
    uint32_t last_seen{0};
};

struct StoredNetworkInfo {
    core::PanId pan_id{0};
    core::Channel channel{11};
    uint16_t short_addr{0};
    uint64_t extended_addr{0};
    uint8_t nwk_update_id{0};
    core::DeviceRole role{core::DeviceRole::Unknown};
    std::vector<uint8_t> network_key;
};

struct StoredTopologyNode {
    core::ShortAddress addr{0};
    core::ShortAddress parent{0};
    uint8_t depth{0};
    uint8_t lqi{0};
    int8_t rssi{0};
    uint32_t last_updated{0};
};

struct StoredRoute {
    core::ShortAddress destination{0};
    core::ShortAddress next_hop{0};
    uint8_t hop_count{0};
    uint16_t cost{0};
    uint32_t age{0};
    bool active{false};
};

class StorageManager {
public:
    StorageManager() = default;

    bool open(const std::string& db_path);
    void close();
    bool isOpen() const { return db_open_; }

    bool saveDevice(const StoredDeviceInfo& device);
    bool updateDevice(const StoredDeviceInfo& device);
    bool removeDevice(core::ExtendedAddress ieee_addr);
    bool getDevice(core::ExtendedAddress ieee_addr, StoredDeviceInfo& device) const;
    std::vector<StoredDeviceInfo> getAllDevices() const;
    bool deviceExists(core::ExtendedAddress ieee_addr) const;

    bool saveNetworkInfo(const StoredNetworkInfo& info);
    bool getNetworkInfo(StoredNetworkInfo& info) const;
    bool removeNetworkInfo();

    bool saveTopologyNode(const StoredTopologyNode& node);
    bool removeTopologyNode(core::ShortAddress addr);
    std::vector<StoredTopologyNode> getAllTopologyNodes() const;
    bool clearTopology();

    bool saveRoute(const StoredRoute& route);
    bool removeRoute(core::ShortAddress destination);
    std::vector<StoredRoute> getAllRoutes() const;
    bool clearRoutes();

    bool saveAttribute(core::ExtendedAddress ieee_addr, uint8_t endpoint,
                       uint16_t cluster_id, uint16_t attr_id, const std::vector<uint8_t>& value);
    bool getAttribute(core::ExtendedAddress ieee_addr, uint8_t endpoint,
                      uint16_t cluster_id, uint16_t attr_id, std::vector<uint8_t>& value) const;

    bool saveBinding(core::ExtendedAddress src_addr, uint8_t src_endpoint,
                     uint16_t cluster_id, core::ExtendedAddress dst_addr, uint8_t dst_endpoint);
    bool removeBinding(core::ExtendedAddress src_addr, uint8_t src_endpoint, uint16_t cluster_id);

    bool vacuum();
    bool backup(const std::string& backup_path);
    bool restore(const std::string& backup_path);

private:
    bool createTables();
    bool ensureSchema();

    void* db_{nullptr};
    bool db_open_{false};
    mutable std::mutex mutex_;
};

} // namespace zigbee_mesh::storage
