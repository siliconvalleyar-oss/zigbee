#include <gtest/gtest.h>
#include "network/routing.h"

using namespace zigbee_mesh::routing;

TEST(RoutingTableTest, AddRoute) {
    RoutingTable table;
    table.setShortAddress(0x0001);
    RouteRecord route;
    route.destination = 0x0010;
    route.next_hop = 0x0005;
    route.hop_count = 2;
    route.status = RouteStatus::Active;
    EXPECT_TRUE(table.addRoute(route));
    EXPECT_EQ(table.getRouteCount(), 1u);
}

TEST(RoutingTableTest, FindRoute) {
    RoutingTable table;
    RouteRecord route;
    route.destination = 0x0010;
    route.next_hop = 0x0005;
    route.hop_count = 2;
    route.status = RouteStatus::Active;
    table.addRoute(route);
    const RouteRecord* found = table.findRoute(0x0010);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->next_hop, 0x0005u);
}

TEST(RoutingTableTest, RemoveRoute) {
    RoutingTable table;
    RouteRecord route;
    route.destination = 0x0010;
    table.addRoute(route);
    EXPECT_TRUE(table.removeRoute(0x0010));
    EXPECT_EQ(table.getRouteCount(), 0u);
}

TEST(RoutingTableTest, FindNonexistentRoute) {
    RoutingTable table;
    EXPECT_EQ(table.findRoute(0x0010), nullptr);
}

TEST(RoutingTableTest, UpdateRouteBetterPath) {
    RoutingTable table;
    RouteRecord route;
    route.destination = 0x0010;
    route.next_hop = 0x0005;
    route.hop_count = 5;
    table.addRoute(route);

    EXPECT_TRUE(table.updateRoute(0x0010, 0x0003, 2, 200));
    const RouteRecord* found = table.findRoute(0x0010);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->next_hop, 0x0003u);
    EXPECT_EQ(found->hop_count, 2u);
}

TEST(RoutingTableTest, UpdateRouteWorsePathIgnored) {
    RoutingTable table;
    RouteRecord route;
    route.destination = 0x0010;
    route.next_hop = 0x0005;
    route.hop_count = 2;
    table.addRoute(route);

    EXPECT_FALSE(table.updateRoute(0x0010, 0x0008, 5, 100));
    const RouteRecord* found = table.findRoute(0x0010);
    EXPECT_EQ(found->next_hop, 0x0005u);
}

TEST(RoutingTableTest, GetAllRoutes) {
    RoutingTable table;
    for (uint16_t i = 1; i <= 5; ++i) {
        RouteRecord route;
        route.destination = i * 0x10;
        route.next_hop = i;
        table.addRoute(route);
    }
    auto routes = table.getAllRoutes();
    EXPECT_EQ(routes.size(), 5u);
}

TEST(RoutingTableTest, Clear) {
    RoutingTable table;
    for (uint16_t i = 1; i <= 3; ++i) {
        RouteRecord route;
        route.destination = i * 0x10;
        table.addRoute(route);
    }
    table.clear();
    EXPECT_EQ(table.getRouteCount(), 0u);
}

TEST(RoutingTableTest, HasRoute) {
    RoutingTable table;
    EXPECT_FALSE(table.hasRoute(0x0010));
    RouteRecord route;
    route.destination = 0x0010;
    route.status = RouteStatus::Active;
    table.addRoute(route);
    EXPECT_TRUE(table.hasRoute(0x0010));
}

TEST(RoutingTableTest, DiscardedRouteNotFound) {
    RoutingTable table;
    RouteRecord route;
    route.destination = 0x0010;
    route.status = RouteStatus::Discarding;
    table.addRoute(route);
    EXPECT_EQ(table.findRoute(0x0010), nullptr);
}

TEST(RoutingTableTest, GetRoutesByNextHop) {
    RoutingTable table;
    RouteRecord r1; r1.destination = 0x0010; r1.next_hop = 0x0005;
    RouteRecord r2; r2.destination = 0x0020; r2.next_hop = 0x0005;
    RouteRecord r3; r3.destination = 0x0030; r3.next_hop = 0x0008;
    table.addRoute(r1);
    table.addRoute(r2);
    table.addRoute(r3);
    auto routes = table.getRoutesByNextHop(0x0005);
    EXPECT_EQ(routes.size(), 2u);
}
