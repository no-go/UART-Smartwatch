#define BUTTON1   10 // time+msg request
#define BUTTON2   13 // show time
#define LED_RED   11
#define LED_GREEN 12
#define LED_BLUE   5

#define MESSAGEPOS     50 // default:  30 = screen middle
#define MEMOSTR_LIMIT 550 // default: 730 = 700 char buffer

#define CHAR_TIME_REQUEST '~'
#define CHAR_TIME_RESPONSE '#'
#define CHAR_NOTIFY_HINT '%'

const int batLength = 60;

const int xHour[13] = {32,40,47,49,47,40,32,23,17,15,17,24,32};
const int yHour[13] = {6,8,14,23,32,38,40,38,31,23,14,8,6};
const int xMin[60]  = {32,34,36,38,41,42,44,46,48,49,50,51,52,53,53,53,53,53,52,51,50,49,48,46,44,42,41,38,36,34,32,30,28,26,23,21,20,18,16,15,14,13,12,11,11,11,11,11,12,13,14,15,16,18,20,22,23,26,28,30};
const int yMin[60]  = {2,2,2,3,4,5,6,7,9,11,12,14,17,19,21,23,25,27,29,32,34,35,37,39,40,41,42,43,44,44,44,44,44,43,42,41,40,39,37,35,33,32,29,27,25,23,21,19,17,14,12,11,9,7,6,5,4,3,2,2};

// ------------------------------------------------------

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "BluefruitConfig.h"

#define OLED_CS     A3
#define OLED_DC     A4
#define OLED_RESET  A5

// A7 = D9 !!!
#define VBATPIN     A7

Adafruit_SSD1306 oled(OLED_DC, OLED_RESET, OLED_CS);
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos   = MESSAGEPOS;
int  page         = 0;

byte COUNT        = 0;

byte hours = 0;
byte minutes = 0;
byte seconds = 0;
byte tick = 0;

bool useAnalogClock = false;

// analogWrite on feather M0 not working for A1..A5 :-(
int redValue   = 250;
int greenValue = 50;
int blueValue  = 50;

// Check it -------------------------
// 0 always on
// 1 = 100 ms of a second on
// 2 = 200 ms of a second on ..
// 9 = 900 ms of a second on
int delayValue = 8;

int readVcc() {
  float mv = analogRead(VBATPIN);
  mv *= 2;
  mv *= 3.3;
  return powerTick(mv);
}

byte powerTick(int mv) {
  float quot = (5100-2700)/(batLength-3); // scale: 5100 -> batLength, 2710 -> 0
  return (mv-2700)/quot;  
}

void wakeUpIcon() {
  oled.clearDisplay();
  oled.drawCircle(32, 23, 10, WHITE);
  oled.drawPixel(31, 20, WHITE);
  oled.drawPixel(35, 19, WHITE);
  oled.drawLine(29, 27, 35, 27, WHITE);
  oled.display();
  delay(500);
  oled.drawRect(29, 19, 3, 3, WHITE);
  oled.drawRect(35, 19, 3, 3, WHITE);
  oled.drawPixel(35, 26, WHITE);
  oled.display();  
  delay(500);
  tick += 10; 
}

void anaClock() {
  oled.drawCircle(32, 23, 23, WHITE);
  int hour = hours;
  if (hour>12) hour-=12;
  oled.drawLine(32,   23, xMin[seconds], yMin[seconds], WHITE);

  oled.drawLine(32,   23, xMin[minutes], yMin[minutes], WHITE);
  oled.drawLine(32,   23, xHour[hour],   yHour[hour], WHITE);
  oled.drawLine(32+1, 23, xHour[hour]+1, yHour[hour], WHITE);
  
  for (int i=0; i<12; ++i) {
    oled.drawPixel(xHour[i], yHour[i], WHITE);  
  }
  // 12 o'clock
  oled.drawPixel(30, 3, WHITE);
  oled.drawPixel(30, 4, WHITE);
  oled.drawPixel(30, 5, WHITE);
  oled.drawPixel(30, 6, WHITE);
  oled.drawPixel(32, 3, WHITE);
  oled.drawPixel(33, 3, WHITE);
  oled.drawPixel(33, 4, WHITE);
  oled.drawPixel(32, 5, WHITE);
  oled.drawPixel(33, 6, WHITE);
  oled.drawPixel(34, 6, WHITE);
  // 3 o'clock
  oled.drawPixel(49, 21, WHITE);
  oled.drawPixel(50, 21, WHITE);
  oled.drawPixel(50, 22, WHITE);
  oled.drawPixel(49, 23, WHITE);
  oled.drawPixel(50, 24, WHITE);
  oled.drawPixel(49, 25, WHITE);
  oled.drawPixel(50, 25, WHITE);
  // 6 o'clock
  oled.drawPixel(32, 41, WHITE);
  oled.drawPixel(31, 42, WHITE);
  oled.drawPixel(30, 43, WHITE);
  oled.drawPixel(31, 43, WHITE);
  oled.drawPixel(30, 44, WHITE);
  oled.drawPixel(31, 44, WHITE);
  oled.drawPixel(32, 44, WHITE);
  oled.drawPixel(31, 45, WHITE);
  // 9 o'clock
  oled.drawPixel(14, 21, WHITE);
  oled.drawPixel(13, 22, WHITE);
  oled.drawPixel(15, 22, WHITE);
  oled.drawPixel(14, 23, WHITE);
  oled.drawPixel(15, 23, WHITE);
  oled.drawPixel(14, 24, WHITE);
  oled.drawPixel(13, 25, WHITE);
}

void filler() {
  for (int i=0; i<MESSAGEPOS; ++i) {
    memoStr[i] = ' ';
  }
}

