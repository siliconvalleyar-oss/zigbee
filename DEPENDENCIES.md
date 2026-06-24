# Dependencies

## Required

| Library | Version | Purpose |
|---------|---------|---------|
| C++20 Compiler | GCC 11+ / Clang 14+ | C++20 features |
| CMake | 3.20+ | Build system |
| OpenSSL | 1.1+ | AES-128 encryption |
| SQLite3 | 3.35+ | Persistent storage |
| Threads | POSIX/Win32 | Multi-threading |

## Optional

| Library | Version | Purpose |
|---------|---------|---------|
| GoogleTest | 1.14+ | Unit testing |
| spdlog | 1.12+ | Advanced logging (bundled fallback) |
| nlohmann/json | 3.11+ | JSON configuration (bundled fallback) |

## Installation

### Ubuntu/Debian
```bash
sudo apt install -y build-essential cmake pkg-config \
    libssl-dev libsqlite3-dev
```

### Raspberry Pi OS
```bash
sudo apt install -y build-essential cmake pkg-config \
    libssl-dev libsqlite3-dev libi2c-dev
```

### macOS
```bash
brew install cmake openssl sqlite
```

### Windows (vcpkg)
```bash
vcpkg install openssl sqlite3 gtest
```

## Zigbee Libraries (Optional)

For production use, consider integrating with:

| Library | Vendor | Purpose |
|---------|--------|---------|
| Z-Stack | Texas Instruments | CC2530/CC2531 firmware |
| EmberZNet | Silicon Labs | EFR32 support |
| ZBOSS | Synopsys | Generic Zigbee stack |
| zigbee-herdsman | Koenkk | Node.js Zigbee library |

## Hardware

### Supported Transceivers

| Device | Interface | Notes |
|--------|-----------|-------|
| MRF24J40 | SPI | Microchip, 2.4GHz |
| CC2530 | UART | Texas Instruments, SoC |
| CC2531 | USB | Texas Instruments, USB dongle |
| CC2652 | UART | Texas Instruments, multiprotocol |
| EFR32 | UART | Silicon Labs, Zigbee 3.0 |
| XBee | UART | Digi, AT/API mode |

### Raspberry Pi Pinout

#### MRF24J40 (SPI)
```
MRF24J40    Raspberry Pi
────────    ────────────
VCC      →  3.3V
GND      →  GND
SDI      →  MOSI (GPIO 10)
SDO      →  MISO (GPIO 9)
SCLK     →  SCLK (GPIO 11)
CS       →  CE0  (GPIO 8)
INT      →  GPIO 17
RESET    →  GPIO 27
```
