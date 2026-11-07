#pragma once

#include "board_profiles.h"

#ifndef KILN_UART_BAUD
#define KILN_UART_BAUD 115200
#endif

// UART2 pins are only preselected for the classic 2.8-inch Sunton layout.
// These are broken-out GPIOs, not a claim that every clone uses the same header.
#if KILN_BOARD_PROFILE == KILN_BOARD_CYD_28_ILI9341_XPT2046 || \
    KILN_BOARD_PROFILE == KILN_BOARD_CYD_28_ST7789_XPT2046
  #define KILN_UART_RX_PIN Cyd28ClassicPins::UART_RX
  #define KILN_UART_TX_PIN Cyd28ClassicPins::UART_TX
  #define KILN_UART_PINMAP_AVAILABLE 1
#elif KILN_BOARD_PROFILE == KILN_BOARD_CUSTOM
  #ifndef KILN_UART_RX_PIN
    #error "Define KILN_UART_RX_PIN for your custom board."
  #endif
  #ifndef KILN_UART_TX_PIN
    #error "Define KILN_UART_TX_PIN for your custom board."
  #endif
  #define KILN_UART_PINMAP_AVAILABLE 1
#else
  #define KILN_UART_RX_PIN -1
  #define KILN_UART_TX_PIN -1
  #define KILN_UART_PINMAP_AVAILABLE 0
#endif

static_assert(KILN_UART_BAUD >= 9600 && KILN_UART_BAUD <= 1000000,
              "Unreasonable UART baud rate.");
