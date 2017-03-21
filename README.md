# UART-Smartwatch

![logo](UART-Smartwatch_App/app/src/main/res/drawable/icon.png)

Android App, Firmware and Circuit for a DIY Smartwatch working with Bluetooth Low Energy (4.x)

This branch is a bit different than the master. Firmware and circuit are
modified for a low cost version.

## App

Perferences have a button to use HM10 bluetooth module.

[UART-Smartwatch APK](https://raw.githubusercontent.com/no-go/UART-Smartwatch/lowCost/UART-Smartwatch_App/app/app-release.apk) or get the App from [f-Droid](http://f-droid.org)

Support me: <a href="https://flattr.com/thing/5195407" target="_blank">![Flattr This](flattr.png)</a>

## The Firmware

This is a **deep sleep** version. The timer2 is set to 1sec and the time is always shown.
Every second Display and powerbar gets an update. It runs for ..?

[UART-Smartwatch / Arduino IDE](https://raw.githubusercontent.com/no-go/UART-Smartwatch/lowCost/UART-Smartwatch_firmware/UART-Smartwatch_firmware.ino)


## Features

- Button 1: time and message request (sends a tilde char)
- Button 2: switch time mode and wakeup on sleep
- receive RGB Notification and message count
- uses MSTimer2 Library
- uses INT1 on Pin3 to wake up the CPU

### Time Modes

- Digital
- Analoge

## Count, RGB, Blink, Package-Name

Send a % followed by a byte (char 0 = 0 new messages, char '0' = 48 new messages).
The 3 next Bytes should be 3 ASCII charaters. 'A' is 0 and 'z' is 57. This range is
mapped from 27-255 to set a RGB value. The last byte is a char from A-I. You can
use it as blink delay (not implemented yet).

%count,R,G,B,blink(A-I is 0-9, 0 is always on),chars (Package-Name)

## Circuit

Attention! The circuit is modified !

- Button2 was on A1: I have to set it to Pin3 (= INT1 to wakeup)
- the blue LED was on Pin3: I have to set it on Pin5 (PWM possible)
- PIN_CS does not need PWM. It was on Pin5: Now PIN_CS is on Pin4

![give it a try](circuit.png)
