// pico sdk libs ----------------------
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "access_point/dhcpserver/dhcpserver.h"
#include "access_point/dnsserver/dnsserver.h"
#include <tusb.h>
// ------------------------------------
// pure C/C++ libs ---------------------
#include <string>
#include <map>
#include <sstream>
// ------------------------------------
// html c-style index file ------------
#include "htmldata.cpp"
// ------------------------------------

// http server settings ---------------
#define TCP_PORT 80
#define POLL_TIME_S 5
#define HTTP_GET "GET"
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define PAGE_TITLE "/settings"
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s" PAGE_TITLE "\n\n"
// ------------------------------------
// AP Wifi settings -------------------
#define AP_WIFI_NAME "picow_test"
#define AP_WIFI_PASSWORD "password"
// ------------------------------------
// http req params --------------------
#define PARAM_CURRENT_DATE "currentDate"
#define PARAM_CURRENT_TIME "currentTime"
#define PARAM_START_DATE "startDate"
#define PARAM_START_TIME "startTime"
#define PARAM_BIRTHDAY_DATE "birthdayDate"
#define PARAM_BIRTHDAY_TIME "birthdayTime"
// ------------------------------------

class TCPServer
{
public:
    tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
    // async_context_t *context;
};
class TCPConnect
{
public:
    tcp_pcb *pcb;
    int sent_len;
    char headers[512];
    char result[8192];
    int header_len;
    int result_len;
    ip_addr_t *gw;
};

class Date
{
private:
    uint8_t m_day;
    uint8_t m_month;
    uint m_year;

public:
    Date(std::string date) : m_year(std::stoi(date.substr(0, 4))),
                             m_month(std::stoi(date.substr(5, 2))),
                             m_day(std::stoi(date.substr(8, 2)))
    {
        // date format should be "yyyy-mm-dd"
    }
};

class Time
{
private:
    uint8_t m_hours;
    uint8_t m_minutes;
    uint8_t m_seconds;
    Time(std::string time) : m_hours(std::stoi(time.substr(0, 2))), m_minutes(std::stoi(time.substr(5, 2))), m_seconds(0)
    {
        // time format should be "HH%3AMM"
    }
};

