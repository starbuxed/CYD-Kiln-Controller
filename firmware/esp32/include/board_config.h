#pragma once

#include "board_profiles.h"

#ifndef KILN_UART_BAUD
#define KILN_UART_BAUD 115200
#endif

// Board profile supplies these pins for the classic 2.8-inch CYD.
// Custom/other boards must supply an explicit, verified UART mapping.
#if CYD_BOARD_PROFILE == CYD_PROFILE_CUSTOM
#ifndef KILN_UART_RX_PIN
#error "Set KILN_UART_RX_PIN for your custom CYD board."
#endif
#ifndef KILN_UART_TX_PIN
#error "Set KILN_UART_TX_PIN for your custom CYD board."
#endif
#else
#define KILN_UART_RX_PIN CYD_BOARD.uart_rx
#define KILN_UART_TX_PIN CYD_BOARD.uart_tx
#endif

static_assert(KILN_UART_BAUD >= 9600 && KILN_UART_BAUD <= 1000000,
              "Unreasonable UART baud rate.");
