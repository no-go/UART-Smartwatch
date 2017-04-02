#include <avr/power.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// HARDWARE SPI OLED (11 -> MOSI/DIN, 13 ->SCK)

// CONFIG -------------------------------------------
//#define BUTTON1    3
//#define BUTTON2    2
//#define LED_RED    10
//#define LED_OFF    LOW
//#define LED_ON     HIGH
//#define PIN_CS     4
//#define PIN_RESET  6
//#define PIN_DC     8
//#define SERIAL_SPEED   9600

#define BUTTON1    A0
#define BUTTON2    A1
#define LED_RED    10
#define LED_OFF    HIGH
#define LED_ON     LOW
#define PIN_CS     5
#define PIN_RESET  6
#define PIN_DC     8
#define SERIAL_SPEED   9600 // or try 115200

// --------------------------------------------------

#define CHAR_TIME_REQUEST     '~'
#define CHAR_TIME_RESPONSE    '#' //#HH:mm:ss
#define CHAR_NOTIFY_HINT      '%' //%[byte]

#define WITHGAME 42 // -------------------- EASTER EGG

#ifdef WITHGAME
  #define MEMOSTR_LIMIT 250
#else
  #define MEMOSTR_LIMIT 270 //  270    10 (inklusive #hh:mm:ss/)
#endif
void game();

const byte batLength =  64;
char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos = 0;
int  cStart     = 0;
int  cEnd       = 0;
char buffMem;

int COUNT = 0;

byte hours   = 0;
byte minutes = 0;
byte seconds = 0;
byte tick    = 0;

// 0=digi, 1=analog, 2=digi 4 ever
byte clockMode = 0;

class OledWrapper : public Adafruit_SSD1306 {
  public:

