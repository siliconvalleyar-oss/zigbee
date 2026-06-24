#pragma once

#include "core/types.h"
#include "core/bytebuffer.h"
#include <vector>
#include <map>
#include <mutex>
#include <functional>

namespace zigbee_mesh::routing {

enum class RouteStatus : uint8_t {
    Active = 0,
    Discarding = 1,
    Repairing = 2,
    Unknown = 0xFF,
};

struct RouteRecord {
    core::ShortAddress destination{0};
    core::ShortAddress next_hop{0};
    uint8_t hop_count{0};
    uint16_t route_score{0};
    RouteStatus status{RouteStatus::Active};
    uint32_t age_ms{0};
    uint32_t created_ms{0};
    bool concentrator{false};
    bool memory_constrained{false};
    uint8_t no_route_cache{0};
};

struct SourceRouteRecord {
    core::ShortAddress destination{0};
    std::vector<core::ShortAddress> route;
    uint8_t relay_count{0};
};

struct NeighborCost {
    core::ShortAddress addr{0};
    uint8_t outgoing_lqi{0};
    uint16_t link_cost{0};
    uint8_t depth{0};
    bool is_parent{false};
};

struct RouteRequest {
    uint8_t route_request_id{0};
    core::ShortAddress originator{0};
    core::ShortAddress destination{0};
    uint8_t path_cost{0};
    uint8_t hop_count{0};
    uint8_t max_hops{30};
    bool destination_only{false};
    bool multicast{false};
};

class RoutingTable {
public:
    RoutingTable() = default;

    bool addRoute(const RouteRecord& route);
    bool removeRoute(core::ShortAddress destination);
    bool updateRoute(core::ShortAddress destination, core::ShortAddress next_hop,
                     uint8_t hop_count, uint16_t lqi);
    const RouteRecord* findRoute(core::ShortAddress destination) const;
    bool hasRoute(core::ShortAddress destination) const;

    std::vector<RouteRecord> getAllRoutes() const;
    std::vector<RouteRecord> getRoutesByNextHop(core::ShortAddress next_hop) const;
    size_t getRouteCount() const;

    void ageRoutes(uint32_t max_age_ms);
    void removeExpiredRoutes(uint32_t max_age_ms);
    void clear();

    bool addSourceRoute(const SourceRouteRecord& record);
    const SourceRouteRecord* findSourceRoute(core::ShortAddress destination) const;
    void removeSourceRoute(core::ShortAddress destination);

    void setShortAddress(core::ShortAddress addr) { local_addr_ = addr; }
    core::ShortAddress getShortAddress() const { return local_addr_; }

    std::string toString() const;

private:
    uint16_t calculateLinkCost(uint8_t lqi) const;

    mutable std::mutex mutex_;
    std::map<core::ShortAddress, RouteRecord> routes_;
    std::map<core::ShortAddress, SourceRouteRecord> source_routes_;
    core::ShortAddress local_addr_{0};
};

class RouteDiscovery {
public:
    using RouteFoundCallback = std::function<void(const RouteRecord&)>;

    explicit RouteDiscovery(RoutingTable* table);

    bool initiateRouteDiscovery(core::ShortAddress destination,
                                RouteFoundCallback callback,
                                uint8_t max_hops = 30,
                                int timeout_ms = 3000);

    bool handleRouteRequest(const RouteRequest& request);
    bool handleRouteReply(core::ShortAddress originator, core::ShortAddress destination,
                          uint8_t path_cost, const std::vector<core::ShortAddress>& relay_list);

    void setLocalAddress(core::ShortAddress addr) { local_addr_ = addr; }
    std::vector<RouteRequest> getPendingRequests() const;

private:
    struct PendingDiscovery {
        uint8_t request_id;
        core::ShortAddress destination;
        RouteFoundCallback callback;
        uint32_t timestamp;
        uint8_t attempts;
    };

    RoutingTable* table_;
    std::vector<PendingDiscovery> pending_;
    core::ShortAddress local_addr_{0};
    uint8_t next_request_id_{0};
    mutable std::mutex mutex_;
};

} // namespace zigbee_mesh::routing
