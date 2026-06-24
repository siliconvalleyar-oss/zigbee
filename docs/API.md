# API Reference

## Core Types

### `zigbee_mesh::core::MacAddress`
```cpp
struct MacAddress {
    ShortAddress short_addr;     // 16-bit address
    ExtendedAddress extended_addr; // 64-bit address

    bool isShort() const;
    bool isExtended() const;
    std::string toString() const;
};
```

### `zigbee_mesh::core::Result<T>`
```cpp
template<typename T>
class Result {
    bool ok() const;
    bool hasError() const;
    const T& value() const;
    T valueOr(T default_val) const;
    ErrorCode errorCode() const;
    std::string errorMessage() const;
};
```

## Drivers

### `zigbee_mesh::drivers::RadioDriver`
```cpp
class RadioDriver {
    virtual bool init(const RadioConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual bool send(const ByteBuffer& frame) = 0;
    virtual bool setChannel(uint8_t channel) = 0;
    virtual bool setTxPower(int8_t power) = 0;
    virtual void setReceiveCallback(ReceiveCallback cb) = 0;
};
```

## Zigbee Stack

### `zigbee_mesh::zigbee::ZigbeeStack`
```cpp
class ZigbeeStack {
    bool init(RadioDriver* radio, const Config& config);
    bool start();
    void stop();

    bool formNetwork(uint8_t channel, PanId pan_id);
    bool joinNetwork(PanId pan_id, uint8_t channel);
    bool leaveNetwork();

    bool sendPayload(ShortAddress dest, uint16_t cluster_id,
                     uint8_t endpoint, const ByteBuffer& payload);
    bool permitJoin(uint8_t duration);

    StackState getState() const;
    NetworkInfo getNetworkInfo() const;
};
```

## Mesh

### `zigbee_mesh::mesh::MeshManager`
```cpp
class MeshManager {
    bool init(RoutingTable* routing_table);
    bool addNode(const MeshNode& node);
    bool removeNode(ShortAddress addr);
    TopologySnapshot getTopology() const;
    std::string generateAsciiTopology() const;
};
```

## Services

### `zigbee_mesh::services::NetworkManager`
```cpp
class NetworkManager {
    bool init(const Config& config);
    bool start();
    void stop();

    bool createNetwork(uint8_t channel, PanId pan_id);
    bool joinNetwork(PanId pan_id, uint8_t channel);
    bool leaveNetwork();
    bool permitJoin(uint8_t duration);

    NetworkInfo getNetworkInfo() const;
    std::vector<DeviceTableEntry> getDevices() const;
};
```

## Storage

### `zigbee_mesh::storage::StorageManager`
```cpp
class StorageManager {
    bool open(const std::string& db_path);
    void close();

    bool saveDevice(const StoredDeviceInfo& device);
    bool getDevice(ExtendedAddress ieee_addr, StoredDeviceInfo& device) const;
    std::vector<StoredDeviceInfo> getAllDevices() const;

    bool saveNetworkInfo(const StoredNetworkInfo& info);
    bool getNetworkInfo(StoredNetworkInfo& info) const;
};
```

## CLI

### `zigbee_mesh::cli::ZigbeeCli`
```cpp
class ZigbeeCli {
    bool init(NetworkManager* network);
    void run();
    void stop();

    bool processCommand(const std::string& input);
    void registerCommand(const std::string& name,
                        const std::string& description,
                        CommandHandler handler);
};
```
