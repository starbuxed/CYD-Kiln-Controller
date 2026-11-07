#include "promicro_link.h"

static constexpr uint32_t HOST_HEARTBEAT_INTERVAL_MS = 250;
static constexpr uint32_t LINK_TIMEOUT_MS = 2500;

void PromicroLink::begin(HardwareSerial& serial, int8_t rxPin, int8_t txPin,
                         uint32_t baud) {
  serial_ = &serial;
  serial_->begin(baud, SERIAL_8N1, rxPin, txPin);
  lastHeartbeatTxMs_ = millis();
  lastHeartbeatRxMs_ = 0;
  helloSeen_ = false;
  heartbeatSeen_ = false;
}

void PromicroLink::sendHostHeartbeat() {
  if (!serial_) return;

  kiln::Frame frame;
  frame.type = kiln::MSG_HOST_HEARTBEAT;
  frame.sequence = txSequence_++;
  frame.length = 0;

  uint8_t bytes[kiln::FRAME_OVERHEAD + kiln::MAX_PAYLOAD];
  const size_t count = kiln::encode(frame, bytes, sizeof(bytes));
  if (count) serial_->write(bytes, count);
}

void PromicroLink::handleFrame(const kiln::Frame& frame) {
  if (frame.version != kiln::PROTOCOL_VERSION) return;

  if (frame.type == kiln::MSG_HELLO) {
    helloSeen_ = true;
    return;
  }

  if (frame.type == kiln::MSG_HEARTBEAT) {
    heartbeatSeen_ = true;
    lastHeartbeatRxMs_ = millis();
    return;
  }

  // ACKs are useful for diagnostics but do not replace the periodic
  // Pro Micro heartbeat as proof that the I/O processor remains alive.
}

void PromicroLink::update() {
  if (!serial_) return;

  while (serial_->available() > 0) {
    const uint8_t incoming = static_cast<uint8_t>(serial_->read());
    kiln::Frame frame;
    if (parser_.feed(incoming, frame)) handleFrame(frame);
  }

  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastHeartbeatTxMs_) >=
      HOST_HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatTxMs_ = now;
    sendHostHeartbeat();
  }
}

bool PromicroLink::healthy() const {
  return heartbeatSeen_ &&
         static_cast<uint32_t>(millis() - lastHeartbeatRxMs_) <= LINK_TIMEOUT_MS;
}

uint32_t PromicroLink::heartbeatAgeMs() const {
  if (!heartbeatSeen_) return UINT32_MAX;
  return static_cast<uint32_t>(millis() - lastHeartbeatRxMs_);
}
