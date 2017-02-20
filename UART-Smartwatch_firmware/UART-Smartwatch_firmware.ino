#define BUTTON1    A0
#define BUTTON2    A1
#define LED_RED    10
// OLED (13 -> MISO/DIN, 11 ->SCK)
#define PIN_CS     5
#define PIN_RESET  6
#define PIN_DC     8
#define DC_JUMPER  0

#define CHAR_TIME_REQUEST     '~'
#define CHAR_TIME_RESPONSE    '#' //#HH:mm:ss
#define CHAR_NOTIFY_HINT      '%' //%[byte]

#define MESSAGEPOS     20
#define MEMOSTR_LIMIT 350
const int batLength =  60;

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

byte hours   = 10;
byte minutes = 10;
byte seconds = 15;
byte tick    = 0;

// 0=digi, 1=analog, 2=digi 4 ever
int clockMode = 0;

byte powerTick(int mv) {
  float quot = (3400-2740)/(batLength-3);
  return (mv-2740)/quot;  
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
  byte y = 31;
  byte radius = 30;
  oled.circle(x, y, radius);
  int hour = hours;
  if (hour>12) hour-=12;
  oled.line(
    x, y,
    x + radius*cos(PI * ((float)seconds-15.0) / 30),
    y + radius*sin(PI * ((float)seconds-15.0) / 30)
  );
  
  oled.line(
    x, y,
    x + (radius-3)*cos(PI * ((float)minutes-15.0) / 30),
    y + (radius-3)*sin(PI * ((float)minutes-15.0) / 30)
  );
  
  oled.line(
    x, y,
    x + (radius-12)*cos(PI * ((float)hour-3.0) / 6),
    y + (radius-12)*sin(PI * ((float)hour-3.0) / 6)
  );
  oled.line(
    x+1, y,
    x-1 +(radius-12)*cos(PI * ((float)hour-3.0) / 6),
    y +  (radius-12)*sin(PI * ((float)hour-3.0) / 6)
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
}

void filler() {
  for (int i=0; i<MESSAGEPOS; ++i) {
    memoStr[i] = ' ';
  }
}

void setup() {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);

  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, HIGH); // off
  
  Serial.begin(9600);

  power_timer1_disable();
  power_timer2_disable();
  power_adc_disable();
  power_twi_disable();
  
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
  oled.setFontType(3);
  oled.setCursor(0, 12);
  if (hours<10) oled.print("0");
  oled.print(hours);

  oled.setFontType(2);
  oled.setCursor(36, 15);
  oled.print(":");

  oled.setFontType(3);
  oled.setCursor(46, 12);
  if (minutes<10) oled.print("0");
  oled.print(minutes); 

  oled.setFontType(0);
  oled.setCursor(30, 40);
  if (seconds<10) oled.print("0");
  oled.print(seconds);
  oled.print(".");
  oled.print(tick);
}

void batteryIcon() {
  byte vccVal = readVcc();
  oled.pixel   (oled.width()-4, oled.height() - batLength);
  oled.pixel   (oled.width()-3, oled.height() - batLength);
  oled.rect    (oled.width()-6, oled.height()  - batLength+1, 6, batLength-1);  
  oled.rectFill(oled.width()-5, oled.height()  - vccVal   -1, 4,      vccVal); 

  int pos = oled.height() - powerTick(3000);
  oled.setCursor(oled.width()-30, pos);
  oled.print(3.0, 1);
  oled.pixel(oled.width()-7,  pos);
}

void wakeUpIcon() {
  oled.clear();
  oled.circle(32, 23, 10);
  oled.pixel(31, 20);
  oled.pixel(35, 19);
  oled.line (29, 27, 35, 27);
  oled.display();
  delay(350);
  oled.rect(29, 19, 3, 3);
  oled.rect(35, 19, 3, 3);
  oled.pixel(35, 26);
  oled.display();  
  delay(350);
  tick += 7; 
}

byte tob(char c) { return c - '0';}

void loop() {
  delay(98); // is < 100 : makes the seconds a bit faster!

  if (digitalRead(BUTTON2) == LOW || clockMode > 1) {
    delay(300); 
    tick += 3; 
    if (digitalRead(BUTTON2) == LOW || clockMode > 1) {
      
      if (clockMode < 2) {
        COUNT = 0;
        digitalWrite(LED_RED, HIGH);
      }
      
      oled.on();

      for (int j=0; j<40; ++j) { // 4sec
        oled.clear();
        ticking();
        if (clockMode == 1) {
          anaClock();
        } else if (clockMode == 2) {
          oled.pixel(5, 0);
          oled.pixel(30, 0);
          digiClock();
        } else {
          digiClock();
        }
        batteryIcon();
        oled.display();
        delay(90); // 10ms in vcc mesurement
      }
      
      if (digitalRead(BUTTON2) == LOW) {
        clockMode++;
        if (clockMode > 2) clockMode = 0;
      }
      
      if (clockMode < 2) oled.off();    
    }
  }
  
  if (digitalRead(BUTTON1) == LOW) {
    delay(300);  
    tick += 3;
    if (digitalRead(BUTTON1) == LOW) {
      
      COUNT = 0;
      digitalWrite(LED_RED, HIGH);

      oled.on();
      wakeUpIcon();
      oled.off();
                      
      // "remove" old chars from buffer
      // print ignores everyting behind \0
      memoStr[MESSAGEPOS] = '\0';
      memoStrPos = MESSAGEPOS;
      Serial.println( CHAR_TIME_REQUEST );
    }
  }

  if (memoStrPos > MESSAGEPOS && page == 0) {

    if (memoStr[MESSAGEPOS] == CHAR_TIME_RESPONSE) {
      
      // extract the time -------------------------
      
      memoStr[MESSAGEPOS] = ' ';
      hours = tob(memoStr[MESSAGEPOS+1])*10 + tob(memoStr[MESSAGEPOS+2]);
      minutes = tob(memoStr[MESSAGEPOS+4])*10 + tob(memoStr[MESSAGEPOS+5]);
      seconds = tob(memoStr[MESSAGEPOS+7])*10 + tob(memoStr[MESSAGEPOS+8]);

    } else if (memoStr[MESSAGEPOS] == CHAR_NOTIFY_HINT) {
      // there is a new message (or a message is deleted)
      
      COUNT = (unsigned char) memoStr[MESSAGEPOS+1];
      page = memoStrPos; // makes a clear and display off        
     }
  }

  // Scrolling message through display
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
    if (tick <= 1) {
      digitalWrite(LED_RED, LOW);
    } else {
      digitalWrite(LED_RED, HIGH);
    }
  } else {
    digitalWrite(LED_RED, HIGH);
  }
   
  page++;
  ticking();
}
