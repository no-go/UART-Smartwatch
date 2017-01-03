#include <SPI.h>
#include <SFE_MicroOLED.h>
#include <avr/power.h>
#include <EEPROM.h>

// avrdude -c usbtiny -p m328p  -B 300 -U flash:w:UART-Smartwatch_firmware.ino.arduino_eightanaloginputs.hex:i

// ---------------------------- Configure ! -------------
#define PIN_RESET  9
#define PIN_DC     8
#define PIN_CS    10
#define DC_JUMPER  0

#define BUTTON1     3
#define BUTTON2     4

#define SPKR        5
#define LED_RED     6
#define LED_YELLOW  A1
#define LED_GREEN   A2

#define MESSAGEPOS     30
#define MEMOSTR_LIMIT 730

#define CHAR_TIME_REQUEST '~'
#define CHAR_TIME_RESPONSE '#'
#define CHAR_INIT_SETUP '!'
#define CHAR_NOTIFY_HINT '%'

// ------------------------------------------------------

int eeAddress = 0;
int score     = 0;
int highscore = 0;
void game();

// display 64x48
MicroOLED oled(PIN_RESET, PIN_DC, PIN_CS);

char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos   = MESSAGEPOS;
int  page         = 0;
int  mode         = -1; // -1 is off
byte COUNT        = 0;

byte hours = 0;
byte minutes = 0;
byte seconds = 0;
byte tick = 0;

bool usingBATpin;

// save power, because sin/cos is to "expensive"
const byte xHour[13] = {29,34,38,39,38,34,29,24,20,19,20,24,29};
const byte yHour[13] = {21,22,26,31,36,40,41,40,36,31,26,22,21};
const byte xMin[60]  = {29,30,32,33,34,35,37,38,39,40,40,41,41,42,42,42,42,42,41,41,40,40,39,38,37,35,34,33,32,30,29,28,26,25,24,22,21,20,19,18,18,17,17,16,16,16,16,16,17,17,18,18,19,20,21,23,24,25,26,28};
const byte yMin[60]  = {18,18,18,19,19,20,20,21,22,23,24,26,27,28,30,31,32,34,35,36,38,39,40,41,42,42,43,43,44,44,44,44,44,43,43,42,42,41,40,39,37,36,35,34,32,31,30,28,27,26,24,23,22,21,20,20,19,19,18,18};

PROGMEM const char comm[][35] = {
  "+++",
  "ATE=0",
  "AT+HWMODELED=BLEUART",
  "AT+GAPDEVNAME=UART Notify Watch",
  "AT+BLEPOWERLEVEL=4",
  "ATZ"
};

void setupBle() {
  for(byte i=0; i<6; ++i) {
    Serial.println(comm[i]); 
    delay(250);  
  }
}

void readTemp() {
  unsigned int wADC;
  int t;
  power_adc_enable();
  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);            // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));
  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;
  power_adc_disable();
  // The offset of 324.31 could be wrong. It is just an indication.
  t = ( wADC - 324.31 ) / 1.22;
  // The returned temperature is in degrees Celcius.
  oled.setCursor(0, 41);
  oled.print(t);
  oled.setCursor(11, 37);
  oled.print((char)248);
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

  pinMode(SPKR, OUTPUT);
  
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

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

  // --------------------------------
  if (mode >= 0) {
    if (mode > 0) 
      analogWrite(LED_RED, 25*tick);
      else
      digitalWrite(LED_RED, LOW);
    if (tick > 9) {
      if (mode > 0) {
        digitalWrite(LED_YELLOW, seconds%2 ? LOW:HIGH);
        digitalWrite(LED_GREEN, seconds%10 ? LOW:HIGH);
      } else {
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_GREEN, LOW);
      }
      if (mode == 2) {
        if (seconds == 60) {
          analogWrite(SPKR, 140);
        } else {
          digitalWrite(SPKR, LOW);      
        }
      }
    }
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

