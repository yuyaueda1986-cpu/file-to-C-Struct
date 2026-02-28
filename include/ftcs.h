#ifndef FTCS_H
#define FTCS_H

#include <stddef.h>

// --- フィールド型定義 ---

/**
 * @brief 構造体フィールドのデータ型
 */
typedef enum {
    FTCS_TYPE_INT,    /**< int 型 */
    FTCS_TYPE_FLOAT,  /**< float 型 */
    FTCS_TYPE_DOUBLE, /**< double 型 */
    FTCS_TYPE_STRING, /**< 文字列型（char 配列） */
    FTCS_TYPE_CHAR,   /**< char 型（1文字） */
    FTCS_TYPE_LONG,   /**< long 型 */
    FTCS_TYPE_SHORT,  /**< short 型 */
} ftcs_field_type_t;

/**
 * @brief フィールドマッピングエントリ1件
 *
 * ファイル上のキー名と、対応する構造体フィールドの位置・型を紐付ける。
 */
typedef struct {
    const char       *field_name; /**< ファイル内のキー名（識別子） */
    size_t            offset;     /**< 対象構造体内のバイトオフセット */
    size_t            size;       /**< フィールドのバイトサイズ */
    ftcs_field_type_t type;       /**< フィールドのデータ型 */
} ftcs_field_mapping_t;

// --- マッピングテーブル定義マクロ ---

/**
 * @brief マッピングテーブル定義の開始マクロ
 *
 * FTCS_MAPPING_END() と対で使用する。
 * @param name        テーブル変数名
 * @param struct_type マッピング対象の構造体型
 */
#define FTCS_MAPPING_BEGIN(name, struct_type) \
    typedef struct_type _ftcs_current_t;       \
    static const ftcs_field_mapping_t name[] = {

/**
 * @brief メンバの型を C11 _Generic で自動推定するマクロ
 * @param member 型推定対象のメンバ名
 */
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

/**
 * @brief フィールドマッピングエントリを1件追加するマクロ
 * @param member 構造体メンバ名
 * @param fname  ファイル内のキー名（文字列）
 */
#define FTCS_FIELD(member, fname) \
    { .field_name = (fname), \
      .offset = offsetof(_ftcs_current_t, member), \
      .size   = sizeof(((_ftcs_current_t *)0)->member), \
      .type   = FTCS_INFER_TYPE(member) },

/**
 * @brief マッピングテーブル定義の終端マクロ
 *
 * 番兵エントリ（field_name == NULL）を追加してテーブルを閉じる。
 */
#define FTCS_MAPPING_END() \
    { .field_name = NULL } };

// --- パーサー ---

/**
 * @brief プライマリキーの検索モード
 */
typedef enum {
    FTCS_KEY_FIELD = 0, /**< 名前付き構造体フィールドで検索（デフォルト） */
    FTCS_KEY_INDEX = 1, /**< 配列添字（整数）で検索 */
} ftcs_primary_key_mode_t;

/**
 * @brief パーサー設定
 */
typedef struct {
    char        comment_char;   /**< コメント行の先頭文字（デフォルト: '#'） */
    const char *kv_separator;   /**< キーと値の区切り文字列（例: "="） */
    const char *primary_key;    /**< プライマリキーのフィールド名（例: "ID"）。
                                     primary_key_mode == FTCS_KEY_FIELD のときのみ使用 */
    ftcs_primary_key_mode_t primary_key_mode; /**< キー検索モード（デフォルト: FTCS_KEY_FIELD） */
    const char *index_field_name; /**< FTCS_KEY_INDEX 時にレコードの配置位置を示すフィールド名。
                                       値は 1-based の整数で、array[値-1] に格納される。
                                       NULL のときは出現順（sequential）に格納する。
                                       このフィールド自体は構造体メンバには書き込まれない。 */
} ftcs_parser_config_t;

/**
 * @brief パース結果のレコード集合（動的配列）
 */
