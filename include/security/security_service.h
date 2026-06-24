#pragma once

#include "core/types.h"
#include "zigbee/zigbee_stack.h"
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <functional>

namespace zigbee_mesh::security {

struct SecurityKeyEntry {
    enum class Type : uint8_t {
        NetworkKey,
        LinkKey,
        MasterKey,
        InstallCode,
    };
    Type type;
    uint8_t key[16]{};
    core::ExtendedAddress partner_addr{0};
    uint32_t frame_counter{0};
    bool active{true};
};

struct TrustCenterPolicy {
    bool require_authentication{true};
    bool allow_open_join{false};
    uint8_t default_security_level{6};
    uint16_t security_materials_threshold{100};
};

class SecurityService {
public:
    SecurityService() = default;

    bool init(const core::Config& config);

    bool installNetworkKey(const uint8_t key[16]);
    bool generateNetworkKey();
    const uint8_t* getNetworkKey() const;

    bool installLinkKey(const core::ExtendedAddress& device, const uint8_t key[16]);
    bool removeLinkKey(const core::ExtendedAddress& device);
    const SecurityKeyEntry* getLinkKey(const core::ExtendedAddress& device) const;

    bool setInstallCode(const core::ExtendedAddress& device, const uint8_t* code, size_t len);
    bool computeKeyFromInstallCode(const uint8_t* code, size_t len, uint8_t key[16]);

    bool encryptNwkPayload(uint8_t* payload, size_t len, uint32_t& frame_counter);
    bool decryptNwkPayload(uint8_t* payload, size_t len, uint32_t frame_counter);

    bool encryptApsPayload(const core::ExtendedAddress& addr, uint8_t* payload, size_t len);
    bool decryptApsPayload(const core::ExtendedAddress& addr, uint8_t* payload, size_t len);

    bool addToWhitelist(const core::ExtendedAddress& addr);
    bool removeFromWhitelist(const core::ExtendedAddress& addr);
    bool isWhitelisted(const core::ExtendedAddress& addr) const;
    std::vector<core::ExtendedAddress> getWhitelist() const;

    bool addToBlacklist(const core::ExtendedAddress& addr);
    bool removeFromBlacklist(const core::ExtendedAddress& addr);
    bool isBlacklisted(const core::ExtendedAddress& addr) const;
    std::vector<core::ExtendedAddress> getBlacklist() const;

    bool isTrustCenter() const { return is_trust_center_; }
    void setTrustCenter(bool tc) { is_trust_center_ = tc; }

    bool setTrustCenterPolicy(const TrustCenterPolicy& policy);
    TrustCenterPolicy getTrustCenterPolicy() const;

    std::string getKeyStatus() const;
    size_t getKeyCount() const;
    bool exportKeys(const std::string& path) const;
    bool importKeys(const std::string& path);

    void removeExpiredKeys(uint32_t max_age_ms);

private:
    static void generateRandomKey(uint8_t key[16]);
    static void aesEncrypt(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);
    static void ccmEncrypt(const uint8_t key[16], const uint8_t nonce[13],
                          uint8_t* data, size_t len, uint8_t* mic, size_t mic_len);

    mutable std::mutex mutex_;
    SecurityKeyEntry network_key_;
    std::map<core::ExtendedAddress, SecurityKeyEntry> link_keys_;
    std::vector<core::ExtendedAddress> whitelist_;
    std::vector<core::ExtendedAddress> blacklist_;
    TrustCenterPolicy policy_;
    bool is_trust_center_{false};
    bool initialized_{false};
};

} // namespace zigbee_mesh::security
