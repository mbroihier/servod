#ifndef INCLUDE_SERVO_H_
#define INCLUDE_SERVO_H_
/*
 * Class for defining all the information needed to refer to and control a servo
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

class Servo {
 private:
  const uint32_t RANGE = 1000;  // 1000 clock range
  const uint32_t MINIMUM_SERVO_ON = 1000;
  const uint32_t MAXIMUM_SERVO_ON = MINIMUM_SERVO_ON + RANGE;
  const uint32_t MAXIMUM_NUMBER_OF_SERVOS = 10;  // per DMA channel
  const uint32_t TICS_PER_SERVO = MAXIMUM_SERVO_ON;
  uint32_t location;         // location of the servo (this is a time ranging from 1000 usec to 2000 usec
  uint32_t gpioBit;          // gpio bit that PWM signal is sent on
  uint32_t DMAChannel;       // DMA channel that will control the pulse timing
  uint32_t number;           // servo number/id
  uint32_t onMask;           // mask sent to turn the referenced gpio pin on
  uint32_t offMask;          // mask sent to turn the referenced gpio pin off
  uint32_t gpioOnOffIndex;   // gpio on/off mask offset
  uint32_t gpioModeIndex;    // gpio mode index (always set to output)
  
 public:
  static const uint32_t PERIOD = 20000;
  bool setNewLocation(uint32_t location);
  inline uint32_t getID() { return number; }
  inline uint32_t getLocation() { return location; }
  inline uint32_t getBit() { return gpioBit; }
  inline uint32_t getOnMask() { return onMask; }
  inline uint32_t getOffMask() { return offMask; }
  inline uint32_t getGPIOOnOffIndex() { return gpioOnOffIndex; }
  inline uint32_t getDMAChannel() { return DMAChannel; }
  Servo(uint32_t number, uint32_t gpioBit, uint32_t DMAChannel);
  ~Servo(void);
};
#endif  // INCLUDE_SERVO_H_
