# UART-Smartwatch

This branch is a bit different than the master. Firmware and circuit are
modified.

## The Firmware

Is full with my private mods.

### old !!!

- Button 1: time and message request
- Poti: scroll through messages
- Button 2: shows time and power level for 4 seconds (seconds included)
- Button 2 (more than 4sec pressed): start Dino Game
- Dino game: Button 2 for jump, 3 lives, 6 speed levels, stores Highscore in EEPROM
- Time: Button 2 to switch between digital and analoge Clock!

## Circuit

### old !!
The 3.3V regulator (BAT Pin) is used! Drive the display with more than 3.3V
reduce its life time. Drive all parts with 3.3V instead of 3.7V or 4.2V
ist better -> we need less mAh and LiPo lives longer.

