#include <MsTimer2.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/sleep.h>

// ---------------------------- Configure YOUR CONNECTIONS !! -------

#define BUTTON1     2
#define BUTTON2     3

#define LED_RED    10

// OLED (A4 -> DIN, A5 ->SCK)
#define PIN_RESET  6

#define PIN_CS 4
#define PIN_DC 8

const int scrollSpeed =  80;
#define SECtoSLEEP       30
#define BLE_UART_SPEED   9600 // or try 115200
#define TIME_PITCH       987    // 1000ms = 1 sec (realy ?)
#define MAX_POWER        3380   // or try 3340, 4300, 5000
#define MIN_POWER        3200   // or try 2740, 3200, 3600

const int batLength   = 64;

// ------------------------------------------------------

#define CHAR_TIME_REQUEST     '~' //unused: idea was to make a deep sleep and get only the time on demand
#define CHAR_MSG_REQUEST      '~' //new
#define CHAR_TIME_RESPONSE    '#' //#HH:mm:ss
#define CHAR_NOTIFY_HINT      '%' //%[byte]

#define MEMOSTR_LIMIT 270

char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos = 0;
int  cStart     = 0;
int  cEnd       = 0;
char buffMem;

byte hours   = 42;
byte minutes = 42;
byte seconds = 42;

byte clockMode = 0;
int countToSleep = 0;


struct OledWrapper {

    Adafruit_SSD1306 * _oled;
    
    OledWrapper() {
       _oled = new Adafruit_SSD1306(PIN_RESET);
    }
    
    void begin() {
      _oled->begin();
      clear();
      _oled->setTextSize(1); // 8 line with 21 chars
      _oled->setTextColor(WHITE);
      setCursor(0,0);
    }
    
    void black(const char c[]) {
      _oled->setTextColor(BLACK);
      _oled->print(c);
      _oled->setTextColor(WHITE);
    } 
    
    void display() {
      _oled->display();
    }

    int height() {
      return _oled->height();
    }
    
    int width() {
      return _oled->width();
    }

    void line(const int & x, const int & y, const int & xx, const int & yy) {
      _oled->drawLine(x,y,xx,yy, WHITE);
    }
    
    void pixel(const int & x, const int & y) {
      _oled->drawPixel(x,y, WHITE);
    }
    
    void rect(const int & x, const int & y, const int & w, const int & h) {
      _oled->drawRect(x,y,w,h, WHITE);
    }
    
    void rectFill(const int & x, const int & y, const int & w, const int & h) {
      _oled->fillRect(x,y,w,h, WHITE);
    }
    
    void black(const int & x, const int & y, const int & w, const int & h) {
      _oled->fillRect(x,y,w,h, BLACK);
    }
    
    void circle(const int & x, const int & y, const int & radius) {
      _oled->drawCircle(x,y,radius, WHITE);
    }
    
    void fcircle(const int & x, const int & y, const int & radius) {
      _oled->fillCircle(x,y,radius, WHITE);
    }

    void setFontType(const int & t) {
      _oled->setTextSize(t);
    }
    
    void print(int c) {
      _oled->print(c);
    } 
    void println(int c) {
      _oled->println(c);
    }
    void print(long c) {
      _oled->print(c);
    } 
    void println(long c) {
      _oled->println(c);
    }
    
    void print(unsigned int c) {
      _oled->print(c);
    } 
    void println(unsigned int c) {
      _oled->println(c);
    }
    void print(unsigned long c) {
      _oled->print(c);
    } 
    void println(unsigned long c) {
      _oled->println(c);
    }
    
    void print(char c) {
      _oled->print(c);
    } 
    void println(char c) {
      _oled->println(c);
    }    
    void print(unsigned char c) {
      _oled->print(c);
    } 
    void println(unsigned char c) {
      _oled->println(c);
    }
    void print(const char c[]) {
      _oled->print(c);
    } 
    void println(const char c[]) {
      _oled->println(c);
    }
    void print(double d, int i) {
      _oled->print(d,i);
    } 
    void println(double d, int i) {
      _oled->println(d,i);
    }
    
    void setCursor(int x, int y) {
      _oled->setCursor(x,y);
    }

    void clear() {
      _oled->clearDisplay();
    }
    
    void free() {
      _oled->clearDisplay();
    }

    void on() {
      _oled->ssd1306_command(SSD1306_DISPLAYON);
    }
    
    void off() {
      _oled->ssd1306_command(SSD1306_DISPLAYOFF);
    }
    
    void command(uint8_t cmd) {
      _oled->ssd1306_command(cmd);
    }

