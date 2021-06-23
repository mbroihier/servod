
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

Class for defining the DMA control blocks used for servo control

Mark Broihier 2021
*/

#ifndef INCLUDE_DMACHANNEL_H_
#define INCLUDE_DMACHANNEL_H_
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/mailbox.h"
#include "../include/Peripheral.h"
#include "../include/Servos.h"
#include "../include/misc.h"

// https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
#define MEM_FLAG_DIRECT (1 << 2)
#define MEM_FLAG_COHERENT (2 << 2)
#define MEM_FLAG_L1_NONALLOCATING (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT)
#define BUS_TO_PHYS(x) ((x) & ~0xC0000000)

/* DMA Base Address */
#define DMA_BASE 0x00007000

/* DMA control block "info" field bits */
#define DMA_NO_WIDE_BURSTS (1 << 26)
#define DMA_PERIPHERAL_MAPPING(x) ((x) << 16)
#define DMA_DEST_DREQ (1 << 6)
#define DMA_WAIT_RESP (1 << 3)

/* DMA CS Control and Status bits */
#define DMA_CHANNEL_RESET (1 << 31)
#define DMA_CHANNEL_ABORT (1 << 30)
#define DMA_WAIT_ON_WRITES (1 << 28)
#define DMA_PANIC_PRIORITY(x) ((x) << 20)
#define DMA_PRIORITY(x) ((x) << 16)
#define DMA_INTERRUPT_STATUS (1 << 2)
#define DMA_END_FLAG (1 << 1)
#define DMA_ACTIVE (1 << 0)
#define DMA_DISDEBUG (1 << 28)

class DMAChannel {
 private:
  static const uint32_t MAXIMUM_NUMBER_OF_SERVOS_PER_CHANNEL = 10;
  const uint32_t CB_COUNT = 4 * MAXIMUM_NUMBER_OF_SERVOS_PER_CHANNEL;
  const uint32_t TICS_PER_TIME_SLOT = Servo::PERIOD / MAXIMUM_NUMBER_OF_SERVOS_PER_CHANNEL;
  const uint32_t ALL_CB_COUNT = 2 * CB_COUNT;
  const uint32_t PAGE_SIZE = 4096;
  typedef struct DMACtrlReg {
    uint32_t cs;       // DMA Channel Control and Status register
    uint32_t cbAddr;   // DMA Channel Control Block Address
  } DMACtrlReg;

  typedef struct DMAControlBlock {
    uint32_t txInfo;      // Transfer information
    uint32_t src;         // Source (bus) address
    uint32_t dest;        // Destination (bus) address
    uint32_t txLen;       // Transfer length (in bytes)
    uint32_t stride;      // 2D stride
    uint32_t nextCB;      // Next DMAControlBlock (bus) address
    uint32_t padding[2];  // 2-word padding
  } DMAControlBlock;

  typedef struct DMAMemHandle {
    void *virtualAddr;  // Virutal base address of the page
    uint32_t busAddr;   // Bus adress of the page, this is not a pointer in user space
    uint32_t mbHandle;  // Used by mailbox property interface
    uint32_t size;
  } DMAMemHandle;

  int mailboxFD = -1;
  DMAMemHandle *dmaCBs;
  DMAMemHandle *dmaGPIOPinOn;
  DMAMemHandle *dmaGPIOPinOff;
  volatile DMACtrlReg *dmaReg;

  uint32_t lastCBStart;

  Servo * servoRef[MAXIMUM_NUMBER_OF_SERVOS_PER_CHANNEL];
  uint32_t dmaGPIOOnOffIndex[MAXIMUM_NUMBER_OF_SERVOS_PER_CHANNEL];
  uint32_t totalNumberOfDMAChannels;
  uint32_t channel;

  DMAMemHandle *dmaMalloc(size_t size);
  void dmaFree(DMAMemHandle *mem);
  void dmaAllocBuffers();
  inline DMAControlBlock *ithCBVirtAddr(int i) {
    return reinterpret_cast<DMAControlBlock *>(dmaCBs->virtualAddr) + i;
  }
  inline uint32_t ithCBBusAddr(int i) { return dmaCBs->busAddr + i * sizeof(DMAControlBlock); }
  inline uint32_t gpioPinOnBusAddr(int i) { return dmaGPIOPinOn->busAddr + i * sizeof(uint32_t); }
  inline uint32_t gpioPinOffBusAddr(int i) { return dmaGPIOPinOff->busAddr + i * sizeof(uint32_t); }
  void dmaInitCBs();
  void dmaEnd();

 public:
  void dmaStart();
  void setNewLocation();
  void swapDMACBs();
  void adjustForAdditionalDMAChannels(uint32_t totalNumberOfDMAChannels);
  void noPulse();
  DMAChannel(Servos::servoListElement * list, uint32_t channel, Peripheral * peripheralUtil);
  ~DMAChannel(void);
};
#endif  // INCLUDE_DMACHANNEL_H_
