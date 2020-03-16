// Library for Kanji font ROM GT20L16J1Y.
// 2020-01-19  T. Nakagawa

#ifndef FONT_H_
#define FONT_H_

#include <SPI.h>

class FONT {
public:
  FONT(uint8_t cs, uint8_t si, uint8_t so, uint8_t sclk): cs_(cs), spi_(si, so, sclk) {
    pinMode(cs_, OUTPUT);
    digitalWrite(cs_, LOW);
  }

  void begin() {
    digitalWrite(cs_, HIGH);
    spi_.begin();
    spi_.setBitOrder(MSBFIRST);
    spi_.setDataMode(SPI_MODE3);
    spi_.setClockDivider(SPI_CLOCK_DIV2);
  }

  void end() {
    digitalWrite(cs_, LOW);
    spi_.end();
  }

  // Get 16x16 font for a kuten-code (32 bytes).
  void get_full(uint8_t msb, uint8_t lsb, uint8_t *font) {
    uint32_t adrs;
    if      (1  <= msb && msb <= 15 && 1 <= lsb && lsb <= 94) adrs = (((uint32_t)msb - 1)  * 94 + (lsb - 1)) * 32;
    else if (16 <= msb && msb <= 47 && 1 <= lsb && lsb <= 94) adrs = (((uint32_t)msb - 16) * 94 + (lsb - 1)) * 32 + 43584;
    else if (48 <= msb && msb <= 84 && 1 <= lsb && lsb <= 94) adrs = (((uint32_t)msb - 48) * 94 + (lsb - 1)) * 32 + 138464;
    else if (msb == 85              && 1 <= lsb && lsb <= 94) adrs = (((uint32_t)msb - 85) * 94 + (lsb - 1)) * 32 + 246944;
    else if (88 <= msb && msb <= 89 && 1 <= lsb && lsb <= 94) adrs = (((uint32_t)msb - 88) * 94 + (lsb - 1)) * 32 + 249952;
    else adrs = 94 + 32;
    digitalWrite(cs_, LOW);
    spi_.transfer(0x03);
    spi_.transfer((adrs >> 16) & 0xff);
    spi_.transfer((adrs >> 8)  & 0xff);
    spi_.transfer(adrs         & 0xff);
    spi_.transfer(font, 32);
    digitalWrite(cs_, HIGH);
  }

  // Get 8x16 font for a ASCII code (16 bytes).
  void get_half(uint8_t ascii, uint8_t *font) {
    uint32_t adrs;
    if (0x20 <= ascii && ascii <= 0x7f) adrs = ((uint32_t)ascii - 0x20) * 16 + 255968;
    else adrs = 255968;
    digitalWrite(cs_, LOW);
    spi_.transfer(0x03);
    spi_.transfer((adrs >> 16) & 0xff);
    spi_.transfer((adrs >> 8)  & 0xff);
    spi_.transfer(adrs         & 0xff);
    spi_.transfer(font, 16);
    digitalWrite(cs_, HIGH);
  }

private:
  uint8_t cs_;
  SPIClass spi_;
};

#endif
