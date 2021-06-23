
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

Class for making objects that store servo information

Mark Broihier 2021
*/

#include "../include/Servo.h"

bool Servo::setNewLocation(uint32_t location) {
  bool valid = false;
  if (location >= MINIMUM_SERVO_ON && location <= MAXIMUM_SERVO_ON) {
    this->location = location;
    valid = true;
  }
  return(valid);
}

Servo::Servo(uint32_t number, uint32_t gpioBit, uint32_t DMAChannel) {
  this->number = number;
  this->gpioBit = gpioBit;
  this->DMAChannel = DMAChannel;
  onMask = 1 << (gpioBit % 32);   // there are 32 gpio bits per offset of the Broadcom interface - kind of
  offMask = 1 << (gpioBit % 32);  //                        ||
  gpioOnOffIndex = gpioBit / 32;
  location = (MINIMUM_SERVO_ON + MAXIMUM_SERVO_ON)/ 2;
  fprintf(stderr, "Servo ID: %d\n", number);
  fprintf(stderr, "GPIO Bit: %d\n", gpioBit);
  fprintf(stderr, "DMA Channel: %d\n", DMAChannel);
  fprintf(stderr, "on or off mask: %8.8x\n", onMask);
  fprintf(stderr, "gpioOnOffIndex: %d\n", gpioOnOffIndex);
}
Servo::~Servo(void) {
  fprintf(stderr, "Shutting down servo\n");
}
