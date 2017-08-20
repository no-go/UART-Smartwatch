#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

byte tick = 0; // myFont

class OledWrapper : public Adafruit_SSD1306 {
  public:
  byte changeDigit;

  OledWrapper(const int & dc, const int & res, const int & cs) : Adafruit_SSD1306(dc,res,cs) {
    changeDigit = B00000001;  
  }

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

  byte myFont(byte x, short y, byte b, byte pos, int he) {
    short yold = y;
    if (tick < 8 && ((pos & changeDigit) != B00000000)) {
      y = y - he*((7.0 - (float)tick)/7.0);
    }
    
    if (b == 0) {
      circle(x+he/4, y+3*he/4, he/4);
      return x+2+he/2;
    } else if (b == 1) {
      line(x,y+5,x+5,y);
      line(x+5,y,x+5,y+he);
      return x+8;
    } else if (b == 2) {
      circle(x+he/4, y-3+3*he/4, he/4);
      black(x,y,he/4,he);
      line(x+he/4,y+he-3,x+he/2,y+he);
      return x+2+he/2;
    } else if (b == 3) {
      circle(x+he/4, y+1*he/4, he/4);
      circle(x+he/4, y+3*he/4, he/4);
      black(x,y,he/4,he);
      return x+2+he/2;
    } else if (b == 4) {
      line(x,y+he/2,x+he/2,y);
      line(x,y+he/2,x+he/2,y+he/2);
      line(x+he/2,y,x+he/2,y+he);
      return x+2+he/2;   
    } else if (b == 5) {
      line(x+he/4,y,x+he/2,y);
      line(x+he/4,y,x+he/4,y+he/2);
      circle(x+he/4, y+3*he/4, he/4);
      black(x,y,he/4,he);
      return x+2+he/2;   
    } else if (b == 6) {
      line(x,y-2+3*he/4,x+he/2,y);
      circle(x+he/4, y+3*he/4, he/4);
      return x+2+he/2;   
    } else if (b == 7) {
      line(x,y+3,x+he/2,y);
      line(x+he/2,y,x,y+he);
      line(x+3,y+he/2,x+he/2-1,y+he/2);
      return x+2+5;  
    } else if (b == 8) {
      circle(x+he/4, y+1*he/4, he/4);
      circle(x+he/4, y+3*he/4, he/4);
      return x+2+he/2;
    } else if (b == 9) {
      circle(x+he/4, y+1*he/4, he/4);
      line (x+he/2, y+1*he/4, x+he/2, y+he);
      return x+2+he/2;
    }
  }
};
