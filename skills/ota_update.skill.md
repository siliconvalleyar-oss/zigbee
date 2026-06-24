# OTA Update Skill

## Role
Over-The-Air (OTA) update manages firmware upgrades for Zigbee devices in the network.

## Responsibilities
- **Firmware Management**: Downloads, verifies, and applies firmware updates
- **Version Control**: Tracks current and target firmware versions
- **Progress Tracking**: Reports download and installation progress
- **Rollback**: Supports reverting to previous firmware on failure
- **Verification**: Validates firmware integrity before installation
- **Selective Update**: Updates specific devices or all devices in network

## Commands
- `ota update <url>` - Start firmware update from URL
- `ota status` - Check update progress
- `ota cancel` - Cancel ongoing update
- `ota version` - Show current firmware version

## Update Process
```
Coordinator                Server
    |                        |
    |-- GET firmware.bin --->|
    |<-- 200 OK (data) -----|
    |                        |
    |  Verify checksum       |
    |  Verify signature      |
    |                        |
Device 1    Device 2    Device 3
    |           |           |
    |-- Data -->|           |
    |           |-- Data -->|
    |           |           |-- Data -->
    |           |           |
    |-- Verify -|-- Verify -|-- Verify
    |           |           |
    |  Apply    |  Apply    |  Apply
```

## Safety Measures
- Checksum verification before installation
- Signature verification for authenticated firmware
- Rollback on boot failure
- Power-fail safe (A/B partition)
- Watchdog timer during update
