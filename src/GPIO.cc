/*
 * Raspberry Pi GPIO broadcom HW control
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
#include "../include/GPIO.h"

GPIO::GPIO(Servos::servoListElement * list, Peripheral * peripheralUtil) {
  Servos::servoListElement * aServo = list;
  volatile uint32_t * gpioModeReg = reinterpret_cast<uint32_t *>(peripheralUtil->mapPeripheralToUserSpace(GPIO_BASE, GPIO_MODE_SIZE));
  uint32_t pin;
  uint32_t offset;
  uint32_t setting;
  uint32_t shift;
  uint32_t mask;
  uint32_t originalSettings;
  if (aServo) {
    while (aServo) {
      pin = aServo->servoPtr->getBit();
      fprintf(stderr, "Setting Servo ID %d GPIO pin %d to be output\n", aServo->servoPtr->getID(), pin);
      offset = pin/10;
      shift = (pin % 10) * 3;
      setting = 1 << shift;
      mask = 7 << shift;
      originalSettings = gpioModeReg[offset];
      fprintf(stderr, "Index: %d, mode: %8.8x, address: %p\n", offset, setting, gpioModeReg);
      gpioModeReg[offset] = setting | (originalSettings & ~mask);
      aServo = aServo->nextServo;
    }
  } else {
    fprintf(stderr, "Invalid list of servos.  At least one should be defined.  Terminating...\n");
    exit(-1);
  }
}

GPIO::~GPIO() {
}
