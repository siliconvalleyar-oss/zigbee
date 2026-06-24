#include <gtest/gtest.h>
#include "core/result.h"

using namespace zigbee_mesh::core;

TEST(ResultTest, SuccessValue) {
    Result<int> result(42);
    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.hasError());
    EXPECT_EQ(result.value(), 42);
}

TEST(ResultTest, ErrorCode) {
    Result<int> result(ErrorCode::NotFound);
    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.errorCode(), ErrorCode::NotFound);
}

TEST(ResultTest, ErrorMessage) {
    Result<int> result(ErrorCode::Timeout, "request timed out");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errorMessage(), "request timed out");
}

TEST(ResultTest, ValueOr) {
    Result<int> result(ErrorCode::NotFound);
    EXPECT_EQ(result.valueOr(99), 99);
}

TEST(ResultTest, ValueOrWithSuccess) {
    Result<int> result(42);
    EXPECT_EQ(result.valueOr(99), 42);
}

TEST(ResultTest, ValueThrowsOnError) {
    Result<int> result(ErrorCode::NotFound);
    EXPECT_THROW(result.value(), std::runtime_error);
}

TEST(ResultTest, VoidResultSuccess) {
    VoidResult result;
    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.hasError());
}

TEST(ResultTest, VoidResultError) {
    VoidResult result(ErrorCode::GenericError, "something failed");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errorCode(), ErrorCode::GenericError);
    EXPECT_EQ(result.errorMessage(), "something failed");
}

TEST(ResultTest, Map) {
    Result<int> result(21);
    auto mapped = result.map<int>([](const int& v) { return v * 2; });
    EXPECT_TRUE(mapped.ok());
    EXPECT_EQ(mapped.value(), 42);
}

TEST(ResultTest, MapOnError) {
    Result<int> result(ErrorCode::NotFound);
    auto mapped = result.map<int>([](const int& v) { return v * 2; });
    EXPECT_FALSE(mapped.ok());
    EXPECT_EQ(mapped.errorCode(), ErrorCode::NotFound);
}

TEST(ResultTest, Otherwise) {
    Result<int> result(ErrorCode::NotFound);
    auto otherwise = result.otherwise([](ErrorCode, const std::string&) {
        return Result<int>(100);
    });
    EXPECT_TRUE(otherwise.ok());
    EXPECT_EQ(otherwise.value(), 100);
}
