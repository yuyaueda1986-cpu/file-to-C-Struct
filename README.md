# file-to-C-Struct (libftcs)

ファイルの内容をC構造体にマッピングするライブラリ。
スペース区切りのKVペア形式で記述されたファイルを読み込み、複数レコードを動的配列に格納し、主キーで検索できる。

## ビルド

```bash
make           # libftcs.a をビルド
make example   # example/sample_loader をビルド  （主キー FIELD モード）
make example2  # example2/sensor_loader をビルド （主キー INDEX モード）
make test      # gtest スイートをビルドして実行
make clean     # 成果物を削除
```

## プロジェクト構成

```
include/
  ftcs.h              # 公開ヘッダ (型定義・マクロ・API すべて)
src/
  ftcs_parser.c       # ファイルパーサ / レコードセット / 主キー検索
  ftcs_core.c         # CLI フレームワーク (ftcs_main)
example/              # 主キー FIELD モード サンプル
  sample_struct.h     # ユーザ定義構造体
  sample_mapping.c    # マッピング定義 + main()
  sample_data.txt     # サンプルデータ
example2/             # 主キー INDEX モード サンプル
  sensor_struct.h     # ID メンバを持たない構造体
  sensor_loader.c     # マッピング定義 + main()
  sensor_data.txt     # 1ベース ID フィールドつきデータ
test/
  test_ftcs.cpp       # gtest スイート
  data/               # テスト用データファイル群
```

## データ形式

スペース区切りのKVペア形式。1行 = 1レコード。`#` で始まる行はコメント:

```
# Sample records
ID=42 NAME=TestItem VALUE=3.14
ID=7  NAME=Widget   VALUE=9.81
ID=100 NAME=Gadget  VALUE=2.718
```

## 使い方

### 主キー FIELD モード（デフォルト）

構造体フィールドの値でレコードを検索する通常モード。

#### 1. 構造体を定義

```c
typedef struct {
    int    id;
    char   name[64];
    double value;
} sample_t;
```

#### 2. マッピングテーブルを定義

```c
#include "ftcs.h"

FTCS_MAPPING_BEGIN(sample_mapping, sample_t)
    FTCS_FIELD(id,    "ID")
    FTCS_FIELD(name,  "NAME")
    FTCS_FIELD(value, "VALUE")
FTCS_MAPPING_END()
```

`FTCS_MAPPING_BEGIN` の第2引数に構造体型を指定する。
`FTCS_FIELD` はメンバの C 型を C11 `_Generic` で自動推論するため、型指定は不要。

#### 3. パーサ設定と実行

```c
ftcs_parser_config_t pcfg = {
    .comment_char = '#',
    .kv_separator = "=",
    .primary_key  = "ID",
    /* primary_key_mode は省略 → FTCS_KEY_FIELD */
};

ftcs_record_set_t *rs = ftcs_parse_file("data.txt", &pcfg,
                                        sample_mapping, sizeof(sample_t));

/* 主キー (ID=42) で検索 */
const sample_t *rec = ftcs_find_by_key(rs, sample_mapping,
                                       "ID", "42", sizeof(sample_t));

ftcs_record_set_free(rs);
```

#### CLI 動作例

```bash
./sample_loader -f data.txt -d          # 全レコード出力
./sample_loader -f data.txt -d -k 42   # ID=42 のみ出力
./sample_loader -f data.txt -d -k 999  # 該当なしエラー
```

---

### 主キー INDEX モード

0ベースの配列添え字でレコードに直接アクセスするモード。
主キーフィールドを構造体メンバとして持つ必要がない。

#### index_field_name を使った位置指定読み込み

データファイルに 1ベース整数の位置フィールド（例: `ID`）を置くと、
**ファイル内の出現順序に関係なく** 各レコードを `array[ID - 1]` の位置に格納する。

データファイル例（順序不問）:
```
# index 2
ID=3 LOCATION=ServerRoom TEMP=18.0 HUMIDITY=40.0
# index 0
ID=1 LOCATION=RoomA      TEMP=22.5 HUMIDITY=60.0
# index 1
ID=2 LOCATION=RoomB      TEMP=25.1 HUMIDITY=55.3
```

構造体（ID メンバ不要）:
```c
typedef struct {
    char  location[32];
    float temperature;
    float humidity;
} sensor_t;
```

設定:
```c
ftcs_parser_config_t pcfg = {
    .comment_char     = '#',
    .kv_separator     = "=",
    .primary_key_mode = FTCS_KEY_INDEX,
    .index_field_name = "ID",  /* 1ベース位置フィールド名 */
};

ftcs_record_set_t *rs = ftcs_parse_file("sensor.txt", &pcfg,
                                        sensor_mapping, sizeof(sensor_t));

/* 0ベース添え字で取得 (-k 0 → ID=1 のレコード) */
const sensor_t *rec = ftcs_find_by_index(rs, "0", sizeof(sensor_t));

ftcs_record_set_free(rs);
```

