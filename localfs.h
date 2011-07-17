#ifndef __LOCALFS_W32_H
#define __LOCALFS_W32_H

#include <websocket/websocket.h>

int 
list_directory(wsk_ctx_t *ctx, const char * parent_path);

#endif // __LOCALFS_W32_H
