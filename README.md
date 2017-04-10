# UART-Smartwatch (mini)

![logo](UART-Smartwatch_App/app/src/main/res/drawable/icon.png)

Android App, Firmware and Circuit for a DIY Smartwatch working with Bluetooth Low Energy (4.x)

This branch is a bit different than the master. Firmware and circuit are modified for a low cost version.

Get a PDF (german) about my [UART Smartwatch Project](https://github.com/no-go/UART-Smartwatch/tree/gplay) from here:
<a href="https://github.com/no-go/Android-nRF-UART/raw/master/Slides/Slides.pdf" target="_blank">
<img src="https://raw.githubusercontent.com/no-go/UART-Smartwatch/gplay/img/Adobe_PDF_file_icon.png" alt="Get Slides" /></a> or as [Website](https://github.com/no-go/Android-nRF-UART/tree/master/Slides)

## App

The UART Smartwatch (mini) App is a smaller and easier version of the non-mini App.

<a href="https://f-droid.org/repository/browse/?fdid=click.dummer.UartSmartwatch" target="_blank">
<img src="https://f-droid.org/badge/get-it-on.png" alt="Get it on F-Droid" height="90"/></a>
<a href="https://play.google.com/store/apps/details?id=click.dummer.UartSmartwatch" target="_blank">
<img src="https://play.google.com/intl/en_us/badges/images/generic/en-play-badge.png" alt="Get it on Google Play" height="90"/></a>

You can optionaly get a signed APK from here: [UART-Smartwatch (mini) APK](https://raw.githubusercontent.com/no-go/UART-Smartwatch/gplay/UART-Smartwatch_App/app/app-release.apk)

Support me: <a href="https://flattr.com/thing/5195407" target="_blank">![Flattr This](img/flattr.png)</a>

## The Firmware

[UART-Smartwatch / Arduino IDE](https://raw.githubusercontent.com/no-go/UART-Smartwatch/gplay/UART-Smartwatch_firmware/UART-Smartwatch_firmware.ino)


## Features

- blinking LED, if a new notification is present
- 250 chars notification buffer
- Button 1: get a fresh time and the notification buffer from your smart phone
- Button 2: display the time and power level for 5 seconds
- Button 2 (press for more than 5 seconds): switch to a different time mode
- runs for about 6h
- Posibility to compile with DINO GAME !!! (Press both buttons)

### Time Modes

- Digital (5sec)
- Analoge (5sec)
- PitBoy Clock design (5sec)
- Digital for ever

## Circuit

![circuit for the UART Smartwatch](img/circuit.png)

**Attention!** You need a serial USB device (less than US 4$) to store the firmware on the ProMini chip. You need a small circuit (less tan US 1$ on aliexpress) to load the Lithium polimere akku (Lipo).

## License

I publish everything under the free BSD-3 License.

## Privacy policy

Google Play requires me to disclose this App will take access to your position and notifications. The your position and notifications are not send to any one. The Bluetooth Low Energy needs access to position data - I do not know, why. Your notifications are only send to your bluetooth device / UART Smartwatch and this data may be sniffed by hackers.