/**
 * the hard way to handle with german(?) UTF-8 stuff
 */
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

/**
 * make hours, minutes, seconds from "ticks" and
 * add it to the time (received from App)
 */
void ticking() {
  tick++;
  if (tick > 9) {
    seconds += tick/10;
  }
  
  if (tick > 9) {
    tick = tick % 10;
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
}

void digiClock() {
  oled.setTextSize(3);
  oled.setCursor(0, 12);
  if (hours<10) oled.print("0");
  oled.print(hours);

  oled.setTextSize(2);
  oled.setCursor(34, 15);
  oled.print(":");

  oled.setTextSize(3);
  oled.setCursor(45, 12);
  if (minutes<10) oled.print("0");
  oled.print(minutes); 

  oled.setTextSize(1);
  oled.setCursor(25, 37);
  if (seconds<10) oled.print("0");
  oled.print(seconds);
  oled.print(".");
  oled.print(tick);
}

void batteryIcon() {
  byte vccVal = readVcc();
  oled.drawPixel(oled.width()-4, oled.height() - batLength, WHITE);
  oled.drawPixel(oled.width()-3, oled.height() - batLength, WHITE);
  oled.drawRect(oled.width()-6, oled.height()  - batLength+1, 6, batLength-1, WHITE);  
  oled.fillRect(oled.width()-5, oled.height()  - vccVal   -1, 4,      vccVal, WHITE); 

  int ptick[5] = {50,42,37,33,20};
  int pos;
  for(int i=0;i<5;++i) {
    pos = oled.height() - powerTick(ptick[i]*100);
    oled.setCursor(oled.width()-30, pos);
    oled.print(ptick[i]/10.0, 1);
    oled.drawPixel(oled.width()-7,  pos, WHITE);
  }
}

byte tob(char c) {
  return c - '0';
}

void setup()   {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  filler();
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
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
      char inChar = (char) ble.read();
      if (inChar == 194) continue; // symbol before utf-8
      if (inChar == 195) continue; // symbol before utf-8
      if (inChar == '\n') {
        oled.ssd1306_command(SSD1306_DISPLAYON);
        memoStr[memoStrPos] = '\0';
        page = 0;
        continue;
      }
      memoStr[memoStrPos] = umlReplace(inChar);
      memoStrPos++;
    }
  }




  if (digitalRead(BUTTON2) == LOW) {
    delay(300); 
    tick += 3; 
    if (digitalRead(BUTTON2) == LOW) {
      
      COUNT = 0;
      digitalWrite(LED_RED,   LOW);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE,  LOW);
      
      oled.ssd1306_command(SSD1306_DISPLAYON);

      for (int j=0; j<40; ++j) { // 4sec
        oled.clearDisplay();
        ticking();
        if (digitalRead(BUTTON1) == LOW) {
          useAnalogClock = (useAnalogClock? false : true); // flipflop
        }
        if (useAnalogClock) {
          anaClock();
        } else {
          digiClock();
        }
        batteryIcon();
        oled.display();
        delay(90); // 10ms in vcc mesurement
      }
      oled.ssd1306_command(SSD1306_DISPLAYOFF);     
    }
  }
  
  if (digitalRead(BUTTON1) == LOW) {
    delay(300);  
    tick += 3;
    if (digitalRead(BUTTON1) == LOW) {
      
      oled.ssd1306_command(SSD1306_DISPLAYON);
      wakeUpIcon();
      oled.ssd1306_command(SSD1306_DISPLAYOFF);
      
      COUNT = 0;
      digitalWrite(LED_RED,   LOW);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE,  LOW);
              
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
    } else if (memoStr[MESSAGEPOS] == CHAR_NOTIFY_HINT) {
      
      // there is a new message (or a message is deleted)
      
      COUNT = (unsigned char) memoStr[MESSAGEPOS+1];
      if (COUNT > 0) {
        redValue   = (unsigned char) memoStr[MESSAGEPOS+2] - 'A';
        greenValue = (unsigned char) memoStr[MESSAGEPOS+3] - 'A';
        blueValue  = (unsigned char) memoStr[MESSAGEPOS+4] - 'A';
        delayValue = (unsigned char) memoStr[MESSAGEPOS+5] - 'A';
      } else {
        memoStr[MESSAGEPOS] = '\0';
      }
      page = memoStrPos; // makes a clear and display off        
     }
  }

  /**
   * Scrolling message through display
   */
  if (memoStrPos > MESSAGEPOS && page <= memoStrPos) {
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.print(&(memoStr[page]));
    oled.display();
  }

  /// Safe power and switch display off, if message is at the end
  if (page == memoStrPos) {
    oled.ssd1306_command(SSD1306_DISPLAYOFF);
    // "remove" old chars from buffer
    // print ignores everyting behind \0
    memoStr[MESSAGEPOS] = '\0';
    memoStrPos = MESSAGEPOS;
  }

  if (COUNT > 0) {
    if (delayValue==0) {
        analogWrite(LED_RED,   redValue);
        analogWrite(LED_GREEN, greenValue);
        analogWrite(LED_BLUE,  blueValue);
    } else {
      if (tick <= delayValue) {
        analogWrite(LED_RED,   redValue);
        analogWrite(LED_GREEN, greenValue);
        analogWrite(LED_BLUE,  blueValue);
      } else {
        analogWrite(LED_RED,   0);      
        analogWrite(LED_GREEN, 0);      
        analogWrite(LED_BLUE,  0);      
      }
    }
  } else {
    analogWrite(LED_RED,   0);      
    analogWrite(LED_GREEN, 0);      
    analogWrite(LED_BLUE,  0);    
  }

  page++;
  ticking();
}

