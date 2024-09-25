#include "st7735.h"

ST7735::ST7735(spi_inst_t *spi, uint baudrate, uint sck_pin, uint mosi_pin, uint cs_pin, uint dc_pin, uint rst_pin)
    : m_spi(spi),
      m_baudrate(baudrate),
      m_sck_pin(sck_pin),
      m_mosi_pin(mosi_pin),
      m_cs_pin(cs_pin),
      m_dc_pin(dc_pin),
      m_rst_pin(rst_pin)
{
    // Initialize chosen SPI port
    spi_init(m_spi, m_baudrate);
    gpio_set_function(m_sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(m_mosi_pin, GPIO_FUNC_SPI);
    // Configure the control pins
    gpio_init(m_cs_pin);
    gpio_set_dir(m_cs_pin, GPIO_OUT);
    gpio_put(m_cs_pin, 1); // Deselect

    gpio_init(m_dc_pin);
    gpio_set_dir(m_dc_pin, GPIO_OUT);

    gpio_init(m_rst_pin);
    gpio_set_dir(m_rst_pin, GPIO_OUT);
}

// Low-level function to send commands
void ST7735::write_command(uint8_t cmd) const
{
    gpio_put(m_dc_pin, 0); // DC low for command
    gpio_put(m_cs_pin, 0); // CS low
    spi_write_blocking(m_spi, &cmd, 1);
    gpio_put(m_cs_pin, 1); // CS high
}

// Low-level function to send data
void ST7735::write_data(uint8_t data) const
{
    gpio_put(m_dc_pin, 1); // DC high for data
    gpio_put(m_cs_pin, 0); // CS low
    spi_write_blocking(m_spi, &data, 1);
    gpio_put(m_cs_pin, 1); // CS high
}

// Write multiple data bytes (useful for bulk transfers like pixel data)
void ST7735::write_data_buffer(const uint8_t *buffer, size_t size) const
{
    gpio_put(m_dc_pin, 1); // DC high for data
    gpio_put(m_cs_pin, 0); // CS low
    spi_write_blocking(m_spi, buffer, size);
    gpio_put(m_cs_pin, 1); // CS high
}

// Set the address window for pixel updates
void ST7735::set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) const
{
    // Column address set
    write_command(ST7735_CMD_CASET);
    x0 += 2;
    x1 += 2;
    y0 += 1;
    y1 += 1;
    uint8_t data[] = {0x00, x0, 0x00, x1};
    write_data_buffer(data, sizeof(data));

    // Row address set
    write_command(ST7735_CMD_RASET);
    uint8_t data2[] = {0x00, y0, 0x00, y1};
    write_data_buffer(data2, sizeof(data2));

    // Write to RAM
    write_command(ST7735_CMD_RAMWR);
}

// Draw a pixel at (x, y) with a given color
void ST7735::draw_pixel(uint8_t x, uint8_t y, uint16_t color) const
{
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT)
        return; // Bounds check
    set_addr_window(x, y, x, y);
    uint8_t color_data[] = {(uint8_t)(color >> 8), (uint8_t)(color & 0x00FF)}; // Split color into 2 bytes (RGB565)
    write_data_buffer(color_data, sizeof(color_data));
}

// Fill the entire screen with a color
void ST7735::fill_screen(uint16_t color) const
{
    set_addr_window(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);
    uint8_t color_data[] = {(uint8_t)(color >> 8), (uint8_t)(color & 0x00FF)}; // Split color into 2 bytes (RGB565)
    for (uint32_t i = 0; i < ST7735_WIDTH * ST7735_HEIGHT; i++)
    {
        write_data_buffer(color_data, sizeof(color_data));
    }
}

void ST7735::reset() const
{
    // Reset the device
    gpio_put(m_dc_pin, 0);
    gpio_put(m_rst_pin, 1);
    sleep_us(500);
    gpio_put(m_rst_pin, 0);
    sleep_us(500);
    gpio_put(m_rst_pin, 1);
    sleep_us(500);
}

