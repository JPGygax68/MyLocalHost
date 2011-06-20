/*
 * mylocalhost
 *
 * Copyright (C) 20111 Jean-Pierre Gygax <gygax@practicomp.ch>
 *
 * Adapted from Andy "Warmcat" Greene's libwebsockets test server.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>

#include <websocket/websocket.h>

#include "urlencode.h"
#include "filesystem.h"
#include "cb_fileaccess.h"

static int serve_http(ws_ctx_t *ctx, const char *uri)
{
	char client_name[128];
	char client_ip[128];
	char normpath[FILENAME_MAX];
	size_t nplen;
	char nativepath[FILENAME_MAX];
	size_t ntplen;

    fprintf(stderr, "serving HTTP URI %s\n", uri);

    if (strcmp(in, "favicon.ico") == 0) {
        if (ws_serve_http_file(wsi, "favicon.ico", "image/x-icon"))
            fprintf(stderr, "Failed to send favicon\n");
    }

    // Decode the (normalized) path (ignoring URL parameters for now)
    nplen = url_decode((const char*)in + 1, -1, normpath, FILENAME_MAX, 0);

    // Now convert the path to native
    ntplen = normalized_path_to_native( normpath, nativepath, FILENAME_MAX );

    /* Send the file  */
    // TODO: check for existence, send 404 if not found
    if ( ws_serve_http_file(wsi, nativepath, "text/html") )
        fprintf(stderr, "Failed to send HTTP file\n");

	return 0;
}

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	/* { "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "killmask",	no_argument,		NULL, 'k' },
	{ "interface",  required_argument, 	NULL, 'i' },
	{ "closetest",  no_argument,		NULL, 'c' },*/
	{ NULL, 0, 0, 0 }
};

int main(int argc, char **argv)
{
    int n = 0;
    //const char *cert_path = "mylocalhost.pem";
    //const char *key_path = "mylocalhost.key.pem";
    //int use_ssl = 0;
    ws_listener_t listener;
    int opts = 0;

	fprintf(stderr, "MyLocalHost server\n"
			"(C) Copyright 2011 Jean-Pierre Gygax <gygax@practicomp.ch>\n" );

    listener.certfile = NULL;
    listener.keyfile = NULL;
    listener.listen_host = "localhost";
    listener.listen_port = 7681; // TODO: symbolic constant
    listener.ssl_only = 0;
    listener.userdata = NULL;
    
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
			port = atoi(optarg);
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

	ws_start_server(listener);

    return 0;
}
