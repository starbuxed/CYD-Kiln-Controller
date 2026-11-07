#include "controller_link.h"
#include "relay_outputs.h"
#include "kiln_control_core.h"
#include "sensor_manager.h"
#include "max31856_array.h"

extern kiln::RelayOutputs relayOutputs;
extern kiln::ControllerState controllerState;
extern kiln::KilnControlCore controlCore;
extern kiln::SensorManager sensorManager;
extern kiln::Max31856Array sensorFrontend;
extern float kilnTemperaturesC[4];

static constexpr uint32_t HOST_TIMEOUT_MS = 2500;
static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 500;
static constexpr uint32_t STATUS_INTERVAL_MS = 500;

void ControllerLink::begin(HardwareSerial& serial, int8_t rxPin, int8_t txPin,
                           uint32_t baud) {
  serial_ = &serial;
  serial_->begin(baud, SERIAL_8N1, rxPin, txPin);
  const uint32_t now = millis();
  lastHostHeartbeatMs_ = now;
  lastHeartbeatTxMs_ = now;
  lastStatusTxMs_ = now;
  hostHeartbeatSeen_ = false;
  commandSequenceSeen_ = false;
}

void ControllerLink::sendFrame(uint8_t type, uint16_t sequence,
                               const uint8_t* payload, uint8_t length) {
  if (!serial_ || length > kiln::MAX_PAYLOAD) return;
  kiln::Frame frame;
  frame.type = type;
  frame.sequence = sequence;
  frame.length = length;
  for (uint8_t i = 0; i < length; ++i) frame.payload[i] = payload[i];
  uint8_t bytes[kiln::MAX_FRAME_SIZE];
  const size_t count = kiln::encode(frame, bytes, sizeof(bytes));
  if (count) serial_->write(bytes, count);
}

void ControllerLink::sendAck(uint16_t sequence, uint8_t command, uint8_t result) {
  const uint8_t payload[2] = {command, result};
  sendFrame(kiln::MSG_ACK, sequence, payload, sizeof(payload));
}

void ControllerLink::sendHeartbeat() {
  uint8_t payload[5] = {0, 0, 0, 0, static_cast<uint8_t>(controllerState)};
  kiln::putU32(payload, millis());
  payload[4] = static_cast<uint8_t>(controllerState);
  sendFrame(kiln::MSG_HEARTBEAT, txSequence_++, payload, sizeof(payload));
}

void ControllerLink::sendStatus() {
  // Fixed v1 status payload (21 bytes):
  // state, fault flags u16, sensor-valid mask, output-command mask,
  // 4 temperatures in deci-degrees C, 4 setpoints in deci-degrees C.
  // Until sensor drivers are integrated, readings are marked invalid.
  uint8_t payload[21] = {};
  payload[0] = static_cast<uint8_t>(controllerState);
  const uint16_t faults = controlCore.faults();
  payload[1] = static_cast<uint8_t>(faults & 0xFF);
  payload[2] = static_cast<uint8_t>(faults >> 8);
  payload[3] = sensorManager.validMask();
  uint8_t outputMask = 0;
  for (uint8_t i = 0; i < kiln::RELAY_OUTPUT_COUNT; ++i) {
    if (relayOutputs.commandedActive(i)) outputMask |= static_cast<uint8_t>(1U << i);
  }
  payload[4] = outputMask;
  for (uint8_t i = 0; i < 4; ++i) {
    const float safeTemp = sensorManager.valid(i) ? kilnTemperaturesC[i] : 0.0f;
    const int16_t temp = static_cast<int16_t>(safeTemp * 10.0f);
    const int16_t setpoint = static_cast<int16_t>(controlCore.setpointC(i) * 10.0f);
    kiln::putI16(payload + 5 + (i * 2), temp);
    kiln::putI16(payload + 13 + (i * 2), setpoint);
  }
  sendFrame(kiln::MSG_STATUS, txSequence_++, payload, sizeof(payload));
}

