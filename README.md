# STM32U5A5-VCP_UART
 VCP UART on U5A5 NUCLEO and my board

## VCP UART via USB HS on U5A5
My project to have a USB VCP UART on NUCLEO-U5A5ZJ_Q as well as on my own PCB
(using a STM32U5A5 LQPF64 package).

It works.

## External HSE
ATTENTION: even the U5A5 datasheet mentions to use an external HSE XTAL
(not a CMOS OSC as I do) - it works. Just important to configure the clock
properly (see function void SystemClock_Config(void) - not all combinations of
PLL setting are working! )

NUCLEO board uses an external (HSE) 16 MHz XTAL oscillator, I use a 8 MHz
CMOS chip. It works with the right ClockConfig.

## Features
It uses the AZURE ThreadX as RTOS (as the NUCLEO demo project does).
I run it as MCU VDD 1V8 (to have all I/O signals as 1V8). Works fine as well.

It provides an UART command line (Shell), using USB VCP UART. But UART1 is also
possible to be used (e.g. with a CP2102N USB-to-UART dongle) - two UART ports to access the MCU.

It is mainly intended to generate QSPI transactions (to read/write to/from an
external QSPI chip, running at 1V8).

## Remarks
I have disabled some features for NUCLEO board, e.g. UCPD1, as well as to use
ADC1 for sensing VUSB (I do not have on my board).

But NUCLEO needs still to drive PA15 and PB15 (contolling the UCPD chip on NUCLEO board). For my board (not having any USB-C UCPD) - no need to do.

The macro "NUCLEO_BOARD" (defined vs. undefined) does all the changes.

The NUCLEO demo project is a bit modified in order to have USB VCP and the UART1 working independently (orignal is to criss-cross betwee both and to wait for
real UART connected and configured, my project is independent now for both).

