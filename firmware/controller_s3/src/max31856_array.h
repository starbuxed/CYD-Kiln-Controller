#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MAX31856.h>
#include "sensor_manager.h"

// Supply these from the selected S3 board pin map. Defaults disable the
// front end so no guessed GPIO assignment can activate a bus.
#ifndef KILN_TC_SPI_SCLK
#define KILN_TC_SPI_SCLK -1
#endif
#ifndef KILN_TC_SPI_MISO
#define KILN_TC_SPI_MISO -1
#endif
#ifndef KILN_TC_SPI_MOSI
#define KILN_TC_SPI_MOSI -1
#endif
#ifndef KILN_TC1_CS_PIN
#define KILN_TC1_CS_PIN -1
#endif
#ifndef KILN_TC2_CS_PIN
#define KILN_TC2_CS_PIN -1
#endif
#ifndef KILN_TC3_CS_PIN
#define KILN_TC3_CS_PIN -1
#endif
#ifndef KILN_TC4_CS_PIN
#define KILN_TC4_CS_PIN -1
#endif

namespace kiln {
class Max31856Array {
 public:
  Max31856Array();
  bool begin();
  bool configureChannel(uint8_t channel, ThermocoupleType type);
  void update(SensorManager& manager, uint32_t nowMs);
  bool ready() const { return ready_; }
  bool channelReady(uint8_t channel) const;
 private:
  Adafruit_MAX31856* devices_[TEMPERATURE_CHANNELS];
  bool channelReady_[TEMPERATURE_CHANNELS];
  bool ready_;
  uint32_t lastReadMs_;
};
} // namespace kiln
