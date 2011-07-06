#ifndef __FILESYSTEM_H
#define __FILESYSTEM_H

int 
read_directory(wsk_ctx_t *ctx, const char * parent_path);

//size_t normalized_path_to_native( const char * normalized, char * native, size_t bufsize );

#endif // __FILESYSTEM_H
