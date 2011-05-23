#include <stdio.h>
#include <stdlib.h>
#include <libwebsockets.h>

#include "cb_fileaccess.h"

/* TODO: in theory, this needs to be implemented in a way that can stretch across several
  LWS_CALLBACK_SERVER_WRITEABLE back-calls.
  */
static int read_drive_list( struct libwebsocket *wsi )
{
	DWORD bits = GetLogicalDrives();
	DWORD mask;
	char letter;
	int flag;
	int n, total;
	char buffer[LWS_SEND_BUFFER_PRE_PADDING+3+LWS_SEND_BUFFER_POST_PADDING], *bp;

	bp = buffer + LWS_SEND_BUFFER_PRE_PADDING;

	total = 0;
	//mg_write( conn, "[", 1 );

	for ( mask = 1, letter = 'A', flag = 0; mask != 0; mask <<= 1, letter ++ )
	{
		if ( (bits & mask) != 0 ) 
		{
			//if ( flag ) mg_write( conn, ",", 1 );
			// mg_printf( conn, "{ \"name\": \"%c\", \"size\": 0, \"isDirectory\": true }", letter );
			sprintf_s( bp, 3, "%c\n", letter );
			// TODO: can this function decide to write less than specified length ?
			n = libwebsocket_write(wsi, (unsigned char*) bp, strlen(bp), LWS_WRITE_TEXT);
			total += n; 
			//flag = 1;
		}
	}

	//mg_write( conn, "]", 1 );

	return total;
}

static int read_directory( struct libwebsocket *wsi, const char * parent_path )
{
	// TODO: real implementation
	return read_drive_list( wsi );
}

int
callback_file_access( 
	struct libwebsocket_context * context,
	struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason,
	void *user, 
	void *in, 
	size_t len
	)
{
	const char * uri;

	// struct per_session_data__fileaccess *psd = user;

	switch (reason) {

	case LWS_CALLBACK_HTTP:
		uri = in;
		fprintf( stderr, "callback_file_access(): HTTP: uri = %s\n", uri ); // TODO: NOT BEING CALLED
		break;

	case LWS_CALLBACK_ESTABLISHED:
		//pss->number = 0;
		fprintf( stderr, "callback_file_access(): ESTABLISHED\n" );
		libwebsocket_callback_on_writable( context, wsi );
		break;

	case LWS_CALLBACK_RECEIVE:
		//pss->number = 0;
		fprintf( stderr, "callback_file_access(): RECEIVE\n" ); // TODO: NOT BEGIN CALLED
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		read_directory( wsi, "/" ); // TODO
		if ( 0 /* TODO */ ) {
			fprintf(stderr, "EOF reached, closing\n");
			libwebsocket_close_and_free_session(context, wsi, LWS_CLOSE_STATUS_NORMAL);
		}
		break;

	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */
	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		//dump_handshake_info((struct lws_tokens *)(long)user);
		/* you could return non-zero here and kill the connection */
		break;

	default:
		break;
	}

	return 0;
}

