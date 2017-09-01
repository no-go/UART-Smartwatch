#include <MsTimer2.h>

#include <Wire.h> //I2C Arduino Library
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

#include "OledWrapper.h"
#include "umlReplace.h"
#include "movies.h"

// CONFIG -------------------------------------------

// --------- HARDWARE SPI OLED: 11 -> MOSI/DIN, 13 -> SCK
#define OLED_CS    4
#define PIN_RESET  6
#define PIN_DC     8

#define BUTTON1    2
#define BUTTON2    3

#define LED_BLUE   10
#define LED_GREEN  9
#define LED_RED    5
#define LED_OFF    LOW
#define LED_ON     HIGH

#define SYS_SWITCH A7
#define MAS_SWITCH A0
#define BLE_LED    7  //unused

#define SERIAL_SPEED   9600
#define DISPLAYSEC     5
#define DISPLAYDIMSEC  10
#define DISPLAYTERMSEC 30
#define YMAGDELTA      40

#define WARN_POWER 3243   // or try 3200, 3500, 3600

// --------------------------------------------------

#define CHAR_TIME_REQUEST '~'

#define STOREAGE 60

int  vcc;
bool powerlow = false;

short yy,yylast;

byte hours        = 0;
byte minutes      = 0;
byte seconds      = 0;
byte dsec         = DISPLAYSEC;
byte terminalTick = 0;

OledWrapper * oled = new OledWrapper(PIN_DC, PIN_RESET, OLED_CS);

struct Midways {
  short _basic;
  short _val[STOREAGE];
  int  _nxt;

  Midways() {
    _nxt = 0;
    _basic = 140;
    for (int i=0; i<STOREAGE; ++i) { 
      _val[i] = _basic;
    }
  }

  void add(short val) {
    _val[_nxt] = val;
    _nxt++;
    if (_nxt == STOREAGE) {
      _nxt = 0;
    }
  }

  float midget() {
    float mid = 0;
    for (int i=0; i<STOREAGE; ++i) mid += _val[i];
    
    return mid/(float)STOREAGE;
  }

  void draw(byte x, byte y, float fak) {
    int id = _nxt-1;
    float mid = midget();
    if (id < 0) id += STOREAGE;

    byte lastx,lasty;
    byte dx = x + 2*(STOREAGE);
    byte dy = y - fak*((float)_val[id] - mid);
    
    for (int i=0; i<STOREAGE; ++i) {
      lastx = dx;
      lasty = dy;
      dx = x+2*(STOREAGE-i);
      dy = y - fak*((float)_val[id] - mid);
      oled->drawLine(lastx, lasty, dx, dy, WHITE);
      id--;
      if (id < 0) id += STOREAGE;
    }
  }
};

Midways mi;

