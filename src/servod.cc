
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

Mark Broihier
*/

#include <sys/select.h>

#include "../include/Clock.h"
#include "../include/DMAChannel.h"
#include "../include/DMAChannels.h"
#include "../include/GPIO.h"
#include "../include/PWMHW.h"
#include "../include/Peripheral.h"
#include "../include/Servos.h"
#include "../include/mailbox.h"

#define SERVO_FIFO "/dev/servo_fifo"
FILE *servo_fifo;
bool stayInLoop = true;
void respondToRequest(DMAChannels * dmaChannels) {
  printf("Press Ctrl-C to end the program!\n");
  int n;
  int id;
  int width;
  char * line = NULL;
  char nl;
  size_t lineLen = 0;
  int fd = fileno(servo_fifo);
  fd_set fifoReady;
  int selectReturn;
  struct timeval timeout;
  while (stayInLoop) {
    FD_ZERO(&fifoReady);
    FD_SET(fd, &fifoReady);
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    selectReturn = select(fd + 1, &fifoReady, NULL, NULL, &timeout);
    if (selectReturn > 0) {
      if (getline(&line, &lineLen, servo_fifo)) {
        n = sscanf(line, "%d, %d%c", &id, &width, &nl);
        if (n != 3 || nl != '\n') {
          fprintf(stderr, "Bad input, enter only decimal digits\n%s\n", line);
        } else if (width < 1000 || width > 2000) {
          if (width == 0) {
            stayInLoop = false;
          } else {
            fprintf(stderr, "Pulse width out of range (1000 to 2000)\nInput was: %d\n", width);
          }
        } else {
          dmaChannels->setNewLocation(id, width);
        }
      } else {
        fprintf(stderr, "FIFO Input error\n");
      }
    }
  }
  unlink(SERVO_FIFO);
  dmaChannels->noPulse();
  fprintf(stderr, "Turn off PWM\n");
  sleep(2);
}

Servos::servoDefinition * readServoList() {
  Servos::servoDefinition * definitionList = NULL;
  Servos::servoDefinition * definitionListHead = NULL;
  FILE * servoFile = fopen("./servoDefinitionFile.txt", "r");
  if (servoFile) {
    fprintf(stderr, "servo definition file found\n");
  } else {
    fprintf(stderr, "servo definition file not found.  Terminating ...\n");
    exit(-1);
  }
  int n;
  uint32_t pin;
  uint32_t dmaChannel;
  char nl;
  while (1) {
    char * line = NULL;
    size_t lineLen = 0;
    if (getline(&line, &lineLen, servoFile)) {
      if (feof(servoFile)) break;
      n = sscanf(line, "%d, %d%c", &pin, &dmaChannel, &nl);
      if (n != 3 || nl != '\n') {
        fprintf(stderr, "Bad input, enter only a pair of decimal digits separated by a comma and space\n");
        fprintf(stderr, "line length returned was %d\n", lineLen);
        break;
      } else if (dmaChannel < 1 || dmaChannel > 15) {
        fprintf(stderr, "DMA Channel out of range (1 to 15)\nInput was: %d\n", dmaChannel);
        exit(-1);
      } else {
        fprintf(stderr, "There will be a servo attached to GPIO pin: %d on DMA Channel %d\n",
                pin, dmaChannel);
        if (definitionList) {
          definitionList->nextDefinition = reinterpret_cast<Servos::servoDefinition *>(malloc(sizeof(Servos::servoDefinition)));
          definitionList = definitionList->nextDefinition;
        } else {
          definitionList = reinterpret_cast<Servos::servoDefinition *>(malloc(sizeof(Servos::servoDefinition)));
          definitionListHead = definitionList;
        }
        definitionList->gpioBit = pin;
        definitionList->DMAChannel = dmaChannel;
        definitionList->nextDefinition = NULL;
      }
    } else {
      fprintf(stderr, "Error while attempting to read servo definitions.  Terminating...\n");
      exit(-1);
    }
  }
  return definitionListHead;
}

void sigint_handler(int signo) {
  if (signo == SIGINT) {
    // Release the resources properly
    fprintf(stderr, "\nEnding Servo daemon!\n");
    stayInLoop = false;
    //exit(0);
  }
}

int main(int argc, char ** argv) {
  signal(SIGINT, sigint_handler);

  if (argc == 1) { // run as daemon
    pid_t pid = fork();
    if (pid == 0) {
      fclose(stdin);
      fclose(stdout);
      fclose(stderr);
      setsid();
    } else {  // parent or error - exit
      if (pid < 0) fprintf(stderr, "Fork failed - servod did not start\n");
      exit(0);
    }
  }
  Peripheral peripheralUtil;  //  create an object to reference peripherals
  Servos::servoDefinition * servoDefinitionList = readServoList();

  fprintf(stderr, "making servo objects\n");
  Servos servos(servoDefinitionList);
  fprintf(stderr, "making dma control blocks\n");
  DMAChannels dmaChannels(servos.getServoList(), &peripheralUtil);
  usleep(100);
  fprintf(stderr, "setup GPIO pins\n");
  GPIO gpio(servos.getServoList(), &peripheralUtil);
  usleep(100);
  fprintf(stderr, "setup clock for PWM Hardware\n");
  Clock clock(&peripheralUtil);
  usleep(100);
  fprintf(stderr, "setup PWM Hardware for DMA delays\n");
  PWMHW pwmHW(&peripheralUtil);
  usleep(100);
  fprintf(stderr, "start DMA\n");
  usleep(100);
  dmaChannels.dmaStart();

  unlink(SERVO_FIFO);  // delete any previous definition
  if (mkfifo(SERVO_FIFO, 0666) < 0) {
    fprintf(stderr, "servo failed to create %s\n", SERVO_FIFO);
    exit(-1);
  }
  if (chmod(SERVO_FIFO, 0666) < 0) {
    fprintf(stderr, "servo failed to set permissions for %s\n", SERVO_FIFO);
    exit(-1);
  }
  servo_fifo = fopen(SERVO_FIFO, "r+");
  // Start monitoring FIFO
  respondToRequest(&dmaChannels);

  return 0;
}
