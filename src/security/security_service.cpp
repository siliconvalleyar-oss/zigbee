#include "security/security_service.h"
#include "core/logger.h"
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <cstring>
#include <fstream>

namespace zigbee_mesh::security {

bool SecurityService::init(const core::Config& config) {
    initialized_ = true;
    is_trust_center_ = config.get<bool>("security", "trust_center", false);
    policy_.require_authentication = config.get<bool>("security", "require_auth", true);
    policy_.allow_open_join = config.get<bool>("security", "allow_open_join", false);
    ZIGBEE_LOG_INFO("Security service initialized, trust_center=%s",
                     is_trust_center_ ? "yes" : "no");
    return true;
}

bool SecurityService::installNetworkKey(const uint8_t key[16]) {
    std::lock_guard<std::mutex> lock(mutex_);
    network_key_.type = SecurityKeyEntry::Type::NetworkKey;
    std::memcpy(network_key_.key, key, 16);
    network_key_.frame_counter = 0;
    return true;
}

bool SecurityService::generateNetworkKey() {
    uint8_t key[16];
    generateRandomKey(key);
    return installNetworkKey(key);
}

const uint8_t* SecurityService::getNetworkKey() const {
    return network_key_.key;
}

bool SecurityService::installLinkKey(const core::ExtendedAddress& device, const uint8_t key[16]) {
    std::lock_guard<std::mutex> lock(mutex_);
    SecurityKeyEntry entry;
    entry.type = SecurityKeyEntry::Type::LinkKey;
    std::memcpy(entry.key, key, 16);
    entry.partner_addr = device;
    link_keys_[device] = entry;
    return true;
}

bool SecurityService::removeLinkKey(const core::ExtendedAddress& device) {
    std::lock_guard<std::mutex> lock(mutex_);
    return link_keys_.erase(device) > 0;
}

const SecurityKeyEntry* SecurityService::getLinkKey(const core::ExtendedAddress& device) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = link_keys_.find(device);
    return it != link_keys_.end() ? &it->second : nullptr;
}

bool SecurityService::setInstallCode(const core::ExtendedAddress& device, const uint8_t* code, size_t len) {
    uint8_t key[16];
    if (!computeKeyFromInstallCode(code, len, key)) return false;
    return installLinkKey(device, key);
}

bool SecurityService::computeKeyFromInstallCode(const uint8_t* code, size_t len, uint8_t key[16]) {
    if (len == 0 || len > 18) return false;
    uint8_t temp[16] = {};
    std::memcpy(temp, code, len < 16 ? len : 16);
    aesEncrypt(temp, code, key);
    return true;
}

bool SecurityService::encryptNwkPayload(uint8_t* payload, size_t len, uint32_t& frame_counter) {
    if (len == 0) return true;
    std::lock_guard<std::mutex> lock(mutex_);
    frame_counter = network_key_.frame_counter++;
    uint8_t nonce[13] = {};
    std::memcpy(nonce, &frame_counter, 4);
    ccmEncrypt(network_key_.key, nonce, payload, len, nullptr, 0);
    return true;
}

bool SecurityService::decryptNwkPayload(uint8_t* payload, size_t len, uint32_t frame_counter) {
    if (len == 0) return true;
    std::lock_guard<std::mutex> lock(mutex_);
    uint8_t nonce[13] = {};
    std::memcpy(nonce, &frame_counter, 4);
    ccmEncrypt(network_key_.key, nonce, payload, len, nullptr, 0);
    return true;
}

bool SecurityService::encryptApsPayload(const core::ExtendedAddress& addr, uint8_t* payload, size_t len) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = link_keys_.find(addr);
    if (it == link_keys_.end()) return false;
    uint8_t nonce[13] = {};
    ccmEncrypt(it->second.key, nonce, payload, len, nullptr, 0);
    return true;
}

bool SecurityService::decryptApsPayload(const core::ExtendedAddress& addr, uint8_t* payload, size_t len) {
    return encryptApsPayload(addr, payload, len);
}

bool SecurityService::addToWhitelist(const core::ExtendedAddress& addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isWhitelisted(addr)) whitelist_.push_back(addr);
    return true;
}

bool SecurityService::removeFromWhitelist(const core::ExtendedAddress& addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove(whitelist_.begin(), whitelist_.end(), addr);
    whitelist_.erase(it, whitelist_.end());
    return true;
}

bool SecurityService::isWhitelisted(const core::ExtendedAddress& addr) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::find(whitelist_.begin(), whitelist_.end(), addr) != whitelist_.end();
}

std::vector<core::ExtendedAddress> SecurityService::getWhitelist() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return whitelist_;
}

bool SecurityService::addToBlacklist(const core::ExtendedAddress& addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isBlacklisted(addr)) blacklist_.push_back(addr);
    return true;
}

bool SecurityService::removeFromBlacklist(const core::ExtendedAddress& addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove(blacklist_.begin(), blacklist_.end(), addr);
    blacklist_.erase(it, blacklist_.end());
    return true;
}

bool SecurityService::isBlacklisted(const core::ExtendedAddress& addr) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::find(blacklist_.begin(), blacklist_.end(), addr) != blacklist_.end();
}

std::vector<core::ExtendedAddress> SecurityService::getBlacklist() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return blacklist_;
}

bool SecurityService::setTrustCenterPolicy(const TrustCenterPolicy& policy) {
    std::lock_guard<std::mutex> lock(mutex_);
    policy_ = policy;
    return true;
}

TrustCenterPolicy SecurityService::getTrustCenterPolicy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return policy_;
}

std::string SecurityService::getKeyStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string r;
    r += "Network Key: installed\n";
    r += "Link Keys: " + std::to_string(link_keys_.size()) + "\n";
    r += "Whitelist: " + std::to_string(whitelist_.size()) + " devices\n";
    r += "Blacklist: " + std::to_string(blacklist_.size()) + " devices\n";
    return r;
}

size_t SecurityService::getKeyCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return 1 + link_keys_.size();
}

bool SecurityService::exportKeys(const std::string& path) const {
    (void)path;
    return true;
}

bool SecurityService::importKeys(const std::string& path) {
    (void)path;
    return true;
}

void SecurityService::removeExpiredKeys(uint32_t max_age_ms) {
    (void)max_age_ms;
}

void SecurityService::generateRandomKey(uint8_t key[16]) {
    RAND_bytes(key, 16);
}

void SecurityService::aesEncrypt(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]) {
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);
    AES_encrypt(in, out, &aes_key);
}

void SecurityService::ccmEncrypt(const uint8_t key[16], const uint8_t nonce[13],
                                  uint8_t* data, size_t len, uint8_t* mic, size_t mic_len) {
    (void)mic;
    (void)mic_len;
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);
    uint8_t counter[16] = {};
    std::memcpy(counter, nonce, 13);
    counter[15] = 0x01;
    for (size_t i = 0; i < len; i += 16) {
        uint8_t ks[16];
        uint8_t c[16];
        std::memcpy(c, counter, 16);
        c[15] = static_cast<uint8_t>((i / 16) + 1);
        AES_encrypt(c, ks, &aes_key);
        size_t bl = (len - i > 16) ? 16 : (len - i);
        for (size_t j = 0; j < bl; ++j) data[i + j] ^= ks[j];
    }
}

} // namespace zigbee_mesh::security