void wakeUpIcon() {
  oled.clear(PAGE);
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

void digitalClock() {
  oled.setCursor(8, 5);
  if (hours<10) oled.print("0");
  oled.print(hours);
  oled.print(":");
  if (minutes<10) oled.print("0");
  oled.print(minutes);
  oled.print(":");
  if (seconds<10) oled.print("0");
  oled.print(seconds);  
}

void analogClock() {
  oled.circle(29, 31, 13);
  byte hour = hours;
  if (hour>12) hour-=12;
  oled.line(29, 31, xMin[minutes], yMin[minutes]);
  oled.line(29, 31, xHour[hour], yHour[hour]);  
}

void batteryIcon() {
  byte vccVal = vcc2hight(readVcc());
  oled.pixel(61, 13);
  oled.pixel(62, 13);
  if (usingBATpin == false) {
    oled.pixel(59, 35); // 3.3V tick
    oled.pixel(58, 26); // 3.7V tick
    oled.pixel(59, 26); // 3.7V tick
  }
  oled.rect(60, 14, 4, 34);
  oled.rectFill(60, 48-vccVal, 4, vccVal);  
}


void menu() {
  mode++;
  if (mode > 2) mode = 0;
  
  oled.clear(PAGE);
  oled.setCursor(0, 0);
  oled.println("   MODE  ");
  oled.print(  "==========");
  oled.println("- silent ");
  oled.println("- LEDs   ");
  oled.println("-  + beep");
  oled.setCursor(4, 8*mode + 16);
  oled.print(">");
  
  oled.command(DISPLAYON);
  oled.display();
  Serial.print("Mode ");
  Serial.println(mode);
}

byte tob(char c) {
  return c - '0';
}

void loop() {
  delay(93); // is < 100 : makes the seconds a bit faster!

  if (digitalRead(BUTTON2) == LOW) {
    delay(300); 
    tick += 3; 
    if (digitalRead(BUTTON2) == LOW) {
      COUNT = 0;
      digitalWrite(LED_RED, LOW);
      
      if (mode == -1) {
        Serial.println("Game !");
        oled.command(DISPLAYON);
        power_adc_enable();
        game();
        power_adc_disable();
        oled.command(DISPLAYOFF);
      } else {
        menu();        
      }
    }
  }
  
  if (digitalRead(BUTTON1) == LOW) {
    delay(500);  
    tick += 5;
    if (digitalRead(BUTTON1) == LOW) {
      
      // ok: You pressed the button more than 500ms
      COUNT = 0;
      digitalWrite(LED_RED, LOW);
      
      oled.command(DISPLAYON);
      oled.clear(PAGE);
      digitalClock();
      analogClock();
      batteryIcon();
      readTemp();
      oled.display();
            
      delay(1000);
      tick += 10;
      oled.command(DISPLAYOFF);
      
      if (digitalRead(BUTTON1) == LOW) {
        
        // ok: You pressed the button more than 500ms + 1000ms
        
        oled.command(DISPLAYON);
        wakeUpIcon();
        oled.command(DISPLAYOFF);
        // "remove" old chars from buffer
        // print ignores everyting behind \0
        memoStr[MESSAGEPOS] = '\0';
        memoStrPos = MESSAGEPOS;
        Serial.println( CHAR_TIME_REQUEST );
      }
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
      if (COUNT > 0) {
        // switch a LED on ------------------- start
        digitalWrite(LED_RED, HIGH);
        page = memoStrPos; // makes a clear and display off
        // switch a LED on ------------------- end

        // alternative to a LED --- works only < 10 ;-D
        //memoStr[MESSAGEPOS+1] = ' ';
        //memoStr[MESSAGEPOS] = '0' + (char) COUNT;
        
      } else {
        digitalWrite(LED_RED, LOW);
        memoStr[MESSAGEPOS] = '\0';
      }      
    } else if (memoStr[MESSAGEPOS] == CHAR_INIT_SETUP) {
      
      // initialize the DIY Smartwatch + BLE module --------
      
      setup();
      setupBle();
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

