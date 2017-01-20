
// avrdude -c usbtiny -p m328p  -B 300 -U flash:w:UART-Smartwatch_firmware.ino.arduino_eightanaloginputs.hex:i

// ---------------------------- Configure YOUR CONNECTIONS !! -------

// OLED (13 -> MISO/DIN, 11 ->SCK)
#define PIN_CS     5
#define PIN_RESET  6
#define PIN_DC     8
#define DC_JUMPER  0

#define BUTTON1    A0
#define BUTTON2    A1

#define LED_RED     10
#define LED_GREEN   9 
#define LED_BLUE    3
// unconnected
#define POTI        A4 
#define SPKR        A5 // A5 has no analog Out


#define CHAR_TIME_REQUEST     '~'
#define CHAR_TIME_RESPONSE    '#' //#HH:mm:ss
#define CHAR_NOTIFY_HINT      '%' //%[byte]

// ------------------------------------------------------

#define MESSAGEPOS     40 // default:  30 = screen middle
#define MEMOSTR_LIMIT 740 // default: 730 = 700 char buffer

#include <SPI.h>
#include <SFE_MicroOLED.h>
#include <avr/power.h>
#include <EEPROM.h>

const int xHour[13] = {32,40,47,49,47,40,32,23,17,15,17,24,32};
const int yHour[13] = {6,8,14,23,32,38,40,38,31,23,14,8,6};
const int xMin[60]  = {32,34,36,38,41,42,44,46,48,49,50,51,52,53,53,53,53,53,52,51,50,49,48,46,44,42,41,38,36,34,32,30,28,26,23,21,20,18,16,15,14,13,12,11,11,11,11,11,12,13,14,15,16,18,20,22,23,26,28,30};
const int yMin[60]  = {2,2,2,3,4,5,6,7,9,11,12,14,17,19,21,23,25,27,29,32,34,35,37,39,40,41,42,43,44,44,44,44,44,43,42,41,40,39,37,35,33,32,29,27,25,23,21,19,17,14,12,11,9,7,6,5,4,3,2,2};

int eeAddress = 0;
int score     = 0;
int highscore = 0;
void game();

// display 64x48
MicroOLED oled(PIN_RESET, PIN_DC, PIN_CS);

char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos   = MESSAGEPOS;
int  page         = 0;

byte COUNT        = 0;

byte hours   = 0;
byte minutes = 0;
byte seconds = 0;
byte tick    = 0;

bool usingBATpin;

// 0=digi, 1=analog, 2=adjust_hour, 3=adjust_min, 4=digi 4 ever
int clockMode = 0;

int redValue   = 255;
int greenValue = 255;
int blueValue  = 255;

// Check it -------------------------
// 0 always on
// 1 = 100 ms of a second on
// 2 = 200 ms of a second on
// 9 = 900 ms of a second on
int delayValue = 8;

int readWheel() {
  power_adc_enable();
  int reading = analogRead(POTI);
  power_adc_disable();
  reading = map(reading, 0, 1023, 0, 60);
  return reading;
}

int readVcc() {
  int result;
  power_adc_enable();
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(10); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
  result = ADCL; 
  result |= ADCH<<8; 
  result = 1126400L / result;
  power_adc_disable();
  return result;
}

