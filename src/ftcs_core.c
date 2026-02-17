#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "ftcs.h"

static void print_usage(const ftcs_config_t *config)
{
    fprintf(stderr,
        "Usage: %s [options]\n"
        "  -f, --file <path>       Input file path (required)\n"
        "  -d, --dump              Dump struct contents\n"
        "  -k, --key <value>       Search by primary key value\n"
        "  -h, --help              Show this help\n",
        config->program_name);
}

int ftcs_main(int argc, char *argv[], const ftcs_config_t *config)
{
    const char *filepath = NULL;
    const char *key_value = NULL;
    int do_dump = 0;

    static struct option long_opts[] = {
        { "file",    required_argument, NULL, 'f' },
        { "dump",    no_argument,       NULL, 'd' },
        { "key",     required_argument, NULL, 'k' },
        { "help",    no_argument,       NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:dk:h", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'f':
            filepath = optarg;
            break;
        case 'd':
            do_dump = 1;
            break;
        case 'k':
            key_value = optarg;
            break;
        case 'h':
            print_usage(config);
            return 0;
        default:
            print_usage(config);
            return 1;
        }
    }

    if (!filepath) {
        fprintf(stderr, "%s: --file is required\n", config->program_name);
        print_usage(config);
        return 1;
    }

    ftcs_record_set_t *rs = ftcs_parse_file(filepath, config->parser_config,
                                            config->mapping, config->struct_size);
    if (!rs) {
        fprintf(stderr, "%s: failed to parse '%s'\n",
                config->program_name, filepath);
        return 1;
    }

    int ret = 0;

    if (do_dump) {
        if (!config->dump_fn) {
            fprintf(stderr, "%s: no dump function registered\n",
                    config->program_name);
            ret = 1;
            goto cleanup;
        }

        if (key_value) {
            const char *pk = config->parser_config->primary_key;
            if (!pk) {
                fprintf(stderr, "%s: no primary_key configured\n",
                        config->program_name);
                ret = 1;
                goto cleanup;
            }
            const void *rec = ftcs_find_by_key(rs, config->mapping,
                                               pk, key_value,
                                               config->struct_size);
            if (!rec) {
                fprintf(stderr, "%s: no record with %s=%s\n",
                        config->program_name, pk, key_value);
                ret = 1;
                goto cleanup;
            }
            config->dump_fn(rec);
        } else {
            for (size_t i = 0; i < rs->count; i++) {
                const void *rec = (const char *)rs->records + i * config->struct_size;
                config->dump_fn(rec);
            }
        }
    }

cleanup:
    ftcs_record_set_free(rs);
    return ret;
}
