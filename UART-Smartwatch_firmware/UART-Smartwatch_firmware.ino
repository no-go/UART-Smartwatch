/*
 * git clone https://github.com/adafruit/Adafruit_ASFcore
 * git clone https://github.com/adafruit/Adafruit_ZeroTimer.git
 */
#include <Arduino.h>
#include <Adafruit_ZeroTimer.h>
#include <Adafruit_GFX.h>
#include "Adafruit_SSD1331.h"
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "BluefruitConfig.h"

#define BUTTON1     11

#define OLED_DC      5
#define OLED_CS     12
#define OLED_RESET   6
#define VBATPIN     A7  // A7 = D9 !!

#define MESSAGEPOS     50
#define MEMOSTR_LIMIT 550
#define CHAR_TIME_REQUEST '~'
#define CHAR_TIME_RESPONSE '#'

// Color definitions
#define BLACK           0x0000
#define GREYBLUE        0b0010000100010000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

byte hours   = 0;
byte minutes = 0;
byte seconds = 0;
byte lastsecond = 0;

byte clkSize;

char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos   = MESSAGEPOS;
int  page         = 0;

struct Pix {
  byte x,y,lastX,lastY;
  void hopp() {
    lastX = x;
    lastY = y; 
  } 
};

Pix secPos;
Pix minPos;
Pix hourPos;

byte batLength = 60;

Adafruit_ZeroTimer zt3 = Adafruit_ZeroTimer(3);
Adafruit_SSD1331  oled = Adafruit_SSD1331(OLED_CS, OLED_DC, OLED_RESET);
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

void filler() {
  for (int i=0; i<MESSAGEPOS; ++i) {
    memoStr[i] = ' ';
  }
}

byte powerTick(int mv) {
  float quot = (4700-2700)/(batLength-3); // scale: 5100 -> batLength, 2710 -> 0
  return (mv-2700)/quot;  
}

int readVcc() {
  float mv = analogRead(VBATPIN);
  mv *= 2;
  mv *= 3.3;
  return powerTick(mv);
}

void showTime() {
  byte x = clkSize+17;
  byte y = clkSize+1;
  byte radius = clkSize;
  
  int hour = hours;
  if (hour>12) hour-=12;
  secPos.x = x + (radius-3)*cos(PI * ((float)seconds-15.0) / 30);
  secPos.y = y + (radius-3)*sin(PI * ((float)seconds-15.0) / 30);
  minPos.x = x + (radius-5)*cos(PI * ((float)minutes-15.0) / 30);
  minPos.y = y + (radius-5)*sin(PI * ((float)minutes-15.0) / 30);
  hourPos.x =x + (radius-10)*cos(PI * ((float)hour-3.0) / 6);
  hourPos.y =y + (radius-10)*sin(PI * ((float)hour-3.0) / 6);

  // remove old
  oled.drawLine(x, y, secPos.lastX, secPos.lastY, BLACK);
  oled.drawLine(x, y, minPos.lastX, minPos.lastY, BLACK);
  oled.drawLine(x, y, hourPos.lastX, hourPos.lastY, BLACK);

  // draw new ones
  oled.drawLine(x, y, secPos.x, secPos.y, YELLOW);
  oled.drawLine(x, y, minPos.x, minPos.y, GREEN);
  oled.drawLine(x, y, hourPos.x, hourPos.y, RED);
  secPos.hopp();
  minPos.hopp();
  hourPos.hopp();

  // dots
  for (byte i=0; i<12; ++i) {
    oled.drawPixel(x + (radius-3)*cos(PI * ((float)i) / 6), y + (radius-3)*sin(PI * ((float)i) / 6), WHITE);  
  }

  /*
  oled.setTextColor(CYAN, BLACK);
  oled.setCursor(x-5,y-radius+4);
  oled.print(12);
  oled.setCursor(x-2,y+radius-11);
  oled.print(6);
  oled.setCursor(x+radius-9,y-3);
  oled.print(3);
  oled.setCursor(x-radius+6,y-3);
  oled.print(9);
  */
  
  oled.setTextColor(WHITE, GREYBLUE);
  oled.setCursor(x-clkSize +3, 4 + 2 * radius);
  if(hours<10) oled.print('0');
  oled.print(hours);
  oled.print(':');
  if(minutes<10) oled.print('0');
  oled.print(minutes);
  oled.print(':');
  if(seconds<10) oled.print('0');
  oled.print(seconds);
}

void ticking() {
  seconds++; 
  if (seconds > 59) {
    minutes += seconds / 60;
    seconds  = seconds % 60;
  }
  if (minutes > 59) {
    hours  += minutes / 60;
    minutes = minutes % 60;
  }
  if (hours > 23) {
    hours = hours % 24;
  }
}

short green2red(int val, int maxi) {
  // 565
  short result = 0x0000;
  int redPart   = 0;
  int greenPart = 0;
  if (val > (maxi/2)) {
    greenPart = 63;
    redPart = 31 - 62 * ((float) val)/((float) maxi); // 31 = 0b11111
  } else {
    redPart = 31;
    greenPart = 127 * ((float) val)/((float) maxi); // 63 = 0b111111
  }
  result += redPart  <<11;
  result += greenPart<<5;
  return result;
}

void batteryBar() {
  byte vccVal = readVcc();
  //oled.fillRect(oled.width()-5, oled.height()  - batLength+2, 4, batLength-3, BLACK);
  oled.fillRect(oled.width()-5, oled.height()  - batLength+2, 4, batLength-3-vccVal, BLACK);
  for (int v=vccVal; v>0; --v) {
    oled.drawLine(
      oled.width()-5, oled.height()-v-1,
      oled.width()-2, oled.height()-v-1,
      green2red(v, batLength)
    );
  }
}

