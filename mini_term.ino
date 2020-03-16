// Mini Terminal
// 2020-01-19  T. Nakagawa

#include "FONT.h"
#include "LCD.h"

constexpr int16_t PIN_BUTTON_0 = PB7;  // Leftmost key.
constexpr int16_t PIN_BUTTON_1 = PB6;
constexpr int16_t PIN_BUTTON_2 = PB5;
constexpr int16_t PIN_BUTTON_3 = PB4;  // Rightmost key.
constexpr int16_t PIN_ROM_VCC = PA8;
constexpr int16_t PIN_ROM_CS = PB12;
constexpr int16_t PIN_ROM_SCLK = PB13;
constexpr int16_t PIN_ROM_SO = PB14;
constexpr int16_t PIN_ROM_SI = PB15;
constexpr int16_t PIN_LCD_VCC = PA1;
constexpr int16_t PIN_LCD_DC = PA2;
constexpr int16_t PIN_LCD_RES = PA3 ;
constexpr int16_t PIN_LCD_SCL = PA5;
constexpr int16_t PIN_LCD_SDA = PA7;
constexpr uint16_t Color[8] = {0x0000, 0x00f8, 0xe007, 0xe0ff, 0x1f00, 0x1ff8, 0xff07, 0xffff};  // Black, red, green, yellow, blue, magenta, cyan, white in little endian.
constexpr uint32_t SLEEP_TIME = 60 * 1000;  // Time to enter sleep mode in ms.
constexpr uint32_t KEY_SAMPLING = 10;  // Sampling period of key events in ms.
constexpr uint32_t KEY_WAIT = 500;  // Wait time to start key repeat in ms.
constexpr uint32_t KEY_REPEAT = 50;  // Key repeat period in ms.
uint8_t KeyCode[4] = {'h', 'j', 'k', 'l'};  // Characters sent by pushing buttons.

class SCREEN {
public:
  SCREEN() : font_(PIN_ROM_CS, PIN_ROM_SI, PIN_ROM_SO, PIN_ROM_SCLK), lcd_(PIN_LCD_DC, PIN_LCD_RES, PIN_LCD_SCL, PIN_LCD_SDA) {
    pinMode(PIN_ROM_VCC, OUTPUT);
    digitalWrite(PIN_ROM_VCC, LOW);
    pinMode(PIN_LCD_VCC, OUTPUT);
    digitalWrite(PIN_LCD_VCC, LOW);

    sleeping_ = true;
    x_ = 0;
    y_ = 0;
    fgc_ = (7 << 12);
    bgc_ = (0 << 8);
    for (int i = 0; i < LINES; i++) {
      vram_[i] = vram_mem_[i];
      for (int j = 0; j < COLUMNS; j++) {
        vram_[i][j] = (fgc_ | bgc_ | ' ');
      }
    }
  }

  bool sleeping() {
    return sleeping_;
  }

  void enable() {
    sleeping_ = false;
    digitalWrite(PIN_ROM_VCC, HIGH);
    digitalWrite(PIN_LCD_VCC, HIGH);
    font_.begin();
    lcd_.begin();
    refresh_lcd(0, 0, COLUMNS - 1, LINES - 1);
  }

  void disable() {
    sleeping_ = true;
    font_.end();
    lcd_.end();
    digitalWrite(PIN_ROM_VCC, LOW);
    digitalWrite(PIN_LCD_VCC, LOW);
  }

  void set_x(uint8_t x) {
    if (x < COLUMNS) x_ = x;
  }

  void set_y(uint8_t y) {
    if (y < LINES) y_ = y;
  }

  void set_fgc(uint8_t c) {
    if (c < 8) fgc_ = ((uint16_t)c << 12);
  }

  void set_bgc(uint8_t c) {
    if (c < 8) bgc_ = ((uint16_t)c << 8);
  }

