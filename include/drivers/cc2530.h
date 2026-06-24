#pragma once

#include "hal.h"
#include "backend.h"

namespace zigbee_mesh::drivers {

class CC2530Driver : public RadioDriver {
public:
    CC2530Driver();
    ~CC2530Driver() override;

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
    RadioType getType() const override { return RadioType::CC2530; }
    BackendType getBackend() const override { return BackendType::UART; }
    std::string getDevicePath() const override { return device_path_; }
    std::string getDeviceInfo() const override { return "CC2530 (UART)"; }

    bool sendZnpCommand(uint8_t cmd, uint8_t sub, const zigbee_mesh::core::ByteBuffer& data, zigbee_mesh::core::ByteBuffer& response, int timeout_ms = 1000);
    bool resetChip();
    bool enterBootloader();

private:
    static constexpr uint8_t SOP_MARKER = 0xFE;
    static constexpr uint8_t SREQ = 0x21;
    static constexpr uint8_t AREQ = 0x41;
    static constexpr uint8_t SRSP = 0x61;

    enum class ZnpCmd : uint8_t {
        SYS_RESET_REQ = 0x00,
        SYS_VERSION = 0x21,
        SYS_SET_EXT_ADDR = 0x26,
        SYS_OSAL_GET_NV = 0x08,
        SYS_OSAL_SET_NV = 0x09,
        AF_REGISTER = 0x2E,
        AF_DATA_REQUEST = 0x21,
        AF_DATA_CONFIRM = 0x60,
        ZDO_ACTIVE_EP_REQ = 0x2C,
        ZDO_IEEE_ADDR_REQ = 0x31,
        ZDO_NWK_ADDR_REQ = 0x30,
        ZDO_SIMPLE_DESC_REQ = 0x2D,
        ZDO_MGMT_LQI_REQ = 0x31,
        ZDO_STARTUP_FROM_APP = 0x40,
        ZDO_SET_TX_POWER = 0xFC,
        UTIL_SET_PANID = 0x62,
        UTIL_SET_ADDR = 0x63,
        UTIL_SET_CHANNEL = 0x6B,
    };

    bool openSerial(const std::string& device, uint32_t baud_rate);
    void closeSerial();
    bool readFrame(zigbee_mesh::core::ByteBuffer& frame, int timeout_ms);
    bool writeFrame(const zigbee_mesh::core::ByteBuffer& frame);
    void rxLoop();

    UARTBackend uart_;
    std::string device_path_;
    uint8_t channel_{11};
    int8_t tx_power_{0};
    bool running_{false};
    std::thread rx_thread_;
    uint8_t sequence_{0};
};

} // namespace zigbee_mesh::drivers
