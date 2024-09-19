
#include "pico/stdlib.h"
#include "access_point/access_point.h"
#include "DS3231/DS3231.h"

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
        if (key == 'd' || key == 'D')
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
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buffer);
}
int main()
{
    stdio_init_all();
    sleep_ms(2000);
    Settings settings;
    sleep_ms(100);
    // settings.reset();
    settings.get_current_time();
    auto a = settings.get_current_time();
    auto b = settings.get_start_time();
    auto c = settings.get_birthday_time();
    printf("settings:\n\t%s\n\t%s\n\t%s\n",
           tmToString(a).c_str(),
           tmToString(b).c_str(),
           tmToString(c).c_str());
    // ap_mode(settings);
    int initStatus = initDS3231();
    struct tm str_bday;
    str_bday.tm_year = 2024 - 1900;
    str_bday.tm_mon = 9 - 1;
    str_bday.tm_mday = 19;
    str_bday.tm_hour = 16;
    str_bday.tm_min = 35;
    str_bday.tm_sec = 0;
    setDS3231Time(&str_bday);

    if (initStatus)
        printf("Error occurred during DS3231 initialization. %s\n", ds3231ErrorString(initStatus));
    else
        printf("DS3231 initialized.\n");
    struct tm datetime;
    while (1)
    {
        int status = readDS3231Time(&datetime);
        if (status)
            printf("Error reading time, %s\n", ds3231ErrorString(status));
        else
            printf("%s", asctime(&datetime));
        settings.set_current_time(datetime);
        sleep_ms(1000);
    }
}