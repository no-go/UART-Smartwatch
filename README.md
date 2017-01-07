# UART-Smartwatch

This branch is a bit different than the master. Firmware and circuit are
modified.

## The Firmware

Is full with my private mods.

## Circuit

The 3.3V regulator (BAT Pin) is used! Drive the display with more than 3.3V
reduce its life time. Drive all parts with 3.3V instead of 3.7V or 4.2V
ist better -> we need less mAh and LiPo lives longer.

![Circuit of the UART-Smartwatch](stuff/circuit.png)

[UART-Smartwatch Circuit PDF](https://raw.githubusercontent.com/no-go/UART-Smartwatch/powerCritical/stuff/UART-Smartwatch.pdf)
