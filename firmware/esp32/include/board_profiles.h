#pragma once
#include <stdint.h>

// Select with PlatformIO environment / -DKILN_BOARD_PROFILE=<id>.
// Profile IDs are stable and may be used in persisted device configuration.
#define KILN_BOARD_CYD_28_ILI9341_XPT2046 1
#define KILN_BOARD_CYD_28_ST7789_XPT2046 2
#define KILN_BOARD_CYD_28_ILI9341_CST820 3
#define KILN_BOARD_CYD_35_ST7796_XPT2046 4
#define KILN_BOARD_CYD_43_S3_RGB_GT911 5
#define KILN_BOARD_CYD_70_S3_RGB_GT911 6
#define KILN_BOARD_CYD_24_FAMILY 7
#define KILN_BOARD_CYD_32_ST7789 8
#define KILN_BOARD_CYD_35_ST7796_GT911 9
#define KILN_BOARD_CUSTOM 99

#ifndef KILN_BOARD_PROFILE
#define KILN_BOARD_PROFILE KILN_BOARD_CYD_28_ILI9341_XPT2046
#endif

enum class DisplayBus : uint8_t { SPI, RGB_PARALLEL };
enum class DisplayDriver : uint8_t { ILI9341, ST7789, ST7796, ST7262, UNKNOWN };
enum class TouchDriver : uint8_t { XPT2046, CST820, GT911, NONE, UNKNOWN };
enum class EspTarget : uint8_t { ESP32, ESP32_S3, UNKNOWN };

struct BoardProfile {
  uint8_t id;
  const char* name;
  const char* commonModel;
  EspTarget target;
  uint16_t width;
  uint16_t height;
  DisplayBus displayBus;
  DisplayDriver displayDriver;
  TouchDriver touchDriver;
  bool pinMapVerified;
};

// IMPORTANT:
// The model name alone does not uniquely identify panel-controller revisions.
// These profiles describe likely/common configurations. Confirm the actual
// controller and board revision before enabling the display/touch driver.
// Unknown pin maps are deliberately NOT populated here.
#if KILN_BOARD_PROFILE == KILN_BOARD_CYD_28_ILI9341_XPT2046
static constexpr BoardProfile KILN_BOARD = {
  1, "CYD 2.8 ILI9341 + XPT2046", "ESP32-2432S028R",
  EspTarget::ESP32, 240, 320, DisplayBus::SPI,
  DisplayDriver::ILI9341, TouchDriver::XPT2046, true
};
#elif KILN_BOARD_PROFILE == KILN_BOARD_CYD_28_ST7789_XPT2046
static constexpr BoardProfile KILN_BOARD = {
  2, "CYD 2.8 ST7789 + XPT2046", "ESP32-2432S028R / dual-USB revision",
  EspTarget::ESP32, 240, 320, DisplayBus::SPI,
  DisplayDriver::ST7789, TouchDriver::XPT2046, false
};
#elif KILN_BOARD_PROFILE == KILN_BOARD_CYD_28_ILI9341_CST820
static constexpr BoardProfile KILN_BOARD = {
  3, "CYD 2.8 ILI9341 + CST820 capacitive", "ESP32-2432S028C",
  EspTarget::ESP32, 240, 320, DisplayBus::SPI,
  DisplayDriver::ILI9341, TouchDriver::CST820, false
};
#elif KILN_BOARD_PROFILE == KILN_BOARD_CYD_35_ST7796_XPT2046
static constexpr BoardProfile KILN_BOARD = {
  4, "CYD 3.5 ST7796 + resistive touch variant", "ESP32-3248S035R",
  EspTarget::ESP32, 320, 480, DisplayBus::SPI,
  DisplayDriver::ST7796, TouchDriver::UNKNOWN, false
};
#elif KILN_BOARD_PROFILE == KILN_BOARD_CYD_43_S3_RGB_GT911
static constexpr BoardProfile KILN_BOARD = {
  5, "CYD 4.3 ESP32-S3 RGB + GT911", "ESP32-8048S043",
  EspTarget::ESP32_S3, 800, 480, DisplayBus::RGB_PARALLEL,
  DisplayDriver::ST7262, TouchDriver::GT911, false
};
#elif KILN_BOARD_PROFILE == KILN_BOARD_CYD_70_S3_RGB_GT911
static constexpr BoardProfile KILN_BOARD = {
  6, "CYD 7.0 ESP32-S3 RGB + GT911", "ESP32-8048S070",
  EspTarget::ESP32_S3, 800, 480, DisplayBus::RGB_PARALLEL,
  DisplayDriver::ST7262, TouchDriver::GT911, false
};
#elif KILN_BOARD_PROFILE == KILN_BOARD_CYD_24_FAMILY
static constexpr BoardProfile KILN_BOARD = {
  7, "CYD 2.4-inch family (controller/touch revision varies)", "ESP32-2432S024",
  EspTarget::ESP32, 240, 320, DisplayBus::SPI,
  DisplayDriver::UNKNOWN, TouchDriver::UNKNOWN, false
};
#elif KILN_BOARD_PROFILE == KILN_BOARD_CYD_32_ST7789
static constexpr BoardProfile KILN_BOARD = {
  8, "CYD 3.2-inch ST7789 family", "ESP32-2432S032",
  EspTarget::ESP32, 240, 320, DisplayBus::SPI,
  DisplayDriver::ST7789, TouchDriver::UNKNOWN, false
};
#elif KILN_BOARD_PROFILE == KILN_BOARD_CYD_35_ST7796_GT911
static constexpr BoardProfile KILN_BOARD = {
  9, "CYD 3.5 ST7796 + GT911 capacitive", "ESP32-3248S035C",
  EspTarget::ESP32, 320, 480, DisplayBus::SPI,
  DisplayDriver::ST7796, TouchDriver::GT911, false
};
#elif KILN_BOARD_PROFILE == KILN_BOARD_CUSTOM
static constexpr BoardProfile KILN_BOARD = {
  99, "Custom CYD-compatible board", "User-defined",
  EspTarget::UNKNOWN, 0, 0, DisplayBus::SPI,
  DisplayDriver::UNKNOWN, TouchDriver::UNKNOWN, false
};
#else
#error "Unknown KILN_BOARD_PROFILE. Select a supported profile in platformio.ini."
#endif

// GPIO map for the common classic 2.8-inch SPI CYD layout only.
// Do not reuse this map for a different board family without schematic review.
struct Cyd28ClassicPins {
  static constexpr int8_t DISPLAY_MISO = 12;
  static constexpr int8_t DISPLAY_MOSI = 13;
  static constexpr int8_t DISPLAY_SCLK = 14;
  static constexpr int8_t DISPLAY_CS = 15;
  static constexpr int8_t DISPLAY_DC = 2;
  static constexpr int8_t DISPLAY_RST = -1; // board reset
  static constexpr int8_t DISPLAY_BACKLIGHT = 21;
  static constexpr int8_t TOUCH_CLK = 25;
  static constexpr int8_t TOUCH_MOSI = 32;
  static constexpr int8_t TOUCH_MISO = 39;
  static constexpr int8_t TOUCH_CS = 33;
  static constexpr int8_t TOUCH_IRQ = 36;
  static constexpr int8_t UART_RX = 22;
  static constexpr int8_t UART_TX = 27;
};
