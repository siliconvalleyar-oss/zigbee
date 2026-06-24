# Build Guide

## Prerequisites

- C++20 compatible compiler (GCC 11+, Clang 14+)
- CMake 3.20+
- OpenSSL development libraries
- SQLite3 development libraries
- GoogleTest (optional, for tests)

## Build Instructions

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
    libssl-dev libsqlite3-dev git

# Clone repository
git clone https://github.com/your-repo/zigbee-mesh.git
cd zigbee-mesh

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run tests
ctest --output-on_failure

# Install
sudo make install
```

### Raspberry Pi OS

```bash
# Additional dependencies for SPI
sudo apt install -y libi2c-dev

# Enable SPI
sudo raspi-config
# Interface Options → SPI → Enable

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### macOS

```bash
# Install dependencies
brew install cmake openssl sqlite

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

### Windows

```bash
# Install Visual Studio 2022 with C++ workload
# Install vcpkg
vcpkg install openssl sqlite3 gtest

# Build
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 \
    -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `ZIGBEE_BUILD_TESTS` | ON | Build unit tests |
| `ZIGBEE_BUILD_EXAMPLES` | ON | Build example applications |
| `ZIGBEE_BUILD_CLI` | ON | Build CLI application |
| `ZIGBEE_USE_OPENSSL` | ON | Use OpenSSL for crypto |
| `ZIGBEE_USE_SQLITE` | ON | Use SQLite for storage |

## Output

```
build/
├── bin/
│   └── zigbee-cli
├── examples/
│   ├── coordinator_example
│   ├── router_example
│   └── end_device_example
└── tests/
    └── zigbee_tests
```
