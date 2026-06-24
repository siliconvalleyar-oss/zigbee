#include "drivers/cc2530.h"
#include "core/logger.h"
#include <cstring>
#include <thread>

namespace zigbee_mesh::drivers {

CC2530Driver::CC2530Driver() = default;
CC2530Driver::~CC2530Driver() { shutdown(); }

bool CC2530Driver::init(const RadioConfig& config) {
    device_path_ = config.device_path;
    channel_ = config.channel;
    tx_power_ = config.tx_power;

    if (!openSerial(config.device_path, config.baud_rate)) {
        ZIGBEE_LOG_ERROR("CC2530: Failed to open serial: %s", config.device_path.c_str());
        return false;
    }

    if (!resetChip()) {
        ZIGBEE_LOG_ERROR("CC2530: Failed to reset chip");
        return false;
    }

    zigbee_mesh::core::ByteBuffer resp;
    sendZnpCommand(static_cast<uint8_t>(ZnpCmd::SYS_VERSION), 0, {}, resp);

    zigbee_mesh::core::ByteBuffer panData;
    panData.appendU16LE(config.pan_id);
    sendZnpCommand(static_cast<uint8_t>(ZnpCmd::UTIL_SET_PANID), 0, panData, resp);

    zigbee_mesh::core::ByteBuffer chanData;
    uint32_t chan_mask = (1UL << (config.channel - 11));
    chanData.appendU32LE(chan_mask);
    sendZnpCommand(static_cast<uint8_t>(ZnpCmd::UTIL_SET_CHANNEL), 0, chanData, resp);

    zigbee_mesh::core::ByteBuffer startup;
    sendZnpCommand(static_cast<uint8_t>(ZnpCmd::ZDO_STARTUP_FROM_APP), 0, startup, resp, 3000);

    running_ = true;
    rx_thread_ = std::thread(&CC2530Driver::rxLoop, this);

    ZIGBEE_LOG_INFO("CC2530: Initialized on %s, channel %d", config.device_path.c_str(), channel_);
    return true;
}

void CC2530Driver::shutdown() {
    running_ = false;
    if (rx_thread_.joinable()) {
        rx_thread_.join();
    }
    closeSerial();
    ZIGBEE_LOG_INFO("CC2530: Shutdown");
}

bool CC2530Driver::isOpen() const {
    return uart_.isOpen();
}

bool CC2530Driver::send(const zigbee_mesh::core::ByteBuffer& frame) {
    if (!isOpen()) return false;

    zigbee_mesh::core::ByteBuffer request;
    request.appendU8(SOP_MARKER);
    request.appendU8(static_cast<uint8_t>(frame.size() >> 8));
    request.appendU8(static_cast<uint8_t>(frame.size() & 0xFF));
    request.appendU8(SREQ);
    request.append(frame);

    uint8_t checksum = 0;
    for (size_t i = 3; i < request.size(); ++i) {
        checksum ^= request[i];
    }
    request.appendU8(checksum);

    auto sent = uart_.write(request.data(), request.size());
    if (sent < 0) return false;

    zigbee_mesh::core::ByteBuffer resp;
    if (readFrame(resp, 500)) {
        if (transmit_cb_) {
            transmit_cb_(zigbee_mesh::core::ErrorCode::Success);
        }
        return true;
    }
    return false;
}

bool CC2530Driver::setChannel(uint8_t channel) {
    if (channel < 11 || channel > 26) return false;
    channel_ = channel;
    zigbee_mesh::core::ByteBuffer data;
    uint32_t chan_mask = (1UL << (channel - 11));
    data.appendU32LE(chan_mask);
    zigbee_mesh::core::ByteBuffer resp;
    return sendZnpCommand(static_cast<uint8_t>(ZnpCmd::UTIL_SET_CHANNEL), 0, data, resp);
}

bool CC2530Driver::setTxPower(int8_t power) {
    tx_power_ = power;
    zigbee_mesh::core::ByteBuffer data;
    data.appendU8(static_cast<uint8_t>(power));
    zigbee_mesh::core::ByteBuffer resp;
    return sendZnpCommand(static_cast<uint8_t>(ZnpCmd::ZDO_SET_TX_POWER), 0, data, resp);
}

bool CC2530Driver::setPanId(uint16_t pan_id) {
    zigbee_mesh::core::ByteBuffer data;
    data.appendU16LE(pan_id);
    zigbee_mesh::core::ByteBuffer resp;
    return sendZnpCommand(static_cast<uint8_t>(ZnpCmd::UTIL_SET_PANID), 0, data, resp);
}

bool CC2530Driver::setShortAddress(uint16_t addr) {
    zigbee_mesh::core::ByteBuffer data;
    data.appendU16LE(addr);
    zigbee_mesh::core::ByteBuffer resp;
    return sendZnpCommand(static_cast<uint8_t>(ZnpCmd::UTIL_SET_ADDR), 0, data, resp);
}

bool CC2530Driver::setExtendedAddress(uint64_t addr) {
    zigbee_mesh::core::ByteBuffer data;
    data.appendU64LE(addr);
    zigbee_mesh::core::ByteBuffer resp;
    return sendZnpCommand(static_cast<uint8_t>(ZnpCmd::SYS_SET_EXT_ADDR), 0, data, resp);
}

bool CC2530Driver::setPromiscuous(bool) {
    return true;
}

uint8_t CC2530Driver::getChannel() const { return channel_; }
int8_t CC2530Driver::getTxPower() const { return tx_power_; }
int8_t CC2530Driver::getRssi() const { return 0; }

bool CC2530Driver::resetChip() {
    zigbee_mesh::core::ByteBuffer data;
    data.appendU8(0x00);
    zigbee_mesh::core::ByteBuffer resp;
    sendZnpCommand(static_cast<uint8_t>(ZnpCmd::SYS_RESET_REQ), 0, data, resp, 3000);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    uart_.flush();
    return true;
}

bool CC2530Driver::sendZnpCommand(uint8_t cmd, uint8_t sub, const zigbee_mesh::core::ByteBuffer& data,
                                   zigbee_mesh::core::ByteBuffer& response, int timeout_ms) {
    zigbee_mesh::core::ByteBuffer frame;
    uint16_t total_len = 2 + data.size();
    frame.appendU8(SOP_MARKER);
    frame.appendU8(static_cast<uint8_t>((total_len >> 8) & 0xFF));
    frame.appendU8(static_cast<uint8_t>(total_len & 0xFF));
    frame.appendU8(cmd);
    frame.appendU8(sub);
    frame.append(data);

    uint8_t checksum = 0;
    for (size_t i = 3; i < frame.size(); ++i) {
        checksum ^= frame[i];
    }
    frame.appendU8(checksum);

    auto sent = uart_.write(frame.data(), frame.size());
    if (sent < 0) return false;

    return readFrame(response, timeout_ms);
}

bool CC2530Driver::readFrame(zigbee_mesh::core::ByteBuffer& frame, int timeout_ms) {
    auto start = std::chrono::steady_clock::now();

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed > timeout_ms) return false;

