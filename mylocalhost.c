/*
 * mylocalhost
 *
 * Copyright (C) 2011 Jean-Pierre Gygax <gygax@practicomp.ch>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <webserver/webserver.h>
#include <websocket/websocket.h>
#include <wsproxy/wsproxy.h>
#include "localfs.h"

#ifdef _WIN32
#include <WinSock.h>
#endif

/* Windows/Visual Studio quirks */

#ifdef _WIN32
#pragma warning(disable:4996)
#define close _close
#define strdup _strdup
#else
#define closesocket close
#endif

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
	{ "help",	no_argument,		NULL, 'h' },
	/* { "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "killmask",	no_argument,		NULL, 'k' },
	{ "interface",  required_argument, 	NULL, 'i' },
	{ "closetest",  no_argument,		NULL, 'c' },*/
	{ NULL, 0, 0, 0 }
};

static int 
tcp_proxying(wsk_ctx_t *ctx, const char *location, void *userdata)
{
    wsv_url_parsing_t *par;
    const char *params;
    const char *name, *value;
    size_t nsize, vsize;
    char *host;
    unsigned port;
    int tsock = 0;
    struct sockaddr_in taddr;
	int err;
    
    params = strrchr(location, '?');
    if (!params) {
        LOG_ERR("No parameters specified with the TCP proxying command");
        goto fail; }
        
    par = wsv_begin_url_param_parsing(params);
    if (par == NULL) {
            LOG_ERR("Failed to start parsing URL parameters");
        goto fail; }
        
    while (wsv_parse_next_url_parameter(par, &name, &nsize, &value, &vsize) == WSVUP_OK) {
        LOG_DBG("Parameter: %.*s = \"%.*s\"", nsize, name, vsize, value);
        if (nsize == 4 && strncmp(name, "host", nsize) == 0) {
			host = (char*) malloc(vsize+1);
			strncpy(host, value, vsize+1);
            LOG_DBG("Got host parameter: \"%s\"", host);
        }
        else if (nsize == 4 && strncmp(name, "port", nsize) == 0) {
            port = atoi(value);
            LOG_DBG("Got port parameter: %u", port);
        }
        else {
            LOG_ERR("Unknown parameter for TCP proxying - full line: \"%s\"", params);
            wsv_done_url_param_parsing(par);
            goto fail; 
        }
    }
    wsv_done_url_param_parsing(par);
    LOG_DBG("TCP proxying command receiced, host = \"%s\", port = %u", host, port);
    
    tsock = socket(AF_INET, SOCK_STREAM, 0);
    if (tsock < 0) {
        LOG_ERR("Could not create target socket: %s", strerror(errno));
        goto fail; }
    memset((char *) &taddr, 0, sizeof(taddr));
    taddr.sin_family = AF_INET;
    taddr.sin_port = htons(port);
    
    /* Resolve target address */
    if (wsv_resolve_host(&taddr.sin_addr, host) < -1) {
        LOG_ERR("Could not resolve target address: %s\n", strerror(errno));
        goto fail; }
    
    if ((err = connect(tsock, (struct sockaddr *) &taddr, sizeof(taddr))) < 0) {
        LOG_ERR("Could not connect to target: code %d, error %s\n", err, wsv_describe_socket_error(0));
        goto fail; }
    
    wsp_do_proxy(ctx, tsock);
    
    return 0;
    
fail:
    if (tsock != 0) closesocket(tsock);
    return -1;
}

static int 
connection_handler(wsk_ctx_t *ctx, const char *location, void *userdata)
{
    const char *p;
    size_t n;
    char command[64], pname[32], encoded[1024], *q;
    char path[1024];
    
    printf("%s: location=%s\n", __FUNCTION__, location);

    // Extract command (we ignore the path here)
    p = strrchr(location, '/'); assert(p);
    p++; // skip the last slash
    for (q = command; *p && *p != '?'; p++, q ++) *q = *p; *q = '\0';
    LOG_DBG("Command: \"%s\"", command);
    
    if (strcmp(command, "$directory") == 0) {
        if (*p != '?') { LOG_ERR("Missing parameter introducer '?'"); goto fail; }
        p ++; // skip the parameter introducer
        // Now extract the parameters
        while (*p) {
            // Get the parameter name
            for (q = pname; *p && *p != '=' && *p != '&'; p++, q++) *q = *p; *q = '\0';
            // Now the parameter value
            if (*p != '=') { 
                LOG_ERR("Missing equality sign after parameter name \"%s\"", pname); 
                goto fail; }                
            for (p++, q = encoded; *p && *p != '&'; p++, q++) *q = *p; *q = '\0';
            // Parameter name decides where the decoded value will go
            if (strcmp(pname, "path") == 0) { q = path, n = sizeof(path); }
            else { LOG_ERR("Unknown parameter \"%s\"", pname); goto fail; }
            // Now decode the parameter value
            wsv_url_decode(encoded, p - encoded, q, n, 0);
            LOG_DBG("Parameter %s=\"%s\"", pname, q);
        }
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
	//	cert_path = key_path = NULL;

    wsv_start_server(&settings);

    return 0;
}
