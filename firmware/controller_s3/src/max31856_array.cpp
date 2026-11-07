#include "max31856_array.h"
#include <math.h>

namespace kiln {
static Adafruit_MAX31856 tc0(KILN_TC1_CS_PIN, &SPI);
static Adafruit_MAX31856 tc1(KILN_TC2_CS_PIN, &SPI);
static Adafruit_MAX31856 tc2(KILN_TC3_CS_PIN, &SPI);
static Adafruit_MAX31856 tc3(KILN_TC4_CS_PIN, &SPI);

Max31856Array::Max31856Array() : ready_(false), lastReadMs_(0) {
  devices_[0] = &tc0;
  devices_[1] = &tc1;
  devices_[2] = &tc2;
  devices_[3] = &tc3;
  for (uint8_t i = 0; i < TEMPERATURE_CHANNELS; ++i)
    channelReady_[i] = false;
}

bool Max31856Array::begin() {
  ready_ = false;
  const int8_t cs[TEMPERATURE_CHANNELS] = {
    KILN_TC1_CS_PIN, KILN_TC2_CS_PIN, KILN_TC3_CS_PIN, KILN_TC4_CS_PIN
  };
  if (KILN_TC_SPI_SCLK < 0 || KILN_TC_SPI_MISO < 0 ||
      KILN_TC_SPI_MOSI < 0) return false;
  uint8_t requested = 0;
  for (uint8_t i = 0; i < TEMPERATURE_CHANNELS; ++i) {
    channelReady_[i] = false;
    if (cs[i] < 0) continue;
    ++requested;
    for (uint8_t j = 0; j < i; ++j)
      if (cs[j] >= 0 && cs[j] == cs[i]) return false;
  }
  if (requested == 0) return false;

  SPI.begin(KILN_TC_SPI_SCLK, KILN_TC_SPI_MISO, KILN_TC_SPI_MOSI);
  uint8_t initialized = 0;
  for (uint8_t i = 0; i < TEMPERATURE_CHANNELS; ++i) {
    if (cs[i] < 0) continue;
    if (devices_[i]->begin()) {
      devices_[i]->setThermocoupleType(MAX31856_TCTYPE_K);
      devices_[i]->setConversionMode(MAX31856_CONTINUOUS);
      devices_[i]->setNoiseFilter(MAX31856_NOISE_FILTER_60HZ);
      channelReady_[i] = true;
      ++initialized;
    }
  }
  ready_ = initialized > 0;
  lastReadMs_ = millis();
  return ready_ && initialized == requested;
}

bool Max31856Array::configureChannel(uint8_t channel, ThermocoupleType type) {
  if (channel >= TEMPERATURE_CHANNELS || !channelReady_[channel]) return false;
  max31856_thermocoupletype_t driverType;
  switch (type) {
    case TC_K: driverType = MAX31856_TCTYPE_K; break;
    case TC_J: driverType = MAX31856_TCTYPE_J; break;
    case TC_N: driverType = MAX31856_TCTYPE_N; break;
    case TC_R: driverType = MAX31856_TCTYPE_R; break;
    case TC_S: driverType = MAX31856_TCTYPE_S; break;
    case TC_T: driverType = MAX31856_TCTYPE_T; break;
    case TC_E: driverType = MAX31856_TCTYPE_E; break;
    case TC_B: driverType = MAX31856_TCTYPE_B; break;
    default: return false;
  }
  devices_[channel]->setThermocoupleType(driverType);
  return true;
}

bool Max31856Array::channelReady(uint8_t channel) const {
  return channel < TEMPERATURE_CHANNELS && channelReady_[channel];
}

void Max31856Array::update(SensorManager& manager, uint32_t nowMs) {
  if (!ready_ || static_cast<uint32_t>(nowMs - lastReadMs_) < 250UL) return;
  lastReadMs_ = nowMs;
  for (uint8_t i = 0; i < TEMPERATURE_CHANNELS; ++i) {
    if (!channelReady_[i] || !manager.monitoringEnabled(i)) continue;
    const uint8_t fault = devices_[i]->readFault();
    if (fault != 0) {
      SensorFault mapped = SENSOR_CONVERSION;
      if (fault & MAX31856_FAULT_OPEN) mapped = SENSOR_OPEN;
      else if (fault & MAX31856_FAULT_TCRANGE) mapped = SENSOR_OUT_OF_RANGE;
      manager.submitSample(i, NAN, mapped, nowMs);
      continue;
    }
    manager.submitSample(i, devices_[i]->readThermocoupleTemperature(),
                         SENSOR_OK, nowMs);
  }
}
} // namespace kiln
