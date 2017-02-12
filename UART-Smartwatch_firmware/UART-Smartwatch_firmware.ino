
// avrdude -c usbtiny -p m328p  -B 300 -U flash:w:UART-Smartwatch_firmware.ino.arduino_eightanaloginputs.hex:i

// ---------------------------- Configure YOUR CONNECTIONS !! -------

#define BUTTON1    A0
#define BUTTON2    A1

#define LED_RED     10
#define LED_GREEN   9 
#define LED_BLUE    3

#define CHAR_TIME_REQUEST     '~'
#define CHAR_TIME_RESPONSE    '#' //#HH:mm:ss
#define CHAR_NOTIFY_HINT      '%' //%[byte]

// ------------------------------------------------------

#define MESSAGEPOS     20
#define MEMOSTR_LIMIT 250 /// @todo:  BAD BAD ! why did ssd1306 lib take so much dyn ram ??

const int batLength = 60;

#include "OledWrapper.cpp"
#include <avr/power.h>
#include <EEPROM.h>

int eeAddress = 0;
int score     = 0;
int highscore = 0;
void game();

OledWrapper oled;

char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos   = MESSAGEPOS;
int  page         = 0;

byte COUNT        = 0;

byte hours   = 10;
byte minutes = 10;
byte seconds = 15;
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
  oled.circle(32, 23, 23);
  int hour = hours;
  if (hour>12) hour-=12;
  oled.line(
    32, 23,
    32 + 23*cos(PI * ((float)seconds-15.0) / 30),
    23 + 23*sin(PI * ((float)seconds-15.0) / 30)
  );
  
  oled.line(
    32, 23,
    32 + 20*cos(PI * ((float)minutes-15.0) / 30),
    23 + 20*sin(PI * ((float)minutes-15.0) / 30)
  );
  
  oled.line(
    32, 23,
    32 + 13*cos(PI * ((float)hour-3.0) / 6),
    23 + 13*sin(PI * ((float)hour-3.0) / 6)
  );
  oled.line(
    32+1, 23,
    32-1 + 13*cos(PI * ((float)hour-3.0) / 6),
    23 + 13*sin(PI * ((float)hour-3.0) / 6)
  );
  
  for (byte i=0; i<12; ++i) {
    oled.pixel(32 + 20*cos(PI * ((float)i) / 6), 23 + 20*sin(PI * ((float)i) / 6));  
  }
  // 12 o'clock
  oled.pixel(30, 3);
  oled.pixel(30, 4);
  oled.pixel(30, 5);
  oled.pixel(30, 6);
  oled.pixel(32, 3);
  oled.pixel(33, 3);
  oled.pixel(33, 4);
  oled.pixel(32, 5);
  oled.pixel(33, 6);
  oled.pixel(34, 6);
  // 3 o'clock
  oled.pixel(49, 21);
  oled.pixel(50, 21);
  oled.pixel(50, 22);
  oled.pixel(49, 23);
  oled.pixel(50, 24);
  oled.pixel(49, 25);
  oled.pixel(50, 25);
  // 6 o'clock
  oled.pixel(32, 41);
  oled.pixel(31, 42);
  oled.pixel(30, 43);
  oled.pixel(31, 43);
  oled.pixel(30, 44);
  oled.pixel(31, 44);
  oled.pixel(32, 44);
  oled.pixel(31, 45);
  // 9 o'clock
  oled.pixel(14, 21);
  oled.pixel(13, 22);
  oled.pixel(15, 22);
  oled.pixel(14, 23);
  oled.pixel(15, 23);
  oled.pixel(14, 24);
  oled.pixel(13, 25);
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
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  
  Serial.begin(9600);

  //power_timer1_disable(); // timer needed for PWM on pin 9/10 ?!
  //power_timer2_disable(); // timer needed for PWM on pin 3 ?!
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

byte tob(char c) {
  return c - '0';
}

void loop() {
  delay(93); // is < 100 : makes the seconds a bit faster!

  if (digitalRead(BUTTON2) == LOW || clockMode > 1) {
    delay(300); 
    tick += 3; 
    if (digitalRead(BUTTON2) == LOW || clockMode > 1) {
      
      if (clockMode < 2) {
        COUNT = 0;
        analogWrite(LED_RED, 255);
        analogWrite(LED_GREEN, 255);
        analogWrite(LED_BLUE, 255);
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
      
      if (digitalRead(BUTTON1) == LOW) {
        clockMode++;
        if (clockMode > 2) clockMode = 0;
      }
      
      if (clockMode == 0 || clockMode == 1) oled.off();

      if (digitalRead(BUTTON2) == LOW) {      
        Serial.println("Game !");
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
    tick += 3;
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
      Serial.println( CHAR_TIME_REQUEST );
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
  ticking();
}







// =====================================================================








void setByte(byte & b, int x, int y) {
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

void setByte90(byte & b, int x, int y) {
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
  oled.println("  Dino");
  oled.println("=========");
  oled.println("Highscore:");
  oled.print(highscore);
  oled.display();
  delay(1000); // to see the highscore!
  analogWrite(LED_BLUE, 127); // 50%
  delay(2000);
  analogWrite(LED_BLUE, 255); // off
  analogWrite(LED_GREEN, 255); // off
  tick += 30;
}

void gameOver() {
  if (score > highscore) {
    // store it in a persistent flash ROM
    highscore = score;
    EEPROM.put(eeAddress, highscore);
  }
  analogWrite(LED_GREEN, 255);
  oled.setCursor(0, 0);
  oled.black("1");
  oled.setCursor(0, 0);
  oled.print("0");
  oled.setCursor(0, 22);
  oled.println(" Game Over");
  oled.display();
  delay(3500); // read your score
  tick += 35;
}

void dino(byte y) {
  byte i;
  byte a[] ={
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
  byte a[] ={
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

void feet1(byte y) {
  byte i;
  byte a[] ={ 
    0b00111000,
    0b00100100
  };
  for (i=0; i<sizeof(a); ++i) {
    setByte(a[i], 2, i+y);
  }
}

void feet2(byte y) {
  byte i;
  byte a[] ={
    0b01111000,
    0b00001000
  };
  for (i=0; i<sizeof(a); ++i) {
    setByte(a[i], 2, i+y);
  }
}

void feet3(byte y) {
  byte i;
  byte a[] ={
    0b00110000,
    0b00101000
  };
  for (i=0; i<sizeof(a); ++i) {
    setByte(a[i], 2, i+y);
  }
}

void printCactus(int & x) {
  byte i;
  int tmp;
  byte a[] = {
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
  int gamespeed;
  int lives = 3;
  int jumpY = 0;
  int cloud = 56;
  int cactus1 = 70;
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
      subTick = 0; ticking();
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

