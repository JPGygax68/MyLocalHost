#ifndef __URLENCODE_H
#define __URLENCODE_H

#include <stdint.h>

size_t url_decode(const char *src, size_t src_len, char *dst,
                  size_t dst_len, int is_form_url_encoded);

#endif // __URLENCODE_H