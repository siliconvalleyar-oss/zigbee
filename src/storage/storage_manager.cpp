#include "storage/storage_manager.h"
#include "core/logger.h"
#include <sqlite3.h>
#include <cstring>

namespace zigbee_mesh::storage {

static int callback(void* data, int argc, char** argv, char** colnames) {
    (void)data; (void)argc; (void)argv; (void)colnames;
    return 0;
}

bool StorageManager::open(const std::string& db_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    int rc = sqlite3_open(db_path.c_str(), reinterpret_cast<sqlite3**>(&db_));
    if (rc != SQLITE_OK) {
        ZIGBEE_LOG_ERROR("Failed to open database: %s", db_path.c_str());
        return false;
    }
    db_open_ = true;
    return createTables();
}

void StorageManager::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) {
        sqlite3_close(static_cast<sqlite3*>(db_));
        db_ = nullptr;
    }
    db_open_ = false;
}

bool StorageManager::createTables() {
    const char* devices = "CREATE TABLE IF NOT EXISTS devices ("
        "ieee_addr INTEGER PRIMARY KEY, nwk_addr INTEGER, role INTEGER, "
        "depth INTEGER, parent_addr INTEGER, authenticated INTEGER, "
        "name TEXT, description TEXT, first_seen INTEGER, last_seen INTEGER);";

    const char* network = "CREATE TABLE IF NOT EXISTS network ("
        "pan_id INTEGER, channel INTEGER, short_addr INTEGER, "
        "extended_addr INTEGER, nwk_update_id INTEGER, role INTEGER, "
        "network_key BLOB);";

    const char* topology = "CREATE TABLE IF NOT EXISTS topology ("
        "addr INTEGER PRIMARY KEY, parent INTEGER, depth INTEGER, "
        "lqi INTEGER, rssi INTEGER, last_updated INTEGER);";

    const char* routes = "CREATE TABLE IF NOT EXISTS routes ("
        "destination INTEGER PRIMARY KEY, next_hop INTEGER, "
        "hop_count INTEGER, cost INTEGER, age INTEGER, active INTEGER);";

    const char* attrs = "CREATE TABLE IF NOT EXISTS attributes ("
        "ieee_addr INTEGER, endpoint INTEGER, cluster_id INTEGER, "
        "attr_id INTEGER, value BLOB, PRIMARY KEY (ieee_addr, endpoint, cluster_id, attr_id));";

    char* errMsg = nullptr;
    sqlite3_exec(static_cast<sqlite3*>(db_), devices, callback, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    sqlite3_exec(static_cast<sqlite3*>(db_), network, callback, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    sqlite3_exec(static_cast<sqlite3*>(db_), topology, callback, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    sqlite3_exec(static_cast<sqlite3*>(db_), routes, callback, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    sqlite3_exec(static_cast<sqlite3*>(db_), attrs, callback, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    return true;
}

bool StorageManager::ensureSchema() { return true; }

bool StorageManager::saveDevice(const StoredDeviceInfo& device) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT OR REPLACE INTO devices VALUES (%llu, %u, %d, %d, %u, %d, '%s', '%s', %u, %u);",
        static_cast<unsigned long long>(device.ieee_addr), device.nwk_addr,
        static_cast<int>(device.role), device.depth, device.parent_addr,
        device.authenticated ? 1 : 0, device.name.c_str(), device.description.c_str(),
        device.first_seen, device.last_seen);
    char* errMsg = nullptr;
    sqlite3_exec(static_cast<sqlite3*>(db_), sql, callback, nullptr, &errMsg);
    if (errMsg) { sqlite3_free(errMsg); return false; }
    return true;
}

bool StorageManager::updateDevice(const StoredDeviceInfo& device) { return saveDevice(device); }

bool StorageManager::removeDevice(core::ExtendedAddress ieee_addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    char sql[128];
    snprintf(sql, sizeof(sql), "DELETE FROM devices WHERE ieee_addr = %llu;",
             static_cast<unsigned long long>(ieee_addr));
    char* errMsg = nullptr;
    sqlite3_exec(static_cast<sqlite3*>(db_), sql, callback, nullptr, &errMsg);
    if (errMsg) { sqlite3_free(errMsg); return false; }
    return true;
}

bool StorageManager::getDevice(core::ExtendedAddress ieee_addr, StoredDeviceInfo& device) const {
    (void)ieee_addr; (void)device;
    return false;
}

std::vector<StoredDeviceInfo> StorageManager::getAllDevices() const {
    return {};
}

bool StorageManager::deviceExists(core::ExtendedAddress ieee_addr) const {
    (void)ieee_addr;
    return false;
}

bool StorageManager::saveNetworkInfo(const StoredNetworkInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "DELETE FROM network; INSERT INTO network VALUES (%u, %d, %u, %llu, %d, %d, NULL);",
        info.pan_id, info.channel, info.short_addr,
        static_cast<unsigned long long>(info.extended_addr),
        info.nwk_update_id, static_cast<int>(info.role));
    char* errMsg = nullptr;
    sqlite3_exec(static_cast<sqlite3*>(db_), sql, callback, nullptr, &errMsg);
    if (errMsg) { sqlite3_free(errMsg); return false; }
    return true;
}

bool StorageManager::getNetworkInfo(StoredNetworkInfo& info) const {
    (void)info;
    return false;
}

bool StorageManager::removeNetworkInfo() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    sqlite3_exec(static_cast<sqlite3*>(db_), "DELETE FROM network;", callback, nullptr, nullptr);
    return true;
}

bool StorageManager::saveTopologyNode(const StoredTopologyNode& node) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "INSERT OR REPLACE INTO topology VALUES (%u, %u, %d, %d, %d, %u);",
        node.addr, node.parent, node.depth, node.lqi, node.rssi, node.last_updated);
    char* errMsg = nullptr;
    sqlite3_exec(static_cast<sqlite3*>(db_), sql, callback, nullptr, &errMsg);
    if (errMsg) { sqlite3_free(errMsg); return false; }
    return true;
}