        uint8_t byte;
        auto n = uart_.read(&byte, 1, 10);
        if (n <= 0) continue;

        if (byte == SOP_MARKER) {
            uint8_t hi, lo;
            if (uart_.read(&hi, 1, 100) <= 0) continue;
            if (uart_.read(&lo, 1, 100) <= 0) continue;
            uint16_t len = (static_cast<uint16_t>(hi) << 8) | lo;

            uint8_t cmd_byte;
            if (uart_.read(&cmd_byte, 1, 100) <= 0) continue;

            std::vector<uint8_t> payload(len > 0 ? len - 1 : 0);
            if (payload.size() > 0) {
                uart_.read(payload.data(), payload.size(), 500);
            }

            uint8_t checksum;
            if (uart_.read(&checksum, 1, 100) <= 0) continue;

            frame.clear();
            frame.appendU8(cmd_byte);
            frame.append(payload.data(), payload.size());
            return true;
        }
    }
}

void CC2530Driver::closeSerial() {
    uart_.close();
}

void CC2530Driver::rxLoop() {
    while (running_) {
        zigbee_mesh::core::ByteBuffer frame;
        if (readFrame(frame, 100)) {
            if (frame.size() > 0 && frame[0] == 0x90) {
                zigbee_mesh::core::ByteBuffer payload(frame.data() + 1, frame.size() - 1);
                if (receive_cb_) {
                    receive_cb_(payload, 0, 0);
                }
            }
        }
    }
}

} // namespace zigbee_mesh::drivers
