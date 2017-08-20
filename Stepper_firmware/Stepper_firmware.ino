#include <MsTimer2.h>

#include <Wire.h> //I2C Arduino Library
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

#include "OledWrapper.h"
#include "umlReplace.h"
#include "movies.h"

// CONFIG -------------------------------------------

// --------- HARDWARE I2C HMC5883: A4 -> SDA, A5 -> SCL
#define HMC_DRDY   A1 //unused

// --------- HARDWARE SPI OLED: 11 -> MOSI/DIN, 13 -> SCK
#define OLED_CS    4
#define PIN_RESET  6
#define PIN_DC     8

#define BUTTON1    3
#define BUTTON2    2

#define LED_BLUE   10
#define LED_GREEN  9
#define LED_RED    5
#define LED_OFF    LOW
#define LED_ON     HIGH

#define SERIAL_SPEED   9600
#define DISPLAYSEC     15
#define DISPLAYDIMSEC  60

#define SYS_SWITCH A7
#define MAS_SWITCH A0
#define BLE_LED    7  //unused

#define WARN_POWER 3243   // or try 3200, 3500, 3600

// --------------------------------------------------

#define CHAR_TIME_REQUEST     '~'
#define CHAR_TIME_RESPONSE    '#' //#HH:mm:ss
#define CHAR_NOTIFY_HINT      '%' //%[byte]

#define MEMOSTR_LIMIT   115
#define STEP_TRESHOLD   0.125

#define MOVIE_X  100
#define MOVIE_Y  29

int  vcc;
bool powerlow = false;

#define MINDIF  10

short xx,yy,zz;
short minx=400, maxx=0;
short miny=400, maxy=0;
short minz=400, maxz=0;
float changes      = 0.0;
float changes_old  = 0.0;
float delta        = 0.0;
unsigned int steps = 0;

char memoStr[MEMOSTR_LIMIT] = {'\0'};
byte memoStrPos = 0;
char buffMem;
byte cEnd  = 0;
byte cStart= 0;
byte COUNT = 0;

byte hours   = 0;
byte minutes = 0;
byte seconds = 0;
byte dsec    = DISPLAYSEC;
byte mtick   = 0;

OledWrapper * oled = new OledWrapper(PIN_DC, PIN_RESET, OLED_CS);

void mesure() {
  static sensors_event_t event; 
  mag.getEvent(&event);
  
  // get new
  xx = event.magnetic.x + 184;
  yy = event.magnetic.y + 184;
  zz = event.magnetic.z + 184;
  if (xx == -188) xx=500;
  if (yy == -188) yy=500;
  if (zz == -188) zz=500;
  if (xx < minx) minx=xx;
  if (yy < miny) miny=yy;
  if (zz < minz) minz=zz;
  if (xx<500 && xx > maxx) maxx=xx;
  if (yy<500 && yy > maxy) maxy=yy;
  if (zz<500 && zz > maxz) maxz=zz;

  changes = ( (float) xx / (float)(maxx-minx) ) + ( (float) yy / (float)(maxy-miny) ) + ( (float) zz / (float)(maxz-minz) );
  delta =  ab(changes_old - changes);
  changes_old = changes;

  if (delta > STEP_TRESHOLD) {
    steps++;
    if (steps%50 == 0) {
      //analogWrite(LED_RED,   250);
      analogWrite(LED_GREEN, 130);
      //analogWrite(LED_BLUE,  130);
      delay(50);
      digitalWrite(LED_RED,   LED_OFF);
      digitalWrite(LED_GREEN, LED_OFF);
      digitalWrite(LED_BLUE,  LED_OFF);
    }
    if (steps%500 == 0) Serial.println(steps);
  }
}

void readVcc() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(10); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
  vcc = ADCL; 
  vcc |= ADCH<<8; 
  vcc = 1126400L / vcc;
}

void serialEvent() {
  while (Serial.available()) {
    if (memoStrPos >= MEMOSTR_LIMIT) memoStrPos = MEMOSTR_LIMIT;
    char inChar = (char)Serial.read();
    if (inChar == -61) continue; // symbol before utf-8
    if (inChar == -62) continue; // other symbol before utf-8
    if (inChar == '\n') {
      memoStr[memoStrPos] = '\0';
      dsec = 0;
      cEnd = 0;
      cStart = 0;
      buffMem = memoStr[cEnd];
      continue;
    }
    umlReplace(&inChar);
    memoStr[memoStrPos] = inChar;
    memoStrPos++;    
    if (memoStrPos >= MEMOSTR_LIMIT) {
      // ignore the other chars
      memoStrPos = MEMOSTR_LIMIT-1;
      memoStr[memoStrPos] = '\0';
    }
  }
}

