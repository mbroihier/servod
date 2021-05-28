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
*/
#include <bcm_host.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>

#include "../include/mailbox.h"

/*
 * Check more about Raspberry Pi's register mapping at:
 * https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 * https://elinux.org/BCM2835_registers
 */
#define PAGE_SIZE 4096

#define SERVO_PIN 18
#define SERVO_FIFO "/dev/servo_fifo"
FILE *servo_fifo;

#define PERI_BUS_BASE 0x7E000000
uint32_t PERI_PHYS_BASE;
#define BUS_TO_PHYS(x) ((x) & ~0xC0000000)

#define GPIO_BASE 0x00200000
#define GPIO_PIN_CLEAR 0x28
#define GPIO_PIN_SET 0x1c

#define CM_BASE 0x00101000
#define CM_LEN 0xA8
#define CM_PWM 0xA0
#define CLK_CTL_BUSY (1 << 7)
#define CLK_CTL_KILL (1 << 5)
#define CLK_CTL_ENAB (1 << 4)
#define CLK_CTL_SRC(x) ((x) << 0)

#define CLK_SRCS 2

#define CLK_CTL_SRC_PLLD 6

#define CLK_DIV_DIVI(x) ((x) << 12)

#define BCM_PASSWD (0x5A << 24)

#define PWM_BASE 0x0020C000
#define PWM_LEN 0x28
#define PWM_FIFO 0x18

/* PWM control bits */
#define PWM_CTL 0
#define PWM_DMAC 2

#define PWM_CTL_PWEN2 (1 << 8)
#define PWM_CTL_CLRF1 (1 << 6)
#define PWM_CTL_USEF1 (1 << 5)
#define PWM_CTL_MODE1 (1 << 1)
#define PWM_CTL_PWEN1 (1 << 0)

#define PWM_DMAC_ENAB (1 << 31)
#define PWM_DMAC_PANIC(x) ((x) << 8)
#define PWM_DMAC_DREQ(x) (x)

#define DMA_BASE 0x00007000
#define DMA_CHANNEL 6
#define DMA_OFFSET 0x100
#define DMA_ADDR (DMA_BASE + DMA_OFFSET * (DMA_CHANNEL >> 2))

/* DMA CS Control and Status bits */
#define DMA_ENABLE (0xFF0 / 4)
#define DMA_CHANNEL_RESET (1 << 31)
#define DMA_CHANNEL_ABORT (1 << 30)
#define DMA_WAIT_ON_WRITES (1 << 28)
#define DMA_PANIC_PRIORITY(x) ((x) << 20)
#define DMA_PRIORITY(x) ((x) << 16)
#define DMA_INTERRUPT_STATUS (1 << 2)
#define DMA_END_FLAG (1 << 1)
#define DMA_ACTIVE (1 << 0)
#define DMA_DISDEBUG (1 << 28)

/* DMA control block "info" field bits */
#define DMA_NO_WIDE_BURSTS (1 << 26)
#define DMA_PERIPHERAL_MAPPING(x) ((x) << 16)
#define DMA_DEST_DREQ (1 << 6)
#define DMA_WAIT_RESP (1 << 3)

#define DMA_PWM_PERIOD 20002  // 20,000 microseconds plus a on and off control block


// https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
#define MEM_FLAG_DIRECT (1 << 2)
#define MEM_FLAG_COHERENT (2 << 2)
#define MEM_FLAG_L1_NONALLOCATING (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT)

#define CB_CNT DMA_PWM_PERIOD  // one block to turn on the servo bit,
                               // one block to turn off the servo bit,
                               // and then 20000 1usec delay blocks

#define CLK_DIVI 5
#define CLK_MICROS 1

typedef struct DMACtrlReg {
    uint32_t cs;       // DMA Channel Control and Status register
    uint32_t cb_addr;  // DMA Channel Control Block Address
} DMACtrlReg;

typedef struct DMAControlBlock {
    uint32_t tx_info;     // Transfer information
    uint32_t src;         // Source (bus) address
    uint32_t dest;        // Destination (bus) address
    uint32_t tx_len;      // Transfer length (in bytes)
    uint32_t stride;      // 2D stride
    uint32_t next_cb;     // Next DMAControlBlock (bus) address
    uint32_t padding[2];  // 2-word padding
} DMAControlBlock;

