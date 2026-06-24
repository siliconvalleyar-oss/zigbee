#include "mesh/mesh_manager.h"
#include "core/logger.h"
#include <algorithm>
#include <queue>
#include <set>

namespace zigbee_mesh::mesh {

bool MeshManager::init(routing::RoutingTable* routing_table) {
    routing_table_ = routing_table;
    initialized_ = true;
    ZIGBEE_LOG_INFO("Mesh manager initialized");
    return true;
}

bool MeshManager::addNode(const MeshNode& node) {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_[node.nwk_addr] = node;
    nodes_[node.nwk_addr].is_alive = true;
    nodes_[node.nwk_addr].last_seen_ms = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return true;
}

bool MeshManager::removeNode(core::ShortAddress addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.erase(addr);
    links_from_.erase(addr);
    for (auto& [k, vec] : links_from_) {
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [addr](const MeshLink& l) { return l.destination == addr; }), vec.end());
    }
    links_to_.erase(addr);
    for (auto& [k, vec] : links_to_) {
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [addr](const MeshLink& l) { return l.source == addr; }), vec.end());
    }
    return true;
}

bool MeshManager::updateNode(core::ShortAddress addr, uint8_t lqi, int8_t rssi) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(addr);
    if (it == nodes_.end()) return false;
    it->second.lqi = lqi;
    it->second.rssi = rssi;
    it->second.is_alive = true;
    it->second.last_seen_ms = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return true;
}

bool MeshManager::setNodeParent(core::ShortAddress addr, core::ShortAddress parent) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(addr);
    if (it == nodes_.end()) return false;
    auto parent_it = nodes_.find(parent);
    if (parent_it == nodes_.end()) return false;

    it->second.parent = parent;
    it->second.depth = parent_it->second.depth + 1;
    parent_it->second.children.push_back(addr);
    return true;
}

bool MeshManager::addLink(core::ShortAddress source, core::ShortAddress dest, uint16_t cost, uint8_t lqi) {
    std::lock_guard<std::mutex> lock(mutex_);
    MeshLink link;
    link.source = source;
    link.destination = dest;
    link.cost = cost;
    link.lqi = lqi;
    link.is_active = true;
    links_from_[source].push_back(link);
    links_to_[dest].push_back(link);
    return true;
}

bool MeshManager::removeLink(core::ShortAddress source, core::ShortAddress dest) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& from = links_from_[source];
    from.erase(std::remove_if(from.begin(), from.end(),
        [dest](const MeshLink& l) { return l.destination == dest; }), from.end());
    auto& to = links_to_[dest];
    to.erase(std::remove_if(to.begin(), to.end(),
        [source](const MeshLink& l) { return l.source == source; }), to.end());
    return true;
}

bool MeshManager::updateLinkCost(core::ShortAddress source, core::ShortAddress dest, uint16_t cost) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = links_from_.find(source);
    if (it == links_from_.end()) return false;
    for (auto& link : it->second) {
        if (link.destination == dest) {
            link.cost = cost;
            return true;
        }
    }
    return false;
}

const MeshNode* MeshManager::getNode(core::ShortAddress addr) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(addr);
    return it != nodes_.end() ? &it->second : nullptr;
}

std::vector<MeshNode> MeshManager::getAllNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MeshNode> result;
    for (const auto& [addr, node] : nodes_) {
        result.push_back(node);
    }
    return result;
}

std::vector<MeshNode> MeshManager::getRouters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MeshNode> result;
    for (const auto& [addr, node] : nodes_) {
        if (node.role == core::DeviceRole::Coordinator ||
            node.role == core::DeviceRole::Router) {
            result.push_back(node);
        }
    }
    return result;
}

std::vector<MeshNode> MeshManager::getEndDevices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MeshNode> result;
    for (const auto& [addr, node] : nodes_) {
        if (node.role == core::DeviceRole::EndDevice) {
            result.push_back(node);
        }
    }
    return result;
}

std::vector<MeshNode> MeshManager::getChildren(core::ShortAddress parent) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MeshNode> result;
    for (const auto& [addr, node] : nodes_) {
        if (node.parent == parent) {
            result.push_back(node);
        }
    }
    return result;
}

std::vector<MeshLink> MeshManager::getLinksFrom(core::ShortAddress addr) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = links_from_.find(addr);
    return it != links_from_.end() ? it->second : std::vector<MeshLink>{};
}

std::vector<MeshLink> MeshManager::getLinksTo(core::ShortAddress addr) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = links_to_.find(addr);
    return it != links_to_.end() ? it->second : std::vector<MeshLink>{};
}

std::vector<MeshLink> MeshManager::getAllLinks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MeshLink> result;
    for (const auto& [addr, links] : links_from_) {
        result.insert(result.end(), links.begin(), links.end());
    }
    return result;
}

TopologySnapshot MeshManager::getTopology() const {
    std::lock_guard<std::mutex> lock(mutex_);
    TopologySnapshot snap;
    snap.timestamp_ms = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    snap.total_nodes = nodes_.size();
    for (const auto& [addr, node] : nodes_) {
        snap.nodes.push_back(node);
        if (node.role == core::DeviceRole::Coordinator) snap.root_addr = addr;
        if (node.role == core::DeviceRole::Router || node.role == core::DeviceRole::Coordinator)
            snap.total_routers++;
        else snap.total_end_devices++;
    }
    for (const auto& [addr, links] : links_from_) {
        for (const auto& link : links) {
            snap.links.push_back(link);
            snap.total_links++;
        }
    }
    return snap;
}