inline void ticking() {
  tick++;
  if (tick%3 && mtick > 0) mtick++;
  if (mtick >= 7) mtick=0;
  
  if (tick > 9) {
    seconds += tick/10;
    if(dsec < 250) dsec++;
    oled->changeDigit = B00000001;
    if (seconds == 10 || seconds == 20 || seconds == 30 || seconds == 40 || seconds == 50) oled->changeDigit = B00000011;
  }
  
  if (tick > 9) {
    tick = tick % 10;
    if (seconds > 59) {
      minutes += seconds / 60;
      seconds  = seconds % 60;
      if (minutes%5 == 0) Serial.println(steps);

      oled->changeDigit = B00000111;
      if (minutes == 10 || minutes == 20 || minutes == 30 || minutes == 40 || minutes == 50) oled->changeDigit = B00001111;

      // modify limits every 1min to make a better re-calibration 
      if (minx > 1 && maxx < 399 && ( (maxx-minx) < MINDIF)) {
        maxx--; minx++;
      }
      if (miny > 1 && maxy < 399 && ( (maxy-miny) < MINDIF)) {
        maxy--; miny++;
      }
      if (minz > 1 && maxz < 399 && ( (maxz-minz) < MINDIF)) {
        maxz--; minz++;
      }
    }
    if (minutes > 59) {
      hours  += minutes / 60;
      minutes = minutes % 60;
      oled->changeDigit = B00011111;
      if (hours == 10 || hours == 20 || hours == 24) oled->changeDigit = B00111111;
    }
    if (hours > 23) {
      hours = hours % 24;
    }
  }
  if (COUNT > 0) {
    if (tick == 0) {
      analogWrite(LED_GREEN, 100); 
      analogWrite(LED_RED,    50); 
    } else {
      digitalWrite(LED_RED,   LED_OFF);
      digitalWrite(LED_GREEN, LED_OFF);
      digitalWrite(LED_BLUE,  LED_OFF);
    }
  }
}

void printClock() {
  byte he = 27;
  byte xx;
  if (dsec > DISPLAYSEC) he = 13;
  
  xx = oled->myFont(3, 7, hours/10, B00100000, he);
  oled->myFont(xx, 7, hours - 10*(hours/10), B00010000, he);
  
  xx = oled->myFont((int)(he*1.75), 7, minutes/10, B00001000, he);
  oled->myFont(xx, 7, minutes - 10*(minutes/10), B00000100, he);
  
  xx = oled->myFont((int)(he*3.25), 7, seconds/10, B00000010, he);
  oled->myFont(xx, 7, seconds - 10*(seconds/10), B00000001, he);
  
  oled->setTextSize(1);
  if (COUNT > 0) {
    oled->setCursor(84, 42);
    oled->print(COUNT);  
  }

  if (delta > 0.2) {
    oled->on();
    oled->dim(false);
    dsec = 0;
  }

  if (delta > 0.05 && mtick == 0 && tick == 0) mtick=1;
  
  if (mtick==1) oled->drawBitmap(MOVIE_X, MOVIE_Y, movi1, 16, 24, WHITE);
  if (mtick==2) oled->drawBitmap(MOVIE_X, MOVIE_Y, movi2, 16, 24, WHITE);
  if (mtick==3) oled->drawBitmap(MOVIE_X+2, MOVIE_Y, movi3, 16, 24, WHITE);
  if (mtick==4) oled->drawBitmap(MOVIE_X+8, MOVIE_Y, movi4, 16, 24, WHITE);
  if (mtick==5) oled->drawBitmap(MOVIE_X+12, MOVIE_Y, movi5, 16, 24, WHITE);
  if (mtick==6) oled->drawBitmap(MOVIE_X+16, MOVIE_Y, movi6, 16, 24, WHITE);
  
  oled->setCursor(5,42);
  oled->print(vcc-WARN_POWER);
  oled->print('%');
  oled->print(' ');
  oled->print('f');
  oled->print('u');
  oled->print('e');
  oled->print('l');
  
  oled->setCursor(5,55);
  oled->print(steps);
  oled->print(' ');
  oled->print('s');
  oled->print('t');
  oled->print('e');
  oled->print('p');
  oled->print('s');
}

