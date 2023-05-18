/*
   SPDX-License-Identifier: BSD-2-Clause
   This file is Copyright (c) 2020, 2021 by Rene Kita
*/

#include "mtojson.h"

#include <stdint.h>
#include <string.h>

static char* gen_array(char *, const void *);
static char* gen_boolean(char *, const void *);
static char* gen_c_array(char *, const void *);
static char* gen_hex(char *, const void *);
static char* gen_hex_u8(char *, const void *);
static char* gen_hex_u16(char *, const void *);
static char* gen_hex_u32(char *, const void *);
static char* gen_hex_u64(char *, const void *);
static char* gen_int(char *, const void *);
static char* gen_int8_t(char *, const void *);
static char* gen_int16_t(char *, const void *);
static char* gen_int32_t(char *, const void *);
static char* gen_int64_t(char *, const void *);
static char* gen_long(char *, const void *);
static char* gen_longlong(char *, const void *);
static char* gen_null(char *, const void *);
static char* gen_object(char *, const void *);
static char* gen_primitive(char *, const void *);
static char* gen_string(char *, const void *);
static char* gen_uint(char *, const void *);
static char* gen_uint8_t(char *, const void *);
static char* gen_uint16_t(char *, const void *);
static char* gen_uint32_t(char *, const void *);
static char* gen_uint64_t(char *, const void *);
static char* gen_ulong(char *, const void *);
static char* gen_ulonglong(char *, const void *);
static char* gen_value(char *, const void *);

static char* (* const gen_functions[])(char *, const void *) = {
	gen_primitive,
	gen_array,
	gen_boolean,
	gen_hex,
	gen_hex_u8,
	gen_hex_u16,
	gen_hex_u32,
	gen_hex_u64,
	gen_int,
	gen_int8_t,
	gen_int16_t,
	gen_int32_t,
	gen_int64_t,
	gen_long,
	gen_longlong,
	gen_null,
	gen_object,
	gen_string,
	gen_uint,
	gen_uint8_t,
	gen_uint16_t,
	gen_uint32_t,
	gen_uint64_t,
	gen_ulong,
	gen_ulonglong,
	gen_value,
};

static size_t remaining_length;

static int
reduce_rem_len(size_t len)
{
	if (remaining_length < len)
		return 0;
	remaining_length -= len;
	return 1;
}

static char*
strcpy_val(char *out, const char *val, size_t len)
{
	if (!reduce_rem_len(len))
		return NULL;
	memcpy(out, val, len);
	return out + len;
}

static char*
gen_null(char *out, const void *val)
{
	(void)val;
	return strcpy_val(out, "null", 4);
}

static char*
gen_boolean(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	if (*(const _Bool*)val)
		return strcpy_val(out, "true", 4);
	else
		return strcpy_val(out, "false", 5);
}

static char*
gen_string(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	if (!reduce_rem_len(2)) // 2 -> ""
		return NULL;

	char chars_to_escape[] = "\"\\";
	const char *begin = (const char*)val;
	const char *end = begin + strlen(begin);

	*out++ = '"';
	char *esc;
	do {
		esc = NULL;
		for (int i = 0; i < 2; i++) {
			char *tmp;
			if ((tmp = strchr(begin, chars_to_escape[i]))) {
				if (!esc)
					esc = tmp;
				else
					esc = esc < tmp ? esc : tmp;
			}
		}

		size_t len = esc ? (size_t)(esc - begin) : (size_t)(end - begin);

		if (!(out = strcpy_val(out, begin, len)))
			return NULL;

		if (esc) {
			char s[2];
			s[0] = '\\';
			s[1] = *esc;
			if (!(out = strcpy_val(out, s, 2)))
				return NULL;
			begin = esc + 1;
		}

	} while (esc && *begin);

	*out++ = '"';
	return out;
}

static char*
mtojson_utoa(char *dst, unsigned n, unsigned base)
{
	char *s = dst;
	char *e;

	for (unsigned m = n; m >= base;  m /= base)
		s++;
	e = s + 1;

	size_t len = (size_t)(e - dst);
	if (!reduce_rem_len(len))
		return NULL;

	for ( ; s >= dst; s--, n /= base)
		*s = "0123456789ABCDEF"[n % base];
	return e;
}

static char*
mtojson_ultoa(char *dst, unsigned long n, unsigned base)
{
	char *s = dst;
	char *e;

	for (unsigned long m = n; m >= base;  m /= base)
		s++;
	e = s + 1;

	size_t len = (size_t)(e - dst);
	if (!reduce_rem_len(len))
		return NULL;

	for ( ; s >= dst; s--, n /= base)
		*s = "0123456789ABCDEF"[n % base];
	return e;
}

