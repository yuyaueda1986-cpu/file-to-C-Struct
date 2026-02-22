#include <stdio.h>
#include "ftcs.h"
#include "sensor_struct.h"

/*
 * Example 2 — FTCS_KEY_INDEX mode
 *
 * Demonstrates index-based primary key:
 *   - The struct has no ID/index member.
 *   - Records are identified solely by their position in the loaded array.
 *   - -k <n>  selects the n-th record (0-based); values >= record count are rejected.
 *
 * Usage:
 *   sensor_loader -f sensor_data.txt -d          # dump all records
 *   sensor_loader -f sensor_data.txt -d -k 0     # first record
 *   sensor_loader -f sensor_data.txt -d -k 2     # third record
 *   sensor_loader -f sensor_data.txt -d -k 99    # error: out of range
 */

FTCS_MAPPING_BEGIN(sensor_mapping, sensor_t)
    FTCS_FIELD(location,    "LOCATION")
    FTCS_FIELD(temperature, "TEMP")
    FTCS_FIELD(humidity,    "HUMIDITY")
FTCS_MAPPING_END()

static void sensor_dump(const void *data)
{
    const sensor_t *s = (const sensor_t *)data;
    printf("sensor_t {\n");
    printf("  location    = \"%s\"\n", s->location);
    printf("  temperature = %.1f C\n", s->temperature);
    printf("  humidity    = %.1f %%\n", s->humidity);
    printf("}\n");
}

int main(int argc, char *argv[])
{
    ftcs_config_t config = {
        .program_name  = "sensor_loader",
        .mapping       = sensor_mapping,
        .parser_config = &(ftcs_parser_config_t){
            .comment_char     = '#',
            .kv_separator     = "=",
            .primary_key_mode = FTCS_KEY_INDEX,
            .index_field_name = "ID",  /* 1-based position field; not mapped to struct */
        },
        .struct_size = sizeof(sensor_t),
        .dump_fn     = sensor_dump,
    };
    return ftcs_main(argc, argv, &config);
}
