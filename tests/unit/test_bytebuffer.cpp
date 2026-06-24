#include <gtest/gtest.h>
#include "core/bytebuffer.h"

using namespace zigbee_mesh::core;

TEST(ByteBufferTest, DefaultConstruction) {
    ByteBuffer buf;
    EXPECT_EQ(buf.size(), 0u);
    EXPECT_TRUE(buf.empty());
}

TEST(ByteBufferTest, CapacityConstruction) {
    ByteBuffer buf(64);
    EXPECT_EQ(buf.size(), 0u);
    EXPECT_FALSE(buf.empty());
}

TEST(ByteBufferTest, DataConstruction) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    ByteBuffer buf(data, 3);
    EXPECT_EQ(buf.size(), 3u);
    EXPECT_EQ(buf[0], 0x01);
    EXPECT_EQ(buf[1], 0x02);
    EXPECT_EQ(buf[2], 0x03);
}

TEST(ByteBufferTest, AppendU8) {
    ByteBuffer buf;
    buf.appendU8(0xAB);
    EXPECT_EQ(buf.size(), 1u);
    EXPECT_EQ(buf[0], 0xAB);
}

TEST(ByteBufferTest, AppendU16LE) {
    ByteBuffer buf;
    buf.appendU16LE(0x1234);
    EXPECT_EQ(buf.size(), 2u);
    EXPECT_EQ(buf[0], 0x34);
    EXPECT_EQ(buf[1], 0x12);
}

TEST(ByteBufferTest, AppendU16BE) {
    ByteBuffer buf;
    buf.appendU16BE(0x1234);
    EXPECT_EQ(buf.size(), 2u);
    EXPECT_EQ(buf[0], 0x12);
    EXPECT_EQ(buf[1], 0x34);
}

TEST(ByteBufferTest, AppendU32LE) {
    ByteBuffer buf;
    buf.appendU32LE(0x12345678);
    EXPECT_EQ(buf.size(), 4u);
    EXPECT_EQ(buf[0], 0x78);
    EXPECT_EQ(buf[1], 0x56);
    EXPECT_EQ(buf[2], 0x34);
    EXPECT_EQ(buf[3], 0x12);
}

TEST(ByteBufferTest, ReadU8) {
    ByteBuffer buf;
    buf.appendU8(0xFF);
    EXPECT_EQ(buf.readU8(0), 0xFF);
}

TEST(ByteBufferTest, ReadU16LE) {
    ByteBuffer buf;
    buf.appendU16LE(0xBEEF);
    EXPECT_EQ(buf.readU16LE(0), 0xBEEF);
}

TEST(ByteBufferTest, ReadU16BE) {
    ByteBuffer buf;
    buf.appendU16BE(0xBEEF);
    EXPECT_EQ(buf.readU16BE(0), 0xBEEF);
}

TEST(ByteBufferTest, FromHex) {
    ByteBuffer buf = ByteBuffer::fromHex("DEADBEEF");
    EXPECT_EQ(buf.size(), 4u);
    EXPECT_EQ(buf[0], 0xDE);
    EXPECT_EQ(buf[1], 0xAD);
    EXPECT_EQ(buf[2], 0xBE);
    EXPECT_EQ(buf[3], 0xEF);
}

TEST(ByteBufferTest, ToHex) {
    ByteBuffer buf;
    buf.appendU8(0xDE);
    buf.appendU8(0xAD);
    buf.appendU8(0xBE);
    buf.appendU8(0xEF);
    EXPECT_EQ(buf.toHex(), "deadbeef");
}

TEST(ByteBufferTest, AppendByteBuffer) {
    ByteBuffer a;
    a.appendU8(0x01);
    ByteBuffer b;
    b.appendU8(0x02);
    a.append(b);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a[0], 0x01);
    EXPECT_EQ(a[1], 0x02);
}

TEST(ByteBufferTest, Subbuffer) {
    ByteBuffer buf;
    buf.appendU8(0x01);
    buf.appendU8(0x02);
    buf.appendU8(0x03);
    auto sub = buf.subbuffer(1, 2);
    EXPECT_EQ(sub.size(), 2u);
    EXPECT_EQ(sub[0], 0x02);
    EXPECT_EQ(sub[1], 0x03);
}

TEST(ByteBufferTest, Equality) {
    ByteBuffer a;
    a.appendU8(0x01);
    ByteBuffer b;
    b.appendU8(0x01);
    EXPECT_EQ(a, b);
}

TEST(ByteBufferTest, OutOfBounds) {
    ByteBuffer buf(2);
    EXPECT_THROW(buf.readU8(5), std::out_of_range);
}

TEST(ByteBufferTest, Clear) {
    ByteBuffer buf;
    buf.appendU8(0x01);
    buf.clear();
    EXPECT_TRUE(buf.empty());
}

TEST(ByteBufferTest, MoveSemantics) {
    ByteBuffer a;
    a.appendU8(0xFF);
    ByteBuffer b = std::move(a);
    EXPECT_EQ(b.size(), 1u);
    EXPECT_EQ(b[0], 0xFF);
}

TEST(ByteBufferTest, AppendVector) {
    ByteBuffer buf;
    buf.appendU8(0x01);
    buf.appendU8(0x02);
    EXPECT_EQ(buf.toVector(), (std::vector<uint8_t>{0x01, 0x02}));
}
