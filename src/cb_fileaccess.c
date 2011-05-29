#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <libwebsockets.h>

#include "cb_fileaccess.h"

#ifdef WIN32

// TODO: w32_ necessary ?

static void w32_get_error_string( char * buffer, size_t len )
{
	FormatMessageA( 0, NULL, 0, 0, buffer, len, NULL );
}

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
	char buffer[LWS_SEND_BUFFER_PRE_PADDING+64+LWS_SEND_BUFFER_POST_PADDING], *bp;

	bp = buffer + LWS_SEND_BUFFER_PRE_PADDING;

	total = 0;
	//mg_write( conn, "[", 1 );

	for ( mask = 1, letter = 'A', flag = 0; mask != 0; mask <<= 1, letter ++ )
	{
		if ( (bits & mask) != 0 ) 
		{
			//if ( flag ) mg_write( conn, ",", 1 );
			n = sprintf_s( bp, 64, "{ \"name\": \"%c\", \"size\": 0, \"isDirectory\": true }", letter );
			// TODO: can this function decide to write less than specified length ?
			(void) libwebsocket_write( wsi, (unsigned char*) bp, n, LWS_WRITE_TEXT );
			total += n; 
			//flag = 1;
		}
	}

	//mg_write( conn, "]", 1 );

	return total;
}

/* We're assuming that we get the path in normalized form (one root), encoded in UTF-8. 
  We need it in native form (drive letters), and in UTF-16.
  Returns 0 if an error occurred ("incorrect normalized path").
  The length of the native buffer (number of wide characters) is assumed to be FILENAME_MAX.
 */
static int normalized_path_to_windows( const char * normalized, wchar_t * native )
{
	size_t nsize;
	const char * p;
	wchar_t *q;

	p = normalized;
	nsize = strnlen( normalized, FILENAME_MAX );
	q = native;
	*q = L'\0';

	// Absolute path ?
	if ( normalized[0] == '/' )
	{
		char drive;
		p ++;
		if ( ! *p ) return 1; // legal: an empty native path means we want the list of drives		
		if ( !isalpha(*p) ) return 0; // illegal: if there is a name after the root specifier, it must be a drive letter
		drive = *p ++;
		if ( ! *p ) return 0; // 
		if ( *p != '/' ) return 0; // illegal: drive letter must be followed by nothing or a slash
		p ++;
		*q ++ = (wchar_t) toupper(drive);
		*q ++ = L':';
		*q ++ = L'\\';
	}

	// Now convert the rest to UTF-16
	(void) MultiByteToWideChar( CP_UTF8, 0, p, -1, q, FILENAME_MAX - (q - native) );

	return 1;
}

