#include <gtest/gtest.h>
#include "mesh/mesh_manager.h"

using namespace zigbee_mesh::mesh;

TEST(MeshManagerTest, AddNode) {
    MeshManager mgr;
    MeshNode node;
    node.nwk_addr = 0x0001;
    node.role = zigbee_mesh::core::DeviceRole::Router;
    EXPECT_TRUE(mgr.addNode(node));
    EXPECT_EQ(mgr.getNodeCount(), 1u);
}

TEST(MeshManagerTest, RemoveNode) {
    MeshManager mgr;
    MeshNode node;
    node.nwk_addr = 0x0001;
    mgr.addNode(node);
    EXPECT_TRUE(mgr.removeNode(0x0001));
    EXPECT_EQ(mgr.getNodeCount(), 0u);
}

TEST(MeshManagerTest, UpdateNode) {
    MeshManager mgr;
    MeshNode node;
    node.nwk_addr = 0x0001;
    mgr.addNode(node);
    EXPECT_TRUE(mgr.updateNode(0x0001, 200, -50));
    const MeshNode* found = mgr.getNode(0x0001);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->lqi, 200u);
    EXPECT_EQ(found->rssi, -50);
}

TEST(MeshManagerTest, SetNodeParent) {
    MeshManager mgr;
    MeshNode parent;
    parent.nwk_addr = 0x0001;
    parent.depth = 0;
    mgr.addNode(parent);

    MeshNode child;
    child.nwk_addr = 0x0002;
    mgr.addNode(child);

    EXPECT_TRUE(mgr.setNodeParent(0x0002, 0x0001));
    const MeshNode* found = mgr.getNode(0x0002);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->parent, 0x0001u);
    EXPECT_EQ(found->depth, 1u);
}

TEST(MeshManagerTest, AddLink) {
    MeshManager mgr;
    EXPECT_TRUE(mgr.addLink(0x0001, 0x0002, 100, 200));
    EXPECT_EQ(mgr.getLinkCount(), 1u);
}

TEST(MeshManagerTest, ContainsNode) {
    MeshManager mgr;
    EXPECT_FALSE(mgr.containsNode(0x0001));
    MeshNode node;
    node.nwk_addr = 0x0001;
    mgr.addNode(node);
    EXPECT_TRUE(mgr.containsNode(0x0001));
}

TEST(MeshManagerTest, ContainsLink) {
    MeshManager mgr;
    EXPECT_FALSE(mgr.containsLink(0x0001, 0x0002));
    mgr.addLink(0x0001, 0x0002, 100, 200);
    EXPECT_TRUE(mgr.containsLink(0x0001, 0x0002));
}

TEST(MeshManagerTest, GetRouters) {
    MeshManager mgr;
    MeshNode r; r.nwk_addr = 0x0001; r.role = zigbee_mesh::core::DeviceRole::Router;
    MeshNode e; e.nwk_addr = 0x0002; e.role = zigbee_mesh::core::DeviceRole::EndDevice;
    mgr.addNode(r);
    mgr.addNode(e);
    auto routers = mgr.getRouters();
    EXPECT_EQ(routers.size(), 1u);
}

TEST(MeshManagerTest, GetEndDevices) {
    MeshManager mgr;
    MeshNode r; r.nwk_addr = 0x0001; r.role = zigbee_mesh::core::DeviceRole::Router;
    MeshNode e; e.nwk_addr = 0x0002; e.role = zigbee_mesh::core::DeviceRole::EndDevice;
    mgr.addNode(r);
    mgr.addNode(e);
    auto devs = mgr.getEndDevices();
    EXPECT_EQ(devs.size(), 1u);
}

TEST(MeshManagerTest, TopologySnapshot) {
    MeshManager mgr;
    MeshNode c; c.nwk_addr = 0x0000; c.role = zigbee_mesh::core::DeviceRole::Coordinator; c.depth = 0;
    MeshNode r; r.nwk_addr = 0x0001; r.role = zigbee_mesh::core::DeviceRole::Router; r.depth = 1; r.parent = 0x0000;
    mgr.addNode(c);
    mgr.addNode(r);
    auto snap = mgr.getTopology();
    EXPECT_EQ(snap.total_nodes, 2u);
    EXPECT_EQ(snap.root_addr, 0x0000u);
}

TEST(MeshManagerTest, GetPath) {
    MeshManager mgr;
    MeshNode a; a.nwk_addr = 0x0001; a.depth = 0;
    MeshNode b; b.nwk_addr = 0x0002; b.depth = 1;
    MeshNode c; c.nwk_addr = 0x0003; c.depth = 2;
    mgr.addNode(a);
    mgr.addNode(b);
    mgr.addNode(c);
    mgr.addLink(0x0001, 0x0002, 100, 200);
    mgr.addLink(0x0002, 0x0003, 100, 200);
    auto path = mgr.getPath(0x0001, 0x0003);
    EXPECT_EQ(path.size(), 3u);
    EXPECT_EQ(path[0], 0x0001u);
    EXPECT_EQ(path[1], 0x0002u);
    EXPECT_EQ(path[2], 0x0003u);
}

TEST(MeshManagerTest, IsNodeReachable) {
    MeshManager mgr;
    MeshNode node;
    node.nwk_addr = 0x0001;
    node.is_alive = true;
    mgr.addNode(node);
    EXPECT_TRUE(mgr.isNodeReachable(0x0001));
    EXPECT_FALSE(mgr.isNodeReachable(0x0002));
}

TEST(MeshManagerTest, AgeNodes) {
    MeshManager mgr;
    MeshNode node;
    node.nwk_addr = 0x0001;
    node.last_seen_ms = 0;
    mgr.addNode(node);
    mgr.ageNodes(1000);
    const MeshNode* found = mgr.getNode(0x0001);
    ASSERT_NE(found, nullptr);
    EXPECT_GT(found->age_ms, 0u);
}

TEST(MeshManagerTest, AsciiTopology) {
    MeshManager mgr;
    MeshNode c; c.nwk_addr = 0x0000; c.role = zigbee_mesh::core::DeviceRole::Coordinator; c.depth = 0;
    mgr.addNode(c);
    std::string ascii = mgr.generateAsciiTopology();
    EXPECT_FALSE(ascii.empty());
    EXPECT_NE(ascii.find("0000"), std::string::npos);
}

TEST(MeshManagerTest, CheckMeshIntegrity) {
    MeshManager mgr;
    MeshNode a; a.nwk_addr = 0x0001; a.depth = 0;
    MeshNode b; b.nwk_addr = 0x0002; b.depth = 1; b.parent = 0x0001;
    mgr.addNode(a);
    mgr.addNode(b);
    EXPECT_TRUE(mgr.checkMeshIntegrity());
}
