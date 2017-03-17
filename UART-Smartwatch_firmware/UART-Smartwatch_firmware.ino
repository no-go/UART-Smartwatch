// ---------------------------- Configure YOUR CONNECTIONS !! -------

#define BUTTON1    A0
#define BUTTON2    A1

#define LED_RED    10
#define LED_GREEN   9 
#define LED_BLUE    3

// OLED (11 -> MOSI/DIN, 13 ->SCK)
#define PIN_CS     5
#define PIN_RESET  6
#define PIN_DC     8
#define DC_JUMPER  0

#define CHAR_TIME_REQUEST     'T' //old - unused: idea was to make a deep sleep and get only the time on demand
#define CHAR_MSG_REQUEST      '~' //new
#define CHAR_TIME_RESPONSE    '#' //#HH:mm:ss
#define CHAR_NOTIFY_HINT      '%' //%[byte]

// ------------------------------------------------------

#define MESSAGEPOS     20
#define MEMOSTR_LIMIT 270 /// @todo:  BAD BAD ! why did ssd1306 lib take so much dyn ram ??
#include <EEPROM.h>
int eeAddress = 0;
int score     = 0;
int highscore = 0;
void game();

const int batLength   = 45;
const int scrollSpeed = 70; // 100 = 100ms each char

/*
  MsTimer2 is a small and very easy to use library to interface Timer2 with
  humans. It's called MsTimer2 because it "hardcodes" a resolution of 1
  millisecond on timer2
  For Details see: http://www.arduino.cc/playground/Main/MsTimer2
*/
#include <MsTimer2.h>


#include <avr/power.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


struct OledWrapper {

    Adafruit_SSD1306 * _oled;
    
    OledWrapper() {
       _oled = new Adafruit_SSD1306(PIN_DC, PIN_RESET, PIN_CS);
    }
    
