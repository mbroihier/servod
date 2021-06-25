
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

Class for defining an object that contains all the DMA Channels

Mark Broihier 2021
*/
#ifndef INCLUDE_DMACHANNELS_H_
#define INCLUDE_DMACHANNELS_H_
#include <map>
#include "../include/DMAChannel.h"
#include "../include/Peripheral.h"
#include "../include/Servos.h"

class DMAChannels {
 private:
  std::map<int, DMAChannel *> servoIDToDMAChannel;  // servo id to DMA channel object
  std::map<int, int> channelDefined;  // channel to boolean declaring that it is defined
  std::map<DMAChannel *, bool> uniqueDMAChannel;
  std::map<int, Servo *> servoIDToServo;  // servo id to Servo object
  std::map<int, int> servoIDToGPIOBit;  // servo id to GPIO Bit
  std::map<Servo *, int> servoToServoID;  // Servo object to servo ID
  std::map<int, int> gpioBitToServoID;  // GPIO Bit to servo ID

 public:
  void dmaStart();
  void swapDMACBs();
  bool setNewLocation(uint32_t servoID, uint32_t location);
  void noPulse();
  DMAChannels(Servos::servoListElement * list, Peripheral * peripheralUtils);
  ~DMAChannels(void);
};
#endif  // INCLUDE_DMACHANNELS_H_
