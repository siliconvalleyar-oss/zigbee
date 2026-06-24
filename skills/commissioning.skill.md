# Commissioning Skill

## Role
Commissioning handles device join, rejoin, leave, and network configuration procedures.

## Responsibilities
- **Device Discovery**: Scans for available networks and devices
- **Join Procedure**: Authenticates and adds devices to the network
- **Rejoin**: Handles network re-entry after disconnection
- **Leave**: Graceful removal of devices from the network
- **Address Assignment**: Allocates short addresses to new devices
- **Key Distribution**: Distributes security keys during join process
- **Permit Join**: Enables/disables network joining temporarily

## Commands
- `network scan` - Scan for available networks
- `network join <pan_id> <channel>` - Join a network
- `network leave` - Leave the current network
- `network permit_join [duration]` - Enable device joining

## Join Process
```
Device                    Network
  |                          |
  |-- Association Req ------>|
  |<-- Association Resp -----|
  |                          |
  |-- Device Announce ------>|
  |                          |
  |-- Node Descriptor Req -->|
  |<-- Node Descriptor Resp -|
  |                          |
  |-- Active Endpoint Req -->|
  |<-- Active Endpoint Resp -|
  |                          |
  |-- Simple Descriptor Req->|
  |<-- Simple Descriptor Resp|
```

## Key Distribution
1. Device sends install code or pre-configured key
2. Coordinator derives link key from install code
3. Trust center sends encrypted network key
4. Device decrypts and installs network key
5. Device sends confirm to trust center
