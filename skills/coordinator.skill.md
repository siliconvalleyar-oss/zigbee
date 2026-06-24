# Coordinator Skill

## Role
The Coordinator is the most capable device in a Zigbee network. It is responsible for forming and managing the network.

## Responsibilities
- **Network Formation**: Creates the PAN, selects the channel, and initializes the network
- **Address Assignment**: Allocates short addresses to joining devices
- **Security Management**: Distributes network keys and manages trust center
- **Network Management**: Monitors network health, manages device table
- **Routing**: Maintains routing tables and facilitates multi-hop communication
- **Binding**: Manages binding tables for direct device-to-device communication
- **Commissioning**: Handles device join/rejoin/leave procedures
- **Trust Center**: Authenticates devices and distributes link keys

## Commands
- `network create [channel] [pan_id]` - Form a new network
- `network status` - Show network information
- `device list` - List all devices in the network
- `device info <addr>` - Get detailed device information
- `security keys` - Show security key status
- `mesh topology` - Display network topology
- `mesh routes` - Show routing table
- `mesh diagnostics` - Show network diagnostics

## Configuration
```ini
[device]
role = 0  # Coordinator

[network]
pan_id = 0x1234
channel = 11

[security]
trust_center = true
require_auth = true
```

## Topology
```
           +----------+
           |Coordinator|
           | (Pi #1)  |
           +----+-----+
                |
        +-------+-------+
        |               |
   +----+----+    +----+----+
   | Router 1|    | Router 2|
   | (Pi #2) |    | (Pi #3) |
   +----+----+    +----+----+
        |               |
   +----+----+    +----+----+
   |EndDevice|    |EndDevice|
   |  (Pi #4) |   |  (Pi #5) |
   +----------+    +----------+
```
