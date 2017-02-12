#include <SPI.h>
#include <SFE_MicroOLED.h>

// OLED (13 -> MISO/DIN, 11 ->SCK)
#define PIN_CS     5
#define PIN_RESET  6
#define PIN_DC     8
#define DC_JUMPER  0

struct OledWrapper {
    MicroOLED * _oled;
    
    OledWrapper() {
      // display 64x48
      _oled = new MicroOLED(PIN_RESET, PIN_DC, PIN_CS);      
    }
    
    void begin() {
      _oled->begin();
    }
    
    void display() {
      _oled->display();
    }

    void line(int x, int y, int xx, int yy) {
      _oled->line(x,y,xx,yy);
    }
    
    void pixel(int x, int y) {
      _oled->pixel(x,y);
    }
    
    void rect(int x, int y, int w, int h) {
      _oled->rect(x,y,w,h);
    }
    
    void rectFill(int x, int y, int w, int h) {
      _oled->rectFill(x,y,w,h);
    }
    
    void circle(int radius, int x, int y) {
      _oled->circle(radius,x,y);
    }

    void setFontType(int t) {
      _oled->setFontType(t);
    }
    
    void print(int c) {
      _oled->print(c);
    } 
    void println(int c) {
      print(c);
      print('\n');
    }
    void print(long c) {
      _oled->print(c);
    } 
    void println(long c) {
      print(c);
      print('\n');
    }
    
    void print(unsigned int c) {
      _oled->print(c);
    } 
    void println(unsigned int c) {
      print(c);
      print('\n');
    }
    void print(unsigned long c) {
      _oled->print(c);
    } 
    void println(unsigned long c) {
      print(c);
      print('\n');
    }
    
    void print(char c) {
      _oled->print(c);
    } 
    void println(char c) {
      print(c);
      print('\n');
    }    
    void print(unsigned char c) {
      _oled->print(c);
    } 
    void println(unsigned char c) {
      print(c);
      print('\n');
    }
    void print(const char c[]) {
      _oled->print(c);
    } 
    void println(const char c[]) {
      print(c);
      print('\n');
    }

    void setCursor(int x, int y) {
      _oled->setCursor(x,y);
    }

    void clear() {
      _oled->clear(PAGE);
    }
    
    void free() {
      _oled->clear(ALL);
    }

    void on() {
      _oled->command(DISPLAYON);
    }
    
    void off() {
      _oled->command(DISPLAYOFF);
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
};
