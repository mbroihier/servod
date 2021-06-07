/*
 * Servo class for making a DMA Channel that controls the PWM timing
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
#include "../include/DMAChannel.h"

DMAChannel::DMAMemHandle * DMAChannel::dmaMalloc(size_t size) {
  if (mailboxFD < 0) {
    mailboxFD = mbox_open();
    assert(mailboxFD >= 0);
  }

  // Make `size` a multiple of PAGE_SIZE
  size = ((size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

  DMAMemHandle *mem = reinterpret_cast<DMAMemHandle *>(malloc(sizeof(DMAMemHandle)));
  // Documentation: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
  mem->mbHandle = mem_alloc(mailboxFD, size, PAGE_SIZE, MEM_FLAG_L1_NONALLOCATING);
  mem->busAddr = mem_lock(mailboxFD, mem->mbHandle);
  mem->virtualAddr = mapmem(BUS_TO_PHYS(mem->busAddr), size);
  mem->size = size;

  assert(mem->busAddr != 0);

  fprintf(stderr, "MBox alloc: %d bytes, bus: %08X, virt: %08X\n", mem->size, mem->busAddr,
          (uint32_t)mem->virtualAddr);

  return mem;
}

void DMAChannel::dmaFree(DMAMemHandle *mem) {
  if (mem->virtualAddr == NULL) return;

  unmapmem(mem->virtualAddr, PAGE_SIZE);
  mem_unlock(mailboxFD, mem->mbHandle);
  mem_free(mailboxFD, mem->mbHandle);
  mem->virtualAddr = NULL;
}

void DMAChannel::dmaAllocBuffers() {
  dmaCBs = dmaMalloc(2 * CB_COUNT * sizeof(DMAControlBlock));
  dmaGPIOPinOn = dmaMalloc(sizeof(uint32_t) * MAXIMUM_NUMBER_OF_SERVOS_PER_CHANNEL);
  dmaGPIOPinOff = dmaMalloc(sizeof(uint32_t) * MAXIMUM_NUMBER_OF_SERVOS_PER_CHANNEL);
}
void DMAChannel::dmaInitCBs() {
  DMAControlBlock *cb;
  int index = 0;
  int timeSlice = 0;
  while (index < ALL_CB_COUNT) {
    // gpio pin on block
    fprintf(stderr, "control block index: %d CB_COUNT: %d ALL_CB_COUNT %d\n",
            index, CB_COUNT, ALL_CB_COUNT);
    cb = ithCBVirtAddr(index);
    cb->txInfo = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
    cb->src = gpioPinOnBusAddr(timeSlice);
    cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_SET + dmaGPIOOnOffIndex[timeSlice];
    fprintf(stderr, "gpioPinOnBusAddr %8.8x\nOffset Index %d\n", gpioPinOnBusAddr(timeSlice),
            dmaGPIOOnOffIndex[timeSlice]);
    fprintf(stderr, "mask: %8.8x\n",
            reinterpret_cast<uint32_t *>(dmaGPIOPinOn->virtualAddr)[timeSlice]);;
    cb->txLen = 4;
    index++;
    fprintf(stderr, "index: %d\n", index);
    cb->nextCB = ithCBBusAddr(index);
    // First Delay block
    cb = ithCBVirtAddr(index);
    cb->txInfo = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_DEST_DREQ | DMA_PERIPHERAL_MAPPING(5);
    cb->src = ithCBBusAddr(0);  // Dummy data
    cb->dest = PERI_BUS_BASE + PWM_BASE + PWM_FIFO;
    if (servoRef[timeSlice]) {  // if there is a servo, get its location
      cb->txLen = 4 * servoRef[timeSlice]->getLocation();
    } else {
      cb->txLen = 4;
    }
    index++;
    fprintf(stderr, "index: %d\n", index);
    cb->nextCB = ithCBBusAddr(index);
    // gpio pin off block
    cb = ithCBVirtAddr(index);
    cb->txInfo = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
    cb->src = gpioPinOffBusAddr(timeSlice);
    cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_CLEAR + dmaGPIOOnOffIndex[timeSlice];
    cb->txLen = 4;
    index++;
    fprintf(stderr, "index: %d\n", index);
    cb->nextCB = ithCBBusAddr(index);
    // Second Delay block
    cb = ithCBVirtAddr(index);
    cb->txInfo = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_DEST_DREQ | DMA_PERIPHERAL_MAPPING(5);
    cb->src = ithCBBusAddr(0);  // Dummy data
    cb->dest = PERI_BUS_BASE + PWM_BASE + PWM_FIFO;
    if (servoRef[timeSlice]) {  // if there is a servo, get its location
      cb->txLen = 4 * (TICS_PER_TIME_SLOT - servoRef[timeSlice]->getLocation());
    } else {
      cb->txLen = 4 * (TICS_PER_TIME_SLOT - 1);
    }
    index++;
    fprintf(stderr, "index: %d\n", index);
    if (index == CB_COUNT) {
      cb->nextCB = ithCBBusAddr(0);  // wrap to the start of this set of blocks
    } else if (index == ALL_CB_COUNT) {
      cb->nextCB = ithCBBusAddr(CB_COUNT);  // wrap to the start of this set of blocks
    } else {  // go to next time slot
      cb->nextCB = ithCBBusAddr(index);
    }
    timeSlice = (timeSlice + 1) % MAXIMUM_NUMBER_OF_SERVOS_PER_CHANNEL;
  }
}

void DMAChannel::setNewLocation() {
  DMAControlBlock *cb;
  // update control blocks that aren't active
  int index = 0;
  int stopIndex = CB_COUNT;
  int timeSlice = 0;
  if (lastCBStart == ithCBBusAddr(0)) {
    index = CB_COUNT;
    stopIndex = ALL_CB_COUNT;
  }
  while (index < stopIndex) {
    // gpio pin on block
    cb = ithCBVirtAddr(index);
    cb->txInfo = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
    cb->src = gpioPinOnBusAddr(timeSlice);
    cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_SET + dmaGPIOOnOffIndex[timeSlice];
    cb->txLen = 4;
    index++;
    cb->nextCB = ithCBBusAddr(index);
    // First Delay block
    cb = ithCBVirtAddr(index);
    cb->txInfo = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_DEST_DREQ | DMA_PERIPHERAL_MAPPING(5);
    cb->src = ithCBBusAddr(0);  // Dummy data
    cb->dest = PERI_BUS_BASE + PWM_BASE + PWM_FIFO;
    if (servoRef[timeSlice]) {  // if there is a servo, get its location
      cb->txLen = 4 * servoRef[timeSlice]->getLocation();
    } else {
      cb->txLen = 4;
    }
    index++;
    cb->nextCB = ithCBBusAddr(index);
    // gpio pin off block
    cb = ithCBVirtAddr(index);
    cb->txInfo = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
    cb->src = gpioPinOffBusAddr(timeSlice);
    cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_CLEAR + dmaGPIOOnOffIndex[timeSlice];
    cb->txLen = 4;
    index++;
    cb->nextCB = ithCBBusAddr(index);
    // Second Delay block
    cb = ithCBVirtAddr(index);
    cb->txInfo = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_DEST_DREQ | DMA_PERIPHERAL_MAPPING(5);
    cb->src = ithCBBusAddr(0);  // Dummy data
    cb->dest = PERI_BUS_BASE + PWM_BASE + PWM_FIFO;
    if (servoRef[timeSlice]) {  // if there is a servo, get its location
      cb->txLen = 4 * (TICS_PER_TIME_SLOT - servoRef[timeSlice]->getLocation());
    } else {
      cb->txLen = 4 * (TICS_PER_TIME_SLOT - 1);
    }
    index++;
    if (index == CB_COUNT) {
      cb->nextCB = ithCBBusAddr(0);  // wrap to the start of this set of blocks
    } else if (index == ALL_CB_COUNT) {
      cb->nextCB = ithCBBusAddr(CB_COUNT);  // wrap to the start of this set of blocks
    } else {  // go to next time slot
      cb->nextCB = ithCBBusAddr(index);
    }
    timeSlice++;
  }
}

void DMAChannel::swapDMACBs() {
  DMAControlBlock *cb;

  if (lastCBStart == ithCBBusAddr(0)) {
    cb = ithCBVirtAddr(CB_COUNT - 1);
    cb->nextCB = ithCBBusAddr(CB_COUNT);
    lastCBStart = ithCBBusAddr(CB_COUNT);
  } else {
    cb = ithCBVirtAddr(ALL_CB_COUNT - 1);
    cb->nextCB = ithCBBusAddr(0);
    lastCBStart = ithCBBusAddr(0);
  }
}

void DMAChannel::dmaStart() {
  // Reset the DMA channel
  fprintf(stderr, "Starting DMA channel controller\n");
  dmaReg->cs = DMA_CHANNEL_ABORT;
  dmaReg->cs = 0;
  dmaReg->cs = DMA_CHANNEL_RESET;
  dmaReg->cbAddr = 0;

  dmaReg->cs = DMA_INTERRUPT_STATUS | DMA_END_FLAG;

  // Make cbAddr point to the first DMA control block and enable DMA transfer
  dmaReg->cbAddr = ithCBBusAddr(0);
  lastCBStart = ithCBBusAddr(0);
  dmaReg->cs = DMA_PRIORITY(8) | DMA_PANIC_PRIORITY(8) | DMA_DISDEBUG;
  dmaReg->cs |= DMA_WAIT_ON_WRITES | DMA_ACTIVE;
}

void DMAChannel::dmaEnd() {
  fprintf(stderr, "Stopping DMA channel controller\n");
  // Shutdown DMA channel.
  dmaReg->cs |= DMA_CHANNEL_ABORT;
  usleep(100);
  dmaReg->cs &= ~DMA_ACTIVE;
  dmaReg->cs |= DMA_CHANNEL_RESET;
  usleep(100);

  // Release the memory used by DMA
  dmaFree(dmaCBs);
  dmaFree(dmaGPIOPinOn);
  dmaFree(dmaGPIOPinOff);

  free(dmaCBs);
  free(dmaGPIOPinOn);
  free(dmaGPIOPinOff);
}

DMAChannel::DMAChannel(Servos::servoListElement * list, uint32_t channel, Peripheral * peripheralUtil) {
  Servos::servoListElement * aServo = list;
  dmaAllocBuffers();
  uint8_t * dmaBasePtr = reinterpret_cast<uint8_t *>(peripheralUtil->mapPeripheralToUserSpace(DMA_BASE, PAGE_SIZE));
  dmaReg = reinterpret_cast<DMACtrlReg *>(dmaBasePtr + channel * 0x100);
  if (aServo) {
    int servoTimeSlice = 0;
    while (servoTimeSlice < MAXIMUM_NUMBER_OF_SERVOS_PER_CHANNEL) {
      fprintf(stderr, "Working on servo time slice %d\n", servoTimeSlice);
      if (aServo) {
        if (!aServo->servoPtr) {
          fprintf(stderr, "Servo not defined.  Terminating...\n");
          exit(-1);
        }
        if (aServo->servoPtr->getDMAChannel() == channel) {  // this servo is timed by this DMA channel
          // add servo to control
          fprintf(stderr, "Putting servo %d info into time slice %d of DMA Channel %d\n",
                  aServo->servoPtr->getID(), servoTimeSlice, channel);
          servoRef[servoTimeSlice] = aServo->servoPtr;
          reinterpret_cast<uint32_t *>(dmaGPIOPinOn->virtualAddr)[servoTimeSlice] = aServo->servoPtr->getOnMask();
          reinterpret_cast<uint32_t *>(dmaGPIOPinOff->virtualAddr)[servoTimeSlice] = aServo->servoPtr->getOffMask();
          dmaGPIOOnOffIndex[servoTimeSlice] = aServo->servoPtr->getGPIOOnOffIndex();
          servoTimeSlice++;  // only advance time slice if we fill it with something
        } else {
          fprintf(stderr, "Servo isn't on this DMA channel", channel);
        }
        // advance to next servo in the list
        aServo = aServo->nextServo;
      } else {  // there are no more servos that could be controlled by this channel
        // fill with the default delay of the time slice
        reinterpret_cast<uint32_t *>(dmaGPIOPinOn->virtualAddr)[servoTimeSlice] = 0;
        reinterpret_cast<uint32_t *>(dmaGPIOPinOff->virtualAddr)[servoTimeSlice] = 0;
        dmaGPIOOnOffIndex[servoTimeSlice] = 0;
        servoRef[servoTimeSlice] = NULL;
        servoTimeSlice++;
      }
    }
    // add DMA control block definitions for this channel
    dmaInitCBs();  
    lastCBStart = ithCBBusAddr(0);
  } else {
    fprintf(stderr, "Invalid list of servos.  At least one should be defined.  Terminating...\n");
    exit(-1);
  }
}
DMAChannel::~DMAChannel(void) {
  dmaEnd();
}
