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
#define DISPLAYSEC     20

#define SYS_SWITCH A7
#define MAS_SWITCH A0
#define BLE_LED    7  //unused

#define MAX_POWER        3380.0   // or try 3340, 4300, 5000
#define WARN_POWER       3270.0   // or try 3200, 3500, 3600
#define MIN_POWER        3240.0   // or try 2740, 3200, 3600

// --------------------------------------------------

#define CHAR_TIME_REQUEST     '~'
#define CHAR_TIME_RESPONSE    '#' //#HH:mm:ss
#define CHAR_NOTIFY_HINT      '%' //%[byte]

#define MEMOSTR_LIMIT   110
#define STEP_TRESHOLD   0.125

#define MOVIE_X  7
#define MOVIE_Y  41

int  vcc;
bool powerlow = false;

#define MINDIF    10

int xx,yy,zz;
int minx=400, maxx=0;
int miny=400, maxy=0;
int minz=400, maxz=0;
float changes      = 0.0;
float changes_old  = 0.0;
float delta        = 0.0;
unsigned int steps = 0;

struct Orient {
  int x,y,z;  
};

enum {NORT, EAS, SOU, WES, FIN};

byte storeMode = NORT;
Orient north, east, south, west;

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




static const uint8_t PROGMEM move0a[] = {
B00001100,B00110000,
B00001100,B00110000,
B00001111,B11110000,
B00001111,B11110000,
B00110011,B11001100,
B00110011,B11001100,
B00110011,B11001100,
B00110011,B11001100,
B00001111,B11110000,
B00001111,B11110000,
B00000011,B11000000,
B00000011,B11000000,
B00001111,B11110000,
B00001111,B11110000,
B00110011,B11001100,
B00110011,B11001100,
B00000011,B11000000,
B00000011,B11000000,
B00001100,B00110000,
B00001100,B00110000,
B00001100,B00110000,
B00001100,B00110000,
B00111100,B00111100,
B00111100,B00111100
};
static const uint8_t PROGMEM move0b[] = { // taps
B00001100,B00110000,
B00001100,B00110000,
B00001111,B11110000,
B00001111,B11110000,
B00110011,B11001100,
B00110011,B11001100,
B00110011,B11001100,
B00110011,B11001100,
B00001111,B11110000,
B00001111,B11110000,
B00000011,B11000000,
B00000011,B11000000,
B00001111,B11110000,
B00001111,B11110000,
B00110011,B11001100,
B00110011,B11001100,
B00000011,B11000000,
B00000011,B11000000,
B00001100,B00110000,
B00001100,B00110000,
B00111100,B00110000,
B00111100,B00110000,
B00000000,B00111100,
B00000000,B00111100
};
static const uint8_t PROGMEM move0c[] = { // blink
B00001100,B00110000,
B00001100,B00110000,
B00001111,B11110000,
B00001111,B11110000,
B00111111,B11111100,
B00111111,B11111100,
B00111111,B11111100,
B00111111,B11111100,
B00001111,B11110000,
B00001111,B11110000,
B00000011,B11000000,
B00000011,B11000000,
B00001111,B11110000,
B00001111,B11110000,
B00110011,B11001100,
B00110011,B11001100,
B00000011,B11000000,
B00000011,B11000000,
B00001100,B00110000,
B00001100,B00110000,
B00001100,B00110000,
B00001100,B00110000,
B00111100,B00111100,
B00111100,B00111100
};
static const uint8_t PROGMEM move1[] = {
B00001111,B11000000,
B00001111,B11000000,
B00111111,B11110000,
B00111111,B11110000,
B00111111,B11001100,
B00111111,B11001100,
B00111111,B11001100,
B00111111,B11001100,
B00001111,B11110000,
B00001111,B11110000,
B00001111,B11000000,
B00001111,B11000000,
B00001111,B11111100,
B00001111,B11111100,
B00001111,B11000000,
B00001111,B11000000,
B00000011,B11000000,
B00000011,B11000000,
B00111111,B11110000,
B00111111,B11110000,
B11000000,B00001100,
B11000000,B00001100,
B00000000,B00001111,
B00000000,B00001111
};
static const uint8_t PROGMEM move2[] = {
B00001111,B11000000,
B00001111,B11000000,
B00111111,B11110000,
B00111111,B11110000,
B00111111,B11001100,
B00111111,B11001100,
B00111111,B11001100,
B00111111,B11001100,
B00001111,B11110000,
B00001111,B11110000,
B00001111,B11000000,
B00001111,B11000000,
B00001111,B11111100,
B00001111,B11111100,
B00001111,B11000000,
B00001111,B11000000,
B00000011,B11000000,
B00000011,B11000000,
B00111111,B00110000,
B00111111,B00110000,
B00110000,B00110000,
B00110000,B00110000,
B00110000,B00111100,
B00110000,B00111100
};
static const uint8_t PROGMEM move3[] = {
B00001111,B11000000,
B00001111,B11000000,
B00111111,B11110000,
B00111111,B11110000,
B00111111,B11001100,
B00111111,B11001100,
B00111111,B11001100,
B00111111,B11001100,
B00001111,B11110000,
B00001111,B11110000,
B00001111,B11000000,
B00001111,B11000000,
B00001111,B11111100,
B00001111,B11111100,
B00001111,B11000000,
B00001111,B11000000,
B00000011,B11000000,
B00111111,B11110000,
B00111111,B11110000,
B00111111,B11110000,
B00111111,B11110000,
B00111111,B00000000,
B00111111,B00000000
};

