#include "json.h"

static json_array * json_parse_array(const char * s, char ** endptr);
static json_object * json_parse_object(const char * s, char ** endptr);

static size_t bkdr_hash(const any * key) {
    size_t h = 0;
    string * str = any_get_pointer(key);
    unsigned char * k = (unsigned char *)string_get_raw(str);
    unsigned char * end = k + string_len(str);

    while (k < end) {
        h = h * 131 + *k;
        k++;
    }

    return h;
}

static int json_key_cmp(const any * a, const any * b) {
    string * str1 = any_get_pointer(a);
    string * str2 = any_get_pointer(b);
    return strcmp(string_get_raw(str1), string_get_raw(str2));
}

static json_string * json_parse_string(const char * s, char ** endptr) {
    json_string * jstr = malloc(sizeof(json_string));
    jstr->base.type = TYPE_JSON_STRING;
    char * ptr = (char*)s;
    while (true) {
        if (*ptr == '"') {
            if (*(ptr - 1) != '\\') {
                break;
            }
        }
        else {
            ptr++;
        }
    }
    *endptr = ptr + 1;
    jstr->str = string_new_s(s, ptr - s);
    return jstr;
}

static json_number * json_parse_number(const char * s, char ** endptr) {
    json_number * jnum = malloc(sizeof(json_number));
    long l = strtol(s, endptr, 10);
    if (**endptr == '.') {
        double d = strtod(s, endptr);
        any_set(&jnum->number, d);
        jnum->base.type = TYPE_JSON_DOUBLE;
    }
    else {
        any_set(&jnum->number, l);
        jnum->base.type = TYPE_JSON_LONG;
    }
    return jnum;
}

static json_bool * json_parse_bool(const char * s, char ** endptr) {
    json_bool * jbool = malloc(sizeof(json_bool));
    jbool->base.type = TYPE_JSON_BOOL;
    if (*s == 't') {
        jbool->boolean = true;
        *endptr = (char*)s + 4;
    }
    else {
        jbool->boolean = false;
        *endptr = (char*)s + 5;
    }
    return jbool;
}

static void * json_parse_null(const char * s, char ** endptr) {
    *endptr = (char *)s + 3;
    return NULL;
}

static json_object * json_parse_object(const char * s, char ** endptr) {
    json_object * jobj = malloc(sizeof(json_object));
    hash_table * htable = hash_table_new(16, bkdr_hash, json_key_cmp);
    jobj->base.type = TYPE_JSON_OBJECT;
    jobj->table = htable;

    char * ptr = (char*)s;
    while (true) {
        char * key_start = strchr(ptr, '"') + 1;
        char * val_start;
        json_string * key_str = json_parse_string(key_start, &val_start);
        any key = ANY_POINTER(key_str->str);
        free(key_str);

        while (*val_start == ':' || isspace(*val_start)) {
            val_start++;
        }
        void * val;
        switch (*val_start) {
            case '{': val = json_parse_object(val_start + 1, &ptr); break;
            case '[': val = json_parse_array(val_start + 1, &ptr); break;
            case '"': val = json_parse_string(val_start + 1, &ptr); break;
            case 't':
            case 'f': val = json_parse_bool(val_start, &ptr); break;
            case 'n': val = json_parse_null(val_start, &ptr); break;
            default: val = json_parse_number(val_start, &ptr); break;
        }
        hash_table_insert(htable, key, ANY_POINTER(val));
        // skip spaces
        while (isspace(*ptr)) {
            ptr++;
        }
        // judge if is the end of object
        if (*ptr == '}') {
            break;
        }
        else {
            ptr++;
        }
    }
    *endptr = ptr + 1;

    return jobj;
}

static json_array * json_parse_array(const char * s, char ** endptr) {
    array * arr = array_new(16);
    json_array * jarr = malloc(sizeof(json_array));
    jarr->base.type = TYPE_JSON_ARRAY;
    jarr->arr = arr;

    char * ptr = (char*)s;
    while (true) {
        char * val_start = ptr;
        void * val;
        while (isspace(*val_start)) {
            val_start++;
        }
        switch (*val_start) {
            case '{': val = json_parse_object(val_start + 1, &ptr); break;
            case '[': val = json_parse_array(val_start + 1, &ptr); break;
            case '"': val = json_parse_string(val_start + 1, &ptr); break;
            case 't':
            case 'f': val = json_parse_bool(val_start, &ptr); break;
            case 'n': val = json_parse_null(val_start, &ptr); break;
            default: val = json_parse_number(val_start, &ptr); break;
        }
        array_append(arr, ANY_POINTER(val));
        // skip spaces
        while (isspace(*ptr)) {
            ptr++;
        }
        // judge if is the end of array
        if (*ptr == ']') {
            break;
        }
        else {
            ptr++;
        }
    }
    *endptr = ptr + 1;

    return jarr;
}