bool StorageManager::removeTopologyNode(core::ShortAddress addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    char sql[64];
    snprintf(sql, sizeof(sql), "DELETE FROM topology WHERE addr = %u;", addr);
    sqlite3_exec(static_cast<sqlite3*>(db_), sql, callback, nullptr, nullptr);
    return true;
}

std::vector<StoredTopologyNode> StorageManager::getAllTopologyNodes() const { return {}; }

bool StorageManager::clearTopology() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    sqlite3_exec(static_cast<sqlite3*>(db_), "DELETE FROM topology;", callback, nullptr, nullptr);
    return true;
}

bool StorageManager::saveRoute(const StoredRoute& route) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "INSERT OR REPLACE INTO routes VALUES (%u, %u, %d, %u, %u, %d);",
        route.destination, route.next_hop, route.hop_count,
        route.cost, route.age, route.active ? 1 : 0);
    char* errMsg = nullptr;
    sqlite3_exec(static_cast<sqlite3*>(db_), sql, callback, nullptr, &errMsg);
    if (errMsg) { sqlite3_free(errMsg); return false; }
    return true;
}

bool StorageManager::removeRoute(core::ShortAddress destination) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    char sql[64];
    snprintf(sql, sizeof(sql), "DELETE FROM routes WHERE destination = %u;", destination);
    sqlite3_exec(static_cast<sqlite3*>(db_), sql, callback, nullptr, nullptr);
    return true;
}

std::vector<StoredRoute> StorageManager::getAllRoutes() const { return {}; }

bool StorageManager::clearRoutes() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    sqlite3_exec(static_cast<sqlite3*>(db_), "DELETE FROM routes;", callback, nullptr, nullptr);
    return true;
}

bool StorageManager::saveAttribute(core::ExtendedAddress ieee_addr, uint8_t endpoint,
                                    uint16_t cluster_id, uint16_t attr_id,
                                    const std::vector<uint8_t>& value) {
    (void)ieee_addr; (void)endpoint; (void)cluster_id; (void)attr_id; (void)value;
    return true;
}

bool StorageManager::getAttribute(core::ExtendedAddress ieee_addr, uint8_t endpoint,
                                   uint16_t cluster_id, uint16_t attr_id,
                                   std::vector<uint8_t>& value) const {
    (void)ieee_addr; (void)endpoint; (void)cluster_id; (void)attr_id; (void)value;
    return false;
}

bool StorageManager::saveBinding(core::ExtendedAddress, uint8_t, uint16_t,
                                  core::ExtendedAddress, uint8_t) { return true; }

bool StorageManager::removeBinding(core::ExtendedAddress, uint8_t, uint16_t) { return true; }

bool StorageManager::vacuum() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_open_) return false;
    sqlite3_exec(static_cast<sqlite3*>(db_), "VACUUM;", callback, nullptr, nullptr);
    return true;
}

bool StorageManager::backup(const std::string&) { return true; }
bool StorageManager::restore(const std::string&) { return true; }

} // namespace zigbee_mesh::storage