void ControllerLink::applyCommand(uint8_t command, uint8_t& result) {
  result = kiln::RESULT_INVALID_STATE;
  switch (command) {
    case kiln::CMD_START:
      if (controllerState == kiln::STATE_FAULT) {
        result = kiln::RESULT_FAULT_LATCHED;
      } else {
        bool outputsReady = true;
        for (uint8_t i = 0; i < kiln::RELAY_OUTPUT_COUNT; ++i)
          if (!relayOutputs.isConfigured(i)) outputsReady = false;
        if (controllerState == kiln::STATE_IDLE && outputsReady &&
            controlCore.start(millis(), kilnTemperaturesC,
                              sensorManager.validMask(), hostLinkHealthy())) {
          controllerState = kiln::STATE_FIRING;
          result = kiln::RESULT_OK;
        } else {
          result = kiln::RESULT_NOT_READY;
        }
      }
      break;

    case kiln::CMD_PAUSE:
      if (controllerState == kiln::STATE_FIRING) {
        if (controlCore.pause(millis())) {
          relayOutputs.allInactive();
          controllerState = kiln::STATE_PAUSED;
          result = kiln::RESULT_OK;
        } else {
          result = kiln::RESULT_NOT_READY;
        }
      }
      break;

    case kiln::CMD_RESUME:
      if (controllerState == kiln::STATE_FAULT) {
        result = kiln::RESULT_FAULT_LATCHED;
      } else if (controllerState == kiln::STATE_PAUSED &&
                 controlCore.resume(millis(), kilnTemperaturesC,
                                    sensorManager.validMask(), hostLinkHealthy())) {
        controllerState = kiln::STATE_FIRING;
        result = kiln::RESULT_OK;
      } else {
        result = kiln::RESULT_NOT_READY;
      }
      break;

    case kiln::CMD_STOP:
      controlCore.stop();
      relayOutputs.allInactive();
      if (controllerState != kiln::STATE_FAULT) controllerState = kiln::STATE_IDLE;
      result = kiln::RESULT_OK;
      break;

    case kiln::CMD_ACK_FAULT:
      if (controllerState == kiln::STATE_FAULT) {
        result = controlCore.acknowledgeFault(kilnTemperaturesC,
                    sensorManager.validMask(), hostLinkHealthy())
                   ? kiln::RESULT_OK : kiln::RESULT_NOT_READY;
        if (result == kiln::RESULT_OK) controllerState = kiln::STATE_IDLE;
      } else {
        result = kiln::RESULT_INVALID_STATE;
      }
      break;

    default:
      result = kiln::RESULT_UNSUPPORTED;
      break;
  }
}

void ControllerLink::handleCommand(const kiln::Frame& frame) {
  if (frame.length != 1) {
    sendAck(frame.sequence, 0, kiln::RESULT_BAD_LENGTH);
    return;
  }
  if (commandSequenceSeen_ &&
      !kiln::sequenceIsNewer(frame.sequence, lastCommandSequence_)) {
    sendAck(frame.sequence, frame.payload[0], kiln::RESULT_BAD_SEQUENCE);
    return;
  }
  commandSequenceSeen_ = true;
  lastCommandSequence_ = frame.sequence;

  uint8_t result = kiln::RESULT_UNSUPPORTED;
  applyCommand(frame.payload[0], result);
  sendAck(frame.sequence, frame.payload[0], result);
}

bool ControllerLink::acceptHostSequence(const kiln::Frame& frame) {
  if (commandSequenceSeen_ &&
      !kiln::sequenceIsNewer(frame.sequence, lastCommandSequence_)) return false;
  commandSequenceSeen_ = true;
  lastCommandSequence_ = frame.sequence;
  return true;
}