void mesure() {
  static sensors_event_t event; 
  mag.getEvent(&event);
  
  yylast = yy;

  // get new
  yy = event.magnetic.y + 184;
  if (yy == -188) yy=500;
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

inline void ticking() {
  tick++;
  
  if (tick > 9) {
    if (terminalTick >= DISPLAYTERMSEC) terminalTick=0;
    if (terminalTick > 0) terminalTick++;

    seconds += tick/10;
    if(dsec < 250) dsec++;
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

void printClock() {
  byte xv;
  
  oled->drawBitmap(6, 2, pipboy, 112, 48, WHITE);
  
  xv = oled->myFont(10,  32, hours/10);
  oled->myFont(xv, 32, hours - 10*(hours/10));
  
  xv = oled->myFont(38, 9, minutes/10);
  oled->myFont(xv, 9, minutes - 10*(minutes/10));
  
  xv = oled->myFont(96, 15, seconds/10);
  oled->myFont(xv+2, 14, seconds - 10*(seconds/10));
  
  oled->setTextSize(1);

  if ( ab(yy - yylast) > YMAGDELTA) {
    oled->on();
    oled->dim(false);
    dsec = 0;
  }

  mi.draw(3, 55, 0.3);
  
  oled->setCursor(5,57);
  oled->print(vcc-WARN_POWER);
  oled->print('%');
  
  oled->setCursor(100,57);
  oled->print(yy); 
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

float ab(float x) {
  if (x<0) return -1*x;
  return x;  
}

void serialEvent() {
  while (Serial.available()) {
    if (terminalTick == 0) {
      terminalTick = 1;
      oled->on();
      oled->dim(false);
      dsec = 0;
      oled->clearDisplay();
      oled->setCursor(0, 0);
    }
    char inChar = (char)Serial.read();
    oled->print(inChar);
    oled->display();
  }
}

void setup() {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(LED_RED,   OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE,  OUTPUT);

  pinMode(SYS_SWITCH, OUTPUT);
  pinMode(MAS_SWITCH, OUTPUT);
  pinMode(BLE_LED,    INPUT);

  mag.begin();

  Serial.begin(SERIAL_SPEED);

  digitalWrite(SYS_SWITCH, LOW);
  digitalWrite(MAS_SWITCH, LOW);
  delay(1200);  
  digitalWrite(SYS_SWITCH, HIGH);
  digitalWrite(MAS_SWITCH, HIGH);
  Serial.println("AT+NAMEGiveUp Boy");
  Serial.println("AT+ROLE0");
  

  oled->begin();
  oled->clearDisplay();
  oled->setTextSize(1);
  oled->display();

  yy=0;

  digitalWrite(LED_RED,   LED_OFF);
  digitalWrite(LED_GREEN, LED_OFF);
  digitalWrite(LED_BLUE,  LED_OFF);
  
  oled->clearDisplay();
  
  MsTimer2::set(100, ticking); // 100ms period
  MsTimer2::start();
}

void loop() {
  delay(40);
  
  if (seconds == 10 || tick == 1) {
    readVcc();
    if (vcc < WARN_POWER && powerlow == false) {
      powerlow = true;
      Serial.print(vcc);
      Serial.println(" mV");
    } else {
      powerlow = false;
    }
  }

  if (terminalTick == 0) {
    oled->clear();
    printClock();
  }
  
  mesure();
  mi.add(yy);

  if (dsec == DISPLAYSEC) {
    oled->dim(true);
  }
  
  if (dsec == DISPLAYDIMSEC) {
    oled->dim(false);
    oled->off();
  }

  if (terminalTick == 0) oled->setCursor(0, 0);
  
  if (digitalRead(BUTTON1) == LOW) {
    terminalTick = 0;
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
        digitalWrite(LED_BLUE, LED_ON);
        delay(500);
        digitalWrite(LED_BLUE, LED_OFF);
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
        digitalWrite(LED_RED,  LED_ON);
        digitalWrite(LED_BLUE, LED_ON);
        delay(500);
        digitalWrite(LED_RED,  LED_OFF);
        digitalWrite(LED_BLUE, LED_OFF);
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
      
      digitalWrite(LED_RED,   LED_OFF);
      digitalWrite(LED_GREEN, LED_OFF);
      digitalWrite(LED_BLUE,  LED_OFF);

      terminalTick = 1;
      oled->on();
      oled->dim(false);
      dsec = 1;
      wakeUpIcon();
      oled->clear();
      Serial.println(CHAR_TIME_REQUEST);
    }
  }
  
  if ( (seconds == 0) && (tick == 0) && (minutes%5 == 0)) Serial.println(CHAR_TIME_REQUEST);
  
  oled->display();
  if (powerlow) {
    analogWrite(LED_RED, 5*tick);
    delay(40);
  } else {
    if (seconds == 0 && tick == 0) {
      digitalWrite(LED_RED,   LED_ON);
      digitalWrite(LED_GREEN, LED_ON);
      digitalWrite(LED_BLUE,  LED_ON);
      delay(40);
      digitalWrite(LED_RED,   LED_OFF);
      digitalWrite(LED_GREEN, LED_OFF);
      digitalWrite(LED_BLUE,  LED_OFF);
    } else {
      delay(40);         
    }
  }
}

