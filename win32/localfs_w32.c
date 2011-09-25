#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <Windows.h>
#include <direct.h>

#include "../localfs.h"

#define BUFFER_SIZE 2048

/* Windows/Visual Studio quirks */

#pragma warning(disable:4996)

#ifdef _WIN32
#define close _close
#define strdup _strdup
#endif

/* Private routines */

static size_t 
w32_get_error_string( char * buffer, size_t len )
{
    size_t size;
    size = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT), (LPTSTR)buffer, len, NULL);
    if (size == 0) {
        fprintf(stderr, "%s: FormatMessage() failed: %\n", __FUNCTION__, GetLastError());
    }
    return size;
}

static int 
read_drive_list(wsk_ctx_t *ctx, wsk_byte_t *buffer)
{
	DWORD bits = GetLogicalDrives();
	DWORD mask;
	char letter;
	int flag;
	int n, total;

	total = 0;
	//mg_write( conn, "[", 1 );

	for ( mask = 1, letter = 'A', flag = 0; mask != 0; mask <<= 1, letter ++ )
	{
		if ( (bits & mask) != 0 ) 
		{
			//if ( flag ) mg_write( conn, ",", 1 );
			n = sprintf((char*)buffer, "{ \"name\": \"%c\", \"size\": 0, \"isDirectory\": true }", letter );
			if (wsk_sendall(ctx, buffer, n) <= 0) return -1;
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
static int 
normalized_path_to_windows(const char * normalized, wchar_t *native)
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
		if ( *p && *p != '/' ) return 0; // illegal: drive letter must be followed by nothing or a slash
		if ( *p ) p ++;
		*q ++ = (wchar_t) toupper(drive);
		*q ++ = L':';
		*q ++ = L'\\';
	}

	// Now convert the rest to UTF-16
	(void) MultiByteToWideChar(CP_UTF8, 0, p, -1, q, FILENAME_MAX - (q - native));

	return 1;
}

static int 
list_folder_directory(wsk_ctx_t *ctx, const wchar_t * path, wsk_byte_t *buffer)
{
	// TODO: Un*x/Posix version
	wchar_t filter[FILENAME_MAX];
	size_t plen;
	wchar_t lastchar;
	WIN32_FIND_DATAW f;
	HANDLE h;
	unsigned n;

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
		if ( isalpha(path[0]) && plen >= 2 && plen <= 3 && path[1] == ':' )
		{
			n = sprintf((char*)buffer, "{ \"name\": \".\", \"size\": \"0\", \"isDirectory\": true }" );
			if (wsk_sendall(ctx, buffer, n) <= 0) {
                FindClose(h);
                return -1; }
			n = sprintf((char*)buffer, "{ \"name\": \"..\", \"size\": \"0\", \"isDirectory\": true }" );
            if (wsk_sendall(ctx, buffer, n) <= 0) {
                FindClose(h);
                return -1; }
            i = 2;
		}
		do {
			char filename[FILENAME_MAX];
			// Convert filename to UTF-8
			WideCharToMultiByte( CP_UTF8, 0, f.cFileName, -1, filename, FILENAME_MAX, NULL, NULL );
			// Now assemble an JSON line full of info
			n = sprintf((char*)buffer, "{ \"name\": \"%s\", \"size\": \"%llu\", \"isDirectory\": %s }", 
				filename, ((unsigned long long) ULONG_MAX + 1) * f.nFileSizeHigh + f.nFileSizeLow,
				(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? "true" : "false" );
			assert(n > 0);
			// Send that line
			if (wsk_sendall(ctx, buffer, n) <= 0) {
                FindClose(h);
                return -1; } 
			i ++;
		} while( FindNextFileW(h, &f) );
		FindClose(h);
        return 0; // success
	}
	else
	{
		char errbuf[1024+1], encoded[1024];
        size_t size;
		size = w32_get_error_string(errbuf, 1024);
        wsv_url_encode(errbuf, encoded, 1024);
        n = sprintf((char*)buffer, "{ \"error\": \"%s\" }", size > 0 ? encoded : "(no error info available)");
		(void) wsk_sendall(ctx, buffer, n);
        return -1; // error
	}
}

/* Public functions */

int 
list_directory(wsk_ctx_t *ctx, const char * parent_path)
{
	wchar_t pathbuf[FILENAME_MAX];
	wsk_byte_t *buffer;
	unsigned n;
    int retcode;

    retcode = -1; // failure

    buffer = wsk_alloc_block(ctx, 4096);

	if ( strcmp(parent_path, "/") == 0 )
	{
		retcode = read_drive_list(ctx, buffer);
	}
	else
	{
		if ( ! normalized_path_to_windows( parent_path, pathbuf ) )
		{
			n = sprintf((char*)buffer, "HTTP/1.0 400 Incorrect directory path \"%s\"\x0d\x0a"
				"Server: libwebsockets\x0d\x0a"
				"\x0d\x0a", parent_path);
			if (wsk_sendall(ctx, buffer, n) > 0)
			    retcode = 0;
		}
		else 
		{
			retcode = list_folder_directory(ctx, pathbuf, buffer);
		}
	}

    if (buffer) wsk_free_block(ctx, buffer);
    return retcode;
}
