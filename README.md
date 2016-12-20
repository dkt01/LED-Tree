# LED-Tree
[![Documentation Status](https://codedocs.xyz/dkt01/LED-Tree.svg)](https://codedocs.xyz/dkt01/LED-Tree/)

[hackaday.io Project Page](https://hackaday.io/project/19038-led-tree-jenkins-build-monitor)

[<img src="https://www.youtube.com/yt/brand/media/image/YouTube-icon-full_color.png" width="30pt" alt="Demo Video">](https://youtu.be/l4QfoKM_p1U)
Demo video

Christmas tree desk ornament with ethernet control.  NeoPixelRing.h/cpp is a library that extends Adafruit's NeoPixel library to control an arbitrary set of NeoPixel rings.  Several patterns are supported and each ring is controlled as a unit.  Patterns are all time-based, so the appearance will be consistent even if loops run slowly.

LEDTree.ino is an example application where the Arduino listens for messages from a host computer over Ethernet and updates the ring patterns.

JenkinsBuildMonitor.py is an example application where a host computer queries a Jenkins server for job build status and sends the Arduino pattern update messages over Ethernet.

More complete documentation is available in the code documentation and on the Hackaday.io project page.

## Software Dependencies

The following packages are necessary to run this software:

+ [EtherCard](https://github.com/jcw/ethercard): Driver for ENC28J60 Ethernet controller on Arduino microcontrollers
+ [Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel): Driver for NeoPixels on Arduino microcontrollers
+ [SparkFun Arduino_Boards](https://github.com/sparkfun/Arduino_Boards): Support for Arduino Pro Micro in Arduino IDE
+ [Jenkins API](https://pypi.python.org/pypi/jenkinsapi): Python API to read Jenkins build status.
