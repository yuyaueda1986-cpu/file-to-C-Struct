#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ftcs.h"

/* 設定ファイルの1行として想定する最大バイト数。
 * 実用的な KV ファイルではこのサイズを超えることはほぼない。 */
#define LINE_BUF_SIZE 4096

/* 初期確保スロット数。大半のユースケースで再アロケートが不要な値として経験的に選択。 */
#define INITIAL_CAPACITY 16

/* ── 関数宣言（目次） ────────────────────────────────────── */

static int   parse_line_kv(char *line, const char *kv_sep,
                            const ftcs_field_mapping_t *mapping, void *out);
static const ftcs_field_mapping_t *find_mapping(const ftcs_field_mapping_t *mapping,
                                                 const char *name);
static int   set_field(void *out, const ftcs_field_mapping_t *m, const char *val);
static char *trim(char *s);
static int   record_set_grow(ftcs_record_set_t *rs);
static int   record_set_ensure(ftcs_record_set_t *rs, size_t required);
static int   extract_field_int(const char *line, const char *kv_sep,
                               const char *field_name, long *out_val);

/* ── 関数定義（概要→詳細の順） ──────────────────────────── */

/**
 * @brief 1行分のスペース区切り KEY=VALUE ペアを構造体に書き込む
 *
 * @param line   解析対象の行文字列（インプレース変更される）
 * @param kv_sep キーと値の区切り文字列
 * @param mapping フィールドマッピングテーブル
 * @param out    書き込み先の構造体ポインタ
 * @return 成功時 0、解析エラー時 -1
 */
static int parse_line_kv(char *line, const char *kv_sep,
                         const ftcs_field_mapping_t *mapping, void *out)
{
    size_t sep_len = strlen(kv_sep);
    char  *saveptr;
    char  *token = strtok_r(line, " \t", &saveptr);

    while (token) {
        char *sep = strstr(token, kv_sep);
        if (!sep) {
            fprintf(stderr, "ftcs: 不正なトークン（区切り文字 '%s' がない）: %s\n",
                    kv_sep, token);
            return -1;
        }

        *sep      = '\0';
        char *key = token;
        char *val = sep + sep_len;

        const ftcs_field_mapping_t *m = find_mapping(mapping, key);
        if (m) {
            if (set_field(out, m, val) != 0) {
                return -1;
            }
        }

        token = strtok_r(NULL, " \t", &saveptr);
    }
    return 0;
}

/**
 * @brief フィールド名でマッピングエントリを検索する（大文字・小文字を区別）
 *
 * @param mapping フィールドマッピングテーブル（末尾は field_name == NULL の番兵）
 * @param name    検索するフィールド名
 * @return 一致エントリへのポインタ、見つからなければ NULL
 */
static const ftcs_field_mapping_t *find_mapping(const ftcs_field_mapping_t *mapping,
                                                 const char *name)
{
    for (const ftcs_field_mapping_t *m = mapping; m->field_name != NULL; m++) {
        if (strcmp(m->field_name, name) == 0) {
            return m;
        }
    }
    return NULL;
}

/**
 * @brief 文字列値をマッピング情報に従い構造体フィールドに書き込む
 *
 * @param out 書き込み先の構造体ポインタ
 * @param m   書き込み先フィールドのマッピングエントリ
 * @param val 書き込む値（文字列）
 * @return 成功時 0、型変換失敗時 -1
 */
static int set_field(void *out, const ftcs_field_mapping_t *m, const char *val)
{
    char *base = (char *)out;
    char *endptr;

    switch (m->type) {
    case FTCS_TYPE_INT:
        *(int *)(base + m->offset) = (int)strtol(val, &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "ftcs: int として無効な値 '%s'（フィールド: '%s'）\n",
                    val, m->field_name);
            return -1;
        }
        break;
    case FTCS_TYPE_LONG:
        *(long *)(base + m->offset) = strtol(val, &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "ftcs: long として無効な値 '%s'（フィールド: '%s'）\n",
                    val, m->field_name);
            return -1;
        }
        break;
    case FTCS_TYPE_FLOAT:
        *(float *)(base + m->offset) = strtof(val, &endptr);
        if (*endptr != '\0') {
            fprintf(stderr, "ftcs: float として無効な値 '%s'（フィールド: '%s'）\n",
                    val, m->field_name);
            return -1;
        }
        break;
    case FTCS_TYPE_DOUBLE:
        *(double *)(base + m->offset) = strtod(val, &endptr);
        if (*endptr != '\0') {
            fprintf(stderr, "ftcs: double として無効な値 '%s'（フィールド: '%s'）\n",
                    val, m->field_name);
            return -1;
        }
        break;
    case FTCS_TYPE_CHAR:
        *(char *)(base + m->offset) = val[0];
        break;
    case FTCS_TYPE_STRING:
        strncpy(base + m->offset, val, m->size - 1);
        (base + m->offset)[m->size - 1] = '\0';
        break;
    case FTCS_TYPE_SHORT:
        *(short *)(base + m->offset) = (short)strtol(val, &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "ftcs: short として無効な値 '%s'（フィールド: '%s'）\n",
                    val, m->field_name);
            return -1;
        }
        break;
    default:
        fprintf(stderr, "ftcs: フィールド '%s' の型が不明\n", m->field_name);
        return -1;
    }
    return 0;
}