  void move(int8_t dx, int8_t dy) {
    if (x_ + dx < 0) {
      x_ = 0;
    } else {
      x_ += dx;
    }

    for (; dy < 0; dy++) {
      if (y_ == 0) {
        auto *tmp = vram_[LINES - 1];
        for (int i = LINES - 1; i >= 1; i--) vram_[i] = vram_[i - 1];
        for (int j = 0; j < COLUMNS; j++) tmp[j] = (fgc_ | bgc_ | ' ');
        vram_[0] = tmp;
        if (!sleeping_) {
          lcd_.scroll(-FONT_SIZE);
          clear_lcd(0, y_, COLUMNS - 1, y_);
        }
      } else {
        y_--;
      }
    }

    for (; dy > 0; dy--) {
      if (y_ == LINES - 1) {
        auto *tmp = vram_[0];
        for (int i = 0; i < LINES - 1; i++) vram_[i] = vram_[i + 1];
        for (int j = 0; j < COLUMNS; j++) tmp[j] = (fgc_ | bgc_ | ' ');
        vram_[LINES - 1] = tmp;
        if (!sleeping_) {
          lcd_.scroll(FONT_SIZE);
          clear_lcd(0, y_, COLUMNS - 1, y_);
        }
      } else {
        y_++;
      }
    }
  }

  void clr_eol() {
    for (int j = x_; j < COLUMNS; j++) vram_[y_][j] = (fgc_ | bgc_ | ' ');
    if (!sleeping_) clear_lcd(x_, y_, COLUMNS - 1, y_);
  }

  void clr_eos() {
    for (int j = x_; j < COLUMNS; j++) vram_[y_][j] = (fgc_ | bgc_ | ' ');
    for (int i = y_ + 1; i < LINES; i++) {
      for (int j = 0; j < COLUMNS; j++) vram_[i][j] = (fgc_ | bgc_ | ' ');
    }
    if (!sleeping_) {
      clear_lcd(x_, y_, COLUMNS - 1, y_);
      clear_lcd(0, y_ + 1, COLUMNS - 1, LINES - 1);
    }
  }

  void put_ascii(uint8_t ascii) {
    if (x_ >= COLUMNS) move(-COLUMNS, 1);
    vram_[y_][x_] = (fgc_ | bgc_ | ascii);
    if (!sleeping_) refresh_lcd(x_, y_, x_, y_);
    x_++;
  }

  void put_kanji(uint8_t msb, uint8_t lsb) {
    if (x_ + 1 >= COLUMNS) move(-COLUMNS, 1);
    vram_[y_][x_] = (fgc_ | bgc_ | msb);
    vram_[y_][x_ + 1] = (fgc_ | bgc_ | lsb);
    if (!sleeping_) refresh_lcd(x_, y_, x_ + 1, y_);
    x_ += 2;
  }

  static constexpr int16_t COLUMNS = 240 / 8;
  static constexpr int16_t LINES = 240 / 16;
  static constexpr int16_t FONT_SIZE = 16;

private:
  // Clear the specified region of LCD.
  void clear_lcd(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    for (int i = y0; i <= y1; i++) {
      for (int j = x0; j <= x1; j++) {
        for (int k = 0; k < FONT_SIZE * FONT_SIZE / 2; k++) bmp_buf_[k] = Color[bgc_ >> 8];
        lcd_.draw(j * FONT_SIZE / 2, i * FONT_SIZE, FONT_SIZE / 2, FONT_SIZE, (uint8_t *)bmp_buf_);
      }
    }
  }

  // Refresh the specified region of LCD.
  void refresh_lcd(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    for (int i = y0; i <= y1; i++) {
      for (int j = x0; j <= x1; j++) {
        const uint16_t fgc = Color[vram_[i][j] >> 12];
        const uint16_t bgc = Color[(vram_[i][j] >> 8) & 0x0f];
        int width;
        if ((vram_[i][j] & 0x80) != 0) {  // EUC-JP character.
          if (j + 1 >= COLUMNS) continue;
          font_.get_full((uint8_t)(vram_[i][j] & 0x00ff) - 0xa0, (uint8_t)(vram_[i][j + 1] & 0x00ff) - 0xa0, font_buf_);
          width = FONT_SIZE;
          j++;
        } else {  // ASCII character.
          font_.get_half((uint8_t)(vram_[i][j] & 0x00ff), font_buf_);
          width = FONT_SIZE / 2;
        }
        uint16_t *ptr = bmp_buf_;
        for (int p = 0; p < width; p++) {
          for (int q = 0; q < 2; q++) {
            uint8_t d = font_buf_[p + q * width];
            for (int r = 0; r < 8; r++) {
              *ptr++ = ((d & 0x01) != 0) ? fgc : bgc;
              d >>= 1;
            }
          }
        }
        lcd_.draw(j * FONT_SIZE / 2 - (width - FONT_SIZE / 2), i * FONT_SIZE, width, FONT_SIZE, (uint8_t *)bmp_buf_);
      }
    }
  }

