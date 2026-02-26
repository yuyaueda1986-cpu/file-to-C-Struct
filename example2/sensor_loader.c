#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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

#define SHM_NAME     "/ftcs_sensor"
#define SHM_CAPACITY 64

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
    size_t shm_size = SHM_CAPACITY * sizeof(sensor_t);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (fd == -1) { perror("shm_open"); return 1; }
    if (ftruncate(fd, (off_t)shm_size) == -1) {
        perror("ftruncate"); close(fd); shm_unlink(SHM_NAME); return 1;
    }
    void *shm_addr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (shm_addr == MAP_FAILED) {
        perror("mmap"); shm_unlink(SHM_NAME); return 1;
    }

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
        .shm_addr    = shm_addr,
        .shm_size    = shm_size,
    };
    int ret = ftcs_main(argc, argv, &config);

    munmap(shm_addr, shm_size);
    shm_unlink(SHM_NAME);
    return ret;
}