/**
 * @brief 文字列の先頭・末尾の空白をインプレースで除去し、先頭ポインタを返す
 *
 * @param s トリム対象の文字列
 * @return トリム後の先頭ポインタ（s の内部アドレス）
 */
static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                       s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
    return s;
}

/**
 * @brief レコード集合に1件分の空きを確保する（順次追加モード用）
 *
 * 容量が足りない場合は2倍に拡張する。
 *
 * @param rs 拡張対象のレコード集合
 * @return 成功時 0、realloc 失敗時 -1
 */
static int record_set_grow(ftcs_record_set_t *rs)
{
    if (rs->count < rs->capacity) {
        return 0;
    }

    size_t new_cap = rs->capacity * 2;
    void  *new_buf = realloc(rs->records, new_cap * rs->struct_size);
    if (!new_buf) {
        perror("ftcs: realloc");
        return -1;
    }
    rs->records  = new_buf;
    rs->capacity = new_cap;
    return 0;
}

/**
 * @brief レコード集合が指定スロット数以上を保持できるよう容量を確保する（インデックスモード用）
 *
 * 必要に応じて2倍ずつ拡張し、新規スロットはゼロ初期化する。
 *
 * @param rs       拡張対象のレコード集合
 * @param required 必要なスロット数
 * @return 成功時 0、realloc 失敗時 -1
 */
static int record_set_ensure(ftcs_record_set_t *rs, size_t required)
{
    if (required <= rs->capacity) {
        return 0;
    }

    size_t new_cap = rs->capacity;
    while (new_cap < required) {
        new_cap *= 2;
    }

    void *new_buf = realloc(rs->records, new_cap * rs->struct_size);
    if (!new_buf) {
        perror("ftcs: realloc");
        return -1;
    }
    /* 新規スロットをゼロ初期化して、未書き込みスロットを安全な状態にする */
    memset((char *)new_buf + rs->capacity * rs->struct_size, 0,
           (new_cap - rs->capacity) * rs->struct_size);
    rs->records  = new_buf;
    rs->capacity = new_cap;
    return 0;
}

/**
 * @brief KV行から指定フィールドの整数値を抽出する（元の行を変更しない）
 *
 * parse_line_kv と異なり元の行文字列を保護するため内部バッファにコピーして処理する。
 *
 * @param line       解析対象の行文字列
 * @param kv_sep     キーと値の区切り文字列
 * @param field_name 取得するフィールド名
 * @param out_val    取得した値の格納先
 * @return 成功時 0、フィールド未発見または整数変換失敗時 -1
 */
static int extract_field_int(const char *line, const char *kv_sep,
                              const char *field_name, long *out_val)
{
    char buf[LINE_BUF_SIZE];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    size_t sep_len = strlen(kv_sep);
    char  *saveptr;
    char  *token = strtok_r(buf, " \t", &saveptr);

    while (token) {
        char *sep = strstr(token, kv_sep);
        if (sep) {
            *sep      = '\0';
            char *key = token;
            char *val = sep + sep_len;
            if (strcmp(key, field_name) == 0) {
                char *endptr;
                *out_val = strtol(val, &endptr, 10);
                if (*endptr != '\0') {
                    return -1;
                }
                return 0;
            }
        }
        token = strtok_r(NULL, " \t", &saveptr);
    }
    return -1; /* フィールドが見つからなかった */
}

/* ── 公開 API ────────────────────────────────────────────── */

