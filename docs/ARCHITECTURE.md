# Architecture

## Overview

Zigbee Mesh Multiplatform follows Clean Architecture principles with clear separation of concerns across multiple layers.

## Layer Structure

### 1. Core Layer (`include/core/`)
Foundation types and utilities:
- `types.h` - Fundamental types (addresses, enums, structs)
- `result.h` - Error handling with Result<T,E>
- `event.h` - Event system (emitter, queue, callbacks)
- `config.h` - Configuration management
- `logger.h` - Logging system (spdlog wrapper)
- `timer.h` - Timer utilities
- `bytebuffer.h` - Binary data handling

### 2. Drivers Layer (`include/drivers/`)
Hardware abstraction:
- `hal.h` - Abstract radio driver interface
- `backend.h` - SPI and UART backend implementations
- `mrf24j40.h` - MRF24J40 radio driver
- `cc2530.h` - CC2530/CC2531 radio driver
- `xbee.h` - XBee radio driver

### 3. Zigbee Stack Layer (`include/zigbee/`)
Protocol implementation:
- `ieee802154.h` - IEEE 802.15.4 frame handling
- `security_manager.h` - AES-128 encryption/decryption
- `zdo_layer.h` - Zigbee Device Objects
- `zigbee_stack.h` - Main protocol stack

### 4. Network Layer (`include/network/`)
Network utilities:
- `routing.h` - Routing table and route discovery

### 5. Mesh Layer (`include/mesh/`)
Mesh networking:
- `mesh_manager.h` - Topology management and path finding

### 6. Services Layer (`include/services/`)
High-level services:
- `network_manager.h` - Network lifecycle management
- Mesh service, diagnostics, OTA update

### 7. Security Layer (`include/security/`)
Security services:
- `security_service.h` - Key management, encryption

### 8. Storage Layer (`include/storage/`)
Persistent storage:
- `storage_manager.h` - SQLite database operations

### 9. CLI Layer (`include/cli/`)
Command-line interface:
- `cli.h` - Interactive CLI console

## Data Flow

```
User Input → CLI → NetworkManager → ZigbeeStack → IEEE 802.15.4 → Radio Driver
                                    ↕
                              MeshManager → RoutingTable
                                    ↕
                              StorageManager → SQLite
```

## Design Patterns

- **Factory Pattern**: Radio driver creation
- **Observer Pattern**: Event emission and callbacks
- **Strategy Pattern**: Pluggable encryption backends
- **Repository Pattern**: Storage abstraction
- **Singleton Pattern**: Logger, Config

## Thread Model

- Main thread: CLI or daemon loop
- RX thread: Radio receive handling
- Timer threads: Periodic tasks
- All shared state protected by mutexes