void anaClock() {
  oled.circle(32, 23, 23);
  int hour = hours;
  if (hour>12) hour-=12;
  oled.line(32,   23, xMin[seconds], yMin[seconds]);

  oled.line(32,   23, xMin[minutes], yMin[minutes]);
  oled.line(32,   23, xHour[hour],   yHour[hour]);
  oled.line(32+1, 23, xHour[hour]+1, yHour[hour]);
  
  for (int i=0; i<12; ++i) {
    oled.pixel(xHour[i], yHour[i]);  
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

/**
 * because there is a power regulator, it is hard to
 * messure the battery power level. If you do not connect bat 
 * to battery and connect batery direkt to 3.3V, you can
 * mesure better, but Display could brake!!
 */
byte vcc2hight(int result) {
  if (usingBATpin) {
    return (result-2700)/18; // scale: 3310 -> 34, 2710 -> 0 (USB5v or 3.3v BAT Regulator)    
  } else {
    return (result-2700)/45; // scale: 4205 -> 34, 2710 -> 0
  }  
}

void filler() {
  for (int i=0; i<MESSAGEPOS; ++i) {
    memoStr[i] = ' ';
  }
}

void setup() {
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(POTI, INPUT);

  pinMode(SPKR, OUTPUT);
  
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(BUTTON1, HIGH);
  digitalWrite(BUTTON2, HIGH);
  Serial.begin(9600);

  power_timer1_disable();
  power_timer2_disable();
  power_adc_disable();
  power_twi_disable();

  int mv = readVcc();
  usingBATpin = (mv < 3400);
  
  oled.begin();
  oled.clear(ALL); // Clear the display's internal memory logo
  oled.display();
  filler();
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

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();    
    if (inChar == -61) continue; // symbol before utf-8
    if (inChar == -62) continue; // other symbol before utf-8
    if (inChar == '\n') {
      oled.command(DISPLAYON);
      memoStr[memoStrPos] = '\0';
      page = 0;
      continue;
    }
    memoStr[memoStrPos] = umlReplace(inChar);
    memoStrPos++;
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
  oled.setFontType(2);
  oled.setCursor(0, 12);
  if (hours<10) oled.print("0");
  oled.print(hours);

  oled.setFontType(1);
  oled.setCursor(24, 13);
  oled.print(":");

  oled.setFontType(2);
  oled.setCursor(32, 12);
  if (minutes<10) oled.print("0");
  oled.print(minutes); 

  oled.setFontType(0);
  oled.setCursor(17, 32);
  if (seconds<10) oled.print("0");
  oled.print(seconds);
  oled.print(".");
  oled.print(tick);
}

void batteryIcon() {
  byte vccVal = vcc2hight(readVcc());
  oled.pixel(60, 13);
  oled.pixel(61, 13);
  if (usingBATpin == false) {
    oled.pixel(57, 35); // 3.3V tick
    oled.pixel(56, 26); // 3.7V tick
    oled.pixel(57, 26); // 3.7V tick
  }
  oled.rect(58, 14, 6, 34);
  oled.rectFill(58, 48-vccVal, 6, vccVal); 
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
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, HIGH);
      }
      
      oled.command(DISPLAYON);

      for (int j=0; j<40; ++j) { // 4sec
        oled.clear(PAGE);
        ticking();
        if (clockMode == 1) {
          anaClock();
        } else if (clockMode == 2) {
          oled.pixel(5, 0);
          oled.pixel(30, 0);
          digiClock();
        } else if (clockMode == 3) {
          oled.pixel(5, 0);
          hours = readWheel();
          digiClock();
        } else if (clockMode == 4) {
          oled.pixel(30, 0);
          minutes = readWheel();
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
        // all 0-3 modes
        //if (clockMode > 4) clockMode = 0;
        // without analog poti to change clock
        if (clockMode > 2) clockMode = 0;
      }
      
      if (clockMode == 0 || clockMode == 1) oled.command(DISPLAYOFF);

      if (digitalRead(BUTTON2) == LOW) {      
        Serial.println("Game !");
        oled.command(DISPLAYON);
        power_adc_enable();
        game();
        power_adc_disable();
        oled.command(DISPLAYOFF);
      }      
    }
  }
  
  if (digitalRead(BUTTON1) == LOW) {
    delay(300);  
    tick += 3;
    if (digitalRead(BUTTON1) == LOW) {
      
      COUNT = 0;
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_BLUE, HIGH);

              
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
        redValue   = 228 - 4 * ((unsigned char) memoStr[MESSAGEPOS+2] - 'A'); // "A" -> 255, "z" -> 0
        greenValue = 228 - 4 * ((unsigned char) memoStr[MESSAGEPOS+3] - 'A');
        blueValue  = 228 - 4 * ((unsigned char) memoStr[MESSAGEPOS+4] - 'A');        
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
    oled.clear(PAGE);
    oled.setCursor(0, 0);
    oled.print(&(memoStr[page]));
    oled.display();
  }

  /// Safe power and switch display off, if message is at the end
  if (page == memoStrPos) {
    oled.command(DISPLAYOFF);
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
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_BLUE, HIGH);     
      }
    }
  } else {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH);     
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
  oled.clear(PAGE);
  EEPROM.get(eeAddress, highscore);

  oled.setCursor(0, 0);
  oled.println("   Dino");
  oled.print(  "==========");
  oled.println("Highscore:");
  oled.print(highscore);
  oled.display();

  delay(3000); // to see the highscore!
  tick += 30;
}

void gameOver() {
  if (score > highscore) {
    // store it in a persistent flash ROM
    highscore = score;
    EEPROM.put(eeAddress, highscore);
  }
  digitalWrite(LED_GREEN, LOW);
  analogWrite(SPKR, 5);
  oled.setCursor(0, 0);
  oled.print("0");
  oled.setCursor(0, 22);
  oled.println(" Game Over");
  oled.display();
  delay(1500);
  digitalWrite(SPKR, LOW);
  delay(2000); // read your score
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
    
    oled.clear(PAGE);
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
      digitalWrite(LED_GREEN, LOW);
      analogWrite(SPKR, 120);
      delay(500);
      analogWrite(SPKR, 32);
      delay(500);
      digitalWrite(SPKR, LOW);
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
      case 1: jumpY = 7; analogWrite(SPKR, 60); digitalWrite(LED_GREEN, HIGH); break;
      case 7: jumpY = 11;  break;
      case 11: jumpY = 13; analogWrite(SPKR, 65); break;
      case 13: jumpY = 15; analogWrite(SPKR, 85); break;
      case 15: jumpY = 14;  break;
      case 14: jumpY = 12; analogWrite(SPKR, 95); break;
      case 12: jumpY = 10;  break;
      case 10: jumpY = 9;  digitalWrite(SPKR, LOW); break;
      case 9:  jumpY = 8; break;
      case 8:  jumpY = 6; break;
      case 6: jumpY = 4;  break;
      case 4: jumpY = 2;  digitalWrite(LED_GREEN, LOW); break;
      case 2: jumpY = 0; break;
    }

    // jump button
    if (jumpY==0 && digitalRead(BUTTON2) == LOW) jumpY = 1;
  }
  gameOver();
}

