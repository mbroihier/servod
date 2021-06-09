# Servo daemon

When completed, this project will implement a servo daemon, servod.  Currently, this is a prototype that only works with one servo.  It was built to verify the major design features that will be used to implement the daemon.

This project will only run on a Raspberry Pi.

This version outputs the PWM servo control signal to physical pin 12 (BCM pin 18) on the 40 pin connector on PI 0, 3, and 4.  The PWM pulse width may be adjusted from 1000 usec to 2000 usec with a resolution of 1 usec.  The PWM period is 20 msec.

Control of the servo is through a FIFO pipe (/dev/servo_fifo).

To compile:
```
$ git clone https://github.com/mbroihier/servod
$ cd servod
$ mkdir build
$ cd build
$ cmake ..
$ make
```
Define a servo or set of servos (one per line):
```
$ echo "18, 6" >./servoDefinitionFile.txt
```
This line creates a file that defines the first servo (0).  The 18 refers to the GPIO pin (using the BCM definition).  This will be the pin the servo signal will be sent on.  The 6 refers to an unused DMA channel that will be used to control the pulse generation.  Each DMA channel can control up to 10 servos.  Subsequent lines in the file define additional servos that must be on different GPIO pins, but may be on the same DMA channel (up to 10).  Multiple DMA channels may be used.

To run:
```
$ sudo ./servod
```
From another terminal, write a string that consists of two integers separated by a comma and space.  The first integer refers to the servo number (zero based).  The second integer should range from 1000 to 2000.  This integer is the the pulse width in microseconds sent to the servo every 20 msec.
```
$ echo "0, 1000" > /dev/servo_fifo
```


## References

[pigpio](http://abyz.me.uk/rpi/pigpio/index.html)
[ServoBlaster](https://github.com/richardghirst/PiBits/tree/master/ServoBlaster)
[this stackoverflow post](https://stackoverflow.com/questions/50427275/raspberry-how-does-the-pwm-via-dma-work)
[Wallacoloo's example](https://github.com/Wallacoloo/Raspberry-Pi-DMA-Example)
[hezller's demo](https://github.com/hzeller/rpi-gpio-dma-demo)

