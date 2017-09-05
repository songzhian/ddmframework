#ifndef __STR_ARRAY_H__
#define __STR_ARRAY_H__

#include "utarray.h"

#include <sys/types.h> //size_t
#include <stdint.h>  //uint32_t
#include <sys/time.h>  //timeval

#ifdef __cplusplus
extern "C" {
#endif

/**
 * array of string
 */
typedef struct _string_array
{
	UT_array *strs;
	uint32_t counter;
} string_array;


int string_array_init(string_array * array);

int string_array_destroy(string_array * array);

int string_array_find(string_array * array, const char * value);

int string_array_add(string_array * array, const char * value);

int string_array_delete(string_array * array, const char * value);

const char * string_array_next(string_array * array);

typedef void walk_through_func_type(const char * value);

void string_array_walk(string_array * array, walk_through_func_type * walk_through_func);

int string_array_content(string_array * array, char * buffer, size_t buf_len);

int string_array_size(string_array * array);

#ifdef __cplusplus
}
#endif

#endif