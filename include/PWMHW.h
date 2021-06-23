
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

Class for creating Raspberry Pi Broadcom PWM HW Control

Mark Broihier 2021
*/

#ifndef INCLUDE_PWMHW_H_
#define INCLUDE_PWMHW_H_
#include <stdint.h>
#include <unistd.h>
#include "../include/Peripheral.h"

/* PWM mapping information */
#define PWM_BASE 0x0020C000
#define PWM_LEN 0x28

/* PWM control bits */
#define PWM_CTL 0
#define PWM_DMAC 2

#define PWM_CTL_PWEN2 (1 << 8)
#define PWM_CTL_CLRF1 (1 << 6)
#define PWM_CTL_USEF1 (1 << 5)
#define PWM_CTL_MODE1 (1 << 1)
#define PWM_CTL_PWEN1 (1 << 0)

#define PWM_DMAC_ENAB (1 << 31)
#define PWM_DMAC_PANIC(x) ((x) << 8)
#define PWM_DMAC_DREQ(x) (x)
#define CLK_MICROS 1

class PWMHW {
 private:
  typedef struct PWMCtrlReg {
    uint32_t ctrl;      // 0x0, Control
    uint32_t status;    // 0x4, Status
    uint32_t dmaCfg;   // 0x8, DMA configuration
    uint32_t padding1;  // 0xC, 4-byte padding
    uint32_t range1;    // 0x10, Channel 1 range
    uint32_t data1;     // 0x14, Channel 1 data
    uint32_t fifoIn;   // 0x18, FIFO input
    uint32_t padding2;  // 0x1C, 4-byte padding again
    uint32_t range2;    // 0x20, Channel 2 range
    uint32_t data2;     // 0x24, Channel 2 data
  } PWMCtrlReg;

  volatile PWMCtrlReg * pwmReg;

 public:
  void initPWM();
  explicit PWMHW(Peripheral * peripheralUtil);
  ~PWMHW(void);
};
#endif  // INCLUDE_PWMHW_H_