typedef struct DMAMemHandle {
    void *virtual_addr;  // Virutal base address of the page
    uint32_t bus_addr;   // Bus adress of the page, this is not a pointer in user space
    uint32_t mb_handle;  // Used by mailbox property interface
    uint32_t size;
} DMAMemHandle;

typedef struct CLKCtrlReg {
    // See https://elinux.org/BCM2835_registers#CM
    uint32_t ctrl;
    uint32_t div;
} CLKCtrlReg;

typedef struct PWMCtrlReg {
    uint32_t ctrl;      // 0x0, Control
    uint32_t status;    // 0x4, Status
    uint32_t dma_cfg;   // 0x8, DMA configuration
    uint32_t padding1;  // 0xC, 4-byte padding
    uint32_t range1;    // 0x10, Channel 1 range
    uint32_t data1;     // 0x14, Channel 1 data
    uint32_t fifo_in;   // 0x18, FIFO input
    uint32_t padding2;  // 0x1C, 4-byte padding again
    uint32_t range2;    // 0x20, Channel 2 range
    uint32_t data2;     // 0x24, Channel 2 data
} PWMCtrlReg;

int mailbox_fd = -1;
DMAMemHandle *dma_cbs;
DMAMemHandle *dma_gpio_pin_on;
DMAMemHandle *dma_gpio_pin_off;
volatile DMACtrlReg *dma_reg;
uint32_t last_cb_start;
volatile PWMCtrlReg *pwm_reg;
volatile CLKCtrlReg *clk_reg;
uint32_t *gpio_mode_reg;

