#include <Arduino.h>
#include "kiln_protocol.h"

// IMPORTANT: These are commissioning defaults, not validated kiln settings.
// Tune the timeout only after measuring the complete system's worst-case latency.
#ifndef KILN_LINK_TIMEOUT_MS
#define KILN_LINK_TIMEOUT_MS 1500UL
#endif
#ifndef KILN_STATUS_INTERVAL_MS
#define KILN_STATUS_INTERVAL_MS 1000UL
#endif

// Output pins remain DISABLED until the exact Pro Micro, driver board,
// relay polarity, and independent safety design are documented.
// Set a pin to -1 to leave that channel unconfigured.
static const int8_t OUTPUT_PINS[4] = {-1, -1, -1, -1};
static const uint8_t OUTPUT_ACTIVE_LEVEL = HIGH;
static const uint8_t OUTPUT_SAFE_LEVEL = (OUTPUT_ACTIVE_LEVEL == HIGH) ? LOW : HIGH;

static kiln::Parser parser;
static uint16_t txSequence = 0;
static uint16_t lastHostSequence = 0;
static bool haveHostSequence = false;
static uint8_t outputMask = 0;
static bool communicationFault = true;
static uint32_t lastValidHostFrameMs = 0;
static uint32_t lastStatusMs = 0;

static void configureSafeOutputs() {
  outputMask = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    if (OUTPUT_PINS[i] < 0) continue;
    digitalWrite(OUTPUT_PINS[i], OUTPUT_SAFE_LEVEL);
    pinMode(OUTPUT_PINS[i], OUTPUT);
    digitalWrite(OUTPUT_PINS[i], OUTPUT_SAFE_LEVEL);
  }
}

static void forceSafeOutputs() {
  outputMask = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    if (OUTPUT_PINS[i] >= 0) digitalWrite(OUTPUT_PINS[i], OUTPUT_SAFE_LEVEL);
  }
}

static uint8_t configuredOutputMask() {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < 4; ++i)
    if (OUTPUT_PINS[i] >= 0) mask |= static_cast<uint8_t>(1U << i);
  return mask;
}

static void sendFrame(uint8_t type, uint16_t sequence,
                      const uint8_t* payload = nullptr, uint8_t length = 0) {
  kiln::Frame frame;
  frame.type = type;
  frame.sequence = sequence;
  frame.length = length;
  for (uint8_t i = 0; i < length && i < kiln::MAX_PAYLOAD; ++i)
    frame.payload[i] = payload[i];
  uint8_t bytes[kiln::FRAME_OVERHEAD + kiln::MAX_PAYLOAD];
  const size_t count = kiln::encode(frame, bytes, sizeof(bytes));
  if (count) Serial1.write(bytes, count);
}

static void sendAck(uint16_t commandSequence, uint8_t result) {
  const uint8_t payload[2] = {
    static_cast<uint8_t>(commandSequence & 0xFF),
    result
  };
  sendFrame(kiln::MSG_ACK, txSequence++, payload, sizeof(payload));
}

static void sendHello() {
  const uint8_t payload[4] = {
    kiln::PROTOCOL_VERSION,
    1,  // Firmware major version
    0,  // Firmware minor version
    configuredOutputMask()
  };
  sendFrame(kiln::MSG_HELLO, txSequence++, payload, sizeof(payload));
}

static void sendHeartbeat() {
  const uint8_t payload[3] = {
    static_cast<uint8_t>(communicationFault ? 1 : 0),
    outputMask,
    configuredOutputMask()
  };
  sendFrame(kiln::MSG_HEARTBEAT, txSequence++, payload, sizeof(payload));
}

static bool isNewerSequence(uint16_t sequence) {
  if (!haveHostSequence) return true;
  return static_cast<int16_t>(sequence - lastHostSequence) > 0;
}

static void handleFrame(const kiln::Frame& frame) {
  if (frame.version != kiln::PROTOCOL_VERSION) return;

  // Host heartbeat and status request are accepted only with a fresh sequence.
  if (!isNewerSequence(frame.sequence)) {
    sendAck(frame.sequence, kiln::RESULT_BAD_SEQUENCE);
    return;
  }

  if (frame.type == kiln::MSG_HOST_HEARTBEAT && frame.length == 0) {
    lastHostSequence = frame.sequence;
    haveHostSequence = true;
    lastValidHostFrameMs = millis();
    communicationFault = false;
    sendAck(frame.sequence, kiln::RESULT_OK);
    return;
  }

  if (frame.type == kiln::MSG_REQUEST_STATUS && frame.length == 0) {
    lastHostSequence = frame.sequence;
    haveHostSequence = true;
    lastValidHostFrameMs = millis();
    sendHeartbeat();
    sendAck(frame.sequence, kiln::RESULT_OK);
    return;
  }

  if (frame.type == kiln::MSG_SET_OUTPUTS && frame.length == 1) {
    lastHostSequence = frame.sequence;
    haveHostSequence = true;
    lastValidHostFrameMs = millis();
    communicationFault = false;

    // Never energize an output that has not been explicitly mapped/configured.
    const uint8_t requested = frame.payload[0] & 0x0F;
    if ((requested & static_cast<uint8_t>(~configuredOutputMask())) != 0) {
      forceSafeOutputs();
      sendAck(frame.sequence, kiln::RESULT_NOT_CONFIGURED);
      return;
    }

    outputMask = requested;
    for (uint8_t i = 0; i < 4; ++i) {
      if (OUTPUT_PINS[i] < 0) continue;
      const bool active = (outputMask & (1U << i)) != 0;
      digitalWrite(OUTPUT_PINS[i], active ? OUTPUT_ACTIVE_LEVEL : OUTPUT_SAFE_LEVEL);
    }
    sendAck(frame.sequence, kiln::RESULT_OK);
    return;
  }

  sendAck(frame.sequence, kiln::RESULT_BAD_LENGTH);
}

void setup() {
  configureSafeOutputs();
  Serial.begin(115200);   // USB diagnostics
  Serial1.begin(115200);  // Hardware UART link to ESP32; voltage conversion required as needed
  lastValidHostFrameMs = millis();
  sendHello();
}

void loop() {
  while (Serial1.available() > 0) {
    const uint8_t byteIn = static_cast<uint8_t>(Serial1.read());
    kiln::Frame frame;
    if (parser.feed(byteIn, frame)) handleFrame(frame);
  }

  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastValidHostFrameMs) > KILN_LINK_TIMEOUT_MS) {
    if (!communicationFault) {
      communicationFault = true;
      forceSafeOutputs();
    }
  }

  if (static_cast<uint32_t>(now - lastStatusMs) >= KILN_STATUS_INTERVAL_MS) {
    lastStatusMs = now;
    sendHeartbeat();
  }
}
