/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019, Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the Async project.
 */

#include <stdio.h>
#include "async.h"
#include "https_get.h"

const char certificate[] = (
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDCjCCAfICCQCoXfSQydXTczANBgkqhkiG9w0BAQsFADBHMQswCQYDVQQGEwJT\n"
    "RTETMBEGA1UECAwKU29tZS1TdGF0ZTEOMAwGA1UECgwFU2ltYmExEzARBgNVBAMM\n"
    "CmZvb2Jhci5vcmcwHhcNMTcwMTA3MTgxOTQ3WhcNMTgwMTA3MTgxOTQ3WjBHMQsw\n"
    "CQYDVQQGEwJTRTETMBEGA1UECAwKU29tZS1TdGF0ZTEOMAwGA1UECgwFU2ltYmEx\n"
    "EzARBgNVBAMMCmZvb2Jhci5vcmcwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
    "AoIBAQDugoc9amynDB/ODJzh2aqB6cnubSJEPlgB87jNG0akcbmUFt6BhPhMPSoP\n"
    "esHWl5OWscoga0cKrPURmMVVhfaeZLQGmrv5N4/liVlwae1n0gUEruX4d6MqSSDW\n"
    "3C/WKjCn647udZwyzCvyrvPOq0qAzaxR4EFRdwjSEPO5sDw2zxeTjGW2WxaH9PEu\n"
    "C8vaNqTsLYl3YBkR3zVCbuQXTQhStsv3gT4Yhz2wJpY0yyWyDiaBkGKpdxJQiNAd\n"
    "x5JKSqtRshlYZM3+cdKLywNoYUnezp6Wm4mzz09TCFv+esJ0h7/6pMdVjhxLsAg5\n"
    "ZbZyrtNIapN07AjIJS4qjkJ/HUC3AgMBAAEwDQYJKoZIhvcNAQELBQADggEBAGHS\n"
    "U5AvDESzTNoak5HHx166bp5+bvECvJg45GMDGhqKeFoOlR34u2b+wyTm/piz3nYJ\n"
    "12kn+BbG/PwGfndL0kZYRz46fY8Rf+MxCFfcglweDJhA6ULNpera11OC35Q/lKn5\n"
    "M6w6eQkZMB4VqwigvDGHGpXRTIhJHHoR2VFBFGoPTLrXilChUpiXi9DmuYXJ/19x\n"
    "sxOVwvvO/m/6g68G+uZYUoCsQsKllM2fgdNLTzbYvnFtsq5QnZS8m7CoZgEy2c3m\n"
    "VrrPsfwmyhwejDawjz2epSLNXaaDeSz4g1cQes+oehaA1IwFfKhb9tdiWUm48VuU\n"
    "oaFZ8e1HofYUy+65pws=\n"
    "-----END CERTIFICATE-----\n");

static void do_get(struct https_get_t *self_p)
{
    printf("Getting 'https://localhost:4443/main.c'...\n");
    async_tcp_client_connect(&self_p->tcp, "localhost", 4443);
}

static void on_ssl_connected(struct async_ssl_connection_t *connection_p, int res)
{
    static const char request[] = (
        "GET /main.c HTTP/1.1\r\n"
        "\r\n");

    if (res == 0) {
        async_ssl_connection_write(connection_p, &request[0], sizeof(request) - 1);
        printf("--------------------- HTTP GET BEGIN ---------------------\n");
    } else {
        printf("SSL open failed.\n");
    }
}

static void on_ssl_disconnected(struct async_ssl_connection_t *connection_p)
{
    (void)connection_p;

    printf("---------------------- HTTP GET END ----------------------\n");
}

static ssize_t ssl_transport_read(struct async_ssl_connection_t *connection_p,
                                  void *buf_p,
                                  size_t size)
{
    struct https_get_t *self_p;

    self_p = async_container_of(connection_p, typeof(*self_p), ssl.connection);

    return (async_tcp_client_read(&self_p->tcp, buf_p, size));
}

static void ssl_transport_write(struct async_ssl_connection_t *connection_p,
                                const void *buf_p,
                                size_t size)
{
    struct https_get_t *self_p;

    self_p = async_container_of(connection_p, typeof(*self_p), ssl.connection);
    async_tcp_client_write(&self_p->tcp, buf_p, size);
}

static void on_ssl_input(struct async_ssl_connection_t *connection_p)
{
    char buf[64];
    size_t size;

    size = async_ssl_connection_read(connection_p, &buf[0], sizeof(buf));

    fwrite(&buf[0], 1, size, stdout);
}

static void on_tcp_connected(struct async_tcp_client_t *tcp_p, int res)
{
    struct https_get_t *self_p;

    self_p = async_container_of(tcp_p, typeof(*self_p), tcp);

    if (res == 0) {
        async_ssl_connection_open(&self_p->ssl.connection,
                                  &self_p->ssl.context,
                                  on_ssl_connected,
                                  on_ssl_disconnected,
                                  on_ssl_input,
                                  ssl_transport_read,
                                  ssl_transport_write);
    } else {
        printf("Connect failed.\n");
    }
}

static void on_tcp_disconnected(struct async_tcp_client_t *tcp_p)
{
    (void)tcp_p;

    printf("TCP disconnected\n");
}

static void on_tcp_input(struct async_tcp_client_t *tcp_p)
{
    struct https_get_t *self_p;

    self_p = async_container_of(tcp_p, typeof(*self_p), tcp);
    async_ssl_connection_on_transport_input(&self_p->ssl.connection);
}

void https_get_init(struct https_get_t *self_p, struct async_t *async_p)
{
    async_tcp_client_init(&self_p->tcp,
                          on_tcp_connected,
                          on_tcp_disconnected,
                          on_tcp_input,
                          async_p);
    async_ssl_context_init(&self_p->ssl.context,
                           async_ssl_protocol_tls_v1_0_t);
    async_ssl_context_load_verify_location(&self_p->ssl.context,
                                           &certificate[0]);
    async_call(async_p, (async_func_t)do_get, self_p);
}
