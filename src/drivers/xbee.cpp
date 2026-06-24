#include "drivers/xbee.h"
#include "core/logger.h"
#include <cstring>
#include <thread>
#include <algorithm>

namespace zigbee_mesh::drivers {

XBeeDriver::XBeeDriver() = default;
XBeeDriver::~XBeeDriver() { shutdown(); }

bool XBeeDriver::init(const RadioConfig& config) {
    device_path_ = config.device_path;
    channel_ = config.channel;
    tx_power_ = config.tx_power;

    if (!uart_.open(config.device_path, config.baud_rate)) {
        ZIGBEE_LOG_ERROR("XBee: Failed to open serial: %s", config.device_path.c_str());
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    setChannel(channel_);

    zigbee_mesh::core::ByteBuffer pan_data;
    pan_data.appendU16BE(config.pan_id);
    std::string pan_hex = pan_data.toHex();
    sendATCommand("ID", pan_hex);

    std::string addr_hex;
    zigbee_mesh::core::ByteBuffer addr_data;
    addr_data.appendU16BE(config.short_addr);
    addr_hex = addr_data.toHex();
    sendATCommand("MY", addr_hex);

    if (config.promiscuous) {
        sendATCommand("PR", "7");
    }

    sendATCommand("AP", "0");

    sendATCommand("WR");
    sendATCommand("AC");

    running_ = true;
    rx_thread_ = std::thread(&XBeeDriver::rxLoop, this);

    ZIGBEE_LOG_INFO("XBee: Initialized on %s, channel %d", config.device_path.c_str(), channel_);
    return true;
}

void XBeeDriver::shutdown() {
    running_ = false;
    if (rx_thread_.joinable()) {
        rx_thread_.join();
    }
    uart_.close();
    ZIGBEE_LOG_INFO("XBee: Shutdown");
}

bool XBeeDriver::isOpen() const { return uart_.isOpen(); }

bool XBeeDriver::send(const zigbee_mesh::core::ByteBuffer& frame) {
    if (!isOpen()) return false;

    zigbee_mesh::core::ByteBuffer packet;
    packet.push_back(XBEE_START_DELIMITER);

    uint16_t api_frame_length = frame.size() + 14;
    packet.appendU16BE(api_frame_length);

    std::vector<uint8_t> api_frame;
    api_frame.push_back(XBEE_TX_FRAME_TYPE);
    frame_id_++;
    api_frame.push_back(frame_id_);
    api_frame.push_back(0x00);
    api_frame.push_back(0x00);
    api_frame.push_back(0x00);
    api_frame.push_back(0x00);
    api_frame.push_back(0x00);
    api_frame.push_back(0x00);
    api_frame.push_back(0xFF);
    api_frame.push_back(0xFE);
    api_frame.push_back(0x00);
    api_frame.push_back(0x00);

    api_frame.insert(api_frame.end(), frame.data(), frame.data() + frame.size());

    std::vector<uint8_t> escaped;
    escaped.reserve(api_frame.size() + 8);
    for (uint8_t byte : api_frame) {
        escapeByte(byte, escaped);
    }

    for (uint8_t byte : escaped) {
        packet.push_back(byte);
    }

    uint8_t checksum = 0;
    for (uint8_t byte : api_frame) {
        checksum += byte;
    }
    checksum = 0xFF - checksum;

    std::vector<uint8_t> check_escaped;
    escapeByte(checksum, check_escaped);
    for (uint8_t byte : check_escaped) {
        packet.push_back(byte);
    }

    auto sent = uart_.write(packet.data(), packet.size());
    return sent == static_cast<ssize_t>(packet.size());
}

bool XBeeDriver::setChannel(uint8_t channel) {
    if (channel < 11 || channel > 26) return false;
    channel_ = channel;
    char hex_ch[8];
    snprintf(hex_ch, sizeof(hex_ch), "%02X", channel);
    return sendATCommand("CH", hex_ch);
}

bool XBeeDriver::setTxPower(int8_t power) {
    tx_power_ = power;
    uint8_t dbm = static_cast<uint8_t>(power + 30);
    if (dbm > 4) dbm = 4;
    char hex[8];
    snprintf(hex, sizeof(hex), "%02X", dbm);
    return sendATCommand("PL", hex);
}

bool XBeeDriver::setPanId(uint16_t pan_id) {
    zigbee_mesh::core::ByteBuffer data;
    data.appendU16BE(pan_id);
    return sendATCommand("ID", data.toHex());
}

bool XBeeDriver::setShortAddress(uint16_t addr) {
    zigbee_mesh::core::ByteBuffer data;
    data.appendU16BE(addr);
    return sendATCommand("MY", data.toHex());
}

bool XBeeDriver::setExtendedAddress(uint64_t addr) {
    zigbee_mesh::core::ByteBuffer data;
    data.appendU64LE(addr);
    return sendATCommand("SL", data.toHex().substr(0, 8));
}

bool XBeeDriver::setPromiscuous(bool enabled) {
    return sendATCommand("PR", enabled ? "7" : "4");
}

uint8_t XBeeDriver::getChannel() const { return channel_; }
int8_t XBeeDriver::getTxPower() const { return tx_power_; }
int8_t XBeeDriver::getRssi() const { return 0; }

bool XBeeDriver::sendATCommand(const std::string& cmd, const std::string& param,
                                std::string* response, int timeout_ms) {
    zigbee_mesh::core::ByteBuffer frame;
    uint16_t api_len = param.size() / 2 + 4;
    frame.appendU16BE(api_len);

    std::vector<uint8_t> api_frame;
    api_frame.push_back(XBEE_AT_FRAME_TYPE);
    frame_id_++;
    api_frame.push_back(frame_id_);
    for (char c : cmd) {
        api_frame.push_back(static_cast<uint8_t>(c));
    }

    if (!param.empty()) {
        zigbee_mesh::core::ByteBuffer param_data = zigbee_mesh::core::ByteBuffer::fromHex(param);
        api_frame.insert(api_frame.end(), param_data.data(), param_data.data() + param_data.size());
    }

    zigbee_mesh::core::ByteBuffer packet;
    packet.push_back(XBEE_START_DELIMITER);

    std::vector<uint8_t> escaped;
    for (uint8_t byte : api_frame) {
        escapeByte(byte, escaped);
    }

    uint16_t len = static_cast<uint16_t>(api_frame.size());
    std::vector<uint8_t> len_bytes;
    escapeByte(static_cast<uint8_t>((len >> 8) & 0xFF), len_bytes);
    escapeByte(static_cast<uint8_t>(len & 0xFF), len_bytes);

    for (uint8_t b : len_bytes) packet.push_back(b);
    for (uint8_t b : escaped) packet.push_back(b);

    uint8_t checksum = 0;
    for (uint8_t byte : api_frame) checksum += byte;
    checksum = 0xFF - checksum;
    std::vector<uint8_t> check_escaped;
    escapeByte(checksum, check_escaped);
    for (uint8_t b : check_escaped) packet.push_back(b);

    uart_.write(packet.data(), packet.size());
    return true;
}

bool XBeeDriver::setNodeIdentifier(const std::string& id) { return sendATCommand("NI", id); }

bool XBeeDriver::getHardwareVersion(std::string& version) {
    return sendATCommand("HV", "", &version);
}

bool XBeeDriver::getFirmwareVersion(std::string& version) {
    return sendATCommand("VR", "", &version);
}

bool XBeeDriver::escapeByte(uint8_t byte, std::vector<uint8_t>& output) {
    if (byte == 0x7E || byte == 0x7D || byte == 0x11 || byte == 0x13) {
        output.push_back(0x7D);
        output.push_back(byte ^ 0x20);
        return true;
    }
    output.push_back(byte);
    return false;
}

uint8_t XBeeDriver::calculateChecksum(const uint8_t* data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i) sum += data[i];
    return 0xFF - sum;
}

void XBeeDriver::rxLoop() {
    while (running_) {
        uint8_t byte;
        if (uart_.read(&byte, 1, 50) <= 0) continue;

        if (byte != XBEE_START_DELIMITER) continue;

        uint8_t len_hi, len_lo;
        if (uart_.read(&len_hi, 1, 100) <= 0) continue;
        if (uart_.read(&len_lo, 1, 100) <= 0) continue;
        uint16_t frame_len = (static_cast<uint16_t>(len_hi) << 8) | len_lo;

        std::vector<uint8_t> api_frame(frame_len + 1);
        uart_.read(api_frame.data(), frame_len + 1, 500);

        if (api_frame.empty()) continue;

        uint8_t frame_type = api_frame[0];

        if (frame_type == XBEE_RX_INDICATOR || frame_type == XBEE_RX_FRAME_TYPE) {
            size_t payload_start = 12;
            if (payload_start < api_frame.size() - 1) {
                zigbee_mesh::core::ByteBuffer payload(
                    api_frame.data() + payload_start,
                    api_frame.size() - payload_start - 1);
                if (receive_cb_) {
                    receive_cb_(payload, 0, 0);
                }
            }
        }
    }
}

} // namespace zigbee_mesh::drivers
