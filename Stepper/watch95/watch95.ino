#include <MsTimer2.h>
#include "movies.h"

#include <Wire.h> //I2C Arduino Library
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------- HARDWARE SPI OLED: 11 -> MOSI/DIN, 13 -> SCK
#define OLED_CS    4
#define PIN_RESET  6
#define PIN_DC     8

#define BUTTON1    2
#define BUTTON2    3

#define LED_BLUE   10
#define LED_GREEN  9
#define LED_RED    5

#define SERIAL_SPEED   9600
#define DISPLAYSEC     3
#define DISPLAYDIMSEC  20
#define YMAGDELTA      30
#define DISPLAYTERMSEC 30

#define WARN_POWER 3243   // or try 3243, 3500, 3600
#define CHAR_TIME_REQUEST '~'

Adafruit_SSD1306 * oled = new Adafruit_SSD1306(PIN_DC, PIN_RESET, OLED_CS);

#define STOREAGE 28
#define STEPTRES 1.025

int  vcc;
bool powerlow = false;

short lasty;
unsigned int steps = 0;

byte  hours        = 0;
byte  minutes      = 0;
byte  seconds      = 0;
short tick         = 0;
byte terminalTick  = 0;
byte dsec          = DISPLAYSEC;

byte lamp = 0;
byte menu = 0;
bool alwaysClock = false;

struct Vec {
  short x;
  short y;
  short z;
  double leng;
};

Vec vnow = {1,1,1,1};
Vec vfirst = {1,1,1,1};
bool mfirst = true;

struct Midways {
  byte _basic;
  byte _val[STOREAGE];
  int  _nxt;
  float _mid;

  Midways() {
    _nxt = 0;
    _basic = 140;
    _mid = 140;
    for (int i=0; i<STOREAGE; ++i) { 
      _val[i] = _basic;
    }
  }

  void add(short val) {
    static bool posi = false;
    val += 184;
    if (val == -188) val=0;
    val = map(val, 0,370, 0,255);
    _val[_nxt] = val;
    _nxt = (_nxt+1) % STOREAGE;
    _mid = midget();
    
    if (val > (_mid * STEPTRES)) {
      posi = true;
    }

    if (posi == true && val < (_mid/STEPTRES)) {
      steps++;
      posi = false;
    }
  }

  float midget() {
    float mid = 0;
    for (int i=0; i<STOREAGE; ++i) mid += _val[i];
    
    return mid/(float)STOREAGE;
  }

  void draw(byte x, byte y, float fak) {
    int id = _nxt-1;
    if (id < 0) id += STOREAGE;

    short lastx,lasty;
    short dx = x + 3*STOREAGE;
    short dy = y - fak*((float)_val[id] - _mid);
    
    for (int i=0; i<STOREAGE; ++i) {
      lastx = dx;
      lasty = dy;
      dx = x+3*(STOREAGE-i);
      dy = y - fak*((float)_val[id] - _mid);
      if (dy > 63) dy=63;
      if (dy < 0)  dy=0;
      if (i>2) {
        oled->drawLine(lastx, lasty, dx, dy, WHITE);       
      } else {
        if (i==0) {
          oled->drawPixel(dx+1, dy-1, WHITE);
          oled->drawCircle(dx, dy, 5, WHITE);
          if (tick>4) oled->drawLine(dx+3, dy+3, dx+5, dy+5, WHITE);
        }
      }
      
      if (i==15) {
        if (tick<3) {
          oled->drawBitmap(dx-10, dy-24, movi1, 16, 24, WHITE);          
        } else if (tick < 7) {
          oled->drawBitmap(dx-10, dy-24, movi2, 16, 24, WHITE);
        } else {
          oled->drawBitmap(dx-10, dy-24, movi3, 16, 24, WHITE);
        }
      }

      id--;
      if (id < 0) id += STOREAGE;
    }
  }
};

Midways msi;

void mesure() {  
  static sensors_event_t event;
  mag.getEvent(&event);
  vnow.x = event.magnetic.x;
  vnow.y = event.magnetic.y;
  vnow.z = event.magnetic.z;
  
  vnow.leng = sqrt( sq((double)vnow.x) + sq((double)vnow.y) + sq((double)vnow.z) );
  
  if (mfirst) {
    mfirst = false;
    vfirst.x = vnow.x;
    vfirst.y = vnow.y;
    vfirst.z = vnow.z;
    vfirst.leng = vnow.leng;
  }
}

