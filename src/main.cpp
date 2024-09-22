#include "pico/stdlib.h"
#include "access_point/access_point.h"
#include "DS3231/DS3231.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "hardware/spi.h"
#include "ST7735/ST7735.h"

// display pins
#define ST7735_PIN_CS 13   // Chip Select
#define ST7735_PIN_DC 9    // Data/Command
#define ST7735_PIN_RST 8   // Reset
#define ST7735_PIN_SCK 10  // SPI Clock
#define ST7735_PIN_MOSI 11 // SPI MOSI (Master Out Slave In)

void ap_mode(Settings &settings)
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
        sleep_ms(1000);
    }
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
    strftime(buffer, sizeof(buffer), "%Y-%m-%d\n%H:%M:%S", &timeinfo);
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

int main()
{
    int error = init_all();
    if (error)
        return error;
    sleep_ms(2000);

    Settings settings;
    // settings.reset(); // TODO: remove this line

    ST7735 display(ST7735_SPI_PORT, ST7735_SPI_BAUDRATE, ST7735_PIN_SCK, ST7735_PIN_MOSI, ST7735_PIN_CS, ST7735_PIN_DC, ST7735_PIN_RST);
    display.init_red();
    display.fill_screen(ST7735_BLACK);

    if (!settings.exist())
    {
        // printf("%s", html_content);
        ap_mode(settings);
        tm current_time = settings.get_current_time();
        setDS3231Time(&current_time);
    }
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
    // Print the time
    while (true)
    {
        rtc_get_datetime(&t);
        tm time;
        time.tm_year = t.year - 1900;
        time.tm_mon = t.month - 1;
        time.tm_mday = t.day;
        time.tm_wday = t.dotw;
        time.tm_hour = t.hour;
        time.tm_min = t.min;
        time.tm_sec = t.sec;

        // display.fill_screen(ST7735_BLACK);
        display.draw_text(5, 5, tmToString(time).c_str(), ST7735_WHITE, 2);
        sleep_ms(100);
    }
    return 0;
}
