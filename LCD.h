// Library for ST7789 240x240 LCD.
// 2020-01-19  T. Nakagawa

#ifndef LCD_H_
#define LCD_H_

#include <SPI.h>

class LCD {
public:
  LCD(uint8_t dc, uint8_t res, uint8_t scl, uint8_t sda, uint8_t miso = MISO) : dc_(dc), res_(res), spi_(sda, miso, scl), offset_(0) {
    pinMode(dc_, OUTPUT);
    pinMode(res_, OUTPUT);
    digitalWrite(dc_, LOW);
    digitalWrite(res_, LOW);
  }

  void begin() {
    digitalWrite(dc_, LOW);
    digitalWrite(res_, HIGH);
    spi_.begin();
    spi_.setBitOrder(MSBFIRST);
    spi_.setDataMode(SPI_MODE2);
    spi_.setClockDivider(SPI_CLOCK_DIV2);
    initialize();
  }

  void end() {
    digitalWrite(dc_, LOW);
    digitalWrite(res_, LOW);
    spi_.end();
  }

  void draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t *bmp) {
    const uint16_t xs = (offset_ + y) % ROW;
    const uint16_t xe = xs + h - 1;
    const uint16_t ys = x;
    const uint16_t ye = ys + w - 1;
    // Column address set.
    send_cmd(0x2a);  // CASET
    send_data(xs >> 8);  // XS high.
    send_data(xs & 0xff);  // XS low.
    send_data(xe >> 8);  // XE high.
    send_data(xe & 0xff);  // XE low.
    // Row address set.
    send_cmd(0x2b);  // RASET
    send_data(ys >> 8);  // YS high.
    send_data(ys & 0xff);  // YS low.
    send_data(ye >> 8);  // YE high.
    send_data(ye & 0xff);  // YE low.
    send_cmd(0x2c);  // RAMWR
    transfer_data(bmp, (uint32_t)w * h * 2);
  }

  void scroll(int16_t n) {
    while (n < 0) n += ROW;
    offset_ = (offset_ + n) % ROW;
    // Vertical scroll start address of RAM.
    send_cmd(0x37);  // VSCSAD
    send_data(offset_ >> 8);  // VSP high.
    send_data(offset_ & 0xff);  // VSP low.
  }

  static constexpr int16_t WIDTH = 240;
  static constexpr int16_t HEIGHT = 240;
  static constexpr uint16_t ROW = 320;

private:
  void send_cmd(uint8_t cmd) {
    digitalWrite(dc_, LOW);
    spi_.transfer(cmd);
  }

  void send_data(uint8_t data) {
    digitalWrite(dc_, HIGH);
    spi_.transfer(data);
  }

  void transfer_data(uint8_t *data, uint32_t size) {
    digitalWrite(dc_, HIGH);
    spi_.transfer(data, size);
  }

  void initialize() {
    // Hardware reset.
    digitalWrite(res_, LOW);
    delay(1);
    digitalWrite(res_, HIGH);
    delay(120);
    // Software reset.
    send_cmd(0x01);  // SWRESET
    delay(120);
    // Sleep out.
    send_cmd(0x11);  // SLPOUT
    delay(5);
    // Interface pixel format.
    send_cmd(0x3a);  // COLMOD
    send_data(0x55);  // 65K of RGB interface, 16bit/pixel.
    // Memory data access control.
    send_cmd(0x36);
    send_data(0x20);  // Top-to-bottom, left-to-right, reverse-mode, LCD-refresh-top-to-bottom, RGB, LCD-refresh-left-to-right
    // Vertical scrolling definition.
    send_cmd(0x33);  // VSCRDEF
    send_data(0 >> 8);  // TFA high.
    send_data(0 & 0xff);  // TFA low.
    send_data(HEIGHT >> 8);  // VSA high.
    send_data(HEIGHT & 0xff);  // VSA low.
    send_data(0 >> 8);  // BFA high.
    send_data(0 & 0xff);  // BFA low.
    // Vertical scroll start address of RAM.
    send_cmd(0x37);  // VSCSAD
    send_data(offset_ >> 8);  // VSP high.
    send_data(offset_ & 0xff);  // VSP low.
    // Display inversion on.
    send_cmd(0x21);  // INVON
    // Display on.
    send_cmd(0x29);  // DISPON
  }

  uint8_t dc_;
  uint8_t res_;
  SPIClass spi_;
  uint16_t offset_;
};

#endif