static char*
mtojson_ulltoa(char *dst, unsigned long long n, unsigned base)
{
	char *s = dst;
	char *e;

	for (unsigned long long m = n; m >= base;  m /= base)
		s++;
	e = s + 1;

	size_t len = (size_t)(e - dst);
	if (!reduce_rem_len(len))
		return NULL;

	for ( ; s >= dst; s--, n /= base)
		*s = "0123456789ABCDEF"[n % base];
	return e;
}

static char*
gen_hex(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	if (!reduce_rem_len(2)) // 2 -> ""
		return NULL;

	*out++ = '"';
	if (!(out =  mtojson_utoa(out, *(const unsigned*)val, 16)))
		return NULL;
	*out++ = '"';

	return out;
}

static char*
gen_hex_u8(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	if (!reduce_rem_len(2)) // 2 -> ""
		return NULL;

	*out++ = '"';
	if (!(out =  mtojson_utoa(out, *(const uint8_t*)val, 16)))
		return NULL;
	*out++ = '"';

	return out;
}

static char*
gen_hex_u16(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	if (!reduce_rem_len(2)) // 2 -> ""
		return NULL;

	*out++ = '"';
	if (!(out =  mtojson_utoa(out, *(const uint16_t*)val, 16)))
		return NULL;
	*out++ = '"';

	return out;
}

static char*
gen_hex_u32(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	if (!reduce_rem_len(2)) // 2 -> ""
		return NULL;

	*out++ = '"';
	if (!(out =  mtojson_ultoa(out, *(const uint32_t*)val, 16)))
		return NULL;
	*out++ = '"';

	return out;
}

static char*
gen_hex_u64(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	if (!reduce_rem_len(2)) // 2 -> ""
		return NULL;

	*out++ = '"';
	if (!(out =  mtojson_ulltoa(out, *(const uint64_t*)val, 16)))
		return NULL;
	*out++ = '"';

	return out;
}

static char*
gen_int(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	int n = *(const int*)val;
	unsigned u = (unsigned)n;
	if (n < 0){
		if (!reduce_rem_len(1))
			return NULL;
		*out++ = '-';
		u = -(unsigned)n;
	}

	return mtojson_utoa(out, u, 10);
}

static char*
gen_int8_t(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	int n = *(const int8_t*)val;
	unsigned u = (unsigned)n;
	if (n < 0){
		if (!reduce_rem_len(1))
			return NULL;
		*out++ = '-';
		u = -(unsigned)n;
	}

	return mtojson_utoa(out, u, 10);
}

static char*
gen_int16_t(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	int n = *(const int16_t*)val;
	unsigned u = (unsigned)n;
	if (n < 0){
		if (!reduce_rem_len(1))
			return NULL;
		*out++ = '-';
		u = -(unsigned)n;
	}

	return mtojson_utoa(out, u, 10);
}

static char*
gen_int32_t(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	int n = *(const int*)val;
	unsigned u = (unsigned)n;
	if (n < 0){
		if (!reduce_rem_len(1))
			return NULL;
		*out++ = '-';
		u = -(unsigned)n;
	}

	return mtojson_utoa(out, u, 10);
}

static char*
gen_int64_t(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	int64_t n = *(const int64_t*)val;
	uint64_t u = (uint64_t)n;
	if (n < 0){
		if (!reduce_rem_len(1))
			return NULL;
		*out++ = '-';
		u = -(uint64_t)n;
	}

	return mtojson_ulltoa(out, u, 10);
}

static char*
gen_uint(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	return mtojson_utoa(out, *(const unsigned*)val, 10);
}

static char*
gen_uint8_t(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	return mtojson_utoa(out, *(const uint8_t*)val, 10);
}

static char*
gen_uint16_t(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	return mtojson_utoa(out, *(const uint16_t*)val, 10);
}

static char*
gen_uint32_t(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	return mtojson_ultoa(out, *(const uint32_t*)val, 10);
}

static char*
gen_uint64_t(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	return mtojson_ulltoa(out, *(const uint64_t*)val, 10);
}

static char*
gen_long(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	long n = *(const long*)val;
	unsigned long u = (unsigned long)n;
	if (n < 0){
		if (!reduce_rem_len(1))
			return NULL;
		*out++ = '-';
		u = -(unsigned long)n;
	}

	return mtojson_ultoa(out, u, 10);
}

static char*
gen_longlong(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	long long n = *(const long long*)val;
	unsigned long long u = (unsigned long long)n;
	if (n < 0){
		if (!reduce_rem_len(1))
			return NULL;
		*out++ = '-';
		u = -(unsigned long long)n;
	}

	return mtojson_ulltoa(out, u, 10);
}


static char*
gen_ulong(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	return mtojson_ultoa(out, *(const unsigned long*)val, 10);
}

static char*
gen_ulonglong(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	return mtojson_ulltoa(out, *(const unsigned long long*)val, 10);
}

static char*
gen_value(char *out, const void *val)
{
	return strcpy_val(out, (const char*)val, strlen((const char*)val));
}

