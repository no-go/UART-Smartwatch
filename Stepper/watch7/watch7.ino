#include <MsTimer2.h>

#include <Wire.h> //I2C Arduino Library
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
#define DISPLAYSEC     10

#define SYS_SWITCH A7
#define MAS_SWITCH A0
#define BLE_LED    7  //unused

#define WARN_POWER 3270.0   // or try 3200, 3500, 3600

// --------------------------------------------------

#define CHAR_TIME_REQUEST     '~'
#define CHAR_TIME_RESPONSE    '#' //#HH:mm:ss
#define CHAR_NOTIFY_HINT      '%' //%[byte]

#define MEMOSTR_LIMIT   115
#define STEP_TRESHOLD   0.125

#define MOVIE_X  3
#define MOVIE_Y  37

int  vcc;
bool powerlow = false;

#define MINDIF  10

int xx,yy,zz;
int minx=400, maxx=0;
int miny=400, maxy=0;
int minz=400, maxz=0;
float changes      = 0.0;
float changes_old  = 0.0;
float delta        = 0.0;
unsigned int steps = 0;

char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos = 0;
char buffMem;
int  cEnd  = 0;
int  cStart= 0;
byte COUNT = 0;

byte hours   = 0;
byte minutes = 0;
byte seconds = 0;
byte tick    = 0;
int  dsec    = DISPLAYSEC;
byte changeDigit = B00000001;

class OledWrapper : public Adafruit_SSD1306 {
  public:

    OledWrapper(const int & dc, const int & res, const int & cs) : Adafruit_SSD1306(dc,res,cs) {}

    void begin() {
      Adafruit_SSD1306::begin(SSD1306_SWITCHCAPVCC);
      clearDisplay();
      setTextSize(1); // 8 line with 21 chars
      setTextColor(WHITE);
      setCursor(0,0);    
    }

    void black(const int & num) {
      setTextColor(BLACK);
      print(num);  
      setTextColor(WHITE);  
    }
    void black(const int & x, const int & y, const int & w, const int & h) {
      fillRect(x,y,w,h, BLACK);
    }
    void line(const int & x, const int & y, const int & xx, const int & yy) {
      drawLine(x,y,xx,yy, WHITE);
    }
    void pixel(const int & x, const int & y) {
      drawPixel(x,y, WHITE);
    }
    void rect(const int & x, const int & y, const int & w, const int & h) {
      drawRect(x,y,w,h, WHITE);
    }
    void rectFill(const int & x, const int & y, const int & w, const int & h) {
      fillRect(x,y,w,h, WHITE);
    }
    void circle(const int & x, const int & y, const int & radius) {
      drawCircle(x,y,radius, WHITE);
    }
    void setFontType(const int & t) {
      setTextSize(t);
    }
    void on() {
      ssd1306_command(SSD1306_DISPLAYON);
    }
    void off() {
      ssd1306_command(SSD1306_DISPLAYOFF);
    }
    void clear() {
      clearDisplay();
    }
    void command(uint8_t cmd) {
      ssd1306_command(cmd);
    }
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
};

OledWrapper * oled = new OledWrapper(PIN_DC, PIN_RESET, OLED_CS);

enum {MSG_NO, MSG_CAL, MSG_MAIL, MSG_SMS, MSG_OTHER};
int iconType = MSG_NO;

static const uint8_t PROGMEM move0a[] = {
B00100100,
B00111100,
B01011010,
B01011010,
B00111100,
B00011000,
B00111100,
B01011010,
B00011000,
B00100100,
B00100100,
B01100110
};
static const uint8_t PROGMEM move0b[] = { // taps
B00100100,
B00111100,
B01011010,
B01011010,
B00111100,
B00011000,
B00111100,
B01011010,
B00011000,
B00100100,
B01100100,
B00000110
};
static const uint8_t PROGMEM move0c[] = { // blink
B00100100,
B00111100,
B01111110,
B01111110,
B00111100,
B00011000,
B00111100,
B01011010,
B00011000,
B00100100,
B00100100,
B01100110
};
static const uint8_t PROGMEM move1[] = {
B00111000,
B01111100,
B01111010,
B01111010,
B00111100,
B00111000,
B00111110,
B00111000,
B00011000,
B01111100,
B10000010,
B00000011
};
static const uint8_t PROGMEM move2[] = {
B00111000,
B01111100,
B01111010,
B01111010,
B00111100,
B00111000,
B00111110,
B00111000,
B00011000,
B01110100,
B01000100,
B01000110
};
static const uint8_t PROGMEM move3[] = {
B00111000,
B01111100,
B01111010,
B01111010,
B00111100,
B00111000,
B00111110,
B00111000,
B00011000,
B01111100,
B01111100,
B01110000
};

