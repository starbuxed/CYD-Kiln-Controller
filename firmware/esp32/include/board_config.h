#pragma once

#include "board_profiles.h"

// Classic 2.8-inch CYD reference wiring reuses the commonly exposed UART2
// pins as TWAI signals. Verify the actual board header/schematic before wiring.
// Other variants remain disabled until their GPIO map is confirmed.
#ifndef KILN_CAN_TX_PIN
  #if KILN_BOARD_PROFILE == KILN_BOARD_CYD_28_ILI9341_XPT2046
    #define KILN_CAN_TX_PIN 27
  #else
    #define KILN_CAN_TX_PIN -1
  #endif
#endif
#ifndef KILN_CAN_RX_PIN
  #if KILN_BOARD_PROFILE == KILN_BOARD_CYD_28_ILI9341_XPT2046
    #define KILN_CAN_RX_PIN 22
  #else
    #define KILN_CAN_RX_PIN -1
  #endif
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
