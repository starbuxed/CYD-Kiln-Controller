#pragma once

#include <Arduino.h>
#include "kiln_protocol.h"

class ControllerLink {
 public:
  void begin(HardwareSerial& serial, int8_t rxPin, int8_t txPin,
             uint32_t baud = 115200);
  void update();
  bool connected() const;
  uint32_t heartbeatAgeMs() const;
  bool sendCommand(uint8_t command);
  bool requestStatus();

  bool hasStatus() const { return hasStatus_; }
  uint8_t controllerState() const { return controllerState_; }
  uint8_t sensorValidMask() const { return sensorValidMask_; }
  uint8_t outputCommandMask() const { return outputCommandMask_; }
  int16_t temperatureDeciC(uint8_t channel) const;
  int16_t setpointDeciC(uint8_t channel) const;
  uint8_t lastAckCommand() const { return lastAckCommand_; }
  uint8_t lastAckResult() const { return lastAckResult_; }

 private:
  void handleFrame(const kiln::Frame& frame);
  void sendFrame(uint8_t type, const uint8_t* payload, uint8_t length);

  HardwareSerial* serial_ = nullptr;
  kiln::Parser parser_;
  uint16_t txSequence_ = 0;
  uint32_t lastHeartbeatTxMs_ = 0;
  uint32_t lastHeartbeatRxMs_ = 0;
  bool heartbeatSeen_ = false;
  bool hasStatus_ = false;
  uint8_t controllerState_ = kiln::STATE_BOOT;
  uint8_t sensorValidMask_ = 0;
  uint8_t outputCommandMask_ = 0;
  int16_t temperatures_[4] = {};
  int16_t setpoints_[4] = {};
  uint8_t lastAckCommand_ = 0;
  uint8_t lastAckResult_ = kiln::RESULT_UNSUPPORTED;
};
