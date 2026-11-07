#include "controller_link.h"

static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 500;
static constexpr uint32_t LINK_TIMEOUT_MS = 2500;

void ControllerLink::begin(HardwareSerial& serial, int8_t rxPin, int8_t txPin,
                           uint32_t baud) {
  serial_ = &serial;
  serial_->begin(baud, SERIAL_8N1, rxPin, txPin);
  const uint32_t now = millis();
  lastHeartbeatTxMs_ = now;
  lastHeartbeatRxMs_ = 0;
  heartbeatSeen_ = false;
  hasStatus_ = false;
  const uint8_t hello[1] = {kiln::PROTOCOL_VERSION};
  sendFrame(kiln::MSG_HELLO, hello, sizeof(hello));
}

void ControllerLink::sendFrame(uint8_t type, const uint8_t* payload,
                               uint8_t length) {
  if (!serial_ || length > kiln::MAX_PAYLOAD) return;
  kiln::Frame frame;
  frame.type = type;
  frame.sequence = txSequence_++;
  frame.length = length;
  for (uint8_t i = 0; i < length; ++i) frame.payload[i] = payload[i];
  uint8_t bytes[kiln::MAX_FRAME_SIZE];
  const size_t count = kiln::encode(frame, bytes, sizeof(bytes));
  if (count) serial_->write(bytes, count);
}

bool ControllerLink::sendCommand(uint8_t command) {
  if (!connected()) return false;
  sendFrame(kiln::MSG_COMMAND, &command, 1);
  return true;
}

bool ControllerLink::requestStatus() {
  if (!serial_) return false;
  sendFrame(kiln::MSG_GET_STATUS, nullptr, 0);
  return true;
}

void ControllerLink::handleFrame(const kiln::Frame& frame) {
  if (frame.version != kiln::PROTOCOL_VERSION) return;
  if (frame.type == kiln::MSG_HEARTBEAT) {
    if (frame.length != 5) return;
    heartbeatSeen_ = true;
    lastHeartbeatRxMs_ = millis();
    controllerState_ = frame.payload[4];
    return;
  }
  if (frame.type == kiln::MSG_STATUS) {
    // S3 status payload: state, fault flags(2), sensor mask, output mask,
    // four temperatures and four setpoints, all signed centi-degrees.
    if (frame.length != 21) return;
    controllerState_ = frame.payload[0];
    sensorValidMask_ = frame.payload[3];
    outputCommandMask_ = frame.payload[4];
    for (uint8_t i = 0; i < 4; ++i) {
      temperatures_[i] = kiln::getI16(frame.payload + 5 + (i * 2));
      setpoints_[i] = kiln::getI16(frame.payload + 13 + (i * 2));
    }
    hasStatus_ = true;
    return;
  }
  if (frame.type == kiln::MSG_ACK && frame.length == 2) {
    lastAckCommand_ = frame.payload[0];
    lastAckResult_ = frame.payload[1];
  }
}

void ControllerLink::update() {
  if (!serial_) return;
  while (serial_->available() > 0) {
    const uint8_t incoming = static_cast<uint8_t>(serial_->read());
    kiln::Frame frame;
    if (parser_.feed(incoming, frame)) handleFrame(frame);
  }
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastHeartbeatTxMs_) >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatTxMs_ = now;
    uint8_t payload[4];
    kiln::putU32(payload, now);
    sendFrame(kiln::MSG_HEARTBEAT, payload, sizeof(payload));
  }
}

bool ControllerLink::connected() const {
  return heartbeatSeen_ &&
         static_cast<uint32_t>(millis() - lastHeartbeatRxMs_) <= LINK_TIMEOUT_MS;
}
uint32_t ControllerLink::heartbeatAgeMs() const {
  if (!heartbeatSeen_) return UINT32_MAX;
  return static_cast<uint32_t>(millis() - lastHeartbeatRxMs_);
}
int16_t ControllerLink::temperatureDeciC(uint8_t channel) const {
  return channel < 4 ? temperatures_[channel] : 0;
}
int16_t ControllerLink::setpointDeciC(uint8_t channel) const {
  return channel < 4 ? setpoints_[channel] : 0;
}
