#pragma once

#include <Arduino.h>

namespace RgbConfig {

// GPIOs drive the gates of three external logic-level N-channel MOSFETs.
// Do not connect the LED strip channels directly to these pins.
// The installed strip's leads are ordered B, R, G across each three-pin bank.
// Keep these names logical so the dashboard and telemetry report actual color.
constexpr uint8_t RED_PIN = 5;
constexpr uint8_t GREEN_PIN = 6;
constexpr uint8_t BLUE_PIN = 4;
constexpr uint8_t SEGMENT_2_RED_PIN = 8;
constexpr uint8_t SEGMENT_2_GREEN_PIN = 9;
constexpr uint8_t SEGMENT_2_BLUE_PIN = 7;
constexpr uint8_t ONBOARD_NEOPIXEL_PIN = 48;

constexpr uint8_t RED_CHANNEL = 0;
constexpr uint8_t GREEN_CHANNEL = 1;
constexpr uint8_t BLUE_CHANNEL = 2;
constexpr uint8_t SEGMENT_2_RED_CHANNEL = 3;
constexpr uint8_t SEGMENT_2_GREEN_CHANNEL = 4;
constexpr uint8_t SEGMENT_2_BLUE_CHANNEL = 5;

constexpr uint32_t PWM_FREQUENCY_HZ = 5000;
constexpr uint8_t PWM_RESOLUTION_BITS = 8;
constexpr uint8_t PWM_MAX = 255;

// Default for newline-delimited serial telemetry. It can also be changed and
// saved at runtime with the dashboard's Enable serial debugging checkbox.
constexpr bool DEBUG_ENABLED = true;
constexpr uint32_t DEBUG_INTERVAL_MS = 500;

}  // namespace RgbConfig