static const uint8_t PROGMEM icon_calendar[] = {
B11111111,
B11111111,
B10000001,
B10000001,
B10000101,
B10000001,
B11111111,
B00000000
};
static const uint8_t PROGMEM icon_mail[] = {
B11111111,
B10000001,
B11000011,
B10100101,
B10011001,
B10000001,
B11111111,
B00000000
};
static const uint8_t PROGMEM icon_messaging[] = {
B00111110,
B01000001,
B01000001,
B01000001,
B01000001,
B01011110,
B01100000,
B01000000
};
static const uint8_t PROGMEM icon_other[] = {
B00000111,
B00011011,
B01100101,
B10001001,
B01010001,
B00110001,
B00101101,
B00110011  
};


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
    if (steps%20 == 0) {
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
    
    memoStr[memoStrPos] = oled->umlReplace(inChar);
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
  
  if (tick > 9) {
    seconds += tick/10;
    dsec++;
    changeDigit = B00000001;
    if (seconds == 10 || seconds == 20 || seconds == 30 || seconds == 40 || seconds == 50) changeDigit = B00000011;
  }
  
  if (tick > 9) {
    tick = tick % 10;
    if (seconds > 59) {
      minutes += seconds / 60;
      seconds  = seconds % 60;
      if (minutes%5 == 0) Serial.println(steps);

      changeDigit = B00000111;
      if (minutes == 10 || minutes == 20 || minutes == 30 || minutes == 40 || minutes == 50) changeDigit = B00001111;

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
      changeDigit = B00011111;
      if (hours == 10 || hours == 20 || hours == 24) changeDigit = B00111111;
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

int myFont(int x, int y, byte b, byte pos) {
    int he = 18;
    int yold = y;
    if (tick < 8 && ((pos & changeDigit) != B00000000)) {
      y = y - he*((7.0 - (float)tick)/7.0);
    }
    
    if (b == 0) {
      oled->circle(x+he/4, y+3*he/4, he/4);
      return x+2+he/2;
    } else if (b == 1) {
      oled->line(x,y+5,x+5,y);
      oled->line(x+5,y,x+5,y+he);
      return x+8;
    } else if (b == 2) {
      oled->circle(x+he/4, y-3+3*he/4, he/4);
      oled->black(x,y,he/4,he);
      oled->line(x+he/4,y+he-3,x+he/2,y+he);
      return x+2+he/2;
    } else if (b == 3) {
      oled->circle(x+he/4, y+1*he/4, he/4);
      oled->circle(x+he/4, y+3*he/4, he/4);
      oled->black(x,y,he/4,he);
      return x+2+he/2;
    } else if (b == 4) {
      oled->line(x,y+he/2,x+he/2,y);
      oled->line(x,y+he/2,x+he/2,y+he/2);
      oled->line(x+he/2,y,x+he/2,y+he);
      return x+2+he/2;   
    } else if (b == 5) {
      oled->line(x+he/4,y,x+he/2,y);
      oled->line(x+he/4,y,x+he/4,y+he/2);
      oled->circle(x+he/4, y+3*he/4, he/4);
      oled->black(x,y,he/4,he);
      return x+2+he/2;   
    } else if (b == 6) {
      oled->line(x,y-2+3*he/4,x+he/2,y);
      oled->circle(x+he/4, y+3*he/4, he/4);
      return x+2+he/2;   
    } else if (b == 7) {
      oled->line(x,y+3,x+he/2,y);
      oled->line(x+he/2,y,x,y+he);
      oled->line(x+3,y+he/2,x+he/2-1,y+he/2);
      return x+2+5;  
    } else if (b == 8) {
      oled->circle(x+he/4, y+1*he/4, he/4);
      oled->circle(x+he/4, y+3*he/4, he/4);
      return x+2+he/2;
    } else if (b == 9) {
      oled->circle(x+he/4, y+1*he/4, he/4);
      oled->line  (x+he/2, y+1*he/4, x+he/2, y+he);
      return x+2+he/2;
    }
}

void anaClock() {
  byte x = 96;
  byte y = 38;
  byte radius = 25;
  oled->circle(x, y, radius);
  int hour = hours;
  if (hour>12) hour-=12;
  oled->line(
    x, y,
    x + (radius-2)*cos(PI * ((float)seconds-15.0) / 30),
    y + (radius-2)*sin(PI * ((float)seconds-15.0) / 30)
  );
  
  oled->line(
    x, y,
    x + (radius-5)*cos(PI * ((float)minutes-15.0) / 30),
    y + (radius-5)*sin(PI * ((float)minutes-15.0) / 30)
  );
  
  oled->line(
    x, y,
    x + (radius-14)*cos(PI * ((float)hour+((float)minutes/60-0) -3.0) / 6),
    y + (radius-14)*sin(PI * ((float)hour+((float)minutes/60.0) -3.0) / 6)
  );
  
  for (byte i=0; i<12; ++i) {
    oled->pixel(x + (radius-3)*cos(PI * ((float)i) / 6), y + (radius-3)*sin(PI * ((float)i) / 6));  
  }  
}

void printClock() {
  int xx = myFont(1, 2, hours/10, B00100000);
  myFont(xx, 2, hours - 10*(hours/10), B00010000);
  
  xx = myFont(27, 2, minutes/10, B00001000);
  myFont(xx, 2, minutes - 10*(minutes/10), B00000100);
  
  xx = myFont(53, 2, seconds/10, B00000010);
  myFont(xx, 2, seconds - 10*(seconds/10), B00000001);

  anaClock();
  
  oled->setTextSize(1);
  if (COUNT > 0) {
    oled->setCursor(97, 2);
    oled->print(COUNT);  
  }

  if (delta > 0.2) {
    oled->on();
    dsec = 0;
  }

  if (delta > 0.05) {
    // moving
    if (tick==0 || tick==1) {
      oled->drawBitmap(MOVIE_X+2*tick, MOVIE_Y, move1, 8, 12, WHITE);
    } else if (tick==2 || tick==3) {
      oled->drawBitmap(MOVIE_X+2*tick, MOVIE_Y, move2, 8, 12, WHITE);
    } else if (tick==6 || tick==7 || tick==9) {
      oled->drawBitmap(MOVIE_X+2*tick, MOVIE_Y, move2, 8, 12, WHITE);
    } else if (tick == 8) {
      oled->drawBitmap(MOVIE_X+2*tick, MOVIE_Y, move1, 8, 12, WHITE);
    } else {
      oled->drawBitmap(MOVIE_X+2*tick, MOVIE_Y, move3, 8, 12, WHITE);      
    }    
  } else {
    if (tick == 2) {
      // tap
      oled->drawBitmap(MOVIE_X, MOVIE_Y, move0b, 8, 12, WHITE);
    } else if (tick == 4) {
      // tap
      oled->drawBitmap(MOVIE_X, MOVIE_Y, move0b, 8, 12, WHITE);
    } else if (tick == 6) {
      // tap
      oled->drawBitmap(MOVIE_X, MOVIE_Y, move0b, 8, 12, WHITE);
    } else if (tick == 8) {
      // blink
      oled->drawBitmap(MOVIE_X, MOVIE_Y, move0c, 8, 12, WHITE);
    } else {
      // normal
      oled->drawBitmap(MOVIE_X, MOVIE_Y, move0a, 8, 12, WHITE);      
    }
  }
  
  oled->setCursor(5,55);
  oled->print(steps);
  
  oled->setCursor(50,55);
  oled->print(vcc/34); //3400 -> 100%
  oled->print('%');
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

int slen(char * str) {
  int i;
  for (i=0;str[i] != '\0' && i < 1000; ++i);
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

  digitalWrite(LED_RED,   LED_OFF);
  digitalWrite(LED_GREEN, LED_OFF);
  digitalWrite(LED_BLUE,  LED_OFF);

  mag.begin();
  delay(7);
  
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
    oled->off();
  }

  oled->setCursor(0, 0);
  
  if (digitalRead(BUTTON1) == LOW) {
    oled->on();
    dsec = 0;
    digitalWrite(LED_RED,   LED_OFF);
    digitalWrite(LED_GREEN, LED_OFF);
    digitalWrite(LED_BLUE,  LED_OFF);
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
      oled->drawBitmap(86, 2, icon_calendar, 8, 8, WHITE);
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
      oled->drawBitmap(86, 2, icon_messaging, 8, 8, WHITE);
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
      oled->drawBitmap(86, 2, icon_mail, 8, 8, WHITE);
      iconType = MSG_MAIL;
    } else if (
      (l > 5) || iconType == MSG_OTHER
    ) {
      oled->drawBitmap(86, 2, icon_other, 8, 8, WHITE);
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