static void read_folder_directory( struct libwebsocket *wsi, const wchar_t * path )
{
	// TODO: Un*x/Posix version
	wchar_t filter[FILENAME_MAX];
	size_t plen;
	wchar_t lastchar;
	WIN32_FIND_DATAW f;
	HANDLE h;
	char buffer[LWS_SEND_BUFFER_PRE_PADDING+1024+LWS_SEND_BUFFER_POST_PADDING], *bp;
	unsigned n;

	bp = buffer + LWS_SEND_BUFFER_PRE_PADDING;

	wcscpy_s( filter, FILENAME_MAX, path );
	plen = wcsnlen(path, FILENAME_MAX);
	lastchar = path[0] == L'\0' ? L'\0' : path[plen-1];
	if ( path[0] != L'\0' && lastchar != L'\\' && lastchar != L'/' ) wcscat_s( filter, FILENAME_MAX, L"/" );
	wcscat_s( filter, FILENAME_MAX, L"*" );

	h = FindFirstFileW( filter, &f );
	if(h != INVALID_HANDLE_VALUE)
	{
		unsigned i;
		i = 0;
		/*
		if ( isalpha(path[0]) && plen >= 2 && plen <= 3 && path[1] == ':' )
		{
			mg_printf( conn, "{ \"name\": \".\", \"size\": \"0\", \"isDirectory\": true }\n" );
			mg_printf( conn, ", { \"name\": \"..\", \"size\": \"0\", \"isDirectory\": true }\n" );
			i = 2;
		}
		*/
		do {
			char filename[FILENAME_MAX];
			// Convert filename to UTF-8
			WideCharToMultiByte( CP_UTF8, 0, f.cFileName, -1, filename, FILENAME_MAX, NULL, NULL );
			// Now assemble an JSON line full of info
			n = sprintf_s( bp, 1024, "{ \"name\": \"%s\", \"size\": \"%llu\", \"isDirectory\": %s }", 
				filename, 				
				((unsigned long long) ULONG_MAX + 1) * f.nFileSizeHigh + f.nFileSizeLow,
				(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? "true" : "false" );
			assert( n > 0 );
			// Send that line
			(void) libwebsocket_write(wsi, (unsigned char*) bp, n, LWS_WRITE_TEXT);
			i ++;
		} while( FindNextFileW(h, &f) );
		FindClose(h);
	}
	else
	{
		char errbuf[1024];
		w32_get_error_string( errbuf, 1024 );
		n = sprintf_s( bp, 1024, "HTTP/1.0 404 Could not open local directory: %s\x0d\x0a"
			"Server: libwebsockets\x0d\x0a"
			"\x0d\x0a", errbuf );
		(void) libwebsocket_write( wsi, (unsigned char *) bp, n, LWS_WRITE_HTTP );
	}
}

static int read_directory( struct libwebsocket *wsi, const char * parent_path )
{
	wchar_t pathbuf[FILENAME_MAX];
	char buffer[1024];
	unsigned n;

	if ( strcmp(parent_path, "/") == 0 )
	{
		return read_drive_list( wsi );
	}
	else
	{
		if ( ! normalized_path_to_windows( parent_path, pathbuf ) )
		{
			n = sprintf_s( buffer, 1024, "HTTP/1.0 400 Incorrect directory path \"%s\"\x0d\x0a"
				"Server: libwebsockets\x0d\x0a"
				"\x0d\x0a", parent_path );
			(void) libwebsocket_write( wsi, (unsigned char *) buffer, n, LWS_WRITE_HTTP );
			return 0;
		}
		else 
		{
			read_folder_directory( wsi, pathbuf );
			return 1; // success
		}
	}
}

#endif // WIN32

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
	struct lws_tokens *tokens;
	struct struct_fileaccess_callback *scb;

	switch (reason) {

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		fprintf( stderr, "callback_file_access(): FILTER_PROTOCOL_CONNECTION\n" );
		/* you could return non-zero here and kill the connection */
		break;

	/*
	case LWS_CALLBACK_HTTP:
		uri = in;
		fprintf( stderr, "callback_file_access(): HTTP: uri = %s\n", uri );
		break;
	*/

	case LWS_CALLBACK_ESTABLISHED:
		//pss->number = 0;
		fprintf( stderr, "callback_file_access(): ESTABLISHED\n" );
		scb = user;
		uri = lws_gettoken( wsi, WSI_TOKEN_GET_URI );
		strcpy_s( scb->path, sizeof(scb->path), uri );
		libwebsocket_callback_on_writable( context, wsi );
		break;

	case LWS_CALLBACK_RECEIVE:
		//pss->number = 0;
		fprintf( stderr, "callback_file_access(): RECEIVE\n" ); // TODO: NOT BEGIN CALLED
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		scb = user;
		read_directory( wsi, scb->path );
		if ( 0 /* TODO */ ) {
			fprintf(stderr, "EOF reached, closing\n");
			libwebsocket_close_and_free_session(context, wsi, LWS_CLOSE_STATUS_NORMAL);
		}
		break;

	default:
		break;
	}

	return 0;
}