void ControllerLink::handleConfiguration(const kiln::Frame& frame) {
  if (controllerState != kiln::STATE_IDLE) {
    sendAck(frame.sequence, frame.type, kiln::RESULT_INVALID_STATE);
    return;
  }
  if (!acceptHostSequence(frame)) {
    sendAck(frame.sequence, frame.type, kiln::RESULT_BAD_SEQUENCE);
    return;
  }
  uint8_t result = kiln::RESULT_BAD_LENGTH;
  switch (frame.type) {
    case kiln::MSG_PROFILE_BEGIN:
      if (frame.length == 1 && frame.payload[0] > 0 &&
          frame.payload[0] <= kiln::MAX_PROFILE_SEGMENTS) {
        stagedProfileCount_ = frame.payload[0];
        stagedProfileMask_ = 0;
        profileStaging_ = true;
        result = kiln::RESULT_OK;
      }
      break;
    case kiln::MSG_PROFILE_SEGMENT:
      if (frame.length == 9 && profileStaging_) {
        const uint8_t index = frame.payload[0];
        if (index < stagedProfileCount_) {
          stagedProfile_[index].targetC = kiln::getI16(frame.payload + 1) / 10.0f;
          stagedProfile_[index].rampCPerHour = kiln::getI16(frame.payload + 3) / 10.0f;
          stagedProfile_[index].holdSeconds = kiln::getU32(frame.payload + 5);
          stagedProfileMask_ |= (1UL << index);
          result = kiln::RESULT_OK;
        } else result = kiln::RESULT_BAD_LENGTH;
      }
      break;
    case kiln::MSG_PROFILE_COMMIT: {
      const uint32_t required = stagedProfileCount_ == 32 ? 0xFFFFFFFFUL :
                                ((1UL << stagedProfileCount_) - 1UL);
      if (frame.length == 0 && profileStaging_ &&
          stagedProfileMask_ == required &&
          controlCore.configureProfile(stagedProfile_, stagedProfileCount_)) {
        profileStaging_ = false;
        result = kiln::RESULT_OK;
      } else if (frame.length == 0) result = kiln::RESULT_NOT_READY;
      break;
    }
    case kiln::MSG_ZONE_CONFIG:
      if (frame.length == 13 && frame.payload[0] < 4) {
        kiln::ZoneRuntimeConfig cfg;
        cfg.control.hysteresisC = kiln::getI16(frame.payload + 1) / 10.0f;
        cfg.control.minimumOnMs = kiln::getU32(frame.payload + 3);
        cfg.control.minimumOffMs = kiln::getU32(frame.payload + 7);
        cfg.setpointOffsetC = kiln::getI16(frame.payload + 11) / 10.0f;
        result = controlCore.configureZone(frame.payload[0], cfg)
                   ? kiln::RESULT_OK : kiln::RESULT_NOT_READY;
      }
      break;
    case kiln::MSG_SENSOR_CONFIG:
      if (frame.length == 4 && frame.payload[0] < 4 && frame.payload[1] <= kiln::TC_B) {
        const float offset = kiln::getI16(frame.payload + 2) / 10.0f;
        const kiln::ThermocoupleType type =
            static_cast<kiln::ThermocoupleType>(frame.payload[1]);
        if (sensorManager.configure(frame.payload[0], type, offset)) {
          result = (!sensorFrontend.ready() ||
                    sensorFrontend.configureChannel(frame.payload[0], type))
                     ? kiln::RESULT_OK : kiln::RESULT_NOT_READY;
        } else {
          result = kiln::RESULT_BAD_LENGTH;
        }
      }
      break;
    case kiln::MSG_SAFETY_CONFIG:
      if (frame.length == 2) {
        const float maximum = kiln::getI16(frame.payload) / 10.0f;
        result = controlCore.setMaximumAllowedTemperatureC(maximum)
                   ? kiln::RESULT_OK : kiln::RESULT_NOT_READY;
      }
      break;
    default:
      result = kiln::RESULT_UNSUPPORTED;
      break;
  }
  sendAck(frame.sequence, frame.type, result);
}

void ControllerLink::handleFrame(const kiln::Frame& frame) {
  if (frame.version != kiln::PROTOCOL_VERSION) return;

  switch (frame.type) {
    case kiln::MSG_HELLO:
      sendHeartbeat();
      sendStatus();
      break;
    case kiln::MSG_HEARTBEAT:
      if (frame.length != 4) return;
      hostHeartbeatSeen_ = true;
      lastHostHeartbeatMs_ = millis();
      break;
    case kiln::MSG_GET_STATUS:
      if (frame.length == 0) sendStatus();
      break;
    case kiln::MSG_COMMAND:
      handleCommand(frame);
      break;
    case kiln::MSG_PROFILE_BEGIN:
    case kiln::MSG_PROFILE_SEGMENT:
    case kiln::MSG_PROFILE_COMMIT:
    case kiln::MSG_ZONE_CONFIG:
    case kiln::MSG_SENSOR_CONFIG:
    case kiln::MSG_SAFETY_CONFIG:
      handleConfiguration(frame);
      break;
    default:
      break;
  }
}

void ControllerLink::update() {
  if (!serial_) return;
  while (serial_->available() > 0) {
    const uint8_t byteIn = static_cast<uint8_t>(serial_->read());
    kiln::Frame frame;
    if (parser_.feed(byteIn, frame)) handleFrame(frame);
  }

  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastHeartbeatTxMs_) >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatTxMs_ = now;
    sendHeartbeat();
  }
  if (static_cast<uint32_t>(now - lastStatusTxMs_) >= STATUS_INTERVAL_MS) {
    lastStatusTxMs_ = now;
    sendStatus();
  }

  if (hostHeartbeatSeen_ &&
      static_cast<uint32_t>(now - lastHostHeartbeatMs_) > HOST_TIMEOUT_MS &&
      (controllerState == kiln::STATE_FIRING ||
       controllerState == kiln::STATE_PAUSED)) {
    relayOutputs.allInactive();
    controlCore.latchFault(kiln::FAULT_HOST_LINK);
    controllerState = kiln::STATE_FAULT;
  }
}

bool ControllerLink::hostLinkHealthy() const {
  return hostHeartbeatSeen_ &&
         static_cast<uint32_t>(millis() - lastHostHeartbeatMs_) <= HOST_TIMEOUT_MS;
}
