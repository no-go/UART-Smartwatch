#include <SPI.h>
#include <SFE_MicroOLED.h>
#include <avr/power.h>

// ---------------------------- Configure ! -------------
#define PIN_RESET  9
#define PIN_DC     8
#define PIN_CS    10
#define DC_JUMPER  0

#define BUTTON     3

#define MESSAGEPOS     30
#define MEMOSTR_LIMIT 430

#define CHAR_TIME_REQUEST '~'
#define CHAR_TIME_RESPONSE '#'
#define CHAR_INIT_SETUP '!'
// ------------------------------------------------------


// display 64x48
MicroOLED oled(PIN_RESET, PIN_DC, PIN_CS);

char memoStr[MEMOSTR_LIMIT] = {'\0'};
int  memoStrPos   = MESSAGEPOS;
int  page         = 0;

byte hours = 0;
byte minutes = 0;
byte seconds = 0;
byte tick = 0;

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

/**
 * because there is a power regulator, it is hard to
 * messure the battery power level.
 */
byte readVcc() {
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
  return (result-2700)/26; // scale: 3310 -> 24, 2710 -> 0
}

void filler() {
  for (int i=0; i<MESSAGEPOS; ++i) {
    memoStr[i] = ' ';
  }
}

void setup() {
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH);
  Serial.begin(9600);

  power_timer1_disable();
  power_timer2_disable();
  power_adc_disable();
  power_twi_disable();

  oled.begin();
  oled.clear(ALL); // Clear the display's internal memory logo
  oled.display();
  delay(500);
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

void dot(byte x, byte y) {
  oled.pixel(x,   y); oled.pixel(x,   y+1);
  oled.pixel(x+1, y); oled.pixel(x+1, y+1);  
}

void nu(int dx) {
  dot(dx,6); dot(dx+2,6); dot(dx+4,6);
             dot(dx+2,8);  
}
void zero(int dx) {
  dot(dx,0); dot(dx+2,0); dot(dx+4,0);
  dot(dx,2);              dot(dx+4,2);
  dot(dx,4);              dot(dx+4,4);
  nu(dx); // -------------------------
}
void nine(int dx) {
  dot(dx,0); dot(dx+2,0); dot(dx+4,0);
  dot(dx,2); dot(dx+2,2); dot(dx+4,2);
  dot(dx,4); dot(dx+2,4); dot(dx+4,4);
  nu(dx); // -------------------------
}
void eight(int dx) {
  dot(dx,0); dot(dx+2,0); dot(dx+4,0);
  dot(dx,2); dot(dx+2,2); dot(dx+4,2);
  dot(dx,4); dot(dx+2,4);
  nu(dx); // -------------------------
}
void seven(int dx) {
  dot(dx,0); dot(dx+2,0); dot(dx+4,0);
  dot(dx,2); dot(dx+2,2); dot(dx+4,2);
  dot(dx,4);
  nu(dx); // -------------------------
}
void six(int dx) {
  dot(dx,0); dot(dx+2,0); dot(dx+4,0);
  dot(dx,2); dot(dx+2,2); dot(dx+4,2);
  nu(dx); // -------------------------
}
void five(int dx) {
  dot(dx,0); dot(dx+2,0); dot(dx+4,0);
  dot(dx,2); dot(dx+2,2);
  nu(dx); // -------------------------
}
void four(int dx) {
  dot(dx,0); dot(dx+2,0); dot(dx+4,0);
  dot(dx,2);
  nu(dx); // -------------------------
}
void three(int dx) {
  dot(dx,0); dot(dx+2,0); dot(dx+4,0);
  nu(dx); // -------------------------
}
void two(int dx) {
  dot(dx,0); dot(dx+2,0);
  nu(dx); // -------------------------
}
void one(int dx) {
  dot(dx,0);
  nu(dx); // -------------------------
}

int tenth(int val, byte dx) {
  if (val >= 90) {
    nine(dx);
    return 90;
  } else if (val >= 80) {
    eight(dx);
    return 80;
  } else if (val >= 70) {
    seven(dx);
    return 70;
  } else if (val >= 60) {
    six(dx);
    return 60;
  } else if (val >= 50) {
    five(dx);
    return 50;
  } else if (val >= 40) {
    four(dx);
    return 40;
  } else if (val >= 30) {
    three(dx);
    return 30;
  } else if (val >= 20) {
    two(dx);
    return 20;
  } else if (val >= 10) {
    one(dx);
    return 10;
  }
  zero(dx);
  return 0;
}

void singles(int val, byte dx) {
  switch (val) {
  case 9:
    nine(dx);
    break;
  case 8:
    eight(dx);
    break;
  case 7:
    seven(dx);
    break;
  case 6:
    six(dx);
    break;
  case 5:
    five(dx);
    break;
  case 4:
    four(dx);
    break;
  case 3:
    three(dx);
    break;
  case 2:
    two(dx);
    break;
  case 1:
    one(dx);
    break;
  default:
    zero(dx);
  }
}

void atlantisClock() {
  int temp = tenth(hours, 0);
  singles(hours-temp, 7);

  temp = tenth(minutes, 18);
  singles(minutes-temp, 25);
  
  temp = tenth(seconds, 35);
  singles(seconds-temp, 42);
        
  oled.pixel(49, 8); oled.pixel(50, 8);
  
  singles(tick, 52);  
}

void analogClock() {
  oled.circle(29, 31, 13);
  byte hour = hours;
  if (hour>12) hour-=12;
  oled.line(29, 31, xMin[minutes], yMin[minutes]);
  oled.line(29, 31, xHour[hour], yHour[hour]);  
}

void batteryIcon() {
  byte vccVal = readVcc();
  oled.pixel(61, 23);
  oled.pixel(62, 23);
  oled.rect(60, 24, 4, 24);
  oled.rectFill(60, 48-vccVal, 4, vccVal);  
}

int pressed = 0;

void loop() {
  delay(93); // is < 100 : makes the seconds a bit faster!

  if (digitalRead(BUTTON) == LOW) {
    pressed += 1;
  }
  
  if (digitalRead(BUTTON) == HIGH && pressed >= 10) {
    pressed = 0;
    oled.command(DISPLAYOFF);
  }
  
  if (pressed == 5) oled.command(DISPLAYON);
  
  if (pressed > 4 && pressed <= 10) {
      oled.clear(PAGE);
      atlantisClock();      
      analogClock();
      batteryIcon();
      oled.display();
  }
  
  if (pressed > 10) {
    oled.command(DISPLAYON);
    wakeUpIcon();
    oled.command(DISPLAYOFF);
    // "remove" old chars from buffer
    // print ignores everyting behind \0
    memoStr[MESSAGEPOS] = '\0';
    memoStrPos = MESSAGEPOS;
    Serial.println( CHAR_TIME_REQUEST );
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
      char dummyChr[] = {
        memoStr[MESSAGEPOS+1],
        memoStr[MESSAGEPOS+2],
        memoStr[MESSAGEPOS+4],
        memoStr[MESSAGEPOS+5],
        memoStr[MESSAGEPOS+7],
        memoStr[MESSAGEPOS+8]
      };
      String dummy(dummyChr);
      hours = dummy.substring(0,2).toInt();
      minutes = dummy.substring(2,4).toInt();
      seconds = dummy.substring(4,6).toInt();
      
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
  if (page == memoStrPos) oled.command(DISPLAYOFF);
  
  page++;
  ticking();
}

