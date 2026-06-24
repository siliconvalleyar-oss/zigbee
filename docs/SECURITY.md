# Security

## Overview

Zigbee Mesh implements Zigbee 3.0 security with AES-128 encryption, Trust Center support, and device authentication.

## Security Architecture

### Encryption Layers

1. **Network Layer Security**: AES-128-CCM encryption of NWK frames
2. **APS Layer Security**: Link key encryption for application data
3. **Transport Layer**: Optional TLS for management interfaces

### Key Types

| Key Type | Size | Purpose |
|----------|------|---------|
| Network Key | 128-bit | Encrypts all NWK frames |
| Link Key | 128-bit | Encrypts APS frames per device pair |
| Master Key | 128-bit | Derives link keys from install codes |
| Install Code | 16-18 byte | Pre-configured code for key derivation |

### Frame Counter

Each key maintains a 32-bit frame counter to prevent replay attacks:
- Outgoing counter incremented on each transmission
- Incoming counter validated on reception
- Out-of-sequence frames rejected

## Trust Center

The Trust Center (TC) is typically the Coordinator:

### Responsibilities
- Distributes network key to joining devices
- Authenticates devices via install codes
- Manages whitelist/blacklist
- Issues link keys for secure communication

### Join Process with Trust Center
```
Device                    Trust Center
  |                          |
  |-- Association Req ------>|
  |                          |
  |<-- Association Resp -----|
  |   (includes NWK key)     |
  |                          |
  |-- Transport Key Confirm ->|
  |                          |
  |-- Device Announce ------->|
  |                          |
  |-- Node Descriptor Req --->|
  |<-- Node Descriptor Resp --|
```

## AES-128-CCM

### Algorithm
- **AES-128**: Block cipher for encryption
- **CCM***: Counter with CBC-MAC for authenticated encryption

### Parameters
- **Nonce**: 13 bytes (frame counter + source address)
- **MIC**: 4 bytes (Message Integrity Code)
- **Key**: 128-bit network or link key

### Nonce Structure
```
┌──────────────┬──────────┬──────────┬──────────┐
│ Frame Counter│ Src Addr │ Src Ext  │ Security │
│  (4 bytes)   │ (2 bytes)│ (8 bytes)│Level(1B) │
└──────────────┴──────────┴──────────┴──────────┘
```

## Whitelist/Blacklist

### Whitelist
- Devices allowed to join the network
- Checked during association process
- Empty whitelist = all devices allowed

### Blacklist
- Devices explicitly denied access
- Checked before whitelist
- Prevents known compromised devices

## Security Levels

| Level | Name | Encryption | MIC |
|-------|------|------------|-----|
| 0x00 | None | No | No |
| 0x01 | MIC-32 | No | 32-bit |
| 0x02 | MIC-64 | No | 64-bit |
| 0x03 | MIC-128 | No | 128-bit |
| 0x04 | ENC-MIC-32 | Yes | 32-bit |
| 0x05 | ENC-MIC-64 | Yes | 64-bit |
| 0x06 | ENC-MIC-128 | Yes | 128-bit |

## Best Practices

1. Enable Trust Center for production networks
2. Use install codes for device authentication
3. Rotate network keys periodically
4. Monitor frame counter for anomalies
5. Maintain whitelist/blacklist for access control
6. Enable MIC for all production frames
