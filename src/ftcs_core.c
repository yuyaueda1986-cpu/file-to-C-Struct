#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "ftcs.h"

// --- 関数宣言（目次） ---

static void print_usage(const ftcs_config_t *config); // 使用方法を stderr に表示する

// --- 関数定義（概要→詳細の順） ---

int ftcs_main(int argc, char *argv[], const ftcs_config_t *config)
{
    const char *filepath  = NULL; // 入力ファイルパス（-f で指定）
    const char *key_value = NULL; // 検索キー値（-k で指定）
    int         do_dump   = 0;    // ダンプ出力フラグ（-d で有効化）

    // getopt_long 用オプション定義テーブル
    static struct option long_opts[] = {
        { "file",    required_argument, NULL, 'f' },
        { "dump",    no_argument,       NULL, 'd' },
        { "key",     required_argument, NULL, 'k' },
        { "help",    no_argument,       NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    // --- CLIオプションを解析する ---
    int opt; // getopt_long の戻り値（オプション文字または -1）
    while ((opt = getopt_long(argc, argv, "f:dk:h", long_opts, NULL)) != -1) {
        // オプション文字に応じて対応する変数を設定する
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

    // --file は必須オプション
    if (!filepath) {
        fprintf(stderr, "%s: --file は必須オプション\n", config->program_name);
        print_usage(config);
        return 1;
    }

    // --- ファイルをパースしてレコード集合を構築する ---
    ftcs_record_set_t *rs = ftcs_parse_file(filepath, config->parser_config,
                                            config->mapping, config->struct_size);
    // パース失敗は致命的エラーのため早期リターンする
    if (!rs) {
        fprintf(stderr, "%s: '%s' のパースに失敗した\n",
                config->program_name, filepath);
        return 1;
    }

    // --- パース結果を共有メモリに書き込む ---
    if (config->shm_addr != NULL && config->shm_size > 0) {
        size_t bytes = rs->count * rs->struct_size; // 書き込みバイト数
        if (bytes > config->shm_size) {
            bytes = config->shm_size; // shm 領域を超えないよう切り詰める
        }
        memcpy(config->shm_addr, rs->records, bytes);
    }

    int ret = 0; // 戻り値（エラー発生時に非ゼロを設定する）

    // --- --dump が指定された場合にレコードを出力する ---
    if (do_dump) {
        // dump_fn 未設定は設定ミスのためエラーとする
        if (!config->dump_fn) {
            fprintf(stderr, "%s: dump 関数が登録されていない\n",
                    config->program_name);
            ret = 1;
            goto cleanup;
        }

        // -k が指定された場合は単一レコードを検索してダンプする
        if (key_value) {
            const void *rec = NULL; // 検索で見つかったレコードへのポインタ
            // キーモードに応じて検索関数を切り替える
            if (config->parser_config->primary_key_mode == FTCS_KEY_INDEX) {
                rec = ftcs_find_by_index(rs, key_value, config->struct_size);
                if (!rec) {
                    // エラーメッセージは ftcs_find_by_index 側で出力済み
                    ret = 1;
                    goto cleanup;
                }
            } else {
                // フィールド名で検索するため primary_key の設定が必要
                const char *pk = config->parser_config->primary_key; // プライマリキーのフィールド名
                if (!pk) {
                    fprintf(stderr, "%s: primary_key が設定されていない\n",
                            config->program_name);
                    ret = 1;
                    goto cleanup;
                }
                rec = ftcs_find_by_key(rs, config->mapping,
                                       pk, key_value,
                                       config->struct_size);
                // 指定キーのレコードが存在しない場合はエラーを報告する
                if (!rec) {
                    fprintf(stderr, "%s: %s=%s のレコードが見つからない\n",
                            config->program_name, pk, key_value);
                    ret = 1;
                    goto cleanup;
                }
            }
            config->dump_fn(rec);
        } else {
            // -k 未指定の場合は全レコードを順にダンプする
            for (size_t i = 0; i < rs->count; i++) {
                const void *rec = (const char *)rs->records + i * config->struct_size; // i 番目のレコード
                config->dump_fn(rec);
            }
        }
    }

cleanup:
    ftcs_record_set_free(rs);
    return ret;
}

/**
 * @brief 使用方法を stderr に表示する
 * @param config フレームワーク設定（プログラム名の取得に使用）
 */
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
