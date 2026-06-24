#pragma once

#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <memory>
#include <unordered_map>

namespace zigbee_mesh::core {

using TimerCallback = std::function<void()>;
using TimerId = uint64_t;
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

class Timer {
public:
    Timer() : id_(0), running_(false) {}

    explicit Timer(TimerCallback cb)
        : id_(0), callback_(std::move(cb)), running_(false) {}

    ~Timer() { stop(); }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    Timer(Timer&& other) noexcept
        : id_(other.id_), callback_(std::move(other.callback_)),
          running_(other.running_.load()) {
        other.running_ = false;
    }

    void start(Duration delay) {
        stop();
        running_ = true;
        end_time_ = Clock::now() + delay;
        thread_ = std::thread([this] {
            std::unique_lock<std::mutex> lock(mutex_);
            auto remaining = end_time_ - Clock::now();
            cv_.wait_for(lock, remaining, [this] {
                return !running_.load();
            });
            if (running_.load()) {
                running_ = false;
                if (callback_) callback_();
            }
        });
    }

    void startMs(int64_t ms) {
        start(std::chrono::milliseconds(ms));
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        cv_.notify_all();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    void reset(Duration delay) {
        stop();
        start(delay);
    }

    bool isRunning() const { return running_.load(); }

    Duration remaining() const {
        if (!running_) return Duration::zero();
        auto rem = end_time_ - Clock::now();
        return rem > Duration::zero() ? rem : Duration::zero();
    }

    void setCallback(TimerCallback cb) { callback_ = std::move(cb); }
    TimerId getId() const { return id_; }

private:
    TimerId id_;
    TimerCallback callback_;
    TimePoint end_time_;
    std::atomic<bool> running_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

class IntervalTimer {
public:
    IntervalTimer() : running_(false) {}
    ~IntervalTimer() { stop(); }

    IntervalTimer(const IntervalTimer&) = delete;
    IntervalTimer& operator=(const IntervalTimer&) = delete;

    void start(Duration interval, TimerCallback callback) {
        stop();
        interval_ = interval;
        callback_ = std::move(callback);
        running_ = true;
        thread_ = std::thread([this] {
            while (running_.load()) {
                auto next = Clock::now() + interval_;
                if (callback_) callback_();
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait_until(lock, next, [this] {
                    return !running_.load();
                });
            }
        });
    }

    void startMs(int64_t ms, TimerCallback callback) {
        start(std::chrono::milliseconds(ms), std::move(callback));
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        cv_.notify_all();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    void setInterval(Duration interval) { interval_ = interval; }
    bool isRunning() const { return running_.load(); }

private:
    Duration interval_;
    TimerCallback callback_;
    std::atomic<bool> running_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

class TimerManager {
public:
    TimerManager() : next_id_(1) {}
    ~TimerManager() { cancelAll(); }

    TimerId schedule(Duration delay, TimerCallback callback) {
        auto timer = std::make_shared<Timer>(std::move(callback));
        TimerId id = next_id_++;
        timer->id_ = id;
        timer->start(delay);

        std::lock_guard<std::mutex> lock(mutex_);
        timers_[id] = timer;
        return id;
    }

    TimerId scheduleMs(int64_t ms, TimerCallback callback) {
        return schedule(std::chrono::milliseconds(ms), std::move(callback));
    }

    TimerId scheduleInterval(Duration interval, TimerCallback callback) {
        auto timer = std::make_unique<IntervalTimer>();
        TimerId id = next_id_++;
        timer->start(interval, std::move(callback));

        std::lock_guard<std::mutex> lock(mutex_);
        interval_timers_[id] = std::move(timer);
        return id;
    }

    void cancel(TimerId id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (auto it = timers_.find(id); it != timers_.end()) {
            it->second->stop();
            timers_.erase(it);
        }
        if (auto it = interval_timers_.find(id); it != interval_timers_.end()) {
            it->second->stop();
            interval_timers_.erase(it);
        }
    }

    void cancelAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, timer] : timers_) {
            timer->stop();
        }
        timers_.clear();
        for (auto& [id, timer] : interval_timers_) {
            timer->stop();
        }
        interval_timers_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<TimerId, std::shared_ptr<Timer>> timers_;
    std::unordered_map<TimerId, std::unique_ptr<IntervalTimer>> interval_timers_;
    std::atomic<TimerId> next_id_;
};

} // namespace zigbee_mesh::core
