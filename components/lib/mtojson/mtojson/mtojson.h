/*
   SPDX-License-Identifier: BSD-2-Clause
   This file is Copyright (c) 2020, 2021 by Rene Kita
*/

#ifndef RKTA_MTOJSON_H
#define RKTA_MTOJSON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum json_to_type {
	t_to_primitive,
	t_to_array,
	t_to_boolean,
	t_to_hex,
	t_to_hex_u8,
	t_to_hex_u16,
	t_to_hex_u32,
	t_to_hex_u64,
	t_to_int,
	t_to_int8_t,
	t_to_int16_t,
	t_to_int32_t,
	t_to_int64_t,
	t_to_long,
	t_to_longlong,
	t_to_null,
	t_to_object,
	t_to_string,
	t_to_uint,
	t_to_uint8_t,
	t_to_uint16_t,
	t_to_uint32_t,
	t_to_uint64_t,
	t_to_ulong,
	t_to_ulonglong,
	t_to_value,
};

struct to_json {
	const char *name;
	const void *value;
	const size_t *count;     // Number of elements in a C array
	enum json_to_type stype; // Type of the struct
	enum json_to_type vtype; // Type of '.value'
};

/* Returns the length of the generated JSON text or 0 in case of an error. */
size_t json_generate(char *out, const struct to_json *tjs, size_t len);

#ifdef __cplusplus
}
#endif

#endif