json_base * json_parse(const char * json_str) {
    json_base * json;
    char * ptr;
    while (isspace(*json_str)) {
        json_str++;
    }
    if (*json_str == '{') {
        json = (json_base*)json_parse_object(json_str + 1, &ptr);
    }
    else {
        json = (json_base*)json_parse_array(json_str + 1, &ptr);
    }
    return json;
}

void json_free(json_base * json) {
    if (json->type == TYPE_JSON_OBJECT) {
        hash_table_iter iter;
        hash_table_iter_init(((json_object*)json)->table, &iter);
        while (hash_table_iter_has(&iter)) {
            hash_node * n = hash_table_iter_next(&iter);
            string_free(any_get_pointer(&n->key));
            json_base * val = any_get_pointer(&n->value);
            if (val) json_free(val);
        }
        hash_table_free(((json_object*)json)->table);
    }
    else if (json->type == TYPE_JSON_ARRAY) {
        array_iter iter;
        array_iter_init(((json_array*)json)->arr, &iter);
        while (array_iter_has(&iter)) {
            json_base * val = any_get_pointer(array_iter_next_ref(&iter));
            if (val) json_free(val);
        }
        array_free(((json_array*)json)->arr);
    }
    else if (json->type == TYPE_JSON_STRING) {
        string_free(((json_string*)json)->str);
    }
    free(json);
}

json_base * json_object_get(const json_object * object, const char * key) {
    string * keystr = string_new(key);
    any _key = ANY_POINTER(keystr);
    any * val = hash_table_get_val(object->table, _key);
    string_free(keystr);
    return any_get_pointer(val);
}

json_base * json_array_get(const json_array * array, size_t idx) {
    json_base * val = any_get_pointer(array_get_ref(array->arr, idx));
    return val;
}

void json_write(const json_base * json, string ** str) {
    if (json == NULL) {
        string_append(str, "null");
    }
    else if (json->type == TYPE_JSON_STRING) {
        string_append(str, "\"");
        string_append(str, string_get_raw(((json_string*)json)->str));
        string_append(str, "\"");
    }
    else if (json->type == TYPE_JSON_BOOL) {
        if (((json_bool*)json)->boolean) {
            string_append(str, "true");
        }
        else {
            string_append(str, "false");
        }
    }
    else if (json->type == TYPE_JSON_LONG) {
        char buf[32] = {'\0'};
        sprintf(buf, "%ld", any_get_long(&((json_number*)json)->number));
        string_append(str, buf);
    }
    else if (json->type == TYPE_JSON_DOUBLE) {
        char buf[32] = {'\0'};
        sprintf(buf, "%f", any_get_double(&((json_number*)json)->number));
        string_append(str, buf);
    }
    else if (json->type == TYPE_JSON_OBJECT) {
        hash_table_iter iter;
        hash_table_iter_init(((json_object*)json)->table, &iter);
        string_append(str, "{");
        while (hash_table_iter_has(&iter)) {
            hash_node * n = hash_table_iter_next(&iter);
            // write key
            string_append(str, "\"");
            string_concat(str, any_get_pointer(&n->key));
            string_append(str, "\":");
            // write value
            json_write(any_get_pointer(&n->value), str);
            if (hash_table_iter_has(&iter)) {
                string_append(str, ",");
            }
        }
        string_append(str, "}");
    }
    else {
        array_iter iter;
        array_iter_init(((json_array*)json)->arr, &iter);
        string_append(str, "[");
        while (array_iter_has(&iter)) {
            json_base * val = any_get_pointer(array_iter_next_ref(&iter));
            json_write(val, str);
            if (array_iter_has(&iter)) {
                string_append(str, ",");
            }
        }
        string_append(str, "]");
    }
}