double skpro(Vec a, Vec b) {
  return ((double)a.x * (double)b.x) + ((double)a.y * (double)b.y) + ((double)a.z * (double)b.z);
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

void printClock() {
  oled->setTextSize(2);
  oled->setCursor(97,1);
  if (hours <10) oled->print('0');
  oled->print(hours);

  oled->setCursor(97,19);
  if (minutes <10) oled->print('0');
  oled->print(minutes);

  oled->setTextSize(1);
  oled->setCursor(107,38);
  if (seconds <10) oled->print('0');
  oled->print(seconds);
}

void printMenu() {
  byte y = 5;
  byte x = 20;
  oled->setTextSize(1);
  oled->setCursor(x, y);
  oled->print('L');
  oled->print('a');
  oled->print('m');
  oled->print('p');
  y=y+10;
  oled->setCursor(x, y);
  oled->print('s');
  oled->print('e');
  oled->print('t');
  oled->print(' ');
  oled->print('h');
  oled->print('o');
  oled->print('u');
  oled->print('r');
  oled->print('s');
  oled->setCursor(110, y);
  oled->print(hours);
  y=y+10;
  oled->setCursor(x, y);
  oled->print('s');
  oled->print('e');
  oled->print('t');
  oled->print(' ');
  oled->print('m');
  oled->print('i');
  oled->print('n');
  oled->print('u');
  oled->print('t');
  oled->print('e');
  oled->print('s');
  oled->setCursor(110, y);
  oled->print(minutes);
  y=y+10;
  oled->setCursor(x, y);
  oled->print('c');
  oled->print('l');
  oled->print('o');
  oled->print('c');
  oled->print('k');
  oled->print(' ');
  oled->print('(');
  oled->print('a');
  oled->print('l');
  oled->print('w');
  oled->print('a');
  oled->print('y');
  oled->print('s');
  oled->print(')');
  y=y+10;
  oled->setCursor(x, y);
  oled->print('c');
  oled->print('l');
  oled->print('o');
  oled->print('c');
  oled->print('k');
  y=y+10;
  oled->setCursor(x, y);
  oled->print('g');
  oled->print('e');
  oled->print('t');
  oled->print(' ');
  oled->print('m');
  oled->print('s');
  oled->print('g');
   
  oled->setCursor(10, 5 + (10*(menu-1)));
  oled->println('>'); 
}

inline void ticking() {
  tick = (tick+1) % 10;
  
  if (tick == 0) {
    if (terminalTick >= DISPLAYTERMSEC) terminalTick=0;
    if (terminalTick > 0) terminalTick++;
    seconds++;
    if(dsec < 250) dsec++;
  }
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

void serialEvent() {
  while (Serial.available()) {
    if (terminalTick == 0) {
      terminalTick = 1;
      oled->ssd1306_command(SSD1306_DISPLAYON);
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

  digitalWrite(LED_RED,   LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE,  LOW);

  Serial.begin(SERIAL_SPEED);
  
  mag.begin();
  delay(80);
  
  oled->begin(SSD1306_SWITCHCAPVCC);
  oled->clearDisplay();
  oled->setTextColor(WHITE);
  oled->setTextSize(1);
  oled->dim(false);
  oled->display(); 
  
  MsTimer2::set(100, ticking); // 100ms period
  MsTimer2::start();
}

void loop() {
  delay(33);
  
  lasty = vnow.y;
  mesure();
  msi.add( acos( skpro(vnow,vfirst) / (vnow.leng * vfirst.leng) ) * 57.3 );
  
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

  // -------------------------------------------------

  // auto off
  if (dsec == DISPLAYDIMSEC && alwaysClock == false) {
    oled->dim(false);
    oled->ssd1306_command(SSD1306_DISPLAYOFF);
  }

  // auto dim
  if (dsec == DISPLAYSEC) oled->dim(true);
  
  // on if move
  if ( abs(lasty - vnow.y) > YMAGDELTA) {
    oled->ssd1306_command(SSD1306_DISPLAYON);
    oled->dim(false);
    dsec = 0;
  }
  
  // auto on every 2min
  if ( (seconds == 0) && (tick == 0) && (minutes%2 == 0) && menu == 0) {
    oled->ssd1306_command(SSD1306_DISPLAYON);
    oled->dim(false);
    dsec = 0;
  }

  // -------------------------------------------------
  
  if (powerlow) {
    analogWrite(LED_RED, 5*tick);
  } else {
    if (menu == 0) {
      if ( (seconds == 0) && (tick < 3) ) {
        digitalWrite(LED_BLUE, HIGH);
      } else if ( (seconds == 0) && (tick == 6) ) {
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, LOW);
      } else if ( (seconds == 0) && (tick > 6) ) {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW);
      }      
    }
  }
  
  // -------------------------------------------------

  if (menu == 0) {
    // clock if not serial terminal is shown
    if (terminalTick == 0) {
      oled->setCursor(0, 0);
      oled->clearDisplay();
      printClock();
      msi.draw(5, 43, 0.75);
      oled->setCursor(10,55);
      oled->setTextSize(1);
      oled->print(steps);
      oled->setCursor(105,57);
      oled->setTextSize(1);
      oled->print(vcc-WARN_POWER);
      oled->print('%');
      oled->display();
    }
  } else if (menu == 1) {
    oled->ssd1306_command(SSD1306_DISPLAYON);
    oled->dim(false);
    dsec = 0;
    oled->clearDisplay();
    printMenu();
    oled->display();
    if (digitalRead(BUTTON2) == LOW) {
      delay(210);
      if (digitalRead(BUTTON2) == LOW) {
        lamp = (lamp+1) % 8;
        if (lamp == 1) {
          digitalWrite(LED_RED,   HIGH);
          digitalWrite(LED_GREEN, LOW);
          digitalWrite(LED_BLUE,  LOW);
        } else if (lamp == 2) {
          digitalWrite(LED_RED,   HIGH);
          digitalWrite(LED_GREEN, HIGH);
          digitalWrite(LED_BLUE,  LOW);
        } else if (lamp == 3) {
          digitalWrite(LED_RED,   HIGH);
          digitalWrite(LED_GREEN, HIGH);
          digitalWrite(LED_BLUE,  HIGH);
        } else if (lamp == 4) {
          digitalWrite(LED_RED,   LOW);
          digitalWrite(LED_GREEN, HIGH);
          digitalWrite(LED_BLUE,  HIGH);
        } else if (lamp == 5) {
          digitalWrite(LED_RED,   HIGH);
          digitalWrite(LED_GREEN, LOW);
          digitalWrite(LED_BLUE,  HIGH);
        } else if (lamp == 6) {
          digitalWrite(LED_RED,   LOW);
          digitalWrite(LED_GREEN, LOW);
          digitalWrite(LED_BLUE,  HIGH);
        } else if (lamp == 7) {
          digitalWrite(LED_RED,   LOW);
          digitalWrite(LED_GREEN, HIGH);
          digitalWrite(LED_BLUE,  LOW);
        } else {
          digitalWrite(LED_RED,   LOW);
          digitalWrite(LED_GREEN, LOW);
          digitalWrite(LED_BLUE,  LOW);
        }
      }
    }
  } else if (menu == 2) {
    oled->ssd1306_command(SSD1306_DISPLAYON);
    oled->dim(false);
    dsec = 0;
    oled->clearDisplay();
    printMenu();
    oled->display();
    if (digitalRead(BUTTON2) == LOW) {
      delay(200);
      if (digitalRead(BUTTON2) == LOW) hours = (hours+1)%24;
    }
  } else if (menu == 3) {
    oled->ssd1306_command(SSD1306_DISPLAYON);
    oled->dim(false);
    dsec = 0;
    oled->clearDisplay();
    printMenu();
    oled->display();
    if (digitalRead(BUTTON2) == LOW) {
      delay(200);
      if (digitalRead(BUTTON2) == LOW) minutes = (minutes+1)%60;
    }
  } else if (menu == 4) {
    oled->ssd1306_command(SSD1306_DISPLAYON);
    oled->dim(false);
    dsec = 0;
    oled->clearDisplay();
    printMenu();
    oled->display();
    if (digitalRead(BUTTON2) == LOW) {
      delay(200);
      if (digitalRead(BUTTON2) == LOW) {
        alwaysClock = true;
        menu = 0;
      }
    }
  } else if (menu == 5) {
    oled->ssd1306_command(SSD1306_DISPLAYON);
    oled->dim(false);
    dsec = 0;
    oled->clearDisplay();
    printMenu();
    oled->display();
    if (digitalRead(BUTTON2) == LOW) {
      delay(200);
      if (digitalRead(BUTTON2) == LOW) {
        alwaysClock = false;
        menu = 0;
      }
    }    
  } else if (menu == 6) {
    oled->ssd1306_command(SSD1306_DISPLAYON);
    oled->dim(false);
    dsec = 0;
    oled->clearDisplay();
    printMenu();
    oled->display();
    if (digitalRead(BUTTON2) == LOW) {
      delay(200);
      if (digitalRead(BUTTON2) == LOW) {
        Serial.println(CHAR_TIME_REQUEST);
        alwaysClock = false;
        menu = 0;
      }
    }
    
  }

  // ---------------------------------------------------------

  if (digitalRead(BUTTON1) == LOW) {
    delay(100);
    if (digitalRead(BUTTON1) == LOW) {
      oled->ssd1306_command(SSD1306_DISPLAYON);
      oled->dim(false);
      dsec = 0;
      delay(300);
      if (digitalRead(BUTTON1) == LOW) {
        menu = (menu+1) % 7;
      }
    }
  }
}

