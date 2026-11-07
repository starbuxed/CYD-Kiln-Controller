#pragma once

#include <Arduino.h>
#include <driver/twai.h>
#include <esp_err.h>
#include <string.h>
#include "kiln_protocol.h"
#include "can_wire.h"

namespace kiln {

enum CanNodeRole : uint8_t { CAN_ROLE_MAIN = 0, CAN_ROLE_SECONDARY = 1 };

// A byte-stream adapter over classic CAN. Existing protocol parser/client code
// can consume this class using available/read/write. One protocol frame is
// segmented into ordered CAN fragments; the protocol CRC remains end-to-end.
class CanStream {
 public:
  explicit CanStream(CanNodeRole role) : role_(role) {}

  bool begin(int8_t txPin, int8_t rxPin, uint32_t bitrate = 500000) {
    if (started_ || txPin < 0 || rxPin < 0 || txPin == rxPin ||
        bitrate != 500000) return false;

    twai_general_config_t general =
        TWAI_GENERAL_CONFIG_DEFAULT(static_cast<gpio_num_t>(txPin),
                                    static_cast<gpio_num_t>(rxPin),
                                    TWAI_MODE_NORMAL);
    general.tx_queue_len = 12;
    general.rx_queue_len = 24;
    twai_timing_config_t timing = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&general, &timing, &filter) != ESP_OK) return false;
    if (twai_start() != ESP_OK) {
      twai_driver_uninstall();
      return false;
    }
    started_ = true;
    return true;
  }

  size_t write(const uint8_t* bytes, size_t length) {
    if (!started_ || bytes == nullptr || length < FRAME_OVERHEAD ||
        length > canwire::MAX_REASSEMBLED_BYTES ||
        bytes[0] != SOF_0 || bytes[1] != SOF_1 ||
        bytes[2] != PROTOCOL_VERSION) return 0;

    const uint16_t canId = identifierFor(bytes[3]);
    if (canId == 0) return 0;
    const uint8_t fragmentCount = static_cast<uint8_t>(
        (length + canwire::FRAGMENT_DATA_BYTES - 1) /
        canwire::FRAGMENT_DATA_BYTES);

    for (uint8_t i = 0; i < fragmentCount; ++i) {
      canwire::Fragment fragment;
      if (!canwire::makeFragment(bytes, length, i, fragment)) return 0;
      twai_message_t message = {};
      message.identifier = canId;
      message.extd = 0;
      message.rtr = 0;
      message.data_length_code = fragment.dlc;
      memcpy(message.data, fragment.bytes, fragment.dlc);
      if (twai_transmit(&message, pdMS_TO_TICKS(25)) != ESP_OK) return 0;
    }
    return length;
  }

  int available() {
    pollReceive();
    return static_cast<int>(rxCount_);
  }

  int read() {
    pollReceive();
    if (rxCount_ == 0) return -1;
    const uint8_t value = rxQueue_[rxTail_];
    rxTail_ = static_cast<uint16_t>((rxTail_ + 1) % RX_QUEUE_SIZE);
    --rxCount_;
    return value;
  }

  bool healthy() const {
    if (!started_) return false;
    twai_status_info_t status = {};
    if (twai_get_status_info(&status) != ESP_OK) return false;
    return status.state == TWAI_STATE_RUNNING;
  }

  bool started() const { return started_; }

 private:
  static constexpr uint16_t RX_QUEUE_SIZE = 256;

  uint16_t identifierFor(uint8_t type) const {
    if (role_ == CAN_ROLE_MAIN) {
      switch (type) {
        case MSG_HELLO: return 0x060;
        case MSG_HEARTBEAT: return 0x090;
        case MSG_COMMAND: return 0x100;
        case MSG_GET_STATUS: return 0x130;
        case MSG_PROFILE_BEGIN: return 0x180;
        case MSG_PROFILE_SEGMENT: return 0x181;
        case MSG_PROFILE_COMMIT: return 0x182;
        case MSG_ZONE_CONFIG: return 0x183;
        case MSG_SENSOR_CONFIG: return 0x184;
        case MSG_SAFETY_CONFIG: return 0x185;
        case MSG_SENSOR_MODE_CONFIG: return 0x186;
        default: return 0x1F0;
      }
    }
    switch (type) {
      case MSG_HEARTBEAT: return 0x091;
      case MSG_STATUS: return 0x080;
      case MSG_FAULT: return 0x081;
      case MSG_ACK: return 0x101;
      default: return 0x0A0;
    }
  }

  void pollReceive() {
    if (!started_) return;
    twai_message_t message = {};
    while (twai_receive(&message, 0) == ESP_OK) {
      if (message.extd || message.rtr || message.data_length_code < 2 ||
          message.data_length_code > canwire::MAX_CAN_DLC) continue;

      uint8_t completed[canwire::MAX_REASSEMBLED_BYTES] = {};
      size_t completedLength = 0;
      if (reassembler_.accept(static_cast<uint16_t>(message.identifier),
                              message.data, message.data_length_code,
                              millis(), completed, sizeof(completed),
                              completedLength)) {
        if (completedLength > (RX_QUEUE_SIZE - rxCount_)) continue;
        for (size_t i = 0; i < completedLength; ++i) {
          rxQueue_[rxHead_] = completed[i];
          rxHead_ = static_cast<uint16_t>((rxHead_ + 1) % RX_QUEUE_SIZE);
          ++rxCount_;
        }
      }
    }
  }

  CanNodeRole role_;
  bool started_ = false;
  canwire::Reassembler reassembler_;
  uint8_t rxQueue_[RX_QUEUE_SIZE] = {};
  uint16_t rxHead_ = 0;
  uint16_t rxTail_ = 0;
  uint16_t rxCount_ = 0;
};

} // namespace kiln
