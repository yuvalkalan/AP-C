#pragma once

#include <string>
#include "pico/stdlib.h"
#include "ST7735/ST7735.h"

class GraphicsRect
{
private:
    int m_x, m_y, m_width, m_height;

public:
    GraphicsRect(int x, int y, int width, int height);
    virtual ~GraphicsRect();
    // getters:
    int top();
    int bottom();
    int left();
    int right();
    int center_x();
    int center_y();
    // setters:
    void top(int value);
    void bottom(int value);
    void left(int value);
    void right(int value);
    void center_x(int value);
    void center_y(int value);
    // display
    virtual void draw(ST7735 &display, uint16_t color);
};

class GraphicsText : public GraphicsRect
{
private:
    std::string m_string;
    uint8_t m_scale;

public:
    GraphicsText(int x, int y, std::string string, uint8_t scale);
    ~GraphicsText() override;
    void draw(ST7735 &display, uint16_t color) override;
};
