#ifndef __LOCALFS_H
#define __LOCALFS_H

#include <websocket/websocket.h>

/* Public routines */

int 
read_directory(wsk_ctx_t *ctx, const char * parent_path);

int
read_file(wsk_ctx_t *ctx, const char *path);

/* Used internally */

int 
normalized_path_to_native(const char * normalized, char *native);

#endif // __LOCALFS_H
