#include "pico/stdlib.h"
#include "access_point/access_point.h"
#include "DS3231/DS3231.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "hardware/spi.h"
#include "ST7735/ST7735.h"
#include "Button/Button.h"
#include "TimeMaster/TimeMaster.h"
// display pins -----------------------
#define ST7735_PIN_CS 13   // Chip Select
#define ST7735_PIN_DC 9    // Data/Command
#define ST7735_PIN_RST 8   // Reset
#define ST7735_PIN_SCK 10  // SPI Clock
#define ST7735_PIN_MOSI 11 // SPI MOSI (Master Out Slave In)
// ------------------------------------
// button pin -------------------------
#define BUTTON_PIN 16
// ------------------------------------
// display modes ----------------------
#define MODE_CLOCK 0
#define MODE_COUNTER 1
#define MODE_BIRTHDAY 2
// ------------------------------------

void ap_mode(tm &current_time, Settings &settings, ST7735 &display)
{
    TCPServer *state = new TCPServer();
    if (!state)
    {
        return;
    }

    if (cyw43_arch_init())
    {
        return;
    }

    // Get notified if the user presses a key
    auto context = cyw43_arch_async_context();

    const char *ap_name = AP_WIFI_NAME;
    const char *password = AP_WIFI_PASSWORD;

    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t mask;
    IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

    // Start the dhcp server
    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &state->gw, &mask);

    // Start the dns server
    dns_server_t dns_server;
    dns_server_init(&dns_server, &state->gw);

    state->settings = &settings;

    if (!tcp_server_open(state, ap_name))
    {
        return;
    }
    display.fill(ST7735_BLACK);
    state->complete = false;
    while (!state->complete)
    {
        // check for disable wifi
        int key = getchar_timeout_us(0);
        if (key == 'd' || key == 'D' || settings.exist())
        {
            printf("Disabling wifi\n");
            cyw43_arch_disable_ap_mode();
            state->complete = true;
        }
        display.fill(ST7735_BLACK);
        display.draw_text(0, 0, "ap mode!", ST7735_WHITE, 2);
        display.update();
        sleep_ms(1000);
    }
    display.fill(ST7735_BLACK);
    current_time = settings.get_current_time();
    tcp_server_close(state);
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    cyw43_arch_deinit();
    delete state;
}

std::string tmToString(const tm &timeinfo)
{
    char buffer[80];
    // Format the time as desired, e.g., "YYYY-MM-DD HH:MM:SS"
    strftime(buffer, sizeof(buffer), "%d-%m-%Y\n%H:%M:%S", &timeinfo);
    return std::string(buffer);
}

static int init_all()
{
    stdio_init_all();
    int initStatus = initDS3231();
    if (initStatus)
    {
        printf("Error occurred during DS3231 initialization. %s\n", ds3231ErrorString(initStatus));
        return 1;
    }
    else
    {
        printf("DS3231 initialized.\n");
    }
    rtc_init();
    return 0;
}

void copy_DS3231_time()
{
    tm current_time;
    readDS3231Time(&current_time);

    datetime_t t = {
        .year = int16_t(current_time.tm_year + 1900),
        .month = int8_t(current_time.tm_mon + 1),
        .day = int8_t(current_time.tm_mday),
        .dotw = int8_t(current_time.tm_wday), // irrelevant, calculate automatically
        .hour = int8_t(current_time.tm_hour),
        .min = int8_t(current_time.tm_min),
        .sec = int8_t(current_time.tm_sec)};
    rtc_set_datetime(&t);
    sleep_ms(10); // wait some time for rtc to reset
}

tm inline get_rtc_time()
{
    datetime_t t;
    rtc_get_datetime(&t);
    tm time;
    time.tm_year = t.year - 1900;
    time.tm_mon = t.month - 1;
    time.tm_mday = t.day;
    time.tm_wday = t.dotw;
    time.tm_hour = t.hour;
    time.tm_min = t.min;
    time.tm_sec = t.sec;
    return time;
}

