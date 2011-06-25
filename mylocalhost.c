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
#include <sys/time.h>

#include <webserver/webserver.h>
#include <websocket/websocket.h>

//#include "urlencode.h"
//#include "filesystem.h"
//#include "cb_fileaccess.h"

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	/* { "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "killmask",	no_argument,		NULL, 'k' },
	{ "interface",  required_argument, 	NULL, 'i' },
	{ "closetest",  no_argument,		NULL, 'c' },*/
	{ NULL, 0, 0, 0 }
};

#ifdef NOT_DEFINED
static void my_connection_handler(ws_ctx_t *ctx, ws_listener_t *settings)
{
    int tsock = 0;
    struct sockaddr_in taddr;
    const char *uri;
    
    // Extract target host and port from URI
    uri = ws_get_location(ctx);
    
    // Connect to target socket
    fprintf(stderr, "connecting to: %s:%d, location=\"%s\"", target->host, target->port, ws_get_location(ctx));
    tsock = socket(AF_INET, SOCK_STREAM, 0);
    if (tsock < 0) {
        LOG_ERR("Could not create target socket: %s", strerror(errno));
        return;
    }
    memset((char *) &taddr, 0, sizeof(taddr));
    taddr.sin_family = AF_INET;
    taddr.sin_port = htons(target->port);
    
    /* Resolve target address */
    if (ws_resolve_host(&taddr.sin_addr, target->host) < -1) {
        LOG_ERR("Could not resolve target address: %s\n", strerror(errno));
    }
    
    if (connect(tsock, (struct sockaddr *) &taddr, sizeof(taddr)) < 0) {
        LOG_ERR("Could not connect to target: %s\n", strerror(errno));
        close(tsock);
        return;
    }
    
    wsp_do_proxy(ctx, tsock);
    
    closesocket(tsock);
    
}
#endif

static void request_handler(wsv_ctx_t *ctx, const char *header, wsv_settings_t *settings)
{
    // TODO
}

int main(int argc, char **argv)
{
    int n = 0;
    //const char *cert_path = "mylocalhost.pem";
    //const char *key_path = "mylocalhost.key.pem";
    //int use_ssl = 0;
    //ws_listener_t listener;
    //wsp_target_t target;
    wsv_settings_t settings;
    int opts = 0;

	fprintf(stderr, "MyLocalHost server\n"
			"(C) Copyright 2011 Jean-Pierre Gygax <gygax@practicomp.ch>\n" );

    settings.certfile = NULL;
    settings.keyfile = NULL;
    strcpy(settings.listen_host, "localhost");
    settings.listen_port = 7681; // TODO: symbolic constant
    settings.ssl_only = 0;
    settings.handler = NULL; // use the default handler
    settings.userdata = NULL;
    
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
