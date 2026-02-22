#ifndef FTCS_H
#define FTCS_H

#include <stddef.h>

/* ── Field types ─────────────────────────────────────────────── */

typedef enum {
    FTCS_TYPE_INT,
    FTCS_TYPE_FLOAT,
    FTCS_TYPE_DOUBLE,
    FTCS_TYPE_STRING,
    FTCS_TYPE_CHAR,
    FTCS_TYPE_LONG,
    FTCS_TYPE_SHORT,
} ftcs_field_type_t;

/* A single field mapping entry */
typedef struct {
    const char       *field_name;   /* Field name/identifier in the file */
    size_t            offset;       /* Offset within the target struct */
    size_t            size;         /* Size of the field in bytes */
    ftcs_field_type_t type;         /* Field type */
} ftcs_field_mapping_t;

/* Macros for defining mapping tables */
#define FTCS_MAPPING_BEGIN(name, struct_type) \
    typedef struct_type _ftcs_current_t;       \
    static const ftcs_field_mapping_t name[] = {

#define FTCS_INFER_TYPE(member) \
    _Generic(((_ftcs_current_t *)0)->member, \
        int:    FTCS_TYPE_INT,    \
        long:   FTCS_TYPE_LONG,   \
        short:  FTCS_TYPE_SHORT,  \
        float:  FTCS_TYPE_FLOAT,  \
        double: FTCS_TYPE_DOUBLE, \
        char:   FTCS_TYPE_CHAR,   \
        char *: FTCS_TYPE_STRING, \
        default: FTCS_TYPE_STRING)

#define FTCS_FIELD(member, fname) \
    { .field_name = (fname), \
      .offset = offsetof(_ftcs_current_t, member), \
      .size   = sizeof(((_ftcs_current_t *)0)->member), \
      .type   = FTCS_INFER_TYPE(member) },

#define FTCS_MAPPING_END() \
    { .field_name = NULL } };

/* ── Parser ──────────────────────────────────────────────────── */

/* Primary key lookup mode */
typedef enum {
    FTCS_KEY_FIELD = 0, /* ON (default): look up by named struct field */
    FTCS_KEY_INDEX = 1, /* OFF: look up by array subscript (int only)  */
} ftcs_primary_key_mode_t;

/* Parser configuration */
typedef struct {
    char        comment_char;   /* Comment line prefix (default: '#') */
    const char *kv_separator;   /* Key-value separator (e.g. "=") */
    const char *primary_key;    /* Primary key field name (e.g. "ID"),
                                   used only when primary_key_mode == FTCS_KEY_FIELD */
    ftcs_primary_key_mode_t primary_key_mode; /* Key lookup mode (default: FTCS_KEY_FIELD) */
    const char *index_field_name; /* FTCS_KEY_INDEX + index_field_name != NULL:
                                     field in the data file that holds a 1-based position.
                                     Each record is placed at array[value - 1].
                                     The field is NOT mapped to any struct member.
                                     If NULL, records are inserted sequentially. */
} ftcs_parser_config_t;

/* Dynamic array of parsed records */
typedef struct {
    void   *records;      /* Contiguous array of structs */
    size_t  count;        /* Number of records */
    size_t  capacity;     /* Allocated capacity */
    size_t  struct_size;  /* Size of one element */
} ftcs_record_set_t;

/*
 * Parse a file and return a record set.
 *
 * Each non-comment, non-empty line is parsed as space-delimited KEY=VALUE pairs
 * into one struct instance.
 *
 * Returns a newly allocated ftcs_record_set_t on success, NULL on error.
 * Caller must free with ftcs_record_set_free().
 */
ftcs_record_set_t *ftcs_parse_file(const char *filepath,
                                   const ftcs_parser_config_t *config,
                                   const ftcs_field_mapping_t *mapping,
                                   size_t struct_size);

/* Free a record set returned by ftcs_parse_file() */
void ftcs_record_set_free(ftcs_record_set_t *rs);

/*
 * Find a record by primary key value (linear search).
 * Used when primary_key_mode == FTCS_KEY_FIELD.
 *
 * Returns pointer to the matching record within rs->records, or NULL if not found.
 */
const void *ftcs_find_by_key(const ftcs_record_set_t *rs,
                             const ftcs_field_mapping_t *mapping,
                             const char *primary_key_name,
                             const char *key_value,
                             size_t struct_size);

/*
 * Find a record by array index (primary_key_mode == FTCS_KEY_INDEX).
 *
 * key_value must be a non-negative integer string.
 * The index must be less than rs->count; larger values are rejected.
 * Returns pointer to the record at that index, or NULL on error.
 */
const void *ftcs_find_by_index(const ftcs_record_set_t *rs,
                               const char *key_value,
                               size_t struct_size);

/* ── Core / CLI ──────────────────────────────────────────────── */

/* Framework configuration – filled in by the user's main() */
typedef struct {
    const char                 *program_name;
    const ftcs_field_mapping_t *mapping;
    const ftcs_parser_config_t *parser_config;
    size_t                      struct_size;
    void (*dump_fn)(const void *data);  /* Optional: pretty-print callback */
} ftcs_config_t;

/*
 * Framework entry point.
 * Parses CLI arguments, reads the file, and operates on shared memory.
 * Returns 0 on success, non-zero on error.
 */
int ftcs_main(int argc, char *argv[], const ftcs_config_t *config);

/* ── Shared memory (stub) ────────────────────────────────────── */

/*
 * Stub: shared memory address getter.
 * Will be replaced with a real implementation later.
 * Currently returns NULL.
 */
void *shm_addr_get(void);

#endif /* FTCS_H */