bool inline tm_is_bigger(const tm &time1, const tm &time2)
{
    if (time1.tm_year == time2.tm_year)
    {
        if (time1.tm_mon == time2.tm_mon)
        {
            if (time1.tm_mday == time2.tm_mday)
            {
                if (time1.tm_hour == time2.tm_hour)
                {
                    if (time1.tm_min == time2.tm_min)
                    {
                        if (time1.tm_sec == time2.tm_sec)
                            return false;
                        return time1.tm_sec > time2.tm_sec;
                    }
                    return time1.tm_min > time2.tm_min;
                }
                return time1.tm_hour > time2.tm_hour;
            }
            return time1.tm_mday > time2.tm_mday;
        }
        return time1.tm_mon > time2.tm_mon;
    }
    return time1.tm_year > time2.tm_year;
}

int main()
{
    int error = init_all();
    if (error)
        return error;
    // sleep_ms(2000);
    printf("start!\n");

    Settings settings;
    ST7735 display(ST7735_SPI_PORT, ST7735_SPI_BAUDRATE, ST7735_PIN_SCK, ST7735_PIN_MOSI, ST7735_PIN_CS, ST7735_PIN_DC, ST7735_PIN_RST);
    Button btn(BUTTON_PIN);
    display.init_red();
    display.fill(ST7735_BLACK);
    if (!settings.exist())
    {
        // printf("%s", html_content);
        tm current_time;
        ap_mode(current_time, settings, display);
        // tm current_time = settings.get_current_time();
        setDS3231Time(&current_time);
    }
    copy_DS3231_time();
    absolute_time_t boot_time = get_absolute_time();
    tm start_time = settings.get_start_time();
    tm birthday_time = settings.get_start_time();
    tm display_time;
    uint8_t mode = MODE_CLOCK;
    uint8_t clock_cx = ST7735_WIDTH / 2, clock_radius = (ST7735_WIDTH > ST7735_HEIGHT ? ST7735_HEIGHT : ST7735_WIDTH) / 4, clock_cy = ST7735_HEIGHT - clock_radius - 10;
    while (true)
    {
        display.fill(ST7735_BLACK);
        btn.update();

        if (btn.clicked())
        {
            mode = (mode + 1) % 3;
            printf("new mode is %d\n", mode);
            display.fill(ST7735_BLACK);
        }
        if (btn.hold_down())
        {
            settings.reset();
        }
        tm current_time = get_rtc_time();

        if (mode == MODE_CLOCK)
        {
            display_time = current_time;
            display.draw_text(5, 5, ("Clock:\n\n" + tmToString(display_time)).c_str(), ST7735_WHITE, 2);
        }
        else if (mode == MODE_COUNTER)
        {
            display_time = calculate_time_dif(start_time, current_time);
            display.draw_text(5, 5, ("Start:\n\n" + tmToString(display_time)).c_str(), ST7735_WHITE, 2);
        }
        else if (mode == MODE_BIRTHDAY)
        {
            birthday_time.tm_year = current_time.tm_year;
            if (tm_is_bigger(current_time, birthday_time))
                birthday_time.tm_year += 1;
            display_time = calculate_time_dif(current_time, birthday_time);
            display.draw_text(5, 5, ("Birthday:\n\n" + tmToString(display_time)).c_str(), ST7735_WHITE, 2);
        }
        // sleep_ms(100);
        int miliseconds = (to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(boot_time)) % 1000;
        float total_secs = display_time.tm_sec + miliseconds / 1000.0f;
        float total_mins = display_time.tm_min + total_secs / 60;
        float total_hours = display_time.tm_hour + total_mins / 60;
        float sec_angle = total_secs * 360 / 60 + 270;
        float min_angle = total_mins * 360 / 60 + 270;
        float hour_angle = total_hours * 360 / 12 + 270;

        display.draw_circle(clock_cx, clock_cy, clock_radius, 2, ST7735_WHITE);
        display.draw_line_with_angle(clock_cx, clock_cy, clock_radius * 0.85f, sec_angle, 2, ST7735_WHITE);
        display.draw_line_with_angle(clock_cx, clock_cy, clock_radius * 0.70f, min_angle, 3, ST7735_RED);
        display.draw_line_with_angle(clock_cx, clock_cy, clock_radius * 0.50f, hour_angle, 4, ST7735_BLUE);
        display.update();
    }
    return 0;
}
