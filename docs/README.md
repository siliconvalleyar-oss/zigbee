# Zigbee Mesh Multiplatform

Professional Zigbee 3.0 mesh networking platform in C++20 for Raspberry Pi.

## Features

- **Zigbee 3.0** compliant (IEEE 802.15.4)
- **Mesh networking** with self-healing routing
- **Multi-platform**: Linux, macOS, Windows
- **Clean Architecture**: Modular, testable, maintainable
- **Multiple radio support**: MRF24J40, CC2530, CC2531, XBee
- **CLI administration** console
- **SQLite storage** for persistent data
- **AES-128 security** with Trust Center
- **OTA firmware updates**

## Quick Start

```bash
# Clone and build
git clone https://github.com/your-repo/zigbee-mesh.git
cd zigbee-mesh
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run CLI
./bin/zigbee-cli

# Create a network
zigbee> network create 11 0x1234
```

## Supported Hardware

| Radio       | Backend | Driver    |
|-------------|---------|-----------|
| MRF24J40    | SPI     | Native    |
| CC2530/2531 | UART    | Z-Stack   |
| XBee        | UART    | AT API    |
| CC2652      | UART    | Custom    |
| EFR32       | UART    | EmberZNet |

## Architecture

```
┌─────────────────────────────────────┐
│          Application Layer           │
├─────────────────────────────────────┤
│            CLI / Services            │
├─────────────────────────────────────┤
│          Zigbee Stack (ZDO/APS)      │
├─────────────────────────────────────┤
│           Mesh / Routing             │
├─────────────────────────────────────┤
│         IEEE 802.15.4 MAC            │
├─────────────────────────────────────┤
│           HAL (Drivers)              │
├─────────────────────────────────────┤
│    MRF24J40 | CC2530 | XBee         │
└─────────────────────────────────────┘
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [API Reference](docs/API.md)
- [Build Guide](docs/BUILD.md)
- [Dependencies](docs/DEPENDENCIES.md)
- [Network Protocol](docs/NETWORK.md)
- [Security](docs/SECURITY.md)
- [Roadmap](docs/ROADMAP.md)
- [Contributing](docs/CONTRIBUTING.md)
- [Rules](docs/RULES.md)
- [TODO](docs/TODO.md)

## License

MIT License - See [LICENSE](LICENSE) for details.
