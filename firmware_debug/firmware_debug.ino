#include <SPI.h>
#include <SFE_MicroOLED.h>

#define PIN_RESET  9
#define PIN_DC     8
#define PIN_CS    10
#define DC_JUMPER  0
#define BUTTON     3

#define CHAR_TIME_REQUEST '~'

// display 64x48
MicroOLED oled(PIN_RESET, PIN_DC, PIN_CS);

String memoStr = "";

void setup() {
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH);
  Serial.begin(9600);

  oled.begin();
  oled.clear(ALL); // Clear the display's internal memory logo
  oled.display();
}

int page = 0;
void serialEvent() {
  while (Serial.available()) {
    if(page ==0) {
      oled.clear(PAGE);
      oled.setCursor(0, 0);
    }
    char inChar = (char)Serial.read();    
    if (inChar == -61) continue; // symbol before utf-8
    if (inChar == -62) continue; // other symbol before utf-8
    if (inChar == '\n') {
      page = 0;      
      continue;
    }
    page++;
    int cint = inChar;
    oled.print(inChar);
    oled.print(cint);
    oled.print(" ");
    oled.display();
  }
}

void loop() {
  delay(100);
  if (digitalRead(BUTTON) == LOW) {
    delay(500);  
    if (digitalRead(BUTTON) == LOW) {  
      Serial.println( CHAR_TIME_REQUEST );
    }
  }
}

