
#define BUTTON1    3

#define MESSAGEPOS    120
#define MEMOSTR_LIMIT 720

#define CHAR_TIME_REQUEST '~'
#define CHAR_TIME_RESPONSE '#'

const int batLength = 60;

// ------------------------------------------------------

const int xHour[13] = {32,40,47,49,47,40,32,23,17,15,17,24,32};
const int yHour[13] = {6,8,14,23,32,38,40,38,31,23,14,8,6};
const int xMin[60]  = {32,34,36,38,41,42,44,46,48,49,50,51,52,53,53,53,53,53,52,51,50,49,48,46,44,42,41,38,36,34,32,30,28,26,23,21,20,18,16,15,14,13,12,11,11,11,11,11,12,13,14,15,16,18,20,22,23,26,28,30};
const int yMin[60]  = {2,2,2,3,4,5,6,7,9,11,12,14,17,19,21,23,25,27,29,32,34,35,37,39,40,41,42,43,44,44,44,44,44,43,42,41,40,39,37,35,33,32,29,27,25,23,21,19,17,14,12,11,9,7,6,5,4,3,2,2};

#define sclk   13
#define mosi   11
#define cs     10
#define rst    9
#define dc     8

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#include <avr/power.h>


// Option 1: use any pins but a little slower
//Adafruit_SSD1331 oled = Adafruit_SSD1331(cs, dc, mosi, sclk, rst);  

// Option 2: must use the hardware SPI pins 
// (for UNO thats sclk = 13 and sid = 11 ???) and pin 10 must be 
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
// (for Spark Core, cs = A2, dc = D3, rst = D2)
Adafruit_SSD1331 oled = Adafruit_SSD1331(cs, dc, rst); 


/*

// Scroll
#define ACTIVATESCROLL              0x2F
#define DEACTIVATESCROLL            0x2E
#define SETVERTICALSCROLLAREA       0xA3
#define RIGHTHORIZONTALSCROLL       0x26
#define LEFT_HORIZONTALSCROLL       0x27
#define VERTICALRIGHTHORIZONTALSCROLL 0x29
#define VERTICALLEFTHORIZONTALSCROLL  0x2A

void scrollStop() {
  oled.writeCommand(DEACTIVATESCROLL);
}

void scrollUp() {
  scrollStop();
  oled.writeCommand(RIGHTHORIZONTALSCROLL);
  oled.writeCommand(0x00);
  oled.writeCommand(0x00);  
  oled.writeCommand(0x40);
  oled.writeCommand(0x01);
  oled.writeCommand(0x02);
  oled.writeCommand(ACTIVATESCROLL);
}

*/

char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos   = MESSAGEPOS;
int  page         = 0;

byte hours = 0;
byte minutes = 0;
byte seconds = 0;
byte tick = 0;

bool usingBATpin;

byte readVcc() {
  int result;
  power_adc_enable();
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(10); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
  result = ADCL; 
  result |= ADCH<<8; 
  result = 1126400L / result;
  power_adc_disable();
  return powerTick(result);
}

byte powerTick(int mv) {
  float quot = (5100-2700)/batLength; // scale: 5100 -> batLength, 2710 -> 0
  return (mv-2700)/quot;  
}

