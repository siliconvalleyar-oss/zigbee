#pragma once

#include "types.h"
#include <functional>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <memory>
#include <any>

namespace zigbee_mesh::core {

using EventTypeId = uint32_t;
using EventCallback = std::function<bool(const struct BaseEvent&)>;
using EventTimePoint = std::chrono::steady_clock::time_point;

enum class EventType : EventTypeId {
    None = 0,
    DeviceJoined = 1,
    DeviceLeft = 2,
    DeviceAnnounce = 3,
    PacketReceived = 4,
    PacketSent = 5,
    PacketFailed = 6,
    RouteChanged = 7,
    RouteDiscovery = 8,
    RouteError = 9,
    SecurityKeyChanged = 10,
    SecurityKeyRequest = 11,
    NetworkJoined = 12,
    NetworkLeft = 13,
    NetworkScan = 14,
    NetworkFormed = 15,
    ChannelChanged = 16,
    BindRequest = 17,
    BindConfirm = 18,
    UnbindRequest = 19,
    UnbindConfirm = 20,
    Heartbeat = 21,
    NodeDescriptor = 22,
    PowerDescriptor = 23,
    ActiveEndpoints = 24,
    SimpleDescriptor = 25,
    ZdoCallback = 26,
    TimerExpired = 27,
    StateChanged = 28,
    Diagnostics = 29,
    OtaStarted = 30,
    OtaProgress = 31,
    OtaCompleted = 32,
    OtaFailed = 33,
    UserDefined = 0x8000,
};

enum class EventPriority : uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3,
};

struct BaseEvent {
    EventType type{EventType::None};
    EventPriority priority{EventPriority::Normal};
    EventTimePoint timestamp{std::chrono::steady_clock::now()};
    ShortAddress source_addr{0};
    ShortAddress dest_addr{0};

    virtual ~BaseEvent() = default;
    virtual std::unique_ptr<BaseEvent> clone() const {
        return std::make_unique<BaseEvent>(*this);
    }
};

struct DeviceEvent : BaseEvent {
    MacAddress device_address;
    DeviceRole role{DeviceRole::Unknown};
    uint8_t lqi{0};
    int8_t rssi{0};

    std::unique_ptr<BaseEvent> clone() const override {
        return std::make_unique<DeviceEvent>(*this);
    }
};

struct PacketEvent : BaseEvent {
    std::vector<uint8_t> payload;
    FrameType frame_type{FrameType::Data};
    uint8_t lqi{0};
    int8_t rssi{0};
    uint16_t cluster_id{0};
    uint8_t endpoint{0};

    std::unique_ptr<BaseEvent> clone() const override {
        return std::make_unique<PacketEvent>(*this);
    }
};

struct RouteEvent : BaseEvent {
    ShortAddress target{0};
    ShortAddress next_hop{0};
    uint8_t metric{0};
    bool is_new_route{false};

    std::unique_ptr<BaseEvent> clone() const override {
        return std::make_unique<RouteEvent>(*this);
    }
};

struct SecurityEvent : BaseEvent {
    enum class Action {
        KeyInstalled,
        KeyRequested,
        KeyUpdated,
        KeyRemoved,
        AuthSuccess,
        AuthFailed,
    };
    Action action{Action::KeyInstalled};
    uint8_t key_type{0};
    ExtendedAddress partner_addr{0};

    std::unique_ptr<BaseEvent> clone() const override {
        return std::make_unique<SecurityEvent>(*this);
    }
};

struct NetworkEvent : BaseEvent {
    PanId pan_id{0};
    Channel channel{0};
    NwkState state{NwkState::OffNetwork};
    uint8_t nwk_update_id{0};

    std::unique_ptr<BaseEvent> clone() const override {
        return std::make_unique<NetworkEvent>(*this);
    }
};

class EventQueue {
public:
    explicit EventQueue(size_t max_size = 10000) : max_size_(max_size) {}

    bool push(std::unique_ptr<BaseEvent> event) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (events_.size() >= max_size_) {
            return false;
        }
        events_.push(std::move(event));
        cv_.notify_one();
        return true;
    }

    bool push(BaseEvent event) {
        return push(std::make_unique<BaseEvent>(std::move(event)));
    }

    std::unique_ptr<BaseEvent> pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (events_.empty()) {
            return nullptr;
        }
        auto event = std::move(events_.front());
        events_.pop();
        return event;
    }

    std::unique_ptr<BaseEvent> waitAndPop(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, timeout, [this] { return !events_.empty(); });
        if (events_.empty()) {
            return nullptr;
        }
        auto event = std::move(events_.front());
        events_.pop();
        return event;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!events_.empty()) {
            events_.pop();
        }
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::unique_ptr<BaseEvent>> events_;
    size_t max_size_;
};

using EventId = uint64_t;

class EventEmitter {
public:
    EventEmitter() : next_id_(1) {}

    EventId on(EventType type, EventCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        EventId id = next_id_++;
        listeners_[type].push_back({id, std::move(callback)});
        return id;
    }

    void off(EventId id) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [type, listeners] : listeners_) {
            auto it = std::remove_if(
                listeners.begin(), listeners.end(),
                [id](const ListenerEntry& e) { return e.id == id; });
            listeners.erase(it, listeners.end());
        }
    }

    void emit(const BaseEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = listeners_.find(event.type);
        if (it != listeners_.end()) {
            for (auto& entry : it->second) {
                entry.callback(event);
            }
        }
    }

    void emitAsync(std::unique_ptr<BaseEvent> event) {
        event_queue_.push(std::move(event));
    }

    EventQueue& queue() { return event_queue_; }

private:
    struct ListenerEntry {
        EventId id;
        EventCallback callback;
    };

    std::mutex mutex_;
    std::unordered_map<EventType, std::vector<ListenerEntry>> listeners_;
    EventQueue event_queue_;
    EventId next_id_;
};

} // namespace zigbee_mesh::core