size_t MeshManager::getNodeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.size();
}

size_t MeshManager::getLinkCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [addr, links] : links_from_) {
        count += links.size();
    }
    return count;
}

bool MeshManager::containsNode(core::ShortAddress addr) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.find(addr) != nodes_.end();
}

bool MeshManager::containsLink(core::ShortAddress src, core::ShortAddress dst) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = links_from_.find(src);
    if (it == links_from_.end()) return false;
    for (const auto& link : it->second) {
        if (link.destination == dst) return true;
    }
    return false;
}

uint8_t MeshManager::getNodeDepth(core::ShortAddress addr) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(addr);
    return it != nodes_.end() ? it->second.depth : 0xFF;
}

bool MeshManager::isNodeReachable(core::ShortAddress addr) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(addr);
    return it != nodes_.end() && it->second.is_alive;
}

std::vector<core::ShortAddress> MeshManager::getPath(core::ShortAddress src, core::ShortAddress dst) const {
    return bfs(src, dst);
}

void MeshManager::ageNodes(uint32_t max_age_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t now = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    for (auto& [addr, node] : nodes_) {
        node.age_ms = now - node.last_seen_ms;
        if (node.age_ms > max_age_ms) {
            node.is_alive = false;
        }
    }
}

void MeshManager::removeStaleNodes(uint32_t max_age_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t now = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    auto it = nodes_.begin();
    while (it != nodes_.end()) {
        if ((now - it->second.last_seen_ms) > max_age_ms) {
            it = nodes_.erase(it);
        } else {
            ++it;
        }
    }
}

void MeshManager::setTopologyUpdateCallback(TopologyUpdateCallback cb) {
    topology_callback_ = std::move(cb);
}

bool MeshManager::checkMeshIntegrity() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [addr, node] : nodes_) {
        if (node.parent != 0 && node.parent != addr) {
            if (nodes_.find(node.parent) == nodes_.end()) {
                ZIGBEE_LOG_WARN("Orphaned node 0x%04X (parent 0x%04X not found)", addr, node.parent);
                return false;
            }
        }
    }
    return true;
}

std::vector<core::ShortAddress> MeshManager::getOrphanedNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<core::ShortAddress> orphans;
    for (const auto& [addr, node] : nodes_) {
        if (node.parent != 0 && node.parent != addr) {
            if (nodes_.find(node.parent) == nodes_.end()) {
                orphans.push_back(addr);
            }
        }
    }
    return orphans;
}

std::string MeshManager::generateAsciiTopology() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result;
    result += "=== Zigbee Mesh Topology ===\n\n";

    std::vector<const MeshNode*> roots;
    for (const auto& [addr, node] : nodes_) {
        if (node.depth == 0) roots.push_back(&node);
    }

    for (const auto* root : roots) {
        result += core::MacAddress::shortAddrToString(root->nwk_addr);
        result += " [" + std::string(core::deviceRoleToString(root->role)) + "]\n";

        std::function<void(core::ShortAddress, const std::string&)> printChildren =
            [&](core::ShortAddress parent, const std::string& prefix) {
                bool first = true;
                for (const auto& [addr, node] : nodes_) {
                    if (node.parent == parent && node.nwk_addr != parent) {
                        if (!first) result += prefix + "|\n";
                        result += prefix + "+--" +
                            core::MacAddress::shortAddrToString(node.nwk_addr) +
                            " [" + std::string(core::deviceRoleToString(node.role)) +
                            "] LQI=" + std::to_string(node.lqi) +
                            " RSSI=" + std::to_string(node.rssi) + "dBm\n";
                        printChildren(node.nwk_addr, prefix + "|   ");
                        first = false;
                    }
                }
            };

        printChildren(root->nwk_addr, "  ");
    }

    result += "\nTotal: " + std::to_string(nodes_.size()) + " nodes\n";
    return result;
}

std::vector<core::ShortAddress> MeshManager::bfs(core::ShortAddress src, core::ShortAddress dst) const {
    if (src == dst) return {src};

    std::map<core::ShortAddress, core::ShortAddress> parents;
    std::queue<core::ShortAddress> queue;
    std::set<core::ShortAddress> visited;

    queue.push(src);
    visited.insert(src);

    while (!queue.empty()) {
        core::ShortAddress current = queue.front();
        queue.pop();

        auto links_it = links_from_.find(current);
        if (links_it != links_from_.end()) {
            for (const auto& link : links_it->second) {
                if (visited.find(link.destination) == visited.end()) {
                    visited.insert(link.destination);
                    parents[link.destination] = current;
                    if (link.destination == dst) {
                        std::vector<core::ShortAddress> path;
                        for (auto at = dst; at != src; at = parents[at]) {
                            path.push_back(at);
                        }
                        path.push_back(src);
                        std::reverse(path.begin(), path.end());
                        return path;
                    }
                    queue.push(link.destination);
                }
            }
        }
    }
    return {};
}

} // namespace zigbee_mesh::mesh
