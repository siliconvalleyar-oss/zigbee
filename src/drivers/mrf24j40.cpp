#include "drivers/mrf24j40.h"
#include "core/logger.h"
#include <cstring>
#include <thread>

namespace zigbee_mesh::drivers {

MRF24J40Driver::MRF24J40Driver() = default;

MRF24J40Driver::~MRF24J40Driver() {
    shutdown();
}

bool MRF24J40Driver::init(const RadioConfig& config) {
    device_path_ = config.device_path;

    if (!spi_.open(config.device_path, 1000000)) {
        ZIGBEE_LOG_ERROR("MRF24J40: Failed to open SPI device: %s", config.device_path.c_str());
        return false;
    }

    if (!reset()) {
        ZIGBEE_LOG_ERROR("MRF24J40: Failed to reset device");
        return false;
    }

    channel_ = config.channel;
    tx_power_ = config.tx_power;

    setChannel(channel_);
    setTxPower(tx_power_);
    setPanId(config.pan_id);
    setShortAddress(config.short_addr);
    setExtendedAddress(config.extended_addr);

    spi_.writeRegister(REG_BBREG0, 0x00);
    spi_.writeRegister(REG_BBREG1, 0xC0);

    if (config.auto_ack) {
        spi_.writeRegister(REG_RXMCR, 0x00);
    } else {
        spi_.writeRegister(REG_RXMCR, 0x01);
    }

    if (config.promiscuous) {
        spi_.writeRegister(REG_RXMCR, 0x08);
    }

    spi_.writeRegister(REG_BBREG6, 0x40);
    spi_.writeRegister(REG_INTCON, 0x00);

    running_ = true;
    rx_thread_ = std::thread(&MRF24J40Driver::rxLoop, this);

    ZIGBEE_LOG_INFO("MRF24J40: Initialized on %s, channel %d, PAN 0x%04X",
                     config.device_path.c_str(), channel_, config.pan_id);
    return true;
}

void MRF24J40Driver::shutdown() {
    running_ = false;
    if (rx_thread_.joinable()) {
        rx_thread_.join();
    }
    spi_.close();
    ZIGBEE_LOG_INFO("MRF24J40: Shutdown");
}

bool MRF24J40Driver::isOpen() const {
    return spi_.isOpen();
}

bool MRF24J40Driver::reset() {
    spi_.writeRegister(0x36, 0x80);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    spi_.writeRegister(0x36, 0x00);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return true;
}

bool MRF24J40Driver::send(const zigbee_mesh::core::ByteBuffer& frame) {
    if (!isOpen()) return false;

    uint8_t state = spi_.readRegister(REG_TXSTATE);
    if (state & 0x01) {
        ZIGBEE_LOG_WARN("MRF24J40: TX FIFO busy, retrying");
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    if (!writeTxFifo(frame)) {
        return false;
    }

    return startTx();
}

bool MRF24J40Driver::writeTxFifo(const zigbee_mesh::core::ByteBuffer& frame) {
    uint8_t len = static_cast<uint8_t>(frame.size());
    spi_.writeRegister(REG_TXNORMALFIFO, len);

    std::vector<uint8_t> data(frame.size());
    std::memcpy(data.data(), frame.data(), frame.size());
    spi_.writeBurst(REG_TXNORMALFIFO + 1, data.data(), data.size());
    return true;
}

bool MRF24J40Driver::startTx() {
    spi_.writeRegister(REG_TXCTRL + 1, 0x01);

    int retries = 10;
    while (retries-- > 0) {
        uint8_t state = spi_.readRegister(REG_TXSTATE);
        if ((state & 0x20) == 0) break;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    spi_.writeRegister(REG_TXCTRL + 1, 0x00);

    if (transmit_cb_) {
        transmit_cb_(zigbee_mesh::core::ErrorCode::Success);
    }
    return true;
}

bool MRF24J40Driver::setChannel(uint8_t channel) {
    if (channel < 11 || channel > 26) return false;
    channel_ = channel;
    uint8_t reg = spi_.readRegister(REG_RFCTRL0);
    reg = (reg & 0xE0) | ((channel - 11) << 5);
    spi_.writeRegister(REG_RFCTRL0, reg);
    return true;
}

bool MRF24J40Driver::setTxPower(int8_t power) {
    tx_power_ = power;
    uint8_t val;
    if (power <= -30) val = 0x00;
    else if (power <= -20) val = 0x01;
    else if (power <= -10) val = 0x02;
    else if (power <= 0) val = 0x03;
    else val = 0x04;
    spi_.writeRegister(REG_TXCTRL, val);
    return true;
}

bool MRF24J40Driver::setPanId(uint16_t pan_id) {
    spi_.writeRegister(REG_PANIDL, pan_id & 0xFF);
    spi_.writeRegister(REG_PANIDH, (pan_id >> 8) & 0xFF);
    return true;
}

bool MRF24J40Driver::setShortAddress(uint16_t addr) {
    spi_.writeRegister(REG_SADRL, addr & 0xFF);
    spi_.writeRegister(REG_SADRH, (addr >> 8) & 0xFF);
    return true;
}

bool MRF24J40Driver::setExtendedAddress(uint64_t addr) {
    for (int i = 0; i < 8; ++i) {
        spi_.writeRegister(REG_EADR0 + i, (addr >> (i * 8)) & 0xFF);
    }
    return true;
}

bool MRF24J40Driver::setPromiscuous(bool enabled) {
    uint8_t val = spi_.readRegister(REG_RXMCR);
    if (enabled) {
        val |= 0x08;
    } else {
        val &= ~0x08;
    }
    spi_.writeRegister(REG_RXMCR, val);
    return true;
}

uint8_t MRF24J40Driver::getChannel() const { return channel_; }
int8_t MRF24J40Driver::getTxPower() const { return tx_power_; }

int8_t MRF24J40Driver::getRssi() const {
    if (!isOpen()) return -128;
    uint8_t rssi_val = spi_.readRegister(REG_BBREG6);
    return static_cast<int8_t>(rssi_val) - 91;
}

void MRF24J40Driver::readRxFifo(zigbee_mesh::core::ByteBuffer& frame, int8_t& rssi, uint8_t& lqi) {
    uint8_t len = spi_.readRegister(REG_RXNORMALFIFO);
    if (len > 0 && len < 128) {
        std::vector<uint8_t> data(len);
        spi_.readBurst(REG_RXNORMALFIFO + 1, data.data(), len);
        frame = zigbee_mesh::core::ByteBuffer(std::move(data));

        rssi = static_cast<int8_t>(spi_.readRegister(REG_BBREG6)) - 91;
        uint8_t lqi_raw = spi_.readRegister(REG_BBREG2);
        lqi = lqi_raw & 0x7F;
    }
}

void MRF24J40Driver::rxLoop() {
    while (running_) {
        uint8_t int_stat = spi_.readRegister(REG_INTSTAT);
        if (int_stat & 0x01) {
            zigbee_mesh::core::ByteBuffer frame;
            int8_t rssi;
            uint8_t lqi;
            readRxFifo(frame, rssi, lqi);
            if (!frame.empty() && receive_cb_) {
                receive_cb_(frame, rssi, lqi);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace zigbee_mesh::drivers