    /**
     * the hard way to handle with german(?) UTF-8 stuff
     */
    char umlReplace(char inChar) {
      if (inChar == -97) {
        inChar = 224; // ß
      } else if (inChar == -80) {
        inChar = 248; // °
      } else if (inChar == -67) {
        inChar = 171; // 1/2
      } else if (inChar == -78) {
        inChar = 253; // ²
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
} oled;

int readVcc() {
  int mv;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(10); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
  mv = ADCL; 
  mv |= ADCH<<8; 
  mv = 1126400L / mv;
  if (mv > MAX_POWER) return batLength-3;
  if (mv < MIN_POWER) return 0;
  float quot = (MAX_POWER - MIN_POWER)/(batLength-3); // scale: MAX_POWER -> batLength, MIN_POWER -> 0
  mv = (mv-MIN_POWER)/quot;
  if (mv > batLength-3) return batLength-3;
  if (mv < 0) return 0;
  return mv;
}

void anaClock() {
  byte x = 33;
  byte y = 32;
  byte radius = 31;
  oled.circle(x, y, radius);
  int hour = hours;
  if (hour>12) hour = hour%12;
  oled.line(
    x, y,
    x + (radius-2)*cos(PI * ((float)seconds-15.0) / 30),
    y + (radius-2)*sin(PI * ((float)seconds-15.0) / 30)
  );
  
  oled.line(
    x, y,
    x + (radius-5)*cos(PI * ((float)minutes-15.0) / 30),
    y + (radius-5)*sin(PI * ((float)minutes-15.0) / 30)
  );
  
  oled.line(
    x, y,
    x + (radius-14)*cos(PI * ((float)hour+((float)minutes/60-0) -3.0) / 6),
    y + (radius-14)*sin(PI * ((float)hour+((float)minutes/60.0) -3.0) / 6)
  );
  
  for (byte i=0; i<12; ++i) {
    oled.pixel(x + (radius-3)*cos(PI * ((float)i) / 6), y + (radius-3)*sin(PI * ((float)i) / 6));  
  }

  oled.setCursor(x-5,y-radius+4);
  oled.print(12);
  oled.setCursor(x-2,y+radius-11);
  oled.print(6);
  oled.setCursor(x+radius-9,y-3);
  oled.print(3);  
  oled.setCursor(x-radius+6,y-3);
  oled.print(9);

  // and small digital
  oled.setCursor(x+radius+2,5);
  oled.setFontType(2);
  oled.print(hours);
  oled.print(':');
  if(minutes < 10) oled.print('0');
  oled.print(minutes);
  oled.setFontType(1);
}

void digiClock() {
  if (hours<10) {
    int xx = myFont(0, 10, 0);
    myFont(xx, 10, hours);
  } else {
    myFont(0, 10, hours);
  }
  if (minutes<10) {
    int xx = myFont(45, 10, 0);
    myFont(xx, 10, minutes);
  } else {
    myFont(45, 10, minutes);
  }
  if (seconds<10) {
    int xx = myFont(91, 10, 0);
    myFont(xx, 10, seconds);
  } else {
    myFont(91, 10, seconds);
  }
}

void printClock() {
  if (clockMode == 0) {
    digiClock();
  } else {
    anaClock();
  }
}

void scroll() {
  unsigned short i = 0;
  for (i=1; i<=8; ++i) {
    oled.black(0,0,oled.width(),i);
    oled.command(SSD1306_SETDISPLAYOFFSET);
    oled.command(i++);
    oled.display();
    delay(scrollSpeed);
  }
}

void batteryIcon() {
  byte vccVal = readVcc();
  oled.pixel   (oled.width() - batLength, oled.height()-3); 
  oled.pixel   (oled.width() - batLength, oled.height()-4);
  oled.rect    (oled.width() - batLength+1, oled.height()-6, batLength-1, 6);  
  oled.rectFill(oled.width() - vccVal   -1, oled.height()-5,      vccVal, 4);
}

void getIcon() {
  oled.clear();
  oled.circle(oled.width()/2, oled.height()/2, 5);
  oled.black(0,0,oled.width()/2, oled.height());
  oled.black(oled.width()/2, oled.height()/2,oled.width()/2, oled.height()/2);
  oled.display();
  delay(100);
  oled.circle(oled.width()/2, oled.height()/2, 10);
  oled.black(0,0,oled.width()/2, oled.height());
  oled.black(oled.width()/2, oled.height()/2,oled.width()/2, oled.height()/2);
  oled.display();
  delay(50);
  oled.circle(oled.width()/2, oled.height()/2, 15);
  oled.black(0,0,oled.width()/2, oled.height());
  oled.black(oled.width()/2, oled.height()/2,oled.width()/2, oled.height()/2);
  oled.display();  
  delay(50);
  oled.circle(oled.width()/2, oled.height()/2, 20);
  oled.black(0,0,oled.width()/2, oled.height());
  oled.black(oled.width()/2, oled.height()/2,oled.width()/2, oled.height()/2);
  oled.display();  
  delay(100);
}

byte tob(char c) {
  return c - '0';
}

int myFont(int x, int y, byte b) {
    int he = 30;
    if (b>9) {
      x = myFont(x,y,b/10);
      b = b%10;
    }
    if (b == 0) {
      oled.circle(x+he/4, y+3*he/4, he/4);
      return x+2+he/2;
    } else if (b == 1) {
      oled.line(x,y+5,x+5,y);
      oled.line(x+5,y,x+5,y+he);
      return x+8;  
    } else if (b == 2) {
      oled.circle(x+he/4, y-3+3*he/4, he/4);
      oled.black(x,y,he/4,he);
      oled.line(x+he/4,y+he-3,x+he/2,y+he);
      return x+2+he/2;
    } else if (b == 3) {
      oled.circle(x+he/4, y+1*he/4, he/4);
      oled.circle(x+he/4, y+3*he/4, he/4);
      oled.black(x,y,he/4,he);
      return x+2+he/2;
    } else if (b == 4) {
      oled.line(x,y+he/2,x+he/2,y);
      oled.line(x,y+he/2,x+he/2,y+he/2);
      oled.line(x+he/2,y,x+he/2,y+he);
      return x+2+he/2;   
    } else if (b == 5) {
      oled.line(x+he/4,y,x+he/2,y);
      oled.line(x+he/4,y,x+he/4,y+he/2);
      oled.circle(x+he/4, y+3*he/4, he/4);
      oled.black(x,y,he/4,he);
      return x+2+he/2;   
    } else if (b == 6) {
      oled.line(x,y-2+3*he/4,x+he/2,y);
      oled.circle(x+he/4, y+3*he/4, he/4);
      return x+2+he/2;   
    } else if (b == 7) {
      oled.line(x,y+3,x+he/2,y);
      oled.line(x+he/2,y,x,y+he);
      oled.line(x+3,y+he/2,x+he/2-1,y+he/2);
      return x+2+5;  
    } else if (b == 8) {
      oled.circle(x+he/4, y+1*he/4, he/4);
      oled.circle(x+he/4, y+3*he/4, he/4);
      return x+2+he/2;
    } else if (b == 9) {
      oled.circle(x+he/4, y+1*he/4, he/4);
      oled.line  (x+he/2, y+1*he/4, x+he/2, y+he);
      return x+2+he/2;
    }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    countToSleep = 0;
    if (inChar == -61) continue; // symbol before utf-8
    if (inChar == -62) continue; // other symbol before utf-8
    
    // Check at the message beginning for some special chars
    if (memoStr[0] == CHAR_TIME_RESPONSE) digitalWrite(LED_RED, HIGH);
    
    if (inChar == '\n') {
      memoStr[memoStrPos] = '\0';
      cStart = 0;
      cEnd = 0;
      buffMem = memoStr[cEnd];
      continue;
    }
    memoStr[memoStrPos] = oled.umlReplace(inChar);
    memoStrPos++;
    if (memoStrPos >= MEMOSTR_LIMIT) memoStrPos = 0;
  }
}

/**
 * make hours, minutes, seconds from "ticks" and
 * add it to the time (received from App)
 */
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
  if (hours != 42 && hours > 23) {
    hours = hours % 24;
  }
  // show Clock, if there is no message scrolling
  if (! (memoStrPos > 0 && cEnd <= memoStrPos) ) {
    
    countToSleep++;
    
    oled.clear();
    if (digitalRead(BUTTON2) == LOW) {
      cEnd = memoStrPos;      
      clockMode++;
      if (clockMode > 1) clockMode = 0;
    }
    printClock();
    batteryIcon();
    oled.display();
  } else {
    if (digitalRead(BUTTON2) == LOW) {
      cEnd = memoStrPos; // stop scrolling
      digitalWrite(LED_RED, LOW);
    }
  }
}

