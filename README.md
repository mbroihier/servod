# Servo daemon

When completed, this project will implement a servo daemon, servod.  Currently, this is an untested prototype that appears to stabily control up to 10 servos on one DMA channel of a Raspberry Pi.  The design allows for multiple DMA channels to be used, but initial testing with multiple DMA channels seems to indicate that the signals are not stable.  My current theory is that the gitter is caused by the fact that nothing is synchronizing the DMA channels.  My belief is that the control blocks controlling the signal timing are not clocked consistently at the same frequency due to how the PWM FIFO is shared between DMA channels.

This daemon will only run on a Raspberry Pi.

The design parameters in this implementation assume that the servos being controlled require pulses with a 20 msec period and that the pulses are high from 1 to 2 msec.

Control of the servo is through a FIFO pipe located at /dev/servo_fifo.

To compile:
```
$ git clone https://github.com/mbroihier/servod
$ cd servod
$ mkdir build
$ cd build
$ cmake ..
$ make
```
Define a servo or set of servos (up to 10, one per line):
```
$ echo "18, 6" >./servoDefinitionFile.txt
```
This line creates a file that defines one servo (0).  The 18 refers to the GPIO pin (using the BCM definition).  This will be the pin the servo signal will be sent on.  The 6 refers to an unused DMA channel that will be programmed by the daemon to control the pulse generation.  Each DMA channel can control up to 10 servos.  Subsequent lines in the file define additional servos that must be on different GPIO pins, but may be on the same DMA channel (up to 10).  Multiple DMA channels may be used, but are not currently recommended.

To run:
```
$ sudo ./servod &
```
Write a string to /dev/servo_fifo that consists of two integers separated by a comma and space.  The first integer refers to the servo number (zero based).  The second integer should range from 1000 to 2000.  This integer is the the pulse width in microseconds sent to the servo every 20 msec.
```
$ echo "0, 1000" > /dev/servo_fifo
```

Design information is located [here](https://gist.github.com/mbroihier/f670a765bedfdbfc79fc3504c1ba0460).

## References

[pigpio](http://abyz.me.uk/rpi/pigpio/index.html)

[ServoBlaster](https://github.com/richardghirst/PiBits/tree/master/ServoBlaster)

[this stackoverflow post](https://stackoverflow.com/questions/50427275/raspberry-how-does-the-pwm-via-dma-work)

[Wallacoloo's example](https://github.com/Wallacoloo/Raspberry-Pi-DMA-Example)

[hezller's demo](https://github.com/hzeller/rpi-gpio-dma-demo)

