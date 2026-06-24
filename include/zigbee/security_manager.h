#pragma once

#include "ieee802154.h"
#include "core/config.h"
#include "core/logger.h"
#include "core/event.h"
#include <memory>
#include <map>

namespace zigbee_mesh::zigbee {

struct SecurityKey {
    enum class Type : uint8_t {
        NetworkKey = 0x01,
        LinkKey = 0x02,
        MasterKey = 0x03,
    };
    Type type{Type::NetworkKey};
    uint8_t key[16]{};
    uint32_t outgoing_frame_counter{0};
    uint32_t incoming_frame_counter{0};
    core::ExtendedAddress partner_addr{0};
};

struct ApsKeyPair {
    core::ExtendedAddress device_addr{0};
    SecurityKey link_key;
    uint32_t outgoing_counter{0};
    uint32_t incoming_counter{0};
};

class SecurityManager {
public:
    SecurityManager();

    bool init(const core::Config& config);
    bool generateNetworkKey();
    bool setNetworkKey(const uint8_t key[16]);
    const uint8_t* getNetworkKey() const;

    bool addLinkKey(const core::ExtendedAddress& addr, const uint8_t key[16]);
    bool removeLinkKey(const core::ExtendedAddress& addr);
    const SecurityKey* getLinkKey(const core::ExtendedAddress& addr) const;

    bool encryptNwkPayload(uint8_t* payload, size_t len, uint32_t frame_counter);
    bool decryptNwkPayload(uint8_t* payload, size_t len, uint32_t frame_counter);

    bool encryptApsPayload(const core::ExtendedAddress& addr, uint8_t* payload, size_t len);
    bool decryptApsPayload(const core::ExtendedAddress& addr, uint8_t* payload, size_t len);

    uint32_t getNextNwkFrameCounter();
    uint32_t getNextApsFrameCounter(const core::ExtendedAddress& addr);

    bool generateInstallCode(const core::ExtendedAddress& addr, uint8_t* code, size_t len);
    bool computeLinkKeyFromInstallCode(const uint8_t* install_code, size_t len, uint8_t link_key[16]);

    void setTrustCenterEnabled(bool enabled) { trust_center_enabled_ = enabled; }
    bool isTrustCenterEnabled() const { return trust_center_enabled_; }

    bool addToWhitelist(const core::ExtendedAddress& addr);
    bool removeFromWhitelist(const core::ExtendedAddress& addr);
    bool isWhitelisted(const core::ExtendedAddress& addr) const;

    bool addToBlacklist(const core::ExtendedAddress& addr);
    bool removeFromBlacklist(const core::ExtendedAddress& addr);
    bool isBlacklisted(const core::ExtendedAddress& addr) const;

    bool exportKeys(const std::string& path);
    bool importKeys(const std::string& path);

private:
    void xorBlocks(uint8_t* result, const uint8_t* a, const uint8_t* b, size_t len);
    void mbedtlsAesEncrypt(const uint8_t key[16], const uint8_t input[16], uint8_t output[16]);
    void mbedtlsCcmEncrypt(const uint8_t key[16], uint32_t frame_counter,
                          uint8_t nonce[13], uint8_t* data, size_t len,
                          uint8_t* mic, size_t mic_len);
    bool computeAesKey(const uint8_t* input, size_t len, uint8_t output[16]);

    SecurityKey network_key_;
    std::map<core::ExtendedAddress, ApsKeyPair> link_keys_;
    std::vector<core::ExtendedAddress> whitelist_;
    std::vector<core::ExtendedAddress> blacklist_;
    bool trust_center_enabled_{false};
    bool initialized_{false};
};

} // namespace zigbee_mesh::zigbee
