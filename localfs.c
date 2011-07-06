/* Linux/Un*x implementation */

#include <sys/time.h>
#include <dirent.h>
#include <websocket/websocket.h>
#include "localfs.h"

static int
read_directory(wsk_ctx_t *ctx, const char *path)
{
    DIR *dir;
    struct dirent *de;
    char *msg;
    char encoded[256];
    int len;
    int err;

    err = -1;
    
    msg = (char*) wsk_alloc_block(ctx, 1024);
    if (!msg) goto fail;
    
    dir = opendir(path);
    if (dir == NULL) {
        len = snprintf(msg, 1024, "{ \"error\": \"Failed to open the directory\" }" );
        wsk_send(ctx, (wsk_byte_t*) msg, len);
        LOG_ERR("Failed to open the directory \"%s\"", path);
        goto fail; }
    
    while ((de = readdir(dir)) != NULL) {
        wsv_url_encode(de->d_name, encoded, 256);
        len = snprintf(msg, 1024, "{ \"name\": \"%s\", \"isDirectory\": \"%s\" }", 
                       encoded, de->d_type == DT_DIR ? "true" : "false" );
        wsk_send(ctx, (wsk_byte_t*) msg, len);
    }
    
    err = 0;
    
fail:
    if (dir) closedir(dir);
    if (msg) wsk_free_block(ctx, (wsk_byte_t*)msg);
    return err;
}
