
#include "pico/stdlib.h"
#include "access_point/access_point.h"

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

int main()
{
    stdio_init_all();
    sleep_ms(500);
    Settings settings;
    sleep_ms(100);
    printf("settings are %d, %d, %d, %d, %d\n", settings.get_max_bright(), settings.get_mode(), settings.get_sensitivity(), settings.get_volume_threshold(), settings.get_config_temp_value());

    ap_mode(settings);
    return 0;
}