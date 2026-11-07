#pragma once

#include <Arduino.h>
#include "kiln_protocol.h"

class ControllerLink {
 public:
  void begin(HardwareSerial& serial, int8_t rxPin, int8_t txPin,
             uint32_t baud = 115200);
  void update();
  void sendStatus();

 private:
  void handleFrame(const kiln::Frame& frame);
  void sendFrame(uint8_t type, uint16_t sequence,
                 const uint8_t* payload, uint8_t length);
  void sendAck(uint16_t sequence, uint8_t command, uint8_t result);
  void handleCommand(const kiln::Frame& frame);
  void applyCommand(uint8_t command, uint8_t& result);
  void sendHeartbeat();

  HardwareSerial* serial_ = nullptr;
  kiln::Parser parser_;
  uint16_t txSequence_ = 0;
  uint16_t lastCommandSequence_ = 0;
  bool commandSequenceSeen_ = false;
  uint32_t lastHostHeartbeatMs_ = 0;
  uint32_t lastHeartbeatTxMs_ = 0;
  uint32_t lastStatusTxMs_ = 0;
  bool hostHeartbeatSeen_ = false;
};
