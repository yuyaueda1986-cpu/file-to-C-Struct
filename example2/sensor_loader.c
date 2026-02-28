#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ftcs.h"
#include "sensor_struct.h"

/*
 * example2 — FTCS_KEY_INDEX モードのデモ
 *
 * 配列添字をプライマリキーとして使用する例:
 *   - 構造体に ID/インデックスメンバは持たない
 *   - レコードはロード済み配列の位置（0-based）で識別する
 *   - -k <n> で n 番目のレコードを指定（レコード数以上の値はエラー）
 *
 * 使用例:
 *   sensor_loader -f sensor_data.txt -d          # 全レコードをダンプ
 *   sensor_loader -f sensor_data.txt -d -k 0     # 先頭レコード
 *   sensor_loader -f sensor_data.txt -d -k 2     # 3番目のレコード
 *   sensor_loader -f sensor_data.txt -d -k 99    # エラー: 範囲外
 */

#define SHM_NAME "/ftcs_sensor"

/* sensor_t を最大 64 件格納できる容量。典型的な用途では十分な値。 */
#define SHM_CAPACITY 64

/* ファイルのキー名と sensor_t メンバを紐付けるマッピングテーブル */
FTCS_MAPPING_BEGIN(sensor_mapping, sensor_t)
    FTCS_FIELD(location,    "LOCATION")
    FTCS_FIELD(temperature, "TEMP")
    FTCS_FIELD(humidity,    "HUMIDITY")
FTCS_MAPPING_END()

/* ── 関数宣言（目次） ────────────────────────────────────── */

static void sensor_dump(const void *data);

/* ── 関数定義（概要→詳細の順） ──────────────────────────── */

int main(int argc, char *argv[])
{
    size_t shm_size = SHM_CAPACITY * sizeof(sensor_t);

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
        .program_name  = "sensor_loader",
        .mapping       = sensor_mapping,
        .parser_config = &(ftcs_parser_config_t){
            .comment_char     = '#',
            .kv_separator     = "=",
            .primary_key_mode = FTCS_KEY_INDEX,
            .index_field_name = "ID", /* 1-based の配置位置フィールド。構造体メンバには書き込まれない */
        },
        .struct_size = sizeof(sensor_t),
        .dump_fn     = sensor_dump,
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
 * @brief sensor_t の内容を標準出力にダンプするコールバック
 * @param data sensor_t へのポインタ（void * として渡される）
 */
static void sensor_dump(const void *data)
{
    const sensor_t *s = (const sensor_t *)data;
    printf("sensor_t {\n");
    printf("  location    = \"%s\"\n", s->location);
    printf("  temperature = %.1f C\n", s->temperature);
    printf("  humidity    = %.1f %%\n", s->humidity);
    printf("}\n");
}
