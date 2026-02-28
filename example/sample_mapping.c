#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ftcs.h"
#include "sample_struct.h"

#define SHM_NAME "/ftcs_sample"

/* sample_t を最大 64 件格納できる容量。典型的な用途では十分な値。 */
#define SHM_CAPACITY 64

/* ファイルのキー名と sample_t メンバを紐付けるマッピングテーブル */
FTCS_MAPPING_BEGIN(sample_mapping, sample_t)
    FTCS_FIELD(id,    "ID")
    FTCS_FIELD(name,  "NAME")
    FTCS_FIELD(value, "VALUE")
FTCS_MAPPING_END()

/* ── 関数宣言（目次） ────────────────────────────────────── */

static void sample_dump(const void *data);

/* ── 関数定義（概要→詳細の順） ──────────────────────────── */

int main(int argc, char *argv[])
{
    size_t shm_size = SHM_CAPACITY * sizeof(sample_t);

    /* --- 共有メモリの作成とマッピング --- */
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(fd, (off_t)shm_size) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(SHM_NAME);
        return 1;
    }
    void *shm_addr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd); /* fd は mmap 後に不要 */
    if (shm_addr == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        return 1;
    }

    /* --- フレームワーク設定と実行 --- */
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
        .shm_addr    = shm_addr,
        .shm_size    = shm_size,
    };
    int ret = ftcs_main(argc, argv, &config);

    /* --- 共有メモリの後始末 --- */
    munmap(shm_addr, shm_size);
    shm_unlink(SHM_NAME);
    return ret;
}

/**
 * @brief sample_t の内容を標準出力にダンプするコールバック
 * @param data sample_t へのポインタ（void * として渡される）
 */
static void sample_dump(const void *data)
{
    const sample_t *s = (const sample_t *)data;
    printf("sample_t {\n");
    printf("  id    = %d\n",    s->id);
    printf("  name  = \"%s\"\n", s->name);
    printf("  value = %f\n",    s->value);
    printf("}\n");
}
