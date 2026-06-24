#include "core/config.h"
#include "core/logger.h"
#include "core/types.h"
#include "services/network_manager.h"
#include "cli/cli.h"
#include "storage/storage_manager.h"
#include "security/security_service.h"
#include <iostream>
#include <csignal>
#include <cstdlib>

static std::atomic<bool> g_running{true};

static void signalHandler(int signum) {
    (void)signum;
    g_running = false;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    zigbee_mesh::core::LoggerConfig log_config;
    log_config.level = zigbee_mesh::core::LogLevel::Info;
    log_config.sink = zigbee_mesh::core::LogSink::Both;
    zigbee_mesh::core::Logger::getInstance().init(log_config);

    ZIGBEE_LOG_INFO("Zigbee Mesh starting...");

    zigbee_mesh::core::Config config;
    std::string config_file = "configs/zigbee_mesh.conf";
    for (int i = 1; i < argc; ++i) {
        if ((std::string(argv[i]) == "-c" || std::string(argv[i]) == "--config") && i + 1 < argc) {
            config_file = argv[++i];
        }
        if (std::string(argv[i]) == "-h" || std::string(argv[i]) == "--help") {
            std::cout << "Zigbee Mesh Multiplatform v1.0.0\n\n";
            std::cout << "Usage: " << argv[0] << " [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  -c, --config <file>  Configuration file (default: configs/zigbee_mesh.conf)\n";
            std::cout << "  -h, --help           Show this help message\n";
            std::cout << "  -v, --version        Show version\n";
            return 0;
        }
        if (std::string(argv[i]) == "-v" || std::string(argv[i]) == "--version") {
            std::cout << "Zigbee Mesh v1.0.0\n";
            return 0;
        }
    }

    config.loadFromFile(config_file);

    zigbee_mesh::services::NetworkManager network;
    if (!network.init(config)) {
        ZIGBEE_LOG_ERROR("Failed to initialize network manager");
        return 1;
    }

    zigbee_mesh::storage::StorageManager storage;
    std::string db_path = config.get<std::string>("storage", "db_path", "data/zigbee_mesh.db");
    storage.open(db_path);

    zigbee_mesh::security::SecurityService security;
    security.init(config);

    zigbee_mesh::cli::ZigbeeCli cli;
    cli.init(&network);

    if (config.get<bool>("general", "daemon", false)) {
        network.start();
        ZIGBEE_LOG_INFO("Running in daemon mode");
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else {
        network.start();
        cli.run();
    }

    network.stop();
    storage.close();
    zigbee_mesh::core::Logger::getInstance().shutdown();

    ZIGBEE_LOG_INFO("Zigbee Mesh shutdown complete");
    return 0;
}
