#include "str_array.h"
#include "log.h"
#include "utstring.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>


/******************************string_array***********************************/

int string_array_init(string_array * array) {
	utarray_new(array->strs, &ut_str_icd);
	array->counter = 0;
	return 0;
}

int string_array_destroy(string_array * array) {
	utarray_free(array->strs);
	return 0;
}

int string_array_find(string_array * array, const char * value) {
	char ** p = NULL;
	while ( (p = (char**)utarray_next(array->strs, p))) {
		int ret = strcmp(*p, value);
		if (ret == 0) {
			return utarray_eltidx(array->strs, p);
		}
	}
	return -1;
}

int string_array_add(string_array * array, const char * value) {
	int idx = string_array_find(array, value);
	if (idx != -1) {
		return 0;
	}
	utarray_push_back(array->strs, &value);
	return 0;
}

int string_array_delete(string_array * array, const char * value) {
	int idx = string_array_find(array, value);
	if (idx == -1) {
		return -1;
	}
	utarray_erase(array->strs, idx, 1);
	return 0;
}

const char * string_array_next(string_array * array) {
	int count = utarray_len(array->strs);
	if (count == 0) {
		return NULL;
	}
	int idx = (array->counter ++) % count;
	char ** p = utarray_eltptr(array->strs, idx);
	return *p;
}

void string_array_walk(string_array * array, walk_through_func_type * walk_through_func) {
	char ** p = NULL;
	while ( (p = (char**)utarray_next(array->strs, p))) {
		if (walk_through_func) {
			(*walk_through_func)(*p);
		}
	}
}

int string_array_content(string_array * array, char * buffer, size_t buf_len) {
	UT_string *s;
	utstring_new(s);
	utstring_printf(s, "counter=%d,size=%d,", array->counter, utarray_len(array->strs));
	char ** p = NULL;
	while ( (p = (char**)utarray_next(array->strs, p))) {
		utstring_printf(s, "%s, ", *p);
	}
	snprintf(buffer, buf_len, "%s", utstring_body(s));
	utstring_free(s);
	return 0;
}

int string_array_size(string_array * array) {
	return utarray_len(array->strs);
}
