
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

Raspberry Pi Broadcom Clock HW Control

Mark Broihier 2021
*/

#include "../include/Clock.h"

void Clock::initClock() {
  // See Chanpter 6.3, BCM2835 ARM peripherals for controlling the hardware clock
  // Also check https://elinux.org/BCM2835_registers#CM for the register mapping

  // kill the clock if busy
  if (clkReg->ctrl & CLK_CTL_BUSY) {
    do {
      clkReg->ctrl = BCM_PASSWD | CLK_CTL_KILL;
    } while (clkReg->ctrl & CLK_CTL_BUSY);
  }

  // Set clock source to plld
  clkReg->ctrl = BCM_PASSWD | CLK_CTL_SRC(CLK_CTL_SRC_PLLD);
  usleep(10);

  // The original clock speed is 500MHZ, we divide it by 5 to get a 100MHZ clock
  clkReg->div = BCM_PASSWD | CLK_DIV_DIVI(CLK_DIVI);
  usleep(10);

  // Enable the clock
  clkReg->ctrl |= (BCM_PASSWD | CLK_CTL_ENAB);
  fprintf(stderr, "Clock initialization complete\n");
}

Clock::Clock(Peripheral * peripheralUtil) {
  uint8_t *cmBasePtr = reinterpret_cast<uint8_t *>(peripheralUtil->mapPeripheralToUserSpace(CM_BASE, CM_LEN));
  clkReg = reinterpret_cast<CLKCtrlReg *>(cmBasePtr + CM_PWM);
  initClock();
}

Clock::~Clock() {
  fprintf(stderr, "Clock shutting down\n");
}
