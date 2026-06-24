#include "zigbee/security_manager.h"
#include "core/logger.h"
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <cstring>

namespace zigbee_mesh::zigbee {

SecurityManager::SecurityManager() = default;

bool SecurityManager::init(const core::Config& config) {
    initialized_ = true;
    trust_center_enabled_ = config.get<bool>("security", "trust_center", false);
    ZIGBEE_LOG_INFO("Security manager initialized, Trust Center: %s",
                     trust_center_enabled_ ? "enabled" : "disabled");
    return true;
}

bool SecurityManager::generateNetworkKey() {
    if (RAND_bytes(network_key_.key, 16) != 1) {
        ZIGBEE_LOG_ERROR("Failed to generate random network key");
        return false;
    }
    network_key_.type = SecurityKey::Type::NetworkKey;
    network_key_.outgoing_frame_counter = 0;
    network_key_.incoming_frame_counter = 0;
    ZIGBEE_LOG_INFO("Network key generated");
    return true;
}

bool SecurityManager::setNetworkKey(const uint8_t key[16]) {
    std::memcpy(network_key_.key, key, 16);
    network_key_.type = SecurityKey::Type::NetworkKey;
    return true;
}

const uint8_t* SecurityManager::getNetworkKey() const {
    return network_key_.key;
}

bool SecurityManager::addLinkKey(const core::ExtendedAddress& addr, const uint8_t key[16]) {
    ApsKeyPair kp;
    kp.device_addr = addr;
    kp.link_key.type = SecurityKey::Type::LinkKey;
    std::memcpy(kp.link_key.key, key, 16);
    kp.link_key.partner_addr = addr;
    link_keys_[addr] = kp;
    return true;
}

bool SecurityManager::removeLinkKey(const core::ExtendedAddress& addr) {
    return link_keys_.erase(addr) > 0;
}

const SecurityKey* SecurityManager::getLinkKey(const core::ExtendedAddress& addr) const {
    auto it = link_keys_.find(addr);
    if (it == link_keys_.end()) return nullptr;
    return &it->second.link_key;
}

bool SecurityManager::encryptNwkPayload(uint8_t* payload, size_t len, uint32_t frame_counter) {
    if (len == 0) return true;
    uint8_t nonce[13];
    std::memcpy(nonce, &frame_counter, 4);
    nonce[4] = 0;
    nonce[5] = 0;
    for (size_t i = 6; i < 13; ++i) nonce[i] = 0;

    mbedtlsCcmEncrypt(network_key_.key, nonce, payload, len, nullptr, 0);
    return true;
}

bool SecurityManager::decryptNwkPayload(uint8_t* payload, size_t len, uint32_t frame_counter) {
    if (len == 0) return true;
    uint8_t nonce[13];
    std::memcpy(nonce, &frame_counter, 4);
    nonce[4] = 0;
    nonce[5] = 0;
    for (size_t i = 6; i < 13; ++i) nonce[i] = 0;

    mbedtlsCcmEncrypt(network_key_.key, nonce, payload, len, nullptr, 0);
    return true;
}

bool SecurityManager::encryptApsPayload(const core::ExtendedAddress& addr, uint8_t* payload, size_t len) {
    auto it = link_keys_.find(addr);
    if (it == link_keys_.end()) return false;
    uint32_t counter = it->second.outgoing_counter++;
    uint8_t nonce[13];
    std::memcpy(nonce, &counter, 4);
    for (size_t i = 4; i < 13; ++i) nonce[i] = 0;
    mbedtlsCcmEncrypt(it->second.link_key.key, nonce, payload, len, nullptr, 0);
    return true;
}

bool SecurityManager::decryptApsPayload(const core::ExtendedAddress& addr, uint8_t* payload, size_t len) {
    auto it = link_keys_.find(addr);
    if (it == link_keys_.end()) return false;
    uint32_t counter = it->second.incoming_counter++;
    uint8_t nonce[13];
    std::memcpy(nonce, &counter, 4);
    for (size_t i = 4; i < 13; ++i) nonce[i] = 0;
    mbedtlsCcmEncrypt(it->second.link_key.key, nonce, payload, len, nullptr, 0);
    return true;
}

uint32_t SecurityManager::getNextNwkFrameCounter() {
    return network_key_.outgoing_frame_counter++;
}

uint32_t SecurityManager::getNextApsFrameCounter(const core::ExtendedAddress& addr) {
    auto it = link_keys_.find(addr);
    if (it == link_keys_.end()) return 0;
    return it->second.outgoing_counter++;
}

bool SecurityManager::generateInstallCode(const core::ExtendedAddress& addr, uint8_t* code, size_t len) {
    if (len < 16) return false;
    if (RAND_bytes(code, len) != 1) return false;
    return true;
}

bool SecurityManager::computeLinkKeyFromInstallCode(const uint8_t* install_code, size_t len, uint8_t link_key[16]) {
    return computeAesKey(install_code, len, link_key);
}

bool SecurityManager::addToWhitelist(const core::ExtendedAddress& addr) {
    if (!isWhitelisted(addr)) {
        whitelist_.push_back(addr);
    }
    return true;
}

bool SecurityManager::removeFromWhitelist(const core::ExtendedAddress& addr) {
    auto it = std::remove(whitelist_.begin(), whitelist_.end(), addr);
    whitelist_.erase(it, whitelist_.end());
    return true;
}

bool SecurityManager::isWhitelisted(const core::ExtendedAddress& addr) const {
    return std::find(whitelist_.begin(), whitelist_.end(), addr) != whitelist_.end();
}

bool SecurityManager::addToBlacklist(const core::ExtendedAddress& addr) {
    if (!isBlacklisted(addr)) {
        blacklist_.push_back(addr);
    }
    return true;
}

bool SecurityManager::removeFromBlacklist(const core::ExtendedAddress& addr) {
    auto it = std::remove(blacklist_.begin(), blacklist_.end(), addr);
    blacklist_.erase(it, blacklist_.end());
    return true;
}

bool SecurityManager::isBlacklisted(const core::ExtendedAddress& addr) const {
    return std::find(blacklist_.begin(), blacklist_.end(), addr) != blacklist_.end();
}

bool SecurityManager::exportKeys(const std::string& path) {
    (void)path;
    return true;
}

bool SecurityManager::importKeys(const std::string& path) {
    (void)path;
    return true;
}

void SecurityManager::xorBlocks(uint8_t* result, const uint8_t* a, const uint8_t* b, size_t len) {
    for (size_t i = 0; i < len; ++i) result[i] = a[i] ^ b[i];
}

void SecurityManager::mbedtlsAesEncrypt(const uint8_t key[16], const uint8_t input[16], uint8_t output[16]) {
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);
    AES_encrypt(input, output, &aes_key);
}