void wakeUpNow() {
  countToSleep = 0;
}

void sleepNow() {
  oled.clear();
  oled.display();
  oled.off();
  digitalWrite(LED_RED, LOW);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // deep sleep - nothing works
  sleep_enable();
  attachInterrupt(1, wakeUpNow, HIGH); // INT1 is on PIN3
  sleep_mode();

  // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  sleep_disable();
  detachInterrupt(1);
  oled.on();
}

void eye1() {
  oled.circle (oled.width()/2 -29, oled.height()/2, 12);
  oled.fcircle(oled.width()/2 -29, oled.height()/2, 3);
  
  oled.circle (oled.width()/2 +29, oled.height()/2, 12);
  oled.fcircle(oled.width()/2 +26, oled.height()/2+2, 3);  
}

void eye2() {
  oled.circle (oled.width()/2 -29, oled.height()/2, 12);
  oled.fcircle(oled.width()/2 -29, oled.height()/2, 3);
  
  oled.circle (oled.width()/2 +29, oled.height()/2, 12);
  oled.fcircle(oled.width()/2 +26, oled.height()/2+2, 3);  

  oled.black(0,0,oled.width(), oled.height()/2 -4);

  oled.line(
    oled.width()/2 -42,
    oled.height()/2 -4,
    oled.width()/2 -15,
    oled.height()/2 -4
  );
  oled.line(
    oled.width()/2 +15,
    oled.height()/2 -4,
    oled.width()/2 +42,
    oled.height()/2 -4
  );
}

