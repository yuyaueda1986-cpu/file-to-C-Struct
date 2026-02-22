#include <stdio.h>
#include "ftcs.h"
#include "sample_struct.h"

/* Define the mapping from file fields to struct members */
FTCS_MAPPING_BEGIN(sample_mapping, sample_t)
    FTCS_FIELD(id,    "ID")
    FTCS_FIELD(name,  "NAME")
    FTCS_FIELD(value, "VALUE")
FTCS_MAPPING_END()

/* Pretty-print callback */
static void sample_dump(const void *data)
{
    const sample_t *s = (const sample_t *)data;
    printf("sample_t {\n");
    printf("  id    = %d\n", s->id);
    printf("  name  = \"%s\"\n", s->name);
    printf("  value = %f\n", s->value);
    printf("}\n");
}

int main(int argc, char *argv[])
{
    ftcs_config_t config = {
        .program_name  = "sample_loader",
        .mapping       = sample_mapping,
        .parser_config = &(ftcs_parser_config_t){
            .comment_char = '#',
            .kv_separator = "=",
            .primary_key  = "ID",
        },
        .struct_size = sizeof(sample_t),
        .dump_fn     = sample_dump,
    };
    return ftcs_main(argc, argv, &config);
}