    OledWrapper(int dc, int res, int cs) : Adafruit_SSD1306(dc,res,cs) {}

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

OledWrapper * oled = new OledWrapper(PIN_DC, PIN_RESET, PIN_CS);
  
byte powerTick(int mv) {
  if (mv < 3000) return 0;
  if (mv >= 3450) return batLength-3;
  return (mv-3000.0)*(batLength-3)/(3450-3000);  
}

void scroll() {
  unsigned short i = 0;
  for (i=1; i<=8; ++i) {
    oled->black(0,0,oled->width(),i);
    oled->command(SSD1306_SETDISPLAYOFFSET);
    oled->command(i++);
    oled->display();
    delay(50);
  }
  tick+=4;
}

int readVcc() {
  int mv;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(10); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
  mv = ADCL; 
  mv |= ADCH<<8; 
  mv = 1126400L / mv;
  return powerTick(mv);
}

int myFont(int x, int y, byte b) {
    int he = 30;
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

void anaClock() {
  byte x = 33;
  byte y = 32;
  byte radius = 31;
  oled->circle(x, y, radius);
  int hour = hours;
  if (hour>12) hour-=12;
  oled->line(
    x, y,
    x + radius*cos(PI * ((float)seconds-15.0) / 30),
    y + radius*sin(PI * ((float)seconds-15.0) / 30)
  );
  
  oled->line(
    x, y,
    x + (radius-3)*cos(PI * ((float)minutes-15.0) / 30),
    y + (radius-3)*sin(PI * ((float)minutes-15.0) / 30)
  );
  
  oled->line(
    x, y,
    x + (radius-12)*cos(PI * ((float)hour-3.0) / 6),
    y + (radius-12)*sin(PI * ((float)hour-3.0) / 6)
  );
  oled->line(
    x+1, y,
    x-1 +(radius-12)*cos(PI * ((float)hour-3.0) / 6),
    y +  (radius-12)*sin(PI * ((float)hour-3.0) / 6)
  );
  
  for (byte i=0; i<12; ++i) {
    oled->pixel(x + (radius-3)*cos(PI * ((float)i) / 6), y + (radius-3)*sin(PI * ((float)i) / 6));  
  }

  oled->setCursor(x-5,y-radius+4);
  oled->print(12);
  oled->setCursor(x-2,y+radius-11);
  oled->print(6);
  oled->setCursor(x+radius-9,y-3);
  oled->print(3);
  oled->setCursor(x-radius+6,y-3);
  oled->print(9);

  // and small digital
  oled->setCursor(x+radius-2,0);
  oled->setFontType(2);
  oled->print(hours);
  oled->print(':');
  if(minutes < 10) oled->print('0');
  oled->print(minutes);
  oled->setFontType(1);
}

void setup() {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, LED_OFF); // off
  Serial.begin(SERIAL_SPEED);

  power_timer1_disable();
  power_timer2_disable();
  power_adc_disable();
  power_twi_disable();

  oled->begin();
  oled->print('U'); // crazy, but saves dynamic mem
  oled->print('A');
  oled->print('R');
  oled->print('T');
  oled->print('-');
  oled->print('S');
  oled->print('m');
  oled->print('a');
  oled->print('r');
  oled->print('t');
  oled->print('w');
  oled->print('a');
  oled->print('t');
  oled->print('c');
  oled->print('h');
  power_adc_enable();
  batteryIcon();
  power_adc_disable();
  oled->display();
  delay(3000);
  oled->clearDisplay();
  buffMem = '\0';
}

void serialEvent() {
  while (Serial.available()) {
    if (memoStrPos >= MEMOSTR_LIMIT) memoStrPos = MEMOSTR_LIMIT;
    char inChar = (char)Serial.read();
    if (inChar == -61) continue; // symbol before utf-8
    if (inChar == -62) continue; // other symbol before utf-8
    if (inChar == '\n') {
      oled->on();
      memoStr[memoStrPos] = '\0';
      cStart = 0;
      cEnd = 0;
      buffMem = memoStr[cEnd];
      continue;
    }
    memoStr[memoStrPos] = oled->umlReplace(inChar);
    memoStrPos++;
    if (memoStrPos >= MEMOSTR_LIMIT) memoStrPos = 0;
  }
}

inline void ticking() {
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
  if (hours<10) {
    int xx = myFont(0, 10, 0);
    myFont(xx, 10, hours);
  } else {
    myFont(0, 10, hours);
  }
  if (minutes<10) {
    int xx = myFont(43, 10, 0);
    myFont(xx, 10, minutes);
  } else {
    myFont(43, 10, minutes);
  }
  if (seconds<10) {
    int xx = myFont(85, 10, 0);
    myFont(xx, 10, seconds);
  } else {
    myFont(85, 10, seconds);
  }
  oled->line(0, oled->height()-2, batLength*(tick/9.0), oled->height()-2);
}

void batteryIcon() {
  byte vccVal = readVcc();
  byte lowV = powerTick(3150);
  byte pos = oled->height() - lowV;
  if (vccVal < lowV) {
    oled->setCursor(oled->width()-30, pos);
    oled->print('L');
    oled->print('o');
    oled->print('w');
  }
  oled->pixel   (oled->width()-4, oled->height() - batLength);
  oled->pixel   (oled->width()-3, oled->height() - batLength);
  oled->rect    (oled->width()-6, oled->height()  - batLength+1, 6, batLength-1);  
  oled->rectFill(oled->width()-5, oled->height()  - vccVal   -1, 4,      vccVal); 

  oled->pixel(oled->width()-7,  pos);
  oled->pixel(oled->width()-6,  pos);
}

inline void wakeUpIcon() {
  oled->clear();
  oled->circle(oled->width()/2, oled->height()/2, 5);
  oled->black(0,0,oled->width()/2, oled->height());
  oled->black(oled->width()/2, oled->height()/2,oled->width()/2, oled->height()/2);
  oled->display();
  delay(100);
  oled->circle(oled->width()/2, oled->height()/2, 10);
  oled->black(0,0,oled->width()/2, oled->height());
  oled->black(oled->width()/2, oled->height()/2,oled->width()/2, oled->height()/2);
  oled->display();
  delay(100);
  oled->circle(oled->width()/2, oled->height()/2, 15);
  oled->black(0,0,oled->width()/2, oled->height());
  oled->black(oled->width()/2, oled->height()/2,oled->width()/2, oled->height()/2);
  oled->display();  
  delay(100);
  oled->circle(oled->width()/2, oled->height()/2, 20);
  oled->black(0,0,oled->width()/2, oled->height());
  oled->black(oled->width()/2, oled->height()/2,oled->width()/2, oled->height()/2);
  oled->display();  
  delay(200);
  tick += 5;
}

inline byte tob(char c) { return c - '0';}

void loop() {
  delay(95); // is < 100 : makes the seconds a bit faster!

  if (digitalRead(BUTTON2) == LOW || clockMode > 1) {
    delay(300); 
    tick += 3; 
    if (digitalRead(BUTTON2) == LOW || clockMode > 1) {
      
      
      if (clockMode < 2) {
        COUNT = 0;
        digitalWrite(LED_RED, LED_OFF);
      }
      
      oled->on();
      
#ifdef WITHGAME
      // Button 1 and 2 ARE PRESSED !!
      if (digitalRead(BUTTON1) == LOW) game();
#endif //  WITHGAME

      power_adc_enable();
      for (int j=0; j<50; ++j) { // 5sec
        oled->clear();
        ticking();
        if (clockMode == 1) {
          anaClock();
        } else if (clockMode == 2) {
          oled->pixel(5, 0);
          oled->pixel(30, 0);
          digiClock();
        } else {
          digiClock();
        }
        batteryIcon();
        oled->display();
        delay(85); // 10ms in vcc mesurement
      }
      power_adc_disable();
      
      if (digitalRead(BUTTON2) == LOW) {
        clockMode++;
        if (clockMode > 2) clockMode = 0;
      }
      
      if (clockMode < 2) oled->off();    
    }
  }
  
  if (digitalRead(BUTTON1) == LOW) {
    delay(300);  
    tick += 3;
    if (digitalRead(BUTTON1) == LOW) {
      
      COUNT = 0;
      digitalWrite(LED_RED, LED_OFF);

      oled->on();
      wakeUpIcon();
      oled->off();
                      
      // "remove" old chars from buffer
      // print ignores everyting behind \0
      memoStr[0] = '\0';
      memoStrPos = 0;
      Serial.print(CHAR_TIME_REQUEST);
      Serial.print('\n');
    }
  }

  if (memoStrPos > 0 && cEnd == 0) {

    if (memoStr[0] == CHAR_TIME_RESPONSE) {
      
      // extract the time -------------------------
      
      memoStr[0] = ' ';
      hours = tob(memoStr[1])*10 + tob(memoStr[2]);
      minutes = tob(memoStr[4])*10 + tob(memoStr[5]);
      seconds = tob(memoStr[7])*10 + tob(memoStr[8]);

    } else if (memoStr[0] == CHAR_NOTIFY_HINT) {
      // there is a new message (or a message is deleted)
      
      COUNT = (unsigned char) memoStr[1];
      cEnd = memoStrPos; // makes a clear and display off        
     }
  }

  // Scrolling message through display
  if (memoStrPos > 0 && cEnd <= memoStrPos) {
    if (cEnd < memoStrPos) memoStr[cEnd] = '\0';
    oled->clear();
    oled->setCursor(0,0);
    oled->print(&(memoStr[cStart]));
    oled->command(SSD1306_SETDISPLAYOFFSET);
    oled->command(0x0);
    oled->display();
    
    memoStr[cEnd] = buffMem;
    if (cEnd%21 == 0 && cEnd > 147) {
      cStart += 21;
      scroll();
    }
  }

  /// Safe power and switch display off, if message is at the end
  if (
    cStart > memoStrPos || cEnd == memoStrPos+147
  ) {
    oled->off();
    // "remove" old chars from buffer
    // print ignores everyting behind \0
    memoStr[0] = '\0';
    memoStrPos = 0;
  }

  if (COUNT > 0) {
    if (tick == 0) {
      digitalWrite(LED_RED, LED_ON);
    } else {
      digitalWrite(LED_RED, LED_OFF);
    }
  } else {
    digitalWrite(LED_RED, LED_OFF);
  }

  cEnd++;
  if (cEnd <= memoStrPos) buffMem = memoStr[cEnd];
  ticking();
}

//                          E A S T E R   E G G
// ----------------------------------------------------------------------------------------

int eeAddress = 0;
int score     = 0;
int highscore = 0;

void setByte(byte b, int x, int y) {
  int tmp;
  if (x<0 || x>63 || y<0 || y>47) return;
  for (byte bitNr=0; bitNr<8; ++bitNr) {
    if (((b >> bitNr) & 0x01)) {
      tmp = 7-bitNr+x;
      if (tmp<0 || tmp>63) continue;
      oled->pixel(tmp, y);
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
      oled->pixel(x, tmp);
    }
  }
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

void gameStart() {
  oled->clear();
  EEPROM.get(eeAddress, highscore);
  oled->setCursor(0, 0);
  oled->print('H');
  oled->print('i');
  oled->print('g');
  oled->print('h');
  oled->print('s');
  oled->print('c');
  oled->print('o');
  oled->print('r');
  oled->print('e');
  oled->print(':');
  oled->print('\n');
  oled->print(highscore);
  oled->display();
  delay(3000); // to see the highscore!
  tick += 30;
}

void gameOver() {
  if (score > highscore) {
    // store it in a persistent flash ROM
    highscore = score;
    EEPROM.put(eeAddress, highscore);
  }
  oled->setCursor(0, 0);
  oled->black(1);
  oled->setCursor(0, 0);
  oled->print(0);
  oled->setCursor(8, 22);
  oled->print('G');
  oled->print('a');
  oled->print('m');
  oled->print('e');
  oled->print(' ');
  oled->print('O');
  oled->print('v');
  oled->print('e');
  oled->print('r');
  oled->display();
  delay(3500); // read your score
  tick += 35;
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
    
    oled->clear();
    oled->setCursor(0, 0);
    oled->print(lives);
    oled->setCursor(20, 0);
    oled->print(score);

    // cloud
    if (cloud == 0) cloud=56;
    oled->setCursor(cloud, 10);
    oled->print('*');
    if (score%2 == 0) cloud--;
      
    // ground
    oled->line(0, 45, 63, 45);

    // cactus
    if (cactus1 < -7) cactus1=70;
    printCactus(cactus1);
    cactus1-=2;


    // collision and die ---------------
    if (cactus1 == 6 && jumpY<13) {
      lives--;
      died();
      
      oled->display();
      digitalWrite(LED_RED, LED_ON); // 100 %
      delay(1000);
      digitalWrite(LED_RED, LED_OFF);
      tick += 10;
      
    } else {
      // a good jump?
      if (jumpY>=13 && cactus1 == 6) score += jumpY;
      
      // alive -------------------------      
      dino(32-jumpY);
      // print the trippling feeds
      if (score%3     == 0) feet1(42-jumpY);
      if ((score+1)%3 == 0) feet2(42-jumpY);
      if ((score+2)%3 == 0) feet3(42-jumpY);
      oled->display();
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
      subTick = 0; ticking();
    }

    // jump animation + sound
    switch(jumpY) {          
      case 1: jumpY = 7; break;
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
      case 4: jumpY = 2;  break;
      case 2: jumpY = 0; break;
    }

    // jump button
    if (jumpY==0 && digitalRead(BUTTON2) == LOW) jumpY = 1;
  }
  gameOver();
}
