/*
 * Raspberry Pi PWM broadcom HW control
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
#include "../include/PWMHW.h"

void PWMHW::initPWM() {
  
  // reset Broadcom PWM
  pwmReg->ctrl = 0;
  usleep(10);
  pwmReg->status = -1;
  usleep(10);

  /*
   * set number of bits to transmit
   * e.g, if CLK_MICROS is 5, since we have set the frequency of the
   * hardware clock to 100 MHZ, then the time taken for `100 * CLK_MICROS` bits
   * is (500 / 100) = 5 us, this is how we control the DMA sampling rate
   */
  pwmReg->range1 = 100 * CLK_MICROS;

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
