#pragma once

#include "types.h"
#include <variant>
#include <string>
#include <stdexcept>
#include <functional>
#include <optional>

namespace zigbee_mesh::core {

template <typename T>
class Result {
public:
    Result(T value) : data_(std::move(value)), error_(ErrorCode::Success) {}
    Result(ErrorCode error) : data_(std::nullopt), error_(error) {}
    Result(ErrorCode error, const std::string& msg)
        : data_(std::nullopt), error_(error), message_(msg) {}

    bool ok() const { return error_ == ErrorCode::Success; }
    bool hasError() const { return error_ != ErrorCode::Success; }

    ErrorCode errorCode() const { return error_; }
    const std::string& errorMessage() const { return message_; }

    const T& value() const & {
        if (hasError()) {
            throw std::runtime_error("Accessed value of error Result: " + message_);
        }
        return *data_;
    }

    T value() && {
        if (hasError()) {
            throw std::runtime_error("Accessed value of error Result: " + message_);
        }
        return std::move(*data_);
    }

    T valueOr(T&& default_val) const & {
        if (hasError()) {
            return std::forward<T>(default_val);
        }
        return *data_;
    }

    T valueOr(const T& default_val) const & {
        if (hasError()) {
            return default_val;
        }
        return *data_;
    }

    template <typename F>
    Result<T> otherwise(F&& func) const {
        if (hasError()) {
            return func(error_, message_);
        }
        return *this;
    }

    template <typename U>
    Result<U> map(std::function<U(const T&)> func) const {
        if (hasError()) {
            return Result<U>(error_, message_);
        }
        return Result<U>(func(*data_));
    }

private:
    std::optional<T> data_;
    ErrorCode error_;
    std::string message_;
};

class VoidResult {
public:
    VoidResult() : error_(ErrorCode::Success) {}
    VoidResult(ErrorCode error) : error_(error) {}
    VoidResult(ErrorCode error, const std::string& msg)
        : error_(error), message_(msg) {}

    bool ok() const { return error_ == ErrorCode::Success; }
    bool hasError() const { return error_ != ErrorCode::Success; }
    ErrorCode errorCode() const { return error_; }
    const std::string& errorMessage() const { return message_; }

    void throwIfError() const {
        if (hasError()) {
            throw std::runtime_error("VoidResult error: " + message_);
        }
    }

private:
    ErrorCode error_;
    std::string message_;
};

#define ZIGBEE_RESULT_OK(value) Result<decltype(value)>(value)
#define ZIGBEE_RESULT_ERROR(error_code) Result<std::nullptr_t>(error_code)
#define ZIGBEE_RESULT_ERROR_MSG(error_code, msg) Result<std::nullptr_t>(error_code, msg)

#define ZIGBEE_TRY_ASSIGN(var, expr) \
    auto _result_##var = (expr); \
    if (_result_##var.hasError()) { \
        return Result<std::decay_t<decltype(var)>>(_result_##var.errorCode(), _result_##var.errorMessage()); \
    } \
    auto var = std::move(_result_##var).value()

#define ZIGBEE_TRY(expr) \
    do { \
        auto _result = (expr); \
        if (_result.hasError()) { \
            return VoidResult(_result.errorCode(), _result.errorMessage()); \
        } \
    } while(0)

} // namespace zigbee_mesh::core
