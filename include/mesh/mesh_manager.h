#pragma once

#include "core/types.h"
#include "network/routing.h"
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <functional>

namespace zigbee_mesh::mesh {

struct MeshNode {
    core::ShortAddress nwk_addr{0};
    core::ExtendedAddress ieee_addr{0};
    core::DeviceRole role{core::DeviceRole::Unknown};
    uint8_t depth{0};
    core::ShortAddress parent{0};
    std::vector<core::ShortAddress> children;
    uint8_t lqi{0};
    int8_t rssi{0};
    bool is_alive{false};
    uint32_t last_seen_ms{0};
    uint32_t age_ms{0};
};

struct MeshLink {
    core::ShortAddress source{0};
    core::ShortAddress destination{0};
    uint16_t cost{0};
    uint8_t lqi{0};
    bool is_active{false};
};

struct TopologySnapshot {
    std::vector<MeshNode> nodes;
    std::vector<MeshLink> links;
    uint32_t timestamp_ms{0};
    core::ShortAddress root_addr{0};
    size_t total_nodes{0};
    size_t total_routers{0};
    size_t total_end_devices{0};
    size_t total_links{0};
};

class MeshManager {
public:
    using TopologyUpdateCallback = std::function<void(const TopologySnapshot&)>;

    MeshManager() = default;

    bool init(routing::RoutingTable* routing_table);

    bool addNode(const MeshNode& node);
    bool removeNode(core::ShortAddress addr);
    bool updateNode(core::ShortAddress addr, uint8_t lqi, int8_t rssi);
    bool setNodeParent(core::ShortAddress addr, core::ShortAddress parent);

    bool addLink(core::ShortAddress source, core::ShortAddress dest, uint16_t cost, uint8_t lqi);
    bool removeLink(core::ShortAddress source, core::ShortAddress dest);
    bool updateLinkCost(core::ShortAddress source, core::ShortAddress dest, uint16_t cost);

    const MeshNode* getNode(core::ShortAddress addr) const;
    std::vector<MeshNode> getAllNodes() const;
    std::vector<MeshNode> getRouters() const;
    std::vector<MeshNode> getEndDevices() const;
    std::vector<MeshNode> getChildren(core::ShortAddress parent) const;

    std::vector<MeshLink> getLinksFrom(core::ShortAddress addr) const;
    std::vector<MeshLink> getLinksTo(core::ShortAddress addr) const;
    std::vector<MeshLink> getAllLinks() const;

    TopologySnapshot getTopology() const;

    size_t getNodeCount() const;
    size_t getLinkCount() const;
    bool containsNode(core::ShortAddress addr) const;
    bool containsLink(core::ShortAddress src, core::ShortAddress dst) const;

    uint8_t getNodeDepth(core::ShortAddress addr) const;
    bool isNodeReachable(core::ShortAddress addr) const;
    std::vector<core::ShortAddress> getPath(core::ShortAddress src, core::ShortAddress dst) const;

    void ageNodes(uint32_t max_age_ms);
    void removeStaleNodes(uint32_t max_age_ms);
    void setTopologyUpdateCallback(TopologyUpdateCallback cb);

    bool checkMeshIntegrity() const;
    std::vector<core::ShortAddress> getOrphanedNodes() const;

    std::string generateAsciiTopology() const;

private:
    std::vector<core::ShortAddress> bfs(core::ShortAddress src, core::ShortAddress dst) const;

    mutable std::mutex mutex_;
    std::map<core::ShortAddress, MeshNode> nodes_;
    std::map<core::ShortAddress, std::vector<MeshLink>> links_from_;
    std::map<core::ShortAddress, std::vector<MeshLink>> links_to_;
    routing::RoutingTable* routing_table_{nullptr};
    TopologyUpdateCallback topology_callback_;
    bool initialized_{false};
};

} // namespace zigbee_mesh::mesh