DMAMemHandle *dma_malloc(unsigned int size) {
  if (mailbox_fd < 0) {
    mailbox_fd = mbox_open();
    assert(mailbox_fd >= 0);
  }

  // Make `size` a multiple of PAGE_SIZE
  size = ((size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

  DMAMemHandle *mem = reinterpret_cast<DMAMemHandle *>(malloc(sizeof(DMAMemHandle)));
  // Documentation: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
  mem->mb_handle = mem_alloc(mailbox_fd, size, PAGE_SIZE, MEM_FLAG_L1_NONALLOCATING);
  mem->bus_addr = mem_lock(mailbox_fd, mem->mb_handle);
  mem->virtual_addr = mapmem(BUS_TO_PHYS(mem->bus_addr), size);
  mem->size = size;

  assert(mem->bus_addr != 0);

  fprintf(stderr, "MBox alloc: %d bytes, bus: %08X, virt: %08X\n", mem->size, mem->bus_addr,
          (uint32_t)mem->virtual_addr);

  return mem;
}

void dma_free(DMAMemHandle *mem) {
  if (mem->virtual_addr == NULL)
        return;

  unmapmem(mem->virtual_addr, PAGE_SIZE);
  mem_unlock(mailbox_fd, mem->mb_handle);
  mem_free(mailbox_fd, mem->mb_handle);
  mem->virtual_addr = NULL;
}

void *map_peripheral(uint32_t addr, uint32_t size) {
  int mem_fd;
  // Check mem(4) about /dev/mem
  if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
    perror("Failed to open /dev/mem: ");
    exit(-1);
  }

  uint32_t *result = reinterpret_cast<uint32_t *>(mmap(
                                                       NULL,
                                                       size,
                                                       PROT_READ | PROT_WRITE,
                                                       MAP_SHARED,
                                                       mem_fd,
                                                       PERI_PHYS_BASE + addr));

  close(mem_fd);

  fprintf(stderr, "mmap to address %8.8x\n", addr);
  if (result == MAP_FAILED) {
    perror("mmap error: ");
    exit(-1);
  }
  return result;
}


void dma_alloc_buffers() {
  dma_cbs = dma_malloc(2 * CB_CNT * sizeof(DMAControlBlock));
  fprintf(stderr, "allocating uncached memory for dma_gpio_pin_on\n");
  dma_gpio_pin_on = dma_malloc(sizeof(uint32_t));
  fprintf(stderr, "allocating uncached memory for dma_gpio_pin_off\n");
  dma_gpio_pin_off = dma_malloc(sizeof(uint32_t));
}

static inline DMAControlBlock *ith_cb_virt_addr(int i) {
  return reinterpret_cast<DMAControlBlock *>(dma_cbs->virtual_addr) + i;
}

static inline uint32_t ith_cb_bus_addr(int i) { return dma_cbs->bus_addr + i * sizeof(DMAControlBlock); }

static inline uint32_t gpio_pin_on_bus_addr() { return dma_gpio_pin_on->bus_addr; }

static inline uint32_t gpio_pin_off_bus_addr() { return dma_gpio_pin_off->bus_addr; }

void dma_init_cbs() {
  int onClocks = 1000;

  DMAControlBlock *cb;
  for (int index = 0; index < DMA_PWM_PERIOD; index++) {
    if (index == 0) {
      // gpio pin on block
      cb = ith_cb_virt_addr(index);
      cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
      cb->src = gpio_pin_on_bus_addr();
      cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_SET;
      cb->tx_len = 4;
      index++;
      cb->next_cb = ith_cb_bus_addr(index);
    } else if (index == onClocks) {
      // gpio pin off block
      cb = ith_cb_virt_addr(index);
      cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
      cb->src = gpio_pin_off_bus_addr();
      cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_CLEAR;
      cb->tx_len = 4;
      index++;
      cb->next_cb = ith_cb_bus_addr(index);
    }
    // Delay block
    cb = ith_cb_virt_addr(index);
    cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_DEST_DREQ | DMA_PERIPHERAL_MAPPING(5);
    cb->src = ith_cb_bus_addr(0);  // Dummy data
    cb->dest = PERI_BUS_BASE + PWM_BASE + PWM_FIFO;
    cb->tx_len = 4;
    cb->next_cb = ith_cb_bus_addr((index + 1) % DMA_PWM_PERIOD);  // on the last cb, this will point to 0
  }
  onClocks = 2000;
  for (int index = DMA_PWM_PERIOD; index < 2 * DMA_PWM_PERIOD; index++) {
    if (index == DMA_PWM_PERIOD) {
      // gpio pin on block
      cb = ith_cb_virt_addr(index);
      cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
      cb->src = gpio_pin_on_bus_addr();
      cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_SET;
      cb->tx_len = 4;
      index++;
      cb->next_cb = ith_cb_bus_addr(index);
    } else if (index == DMA_PWM_PERIOD + onClocks) {
      // gpio pin off block
      cb = ith_cb_virt_addr(index);
      cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
      cb->src = gpio_pin_off_bus_addr();
      cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_CLEAR;
      cb->tx_len = 4;
      index++;
      cb->next_cb = ith_cb_bus_addr(index);
    }
    // Delay block
    cb = ith_cb_virt_addr(index);
    cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_DEST_DREQ | DMA_PERIPHERAL_MAPPING(5);
    cb->src = ith_cb_bus_addr(0);  // Dummy data
    cb->dest = PERI_BUS_BASE + PWM_BASE + PWM_FIFO;
    cb->tx_len = 4;
    // on the last cb, this will point to DMA_PWM_PERIOD
    cb->next_cb = ith_cb_bus_addr(((index + 1) % DMA_PWM_PERIOD) + DMA_PWM_PERIOD);
  }
}

void setNewLocation(int onClocks) {
  DMAControlBlock *cb;
  // update control blocks that aren't active
  if (last_cb_start == ith_cb_bus_addr(0)) {
    for (int index = DMA_PWM_PERIOD; index < 2 * DMA_PWM_PERIOD; index++) {
      if (index == DMA_PWM_PERIOD) {
        // gpio pin on block
        cb = ith_cb_virt_addr(index);
        cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
        cb->src = gpio_pin_on_bus_addr();
        cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_SET;
        cb->tx_len = 4;
        index++;
        cb->next_cb = ith_cb_bus_addr(index);
      } else if (index == DMA_PWM_PERIOD + onClocks) {
        // gpio pin off block
        cb = ith_cb_virt_addr(index);
        cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
        cb->src = gpio_pin_off_bus_addr();
        cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_CLEAR;
        cb->tx_len = 4;
        index++;
        cb->next_cb = ith_cb_bus_addr(index);
      }
      // Delay block
      cb = ith_cb_virt_addr(index);
      cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_DEST_DREQ | DMA_PERIPHERAL_MAPPING(5);
      cb->src = ith_cb_bus_addr(0);  // Dummy data
      cb->dest = PERI_BUS_BASE + PWM_BASE + PWM_FIFO;
      cb->tx_len = 4;
      // on the last cb, this will point to DMA_PWM_PERIOD
      cb->next_cb = ith_cb_bus_addr(((index + 1) % DMA_PWM_PERIOD) + DMA_PWM_PERIOD);
    }
  } else {
    for (int index = 0; index < DMA_PWM_PERIOD; index++) {
      if (index == 0) {
        // gpio pin on block
        cb = ith_cb_virt_addr(index);
        cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
        cb->src = gpio_pin_on_bus_addr();
        cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_SET;
        cb->tx_len = 4;
        index++;
        cb->next_cb = ith_cb_bus_addr(index);
      } else if (index == onClocks) {
        // gpio pin off block
        cb = ith_cb_virt_addr(index);
        cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP;
        cb->src = gpio_pin_off_bus_addr();
        cb->dest = PERI_BUS_BASE + GPIO_BASE + GPIO_PIN_CLEAR;
        cb->tx_len = 4;
        index++;
        cb->next_cb = ith_cb_bus_addr(index);
      }
      // Delay block
      cb = ith_cb_virt_addr(index);
      cb->tx_info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP | DMA_DEST_DREQ | DMA_PERIPHERAL_MAPPING(5);
      cb->src = ith_cb_bus_addr(0);  // Dummy data
      cb->dest = PERI_BUS_BASE + PWM_BASE + PWM_FIFO;
      cb->tx_len = 4;
      cb->next_cb = ith_cb_bus_addr((index + 1) % DMA_PWM_PERIOD);  // on the last cb, this will point to 0
    }
  }
}

void init_hw_clk() {
  // See Chanpter 6.3, BCM2835 ARM peripherals for controlling the hardware clock
  // Also check https://elinux.org/BCM2835_registers#CM for the register mapping

  // kill the clock if busy
  if (clk_reg->ctrl & CLK_CTL_BUSY) {
    do {
      clk_reg->ctrl = BCM_PASSWD | CLK_CTL_KILL;
    } while (clk_reg->ctrl & CLK_CTL_BUSY);
  }

  // Set clock source to plld
  clk_reg->ctrl = BCM_PASSWD | CLK_CTL_SRC(CLK_CTL_SRC_PLLD);
  usleep(10);

  // The original clock speed is 500MHZ, we divide it by 5 to get a 100MHZ clock
  clk_reg->div = BCM_PASSWD | CLK_DIV_DIVI(CLK_DIVI);
  usleep(10);

  // Enable the clock
  clk_reg->ctrl |= (BCM_PASSWD | CLK_CTL_ENAB);
}

void init_pwm() {
  // reset PWM
  pwm_reg->ctrl = 0;
  usleep(10);
  pwm_reg->status = -1;
  usleep(10);

  /*
   * set number of bits to transmit
   * e.g, if CLK_MICROS is 5, since we have set the frequency of the
   * hardware clock to 100 MHZ, then the time taken for `100 * CLK_MICROS` bits
   * is (500 / 100) = 5 us, this is how we control the DMA sampling rate
   */
  pwm_reg->range1 = 100 * CLK_MICROS;

  // enable PWM DMA, raise panic and dreq thresholds to 15
  pwm_reg->dma_cfg = PWM_DMAC_ENAB | PWM_DMAC_PANIC(15) | PWM_DMAC_DREQ(15);
  usleep(10);

  // clear PWM fifo
  pwm_reg->ctrl = PWM_CTL_CLRF1;
  usleep(10);

  // enable PWM channel 1 and use fifo
  pwm_reg->ctrl = PWM_CTL_USEF1 | PWM_CTL_MODE1 | PWM_CTL_PWEN1;
}

void dma_start() {
  // Reset the DMA channel
  dma_reg->cs = DMA_CHANNEL_ABORT;
  dma_reg->cs = 0;
  dma_reg->cs = DMA_CHANNEL_RESET;
  dma_reg->cb_addr = 0;

  dma_reg->cs = DMA_INTERRUPT_STATUS | DMA_END_FLAG;

  // Make cb_addr point to the first DMA control block and enable DMA transfer
  dma_reg->cb_addr = ith_cb_bus_addr(0);
  last_cb_start = ith_cb_bus_addr(0);
  dma_reg->cs = DMA_PRIORITY(8) | DMA_PANIC_PRIORITY(8) | DMA_DISDEBUG;
  dma_reg->cs |= DMA_WAIT_ON_WRITES | DMA_ACTIVE;
}

void dma_end() {
  // Shutdown DMA channel.
  dma_reg->cs |= DMA_CHANNEL_ABORT;
  usleep(100);
  dma_reg->cs &= ~DMA_ACTIVE;
  dma_reg->cs |= DMA_CHANNEL_RESET;
  usleep(100);

  // Release the memory used by DMA
  dma_free(dma_cbs);
  dma_free(dma_gpio_pin_on);
  dma_free(dma_gpio_pin_off);

  free(dma_cbs);
  free(dma_gpio_pin_on);
  free(dma_gpio_pin_off);
}

void dma_swap() {
  DMAControlBlock *cb;

  if (last_cb_start == ith_cb_bus_addr(0)) {
    cb = ith_cb_virt_addr(DMA_PWM_PERIOD - 1);
    cb->next_cb = ith_cb_bus_addr(DMA_PWM_PERIOD);
    last_cb_start = ith_cb_bus_addr(DMA_PWM_PERIOD);
  } else {
    cb = ith_cb_virt_addr(2*DMA_PWM_PERIOD - 1);
    cb->next_cb = ith_cb_bus_addr(0);
    last_cb_start = ith_cb_bus_addr(0);
  }
}

void respondToRequest() {
  printf("Press Ctrl-C to end the program!\n");
  int n;
  int width;
  char * line = NULL;
  char nl;
  size_t lineLen = 0;
  while (1) {
    if (getline(&line, &lineLen, servo_fifo)) {
      n = sscanf(line, "%d%c", &width, &nl);
      if (n != 2 || nl != '\n') {
        fprintf(stderr, "Bad input, enter only decimal digits\n%s\n", line);
      } else if (width < 1000 || width > 2000) {
        fprintf(stderr, "Pulse width out of range (1000 to 2000)\nInput was: %d\n", width);
      } else {
        setNewLocation(width);
        dma_swap();
      }
    } else {
      fprintf(stderr, "FIFO Input error\n");
    }
  }
}

void sigint_handler(int signo) {
  if (signo == SIGINT) {
    // Release the resources properly
    fprintf(stderr, "\nEnding Servo daemon!\n");
    dma_end();
    unlink(SERVO_FIFO);
    exit(0);
  }
}

int main() {
  signal(SIGINT, sigint_handler);

  PERI_PHYS_BASE = bcm_host_get_peripheral_address();

  uint8_t *dma_base_ptr = reinterpret_cast<uint8_t *>(map_peripheral(DMA_BASE, PAGE_SIZE));
  dma_reg = reinterpret_cast<DMACtrlReg *>(dma_base_ptr + DMA_CHANNEL * 0x100);

  pwm_reg = reinterpret_cast<PWMCtrlReg *>(map_peripheral(PWM_BASE, PWM_LEN));

  uint8_t *cm_base_ptr = reinterpret_cast<uint8_t *>(map_peripheral(CM_BASE, CM_LEN));
  clk_reg = reinterpret_cast<CLKCtrlReg *>(cm_base_ptr + CM_PWM);

  fprintf(stderr, "dma_alloc_buffers\n");
  dma_alloc_buffers();
  usleep(100);
  fprintf(stderr, "setting pin on/off masks\n");
  *reinterpret_cast<uint32_t *>(dma_gpio_pin_on->virtual_addr) = 1 << SERVO_PIN;
  *reinterpret_cast<uint32_t *>(dma_gpio_pin_off->virtual_addr) = 1 << SERVO_PIN;

  gpio_mode_reg = reinterpret_cast<uint32_t *>(map_peripheral(GPIO_BASE, 8));
  usleep(100);
  gpio_mode_reg[1] = 1 << ((SERVO_PIN % 10) * 3);  // set gpio pin to output
  munmap(reinterpret_cast<void *>(gpio_mode_reg), 8);  // once mode is set, this is no longer needed

  fprintf(stderr, "making dma control blocks\n");
  dma_init_cbs();
  usleep(100);

  init_hw_clk();
  usleep(100);

  init_pwm();
  usleep(100);

  dma_start();
  usleep(100);

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
  respondToRequest();

  // Should not reach here
  dma_end();
  return 0;
}
