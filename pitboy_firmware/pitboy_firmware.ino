#include <MsTimer2.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/sleep.h>

// ---------------------------- Configure YOUR CONNECTIONS !! -------

#define BUTTON1     2
#define BUTTON2     3

#define LED_RED    10

// OLED (11 -> MOSI/DIN, 13 ->SCK)
#define PIN_CS     4
#define PIN_RESET  6
#define PIN_DC     8

const int scrollSpeed =  40;
#define SECtoSLEEP       60     // = quarter seconds!!
#define BLE_UART_SPEED   9600   // or try 115200
#define TIME_PITCH       247    // 987 is 1000ms = 1 sec (realy ?)
#define MAX_POWER        4200   // or try 3340, 4300, 5000
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
bool ledSwitch = true;

byte hours   = 42;
byte minutes = 42;
byte seconds = 42;
byte tick = 0;
  
int countToSleep = 0;

static const uint8_t PROGMEM bitmap[] = {
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000001,B10000000,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000010,B01100000,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00111100,B00011000,B00000000,B00000000,B00000000,B00000000,
B00000000,B00011000,B11000000,B00000000,B00000000,B00000000,B00000000,B00000110,B01000000,B00000100,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00001010,B10000000,B00000010,B00000000,B00000000,B00000000,B00000000,
B00000000,B01111000,B11111110,B00000000,B00000000,B00000000,B00000000,B00010001,B00000000,B00000001,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00010000,B00000000,B00000000,B10000000,B00000000,B00000000,B00000000,
B00001000,B01111000,B11111111,B10000000,B00000000,B00000000,B00000000,B00010000,B00000000,B00000000,B01000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00001000,B00000000,B00000000,B01000000,B00000000,B00000000,B00000000,
B00111000,B01111000,B11111111,B11100000,B00000000,B00000000,B00000000,B00010000,B01100000,B01000000,B01000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00010000,B01000000,B01100000,B00100000,B00000000,B00000000,B00000000,
B01111000,B01111000,B11111111,B11110000,B00000000,B00000000,B00000000,B00001000,B00000000,B00100000,B00100000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00001000,B10000000,B00000000,B00100000,B00000000,B00000000,B00000000,
B11111000,B01111000,B11111111,B10000000,B00000000,B00000000,B00000000,B00001000,B11001000,B11000000,B01000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00010000,B10011000,B11000000,B01000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00010000,B00110000,B10000000,B01000000,B00000000,B00000000,B00000000,
B11111111,B11111000,B11111111,B10000000,B00000000,B00000000,B00000000,B00100000,B00100000,B00000000,B01000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00100000,B00100000,B00000000,B10000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00100000,B00010000,B00000000,B10000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00100011,B00000000,B10000000,B10000000,B00000000,B00000000,B00000000,
B11111111,B11111000,B11111111,B11111000,B00000000,B00000000,B00000000,B00100011,B11111111,B10000000,B10000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00010001,B10000001,B11000000,B10000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00010000,B01000100,B00000111,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00001000,B00110000,B00001000,B00000000,B00000000,B00000000,B00000000,
B00000000,B11111000,B11111111,B11100000,B00000000,B00000000,B00000000,B00000100,B00000000,B00001000,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00110000,B00000000,B00000000,B00000010,B00000000,B00111000,B00000000,B00000000,B00000011,B00000000,
B00000000,B00000000,B00000000,B00000000,B01001000,B00000000,B00000000,B00001100,B00000000,B00000111,B10000000,B00000000,B00000100,B10000000,
B00000000,B11111000,B11111111,B00000000,B01001100,B00000000,B00000011,B11110000,B00000000,B00000000,B01111000,B00000000,B00001000,B10000000,
B00000000,B00000000,B00000000,B00000000,B01000111,B11000000,B01111100,B00000000,B00000000,B00000000,B00000111,B00000000,B01110001,B00000000,
B00000000,B00000000,B00000000,B00000001,B10110000,B00111111,B10000000,B00000000,B00000000,B00000000,B00000000,B11111111,B00000011,B11111100,
B00000000,B00011000,B11000000,B00000010,B01000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000010,B00000100,
B00000000,B00000000,B00000000,B00000010,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00011000,
B00000000,B00000000,B00000000,B00000001,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00010000,
B00000000,B00000000,B00000000,B00000000,B11100000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B11100000,
B00000000,B00000000,B00000000,B00000001,B00000111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00011000,
B00000000,B00000000,B00000000,B00000000,B11111000,B11000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00111110,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00110000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B01110001,B11110000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00011100,B00000001,B10000000,B00000000,B00000000,B00000000,B00000011,B11000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000011,B11000110,B01000000,B00000000,B00000000,B00000000,B00001110,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00111000,B01000000,B00000000,B00000000,B11111111,B11110000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B01000000,B00000000,B00000001,B10000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B01000000,B00000000,B00000001,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00100000,B00000000,B00000001,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00100000,B00000000,B00000001,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00100000,B00000000,B00000001,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00100000,B00000000,B00000001,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00010000,B00000000,B00000001,B00000000,B00000000,B00000000,B00000000,
B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00010000,B00000000,B00000001,B00000000,B00000000,B00000000,B00000000
};

