/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "dhcpserver/dhcpserver.h"
#include "dnsserver/dnsserver.h"

#define TCP_PORT 80
#define POLL_TIME_S 5
#define HTTP_GET "GET"
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define LED_TEST_BODY "<html><body><h1>Hello from Pico W.</h1><p>Led is %s</p><p><a href=\"?led=%d\">Turn led %s</a></body></html>"
#define LED_PARAM "led=%d"
#define LED_TEST "/ledtest"
#define LED_GPIO 0
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s" LED_TEST "\n\n"

#define AP_WIFI_NAME "tester"
#define AP_WIFI_PASSWORD "12345678"

typedef struct TCP_SERVER_T_
{
    tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
    async_context_t *context;
} TCP_SERVER_T;
typedef struct TCP_CONNECT_STATE_T_
{
    tcp_pcb *pcb;
    int sent_len;
    char headers[128];
    char result[256];
    int header_len;
    int result_len;
    ip_addr_t *gw;
} TCP_CONNECT_STATE_T;

static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T *con_state, tcp_pcb *client_pcb, err_t close_err)
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

static void tcp_server_close(TCP_SERVER_T *state)
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
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    printf("tcp_server_sent %u\n", len);
    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->result_len)
    {
        printf("all done\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

static int test_server_content(const char *request, const char *params, char *result, size_t max_result_len)
{
    int len = 0;
    if (strncmp(request, LED_TEST, sizeof(LED_TEST) - 1) == 0)
    {
        // Get the state of the led
        bool value;
        cyw43_gpio_get(&cyw43_state, LED_GPIO, &value);
        int led_state = value;

        // See if the user changed it
        if (params)
        {
            int led_param = sscanf(params, LED_PARAM, &led_state);
            if (led_param == 1)
            {
                if (led_state)
                {
                    // Turn led on
                    cyw43_gpio_set(&cyw43_state, 0, true);
                }
                else
                {
                    // Turn led off
                    cyw43_gpio_set(&cyw43_state, 0, false);
                }
            }
        }
        // Generate result
        if (led_state)
        {
            len = snprintf(result, max_result_len, LED_TEST_BODY, "ON", 0, "OFF");
        }
        else
        {
            len = snprintf(result, max_result_len, LED_TEST_BODY, "OFF", 1, "ON");
        }
    }
    return len;
}

err_t tcp_server_recv(void *arg, tcp_pcb *pcb, pbuf *p, err_t err)
{
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    if (!p)
    {
        printf("connection closed\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    assert(con_state && con_state->pcb == pcb);
    if (p->tot_len > 0)
    {
        printf("tcp_server_recv %d err %d\n", p->tot_len, err);
        // Copy the request into the buffer
        pbuf_copy_partial(p, con_state->headers, p->tot_len > sizeof(con_state->headers) - 1 ? sizeof(con_state->headers) - 1 : p->tot_len, 0);

        // Handle GET request
        if (strncmp(HTTP_GET, con_state->headers, sizeof(HTTP_GET) - 1) == 0)
        {
            char *request = con_state->headers + sizeof(HTTP_GET); // + space
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
            con_state->result_len = test_server_content(request, params, con_state->result, sizeof(con_state->result));
            printf("Request: %s?%s\n", request, params);
            printf("Result: %d\n", con_state->result_len);

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
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    printf("tcp_server_poll_fn\n");
    return tcp_close_client_connection(con_state, pcb, ERR_OK); // Just disconnect clent?
}

static void tcp_server_err(void *arg, err_t err)
{
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T *)arg;
    if (err != ERR_ABRT)
    {
        printf("tcp_client_err_fn %d\n", err);
        tcp_close_client_connection(con_state, con_state->pcb, err);
    }
}

static err_t tcp_server_accept(void *arg, tcp_pcb *client_pcb, err_t err)
{
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;
    if (err != ERR_OK || client_pcb == NULL)
    {
        printf("failure in accept\n");
        return ERR_VAL;
    }
    printf("client connected\n");

    // Create the state for the connection
    TCP_CONNECT_STATE_T *con_state = new TCP_CONNECT_STATE_T();
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
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;
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

// This "worker" function is called to safely perform work when instructed by key_pressed_func
void key_pressed_worker_func(async_context_t *context, async_when_pending_worker_t *worker)
{
    assert(worker->user_data);
    printf("Disabling wifi\n");
    cyw43_arch_disable_ap_mode();
    ((TCP_SERVER_T *)(worker->user_data))->complete = true;
}

static async_when_pending_worker_t key_pressed_worker = {
    .do_work = key_pressed_worker_func};

void key_pressed_func(void *param)
{
    assert(param);
    int key = getchar_timeout_us(0); // get any pending key press but don't wait
    if (key == 'd' || key == 'D')
    {
        // We are probably in irq context so call wifi in a "worker"
        async_context_set_work_pending(((TCP_SERVER_T *)param)->context, &key_pressed_worker);
    }
}

void start_access_point()
{
    TCP_SERVER_T *state = new TCP_SERVER_T();
    cyw43_arch_init();

    // Get notified if the user presses a key
    state->context = cyw43_arch_async_context();
    key_pressed_worker.user_data = state;
    async_context_add_when_pending_worker(cyw43_arch_async_context(), &key_pressed_worker);
    stdio_set_chars_available_callback(key_pressed_func, state);

    cyw43_arch_enable_ap_mode(AP_WIFI_NAME, AP_WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t mask;
    IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

    // Start the dhcp server
    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &state->gw, &mask);

    // Start the dns server
    dns_server_t dns_server;
    dns_server_init(&dns_server, &state->gw);

    if (!tcp_server_open(state, AP_WIFI_NAME))
    {
        printf("failed to open server\n");
        return;
    }

    state->complete = false;
    while (!state->complete)
    {
        sleep_ms(1000);
    }
    tcp_server_close(state);
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    cyw43_arch_deinit();
}

int main()
{
    stdio_init_all();
    start_access_point();
    return 0;
}
