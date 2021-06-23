
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

Class for holding all servos to be controlled

Mark Broihier 2021
*/

#include "../include/Servos.h"

Servos::Servos(servoDefinition * list) {
  servoDefinition * aServo = list;
  if (aServo) {
    numberOfServos = 0;
    servoList = reinterpret_cast<servoListElement *>(malloc(sizeof(servoListElement)));
    servoListElement * firstEntry = servoList;
    while (aServo != NULL) {
      if (numberOfServos >= MAXIMUM_NUMBER_OF_SERVOS) {
        // don't let the software attempt to control more servos than can be present
        fprintf(stderr, "Error - attempting to control too many servos. Terminating ....\n");
        exit(-1);
      }
      servoList->servoPtr = new Servo(numberOfServos++, aServo->gpioBit, aServo->DMAChannel);
      aServo = aServo->nextDefinition;
      if (aServo) {
        servoList->nextServo = reinterpret_cast<servoListElement *>(malloc(sizeof(servoListElement)));
        servoList = servoList->nextServo;
      } else {
        servoList->nextServo = NULL;
        servoList = firstEntry;
      }
    }
  } else {
    fprintf(stderr, "Bad servo initialization list.  Terminating....\n");
    exit(-1);
  }
}
Servos::~Servos(void) {
  fprintf(stderr, "Shutting down Servos\n");
  while (servoList) {
    servoListElement * toFree = servoList;
    delete (servoList->servoPtr);
    servoList = servoList->nextServo;
    free(toFree);
  }
}
