#ifndef INCLUDE_SERVOS_H_
#define INCLUDE_SERVOS_H_
/*
 * Class for controlling all the servos connected to the Pi
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
#include <stdio.h>
#include <stdlib.h>
#include "../include/Servo.h"

class Servos {
 public:
  typedef struct servoDefinition {
    uint32_t gpioBit;
    uint32_t DMAChannel;
    servoDefinition * nextDefinition;
  } servoDefinition;
  typedef struct servoListElement {
    Servo * servoPtr;
    servoListElement * nextServo;
  } servoListElement;
 private:
  const uint32_t MAXIMUM_NUMBER_OF_SERVOS = 10;
  const uint32_t NUMBER_OF_DMA_CHANNELS = 1;
  uint32_t numberOfServos;
  servoListElement * servoList;
 public:
  inline servoListElement * getServoList() { return servoList; };
  explicit Servos(servoDefinition * list);
  ~Servos(void);
};
#endif  // INCLUDE_SERVOS_H_
