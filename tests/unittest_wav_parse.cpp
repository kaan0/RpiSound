#include <gtest/gtest.h>

#include <memory>

#include "rpi_sound/wav_parser.hpp"

class WavParserTest : public ::testing::Test {

protected:

    void SetUp() override {
        testee_ = std::make_unique<WavParser>();
    }

    void TearDown() override {

    }
    std::unique_ptr<WavParser> testee_;
};

TEST_F(WavParserTest, TestValidFile) {
    // When

    // Then
    auto is_loaded = testee_->load("test.wav");

    // Expect
    EXPECT_EQ(is_loaded, true);
}