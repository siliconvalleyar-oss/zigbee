#pragma once

#include "ieee802154.h"
#include "security_manager.h"
#include "core/timer.h"
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

namespace zigbee_mesh::zigbee {

enum class ZdoCommandId : uint8_t {
    NetworkAddrReq = 0x00,
    NetworkAddrResp = 0x80,
    IeeeAddrReq = 0x01,
    IeeeAddrResp = 0x81,
    NodeDescReq = 0x02,
    NodeDescResp = 0x82,
    PowerDescReq = 0x03,
    PowerDescResp = 0x83,
    ActiveEpReq = 0x04,
    ActiveEpResp = 0x84,
    SimpleDescReq = 0x05,
    SimpleDescResp = 0x85,
    MatchDescReq = 0x06,
    MatchDescResp = 0x86,
    ComplexDescReq = 0x10,
    ComplexDescResp = 0x90,
    UserDescReq = 0x11,
    UserDescResp = 0x91,
    DiscoveryCacheReq = 0x12,
    DeviceAnnce = 0x13,
    UserDescConf = 0x14,
    SystemServerDiscoveryReq = 0x15,
    SystemServerDiscoveryResp = 0x95,
    ActiveEpAlertReq = 0x16,
    ActiveEpAlertConf = 0x17,
    EndDeviceBindReq = 0x20,
    EndDeviceBindConf = 0xA0,
    BindReq = 0x21,
    BindConf = 0xA1,
    UnbindReq = 0x22,
    UnbindConf = 0xA2,
    MgmtNwkDiscReq = 0x30,
    MgmtNwkDiscResp = 0xB0,
    MgmtLqiReq = 0x31,
    MgmtLqiResp = 0xB1,
    MgmtRtgReq = 0x32,
    MgmtRtgResp = 0xB2,
    MgmtBindReq = 0x33,
    MgmtBindResp = 0xB3,
    MgmtLeaveReq = 0x34,
    MgmtLeaveConf = 0xB4,
    MgmtDirectJoinReq = 0x35,
    MgmtDirectJoinConf = 0xB5,
    MgmtPermitJoinReq = 0x36,
    MgmtPermitJoinConf = 0xB6,
    MgmtNwkUpdateReq = 0x38,
    MgmtNwkUpdateNotify = 0xB8,
};

enum class ZdpStatusCode : uint8_t {
    Success = 0x00,
    InvalidParameter = 0xC1,
    InvalidEndpoint = 0xC2,
    InvalidCluster = 0xC3,
    NotActive = 0xC4,
    NotSupported = 0xC5,
    NoKey = 0xC6,
    NoEntry = 0xC7,
    NoDescriptor = 0xC8,
    NotAuthenticated = 0xC9,
    NoMatching = 0xCA,
    NoTable = 0xCB,
    NoAddress = 0xCC,
};

struct ZdoFrame {
    ZdoCommandId command_id;
    core::ShortAddress src_addr{0};
    core::ShortAddress dest_addr{0};
    uint8_t transaction_id{0};
    std::vector<uint8_t> payload;

    core::ByteBuffer serialize() const;
    static bool deserialize(const core::ByteBuffer& data, ZdoFrame& frame);
};

class ZdoLayer {
public:
    using ZdoCallback = std::function<void(const ZdoFrame&)>;
    using PendingZdoRequest = struct {
        ZdoCommandId expected_response;
        ZdoCallback callback;
        core::Timer timer;
    };

    ZdoLayer(Ieee802154Layer* ieee802154, SecurityManager* security);

    bool init();
    bool sendZdoRequest(const ZdoFrame& request, ZdoCallback callback, int timeout_ms = 3000);

    bool networkAddrRequest(core::ExtendedAddress ieee_addr, bool request_type, uint8_t start_index);
    bool ieeeAddrRequest(core::ShortAddress nwk_addr, bool request_type, uint8_t start_index);
    bool nodeDescRequest(core::ShortAddress dest);
    bool powerDescRequest(core::ShortAddress dest);
    bool activeEpRequest(core::ShortAddress dest);
    bool simpleDescRequest(core::ShortAddress dest, uint8_t endpoint);
    bool matchDescRequest(core::ShortAddress dest, uint16_t profile_id, const std::vector<uint16_t>& clusters);
    bool deviceAnnce(core::ShortAddress nwk_addr, core::ExtendedAddress ieee_addr, uint8_t capability);
    bool bindRequest(const core::ExtendedAddress& src_addr, uint8_t src_endpoint,
                     uint16_t cluster_id, const core::ExtendedAddress& dst_addr,
                     uint8_t dst_endpoint);
    bool unbindRequest(const core::ExtendedAddress& src_addr, uint8_t src_endpoint,
                       uint16_t cluster_id, const core::ExtendedAddress& dst_addr,
                       uint8_t dst_endpoint);
    bool mgmtLqiRequest(core::ShortAddress dest, uint8_t start_index = 0);
    bool mgmtRtgRequest(core::ShortAddress dest, uint8_t start_index = 0);
    bool mgmtLeaveRequest(core::ShortAddress dest, const core::ExtendedAddress& device_addr,
                          bool remove_children, bool rejoin);
    bool mgmtPermitJoinRequest(core::ShortAddress dest, uint8_t duration, bool authentication);

    void handleIncomingZdo(const ZdoFrame& frame);

    void setZdoCallback(ZdoCommandId cmd_id, ZdoCallback cb);

private:
    Ieee802154Layer* ieee802154_{nullptr};
    SecurityManager* security_{nullptr};
    core::TimerManager timer_mgr_;
    std::map<ZdoCommandId, ZdoCallback> zdo_callbacks_;
    std::map<uint8_t, PendingZdoRequest> pending_requests_;
    std::mutex mutex_;
    uint8_t transaction_id_{0};
    bool initialized_{false};
};

} // namespace zigbee_mesh::zigbee