void ST7735::init_red() const
{
    // Initialize a red tab version
    reset();
    write_command(ST7735_CMD_SWRESET); // Software reset
    sleep_us(150);
    write_command(ST7735_CMD_SLPOUT); // out of sleep mode.
    sleep_us(500);

    uint8_t data3[] = {0x01, 0x2C, 0x2D}; // fastest refresh, 6 lines front, 3 lines back.
    write_command(ST7735_CMD_FRMCTR1);    // Frame rate control.
    write_data_buffer(data3, sizeof(data3));

    write_command(ST7735_CMD_FRMCTR2); // Frame rate control.
    write_data_buffer(data3, sizeof(data3));

    uint8_t data6[] = {0x01, 0x2c, 0x2d, 0x01, 0x2c, 0x2d};
    write_command(ST7735_CMD_FRMCTR3); // Frame rate control.
    write_data_buffer(data6, sizeof(data6));
    sleep_us(10);

    uint8_t data1[] = {0};
    write_command(ST7735_CMD_INVCTR); // Display inversion control
    data1[0] = 0x07;                  // Line inversion.
    write_data_buffer(data1, sizeof(data1));

    write_command(ST7735_CMD_PWCTR1); // Power control
    data3[0] = 0xA2;
    data3[1] = 0x02;
    data3[2] = 0x84;
    write_data_buffer(data3, sizeof(data3));

    write_command(ST7735_CMD_PWCTR2); // Power control
    data1[0] = 0xC5;                  // VGH = 14.7V, VGL = -7.35V
    write_data_buffer(data1, sizeof(data1));

    uint8_t data2[] = {0, 0};
    write_command(ST7735_CMD_PWCTR3); // Power control
    data2[0] = 0x0A;                  // Opamp current small
    data2[1] = 0x00;                  // Boost frequency
    write_data_buffer(data2, sizeof(data2));

    write_command(ST7735_CMD_PWCTR4); // Power control
    data2[0] = 0x8A;                  // Opamp current small
    data2[1] = 0x2A;                  // Boost frequency
    write_data_buffer(data2, sizeof(data2));

    write_command(ST7735_CMD_PWCTR5); // Power control
    data2[0] = 0x8A;                  // Opamp current small
    data2[1] = 0xEE;                  // Boost frequency
    write_data_buffer(data2, sizeof(data2));

    write_command(ST7735_CMD_VMCTR1); // Power control
    data1[0] = 0x0E;
    write_data_buffer(data1, sizeof(data1));

    write_command(ST7735_CMD_INVOFF);

    write_command(ST7735_CMD_MADCTL); // Power control
    data1[0] = 0x00;
    write_data_buffer(data1, sizeof(data1));

    write_command(ST7735_CMD_COLMOD);
    data1[0] = 0x05;
    write_data_buffer(data1, sizeof(data1));

    write_command(ST7735_CMD_CASET); // Column address set.
    uint8_t windowLocData[] = {0x00, 0x00, 0x00, ST7735_WIDTH - 1};
    write_data_buffer(windowLocData, sizeof(windowLocData));

    write_command(ST7735_CMD_RASET); // Row address set.
    windowLocData[3] = ST7735_HEIGHT - 1;
    write_data_buffer(windowLocData, sizeof(windowLocData));

    uint8_t dataGMCTRP[] = {0x0f, 0x1a, 0x0f, 0x18, 0x2f, 0x28, 0x20, 0x22,
                            0x1f, 0x1b, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10};
    write_command(ST7735_CMD_GMCTRP1);
    write_data_buffer(dataGMCTRP, sizeof(dataGMCTRP));

    uint8_t dataGMCTRN[] = {0x0f, 0x1b, 0x0f, 0x17, 0x33, 0x2c, 0x29, 0x2e,
                            0x30, 0x30, 0x39, 0x3f, 0x00, 0x07, 0x03, 0x10};
    write_command(ST7735_CMD_GMCTRN1);
    write_data_buffer(dataGMCTRN, sizeof(dataGMCTRN));
    sleep_us(10);

    write_command(ST7735_CMD_DISPON);
    sleep_us(100);

    write_command(ST7735_CMD_NORON); // Normal display on.
    sleep_us(10);

    gpio_put(m_cs_pin, 1);
}

void ST7735::draw_char(uint8_t x, uint8_t y, char c, uint16_t color, uint8_t scale) const
{
    // draw char without backgound color
    if (c < 32 || c > sizeof(font5x7) / sizeof(font5x7[0]) - 1 + 32)
        return; // Only print characters in our font range

    const uint8_t *bitmap = font5x7[c - 32]; // Get font bitmap for character

    for (int i = 0; i < 5; i++) // 5 columns per character
    {
        uint8_t line = bitmap[i];
        for (int j = 0; j < 7; j++) // 7 rows per column
        {
            for (uint8_t sx = 0; sx < scale; sx++) // Draw scaled pixel
            {
                for (uint8_t sy = 0; sy < scale; sy++)
                {
                    draw_pixel(x + (i * scale) + sx, y + (j * scale) + sy, line & 0x1 ? color : ST7735_BLACK);
                }
            }
            line >>= 1; // Shift the line to get the next bit
        }
    }
}

void ST7735::draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint8_t scale) const
{
    int counter = 0;
    uint16_t ori_x = x;
    while (*text)
    {
        if (*text == '\n')
        {
            x = ori_x;
            y += 8 * scale;
            text++;
        }
        else
        {
            draw_char(x, y, *text++, color, scale);
            x += 6 * scale; // Move x cursor, 5 pixels for the character + 1 pixel space
        }
    }
}
