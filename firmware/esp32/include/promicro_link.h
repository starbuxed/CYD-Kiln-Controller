#pragma once

#include <Arduino.h>
#include "kiln_protocol.h"

class PromicroLink {
 public:
  void begin(HardwareSerial& serial, int8_t rxPin, int8_t txPin,
             uint32_t baud = 115200);
  void update();

  bool healthy() const;
  bool helloSeen() const { return helloSeen_; }
  uint32_t heartbeatAgeMs() const;

 private:
  void sendHostHeartbeat();
  void handleFrame(const kiln::Frame& frame);

  HardwareSerial* serial_ = nullptr;
  kiln::Parser parser_;
  uint16_t txSequence_ = 0;
  uint32_t lastHeartbeatTxMs_ = 0;
  uint32_t lastHeartbeatRxMs_ = 0;
  bool helloSeen_ = false;
  bool heartbeatSeen_ = false;
};