    void begin() {
      _oled->begin(SSD1306_SWITCHCAPVCC);
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

char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos   = MESSAGEPOS;
int  page         = 0;
byte COUNT        = 0;

byte hours   = 0;
byte minutes = 0;
byte seconds = 0;
byte tick    = 0;

// 0=digi, 1=analog, 2=digi 4 ever, 3=adjust_hour, 4=adjust_min
int clockMode = 0;

int redValue   = 255;
int greenValue = 255;
int blueValue  = 255;

// Check it -------------------------
// 0 always on
// 1 = 100 ms of a second on
// 2 = 200 ms of a second on
// 9 = 900 ms of a second on
int delayValue = 1;

byte powerTick(int mv) {
  //float quot = (5100-2700)/(batLength-3); // scale: 5100 -> batLength, 2710 -> 0
  float quot = (3400-2800)/(batLength-3);
  return (mv-2800)/quot;  
}

int readVcc() {
  int mv;
  power_adc_enable();
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(10); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
  mv = ADCL; 
  mv |= ADCH<<8; 
  mv = 1126400L / mv;
  power_adc_disable();
  return powerTick(mv);
}

void anaClock() {
  byte x = 60;
  byte y = 32;
  byte radius = 32;
  //oled.circle(x, y, radius);
  int hour = hours;
  if (hour>12) hour-=12;
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
  
  oled.line (0, oled.height()-2, batLength*(tick/9.0), oled.height()-2);
}

void filler() {
  for (int i=0; i<MESSAGEPOS; ++i) {
    memoStr[i] = ' ';
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

void setup() {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  
  Serial.begin(9600);

  power_adc_disable();
  power_twi_disable();
  
  MsTimer2::set(100, ticking); // 100ms period
  MsTimer2::start();
    
  oled.begin();
  oled.free(); // Clear the display's internal memory logo
  oled.display();
  filler();
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == -61) continue; // symbol before utf-8
    if (inChar == -62) continue; // other symbol before utf-8
    if (inChar == '\n') {
      oled.on();
      memoStr[memoStrPos] = '\0';
      page = 0;
      continue;
    }
    memoStr[memoStrPos] = oled.umlReplace(inChar);
    memoStrPos++;
    if (memoStrPos >= MEMOSTR_LIMIT) memoStrPos = MESSAGEPOS;
  }
}

void digiClock() {
  if (hours<10) {
    int xx = bprint(0, 10, 0);
    bprint(xx, 10, hours);
  } else {
    bprint(0, 10, hours);
  }
  if (minutes<10) {
    int xx = bprint(45, 10, 0);
    bprint(xx, 10, minutes);
  } else {
    bprint(45, 10, minutes);
  }

  if (seconds<10) {
    int xx = bprint(91, 10, 0);
    bprint(xx, 10, seconds);
  } else {
    bprint(91, 10, seconds);
  }
  oled.line (0, oled.height()-2, batLength*(tick/9.0), oled.height()-2);
}

void batteryIcon() {
  byte vccVal = readVcc();
  oled.pixel   (oled.width() - batLength, oled.height()-2); 
  oled.pixel   (oled.width() - batLength, oled.height()-3);
  oled.rect    (oled.width() - batLength+1, oled.height()-4, batLength-1, 4);  
  oled.rectFill(oled.width() - vccVal   -1, oled.height()-3,      vccVal, 2);

  int pos = oled.width() - powerTick(2900);
  oled.pixel(pos, oled.height()-5);

  pos = oled.width() - powerTick(3000);
  oled.pixel(pos,oled.height()-5);
  oled.pixel(pos,oled.height()-6);
  pos = oled.width() - powerTick(3100);
  oled.pixel(pos,oled.height()-5);
  pos = oled.width() - powerTick(3200);
  oled.pixel(pos,oled.height()-5);
  pos = oled.width() - powerTick(3300);
  oled.pixel(pos,oled.height()-5);
}

void wakeUpIcon() {
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
  delay(100);
  oled.circle(oled.width()/2, oled.height()/2, 15);
  oled.black(0,0,oled.width()/2, oled.height());
  oled.black(oled.width()/2, oled.height()/2,oled.width()/2, oled.height()/2);
  oled.display();  
  delay(100);
  oled.circle(oled.width()/2, oled.height()/2, 20);
  oled.black(0,0,oled.width()/2, oled.height());
  oled.black(oled.width()/2, oled.height()/2,oled.width()/2, oled.height()/2);
  oled.display();  
  delay(200);
}

byte tob(char c) {
  return c - '0';
}

void loop() {
  delay(scrollSpeed);
  
  if (digitalRead(BUTTON2) == LOW || clockMode > 1) {
    delay(300); 
    if (digitalRead(BUTTON2) == LOW || clockMode > 1) {
      Serial.println( CHAR_TIME_REQUEST );
      if (clockMode < 2) {
        COUNT = 0;
        analogWrite(LED_RED, 255);
        analogWrite(LED_GREEN, 255);
        analogWrite(LED_BLUE, 255);
      }
      
      oled.on();
      int dummySec = (seconds+8)%60;
      while(seconds != dummySec) {
        oled.clear();
        if (clockMode == 1) {
          anaClock();
        } else if (clockMode == 2) {
          oled.pixel(0, 0);
          digiClock();
        } else {
          digiClock();
        }
        batteryIcon();
        oled.display();
      }
      
      if (digitalRead(BUTTON1) == LOW) {
        clockMode++;
        if (clockMode > 2) clockMode = 0;
      }
      
      if (clockMode == 0 || clockMode == 1) oled.off();

      if (digitalRead(BUTTON2) == LOW) {      
        //Serial.println("Game !");
        oled.on();
        power_adc_enable();
        game();
        power_adc_disable();
        oled.off();
      }  
    }
  }
  
  if (digitalRead(BUTTON1) == LOW) {
    delay(300);  
    if (digitalRead(BUTTON1) == LOW) {
      
      COUNT = 0;
      analogWrite(LED_RED, 255);
      analogWrite(LED_GREEN, 255);
      analogWrite(LED_BLUE, 255);

      oled.on();
      wakeUpIcon();
      oled.off();
                      
      // "remove" old chars from buffer
      // print ignores everyting behind \0
      memoStr[MESSAGEPOS] = '\0';
      memoStrPos = MESSAGEPOS;
      Serial.println( CHAR_MSG_REQUEST );
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
      
      if (COUNT > 0) { // 4*57 = 228
        redValue   = 255 - 4 * ((unsigned char) memoStr[MESSAGEPOS+2] - 'A'); // "A" -> 255, "z" -> 0
        greenValue = 255 - 4 * ((unsigned char) memoStr[MESSAGEPOS+3] - 'A');
        blueValue  = 255 - 4 * ((unsigned char) memoStr[MESSAGEPOS+4] - 'A');        
        delayValue = (unsigned char) memoStr[MESSAGEPOS+5] - 'A';
      } else {
        redValue = greenValue = blueValue = 255;
        memoStr[MESSAGEPOS] = '\0';
      }
      page = memoStrPos; // makes a clear and display off        
     }
  }

  /**
   * Scrolling message through display
   */
  if (memoStrPos > MESSAGEPOS && page <= memoStrPos) {
    oled.clear();
    oled.setCursor(0, 0);
    oled.print(&(memoStr[page]));
    oled.display();
  }

  /// Safe power and switch display off, if message is at the end
  if (page == memoStrPos) {
    oled.off();
    // "remove" old chars from buffer
    // print ignores everyting behind \0
    memoStr[MESSAGEPOS] = '\0';
    memoStrPos = MESSAGEPOS;
  }

  if (COUNT > 0) {
    if (delayValue==0) {
        analogWrite(LED_RED, redValue);      
        analogWrite(LED_GREEN, greenValue);      
        analogWrite(LED_BLUE, blueValue);      
    } else {
      if (tick <= delayValue) {
        analogWrite(LED_RED, redValue);      
        analogWrite(LED_GREEN, greenValue);      
        analogWrite(LED_BLUE, blueValue);      
      } else {
        analogWrite(LED_RED, 255);
        analogWrite(LED_GREEN, 255);
        analogWrite(LED_BLUE, 255);
      }
    }
  } else {
    analogWrite(LED_RED, 255);
    analogWrite(LED_GREEN, 255);
    analogWrite(LED_BLUE, 255);
  }
   
  page++;
}




int bprint(int x, int y, byte b) {
    int he = 32;
    if (b>9) {
      x = bprint(x,y,b/10);
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






// =====================================================================




void setByte(byte b, int x, int y) {
  int tmp;
  if (x<0 || x>63 || y<0 || y>47) return;
  for (byte bitNr=0; bitNr<8; ++bitNr) {
    if (((b >> bitNr) & 0x01)) {
      tmp = 7-bitNr+x;
      if (tmp<0 || tmp>63) continue;
      oled.pixel(tmp, y);
    }
  }
}

void setByte90(byte b, int x, int y) {
  int tmp;
  if (x<0 || x>63 || y<0 || y>47) return;
  for (byte bitNr=0; bitNr<8; ++bitNr) {
    if (((b >> bitNr) & 0x01)) {
      tmp = 7-bitNr+y;
      if (tmp<0 || tmp>47) continue;
      oled.pixel(x, tmp);
    }
  }
}

void gameStart() {
  oled.clear();
  EEPROM.get(eeAddress, highscore);
  analogWrite(LED_BLUE, 0); // 100%
  analogWrite(LED_GREEN, 0); // 100%

  oled.setCursor(0, 0);
  oled.print(' ');
  oled.print(' ');
  oled.print('D');
  oled.print('i');
  oled.print('n');
  oled.print('o');
  oled.print('\n');
  oled.print('=');
  oled.print('=');
  oled.print('=');
  oled.print('=');
  oled.print('=');
  oled.print('=');
  oled.print('=');
  oled.print('=');
  oled.print('=');
  oled.print('\n');
  oled.print('H');
  oled.print('i');
  oled.print('g');
  oled.print('h');
  oled.print('s');
  oled.print('c');
  oled.print('o');
  oled.print('r');
  oled.print('e');
  oled.print(':');
  oled.print('\n');
  oled.print(highscore);
  oled.display();
  delay(1000); // to see the highscore!
  analogWrite(LED_BLUE, 127); // 50%
  delay(2000);
  analogWrite(LED_BLUE, 255); // off
  analogWrite(LED_GREEN, 255); // off
}

void gameOver() {
  if (score > highscore) {
    // store it in a persistent flash ROM
    highscore = score;
    EEPROM.put(eeAddress, highscore);
  }
  analogWrite(LED_GREEN, 255);
  oled.setCursor(0, 0);
  oled.black('1');
  oled.setCursor(0, 0);
  oled.print(0);
  oled.setCursor(0, 22);
  oled.print(' ');
  oled.print('G');
  oled.print('a');
  oled.print('m');
  oled.print('e');
  oled.print(' ');
  oled.print('O');
  oled.print('v');
  oled.print('e');
  oled.print('r');
  oled.display();
  delay(3500); // read your score
}

void dino(byte y) {
  byte i;
  static const byte a[] ={
    0b00111000,
    0b00101100,
    0b00111110,
    0b00111110,
    0b00111000,
    0b00111110,
    0b10011000,
    0b01111000,
    0b00111100,
    0b00011000
  };
  for (i=0; i<sizeof(a); ++i) {
    setByte(a[i], 2, i+y);
  }
}

void died() {
  byte i;
  static const byte a[] ={
    0b00011100,
    0b01111100,
    0b01010100,
    0b01101100,
    0b01010100,
    0b01111100,
    0b00111100,
    0b00001100,
    0b00011100,
    0b00011100,
    0b00001100,
    0b00111100
  };
  for (i=0; i<sizeof(a); ++i) {
    setByte90(a[i], 26-i, 38);
  }
}

void feet1(const byte & y) {
  byte i;
  static const byte a[] ={ 
    0b00111000,
    0b00100100
  };
  for (i=0; i<sizeof(a); ++i) {
    setByte(a[i], 2, i+y);
  }
}

void feet2(const byte & y) {
  byte i;
  static const byte a[] ={
    0b01111000,
    0b00001000
  };
  for (i=0; i<sizeof(a); ++i) {
    setByte(a[i], 2, i+y);
  }
}

void feet3(const byte & y) {
  byte i;
  static const byte a[] ={
    0b00110000,
    0b00101000
  };
  for (i=0; i<sizeof(a); ++i) {
    setByte(a[i], 2, i+y);
  }
}

void printCactus(const int & x) {
  byte i;
  int tmp;
  static const byte a[] = {
    0b00100000,
    0b01100000,
    0b00101000,
    0b10101000,
    0b10101000,
    0b11111000,
    0b01111000,
    0b00111000,
    0b00110000,
    0b00110000,
    0b00100000,
    0b00100000
  };
  for (i=0; i<sizeof(a); ++i) {
    setByte(a[i], x, i+33);
  }
}

void game() {
  score = 0;
  byte gamespeed;
  byte lives = 3;
  byte jumpY = 0;
  byte cloud = 56;
  short cactus1 = 70;
  int subTick = 0;

  gameStart();
    
  while(lives > 0) {
    // score is time :-D
    score++;
    
    oled.clear();
    oled.setCursor(0, 0);
    oled.print(lives);
    oled.print("  ");
    oled.print(score);

    // cloud
    if (cloud == 0) cloud=56;
    oled.setCursor(cloud, 10);
    oled.print("*");
    if (score%2 == 0) cloud--;
      
    // ground
    oled.line(0, 45, 63, 45);

    // cactus
    if (cactus1 < -7) cactus1=70;
    printCactus(cactus1);
    cactus1-=2;


    // collision and die ---------------
    if (cactus1 == 6 && jumpY<13) {
      lives--;
      died();
      
      oled.display();
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW); // 100 %
      delay(500);
      analogWrite(LED_RED, 127); // 50 %
      delay(500);
      digitalWrite(LED_RED, HIGH);
      
    } else {
      // a good jump?
      if (jumpY>=13 && cactus1 == 6) score += jumpY;
      
      // alive -------------------------      
      dino(32-jumpY);
      // print the trippling feeds
      if (score%3     == 0) feet1(42-jumpY);
      if ((score+1)%3 == 0) feet2(42-jumpY);
      if ((score+2)%3 == 0) feet3(42-jumpY);
      oled.display();
    }

    // speedup ?!
    if (score > 4000) {
      gamespeed = 10;
    } else if (score > 3000) {
      gamespeed = 20;
    } else if (score > 1500) {
      gamespeed = 30;
    } else if (score > 1000) {
      gamespeed = 40;
    } else if (score > 500) {
      gamespeed = 50;
    } else {
      gamespeed = 70;
    }
    
    subTick += gamespeed;
    delay(gamespeed);
    if (subTick > 100) {
      subTick = 0;
    }

    // jump animation + sound
    switch(jumpY) {          
      case 1: jumpY = 7; analogWrite(LED_GREEN, 127); break;
      case 7: jumpY = 11;  break;
      case 11: jumpY = 13; break;
      case 13: jumpY = 15; break;
      case 15: jumpY = 14; break;
      case 14: jumpY = 12; break;
      case 12: jumpY = 10; break;
      case 10: jumpY = 9;  break;
      case 9:  jumpY = 8; break;
      case 8:  jumpY = 6; break;
      case 6: jumpY = 4;  break;
      case 4: jumpY = 2;  digitalWrite(LED_GREEN, HIGH); break;
      case 2: jumpY = 0; break;
    }

    // jump button
    if (jumpY==0 && digitalRead(BUTTON2) == LOW) jumpY = 1;
  }
  gameOver();
}

