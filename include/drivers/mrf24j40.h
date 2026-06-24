#pragma once

#include "hal.h"
#include "backend.h"

namespace zigbee_mesh::drivers {

class MRF24J40Driver : public RadioDriver {
public:
    MRF24J40Driver();
    ~MRF24J40Driver() override;

    bool init(const RadioConfig& config) override;
    void shutdown() override;
    bool isOpen() const override;

    bool send(const zigbee_mesh::core::ByteBuffer& frame) override;
    bool setChannel(uint8_t channel) override;
    bool setTxPower(int8_t power) override;
    bool setPanId(uint16_t pan_id) override;
    bool setShortAddress(uint16_t addr) override;
    bool setExtendedAddress(uint64_t addr) override;
    bool setPromiscuous(bool enabled) override;

    uint8_t getChannel() const override;
    int8_t getTxPower() const override;
    int8_t getRssi() const override;
    RadioType getType() const override { return RadioType::MRF24J40; }
    BackendType getBackend() const override { return BackendType::SPI; }
    std::string getDevicePath() const override { return device_path_; }
    std::string getDeviceInfo() const override { return "MRF24J40 (SPI)"; }

private:
    static constexpr uint16_t REG_TXCTRL = 0x130;
    static constexpr uint16_t REG_CHANNEL = 0x32;
    static constexpr uint16_t REG_TXSTATE = 0x36;
    static constexpr uint16_t REG_BBREG0 = 0x38;
    static constexpr uint16_t REG_BBREG1 = 0x39;
    static constexpr uint16_t REG_BBREG2 = 0x3A;
    static constexpr uint16_t REG_BBREG6 = 0x3E;
    static constexpr uint16_t REG_INTCON = 0x31;
    static constexpr uint16_t REG_INTSTAT = 0x31;
    static constexpr uint16_t REG_RFCTRL0 = 0x200;
    static constexpr uint16_t REG_RFCTRL1 = 0x201;
    static constexpr uint16_t REG_RFCTRL2 = 0x202;
    static constexpr uint16_t REG_RFCTRL3 = 0x203;
    static constexpr uint16_t REG_RFCTRL6 = 0x206;
    static constexpr uint16_t REG_RFCTRL7 = 0x207;
    static constexpr uint16_t REG_RXMCR = 0x300;
    static constexpr uint16_t REG_PANIDL = 0x301;
    static constexpr uint16_t REG_PANIDH = 0x302;
    static constexpr uint16_t REG_SADRL = 0x303;
    static constexpr uint16_t REG_SADRH = 0x304;
    static constexpr uint16_t REG_EADR0 = 0x305;
    static constexpr uint16_t REG_FIFO = 0x000;
    static constexpr uint16_t REG_TXNORMALFIFO = 0x000;
    static constexpr uint16_t REG_RXNORMALFIFO = 0x300;
    static constexpr uint16_t REG_TXGTSFIFO = 0x100;

    static constexpr uint8_t TX_FRAME_LEN_OFFSET = 0;

    bool reset();
    bool writeTxFifo(const zigbee_mesh::core::ByteBuffer& frame);
    bool startTx();
    void readRxFifo(zigbee_mesh::core::ByteBuffer& frame, int8_t& rssi, uint8_t& lqi);
    void rxLoop();

    SPIBackend spi_;
    std::string device_path_;
    uint8_t channel_{11};
    int8_t tx_power_{0};
    bool running_{false};
    std::thread rx_thread_;
};

} // namespace zigbee_mesh::drivers