void anaClock() {
  oled.drawCircle(32, 23, 23, WHITE);
  int hour = hours;
  if (hour>12) hour-=12;
  if (seconds == 0) {
    oled.drawLine(32, 23, xMin[59], yMin[59], BLACK);
  } else {
    oled.drawLine(32, 23, xMin[seconds-1], yMin[seconds-1], BLACK);    
  }
  
  oled.drawLine(32,   23, xMin[seconds], yMin[seconds], RED);

  oled.drawLine(32,   23, xMin[minutes], yMin[minutes], CYAN);
  oled.drawLine(32,   23, xHour[hour],   yHour[hour], YELLOW);
  
  for (int i=0; i<12; ++i) {
    oled.drawPixel(xHour[i], yHour[i], MAGENTA);  
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

  oled.setCursor(16, 50);
  oled.setTextSize(1);
  oled.setTextColor(YELLOW, BLACK);
  oled.print(hours);
  oled.print(":");
  if (minutes<10) oled.print("0");
  oled.print(minutes);
}

void filler() {
  for (int i=0; i<MESSAGEPOS; ++i) {
    memoStr[i] = ' ';
  }
}

void setup(void) {
  pinMode(BUTTON1, INPUT);
  
  digitalWrite(BUTTON1, HIGH);
  Serial.begin(9600);

  power_timer1_disable();
  power_timer2_disable();
  power_adc_disable();
  power_twi_disable();
  
  oled.begin();
  //scrollUp();
  filler();
}

char umlReplace(char inChar) {
  if (inChar == -97) {
    inChar = 224; // ß
  } else if (inChar == -92) {
    inChar = 132; // ä
  } else if (inChar == -74) {
    inChar = 148; // ö
  } else if (inChar == -68) {
    inChar = 129; // ü
  } else if (inChar == -124) {
    inChar = 142; // Ä
  } else if (inChar == -106) {
    inChar = 153; // Ö
  } else if (inChar == -100) {
    inChar = 154; // Ü
  } else if (inChar == -85) {
    inChar = 0xAE; // <<
  } else if (inChar == -69) {
    inChar = 0xAF; // >>
  }
  return inChar;  
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();    
    if (inChar == -61) continue; // symbol before utf-8
    if (inChar == -62) continue; // other symbol before utf-8
    if (inChar == '\n') {
      
      oled.writeCommand(SSD1331_CMD_DISPLAYON);
      oled.fillScreen(oled.Color565(128, 128, 255));
      
      memoStr[memoStrPos] = '\0';
      page = 0;
      continue;
    }
    memoStr[memoStrPos] = umlReplace(inChar);
    memoStrPos++;
  }
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

void batteryIcon() {
  byte vccVal = readVcc();
  oled.drawPixel(oled.width()-4, oled.height() - batLength -1, WHITE);
  oled.drawPixel(oled.width()-3, oled.height() - batLength -1, WHITE);
  oled.fillRect(oled.width()-6, oled.height()  -vccVal,    6,    vccVal, GREEN); 
  oled.drawRect(oled.width()-6, oled.height()  -batLength, 6, batLength, WHITE);

  int ptick[5] = {50,42,37,33,20};
  int pos;
  for(int i=0;i<5;++i) {
    pos = oled.height() - powerTick(ptick[i]*100);
    oled.setCursor(oled.width()-30, pos);
    oled.setTextSize(0);
    oled.setTextColor(BLUE, BLACK);
    oled.print(ptick[i]/10.0, 1);
    oled.drawPixel(oled.width()-7,  pos, WHITE);
  }
}

byte tob(char c) {
  return c - '0';
}

void loop() {
  delay(90);

  if (digitalRead(BUTTON1) == LOW) {
    delay(300); 
    tick += 3; 
    if (digitalRead(BUTTON1) == LOW) {
      
      oled.writeCommand(SSD1331_CMD_DISPLAYON);
      oled.fillScreen(BLACK);

      batteryIcon();
      
      for (int j=0; j<40; ++j) { // 4sec
        ticking();
        anaClock();
        delay(100);
      }
      
      oled.writeCommand(SSD1331_CMD_DISPLAYOFF);
      
      if (digitalRead(BUTTON1) == LOW) {
        memoStr[MESSAGEPOS] = '\0';
        memoStrPos = MESSAGEPOS;
        Serial.println( CHAR_TIME_REQUEST );
      }
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

  /**
   * Scrolling message through display
   */
  if (memoStrPos > MESSAGEPOS && page <= memoStrPos) {
    oled.setCursor(0, 0);
    oled.setTextSize(0);
    oled.setTextColor(oled.Color565(192, 192, 255), oled.Color565(0, 0, 48));
    oled.print(&(memoStr[page]));
    oled.print(" ");
  }

  /// Safe power and switch display off, if message is at the end
  if (page == memoStrPos) {
    
    oled.writeCommand(SSD1331_CMD_DISPLAYOFF);
    
    // "remove" old chars from buffer
    // print ignores everyting behind \0
    memoStr[MESSAGEPOS] = '\0';
    memoStrPos = MESSAGEPOS;
  }
   
  page += 15;
  if (page > memoStrPos) page = memoStrPos;
  
  ticking();
}

