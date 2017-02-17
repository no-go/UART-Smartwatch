# UART-Smartwatch (mini)

![logo](UART-Smartwatch_App/app/src/main/res/drawable/icon.png)

Android App, Firmware and Circuit for a DIY Smartwatch working with Bluetooth Low Energy (4.x)

This branch is a bit different than the master. Firmware and circuit are modified for a low cost version.

## App

The UART Smartwatch (mini) App is a smaller and easier version of the non-mini App.

[UART-Smartwatch (mini) APK](https://raw.githubusercontent.com/no-go/UART-Smartwatch/gplay/UART-Smartwatch_App/app/app-release.apk) or get the non mini App from [f-Droid](http://f-droid.org)

Support me: <a href="https://flattr.com/thing/5195407" target="_blank">![Flattr This](flattr.png)</a>

## The Firmware

Is full with my private mods.

[UART-Smartwatch / Arduino IDE](https://raw.githubusercontent.com/no-go/UART-Smartwatch/gplay/UART-Smartwatch_firmware/UART-Smartwatch_firmware.ino)


## Features

- blinking LED, if a new notification is present
- 315 chars notification buffer
- Button 1: get a fresh time and the notification buffer from your smart phone
- Button 2: display the time and power level for 4 seconds
- Button 2 (press for more than 4 seconds): switch to a different time mode
- runs for more than 10h (first and second time mode)

### Time Modes

- Digital (4sec)
- Analoge (4sec)
- Digital for ever

## Circuit

![circuit for the UART Smartwatch](circuit.png)

**Attention!** You need a serial USB device (less than US 4$) to store the firmware on the ProMini chip. You need a small circuit (less tan US 1$ on aliexpress) to load the Lithium polimere akku (Lipo).

## License

I publish everything under the free BSD-3 License.

## Privacy policy

Google Play requires me to disclose this App will take access to your position and notifications. The your position and notifications are not send to any one. The Bluetooth Low Energy needs access to position data - I do not know, why. Your notifications are only send to your bluetooth device / UART Smartwatch and this data may be sniffed by hackers.
