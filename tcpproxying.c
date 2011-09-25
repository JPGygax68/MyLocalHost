#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
#include <WinSock.h>
#endif
#include <websocket/websocket.h>
#include <wsproxy/wsproxy.h>

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

/* Public routine implementations */

int 
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
