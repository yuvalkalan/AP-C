#include "pico/stdlib.h"
#include "access_point/access_point.h"
#include "DS3231/DS3231.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "hardware/spi.h"
#include "ST7735/ST7735.h"
#include "Button/Button.h"
#include "TimeMaster/TimeMaster.h"
// display pins
#define ST7735_PIN_CS 13   // Chip Select
#define ST7735_PIN_DC 9    // Data/Command
#define ST7735_PIN_RST 8   // Reset
#define ST7735_PIN_SCK 10  // SPI Clock
#define ST7735_PIN_MOSI 11 // SPI MOSI (Master Out Slave In)
#define BUTTON_PIN 16

#define MODE_CLOCK 0
#define MODE_COUNTER 1
#define MODE_BIRTHDAY 2

void ap_mode(tm &current_time, Settings &settings, const ST7735 &display)
{
    TCPServer *state = new TCPServer();
    if (!state)
    {
        printf("failed to allocate state\n");
        return;
    }

    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
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
        printf("failed to open server\n");
        return;
    }
    display.fill_screen(ST7735_BLACK);
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
        display.draw_text(0, 0, "ap mode!", ST7735_WHITE, 2);
        sleep_ms(1000);
    }
    display.fill_screen(ST7735_BLACK);
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

tm rtc_time()
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



int main()
{
    int error = init_all();
    if (error)
        return error;
    // sleep_ms(2000);

    Settings settings;
    ST7735 display(ST7735_SPI_PORT, ST7735_SPI_BAUDRATE, ST7735_PIN_SCK, ST7735_PIN_MOSI, ST7735_PIN_CS, ST7735_PIN_DC, ST7735_PIN_RST);
    Button btn(BUTTON_PIN);
    display.init_red();
    display.fill_screen(ST7735_BLACK);
    if (!settings.exist())
    {
        // printf("%s", html_content);
        tm current_time;
        ap_mode(current_time, settings, display);
        // tm current_time = settings.get_current_time();
        setDS3231Time(&current_time);
    }
    copy_DS3231_time();
    // Print the time
    datetime_t t;
    tm start_time = settings.get_start_time();
    tm birthday_time = settings.get_start_time();
    uint8_t mode = MODE_CLOCK;
    auto start_time_tuple = std::make_tuple(start_time.tm_year, start_time.tm_mon, start_time.tm_mday, start_time.tm_hour, start_time.tm_min, start_time.tm_sec);

    while (true)
    {
        btn.update();
        if (btn.clicked())
        {
            mode = (mode + 1) % 3;
            printf("new mode is %d\n", mode);
            display.fill_screen(ST7735_BLACK);
        }
        if (btn.hold_down())
        {
            settings.reset();
        }
        rtc_get_datetime(&t);
        tm time;
        time.tm_year = t.year - 1900;
        time.tm_mon = t.month - 1;
        time.tm_mday = t.day;
        time.tm_wday = t.dotw;
        time.tm_hour = t.hour;
        time.tm_min = t.min;
        time.tm_sec = t.sec;
        auto current_time_tuple = std::make_tuple(time.tm_year, time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
        auto delta_time = calculate_time_dif(start_time_tuple, current_time_tuple);
        printf("%d-%d-%d %d:%d:%d\n", std::get<0>(delta_time), std::get<1>(delta_time), std::get<2>(delta_time), std::get<3>(delta_time), std::get<4>(delta_time), std::get<5>(delta_time));
        // display.draw_text(5,5,)
        // tm delta; // = deltaTime(time, start_time);
        // display.fill_screen(ST7735_BLACK);
        // if (mode == MODE_CLOCK)
        //     display.draw_text(5, 5, ("Clock:\n\n" + tmToString(time)).c_str(), ST7735_WHITE, 2);
        // else if (mode == MODE_COUNTER)
        //     display.draw_text(5, 5, ("Start:\n\n" + tmToString(delta)).c_str(), ST7735_WHITE, 2);
        // else if (mode == MODE_BIRTHDAY)
        //     display.draw_text(5, 5, ("Birthday:\n\n" + tmToString(time)).c_str(), ST7735_WHITE, 2);
        sleep_ms(100);
    }
    return 0;
}
