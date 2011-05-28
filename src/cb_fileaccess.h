#ifndef __CB_FILEACCESS_H
#define __CB_FILEACCESS_H

struct struct_fileaccess_callback {
	char path[FILENAME_MAX];
};

int
callback_file_access( 
	struct libwebsocket_context * context,
	struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason,
	void *user, 
	void *in, 
	size_t len
	);

#endif // __CB_FILEACCESS_H
