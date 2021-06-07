#ifndef INCLUDE_CLOCK_H_
#define INCLUDE_CLOCK_H_
/*
 * Class for creating an object that controls the Broadcom clock
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 *
 * Mark Broihier
 *
 */
#include <stdint.h>
#include <unistd.h>
#include "../include/Peripheral.h"

#define CM_BASE 0x00101000
#define CM_LEN 0xA8
#define CM_PWM 0xA0
#define CLK_CTL_BUSY (1 << 7)
#define CLK_CTL_KILL (1 << 5)
#define CLK_CTL_ENAB (1 << 4)
#define CLK_CTL_SRC(x) ((x) << 0)

#define CLK_CTL_SRC_PLLD 6

#define CLK_DIVI 5
#define CLK_DIV_DIVI(x) ((x) << 12)

#define BCM_PASSWD (0x5A << 24)


class Clock {
 private:
  typedef struct CLKCtrlReg {
    // See https://elinux.org/BCM2835_registers#CM
    uint32_t ctrl;
    uint32_t div;
  } CLKCtrlReg;


  volatile CLKCtrlReg *clkReg;
  
 public:
  void initClock();
  Clock(Peripheral * peripheralUtil);
  ~Clock(void);
};
#endif  // INCLUDE_CLOCK_H_
