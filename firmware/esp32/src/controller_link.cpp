#include "controller_link.h"

static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 500;
static constexpr uint32_t LINK_TIMEOUT_MS = 2500;

bool ControllerLink::begin(kiln::CanStream& stream, int8_t txPin, int8_t rxPin,
                           uint32_t bitrate) {
  stream_ = &stream;
  if (!stream_->begin(txPin, rxPin, bitrate)) return false;
  const uint32_t now = millis();
  lastHeartbeatTxMs_ = now;
  lastHeartbeatRxMs_ = 0;
  heartbeatSeen_ = false;
  hasStatus_ = false;
  const uint8_t hello[1] = {kiln::PROTOCOL_VERSION};
  sendFrame(kiln::MSG_HELLO, hello, sizeof(hello));
  return true;
}

void ControllerLink::sendFrame(uint8_t type, const uint8_t* payload,
                               uint8_t length) {
  if (!stream_ || length > kiln::MAX_PAYLOAD) return;
  kiln::Frame frame;
  frame.type = type;
  frame.sequence = txSequence_++;
  frame.length = length;
  for (uint8_t i = 0; i < length; ++i) frame.payload[i] = payload[i];
  uint8_t bytes[kiln::MAX_FRAME_SIZE];
  const size_t count = kiln::encode(frame, bytes, sizeof(bytes));
  if (count) stream_->write(bytes, count);
}

bool ControllerLink::sendCommand(uint8_t command) {
  if (!connected()) return false;
  sendFrame(kiln::MSG_COMMAND, &command, 1);
  return true;
}

bool ControllerLink::requestStatus() {
  if (!stream_ || !stream_->started()) return false;
  sendFrame(kiln::MSG_GET_STATUS, nullptr, 0);
  return true;
}

bool ControllerLink::beginProfileConfig(uint8_t count) {
  if (!connected() || count == 0 || count > kiln::MAX_PAYLOAD) return false;
  sendFrame(kiln::MSG_PROFILE_BEGIN, &count, 1);
  return true;
}
bool ControllerLink::sendProfileSegment(uint8_t index, int16_t targetDeciC,
                                        int16_t rampDeciCPerHour,
                                        uint32_t holdSeconds) {
  if (!connected()) return false;
  uint8_t payload[9];
  payload[0] = index;
  kiln::putI16(payload + 1, targetDeciC);
  kiln::putI16(payload + 3, rampDeciCPerHour);
  kiln::putU32(payload + 5, holdSeconds);
  sendFrame(kiln::MSG_PROFILE_SEGMENT, payload, sizeof(payload));
  return true;
}
bool ControllerLink::commitProfileConfig() {
  if (!connected()) return false;
  sendFrame(kiln::MSG_PROFILE_COMMIT, nullptr, 0);
  return true;
}
bool ControllerLink::configureZone(uint8_t zone, int16_t hysteresisDeciC,
                                   uint32_t minimumOnMs, uint32_t minimumOffMs,
                                   int16_t offsetDeciC) {
  if (!connected() || zone >= 4) return false;
  uint8_t payload[13];
  payload[0] = zone;
  kiln::putI16(payload + 1, hysteresisDeciC);
  kiln::putU32(payload + 3, minimumOnMs);
  kiln::putU32(payload + 7, minimumOffMs);
  kiln::putI16(payload + 11, offsetDeciC);
  sendFrame(kiln::MSG_ZONE_CONFIG, payload, sizeof(payload));
  return true;
}
bool ControllerLink::configureSensor(uint8_t channel, uint8_t type,
                                     int16_t offsetDeciC) {
  if (!connected() || channel >= 4 || type > 7) return false;
  uint8_t payload[4];
  payload[0] = channel;
  payload[1] = type;
  kiln::putI16(payload + 2, offsetDeciC);
  sendFrame(kiln::MSG_SENSOR_CONFIG, payload, sizeof(payload));
  return true;
}
bool ControllerLink::configureSensorMode(uint8_t channel, uint8_t mode) {
  if (!connected() || channel >= 4 || mode > 3) return false;
  const uint8_t payload[2] = {channel, mode};
  sendFrame(kiln::MSG_SENSOR_MODE_CONFIG, payload, sizeof(payload));
  return true;
}
bool ControllerLink::configureSafetyLimit(int16_t maximumDeciC) {
  if (!connected()) return false;
  uint8_t payload[2];
  kiln::putI16(payload, maximumDeciC);
  sendFrame(kiln::MSG_SAFETY_CONFIG, payload, sizeof(payload));
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
    // four temperatures and four setpoints, all signed deci-degrees.
    if (frame.length != 29) return;
    controllerState_ = frame.payload[0];
    faultFlags_ = static_cast<uint16_t>(frame.payload[1]) |
                  (static_cast<uint16_t>(frame.payload[2]) << 8);
    sensorValidMask_ = frame.payload[3];
    outputCommandMask_ = frame.payload[4];
    for (uint8_t i = 0; i < 4; ++i) {
      temperatures_[i] = kiln::getI16(frame.payload + 5 + (i * 2));
      setpoints_[i] = kiln::getI16(frame.payload + 13 + (i * 2));
      sensorFaults_[i] = frame.payload[21 + i];
      sensorModes_[i] = frame.payload[25 + i];
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
  if (!stream_) return;
  while (stream_->available() > 0) {
    const uint8_t incoming = static_cast<uint8_t>(stream_->read());
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
  return stream_ && stream_->healthy() && heartbeatSeen_ &&
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