inline void wakeUpIcon() {
  oled->clear();
  digitalWrite(LED_RED,   LED_OFF);
  analogWrite(LED_BLUE,  10);
  analogWrite(LED_GREEN,  5);
  oled->circle(oled->width()/2, oled->height()/2, 5);
  oled->black(0,0,oled->width()/2, oled->height());
  oled->black(oled->width()/2, oled->height()/2,oled->width()/2, oled->height()/2);
  oled->display();
  delay(100);
  analogWrite(LED_BLUE,  50);
  analogWrite(LED_GREEN, 20);
  oled->circle(oled->width()/2, oled->height()/2, 10);
  oled->black(0,0,oled->width()/2, oled->height());
  oled->black(oled->width()/2, oled->height()/2,oled->width()/2, oled->height()/2);
  oled->display();
  delay(100);
  analogWrite(LED_BLUE, 100);
  analogWrite(LED_GREEN, 50);
  oled->circle(oled->width()/2, oled->height()/2, 15);
  oled->black(0,0,oled->width()/2, oled->height());
  oled->black(oled->width()/2, oled->height()/2,oled->width()/2, oled->height()/2);
  oled->display();  
  delay(100);
  analogWrite(LED_BLUE, 200);
  analogWrite(LED_GREEN, 90);
  oled->circle(oled->width()/2, oled->height()/2, 20);
  oled->black(0,0,oled->width()/2, oled->height());
  oled->black(oled->width()/2, oled->height()/2,oled->width()/2, oled->height()/2);
  oled->display();  
  delay(200);
  digitalWrite(LED_RED,   LED_OFF);
  digitalWrite(LED_GREEN, LED_OFF);
  digitalWrite(LED_BLUE,  LED_OFF);
}

inline byte tob(char c) { return c - '0';}

float ab(float x) {
  if (x<0) return -1*x;
  return x;  
}

byte slen(char * str) {
  byte i;
  for (i=0;str[i] != '\0' && i < 255; ++i);
  return i;
}

void setup() {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(LED_RED,   OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE,  OUTPUT);

  Serial.begin(SERIAL_SPEED);

  oled->begin();
  oled->clearDisplay();
  oled->display();
  buffMem = '\0';

  mag.begin();

  // disco
  for (xx=0;xx<256;xx+=31) {
    analogWrite(LED_RED, xx);
    for (yy=0;yy<256;yy+=31) {
      analogWrite(LED_GREEN, yy);
      for (zz=0;zz<256;zz+=31) {
        analogWrite(LED_BLUE, zz);
        delay(5);
      }
      oled->clearDisplay();
      oled->print('B');
      oled->print('o');
      oled->print('u');
      oled->print('l');
      oled->print('d');
      oled->print('e');
      oled->print('r');
      oled->print(' ');
      oled->print('C');
      oled->print('l');
      oled->print('o');
      oled->print('c');
      oled->print('k');
      if (yy<31) oled->drawBitmap(50, 20, movi1, 16, 24, WHITE);
      else if (yy<80) oled->drawBitmap(50, 20, movi2, 16, 24, WHITE);
      else if (yy<100) oled->drawBitmap(52, 20, movi3, 16, 24, WHITE);
      else if (yy<140) oled->drawBitmap(58, 20, movi4, 16, 24, WHITE);
      else if (yy<190) oled->drawBitmap(62, 20, movi5, 16, 24, WHITE);
      else oled->drawBitmap(66, 20, movi6, 16, 24, WHITE);
      oled->display();
    }
  }

  xx=0;
  yy=0;
  zz=0;

  digitalWrite(LED_RED,   LED_OFF);
  digitalWrite(LED_GREEN, LED_OFF);
  digitalWrite(LED_BLUE,  LED_OFF);
  
  oled->clearDisplay();
  buffMem = '\0';
  
  MsTimer2::set(100, ticking); // 100ms period
  MsTimer2::start();
}

