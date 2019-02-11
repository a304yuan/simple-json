#ifndef SIMPLE_JSON_H
#define SIMPLE_JSON_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "cstring.h"
#include "array.h"
#include "hash_table.h"
#include "any.h"

typedef struct json_base json_base;
typedef struct json_object json_object;
typedef struct json_array json_array;
typedef struct json_string json_string;
typedef struct json_number json_number;
typedef struct json_bool json_bool;

enum json_type {
    TYPE_JSON_OBJECT,
    TYPE_JSON_ARRAY,
    TYPE_JSON_STRING,
    TYPE_JSON_BOOL,
    TYPE_JSON_LONG,
    TYPE_JSON_DOUBLE
};

struct json_base {
    enum json_type type;
};

struct json_object {
    json_base base;
    hash_table * table;
};

struct json_array {
    json_base base;
    array * arr;
};

struct json_string {
    json_base base;
    string * str;
};

struct json_number {
    json_base base;
    any number;
};

struct json_bool {
    json_base base;
    bool boolean;
};

extern json_base * json_parse(const char * json_str);
extern void json_free(json_base * json);
extern json_base * json_object_get(const json_object * object, const char * key);
extern json_base * json_array_get(const json_array * array, size_t idx);

// extract value from json base element
static inline json_object * json_get_object(const json_base * json) {
    return (json_object*)json;
}
static inline json_array * json_get_array(const json_base * json) {
    return (json_array*)json;
}
static inline char * json_get_string(const json_base * json) {
    return string_get_raw(((json_string*)json)->str);
}
static inline long json_get_long(const json_base * json) {
    return any_get_long(&((json_number*)json)->number);
}
static inline double json_get_double(const json_base * json) {
    return any_get_double(&((json_number*)json)->number);
}
static inline bool json_get_bool(const json_base * json) {
    return ((json_bool*)json)->boolean;
}

#endif /* end of include guard: SIMPLE_JSON_H */
