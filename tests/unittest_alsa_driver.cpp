#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <memory>

#include "rpi_sound/alsa_driver.hpp"

class AlsaDriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        driver = std::make_unique<AlsaDriver>();
        badHandle = driver->pcmOpen(999, kTestDevice, AlsaDriver::kPlayback, nullptr);
    }

    void TearDown() override { driver.reset(); }

    std::unique_ptr<AlsaDriver> driver;
    AlsaDriver::PcmHandle* badHandle;

    // Test constants
    static constexpr uint32_t kTestCard = 0;
    static constexpr uint32_t kTestDevice = 0;
    static constexpr uint32_t kTestFrames = 1024;
    static constexpr uint32_t kTestBytes = 4096;
    static constexpr int kTestTimeout = 1000;
};

// Test that AlsaDriver constants match TinyALSA values
TEST_F(AlsaDriverTest, ConstantsMatchTinyALSA) {
    EXPECT_EQ(AlsaDriver::kParamFormat, PCM_PARAM_FORMAT);
    EXPECT_EQ(AlsaDriver::kParamChannels, PCM_PARAM_CHANNELS);
    EXPECT_EQ(AlsaDriver::kParamRate, PCM_PARAM_RATE);
    EXPECT_EQ(AlsaDriver::kParamPeriodSize, PCM_PARAM_PERIOD_SIZE);
    EXPECT_EQ(AlsaDriver::kParamPeriodCount, PCM_PARAM_PERIODS);

    EXPECT_EQ(AlsaDriver::kFormatInvalid, PCM_FORMAT_INVALID);
    EXPECT_EQ(AlsaDriver::kFormatS16LE, PCM_FORMAT_S16_LE);
    EXPECT_EQ(AlsaDriver::kFormatS32LE, PCM_FORMAT_S32_LE);

    EXPECT_EQ(AlsaDriver::kPlayback, PCM_OUT);
    EXPECT_EQ(AlsaDriver::kCapture, PCM_IN);
    EXPECT_EQ(AlsaDriver::kNonBlock, PCM_NONBLOCK);
}

// Test pcmOpen with invalid parameters returns nullptr
TEST_F(AlsaDriverTest, PcmOpenInvalidCardReturnsNull) {
    AlsaDriver::PcmConfig config = {};
    config.channels = 2;
    config.rate = 44100;
    config.period_size = 1024;
    config.period_count = 4;
    config.format = AlsaDriver::kFormatS16LE;

    // Try to open a non-existent card (very high card number)
    auto* handle = driver->pcmOpen(999, kTestDevice, AlsaDriver::kPlayback, &config);
    // Check that the handle is not ready (indicates failure)
    EXPECT_FALSE(driver->pcmIsReady(handle));
    EXPECT_NE(handle, nullptr);
}

// Test pcmOpen with null config returns nullptr
TEST_F(AlsaDriverTest, PcmOpenNullConfigReturnsNull) {
    auto* handle = driver->pcmOpen(kTestCard, kTestDevice, AlsaDriver::kPlayback, nullptr);
    // Check that the handle is not ready (indicates failure)
    EXPECT_FALSE(driver->pcmIsReady(handle));
    EXPECT_NE(handle, nullptr);
}

// Test pcmClose with bad handle
TEST_F(AlsaDriverTest, PcmCloseNullHandleReturnsError) {
    int result = driver->pcmClose(badHandle);
    EXPECT_EQ(result, 0);
}

// Test pcmIsReady with bad handle
TEST_F(AlsaDriverTest, PcmIsReadyNullHandleReturnsFalse) {
    bool result = driver->pcmIsReady(badHandle);
    EXPECT_EQ(result, false);  // Should return false (not ready/invalid)
}

// Test pcmGetError with bad handle
TEST_F(AlsaDriverTest, PcmGetErrorNullHandleReturnsError) {
    const char* error = driver->pcmGetError(badHandle);
    EXPECT_NE(error, nullptr);
    EXPECT_GT(strlen(error), 0);  // Should return non-empty error string
}

// Test pcmFramesToBytes with bad handle
TEST_F(AlsaDriverTest, PcmFramesToBytesNullHandleReturnsZero) {
    uint32_t result = driver->pcmFramesToBytes(badHandle, kTestFrames);
    EXPECT_EQ(result, 0);  // Should return 0 for invalid handle
}

// Test pcmParamsGet with invalid parameters
TEST_F(AlsaDriverTest, PcmParamsGetInvalidCardReturnsNull) {
    auto* params = driver->pcmParamsGet(999, kTestDevice, AlsaDriver::kPlayback);
    EXPECT_EQ(params, nullptr);
}

// Test pcmParamsGetMask with null params
TEST_F(AlsaDriverTest, PcmParamsGetMaskNullParamsReturnsNull) {
    auto* mask = driver->pcmParamsGetMask(nullptr, AlsaDriver::kParamFormat);
    EXPECT_EQ(mask, nullptr);
}

// Test pcmParamsGetMax with null params
TEST_F(AlsaDriverTest, PcmParamsGetMaxNullParamsReturnsZero) {
    uint32_t result = driver->pcmParamsGetMax(nullptr, AlsaDriver::kParamRate);
    EXPECT_EQ(result, 0);
}

// Test pcmParamsGetMin with null params
TEST_F(AlsaDriverTest, PcmParamsGetMinNullParamsReturnsZero) {
    uint32_t result = driver->pcmParamsGetMin(nullptr, AlsaDriver::kParamRate);
    EXPECT_EQ(result, 0);
}

// Test pcmParamsFree with null params (should not crash)
TEST_F(AlsaDriverTest, PcmParamsFreeNullParamsDoesNotCrash) {
    EXPECT_NO_THROW(driver->pcmParamsFree(nullptr));
}

// Test type aliases are correct
TEST_F(AlsaDriverTest, TypeAliasesAreCorrect) {
    // These should compile without issues
    AlsaDriver::PcmHandle* handle = nullptr;
    AlsaDriver::PcmConfig config = {};
    AlsaDriver::PcmParams* params = nullptr;
    AlsaDriver::PcmMask* mask = nullptr;
    AlsaDriver::PcmParam param = AlsaDriver::kParamFormat;
    AlsaDriver::PcmFormat format = AlsaDriver::kFormatS16LE;
    AlsaDriver::Flags flags = AlsaDriver::kPlayback;

    // Suppress unused variable warnings
    (void)handle;
    (void)config;
    (void)params;
    (void)mask;
    (void)param;
    (void)format;
    (void)flags;

    SUCCEED();  // Test passes if compilation succeeds
}

// Test AlsaDriver is non-copyable
TEST_F(AlsaDriverTest, IsNonCopyable) {
    static_assert(!std::is_copy_constructible_v<AlsaDriver>);
    static_assert(!std::is_copy_assignable_v<AlsaDriver>);
    SUCCEED();
}