static char*
gen_c_array(char *out, const void *val)
{
	const struct to_json *tjs = (const struct to_json*)val;
	if (!reduce_rem_len(2)) // 2 -> []
		return NULL;

	*out++ = '[';
	if (*tjs->count == 0){
		*out++ = ']';
		return out;
	}

	size_t incr = 0;
	char* (*func)(char *, const void *) = gen_functions[tjs->vtype];
	switch (tjs->vtype) {
	case t_to_boolean:
		incr = sizeof(_Bool);
		break;

	case t_to_int:
		incr = sizeof(int);
		break;

	case t_to_int8_t:
		incr = sizeof(int8_t);
		break;

	case t_to_int16_t:
		incr = sizeof(int16_t);
		break;

	case t_to_int32_t:
		incr = sizeof(int32_t);
		break;

	case t_to_int64_t:
		incr = sizeof(int64_t);
		break;

	case t_to_long:
		incr = sizeof(unsigned long);
		break;

	case t_to_longlong:
		incr = sizeof(unsigned long long);
		break;

	case t_to_object:
		incr = sizeof(struct to_json);
		break;

	case t_to_hex:
	case t_to_uint:
		incr = sizeof(unsigned);
		break;

	case t_to_hex_u8:
	case t_to_uint8_t:
		incr = sizeof(uint8_t);
		break;

	case t_to_hex_u16:
	case t_to_uint16_t:
		incr = sizeof(uint16_t);
		break;

	case t_to_hex_u32:
	case t_to_uint32_t:
		incr = sizeof(uint32_t);
		break;

	case t_to_hex_u64:
	case t_to_uint64_t:
		incr = sizeof(uint64_t);
		break;

	case t_to_ulong:
		incr = sizeof(unsigned long);
		break;

	case t_to_ulonglong:
		incr = sizeof(unsigned long long);
		break;

	case t_to_array:
	case t_to_null:
	case t_to_primitive:
	case t_to_string:
	case t_to_value:
		return NULL;
	}

	const char *p = tjs->value;
	for (size_t i = 0; i < *tjs->count - 1; i++){
		if (!(out = (*func)(out, p)))
			return NULL;
		if (!reduce_rem_len(1))
			return NULL;
		*out++ = ',';

		p += incr;
	}

	if (!(out = (*func)(out, p)))
		return NULL;

	*out++ = ']';
	return out;
}

static char*
gen_array(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	const struct to_json *tjs = (const struct to_json*)val;
	if (!reduce_rem_len(2)) // 2 -> []
		return NULL;

	*out++ = '[';
	while (tjs->value){
		if (tjs->count)
			out = gen_c_array(out, tjs);
		else
			out = gen_functions[tjs->vtype](out, tjs->value);
		if (!out)
			return NULL;
		tjs++;
		if (tjs->value){
			if (!reduce_rem_len(1))
				return NULL;
			*out++ = ',';
		}
	}
	*out++ = ']';
	return out;
}

static char*
gen_object(char *out, const void *val)
{
	if (!val)
		return gen_null(out, val);

	const struct to_json *tjs = (const struct to_json*)val;

	if (!reduce_rem_len(2)) // 2 -> {}
		return NULL;

	*out++ = '{';
	while (tjs->name){
		const char *name = tjs->name;
		size_t len = strlen(name);
		if (!reduce_rem_len(len + 3)) // 3 -> "":
			return NULL;

		*out++ = '"';
		memcpy(out, name, len);
		out += len;
		*out++ = '"';
		*out++ = ':';

		if (tjs->count)
			out =  gen_c_array(out, tjs);
		else
			out =  gen_functions[tjs->vtype](out, tjs->value);

		if (!out)
			return NULL;

		tjs++;
		if (tjs->name){
			if (!reduce_rem_len(1))
				return NULL;
			*out++ = ',';
		}
	}

	*out++ = '}';
	return out;
}

static char*
gen_primitive(char *out, const void *to_json)
{
	const struct to_json *tjs = (const struct to_json *)to_json;
	if (tjs->count)
		return gen_c_array(out, tjs);

	return gen_functions[tjs->vtype](out, tjs->value);
}

size_t
json_generate(char *out, const struct to_json *tjs, size_t len)
{
	const char *start = out;

	remaining_length = len;
	if (!reduce_rem_len(1)) // \0
		return 0;

	switch (tjs->stype) {
	case t_to_array:
		out = gen_array(out, tjs);
		break;
	case t_to_object:
		out = gen_object(out, tjs);
		break;
	case t_to_primitive:
		out = gen_primitive(out, tjs);
		break;
	/* These are not valid ctypes */
	case t_to_boolean:
	case t_to_int:
	case t_to_null:
	case t_to_string:
	case t_to_uint:
	case t_to_value:
	default:
		return 0;
	}

	if (!out)
		return 0;

	*out = '\0';
	return (size_t)(out - start);
}
