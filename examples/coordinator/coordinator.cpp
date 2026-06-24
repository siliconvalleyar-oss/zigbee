#include "core/config.h"
#include "core/logger.h"
#include "services/network_manager.h"
#include <iostream>

int main() {
    zigbee_mesh::core::LoggerConfig log_cfg;
    log_cfg.level = zigbee_mesh::core::LogLevel::Info;
    zigbee_mesh::core::Logger::getInstance().init(log_cfg);

    std::cout << "Zigbee Coordinator Example\n";

    zigbee_mesh::core::Config config;
    config.set<uint8_t>("device", "role", 0);
    config.set<uint16_t>("network", "pan_id", 0x1234);
    config.set<uint8_t>("network", "channel", 11);

    zigbee_mesh::services::NetworkManager network;
    if (!network.init(config)) {
        std::cerr << "Failed to initialize network\n";
        return 1;
    }

    network.setRole(zigbee_mesh::core::DeviceRole::Coordinator);
    network.start();

    if (network.createNetwork(11, 0x1234)) {
        std::cout << "Network created successfully\n";
        std::cout << "PAN ID: 0x" << std::hex << 0x1234 << std::dec << "\n";
        std::cout << "Channel: 11\n";
        std::cout << "Role: Coordinator\n";

        std::cout << "Waiting for devices... (Ctrl+C to exit)\n";
        std::this_thread::sleep_for(std::chrono::seconds(60));
    } else {
        std::cerr << "Failed to create network\n";
    }

    network.stop();
    return 0;
}