ftcs_record_set_t *ftcs_parse_file(const char *filepath,
                                   const ftcs_parser_config_t *config,
                                   const ftcs_field_mapping_t *mapping,
                                   size_t struct_size)
{
    if (!filepath || !config || !mapping || !config->kv_separator) {
        fprintf(stderr, "ftcs: ftcs_parse_file に NULL 引数が渡された\n");
        return NULL;
    }

    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "ftcs: '%s' を開けない: %s\n", filepath, strerror(errno));
        return NULL;
    }

    /* rs と rs->records は ftcs_record_set_free() で解放される */
    ftcs_record_set_t *rs = calloc(1, sizeof(*rs));
    if (!rs) {
        perror("ftcs: calloc");
        fclose(fp);
        return NULL;
    }

    rs->struct_size = struct_size;
    rs->capacity    = INITIAL_CAPACITY;
    rs->records     = calloc(rs->capacity, struct_size);
    if (!rs->records) {
        perror("ftcs: calloc");
        free(rs);
        fclose(fp);
        return NULL;
    }

    char line[LINE_BUF_SIZE];
    char comment = config->comment_char ? config->comment_char : '#';

    /* index_field_name が指定されている場合は配置位置指定モード */
    int use_index_field = (config->primary_key_mode == FTCS_KEY_INDEX)
                          && (config->index_field_name != NULL);

    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == comment) {
            continue;
        }

        if (use_index_field) {
            /* --- 配置位置指定モード: 1-based インデックスで array[値-1] に格納 --- */
            long id_val;
            if (extract_field_int(trimmed, config->kv_separator,
                                  config->index_field_name, &id_val) != 0) {
                fprintf(stderr,
                        "ftcs: インデックスフィールド '%s' が欠落または不正: %s\n",
                        config->index_field_name, trimmed);
                ftcs_record_set_free(rs);
                fclose(fp);
                return NULL;
            }
            if (id_val < 1) {
                fprintf(stderr,
                        "ftcs: インデックスフィールド '%s' は 1 以上でなければならない（値: %ld）\n",
                        config->index_field_name, id_val);
                ftcs_record_set_free(rs);
                fclose(fp);
                return NULL;
            }

            size_t pos = (size_t)(id_val - 1); /* 1-based を 0-based に変換 */

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

            if (pos + 1 > rs->count) {
                rs->count = pos + 1;
            }

        } else {
            /* --- 順次モード: ファイルの出現順に末尾へ追加 --- */
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
    if (!rs) {
        return;
    }
    free(rs->records);
    free(rs);
}

const void *ftcs_find_by_index(const ftcs_record_set_t *rs,
                               const char *key_value,
                               size_t struct_size)
{
    if (!rs || !key_value) {
        return NULL;
    }

    char *endptr;
    long  idx = strtol(key_value, &endptr, 10);
    if (*endptr != '\0' || idx < 0) {
        fprintf(stderr, "ftcs: インデックスキーは非負整数でなければならない: '%s'\n",
                key_value);
        return NULL;
    }

    if ((size_t)idx >= rs->count) {
        fprintf(stderr, "ftcs: インデックス %ld は範囲外（レコード数: %zu）\n",
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
    if (!rs || !mapping || !primary_key_name || !key_value) {
        return NULL;
    }

    const ftcs_field_mapping_t *m = find_mapping(mapping, primary_key_name);
    if (!m) {
        return NULL;
    }

    for (size_t i = 0; i < rs->count; i++) {
        const char *rec   = (const char *)rs->records + i * struct_size;
        const void *field = rec + m->offset;

        switch (m->type) {
        case FTCS_TYPE_INT:
            if (*(const int *)field == (int)strtol(key_value, NULL, 10)) {
                return rec;
            }
            break;
        case FTCS_TYPE_LONG:
            if (*(const long *)field == strtol(key_value, NULL, 10)) {
                return rec;
            }
            break;
        case FTCS_TYPE_FLOAT:
            if (*(const float *)field == strtof(key_value, NULL)) {
                return rec;
            }
            break;
        case FTCS_TYPE_DOUBLE:
            if (*(const double *)field == strtod(key_value, NULL)) {
                return rec;
            }
            break;
        case FTCS_TYPE_CHAR:
            if (*(const char *)field == key_value[0]) {
                return rec;
            }
            break;
        case FTCS_TYPE_STRING:
            if (strcmp((const char *)field, key_value) == 0) {
                return rec;
            }
            break;
        case FTCS_TYPE_SHORT:
            if (*(const short *)field == (short)strtol(key_value, NULL, 10)) {
                return rec;
            }
            break;
        }
    }
    return NULL;
}