void SecurityManager::mbedtlsCcmEncrypt(const uint8_t key[16], uint32_t frame_counter,
                                         uint8_t nonce[13], uint8_t* data, size_t len,
                                         uint8_t* mic, size_t mic_len) {
    (void)frame_counter;
    (void)mic;
    (void)mic_len;
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);

    uint8_t counter_block[16] = {};
    std::memcpy(counter_block, nonce, 13);
    counter_block[15] = 0x01;

    for (size_t i = 0; i < len; i += 16) {
        uint8_t keystream[16];
        uint8_t ctr[16];
        std::memcpy(ctr, counter_block, 16);
        ctr[15] = static_cast<uint8_t>((i / 16) + 1);
        AES_encrypt(ctr, keystream, &aes_key);
        size_t block_len = (len - i > 16) ? 16 : (len - i);
        for (size_t j = 0; j < block_len; ++j) {
            data[i + j] ^= keystream[j];
        }
    }
}

bool SecurityManager::computeAesKey(const uint8_t* input, size_t len, uint8_t output[16]) {
    if (len == 0) return false;
    uint8_t key[16] = {};
    mbedtlsAesEncrypt(key, input, output);
    for (size_t i = 16; i < len; i += 16) {
        uint8_t temp[16];
        mbedtlsAesEncrypt(output, input + i, temp);
        std::memcpy(output, temp, 16);
    }
    return true;
}

} // namespace zigbee_mesh::zigbee
