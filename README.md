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

To run:
```
$ sudo ./servod
```
From another terminal or an application writing to the pipe, write a string of digits(between 1000 and 2000):
```
$ echo "1000" > /dev/servo_fifo
```


## References

[pigpio](http://abyz.me.uk/rpi/pigpio/index.html)
[ServoBlaster](https://github.com/richardghirst/PiBits/tree/master/ServoBlaster)
[this stackoverflow post](https://stackoverflow.com/questions/50427275/raspberry-how-does-the-pwm-via-dma-work)
[Wallacoloo's example](https://github.com/Wallacoloo/Raspberry-Pi-DMA-Example)
[hezller's demo](https://github.com/hzeller/rpi-gpio-dma-demo)

