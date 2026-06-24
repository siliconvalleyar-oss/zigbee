#pragma once

#include "core/types.h"
#include "core/bytebuffer.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace zigbee_mesh::drivers {

enum class RadioType : uint8_t {
    Unknown = 0,
    MRF24J40,
    CC2530,
    CC2531,
    CC2652,
    EFR32,
    XBee,
};

enum class BackendType : uint8_t {
    SPI,
    UART,
    USB,
    Unknown,
};

struct RadioConfig {
    RadioType type{RadioType::Unknown};
    BackendType backend{BackendType::Unknown};
    std::string device_path{"/dev/ttyUSB0"};
    uint32_t baud_rate{115200};
    uint8_t channel{11};
    int8_t tx_power{0};
    uint16_t pan_id{0xFFFF};
    uint16_t short_addr{0xFFFF};
    uint64_t extended_addr{0};
    bool auto_ack{true};
    bool promiscuous{false};
    bool security_enabled{false};
};

using ReceiveCallback = std::function<void(const zigbee_mesh::core::ByteBuffer& frame, int8_t rssi, uint8_t lqi)>;
using TransmitCallback = std::function<void(zigbee_mesh::core::ErrorCode status)>;

class RadioDriver {
public:
    virtual ~RadioDriver() = default;

    virtual bool init(const RadioConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual bool isOpen() const = 0;

    virtual bool send(const zigbee_mesh::core::ByteBuffer& frame) = 0;
    virtual bool setChannel(uint8_t channel) = 0;
    virtual bool setTxPower(int8_t power) = 0;
    virtual bool setPanId(uint16_t pan_id) = 0;
    virtual bool setShortAddress(uint16_t addr) = 0;
    virtual bool setExtendedAddress(uint64_t addr) = 0;
    virtual bool setPromiscuous(bool enabled) = 0;

    virtual uint8_t getChannel() const = 0;
    virtual int8_t getTxPower() const = 0;
    virtual int8_t getRssi() const = 0;
    virtual RadioType getType() const = 0;
    virtual BackendType getBackend() const = 0;
    virtual std::string getDevicePath() const = 0;

    virtual void setReceiveCallback(ReceiveCallback cb) { receive_cb_ = std::move(cb); }
    virtual void setTransmitCallback(TransmitCallback cb) { transmit_cb_ = std::move(cb); }

    virtual std::string getDeviceInfo() const = 0;

protected:
    ReceiveCallback receive_cb_;
    TransmitCallback transmit_cb_;
};

using RadioDriverPtr = std::unique_ptr<RadioDriver>;

RadioDriverPtr createRadioDriver(RadioType type);

} // namespace zigbee_mesh::drivers