typedef struct {
    void   *records;     /**< 構造体の連続配列（calloc で確保） */
    size_t  count;       /**< 格納済みレコード数 */
    size_t  capacity;    /**< 確保済みスロット数 */
    size_t  struct_size; /**< 1レコードのバイトサイズ */
} ftcs_record_set_t;

/**
 * @brief ファイルをパースしてレコード集合を返す
 *
 * コメント行・空行を読み飛ばし、各行をスペース区切りの KEY=VALUE 形式として
 * 構造体インスタンスにマッピングする。
 *
 * @param filepath    入力ファイルのパス
 * @param config      パーサー設定
 * @param mapping     フィールドマッピングテーブル（末尾は field_name == NULL の番兵）
 * @param struct_size 1レコードのバイトサイズ（sizeof(型) を渡すこと）
 * @return 成功時は新たに確保した ftcs_record_set_t へのポインタ、失敗時は NULL
 * @note 戻り値は必ず ftcs_record_set_free() で解放すること
 */
ftcs_record_set_t *ftcs_parse_file(const char *filepath,
                                   const ftcs_parser_config_t *config,
                                   const ftcs_field_mapping_t *mapping,
                                   size_t struct_size);

/**
 * @brief ftcs_parse_file() が返したレコード集合を解放する
 * @param rs 解放対象（NULL でも安全に無視される）
 */
void ftcs_record_set_free(ftcs_record_set_t *rs);

/**
 * @brief プライマリキー値でレコードを線形検索する（FTCS_KEY_FIELD 用）
 *
 * @param rs               検索対象のレコード集合
 * @param mapping          フィールドマッピングテーブル
 * @param primary_key_name プライマリキーのフィールド名
 * @param key_value        検索するキー値（文字列）
 * @param struct_size      1レコードのバイトサイズ
 * @return 一致レコードへのポインタ（rs->records 内）、見つからなければ NULL
 */
const void *ftcs_find_by_key(const ftcs_record_set_t *rs,
                             const ftcs_field_mapping_t *mapping,
                             const char *primary_key_name,
                             const char *key_value,
                             size_t struct_size);

/**
 * @brief 配列添字でレコードを取得する（FTCS_KEY_INDEX 用）
 *
 * @param rs          検索対象のレコード集合
 * @param key_value   0-based の整数を表す文字列
 * @param struct_size 1レコードのバイトサイズ
 * @return 対応レコードへのポインタ、範囲外または不正な場合は NULL
 */
const void *ftcs_find_by_index(const ftcs_record_set_t *rs,
                               const char *key_value,
                               size_t struct_size);

// --- フレームワーク エントリポイント ---

/**
 * @brief フレームワーク全体の設定（利用者の main() が用意する）
 */
typedef struct {
    const char                 *program_name;  /**< プログラム名（使用方法表示に使用） */
    const ftcs_field_mapping_t *mapping;       /**< フィールドマッピングテーブル */
    const ftcs_parser_config_t *parser_config; /**< パーサー設定 */
    size_t                      struct_size;   /**< 1レコードのバイトサイズ */
    void (*dump_fn)(const void *data);         /**< レコード内容をダンプするコールバック（省略可） */
    void                       *shm_addr;      /**< 呼び出し元が用意した共有メモリ先頭アドレス（NULL = 不使用） */
    size_t                      shm_size;      /**< 共有メモリ領域のバイトサイズ */
} ftcs_config_t;

/**
 * @brief フレームワークのエントリポイント
 *
 * CLIオプションを解釈し、ファイルをパースして共有メモリへ書き込む。
 *
 * @param argc   コマンドライン引数の数
 * @param argv   コマンドライン引数の配列
 * @param config フレームワーク設定
 * @return 成功時 0、エラー時は非ゼロ
 */
int ftcs_main(int argc, char *argv[], const ftcs_config_t *config);


#endif /* FTCS_H */
