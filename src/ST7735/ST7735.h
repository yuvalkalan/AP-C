#pragma once
#include "pico/stdlib.h"
#include "hardware/spi.h"

// ST7735 command definitions
#define ST7735_CMD_NOP 0x00
#define ST7735_CMD_SWRESET 0x01
#define ST7735_CMD_RDDID 0x04
#define ST7735_CMD_RDDST 0x09
#define ST7735_CMD_SLPIN 0x10
#define ST7735_CMD_SLPOUT 0x11
#define ST7735_CMD_PTLON 0x12
#define ST7735_CMD_NORON 0x13
#define ST7735_CMD_INVOFF 0x20
#define ST7735_CMD_INVON 0x21
#define ST7735_CMD_DISPOFF 0x28
#define ST7735_CMD_DISPON 0x29
#define ST7735_CMD_CASET 0x2A
#define ST7735_CMD_RASET 0x2B
#define ST7735_CMD_RAMWR 0x2C
#define ST7735_CMD_RAMRD 0x2E
#define ST7735_CMD_VSCRDEF 0x33
#define ST7735_CMD_VSCSAD 0x37
#define ST7735_CMD_COLMOD 0x3A
#define ST7735_CMD_MADCTL 0x36
#define ST7735_CMD_FRMCTR1 0xB1
#define ST7735_CMD_FRMCTR2 0xB2
#define ST7735_CMD_FRMCTR3 0xB3
#define ST7735_CMD_INVCTR 0xB4
#define ST7735_CMD_DISSET5 0xB6
#define ST7735_CMD_PWCTR1 0xC0
#define ST7735_CMD_PWCTR2 0xC1
#define ST7735_CMD_PWCTR3 0xC2
#define ST7735_CMD_PWCTR4 0xC3
#define ST7735_CMD_PWCTR5 0xC4
#define ST7735_CMD_VMCTR1 0xC5
#define ST7735_CMD_RDID1 0xDA
#define ST7735_CMD_RDID2 0xDB
#define ST7735_CMD_RDID3 0xDC
#define ST7735_CMD_RDID4 0xDD
#define ST7735_CMD_PWCTR6 0xFC
#define ST7735_CMD_GMCTRP1 0xE0
#define ST7735_CMD_GMCTRN1 0xE1

// Display dimensions (define according to your display model)
#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160

// SPI settings
#define ST7735_SPI_PORT spi1
#define ST7735_SPI_BAUDRATE 32000000 // 32 MHz

// Color definitions (RGB565 format)
#define ST7735_BLACK 0x0000
#define ST7735_RED 0xF800
#define ST7735_GREEN 0x07E0
#define ST7735_BLUE 0x001F
#define ST7735_WHITE 0xFFFF

const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x03, 0x00, 0x03, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /

    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9

    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?

    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O

    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z

    {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // Backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `

    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o

    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z

    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}  // ~
};

class ST7735
{
private:
    spi_inst_t *m_spi;
    uint m_baudrate;
    uint m_sck_pin;
    uint m_mosi_pin;
    uint m_cs_pin;
    uint m_dc_pin;
    uint m_rst_pin;

private:
    void write_command(uint8_t cmd) const;
    void write_data(uint8_t data) const;
    void write_data_buffer(const uint8_t *buffer, size_t size) const;
    void set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) const;
    void reset() const;

public:
    ST7735(spi_inst_t *spi, uint baudrate, uint sck_pin, uint mosi_pin, uint cs_pin, uint dc_pin, uint rst_pin);
    void init_red() const;
    void draw_pixel(uint8_t x, uint8_t y, uint16_t color) const;
    void fill_screen(uint16_t color) const;
    // void draw_char(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg, uint8_t scale);
    void draw_char(uint8_t x, uint8_t y, char c, uint16_t color, uint8_t scale) const;
    // void draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg, uint8_t scale);
    void draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint8_t scale) const;
};