enum {MSG_NO, MSG_CAL, MSG_MAIL, MSG_SMS, MSG_OTHER};
int iconType = MSG_NO;

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

void power(int x, int y) {
  if (vcc <= MIN_POWER) {
    vcc = 0;
  } else if(vcc >= MAX_POWER) {
    vcc = MAX_POWER;
  } else {
    vcc = 35 * (float)(vcc-MIN_POWER)/(MAX_POWER-MIN_POWER);
  }
  oled->rect(    x,   y,        8, 38);
  oled->rectFill(x+2, y+36-vcc, 4, vcc);    
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
  }
  
  if (tick > 9) {
    tick = tick % 10;
    if (seconds > 59) {
      minutes += seconds / 60;
      seconds  = seconds % 60;
      if (minutes%5 == 0) Serial.println(steps);
      
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
void printDirection() {
  oled->setTextSize(1);
  oled->setCursor(50, 29);
  if (
    north.x > xx-5 && north.x < xx+5 &&
    north.y > yy-5 && north.y < yy+5 &&
    north.z > zz-5 && north.z < zz+5
  ) { oled->print('N'); oled->print('o'); oled->print('r'); oled->print('t'); oled->print('h'); }

  if (
    east.x > xx-5 && east.x < xx+5 &&
    east.y > yy-5 && east.y < yy+5 &&
    east.z > zz-5 && east.z < zz+5
  ) { oled->print('E'); oled->print('a'); oled->print('s'); oled->print('t'); }

  if (
    south.x > xx-5 && south.x < xx+5 &&
    south.y > yy-5 && south.y < yy+5 &&
    south.z > zz-5 && south.z < zz+5
  ) { oled->print('S'); oled->print('o'); oled->print('u'); oled->print('t'); oled->print('h'); }

  if (
    west.x > xx-5 && west.x < xx+5 &&
    west.y > yy-5 && west.y < yy+5 &&
    west.z > zz-5 && west.z < zz+5
  ) { oled->print('W'); oled->print('e'); oled->print('s'); oled->print('t'); }

}

int myFont(int x, int y, byte b) {
    int he = 25;
    if (b>9) {
      x = myFont(x,y,b/10);
      b = b%10;
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

void printClock() {
  oled->line(17, 0, 17 + tick*10, 0);

  if (hours<10) {
    int xx = myFont(17, 2, 0);
    myFont(xx, 2, hours);
  } else {
    myFont(17, 2, hours);
  }
  if (minutes<10) {
    int xx = myFont(52, 2, 0);
    myFont(xx, 2, minutes);
  } else {
    myFont(52, 2, minutes);
  }
  if (seconds<10) {
    int xx = myFont(86, 2, 0);
    myFont(xx, 2, seconds);
  } else {
    myFont(86, 2, seconds);
  }
  
  printDirection();
  
  oled->setTextSize(1);
  if (COUNT > 0) {
    oled->setCursor(2,14);
    oled->print(COUNT);  
  }

  oled->setCursor(119,0);
  switch(storeMode) {
    case NORT:
      oled->print('n');
      break;
    case EAS:
      oled->print('e');
      break;
    case SOU:
      oled->print('s');
      break;
    default:
      oled->print('w');
  }
  
  if (delta > 0.05) {
    // moving
    if (tick==0 || tick==1) {
      oled->drawBitmap(MOVIE_X+2*tick, MOVIE_Y, move1, 16, 24, WHITE);
    } else if (tick==2 || tick==3) {
      oled->drawBitmap(MOVIE_X+2*tick, MOVIE_Y, move2, 16, 24, WHITE);
    } else if (tick==6 || tick==7 || tick==9) {
      oled->drawBitmap(MOVIE_X+2*tick, MOVIE_Y, move2, 16, 24, WHITE);
    } else if (tick == 8) {
      oled->drawBitmap(MOVIE_X+2*tick, MOVIE_Y, move1, 16, 24, WHITE);
    } else {
      oled->drawBitmap(MOVIE_X+2*tick, MOVIE_Y, move3, 16, 24, WHITE);      
    }    
    oled->setTextSize(2);
    oled->setCursor(31+2*tick,45);
    oled->print(steps);
    
  } else {
    if (tick == 2) {
      // tap
      oled->drawBitmap(MOVIE_X, MOVIE_Y, move0b, 16, 24, WHITE);
    } else if (tick == 4) {
      // tap
      oled->drawBitmap(MOVIE_X, MOVIE_Y, move0b, 16, 24, WHITE);
    } else if (tick == 6) {
      // tap
      oled->drawBitmap(MOVIE_X, MOVIE_Y, move0b, 16, 24, WHITE);
    } else if (tick == 8) {
      // blink
      oled->drawBitmap(MOVIE_X, MOVIE_Y, move0c, 16, 24, WHITE);
    } else {
      // normal
      oled->drawBitmap(MOVIE_X, MOVIE_Y, move0a, 16, 24, WHITE);      
    }
    oled->setTextSize(2);
    oled->setCursor(31,45);
    oled->print(steps);
  }
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
  oled->drawBitmap(56, 20, move0a, 16, 24, WHITE);
  oled->display();

  digitalWrite(SYS_SWITCH, HIGH);
  digitalWrite(MAS_SWITCH, HIGH);
  delay(1200);
  Serial.println("AT+NAMEStep Watch");
  Serial.println("AT+ROLE0");

  oled->clearDisplay();
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
  delay(151);
  readVcc();
  
  if (seconds == 10) {
    if (vcc < WARN_POWER) {
      powerlow = true;
    } else {
      powerlow = false;
    }
  }
  
  oled->clear();
  printClock();
  oled->setTextSize(1);
  mesure();
  
  oled->line(121,23,124,23);
  oled->line(121,24,124,24);
  power(119,25);
  
  if (dsec < DISPLAYSEC && cStart < memoStrPos) {
    oled->setCursor(0,22);
    oled->black(0,22,128,42);
    oled->print(&(memoStr[cStart]));
  }

  if (dsec == DISPLAYSEC) {
    memoStr[0] = '\0';
    memoStrPos = 0;
  }

  oled->setCursor(0, 0);
  
  if (digitalRead(BUTTON2) == LOW) {
    delay(300);  
    if (digitalRead(BUTTON2) == LOW) {
      digitalWrite(LED_RED,   LED_ON);
      digitalWrite(LED_GREEN, LED_ON);
      digitalWrite(LED_BLUE,  LED_ON);
      switch(storeMode) {
        case NORT:
          north.x = xx;
          north.y = yy;
          north.z = zz;
          break;
        case EAS:
          east.x = xx;
          east.y = yy;
          east.z = zz;
          break;
        case SOU:
          south.x = xx;
          south.y = yy;
          south.z = zz;
          break;
        case WES:
          west.x = xx;
          west.y = yy;
          west.z = zz;
          break;
        default:
          ;
      }
      delay(200);
      digitalWrite(LED_RED,   LED_OFF);
      digitalWrite(LED_GREEN, LED_OFF);
      digitalWrite(LED_BLUE,  LED_OFF);
      storeMode++;
      if (storeMode == FIN) storeMode = NORT;
    }
  }

  if (digitalRead(BUTTON1) == LOW) {
    delay(300);  
    if (digitalRead(BUTTON1) == LOW) {
      
      COUNT = 0;
      digitalWrite(LED_RED,   LED_OFF);
      digitalWrite(LED_GREEN, LED_OFF);
      digitalWrite(LED_BLUE,  LED_OFF);

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
      oled->drawBitmap(2, 5, icon_calendar, 8, 8, WHITE);
      iconType = MSG_CAL;
    }
    if (
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
      oled->drawBitmap(2, 5, icon_messaging, 8, 8, WHITE);
      iconType = MSG_SMS;
    }
    if (
      (l > 7 &&
      memoStr[l-7] == 'f' &&
      memoStr[l-6] == 's' &&
      memoStr[l-5] == 'c' &&
      memoStr[l-4] == 'k' &&
      memoStr[l-3] == '.' &&
      memoStr[l-2] == 'k' &&
      memoStr[l-1] == '9') || iconType == MSG_MAIL
    ) {
      oled->drawBitmap(2, 5, icon_mail, 8, 8, WHITE);
      iconType = MSG_MAIL;
    }
    if (
      (l > 9 &&
      memoStr[l-9] == 'm' &&
      memoStr[l-8] == 'e' &&
      memoStr[l-7] == 's' &&
      memoStr[l-6] == 's' &&
      memoStr[l-5] == 'e' &&
      memoStr[l-4] == 'n' &&
      memoStr[l-3] == 'g' &&
      memoStr[l-2] == 'e' &&
      memoStr[l-1] == 'r') || iconType == MSG_OTHER
    ) {
      oled->drawBitmap(2, 5, icon_other, 8, 8, WHITE);
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

