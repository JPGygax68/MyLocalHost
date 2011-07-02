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
#include <sys/time.h>

#include <webserver/webserver.h>
#include <websocket/websocket.h>
#include <wsproxy/wsproxy.h>

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	/* { "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "killmask",	no_argument,		NULL, 'k' },
	{ "interface",  required_argument, 	NULL, 'i' },
	{ "closetest",  no_argument,		NULL, 'c' },*/
	{ NULL, 0, 0, 0 }
};

static int my_connection_handler(wsv_ctx_t *wsvctx, char *header, void *userdata)
{
    wsk_ctx_t *ctx;
    int tsock = 0;
    struct sockaddr_in taddr;
    char uri[512];

    fprintf(stderr, "%s\n", __FUNCTION__); // TODO: real logging
    
    // Upgrade the connection
    ctx = wsk_handshake(wsvctx, 0); // TODO: handle SSL
    if (!ctx) {
        fprintf(stderr, "Failed to upgrade the connection\n");
        return -1;
    }
    printf("Upgraded the connection successfully\n");

    // Look at the URI
    wsv_extract_url(header, uri);
    printf("URI=\"%s\"\n", uri);
    
    // Resolve target host and port
    memset((char *) &taddr, 0, sizeof(taddr));
    //taddr.sin_family = AF_INET;
    if (wsv_resolve_host(&taddr.sin_addr, wsv_extract_url(header, uri)) < -1) {
        fprintf(stderr, "Could not resolve target address \"%s\"\n", uri);
        return -1;
    }
    
    // Create and connect to target socket
    tsock = socket(AF_INET, SOCK_STREAM, 0);
    if (tsock < 0) {
        fprintf(stderr, "Could not create target socket: %s", strerror(errno));
        return -1;
    }    
    if (connect(tsock, (struct sockaddr *) &taddr, sizeof(taddr)) < 0) {
        fprintf(stderr, "Could not connect to target: %s\n", strerror(errno));
        close(tsock);
        return -1;
    }
    
    // TODO: free the context
    
    return 0;
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

    //memset(&settings, sizeof(settings), 0);
    settings.certfile = NULL;
    settings.keyfile = NULL;
    strcpy(settings.listen_host, "localhost");
    settings.listen_port = 7681; // TODO: symbolic constant
    settings.ssl_policy = wsv_allow_ssl;
    settings.handler = NULL; // use the default handler
    settings.protocols = NULL;
    settings.userdata = NULL;
    
    wsv_register_protocol(&settings, "WebSocket", my_connection_handler);
    
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