static err_t tcp_close_client_connection(TCPConnect *con_state, tcp_pcb *client_pcb, err_t close_err)
{
    if (client_pcb)
    {
        assert(con_state && con_state->pcb == client_pcb);
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK)
        {
            printf("close failed %d, calling abort\n", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if (con_state)
        {
            delete con_state;
        }
    }
    return close_err;
}
static void tcp_server_close(TCPServer *state)
{
    if (state->server_pcb)
    {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}
static err_t tcp_server_sent(void *arg, tcp_pcb *pcb, u16_t len)
{
    TCPConnect *con_state = (TCPConnect *)arg;
    // printf("tcp_server_sent %u\n", len);
    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->result_len)
    {
        // printf("all done\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

// Function to parse query string and return a map of key-value pairs
std::map<std::string, std::string> extract_params(const std::string &params)
{
    std::map<std::string, std::string> params_map;
    std::stringstream ss(params);
    std::string token;

    while (std::getline(ss, token, '&'))
    {

        size_t pos = token.find('=');

        if (pos != std::string::npos)
        {
            std::string key = token.substr(0, pos);
            std::string value = token.substr(pos + 1);
            printf("key = %s, value = %s", key.c_str(), value.c_str());
            params_map[key] = value;
        }
    }
    return params_map;
}

static int handle_http(const char *request, const char *params, char *result, size_t max_result_len)
{
    // debug -----------------------
    printf("request is <%s>\n", request);
    printf("params is <%s>\n", params);
    // -----------------------------
    int len = 0;
    if (strncmp(request, PAGE_TITLE, sizeof(PAGE_TITLE) - 1) == 0)
    {
        // See if the user sent params
        if (params)
        {
            printf("\ngot params!\n\n");
            auto params_map = extract_params(params);
            printf("current:\n\tdate is %s, time is %s\n", params_map[PARAM_CURRENT_DATE].c_str(), params_map[PARAM_CURRENT_TIME].c_str());
            printf("start:\n\tdate is %s, time is %s\n", params_map[PARAM_START_DATE].c_str(), params_map[PARAM_START_TIME].c_str());
            printf("birthday:\n\tdate is %s, time is %s\n", params_map[PARAM_BIRTHDAY_DATE].c_str(), params_map[PARAM_BIRTHDAY_TIME].c_str());
        }
        // Generate result
        len = snprintf(result, max_result_len, html_content);
    }
    // printf("result is <%s>\n", result);
    return len;
}
err_t tcp_server_recv(void *arg, tcp_pcb *pcb, pbuf *p, err_t err)
{
    TCPConnect *con_state = (TCPConnect *)arg;
    if (!p)
    {
        printf("connection closed\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    assert(con_state && con_state->pcb == pcb);
    if (p->tot_len > 0)
    {
        // printf("tcp_server_recv %d err %d\n", p->tot_len, err);
        // Copy the request into the buffer
        pbuf_copy_partial(p, con_state->headers, p->tot_len > sizeof(con_state->headers) - 1 ? sizeof(con_state->headers) - 1 : p->tot_len, 0);

        // Handle GET request
        if (strncmp(HTTP_GET, con_state->headers, sizeof(HTTP_GET) - 1) == 0)
        {
            char *request = con_state->headers + sizeof(HTTP_GET); // + space
            printf("HTTP REQUEST\n--------------------\nGET %s", request);
            char *params = strchr(request, '?');
            if (params)
            {
                if (*params)
                {
                    char *space = strchr(request, ' ');
                    *params++ = 0;
                    if (space)
                    {
                        *space = 0;
                    }
                }
                else
                {
                    params = NULL;
                }
            }

            // Generate content
            con_state->result_len = handle_http(request, params, con_state->result, sizeof(con_state->result));
            // printf("Request: %s?%s\n", request, params);
            // printf("Result: %d\n", con_state->result_len);

            // Check we had enough buffer space
            if (con_state->result_len > sizeof(con_state->result) - 1)
            {
                printf("Too much result data %d\n", con_state->result_len);
                return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
            }

            // Generate web page
            if (con_state->result_len > 0)
            {
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADERS,
                                                 200, con_state->result_len);
                if (con_state->header_len > sizeof(con_state->headers) - 1)
                {
                    printf("Too much header data %d\n", con_state->header_len);
                    return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
                }
            }
            else
            {
                // Send redirect
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_REDIRECT,
                                                 ipaddr_ntoa(con_state->gw));
                printf("Sending redirect %s", con_state->headers);
            }

            // Send the headers to the client
            con_state->sent_len = 0;
            err_t err = tcp_write(pcb, con_state->headers, con_state->header_len, 0);
            if (err != ERR_OK)
            {
                printf("failed to write header data %d\n", err);
                return tcp_close_client_connection(con_state, pcb, err);
            }

            // Send the body to the client
            if (con_state->result_len)
            {
                err = tcp_write(pcb, con_state->result, con_state->result_len, 0);
                if (err != ERR_OK)
                {
                    printf("failed to write result data %d\n", err);
                    return tcp_close_client_connection(con_state, pcb, err);
                }
            }
        }
        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}
static err_t tcp_server_poll(void *arg, tcp_pcb *pcb)
{
    TCPConnect *con_state = (TCPConnect *)arg;
    // printf("tcp_server_poll_fn\n");
    return tcp_close_client_connection(con_state, pcb, ERR_OK); // Just disconnect clent?
}
static void tcp_server_err(void *arg, err_t err)
{
    TCPConnect *con_state = (TCPConnect *)arg;
    if (err != ERR_ABRT)
    {
        // printf("tcp_client_err_fn %d\n", err);
        tcp_close_client_connection(con_state, con_state->pcb, err);
    }
}
static err_t tcp_server_accept(void *arg, tcp_pcb *client_pcb, err_t err)
{
    TCPServer *state = (TCPServer *)arg;
    if (err != ERR_OK || client_pcb == NULL)
    {
        printf("failure in accept\n");
        return ERR_VAL;
    }
    printf("client connected\n");

    // Create the state for the connection
    TCPConnect *con_state = new TCPConnect();
    if (!con_state)
    {
        printf("failed to allocate connect state\n");
        return ERR_MEM;
    }
    con_state->pcb = client_pcb; // for checking
    con_state->gw = &state->gw;

    // setup connection to client
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}
static bool tcp_server_open(void *arg, const char *ap_name)
{
    TCPServer *state = (TCPServer *)arg;
    printf("starting server on port %d\n", TCP_PORT);

    tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb)
    {
        printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if (err)
    {
        printf("failed to bind to port %d\n", TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb)
    {
        printf("failed to listen\n");
        if (pcb)
        {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    printf("Try connecting to '%s' (press 'd' to disable access point)\n", ap_name);
    return true;
}

int main()
{
    stdio_init_all();
    TCPServer *state = new TCPServer();
    if (!state)
    {
        printf("failed to allocate state\n");
        return 1;
    }

    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return 1;
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

    if (!tcp_server_open(state, ap_name))
    {
        printf("failed to open server\n");
        return 1;
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
    return 0;
}