  bool sleeping_;
  uint8_t x_;
  uint8_t y_;
  uint16_t fgc_;  // Foreground color (shifted left by 12).
  uint16_t bgc_;  // Background color (shifted left by 8).
  uint16_t *vram_[LINES];  // [15:12]: foreground color, [11:8]: background color, [7:0]: text
  uint16_t vram_mem_[LINES][COLUMNS];
  uint8_t font_buf_[32];
  uint16_t bmp_buf_[FONT_SIZE * FONT_SIZE];
  FONT font_;
  LCD lcd_;
} Screen;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON_0, INPUT_PULLUP);
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
  pinMode(PIN_BUTTON_3, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // LED off.
}

void loop() {
  // Sleep.
  static uint32_t sleep_time = 0;
  if (!Screen.sleeping() && millis() - sleep_time >= SLEEP_TIME) {
    Screen.disable();
  }

  // Process keys.
  static uint32_t key_time = 0;
  static int8_t key_prev = -1;
  static uint32_t key_count;
  if (millis() - key_time >= KEY_SAMPLING) {
    key_time = millis();
    int8_t key = -1;
    if (digitalRead(PIN_BUTTON_0) == LOW) key = 0;
    else if (digitalRead(PIN_BUTTON_1) == LOW) key = 1;
    else if (digitalRead(PIN_BUTTON_2) == LOW) key = 2;
    else if (digitalRead(PIN_BUTTON_3) == LOW) key = 3;
    if (key >= 0) {
      if (Screen.sleeping()) {
        Screen.enable();
      } else if (key == key_prev) {
        if (--key_count == 0) {
          Serial.write(KeyCode[key]);
          key_count = KEY_REPEAT / KEY_SAMPLING;
        }
      } else {
        Serial.write(KeyCode[key]);
        key_count = KEY_WAIT / KEY_SAMPLING;
      }
      sleep_time = millis();
    }
    key_prev = key;
  }

  // Process screen output.
  static uint8_t cc = '\x00';
  if (Serial.available() > 0) {
    const uint8_t c = (uint8_t)Serial.read();
    if (cc != '\x00') {
      if (cc == '\x01') {
        Screen.set_x(c - 32);
      } else if (cc == '\x02') {
        Screen.set_y(c - 32);
      } else if (cc == '\x03') {
        Screen.set_fgc(c - 32);
      } else if (cc == '\x04') {
        Screen.set_bgc(c - 32);
      } else if ('\x11' <= cc && cc <= '\x14') {
        KeyCode[cc - 0x11] = c;
      } else {  // EUC-JP character
        if ('\xa1' <= c && c <= '\xfe') {
          Screen.put_kanji(cc, c);
        }
      }
      cc = '\x00';
    } else {
      if (c == '\x01' ||
          c == '\x02' ||
          c == '\x03' ||
          c == '\x04' ||
          ('\x11' <= c && c <= '\x14') ||
          ('\xa1' <= c && c <= '\xfe')) {
        cc = c;  // Preserve the input and wait for the next input.
      } else if ('\x20' <= c && c <= '\x7e') {  // ASCII character
        Screen.put_ascii(c);
      } else if (c == '\x07') {  // Reverse line feed
        Screen.move(0, -1);
      } else if (c == '\x08') {  // Backspace
        Screen.move(-1, 0);
      } else if (c == '\x0a') {  // Linefeed
        Screen.move(0, 1);
      } else if (c == '\x0b') {  // Erase the rest of the line
        Screen.clr_eol();
      } else if (c == '\x0d') {  // Carriage return
        Screen.move(-Screen.COLUMNS, 0);
      } else if (c == '\x0e') {  // Erase the rest of the screen
        Screen.clr_eos();
      }
    }
  }
}
