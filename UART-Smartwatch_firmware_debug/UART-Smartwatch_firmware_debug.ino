#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

#define OLED_DC      5
#define OLED_CS     12
#define OLED_RESET   6
#define BUTTON1   11
#define CHAR_TIME_REQUEST '~'

Adafruit_SSD1306 oled(OLED_DC, OLED_RESET, OLED_CS);
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

String memoStr = "";
int page = 0;

void setup()   {
  pinMode(BUTTON1, INPUT);
  digitalWrite(BUTTON1, HIGH);

  oled.begin(SSD1306_SWITCHCAPVCC);
  oled.clearDisplay();
  oled.setTextSize(1); // 8 line with 21 chars
  oled.setTextColor(WHITE);
  oled.setCursor(0,0);

  ble.begin(false);
  ble.echo(false);
  ble.sendCommandCheckOK("AT+HWModeLED=BLEUART");
  ble.setMode(BLUEFRUIT_MODE_DATA);
}

void loop() {
  delay(100);  
  
  if (ble.isConnected()) {
    while ( ble.available() ) {
      if(page ==0) {
        oled.clearDisplay();
        oled.setCursor(0, 0);
      }
      char inChar = (char)ble.read();    
      if (inChar == 195) continue; // symbol before utf-8
      if (inChar == '\n') {
        page = 0;      
        continue;
      }
      page++;
      int cint = inChar;
      oled.print(inChar);
      oled.print(cint);
      oled.print(" ");
      oled.display();
    }
  }
  
  if (digitalRead(BUTTON1) == LOW) {
    delay(500);  
    if (digitalRead(BUTTON1) == LOW) {  
      ble.println( CHAR_TIME_REQUEST );
    }
  }
}

