#pragma once
#include "pico/stdlib.h"
#include "hardware/spi.h"

// ST7735 command definitions
// #define ST7735_CMD_NOP 0x00
// #define ST7735_CMD_SWRESET 0x01
// #define ST7735_CMD_SLPOUT 0x11
// #define ST7735_CMD_DISPON 0x29
// #define ST7735_CMD_CASET 0x2A
// #define ST7735_CMD_RASET 0x2B
// #define ST7735_CMD_RAMWR 0x2C
// #define ST7735_CMD_MADCTL 0x36
// #define ST7735_CMD_COLMOD 0x3A

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
#define ST7735_SPI_BAUDRATE 20000000 // 20 MHz is often the max supported by the ST7735

// Color definitions (RGB565 format)
#define ST7735_BLACK 0x0000
#define ST7735_RED 0xF800
#define ST7735_GREEN 0x07E0
#define ST7735_BLUE 0x001F
#define ST7735_WHITE 0xFFFF

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
    void write_command(uint8_t cmd);
    void write_data(uint8_t data);
    void write_data_buffer(const uint8_t *buffer, size_t size);
    void set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
    void reset();

public:
    ST7735(spi_inst_t *spi, uint baudrate, uint sck_pin, uint mosi_pin, uint cs_pin, uint dc_pin, uint rst_pin);
    void init_red();
    void draw_pixel(uint8_t x, uint8_t y, uint16_t color);
    void fill_screen(uint16_t color);
};
