#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "movies.h"

byte tick = 0; // myFont

class OledWrapper : public Adafruit_SSD1306 {
  public:

  OledWrapper(const int & dc, const int & res, const int & cs) : Adafruit_SSD1306(dc,res,cs) {}

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
  void command(const uint8_t & cmd) {
    ssd1306_command(cmd);
  }

  byte myFont(byte x, short y, byte b) {
    if (b == 0) {
      drawBitmap(x, y, z0, 8, 8, WHITE);
    } else if (b == 1) {
      drawBitmap(x, y, z1, 8, 8, WHITE);
    } else if (b == 2) {
      drawBitmap(x, y, z2, 8, 8, WHITE);
    } else if (b == 3) {
      drawBitmap(x, y, z3, 8, 8, WHITE);
    } else if (b == 4) {
      drawBitmap(x, y, z4, 8, 8, WHITE);
    } else if (b == 5) {
      drawBitmap(x, y, z5, 8, 8, WHITE);
    } else if (b == 6) {
      drawBitmap(x, y, z6, 8, 8, WHITE);
    } else if (b == 7) {
      drawBitmap(x, y, z7, 8, 8, WHITE);
    } else if (b == 8) {
      drawBitmap(x, y, z8, 8, 8, WHITE);
    } else if (b == 9) {
      drawBitmap(x, y, z9, 8, 8, WHITE);
    }
    return x+10;
  }
};
