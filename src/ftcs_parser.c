#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ftcs.h"

#define INITIAL_CAPACITY 16

/* Find a mapping entry by field name (case-sensitive) */
static const ftcs_field_mapping_t *find_mapping(const ftcs_field_mapping_t *mapping,
                                                 const char *name)
{
    for (const ftcs_field_mapping_t *m = mapping; m->field_name != NULL; m++) {
        if (strcmp(m->field_name, name) == 0)
            return m;
    }
    return NULL;
}

/* Trim leading and trailing whitespace in-place, return pointer to trimmed start */
static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                       s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
    return s;
}

/* Set a struct field from a string value based on the mapping entry */
static int set_field(void *out_struct, const ftcs_field_mapping_t *m, const char *value)
{
    char *base = (char *)out_struct;
    char *endptr;

    switch (m->type) {
    case FTCS_TYPE_INT:
        *(int *)(base + m->offset) = (int)strtol(value, &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "ftcs: invalid int value '%s' for field '%s'\n",
                    value, m->field_name);
            return -1;
        }
        break;
    case FTCS_TYPE_LONG:
        *(long *)(base + m->offset) = strtol(value, &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "ftcs: invalid long value '%s' for field '%s'\n",
                    value, m->field_name);
            return -1;
        }
        break;
    case FTCS_TYPE_FLOAT:
        *(float *)(base + m->offset) = strtof(value, &endptr);
        if (*endptr != '\0') {
            fprintf(stderr, "ftcs: invalid float value '%s' for field '%s'\n",
                    value, m->field_name);
            return -1;
        }
        break;
    case FTCS_TYPE_DOUBLE:
        *(double *)(base + m->offset) = strtod(value, &endptr);
        if (*endptr != '\0') {
            fprintf(stderr, "ftcs: invalid double value '%s' for field '%s'\n",
                    value, m->field_name);
            return -1;
        }
        break;
    case FTCS_TYPE_CHAR:
        *(char *)(base + m->offset) = value[0];
        break;
    case FTCS_TYPE_STRING:
        strncpy(base + m->offset, value, m->size - 1);
        (base + m->offset)[m->size - 1] = '\0';
        break;
    case FTCS_TYPE_SHORT:
        *(short *)(base + m->offset) = (short)strtol(value, &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "ftcs: invalid short value '%s' for field '%s'\n",
                    value, m->field_name);
            return -1;
        }
        break;
    default:
        fprintf(stderr, "ftcs: unknown type for field '%s'\n", m->field_name);
        return -1;
    }
    return 0;
}

/*
 * Parse a single line of space-delimited KEY=VALUE pairs into a struct.
 * The line is modified in-place.
 */
static int parse_line_kv(char *line, const char *kv_separator,
                         const ftcs_field_mapping_t *mapping,
                         void *out_struct)
{
    size_t sep_len = strlen(kv_separator);
    char *saveptr;
    char *token = strtok_r(line, " \t", &saveptr);

    while (token) {
        char *sep = strstr(token, kv_separator);
        if (!sep) {
            fprintf(stderr, "ftcs: malformed token (no '%s'): %s\n",
                    kv_separator, token);
            return -1;
        }

        *sep = '\0';
        char *key = token;
        char *val = sep + sep_len;

        const ftcs_field_mapping_t *m = find_mapping(mapping, key);
        if (m) {
            if (set_field(out_struct, m, val) != 0)
                return -1;
        }

        token = strtok_r(NULL, " \t", &saveptr);
    }
    return 0;
}

/* Ensure record set has room for one more record (sequential mode) */
static int record_set_grow(ftcs_record_set_t *rs)
{
    if (rs->count < rs->capacity)
        return 0;

    size_t new_cap = rs->capacity * 2;
    void *new_buf = realloc(rs->records, new_cap * rs->struct_size);
    if (!new_buf) {
        perror("ftcs: realloc");
        return -1;
    }
    rs->records = new_buf;
    rs->capacity = new_cap;
    return 0;
}

/* Ensure record set capacity covers at least `required` slots (index mode) */
static int record_set_ensure(ftcs_record_set_t *rs, size_t required)
{
    if (required <= rs->capacity)
        return 0;

    size_t new_cap = rs->capacity;
    while (new_cap < required)
        new_cap *= 2;

    void *new_buf = realloc(rs->records, new_cap * rs->struct_size);
    if (!new_buf) {
        perror("ftcs: realloc");
        return -1;
    }
    /* Zero-initialise the newly added slots */
    memset((char *)new_buf + rs->capacity * rs->struct_size, 0,
           (new_cap - rs->capacity) * rs->struct_size);
    rs->records = new_buf;
    rs->capacity = new_cap;
    return 0;
}

/*
 * Extract a single integer field from a KV line without modifying the original.
 * Returns 0 and sets *out_val on success, -1 if the field is absent or not an integer.
 */
static int extract_field_int(const char *line, const char *kv_separator,
                              const char *field_name, long *out_val)
{
    char buf[4096];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    size_t sep_len = strlen(kv_separator);
    char *saveptr;
    char *token = strtok_r(buf, " \t", &saveptr);

    while (token) {
        char *sep = strstr(token, kv_separator);
        if (sep) {
            *sep = '\0';
            char *key = token;
            char *val = sep + sep_len;
            if (strcmp(key, field_name) == 0) {
                char *endptr;
                *out_val = strtol(val, &endptr, 10);
                if (*endptr != '\0')
                    return -1;
                return 0;
            }
        }
        token = strtok_r(NULL, " \t", &saveptr);
    }
    return -1; /* field not found */
}

