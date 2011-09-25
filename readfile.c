#include <stdio.h>
#include <string.h>
#include <websocket/websocket.h>

#include "localfs.h"

/* Windows/Visual Studio quirks */

#ifdef _WIN32
#pragma warning(disable:4996)
#endif

int
read_file(wsk_ctx_t *ctx, const char *path)
{
	char native[FILENAME_MAX];
	wsk_byte_t *buffer;
	unsigned n;
    int retcode;

    retcode = -1; // failure

    buffer = wsk_alloc_block(ctx, 4096);

	if (!normalized_path_to_native(path, native))
	{
		n = sprintf((char*)buffer, "HTTP/1.0 400 Incorrect file path \"%s\"\x0d\x0a"
			"Server: libwebsockets\x0d\x0a"
			"\x0d\x0a", path);
		if (wsk_sendall(ctx, buffer, n) > 0)
			retcode = 0;
	}
	else 
	{
        FILE *fp;
        size_t n;

        fp = fopen(native, "rb");
        if (fp == NULL) {
            fprintf(stderr, "Failed to open file \"%s\"", native);
            return -1; }
        while ((n = fread(buffer, 1, 4096, fp)) > 0) {
            if (wsk_sendall(ctx, buffer, n) <= 0) {
                fprintf(stderr, "Error trying to send file content to client");
                break;
            }
        }
        fclose(fp);
	}

    if (buffer) wsk_free_block(ctx, buffer);
    return retcode;
}
