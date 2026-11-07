#include "controller_link.h"
#include "relay_outputs.h"

extern kiln::RelayOutputs relayOutputs;
extern kiln::ControllerState controllerState;

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
  // Fixed v1 status payload (22 bytes):
  // state, fault flags u16, sensor-valid mask, output-command mask,
  // 4 temperatures in centi-degrees C, 4 setpoints in centi-degrees C.
  // Until sensor drivers are integrated, readings are marked invalid.
  uint8_t payload[22] = {};
  payload[0] = static_cast<uint8_t>(controllerState);
  payload[1] = 0; payload[2] = 0; // fault flags
  payload[3] = 0; // sensor-valid mask: no hardware sensors implemented yet
  uint8_t outputMask = 0;
  for (uint8_t i = 0; i < kiln::RELAY_OUTPUT_COUNT; ++i) {
    if (relayOutputs.commandedActive(i)) outputMask |= static_cast<uint8_t>(1U << i);
  }
  payload[4] = outputMask;
  for (uint8_t i = 0; i < 4; ++i) {
    kiln::putI16(payload + 5 + (i * 2), 0);
    kiln::putI16(payload + 13 + (i * 2), 0);
  }
  sendFrame(kiln::MSG_STATUS, txSequence_++, payload, sizeof(payload));
}

void ControllerLink::applyCommand(uint8_t command, uint8_t& result) {
  result = kiln::RESULT_INVALID_STATE;
  switch (command) {
    case kiln::CMD_START:
      // Start remains deliberately blocked until profile validation,
      // temperature acquisition, and safety interlocks are implemented.
      result = (controllerState == kiln::STATE_FAULT)
                 ? kiln::RESULT_FAULT_LATCHED : kiln::RESULT_NOT_READY;
      break;

    case kiln::CMD_PAUSE:
      if (controllerState == kiln::STATE_FIRING) {
        relayOutputs.allInactive();
        controllerState = kiln::STATE_PAUSED;
        result = kiln::RESULT_OK;
      }
      break;

    case kiln::CMD_RESUME:
      result = (controllerState == kiln::STATE_FAULT)
                 ? kiln::RESULT_FAULT_LATCHED : kiln::RESULT_NOT_READY;
      break;

    case kiln::CMD_STOP:
      relayOutputs.allInactive();
      if (controllerState != kiln::STATE_FAULT) controllerState = kiln::STATE_IDLE;
      result = kiln::RESULT_OK;
      break;

    case kiln::CMD_ACK_FAULT:
      // Fault clearing requires explicit recovery checks; not implemented yet.
      result = (controllerState == kiln::STATE_FAULT)
                 ? kiln::RESULT_NOT_READY : kiln::RESULT_INVALID_STATE;
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
      controllerState == kiln::STATE_FIRING) {
    relayOutputs.allInactive();
    controllerState = kiln::STATE_FAULT;
  }
}