**INDEX モードの制約:**
- `-k` には非負整数のみ指定可能
- 指定値がレコード数以上の場合はエラー（配列外アクセス防止）
- `index_field_name` フィールドは構造体マッピングに含めない

#### CLI 動作例

```bash
./sensor_loader -f sensor.txt -d         # 全レコード出力（ID順に並んでいる）
./sensor_loader -f sensor.txt -d -k 0   # array[0] = ID=1 のレコード
./sensor_loader -f sensor.txt -d -k 2   # array[2] = ID=3 のレコード
./sensor_loader -f sensor.txt -d -k 99  # 範囲外エラー
```

---

## 主要API

| 関数 | 説明 |
|---|---|
| `ftcs_parse_file()` | ファイルを解析し `ftcs_record_set_t *` を返す |
| `ftcs_record_set_free()` | レコードセットを解放 |
| `ftcs_find_by_key()` | 主キーフィールドでレコードを線形探索（FTCS_KEY_FIELD） |
| `ftcs_find_by_index()` | 0ベース添え字でレコードを直接取得（FTCS_KEY_INDEX、O(1)） |
| `ftcs_main()` | CLIエントリポイント (`-f`, `-d`, `-k`, `-h`) |

`ftcs_config_t` の `shm_addr` / `shm_size` フィールドに呼び出し元が確保した共有メモリ領域を渡すことで、共有メモリへの書き込みが有効になる（`NULL` で無効）。

## 対応フィールド型

| マクロ | C型 | 自動推論 |
|---|---|---|
| `FTCS_TYPE_INT` | `int` | ✓ |
| `FTCS_TYPE_LONG` | `long` | ✓ |
| `FTCS_TYPE_SHORT` | `short` | ✓ |
| `FTCS_TYPE_FLOAT` | `float` | ✓ |
| `FTCS_TYPE_DOUBLE` | `double` | ✓ |
| `FTCS_TYPE_CHAR` | `char` | ✓ |
| `FTCS_TYPE_STRING` | `char[]` (固定長配列) | ✓ (配列は `char *` に decay) |

## コーディング規約

### 基本

- 言語標準: C11 (`-std=c11`)、テストは C++17
- コンパイラ警告: `-Wall -Wextra`
- プレフィックス: 公開APIは `ftcs_` を使用
- マッピング定義には `FTCS_FIELD` / `FTCS_MAPPING_BEGIN` / `FTCS_MAPPING_END` マクロを使用
- **1翻訳単位に 1 マッピングのみ**（`_ftcs_current_t` typedef の衝突を避けるため）

### コメント

- コメントはすべて**日本語**で記述する
- 関数・構造体・enum には **Doxygen 形式** (`/** @brief ... */`) のコメントを付与する
- コメントは *what*（何をしているか）ではなく *why*（なぜそうするか）を語る

```c
/* Good — 理由を説明している */
timeout_ms = 200;  // センサーの応答仕様上限が180ms のため、余裕を持たせた値

/* Bad — コードの翻訳にすぎない */
timeout_ms = 200;  // タイムアウトを200msに設定する
```

### コードスタイル

- `if` / `else` / `for` / `while` の本体が1行でも必ず `{}` を付ける
- マジックナンバーは必ず名前付き定数 (`#define` / `const`) にし、値の根拠をコメントで示す
- 識別子はすべて `snake_case`、定数・マクロは `ALL_CAPS`、typedef 型は末尾に `_t`

### static 関数の配置

ファイル先頭に前方宣言（目次）をまとめ、関数定義はトップダウン（呼び出す側を上）に並べる。
読者がファイルの先頭から「概要→詳細」の順に読めるようにするためである。

```c
/* --- 関数宣言（目次） --- */
static int  parse(char *line, ...);   /* 上位処理 */
static int  lookup(const char *key);  /* 中位処理 */
static void convert(void *field, ...); /* 末端処理 */

/* --- 関数定義（概要→詳細の順） --- */
static int parse(char *line, ...) { ... lookup(...); ... }
static int lookup(const char *key) { ... convert(...); ... }
static void convert(void *field, ...) { ... }
```

## 注意事項

- 共有メモリ管理は呼び出し元の責務。`ftcs_config_t` の `shm_addr` / `shm_size` に確保済み領域を渡すこと（`NULL` で無効）。
- `ftcs_record_set_t` を使い終わったら必ず `ftcs_record_set_free()` で解放すること。
- `ftcs_find_by_key()` は線形探索のため、大量レコード時はパフォーマンスに注意。
- `ftcs_find_by_index()` は O(1) だがバウンドチェックあり。
- `index_field_name` を使う場合、ID が飛び番だと間のスロットはゼロ初期化される。
