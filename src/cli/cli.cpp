#include "cli/cli.h"
#include "core/logger.h"
#include <iostream>
#include <sstream>

namespace zigbee_mesh::cli {

ZigbeeCli::ZigbeeCli() = default;
ZigbeeCli::~ZigbeeCli() { stop(); }

bool ZigbeeCli::init(services::NetworkManager* network) {
    network_ = network;
    registerBuiltinCommands();
    return true;
}

void ZigbeeCli::run() {
    running_ = true;
    printBanner();
    std::string input;
    while (running_) {
        std::cout << prompt_;
        if (!std::getline(std::cin, input)) break;
        if (input.empty()) continue;
        if (history_.size() >= max_history_) history_.erase(history_.begin());
        history_.push_back(input);
        processCommand(input);
    }
}

void ZigbeeCli::stop() { running_ = false; }

bool ZigbeeCli::processCommand(const std::string& input) {
    auto args = parseArgs(input);
    if (args.empty()) return false;

    std::string cmd = args[0];
    auto alias_it = aliases_.find(cmd);
    if (alias_it != aliases_.end()) cmd = alias_it->second;

    auto it = commands_.find(cmd);
    if (it != commands_.end()) {
        it->second.handler(args);
        return true;
    }
    std::cout << "Unknown command: " << cmd << ". Type 'help' for commands.\n";
    return false;
}

std::string ZigbeeCli::getHelp() const {
    std::string r;
    for (const auto& [name, cmd] : commands_) {
        r += "  " + name + " - " + cmd.description + "\n";
    }
    return r;
}

void ZigbeeCli::registerCommand(const std::string& name, const std::string& description,
                                  std::function<void(const std::vector<std::string>&)> handler) {
    commands_[name] = {name, description, {}, std::move(handler)};
}

void ZigbeeCli::registerAlias(const std::string& alias, const std::string& original) {
    aliases_[alias] = original;
}

std::vector<std::string> ZigbeeCli::parseArgs(const std::string& input) {
    std::vector<std::string> args;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {
        if (!token.empty() && (token[0] == '"' || token[0] == '\'')) {
            char quote = token[0];
            std::string full = token.substr(1);
            while (iss >> token) {
                if (!token.empty() && token.back() == quote) {
                    full += " " + token.substr(0, token.size() - 1);
                    break;
                }
                full += " " + token;
            }
            args.push_back(full);
        } else {
            args.push_back(token);
        }
    }
    return args;
}

std::string ZigbeeCli::joinArgs(const std::vector<std::string>& args, size_t start) {
    std::string result;
    for (size_t i = start; i < args.size(); ++i) {
        if (!result.empty()) result += " ";
        result += args[i];
    }
    return result;
}

void ZigbeeCli::registerBuiltinCommands() {
    registerCommand("help", "Show help", [this](const auto& args) {
        if (args.size() > 1) printHelp(args[1]);
        else printHelp();
    });
    registerCommand("quit", "Exit CLI", [this](const auto&) { running_ = false; });
    registerCommand("exit", "Exit CLI", [this](const auto&) { running_ = false; });
    registerCommand("version", "Show version", [](const auto&) {
        std::cout << "Zigbee Mesh v1.0.0\n";
        std::cout << "C++20 | IEEE 802.15.4 | Zigbee 3.0\n";
    });
    registerCommand("network", "Network operations", [this](const auto& args) {
        if (args.size() < 2) { std::cout << "Usage: network <create|join|leave|scan|status>\n"; return; }
        if (args[1] == "create") cmdNetworkCreate(args);
        else if (args[1] == "join") cmdNetworkJoin(args);
        else if (args[1] == "leave") cmdNetworkLeave(args);
        else if (args[1] == "scan") cmdNetworkScan(args);
        else if (args[1] == "status") cmdNetworkStatus(args);
        else std::cout << "Unknown network subcommand: " << args[1] << "\n";
    });
    registerCommand("device", "Device operations", [this](const auto& args) {
        if (args.size() < 2) { std::cout << "Usage: device <list|info|remove>\n"; return; }
        if (args[1] == "list") cmdDeviceList(args);
        else if (args[1] == "info") cmdDeviceInfo(args);
        else if (args[1] == "remove") cmdDeviceRemove(args);
        else std::cout << "Unknown device subcommand: " << args[1] << "\n";
    });
    registerCommand("mesh", "Mesh operations", [this](const auto& args) {
        if (args.size() < 2) { std::cout << "Usage: mesh <topology|routes|diagnostics>\n"; return; }
        if (args[1] == "topology") cmdMeshTopology(args);
        else if (args[1] == "routes") cmdMeshRoutes(args);
        else if (args[1] == "diagnostics") cmdMeshDiagnostics(args);
    });
    registerCommand("security", "Security operations", [this](const auto& args) {
        if (args.size() < 2) { std::cout << "Usage: security <keys>\n"; return; }
        if (args[1] == "keys") cmdSecurityKeys(args);
    });
    registerAlias("q", "quit");
    registerAlias("h", "help");
    registerAlias("v", "version");
}

void ZigbeeCli::printBanner() {
    std::cout << "========================================\n";
    std::cout << "  Zigbee Mesh CLI v1.0.0\n";
    std::cout << "  IEEE 802.15.4 | Zigbee 3.0\n";
    std::cout << "========================================\n";
    std::cout << "  Type 'help' for available commands\n\n";
}

void ZigbeeCli::printHelp(const std::string& topic) {
    if (!topic.empty()) {
        auto it = commands_.find(topic);
        if (it != commands_.end()) {
            std::cout << it->second.name << " - " << it->second.description << "\n";
            return;
        }
    }
    std::cout << "Available commands:\n" << getHelp();
}

void ZigbeeCli::printCompletions(const std::string& prefix) {
    for (const auto& [name, cmd] : commands_) {
        if (name.find(prefix) == 0) {
            std::cout << "  " << name << " - " << cmd.description << "\n";
        }
    }
}

void ZigbeeCli::cmdNetworkCreate(const std::vector<std::string>& args) {
    uint8_t channel = 11;
    core::PanId pan_id = 0x1234;
    if (args.size() > 2) channel = static_cast<uint8_t>(std::stoi(args[2]));
    if (args.size() > 3) pan_id = static_cast<core::PanId>(std::stoi(args[3], nullptr, 0));
    if (network_) {
        network_->setRole(core::DeviceRole::Coordinator);
        bool ok = network_->createNetwork(channel, pan_id);
        std::cout << (ok ? "Network created" : "Failed to create network")
                  << " (CH=" << static_cast<int>(channel)
                  << ", PAN=0x" << std::hex << pan_id << std::dec << ")\n";
    } else {
        std::cout << "Network manager not initialized\n";
    }
}

void ZigbeeCli::cmdNetworkJoin(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        std::cout << "Usage: network join <pan_id> <channel>\n";
        return;
    }
    core::PanId pan_id = static_cast<core::PanId>(std::stoi(args[2], nullptr, 0));
    uint8_t channel = static_cast<uint8_t>(std::stoi(args[3]));
    if (network_) {
        bool ok = network_->joinNetwork(pan_id, channel);
        std::cout << (ok ? "Joined network" : "Failed to join") << "\n";
    }
}

