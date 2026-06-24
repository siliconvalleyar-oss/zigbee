#include "network/routing.h"
#include "core/logger.h"
#include <algorithm>

namespace zigbee_mesh::routing {

bool RoutingTable::addRoute(const RouteRecord& route) {
    std::lock_guard<std::mutex> lock(mutex_);
    routes_[route.destination] = route;
    routes_[route.destination].created_ms = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return true;
}

bool RoutingTable::removeRoute(core::ShortAddress destination) {
    std::lock_guard<std::mutex> lock(mutex_);
    return routes_.erase(destination) > 0;
}

bool RoutingTable::updateRoute(core::ShortAddress destination, core::ShortAddress next_hop,
                                uint8_t hop_count, uint16_t lqi) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = routes_.find(destination);
    if (it != routes_.end()) {
        if (hop_count < it->second.hop_count) {
            it->second.next_hop = next_hop;
            it->second.hop_count = hop_count;
            it->second.route_score = lqi;
            it->second.age_ms = 0;
            return true;
        }
        return false;
    }
    RouteRecord route;
    route.destination = destination;
    route.next_hop = next_hop;
    route.hop_count = hop_count;
    route.route_score = lqi;
    route.status = RouteStatus::Active;
    route.age_ms = 0;
    routes_[destination] = route;
    return true;
}

const RouteRecord* RoutingTable::findRoute(core::ShortAddress destination) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = routes_.find(destination);
    if (it == routes_.end()) return nullptr;
    if (it->second.status == RouteStatus::Discarding) return nullptr;
    return &it->second;
}

bool RoutingTable::hasRoute(core::ShortAddress destination) const {
    return findRoute(destination) != nullptr;
}

std::vector<RouteRecord> RoutingTable::getAllRoutes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RouteRecord> result;
    for (const auto& [addr, route] : routes_) {
        result.push_back(route);
    }
    return result;
}

std::vector<RouteRecord> RoutingTable::getRoutesByNextHop(core::ShortAddress next_hop) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RouteRecord> result;
    for (const auto& [addr, route] : routes_) {
        if (route.next_hop == next_hop) result.push_back(route);
    }
    return result;
}

size_t RoutingTable::getRouteCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return routes_.size();
}

void RoutingTable::ageRoutes(uint32_t max_age_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [addr, route] : routes_) {
        route.age_ms += max_age_ms;
    }
}

void RoutingTable::removeExpiredRoutes(uint32_t max_age_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = routes_.begin();
    while (it != routes_.end()) {
        if (it->second.age_ms >= max_age_ms) {
            it = routes_.erase(it);
        } else {
            ++it;
        }
    }
}

void RoutingTable::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    routes_.clear();
    source_routes_.clear();
}

bool RoutingTable::addSourceRoute(const SourceRouteRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    source_routes_[record.destination] = record;
    return true;
}

const SourceRouteRecord* RoutingTable::findSourceRoute(core::ShortAddress destination) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = source_routes_.find(destination);
    return it != source_routes_.end() ? &it->second : nullptr;
}

void RoutingTable::removeSourceRoute(core::ShortAddress destination) {
    std::lock_guard<std::mutex> lock(mutex_);
    source_routes_.erase(destination);
}

std::string RoutingTable::toString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result = "Routing Table:\n";
    result += "-------------\n";
    for (const auto& [addr, route] : routes_) {
        char buf[128];
        snprintf(buf, sizeof(buf), "  0x%04X -> 0x%04X (hops=%d, score=%u, status=%d)\n",
                 addr, route.next_hop, route.hop_count, route.route_score,
                 static_cast<int>(route.status));
        result += buf;
    }
    result += "Source Routes: " + std::to_string(source_routes_.size()) + "\n";
    return result;
}

uint16_t RoutingTable::calculateLinkCost(uint8_t lqi) const {
    if (lqi == 0) return 0;
    if (lqi < 32) return 7;
    if (lqi < 64) return 6;
    if (lqi < 96) return 5;
    if (lqi < 128) return 4;
    if (lqi < 160) return 3;
    if (lqi < 192) return 2;
    return 1;
}

RouteDiscovery::RouteDiscovery(RoutingTable* table) : table_(table) {}

bool RouteDiscovery::initiateRouteDiscovery(core::ShortAddress destination,
                                             RouteFoundCallback callback,
                                             uint8_t max_hops, int timeout_ms) {
    (void)max_hops;
    (void)timeout_ms;
    if (!table_) return false;

    PendingDiscovery disc;
    disc.request_id = next_request_id_++;
    disc.destination = destination;
    disc.callback = std::move(callback);
    disc.timestamp = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    disc.attempts = 0;

    std::lock_guard<std::mutex> lock(mutex_);
    pending_.push_back(disc);
    return true;
}

bool RouteDiscovery::handleRouteRequest(const RouteRequest& request) {
    if (!table_) return false;
    if (request.destination == local_addr_) {
        RouteRecord route;
        route.destination = request.originator;
        route.next_hop = request.originator;
        route.hop_count = request.hop_count + 1;
        route.route_score = request.path_cost;
        table_->addRoute(route);
        return true;
    }
    return false;
}

bool RouteDiscovery::handleRouteReply(core::ShortAddress originator, core::ShortAddress destination,
                                       uint8_t path_cost, const std::vector<core::ShortAddress>& relay_list) {
    (void)relay_list;
    if (!table_) return false;

    RouteRecord route;
    route.destination = destination;
    route.next_hop = originator;
    route.hop_count = 0;
    route.route_score = path_cost;
    table_->addRoute(route);

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(pending_.begin(), pending_.end(),
        [&](const PendingDiscovery& d) { return d.destination == destination; });
    if (it != pending_.end()) {
        if (it->callback) it->callback(route);
        pending_.erase(it);
    }
    return true;
}

std::vector<RouteRequest> RouteDiscovery::getPendingRequests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {};
}

} // namespace zigbee_mesh::routing
