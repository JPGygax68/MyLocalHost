#ifdef _WIN32
#include <Windows.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "filesystem.h"

#ifdef _WIN32

// We get (or assume we get...) the normalized path as UTF-8 (URL-decoded).

// RETURNS THE NUMBER OF BYTES, *NOT* UTF-8 CHARACTER SEQUENCES!

size_t normalized_path_to_native( const char * normalized, char * native, size_t bufsize )
{
	size_t nsize;
	const char * p;
	char *q;
	size_t ntsize;

	p = normalized;
	nsize = strnlen( normalized, FILENAME_MAX );
	q = native;
	*q = '\0';
	ntsize = 0;

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
		*q ++ = ':';
		*q ++ = '\\';
		ntsize = 3;
	}

	for ( ; *p; p ++, q++, ntsize ++ ) 
	{
		if ( q >= (native+bufsize-1) )
			return 0;
		*q = *p;
	}
	*q = '\0';

	return ntsize;
}


#else // _WIN32

// TODO

#endif // _WIN32
