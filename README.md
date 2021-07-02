# Servo daemon

This project implements a servo daemon, servod.  Currently, this daemon controls up to 10 servos on one DMA channel of a Raspberry Pi.  The design allows for multiple DMA channels to be used, but the use of multiple DMA channels causes the pulse timing to be unstable.  Soon, I will either fix this problem or inhibit the use of multple channels in one servod session.

This daemon will only run successfully on a Raspberry Pi.  It directly interfaces to the Broadcom chip on a Pi.  I have tested the code on a Pi 0 and Pi 3.  I've run it on a Pi 4, but haven't verified its operation.

The design parameters in this implementation assume that the servos being controlled require pulses with a 20 msec period and that the pulses are high from 1 to 2 msec.

Control of the servo is through a FIFO pipe located at /dev/servo_fifo.

## To compile
```
$ git clone https://github.com/mbroihier/servod
$ cd servod
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## To Use
In the build directory, define a servo or set of servos (up to 10, one per line):
```
$ echo "18, 6" >./servoDefinitionFile.txt
```
This line creates a file that defines one servo (0).  The 18 refers to the GPIO pin (using the BCM definition).  This will be the pin the servo signal will be sent on.  The 6 refers to an unused DMA channel that will be programmed by the daemon to control the pulse generation.  Each DMA channel can control up to 10 servos.  Subsequent lines in the file define additional servos that must be on different GPIO pins, but may be on the same DMA channel (up to 10).  Multiple DMA channels may be used, but are not recommended.

I have found DMA channels 5 and 6 to be free on my Pi's which most often only have the "lite" versions of Raspberry Pi OS installed.

There is an example servoDefinitionFile.txt file in the servod/ directory.  This file was used during my testing.

## To run (from build directory)
```
$ sudo ./servod
```

To move a servo, write a string to /dev/servo_fifo that consists of two integers separated by a comma and optional space.  The first integer refers to the servo number (zero based).  The second integer should range from 1000 to 2000.  This integer is the the pulse width in microseconds sent to the servo every 20 msec.
```
$ echo "0, 1000" > /dev/servo_fifo
```

To run as a service (assumes intallation and build at /home/pi/servod):
```
$ sudo cp -p servod.service /lib/systemd/system/
$ sudo systemctl enable servod
$ sudo systemctl start servod
```

## Other useful commands
To shutdown:
```
$ echo "0, 0" >/dev/servo_fifo
```

To enable debug (assuming no servod is running):
```
$ sudo ./servod -d
```
Control C will terminate servod in debug mode.

## References

Design information is located [here](https://gist.github.com/mbroihier/f670a765bedfdbfc79fc3504c1ba0460).  This can be used to adjust the timing for other servo specifications.

[pigpio](http://abyz.me.uk/rpi/pigpio/index.html)

[ServoBlaster](https://github.com/richardghirst/PiBits/tree/master/ServoBlaster)

[this stackoverflow post](https://stackoverflow.com/questions/50427275/raspberry-how-does-the-pwm-via-dma-work)

[Wallacoloo's example](https://github.com/Wallacoloo/Raspberry-Pi-DMA-Example)

[hezller's demo](https://github.com/hzeller/rpi-gpio-dma-demo)

## Notes

mailbox.cc is not my code and has a Copyright issued by Broadcom Europe Ltd.  Please read its prologue for proper use and distribution.