void eye3() {
  oled.circle(oled.width()/2 -29, oled.height()/2, 12);
  oled.circle(oled.width()/2 +29, oled.height()/2, 12);
  oled.black(0,0,oled.width(), oled.height()/2 +6);
}


void setup() {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);
  //pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  //digitalWrite(PIN_CS, HIGH);
  //digitalWrite(PIN_DC, LOW); // LOW = ADDRESS 0x3C
  digitalWrite(PIN_DC, HIGH); // HIGH = ADDRESS 0x3D

  attachInterrupt(1, wakeUpNow, HIGH); // INT1 is on PIN3
  Serial.begin(BLE_UART_SPEED);

  MsTimer2::set(TIME_PITCH, ticking); // 1000ms period
  MsTimer2::start();
  oled.begin();
  buffMem = '\0';
}

void loop() {    
  if (digitalRead(BUTTON1) == LOW) {
    delay(300);
    if (digitalRead(BUTTON1) == LOW) {
      MsTimer2::stop();
      getIcon();
      // "remove" old chars from buffer
      // print ignores everyting behind \0
      memoStr[0] = '\0';
      memoStrPos = 0;
      MsTimer2::start();
      Serial.println( CHAR_MSG_REQUEST );
    }
  }
  
  // React, if there is a new message
  if (memoStrPos > 0 && cEnd == 0) {

    // Check at the message beginning for some special chars
    if (memoStr[0] == CHAR_TIME_RESPONSE) {
      
      // extract the time -------------------------
      memoStr[0] = ' ';
      hours = tob(memoStr[1])*10 + tob(memoStr[2]);
      minutes = tob(memoStr[4])*10 + tob(memoStr[5]);
      seconds = tob(memoStr[7])*10 + tob(memoStr[8]);
      oled.clear();
      oled.setCursor(0, 0);
      printClock();
      oled.display();
      delay(1000);
      seconds++;
      digitalWrite(LED_RED, LOW);
    }
  }
  
  //Scrolling message through display
  if (memoStrPos > 0 && cEnd <= memoStrPos) {
    if (cEnd < memoStrPos) memoStr[cEnd] = '\0';
    oled.clear();
    oled.setCursor(0,0);
    oled.print(&(memoStr[cStart]));
    oled.command(SSD1306_SETDISPLAYOFFSET);
    oled.command(0x0);
    oled.display();
    delay(scrollSpeed);
    
    memoStr[cEnd] = buffMem;
    if (cEnd%21 == 0 && cEnd > 147) {
      cStart += 21;
      scroll();
    }
    cEnd++;
    if (cEnd <= memoStrPos) buffMem = memoStr[cEnd];
  }
  
  // message is at the end
  if (cStart > memoStrPos) {
    memoStr[0] = '\0';
    memoStrPos = 0;
  }

  if (countToSleep > SECtoSLEEP) {
    MsTimer2::stop();
    oled.clear();
    eye1();
    oled.display();
    delay(200);
    oled.clear();
    eye2();
    oled.display();
    delay(100);
    oled.clear();
    eye3();
    oled.display();
    delay(100);
    hours   = 42;
    minutes = 42;
    seconds = 42;
    memoStr[0] = '\0';
        sleepNow();
    oled.clear();
    eye3();
    oled.display();
    delay(200);
    oled.clear();
    eye2();
    oled.display();
    delay(100);
    oled.clear();
    eye1();
    oled.display();
    delay(100);
    MsTimer2::start();
    Serial.println( CHAR_TIME_REQUEST );
  }

}


