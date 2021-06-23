
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

Class for making all DMA channels necessary to control all servos

Mark Broihier 2021
*/
#include "../include/DMAChannels.h"

void DMAChannels::dmaStart() {
  for (std::map<DMAChannel *, bool>::iterator i = uniqueDMAChannel.begin(); i != uniqueDMAChannel.end(); i++) {
    i->first->dmaStart();
  }
}

void DMAChannels::setNewLocation(uint32_t servoID, uint32_t location) {
  std::map<int, Servo *>::iterator i = servoIDToServo.find(servoID);
  if (i != servoIDToServo.end()) {
    servoIDToServo[servoID]->setNewLocation(location);
  } else {
    fprintf(stderr, "reference to an undefined servo: %d\n", servoID);
    return;
  }
  servoIDToDMAChannel[servoID]->setNewLocation();
}

void DMAChannels::swapDMACBs() {
  fprintf(stderr, "swapping in new DMA Control Buffsers\n");
  for (std::map<DMAChannel *, bool>::iterator i = uniqueDMAChannel.begin(); i != uniqueDMAChannel.end(); i++) {
    i->first->swapDMACBs();
  }
}

void DMAChannels::noPulse() {
  //  force on masks to be zero so no GPIO bit is turned on
  for (std::map<DMAChannel *, bool>::iterator i = uniqueDMAChannel.begin(); i != uniqueDMAChannel.end(); i++) {
    i->first->noPulse();
  }
}

DMAChannels::DMAChannels(Servos::servoListElement * list, Peripheral * peripheralUtils) {
  Servos::servoListElement * aServo = list;
  uint32_t servoID = 0;
  uint32_t totalNumberOfDMAChannels = 0;
  if (aServo) {
    while (aServo) {  // walk through list making DMA Channels when necessary
      uint32_t gpioBit = aServo->servoPtr->getBit();
      {
        std::map<int, int>::iterator location = gpioBitToServoID.find(gpioBit);
        if (location != gpioBitToServoID.end()) {  // this GPIO bit/pin has been defined
          fprintf(stderr, "Error - duplicate GPIO bit/pin: %d, terminating....\n", gpioBit);
          exit(-1);
        }
        gpioBitToServoID[gpioBit] = servoID;
        servoIDToGPIOBit[servoID] = gpioBit;
      }
      Servo * servoPtr = aServo->servoPtr;
      {
        std::map<Servo *, int>::iterator location = servoToServoID.find(servoPtr);
        if (location != servoToServoID.end()) {  // this Servo has been defined
          fprintf(stderr, "Error - duplicate servo reference S/W error, terminating....\n");
          exit(-1);
        }
        servoIDToServo[servoID] = servoPtr;
        servoToServoID[servoPtr] = servoID;
      }
      uint32_t channel = aServo->servoPtr->getDMAChannel();
      {
        std::map<int, int>::iterator location = channelDefined.find(channel);
        if (location == channelDefined.end()) {  // this DMA channel needs to be constructed
          fprintf(stderr, "Building DMA channel %d control blocks\n", channel);
          servoIDToDMAChannel[servoID] = new DMAChannel(list, channel, peripheralUtils);
          uniqueDMAChannel[servoIDToDMAChannel[servoID]] = true;
          channelDefined[channel] = servoID;  // record this so we can use it
          totalNumberOfDMAChannels++;
        } else {  // alread constructed, simply reference
          servoIDToDMAChannel[servoID] = servoIDToDMAChannel[location->second];
        }
      }
      servoID++;
      aServo = aServo->nextServo;
    }
    if (totalNumberOfDMAChannels > 1) {  // recalibrate timing
      for (std::map<DMAChannel *, bool>::iterator i = uniqueDMAChannel.begin(); i != uniqueDMAChannel.end(); i++) {
        i->first->adjustForAdditionalDMAChannels(totalNumberOfDMAChannels);
      }
    }
  } else {
    fprintf(stderr, "Invalid list of servos.  At least one should be defined.  Terminating...\n");
    exit(-1);
  }
}
DMAChannels::~DMAChannels(void) {
  // delete the DMA channels
  for (std::map<DMAChannel *, bool>::iterator i = uniqueDMAChannel.begin(); i != uniqueDMAChannel.end(); i++) {
    delete (i->first);
  }
}