void ZigbeeCli::cmdNetworkLeave(const std::vector<std::string>&) {
    if (network_) {
        bool ok = network_->leaveNetwork();
        std::cout << (ok ? "Left network" : "Failed to leave") << "\n";
    }
}

void ZigbeeCli::cmdNetworkScan(const std::vector<std::string>&) {
    std::cout << "Scanning...\n";
    if (network_) {
        auto info = network_->getNetworkInfo();
        std::cout << "Current: CH=" << static_cast<int>(info.channel)
                  << " PAN=0x" << std::hex << info.pan_id << std::dec << "\n";
    }
}

void ZigbeeCli::cmdNetworkStatus(const std::vector<std::string>&) {
    if (network_) {
        auto info = network_->getNetworkInfo();
        auto devs = network_->getDevices();
        std::cout << "Network Status:\n";
        std::cout << "  Role: " << core::deviceRoleToString(info.role) << "\n";
        std::cout << "  PAN: 0x" << std::hex << info.pan_id << std::dec << "\n";
        std::cout << "  Channel: " << static_cast<int>(info.channel) << "\n";
        std::cout << "  Short Addr: 0x" << std::hex << info.short_addr << std::dec << "\n";
        std::cout << "  State: " << core::nwkStateToString(info.state) << "\n";
        std::cout << "  Devices: " << devs.size() << "\n";
    }
}

void ZigbeeCli::cmdDeviceList(const std::vector<std::string>&) {
    if (network_) {
        auto devs = network_->getDevices();
        if (devs.empty()) { std::cout << "No devices found\n"; return; }
        std::cout << "Devices:\n";
        for (const auto& d : devs) {
            std::cout << "  0x" << std::hex << d.nwk_addr << std::dec
                      << " [" << core::deviceRoleToString(d.role) << "]"
                      << " LQI=" << static_cast<int>(d.lqi)
                      << " RSSI=" << static_cast<int>(d.rssi) << "dBm\n";
        }
    }
}

void ZigbeeCli::cmdDeviceInfo(const std::vector<std::string>& args) {
    if (args.size() < 3) { std::cout << "Usage: device info <short_addr>\n"; return; }
    core::ShortAddress addr = static_cast<core::ShortAddress>(std::stoi(args[2], nullptr, 0));
    std::cout << "Device info for 0x" << std::hex << addr << std::dec << "\n";
}

void ZigbeeCli::cmdDeviceRemove(const std::vector<std::string>& args) {
    if (args.size() < 3) { std::cout << "Usage: device remove <short_addr>\n"; return; }
    std::cout << "Removing device " << args[2] << "\n";
}

void ZigbeeCli::cmdMeshTopology(const std::vector<std::string>&) {
    std::cout << "Mesh Topology:\n  (Topology data would be displayed here)\n";
}

void ZigbeeCli::cmdMeshRoutes(const std::vector<std::string>&) {
    std::cout << "Mesh Routes:\n  (Route table would be displayed here)\n";
}

void ZigbeeCli::cmdMeshDiagnostics(const std::vector<std::string>&) {
    std::cout << "Mesh Diagnostics:\n  (Diagnostics would be displayed here)\n";
}

void ZigbeeCli::cmdSecurityKeys(const std::vector<std::string>&) {
    std::cout << "Security Keys:\n  (Key status would be displayed here)\n";
}

} // namespace zigbee_mesh::cli
