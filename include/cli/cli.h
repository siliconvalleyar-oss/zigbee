#pragma once

#include "core/types.h"
#include "services/network_manager.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace zigbee_mesh::cli {

struct CliCommand {
    std::string name;
    std::string description;
    std::vector<std::string> aliases;
    std::function<void(const std::vector<std::string>&)> handler;
};

struct CliHelpEntry {
    std::string command;
    std::string usage;
    std::string description;
    std::vector<std::pair<std::string, std::string>> options;
};

class ZigbeeCli {
public:
    ZigbeeCli();
    ~ZigbeeCli();

    bool init(services::NetworkManager* network);
    void run();
    void stop();

    bool processCommand(const std::string& input);
    std::string getHelp() const;

    void registerCommand(const std::string& name, const std::string& description,
                         std::function<void(const std::vector<std::string>&)> handler);
    void registerAlias(const std::string& alias, const std::string& original);

    void setPrompt(const std::string& prompt) { prompt_ = prompt; }
    void setHistorySize(size_t size) { max_history_ = size; }

    static std::vector<std::string> parseArgs(const std::string& input);
    static std::string joinArgs(const std::vector<std::string>& args, size_t start = 0);

private:
    void registerBuiltinCommands();
    void printBanner();
    void printHelp(const std::string& topic = "");
    void printCompletions(const std::string& prefix);

    void cmdNetworkCreate(const std::vector<std::string>& args);
    void cmdNetworkJoin(const std::vector<std::string>& args);
    void cmdNetworkLeave(const std::vector<std::string>& args);
    void cmdNetworkScan(const std::vector<std::string>& args);
    void cmdNetworkStatus(const std::vector<std::string>& args);

    void cmdDeviceList(const std::vector<std::string>& args);
    void cmdDeviceInfo(const std::vector<std::string>& args);
    void cmdDeviceRemove(const std::vector<std::string>& args);

    void cmdMeshTopology(const std::vector<std::string>& args);
    void cmdMeshRoutes(const std::vector<std::string>& args);
    void cmdMeshDiagnostics(const std::vector<std::string>& args);

    void cmdSecurityKeys(const std::vector<std::string>& args);

    void cmdHelp(const std::vector<std::string>& args);
    void cmdQuit(const std::vector<std::string>& args);
    void cmdVersion(const std::vector<std::string>& args);

    services::NetworkManager* network_{nullptr};
    std::map<std::string, CliCommand> commands_;
    std::map<std::string, std::string> aliases_;
    std::vector<std::string> history_;
    std::string prompt_{"zigbee> "};
    size_t max_history_{100};
    bool running_{false};
};

} // namespace zigbee_mesh::cli
