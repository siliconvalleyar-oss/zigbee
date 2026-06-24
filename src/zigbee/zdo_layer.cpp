#include "zigbee/zdo_layer.h"
#include "core/logger.h"

namespace zigbee_mesh::zigbee {

core::ByteBuffer ZdoFrame::serialize() const {
    core::ByteBuffer buf;
    buf.appendU8(static_cast<uint8_t>(command_id));
    buf.appendU8(transaction_id);
    buf.append(payload.data(), payload.size());
    return buf;
}

bool ZdoFrame::deserialize(const core::ByteBuffer& data, ZdoFrame& frame) {
    if (data.size() < 2) return false;
    frame.command_id = static_cast<ZdoCommandId>(data.readU8(0));
    frame.transaction_id = data.readU8(1);
    if (data.size() > 2) {
        frame.payload.assign(data.data() + 2, data.data() + data.size());
    }
    return true;
}

ZdoLayer::ZdoLayer(Ieee802154Layer* ieee802154, SecurityManager* security)
    : ieee802154_(ieee802154), security_(security) {}

bool ZdoLayer::init() {
    initialized_ = true;
    ZIGBEE_LOG_INFO("ZDO layer initialized");
    return true;
}

bool ZdoLayer::sendZdoRequest(const ZdoFrame& request, ZdoCallback callback, int timeout_ms) {
    if (!initialized_) return false;

    core::ByteBuffer serialized = request.serialize();

    uint8_t t_id = transaction_id_++;
    PendingZdoRequest pending;
    pending.expected_response = static_cast<ZdoCommandId>(
        static_cast<uint8_t>(request.command_id) | 0x80);
    pending.callback = std::move(callback);

    std::lock_guard<std::mutex> lock(mutex_);
    pending_requests_[t_id] = std::move(pending);
    pending_requests_[t_id].timer.startMs(timeout_ms, [this, t_id]() {
        std::lock_guard<std::mutex> l(mutex_);
        pending_requests_.erase(t_id);
    });

    Ieee802154Frame frame;
    frame.frame_type = FrameType::Data;
    frame.sequence_number = t_id;
    frame.dest_addr = request.dest_addr;
    frame.dest_pan_id = ieee802154_->getChannel() ? 0 : 0;
    frame.payload.assign(serialized.data(), serialized.data() + serialized.size());

    return ieee802154_->sendFrame(frame);
}

bool ZdoLayer::networkAddrRequest(core::ExtendedAddress ieee_addr, bool request_type, uint8_t start_index) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::NetworkAddrReq;
    req.dest_addr = 0x0000;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU64LE(ieee_addr);
    payload.appendU8(request_type ? 0x00 : 0x01);
    payload.appendU8(start_index);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::ieeeAddrRequest(core::ShortAddress nwk_addr, bool request_type, uint8_t start_index) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::IeeeAddrReq;
    req.dest_addr = nwk_addr;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU16LE(nwk_addr);
    payload.appendU8(request_type ? 0x00 : 0x01);
    payload.appendU8(start_index);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::nodeDescRequest(core::ShortAddress dest) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::NodeDescReq;
    req.dest_addr = dest;
    req.transaction_id = transaction_id_++;
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::powerDescRequest(core::ShortAddress dest) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::PowerDescReq;
    req.dest_addr = dest;
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::activeEpRequest(core::ShortAddress dest) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::ActiveEpReq;
    req.dest_addr = dest;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU16LE(dest);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::simpleDescRequest(core::ShortAddress dest, uint8_t endpoint) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::SimpleDescReq;
    req.dest_addr = dest;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU16LE(dest);
    payload.appendU8(endpoint);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::matchDescRequest(core::ShortAddress dest, uint16_t profile_id,
                                 const std::vector<uint16_t>& clusters) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::MatchDescReq;
    req.dest_addr = dest;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU16LE(dest);
    payload.appendU16LE(profile_id);
    payload.appendU16LE(static_cast<uint16_t>(clusters.size()));
    payload.appendU16LE(static_cast<uint16_t>(0));
    for (auto c : clusters) payload.appendU16LE(c);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::deviceAnnce(core::ShortAddress nwk_addr, core::ExtendedAddress ieee_addr, uint8_t capability) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::DeviceAnnce;
    req.dest_addr = core::kBroadcastAddress;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU16LE(nwk_addr);
    payload.appendU64LE(ieee_addr);
    payload.appendU8(capability);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::bindRequest(const core::ExtendedAddress& src_addr, uint8_t src_endpoint,
                            uint16_t cluster_id, const core::ExtendedAddress& dst_addr,
                            uint8_t dst_endpoint) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::BindReq;
    req.dest_addr = 0x0000;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU64LE(src_addr);
    payload.appendU8(src_endpoint);
    payload.appendU16LE(cluster_id);
    payload.appendU8(0x01);
    payload.appendU64LE(dst_addr);
    payload.appendU8(dst_endpoint);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::unbindRequest(const core::ExtendedAddress& src_addr, uint8_t src_endpoint,
                              uint16_t cluster_id, const core::ExtendedAddress& dst_addr,
                              uint8_t dst_endpoint) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::UnbindReq;
    req.dest_addr = 0x0000;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU64LE(src_addr);
    payload.appendU8(src_endpoint);
    payload.appendU16LE(cluster_id);
    payload.appendU8(0x01);
    payload.appendU64LE(dst_addr);
    payload.appendU8(dst_endpoint);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::mgmtLqiRequest(core::ShortAddress dest, uint8_t start_index) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::MgmtLqiReq;
    req.dest_addr = dest;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU8(start_index);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::mgmtRtgRequest(core::ShortAddress dest, uint8_t start_index) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::MgmtRtgReq;
    req.dest_addr = dest;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU8(start_index);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::mgmtLeaveRequest(core::ShortAddress dest, const core::ExtendedAddress& device_addr,
                                 bool remove_children, bool rejoin) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::MgmtLeaveReq;
    req.dest_addr = dest;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU64LE(device_addr);
    payload.appendU8((remove_children ? 0x80 : 0x00) | (rejoin ? 0x40 : 0x00));
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

bool ZdoLayer::mgmtPermitJoinRequest(core::ShortAddress dest, uint8_t duration, bool authentication) {
    ZdoFrame req;
    req.command_id = ZdoCommandId::MgmtPermitJoinReq;
    req.dest_addr = dest;
    req.transaction_id = transaction_id_++;
    core::ByteBuffer payload;
    payload.appendU8(duration);
    payload.appendU8(authentication ? 0x01 : 0x00);
    req.payload.assign(payload.data(), payload.data() + payload.size());
    return sendZdoRequest(req, nullptr);
}

void ZdoLayer::handleIncomingZdo(const ZdoFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = zdo_callbacks_.find(frame.command_id);
    if (it != zdo_callbacks_.end()) {
        it->second(frame);
    }
}

void ZdoLayer::setZdoCallback(ZdoCommandId cmd_id, ZdoCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    zdo_callbacks_[cmd_id] = std::move(cb);
}

} // namespace zigbee_mesh::zigbee
