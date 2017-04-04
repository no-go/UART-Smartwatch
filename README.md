# UART-Smartwatch

This branch is a bit different than the master. Firmware and circuit are
modified for a low cost version and long live via sleep mode

## App

Perferences have a button to use HM10 bluetooth module.

[UART-Smartwatch APK](https://raw.githubusercontent.com/no-go/UART-Smartwatch/lowCost/UART-Smartwatch_App/app/app-release.apk) or get the App from [f-Droid](http://f-droid.org)

## The Firmware

This is a **deep sleep** version. The timer2 is set to 1sec and the time is always shown.
Every second Display and powerbar gets an update. It runs for more than 8h (still testing)?

## Features

- Button 1: time and message request (sends a tilde char)
- Button 2: switch (analog/digital) and wakeup on sleep
- uses MSTimer2 Library
- uses INT1 on Pin3 to wake up the CPU
- LED: just notify, if the clock get a valid(?) time response

## Circuit

Attention! The circuit is massively modified !

![give it a try](circuit.png)
