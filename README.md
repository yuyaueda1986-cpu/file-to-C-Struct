# file-to-C-Struct (libftcs)

ファイルの内容をC構造体にマッピングするライブラリ。
スペース区切りのKVペア形式で記述されたファイルを読み込み、複数レコードを動的配列に格納し、主キーで検索できる。

## ビルド

```bash
make          # libftcs.a をビルド
make example  # example/sample_loader をビルド
make clean    # 成果物を削除
```

## プロジェクト構成

```
include/
  ftcs.h              # 公開ヘッダ (型定義・マクロ・API すべて)
src/
  ftcs_parser.c       # ファイルパーサ / レコードセット / 主キー検索
  ftcs_core.c         # CLI フレームワーク (ftcs_main)
  ftcs_shm.c          # 共有メモリ (スタブ)
example/
  sample_struct.h     # ユーザ定義構造体
  sample_mapping.c    # マッピング定義 + main()
  sample_data.txt     # サンプルデータ
```

## データ形式

スペース区切りのKVペア形式。1行 = 1レコード。`#` で始まる行はコメント:

```
# Sample records
ID=42 NAME=TestItem VALUE=3.14
ID=7 NAME=Widget VALUE=9.81
ID=100 NAME=Gadget VALUE=2.718
```

## 使い方

### 1. 構造体を定義

```c
typedef struct {
    int    id;
    char   name[64];
    double value;
} sample_t;
```

### 2. マッピングテーブルを定義

```c
#include "ftcs.h"

FTCS_MAPPING_BEGIN(sample_mapping)
    FTCS_FIELD(sample_t, id,    FTCS_TYPE_INT,    "ID")
    FTCS_FIELD(sample_t, name,  FTCS_TYPE_STRING, "NAME")
    FTCS_FIELD(sample_t, value, FTCS_TYPE_DOUBLE, "VALUE")
FTCS_MAPPING_END()
```

### 3. パーサ設定と実行

```c
ftcs_parser_config_t pcfg = {
    .comment_char = '#',
    .kv_separator = "=",
    .primary_key  = "ID",
};

ftcs_record_set_t *rs = ftcs_parse_file("data.txt", &pcfg,
                                        sample_mapping, sizeof(sample_t));

/* 主キーで検索 */
const sample_t *rec = ftcs_find_by_key(rs, sample_mapping,
                                       "ID", "42", sizeof(sample_t));

ftcs_record_set_free(rs);
```

### 4. CLI フレームワーク経由で使う

`ftcs_main()` にコンフィグを渡すと、コマンドラインオプションを自動処理する:

```bash
./sample_loader -f data.txt -d          # 全レコード出力
./sample_loader -f data.txt -d -k 42   # ID=42 のみ出力
./sample_loader -f data.txt -d -k 999  # 該当なしエラー
```

## 主要API

| 関数 | 説明 |
|---|---|
| `ftcs_parse_file()` | ファイルを解析し `ftcs_record_set_t *` を返す |
| `ftcs_record_set_free()` | レコードセットを解放 |
| `ftcs_find_by_key()` | 主キーでレコードを線形探索 |
| `ftcs_main()` | CLIエントリポイント (`-f`, `-d`, `-k`, `-h`) |

## 対応フィールド型

| マクロ | C型 |
|---|---|
| `FTCS_TYPE_INT` | `int` |
| `FTCS_TYPE_LONG` | `long` |
| `FTCS_TYPE_FLOAT` | `float` |
| `FTCS_TYPE_DOUBLE` | `double` |
| `FTCS_TYPE_CHAR` | `char` |
| `FTCS_TYPE_STRING` | `char[]` (固定長配列) |

## コーディング規約

- C11 (`-std=c11`)
- コンパイラ警告: `-Wall -Wextra`
- プレフィックス: 公開APIは `ftcs_` を使用
- マッピング定義には `FTCS_FIELD` / `FTCS_MAPPING_BEGIN` / `FTCS_MAPPING_END` マクロを使用
