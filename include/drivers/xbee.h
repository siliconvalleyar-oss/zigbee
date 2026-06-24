#pragma once

#include "hal.h"
#include "backend.h"

namespace zigbee_mesh::drivers {

class XBeeDriver : public RadioDriver {
public:
    XBeeDriver();
    ~XBeeDriver() override;

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
    RadioType getType() const override { return RadioType::XBee; }
    BackendType getBackend() const override { return BackendType::UART; }
    std::string getDevicePath() const override { return device_path_; }
    std::string getDeviceInfo() const override { return "XBee (UART)"; }

    bool sendATCommand(const std::string& cmd, const std::string& param = "", std::string* response = nullptr, int timeout_ms = 1000);
    bool setNodeIdentifier(const std::string& id);
    bool getHardwareVersion(std::string& version);
    bool getFirmwareVersion(std::string& version);

private:
    static constexpr uint8_t XBEE_START_DELIMITER = 0x7E;
    static constexpr uint8_t XBEE_AT_FRAME_TYPE = 0x08;
    static constexpr uint8_t XBEE_AT_RESPONSE = 0x88;
    static constexpr uint8_t XBEE_TX_FRAME_TYPE = 0x10;
    static constexpr uint8_t XBEE_RX_FRAME_TYPE = 0x90;
    static constexpr uint8_t XBEE_RX_INDICATOR = 0x90;

    bool parseFrame(zigbee_mesh::core::ByteBuffer& frame);
    bool escapeByte(uint8_t byte, std::vector<uint8_t>& output);
    uint8_t calculateChecksum(const uint8_t* data, size_t len);
    void rxLoop();

    UARTBackend uart_;
    std::string device_path_;
    uint8_t channel_{11};
    int8_t tx_power_{0};
    bool running_{false};
    std::thread rx_thread_;
    uint8_t frame_id_{0};
};

} // namespace zigbee_mesh::drivers
