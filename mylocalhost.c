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
#include <assert.h>

#include <webserver/webserver.h>
#include <websocket/websocket.h>
#include <wsproxy/wsproxy.h>

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
localfs_handler(wsk_ctx_t *ctx, const char *location, void *userdata)
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

    wsksvc = wsk_extend_webservice(&settings);
    if (!wsksvc) {
        fprintf(stderr, "Failed to extend web service with WebSocket protocol");
        return -1;
    }
    
    if (wsk_register_subprotocol(wsksvc, "localfs", localfs_handler, NULL)) {
        fprintf(stderr, "Failed to register \"localfs\" subprotocol handler\n" );
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
