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
} ftcs_field_type_t;

/* A single field mapping entry */
typedef struct {
    const char       *field_name;   /* Field name/identifier in the file */
    size_t            offset;       /* Offset within the target struct */
    size_t            size;         /* Size of the field in bytes */
    ftcs_field_type_t type;         /* Field type */
} ftcs_field_mapping_t;

/* Macros for defining mapping tables */
#define FTCS_MAPPING_BEGIN(name) \
    static const ftcs_field_mapping_t name[] = {

#define FTCS_FIELD(struct_type, member, ftype, fname) \
    { .field_name = (fname), \
      .offset = offsetof(struct_type, member), \
      .size = sizeof(((struct_type *)0)->member), \
      .type = (ftype) },

#define FTCS_MAPPING_END() \
    { .field_name = NULL } };

/* ── Parser ──────────────────────────────────────────────────── */

/* Parser configuration */
typedef struct {
    char        comment_char;   /* Comment line prefix (default: '#') */
    const char *kv_separator;   /* Key-value separator (e.g. "=") */
    const char *primary_key;    /* Primary key field name (e.g. "ID") */
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
 *
 * Returns pointer to the matching record within rs->records, or NULL if not found.
 */
const void *ftcs_find_by_key(const ftcs_record_set_t *rs,
                             const ftcs_field_mapping_t *mapping,
                             const char *primary_key_name,
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
