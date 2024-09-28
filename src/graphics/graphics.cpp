#include "graphics.h"

static int get_string_width(const std::string &string, uint8_t scale)
{
    int current_length = 0;
    int max_length = 0;
    for (size_t i = 0; i < string.length(); i++)
    {
        if (string[i] == '\n')
        {
            max_length = current_length > max_length ? current_length : max_length;
            current_length = 0;
        }
        else
        {
            current_length++;
        }
    }
    max_length = current_length > max_length ? current_length : max_length;
    return (max_length * scale * (sizeof(font5x7[0]) + 1) - scale);
}
static int get_string_height(const std::string &string, uint8_t scale)
{
    int num_of_lines = 1;
    for (size_t i = 0; i < string.length(); i++)
    {
        if (string[i] == '\n')
        {
            num_of_lines++;
        }
    }
    return num_of_lines * scale * 8;
}

GraphicsRect::GraphicsRect(int x, int y, int width, int height) : m_x(x), m_y(y), m_width(width), m_height(height)
{
}
GraphicsRect::~GraphicsRect()
{
}
int GraphicsRect::top()
{
    return m_y;
}
int GraphicsRect::bottom()
{
    return m_y + m_height;
}
int GraphicsRect::left()
{
    return m_x;
}
int GraphicsRect::right()
{
    return m_x + m_width;
}
int GraphicsRect::center_x()
{
    return m_x + m_width / 2;
}
int GraphicsRect::center_y()
{
    return m_y + m_height / 2;
}
void GraphicsRect::top(int value)
{
    m_y = value;
}
void GraphicsRect::bottom(int value)
{
    m_y = value - m_height;
}
void GraphicsRect::left(int value)
{
    m_x = value;
}
void GraphicsRect::right(int value)
{
    m_x = value - m_width;
}
void GraphicsRect::center_x(int value)
{
    m_x = value - m_width / 2;
}
void GraphicsRect::center_y(int value)
{
    m_y = value - m_height / 2;
}
void GraphicsRect::draw(ST7735 &display, uint16_t color)
{
    for (int x = 0; x < m_width; x++)
    {
        for (int y = 0; y < m_height; y++)
        {
            display.draw_pixel(x, y, color);
        }
    }
}

GraphicsText::GraphicsText(int x, int y, std::string string, uint8_t scale) : GraphicsRect(x,
                                                                                           y,
                                                                                           get_string_width(string, scale),
                                                                                           get_string_height(string, scale)),
                                                                              m_string(string),
                                                                              m_scale(scale)
{
}
GraphicsText::~GraphicsText()
{
}
void GraphicsText::draw(ST7735 &display, uint16_t color)
{
    display.draw_text(left(), top(), m_string, color, m_scale);
}
