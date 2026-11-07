#pragma once

#include "board_profiles.h"

// CAN pins are intentionally disabled until the exact CYD board revision,
// exposed headers, and schematic have been verified. Do not guess these pins.
#ifndef KILN_CAN_TX_PIN
#define KILN_CAN_TX_PIN -1
#endif
#ifndef KILN_CAN_RX_PIN
#define KILN_CAN_RX_PIN -1
#endif
#ifndef KILN_CAN_BITRATE
#define KILN_CAN_BITRATE 500000
#endif
#ifndef KILN_MAIN_SHUTDOWN_REQUEST_PIN
#define KILN_MAIN_SHUTDOWN_REQUEST_PIN -1
#endif

#if KILN_CAN_TX_PIN >= 0 && KILN_CAN_RX_PIN >= 0
#define KILN_CAN_PINMAP_AVAILABLE 1
#else
#define KILN_CAN_PINMAP_AVAILABLE 0
#endif

static_assert(KILN_CAN_BITRATE == 500000,
              "This initial CAN transport supports 500 kbit/s pending validation.");