ftcs_record_set_t *ftcs_parse_file(const char *filepath,
                                   const ftcs_parser_config_t *config,
                                   const ftcs_field_mapping_t *mapping,
                                   size_t struct_size)
{
    if (!filepath || !config || !mapping || !config->kv_separator) {
        fprintf(stderr, "ftcs: NULL argument to ftcs_parse_file\n");
        return NULL;
    }

    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "ftcs: cannot open '%s': %s\n", filepath, strerror(errno));
        return NULL;
    }

    ftcs_record_set_t *rs = calloc(1, sizeof(*rs));
    if (!rs) {
        perror("ftcs: calloc");
        fclose(fp);
        return NULL;
    }

    rs->struct_size = struct_size;
    rs->capacity = INITIAL_CAPACITY;
    rs->records = calloc(rs->capacity, struct_size);
    if (!rs->records) {
        perror("ftcs: calloc");
        free(rs);
        fclose(fp);
        return NULL;
    }

    char line[4096];
    char comment = config->comment_char ? config->comment_char : '#';

    int use_index_field = (config->primary_key_mode == FTCS_KEY_INDEX)
                          && (config->index_field_name != NULL);

    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == comment)
            continue;

        if (use_index_field) {
            /* Index-placement mode: read the 1-based position field and
             * place the record at array[value - 1]. */
            long id_val;
            if (extract_field_int(trimmed, config->kv_separator,
                                  config->index_field_name, &id_val) != 0) {
                fprintf(stderr,
                        "ftcs: missing or invalid index field '%s' in line: %s\n",
                        config->index_field_name, trimmed);
                ftcs_record_set_free(rs);
                fclose(fp);
                return NULL;
            }
            if (id_val < 1) {
                fprintf(stderr,
                        "ftcs: index field '%s' must be >= 1, got %ld\n",
                        config->index_field_name, id_val);
                ftcs_record_set_free(rs);
                fclose(fp);
                return NULL;
            }

            size_t pos = (size_t)(id_val - 1); /* 1-based → 0-based */

            if (record_set_ensure(rs, pos + 1) != 0) {
                ftcs_record_set_free(rs);
                fclose(fp);
                return NULL;
            }

            void *rec = (char *)rs->records + pos * struct_size;
            memset(rec, 0, struct_size);

            if (parse_line_kv(trimmed, config->kv_separator, mapping, rec) != 0) {
                ftcs_record_set_free(rs);
                fclose(fp);
                return NULL;
            }

            if (pos + 1 > rs->count)
                rs->count = pos + 1;

        } else {
            /* Sequential mode: append records in file order */
            if (record_set_grow(rs) != 0) {
                ftcs_record_set_free(rs);
                fclose(fp);
                return NULL;
            }

            void *rec = (char *)rs->records + rs->count * struct_size;
            memset(rec, 0, struct_size);

            if (parse_line_kv(trimmed, config->kv_separator, mapping, rec) != 0) {
                ftcs_record_set_free(rs);
                fclose(fp);
                return NULL;
            }

            rs->count++;
        }
    }

    fclose(fp);
    return rs;
}

void ftcs_record_set_free(ftcs_record_set_t *rs)
{
    if (!rs)
        return;
    free(rs->records);
    free(rs);
}

const void *ftcs_find_by_index(const ftcs_record_set_t *rs,
                               const char *key_value,
                               size_t struct_size)
{
    if (!rs || !key_value)
        return NULL;

    char *endptr;
    long idx = strtol(key_value, &endptr, 10);
    if (*endptr != '\0' || idx < 0) {
        fprintf(stderr, "ftcs: index key must be a non-negative integer: '%s'\n",
                key_value);
        return NULL;
    }

    if ((size_t)idx >= rs->count) {
        fprintf(stderr, "ftcs: index %ld is out of range (record count: %zu)\n",
                idx, rs->count);
        return NULL;
    }

    return (const char *)rs->records + (size_t)idx * struct_size;
}

const void *ftcs_find_by_key(const ftcs_record_set_t *rs,
                             const ftcs_field_mapping_t *mapping,
                             const char *primary_key_name,
                             const char *key_value,
                             size_t struct_size)
{
    if (!rs || !mapping || !primary_key_name || !key_value)
        return NULL;

    const ftcs_field_mapping_t *m = find_mapping(mapping, primary_key_name);
    if (!m)
        return NULL;

    for (size_t i = 0; i < rs->count; i++) {
        const char *rec = (const char *)rs->records + i * struct_size;
        const void *field = rec + m->offset;

        switch (m->type) {
        case FTCS_TYPE_INT:
            if (*(const int *)field == (int)strtol(key_value, NULL, 10))
                return rec;
            break;
        case FTCS_TYPE_LONG:
            if (*(const long *)field == strtol(key_value, NULL, 10))
                return rec;
            break;
        case FTCS_TYPE_FLOAT:
            if (*(const float *)field == strtof(key_value, NULL))
                return rec;
            break;
        case FTCS_TYPE_DOUBLE:
            if (*(const double *)field == strtod(key_value, NULL))
                return rec;
            break;
        case FTCS_TYPE_CHAR:
            if (*(const char *)field == key_value[0])
                return rec;
            break;
        case FTCS_TYPE_STRING:
            if (strcmp((const char *)field, key_value) == 0)
                return rec;
            break;
        case FTCS_TYPE_SHORT:
            if (*(const short *)field == (short)strtol(key_value, NULL, 10))
                return rec;
            break;
        }
    }
    return NULL;
}
