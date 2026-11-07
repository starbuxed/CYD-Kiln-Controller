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
// GPIO16 is the common CYD onboard RGB green LED channel (active-low). It is
// not on the normal expansion header; enabling this option requires a soldered
// tap at the verified GPIO16/LED resistor pad and a high-impedance driver input.
// Do not enable on a board revision that uses GPIO16 for another function.
#ifndef KILN_CYD_USE_GPIO16_GREEN_LED_FOR_MASTER_PERMIT
#define KILN_CYD_USE_GPIO16_GREEN_LED_FOR_MASTER_PERMIT 0
#endif
#ifndef KILN_MAIN_POWER_PERMIT_PIN
  #if KILN_BOARD_PROFILE == KILN_BOARD_CYD_28_ILI9341_XPT2046 && \
      KILN_CYD_USE_GPIO16_GREEN_LED_FOR_MASTER_PERMIT
    #define KILN_MAIN_POWER_PERMIT_PIN 16
    #define KILN_MAIN_POWER_PERMIT_ACTIVE_HIGH 0
  #else
    #define KILN_MAIN_POWER_PERMIT_PIN -1
    #define KILN_MAIN_POWER_PERMIT_ACTIVE_HIGH 1
  #endif
#endif
#ifndef KILN_MAIN_POWER_PERMIT_ACTIVE_HIGH
#define KILN_MAIN_POWER_PERMIT_ACTIVE_HIGH 1
#endif

#if KILN_CAN_TX_PIN >= 0 && KILN_CAN_RX_PIN >= 0
#define KILN_CAN_PINMAP_AVAILABLE 1
#else
#define KILN_CAN_PINMAP_AVAILABLE 0
#endif

static_assert(KILN_CAN_BITRATE == 500000,
              "This initial CAN transport supports 500 kbit/s pending validation.");