void Timer3Callback0(struct tc_module *const module_inst) {
  ticking();
}

char umlReplace(char inChar) {
  if (inChar == 159) {
    inChar = 224; // ß
  } else if (inChar == 164) {
    inChar = 132; // ä
  } else if (inChar == 182) {
    inChar = 148; // ö
  } else if (inChar == 188) {
    inChar = 129; // ü
  } else if (inChar == 132) {
    inChar = 142; // Ä
  } else if (inChar == 150) {
    inChar = 153; // Ö
  } else if (inChar == 156) {
    inChar = 154; // Ü
  } else if (inChar == 171) {
    inChar = 0xAE; // <<
  } else if (inChar == 187) {
    inChar = 0xAF; // >>
  }  
  return inChar;  
}

byte tob(char c) {
  return c - '0';
}

void cleanup() {
  oled.fillScreen(GREYBLUE);
  
  oled.fillCircle(clkSize+17, clkSize+1, clkSize, BLACK);
  oled.drawCircle(clkSize+17, clkSize+1, clkSize, WHITE);

  oled.drawPixel(oled.width()-4, oled.height() - batLength, WHITE);
  oled.drawPixel(oled.width()-3, oled.height() - batLength, WHITE);
  oled.drawRect(oled.width()-6, oled.height()  - batLength+1, 6, batLength-1, WHITE);  
  byte pos = oled.height() - powerTick(3000);
  oled.drawLine(oled.width()-9,  pos, oled.width()-6,  pos, WHITE);
  pos = oled.height() - powerTick(3500);
  oled.drawLine(oled.width()-7,  pos, oled.width()-6,  pos, WHITE);
  pos = oled.height() - powerTick(4000);
  oled.drawLine(oled.width()-9,  pos, oled.width()-6,  pos, WHITE);
  pos = oled.height() - powerTick(4500);
  oled.drawLine(oled.width()-7,  pos, oled.width()-6,  pos, WHITE);  
}

void setup() {
  oled.begin();
  oled.setTextSize(0);
  clkSize = oled.height()/2 -6;
  showTime();
  cleanup();
  filler();
  pinMode(BUTTON1, INPUT_PULLUP);
  ble.begin(false);
  ble.echo(false);
  ble.sendCommandCheckOK("AT+HWModeLED=BLEUART");
  ble.sendCommandCheckOK("AT+GAPDEVNAME=oLED Feather");
  ble.sendCommandCheckOK("ATE=0");
  ble.sendCommandCheckOK("AT+BAUDRATE=115200");
  ble.sendCommandCheckOK("AT+BLEPOWERLEVEL=4");
  ble.sendCommandCheckOK("ATZ");
  ble.setMode(BLUEFRUIT_MODE_DATA);
  ble.verbose(false);
  /********************* Timer #3, 16 bit, two PWM outs, period = 65535 */
  zt3.configure(TC_CLOCK_PRESCALER_DIV1024, // prescaler
                TC_COUNTER_SIZE_16BIT,   // bit width of timer/counter
                TC_WAVE_GENERATION_MATCH_PWM // frequency or PWM mode 
                );

  zt3.setPeriodMatch(48000, 1, 0); // 48MHz / 1024   ->   48 = 1ms
  zt3.setCallback(true, TC_CALLBACK_CC_CHANNEL0, Timer3Callback0);  // this one sets pin low
  zt3.enable(true);
}


void loop() {
  
  if (lastsecond != seconds) {
    showTime();
    batteryBar();
    lastsecond = seconds;   
  }

  if (ble.isConnected()) {
    while ( ble.available() ) {
      char inChar = (char) ble.read();
      if (inChar == 194) continue; // symbol before utf-8
      if (inChar == 195) continue; // symbol before utf-8
      if (inChar == '\n') {
        memoStr[memoStrPos] = '\0';
        page = 0;
        continue;
      }
      memoStr[memoStrPos] = umlReplace(inChar);
      memoStrPos++;
    }
  }

  if (digitalRead(BUTTON1) == LOW) {
    delay(300);  
    if (digitalRead(BUTTON1) == LOW) {    
      // "remove" old chars from buffer
      // print ignores everyting behind \0
      memoStr[MESSAGEPOS] = '\0';
      memoStrPos = MESSAGEPOS;
      ble.println( CHAR_TIME_REQUEST );
    }
  }

  /**
   * React, if there is a new message
   */
  if (memoStrPos > MESSAGEPOS && page == 0) {
    /**
     * Check at the message beginning for some special chars
     */
    if (memoStr[MESSAGEPOS] == CHAR_TIME_RESPONSE) {
      
      // extract the time -------------------------
      
      memoStr[MESSAGEPOS] = ' ';
      hours = tob(memoStr[MESSAGEPOS+1])*10 + tob(memoStr[MESSAGEPOS+2]);
      minutes = tob(memoStr[MESSAGEPOS+4])*10 + tob(memoStr[MESSAGEPOS+5]);
      seconds = tob(memoStr[MESSAGEPOS+7])*10 + tob(memoStr[MESSAGEPOS+8]);
    }
  }

  if (memoStrPos > MESSAGEPOS && page <= memoStrPos) {
    oled.setCursor(0, 0);
    oled.setTextColor(WHITE, BLACK);
    oled.print(&(memoStr[page]));
    oled.print(" ");
  }

  if (page == memoStrPos) {
    memoStr[MESSAGEPOS] = '\0';
    memoStrPos = MESSAGEPOS;
    cleanup();
  }
     
  page++;
}

