/*
 * mylocalhost.c
 *
 * Generic WebSocket gateway to a PC's resources.
 *
 * Copyright (C) 2011 Jean-Pierre Gygax <gygax@practicomp.ch>
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <webserver/webserver.h>
#include <websocket/websocket.h>
#include <wsproxy/wsproxy.h>
#include "localfs.h"

/* Windows/Visual Studio quirks */

#ifdef _WIN32
#pragma warning(disable:4996)
#define close _close
#define strdup _strdup
#else
#define closesocket close
#endif

/* Module imports */

int 
tcp_proxying(wsk_ctx_t *ctx, const char *location, void *userdata);

/* Debugging */

#define __LOG(stream, ...) \
{ \
fprintf(stream, __VA_ARGS__); \
fprintf(stream, "\n" ); \
}

#define LOG_MSG(...) __LOG(stdout, __VA_ARGS__);
#define LOG_ERR(...) __LOG(stderr, __VA_ARGS__);
#define LOG_DBG LOG_MSG

static struct option options[] = {
    { "help",    no_argument,        NULL, 'h' },
    /* { "port",    required_argument,    NULL, 'p' },
    { "ssl",    no_argument,        NULL, 's' },
    { "killmask",    no_argument,        NULL, 'k' },
    { "interface",  required_argument,     NULL, 'i' },
    { "closetest",  no_argument,        NULL, 'c' },*/
    { NULL, 0, 0, 0 }
};

static int
extract_command(const char *location, char *buffer)
{
    const char *p;
    char *q;

    // Extract command (we ignore the path here)
    p = strrchr(location, '/'); assert(p);
    p++; // skip the last slash
    for (q = buffer; *p && *p != '?'; p++, q ++) *q = *p; *q = '\0';
    LOG_DBG("Command: \"%s\"", buffer);

    return 1;
}

static int
extract_parameter(const char *location, const char *name, char *buffer, size_t bsize)
{
    const char *p, *q;
    size_t n;

    p = strchr(location, '?');
    if (p == NULL) return -1;
    p ++;

    while (p != NULL && *p != '\0')
    {
        if ((q = strchr(p, '=')) == NULL) {
            LOG_ERR("Valueless parameter string");
            return 0; }
        // Is this the parameter we've been looking for ?
        if (strncmp(name, p, q - p) == 0) {
            p = q + 1;
            q = strchr(p, '&');
            n = q == NULL ? strlen(p) : q - p;
            if (wsv_url_decode(p, n, buffer, bsize, 0) == 0) {
                LOG_ERR("Error decoding parameter \"%s\"", name );
                return 0; }
            return 1;
        }
        else {
            p = strchr(p, '&');
            if (p != NULL) p ++;
        }
    }

    return 0; // parameter not found
}

static int
connection_handler(wsk_ctx_t *ctx, const char *location, void *userdata)
{
    char command[64];
    char path[1024];
    
    LOG_DBG("%s: location=%s\n", __FUNCTION__, location);

    if (!extract_command(location, command)) {
        LOG_ERR("No command passed in WebSocket connection header");
        goto fail; }

    if (strcmp(command, "$directory") == 0) {
        // Get path
        if (!extract_parameter(location, "path", path, 1024)) {
            LOG_ERR("$directory command received but no path specified");
            goto fail; }
        // Read the specified directory and send it back
        if (list_directory(ctx, path) < 0) {
            LOG_ERR("Error listing directory \"%s\"", path);
            goto fail; }
    }
    else if (strcmp(command, "$tcp") == 0) {
        return tcp_proxying(ctx, location, userdata);
    }
    else { LOG_ERR("Unknown command \"%s\"", command); goto fail; }

    return 0;
    
fail:
    return -1;
}

int 
main(int argc, char **argv)
{
    int n = 0;
    //const char *cert_path = "mylocalhost.pem";
    //const char *key_path = "mylocalhost.key.pem";
    //int use_ssl = 0;
    //ws_listener_t listener;
    //wsp_target_t target;
    wsv_settings_t settings;
    wsk_service_t *wsksvc;
    //int opts = 0;

    fprintf(stderr, "MyLocalHost server\n"
            "(C) Copyright 2011 Jean-Pierre Gygax <gygax@practicomp.ch>\n" );

    //memset(&settings, sizeof(settings), 0);
    settings.certfile = NULL;
    settings.keyfile = NULL;
    strcpy(settings.listen_host, "localhost");
    settings.listen_port = 7681; // TODO: symbolic constant
    settings.ssl_policy = wsv_allow_ssl;
    settings.handler = NULL; // use the default handler
    settings.protocols = NULL;
    settings.userdata = NULL;

    wsksvc = wsk_extend_webservice(&settings, connection_handler, NULL);
    if (!wsksvc) {
        fprintf(stderr, "Failed to extend web service with WebSocket protocol");
        return -1;
    }
    
    while (n >= 0) {
        n = getopt_long(argc, argv, "ci:khsp:", options, NULL); // TODO: adapt
        if (n < 0)
            continue;
        switch (n) {
        /* case 's':
            use_ssl = 1;
            break; */
        /* case 'k':
            opts = LWS_SERVER_OPTION_DEFEAT_CLIENT_MASK;
            break; */
        case 'p':
            settings.listen_port = atoi(optarg);
            break;
        /* case 'i':
            strncpy(interface_name, optarg, sizeof interface_name);
            interface_name[(sizeof interface_name) - 1] = '\0';
            interface = interface_name;
            break; */
        case 'h':
            fprintf(stderr, "Usage: mylocalhost "
                         "[--port=<p>]\n");
            exit(1);
        }
    }

    //if (!use_ssl)
    //    cert_path = key_path = NULL;

    wsv_start_server(&settings);

    return 0;
}
