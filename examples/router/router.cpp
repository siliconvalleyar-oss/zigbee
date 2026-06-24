#include "core/config.h"
#include "core/logger.h"
#include "services/network_manager.h"
#include <iostream>

int main() {
    zigbee_mesh::core::LoggerConfig log_cfg;
    zigbee_mesh::core::Logger::getInstance().init(log_cfg);

    std::cout << "Zigbee Router Example\n";

    zigbee_mesh::core::Config config;
    config.set<uint8_t>("device", "role", 1);
    config.set<uint16_t>("network", "pan_id", 0x1234);
    config.set<uint8_t>("network", "channel", 11);

    zigbee_mesh::services::NetworkManager network;
    if (!network.init(config)) {
        std::cerr << "Failed to initialize network\n";
        return 1;
    }

    network.setRole(zigbee_mesh::core::DeviceRole::Router);
    network.start();

    if (network.joinNetwork(0x1234, 11)) {
        std::cout << "Joined network as Router\n";
        std::cout << "Waiting... (Ctrl+C to exit)\n";
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }

    network.stop();
    return 0;
}
