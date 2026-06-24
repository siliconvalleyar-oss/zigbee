#include <gtest/gtest.h>
#include "core/types.h"

using namespace zigbee_mesh::core;

TEST(CoreTypesTest, ShortAddress) {
    ShortAddress addr = 0x1234;
    EXPECT_EQ(addr, 0x1234u);
}

TEST(CoreTypesTest, ExtendedAddress) {
    ExtendedAddress addr = 0x0011223344556677ULL;
    EXPECT_EQ(addr, 0x0011223344556677ULL);
}

TEST(CoreTypesTest, MacAddressShort) {
    MacAddress mac;
    mac.short_addr = 0x1234;
    EXPECT_TRUE(mac.isShort());
    EXPECT_FALSE(mac.isExtended());
    EXPECT_EQ(mac.toString(), "0x1234");
}

TEST(CoreTypesTest, MacAddressExtended) {
    MacAddress mac;
    mac.extended_addr = 0x0011223344556677ULL;
    EXPECT_FALSE(mac.isShort());
    EXPECT_TRUE(mac.isExtended());
}

TEST(CoreTypesTest, MacAddressEquality) {
    MacAddress a;
    a.short_addr = 0x1234;
    a.extended_addr = 0xFFFFFFFFFFFFFFFFULL;

    MacAddress b;
    b.short_addr = 0x1234;
    b.extended_addr = 0xFFFFFFFFFFFFFFFFULL;

    EXPECT_EQ(a, b);
}

TEST(CoreTypesTest, ErrorCodeToString) {
    EXPECT_STREQ(errorCodeToString(ErrorCode::Success), "Success");
    EXPECT_STREQ(errorCodeToString(ErrorCode::NotFound), "NotFound");
    EXPECT_STREQ(errorCodeToString(ErrorCode::Timeout), "Timeout");
}

TEST(CoreTypesTest, DeviceRoleToString) {
    EXPECT_STREQ(deviceRoleToString(DeviceRole::Coordinator), "Coordinator");
    EXPECT_STREQ(deviceRoleToString(DeviceRole::Router), "Router");
    EXPECT_STREQ(deviceRoleToString(DeviceRole::EndDevice), "EndDevice");
}

TEST(CoreTypesTest, NwkStateToString) {
    EXPECT_STREQ(nwkStateToString(NwkState::OffNetwork), "OffNetwork");
    EXPECT_STREQ(nwkStateToString(NwkState::Joined), "Joined");
}

TEST(CoreTypesTest, DeviceInfoDefaults) {
    DeviceInfo info;
    EXPECT_EQ(info.role, DeviceRole::Unknown);
    EXPECT_EQ(info.status, DeviceStatus::Unknown);
    EXPECT_EQ(info.lqi, 0u);
    EXPECT_EQ(info.rssi, 0);
}

TEST(CoreTypesTest, NetworkInfoDefaults) {
    NetworkInfo info;
    EXPECT_EQ(info.channel, kMinChannel);
    EXPECT_EQ(info.state, NwkState::OffNetwork);
}

TEST(CoreTypesTest, RouteEntryDefaults) {
    RouteEntry entry;
    EXPECT_FALSE(entry.active);
    EXPECT_EQ(entry.metric, 0u);
}

TEST(CoreTypesTest, NeighborEntryDefaults) {
    NeighborEntry entry;
    EXPECT_EQ(entry.lqi, 0u);
    EXPECT_EQ(entry.depth, 0u);
}
