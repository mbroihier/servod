
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

Raspberry Pi PWM Broadcom HW Control

Mark Broihier 2021
*/

#include "../include/PWMHW.h"

void PWMHW::initPWM() {
  // reset Broadcom PWM
  pwmReg->ctrl = 0;
  usleep(10);
  pwmReg->status = -1;
  usleep(10);

  /* get the PLLD clock frequency we are using */

  uint32_t rangeSetting;
  FILE * pipe = popen("sudo cat /sys/kernel/debug/clk/clk_summary | grep \"plld_per\" |"
                      "awk \'{ split($0, a, \" +\"); print a[6]}\'", "r");
  fscanf(pipe, "%d", &rangeSetting);
  fprintf(stderr, "Clock reading was: %d\n", rangeSetting);
  pclose(pipe);
  if (rangeSetting >= 500000000 && rangeSetting <= 751000000) {
    rangeSetting /= 5000000;  // adjust so that the setting will be 1 usec per clock
    fprintf(stderr, "Clock divider will be: %d\n", rangeSetting);
  } else {
    fprintf(stderr, "PLLD frequency was not in an expected range.  Terminating...\n");
    exit(-1);
  }

  pwmReg->range1 = rangeSetting;

  // enable PWM DMA, raise panic and dreq thresholds to 15
  pwmReg->dmaCfg = PWM_DMAC_ENAB | PWM_DMAC_PANIC(15) | PWM_DMAC_DREQ(15);
  usleep(10);

  // clear PWM fifo
  pwmReg->ctrl = PWM_CTL_CLRF1;
  usleep(10);

  // enable PWM channel 1 and use fifo
  pwmReg->ctrl = PWM_CTL_USEF1 | PWM_CTL_MODE1 | PWM_CTL_PWEN1;

  fprintf(stderr, "PWMHW setup complete\n");
}

PWMHW::PWMHW(Peripheral * peripheralUtil) {
  pwmReg = reinterpret_cast<PWMCtrlReg *>(peripheralUtil->mapPeripheralToUserSpace(PWM_BASE, PWM_LEN));
  initPWM();
}

PWMHW::~PWMHW() {
  fprintf(stderr, "Shutting down PWMHW\n");
}