struct OledWrapper {

    Adafruit_SSD1306 * _oled;
    
    OledWrapper() {
       _oled = new Adafruit_SSD1306(PIN_DC, PIN_RESET, PIN_CS);
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

void printClock() {
  // and small digital
  oled.setCursor(0,0);
  oled.setFontType(2);
  oled.print(hours);
  oled.print(':');
  if(minutes < 10) oled.print('0');
  oled.print(minutes);
  oled.setFontType(1);
  oled.setCursor(115,8);
  if(seconds < 10) oled.print('0');
  oled.print(seconds);
  
  oled._oled->drawBitmap(14, 15, bitmap, 112, 48, WHITE);
  oled.line(56, 35, 57+(hours%12), 35);
  oled.line(56, 36, 57+(hours%12), 36);
  
  oled.line(80, 48, 81+minutes/5 , 48);
  oled.line(80, 49, 81+minutes%12, 49);
  
  oled.line(105, 30, 106+(seconds/5),  30);
  oled.line(105, 31, 106+(seconds%12), 31);

  oled.black(14,15+5*tick,30,10);
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
  oled.pixel   (oled.width() - batLength, 3); 
  oled.pixel   (oled.width() - batLength, 2);
  oled.rect    (oled.width() - batLength+1, 0, batLength-1, 6);  
  oled.rectFill(oled.width() - vccVal   -1, 1,      vccVal, 4);
}

void printTemp() {
  unsigned int wADC;
  float t;
  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);            // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));
  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;
  // The offset of 324.31 could be wrong. It is just an indication.
  t = ( wADC - 91.31 ) / 1.22;
  // The returned temperature is in degrees Celcius.
  oled.setCursor(0, oled.height()-10);
  oled.print(t,2); 
  oled.print(' ');
  oled.print('C');
}

void getIcon() {
  oled.clear();
  oled.line(0, oled.height()/2, 8, oled.height()/2);
  oled.display();
  delay(20);
  oled.line(0, oled.height()/2, 16, oled.height()/2);
  oled.display();
  delay(10);
  oled.line(0, oled.height()/2, 32, oled.height()/2);
  oled.display();
  delay(10);
  oled.line(0, oled.height()/2, 64, oled.height()/2);
  oled.display();
  delay(10);
  oled.circle(oled.width()/2, oled.height()/2, 5);
  oled.display();
  delay(30);
  oled.circle(oled.width()/2, oled.height()/2, 10);
  oled.display();
  delay(20);
  oled.circle(oled.width()/2, oled.height()/2, 15);
  oled.display();  
  delay(20);
  oled.circle(oled.width()/2, oled.height()/2, 20);
  oled.display();  
  delay(20);
  oled.circle(oled.width()/2, oled.height()/2, 25);
  oled.display();  
  delay(100);
}

byte tob(char c) {
  return c - '0';
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
  tick++;
  if(tick>=4) {
    tick=0;
    seconds++;
  }
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
    }
    printClock();
    batteryIcon();
    printTemp();
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
  oled.rect(0, 0, oled.width()-1, oled.height()-1);
}

void eye2() {
  oled.rect(oled.width()/4, oled.height()/4, oled.width()/2, oled.height()/2);
}

void eye3() {
  oled.rect(10, oled.height()/2, oled.width()-20, 1);
}


void setup() {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);
  attachInterrupt(1, wakeUpNow, HIGH); // INT1 is on PIN3
  Serial.begin(BLE_UART_SPEED);
  Serial.println("AT+NAMEPitBoy Watch");

  MsTimer2::set(TIME_PITCH, ticking); // 250ms period
  MsTimer2::start();
  oled.begin();
  buffMem = '\0';
}

void loop() {    
  if (digitalRead(BUTTON1) == LOW) {
    delay(300);
    if (digitalRead(BUTTON1) == LOW) {
      ledSwitch = !ledSwitch;
      digitalWrite(LED_RED, ledSwitch);
    }
  }
  if (digitalRead(BUTTON2) == LOW) {
    delay(300);
    if (digitalRead(BUTTON2) == LOW) {
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
    digitalWrite(LED_RED, LOW);
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
    oled.clear();
    oled.display();
    MsTimer2::start();
    Serial.println( CHAR_TIME_REQUEST );
  }

}