void loop() {
  delay(81);
  
  if (seconds == 10 || tick == 1) {
    readVcc();
    if (vcc < WARN_POWER && powerlow == false) {
      powerlow = true;
      Serial.print("Steps: ");
      Serial.println(steps);
      Serial.print(vcc);
      Serial.println(" mV");
    } else {
      powerlow = false;
    }
  }
  
  oled->clear();
  printClock();
  oled->setTextSize(1);
  mesure();
  
  if (dsec < DISPLAYSEC && cStart < memoStrPos) {
    oled->setCursor(0,22);
    oled->black(0,22,128,42);
    oled->print(&(memoStr[cStart]));
  }

  if (dsec == DISPLAYSEC) {
    memoStr[0] = '\0';
    memoStrPos = 0;
    oled->dim(true);
  }
  
  if (dsec == DISPLAYDIMSEC) {
    oled->dim(false);
    oled->off();
  }

  oled->setCursor(0, 0);
  
  if (digitalRead(BUTTON1) == LOW) {
    oled->on();
    oled->dim(false);
    dsec = 0;
    digitalWrite(LED_RED,   LED_OFF);
    digitalWrite(LED_GREEN, LED_OFF);
    digitalWrite(LED_BLUE,  LED_OFF);
    delay(500);
    
    if (digitalRead(BUTTON1) == LOW) {
      delay(500);
      while (digitalRead(BUTTON1) == LOW) {
        delay(500);
        minutes++;
        seconds=0;
        tick=0;
        oled->clear();
        printClock();
        oled->display();    
      }
    }
    if (digitalRead(BUTTON2) == LOW) {
      delay(500);
      while (digitalRead(BUTTON2) == LOW) {
        delay(500);
        hours++;
        seconds=0;
        tick=0;
        oled->clear();
        printClock();
        oled->display();    
      }
    }
  }

  if (digitalRead(BUTTON2) == LOW) {
    delay(200);  
    if (digitalRead(BUTTON2) == LOW) {
      
      COUNT = 0;
      digitalWrite(LED_RED,   LED_OFF);
      digitalWrite(LED_GREEN, LED_OFF);
      digitalWrite(LED_BLUE,  LED_OFF);

      Serial.print("Steps: ");
      Serial.println(steps);
      Serial.print(vcc);
      Serial.println(" mV");

      oled->on();
      oled->dim(false);
      dsec = 0;
      wakeUpIcon();
                      
      // "remove" old chars from buffer
      // print ignores everyting behind \0
      memoStr[0] = '\0';
      memoStrPos = 0;
      iconType = MSG_NO;
      Serial.print(CHAR_TIME_REQUEST);
      Serial.print('\n');
    }
  }
  
  if (COUNT > 0) {
    int l = slen(memoStr);
    if (
      (l > 8 &&
      memoStr[l-8] == 'c' &&
      memoStr[l-7] == 'a' &&
      memoStr[l-6] == 'l' &&
      memoStr[l-5] == 'e' &&
      memoStr[l-4] == 'n' &&
      memoStr[l-3] == 'd' &&
      memoStr[l-2] == 'a' &&
      memoStr[l-1] == 'r') || iconType == MSG_CAL
    ) {
      oled->drawBitmap(74, 42, icon_calendar, 8, 8, WHITE);
      iconType = MSG_CAL;
    } else if (
      (l > 9 &&
      memoStr[l-9] == 'm' &&
      memoStr[l-8] == 'e' &&
      memoStr[l-7] == 's' &&
      memoStr[l-6] == 's' &&
      memoStr[l-5] == 'a' &&
      memoStr[l-4] == 'g' &&
      memoStr[l-3] == 'i' &&
      memoStr[l-2] == 'n' &&
      memoStr[l-1] == 'g') || iconType == MSG_SMS
    ) {
      oled->drawBitmap(74, 42, icon_messaging, 8, 8, WHITE);
      iconType = MSG_SMS;
    } else if (
      (l > 7 &&
      memoStr[l-7] == 'f' &&
      memoStr[l-6] == 's' &&
      memoStr[l-5] == 'c' &&
      memoStr[l-4] == 'k' &&
      memoStr[l-3] == '.' &&
      memoStr[l-2] == 'k' &&
      memoStr[l-1] == '9') || iconType == MSG_MAIL
    ) {
      oled->drawBitmap(74, 42, icon_mail, 8, 8, WHITE);
      iconType = MSG_MAIL;
    } else if (
      (l > 5) || iconType == MSG_OTHER
    ) {
      oled->drawBitmap(74, 42, icon_other, 8, 8, WHITE);
      iconType = MSG_OTHER;
    }
  } else {
    digitalWrite(LED_RED,   LED_OFF);
    digitalWrite(LED_GREEN, LED_OFF);
    digitalWrite(LED_BLUE,  LED_OFF);
    iconType = MSG_NO;
  }
  
  if (memoStr[0] == CHAR_TIME_RESPONSE) {      
    // extract the time -------------------------
    
    memoStr[0] = ' ';
    hours = tob(memoStr[1])*10 + tob(memoStr[2]);
    minutes = tob(memoStr[4])*10 + tob(memoStr[5]);
    seconds = tob(memoStr[7])*10 + tob(memoStr[8]);

  } else if (memoStr[0] == CHAR_NOTIFY_HINT) {
    // there is a new message (or a message is deleted)
    COUNT = (unsigned char) memoStr[1];
  }

  oled->display();
  
  if (powerlow) analogWrite(LED_RED, 5*tick);

  cEnd++;
  cStart++;
  if (cEnd <= memoStrPos) buffMem = memoStr[cEnd];
}

