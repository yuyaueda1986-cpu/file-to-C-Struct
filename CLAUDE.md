# CLAUDE.md — file-to-C-Struct (libftcs)

## プロジェクト概要

スペース区切りKVペア形式のファイルをC構造体にマッピングするライブラリ。
動的配列でレコードを管理し、主キー検索をサポートする。

## ビルド

```bash
make            # libftcs.a をビルド
make example    # example/sample_loader をビルド
make example2   # example2/sensor_loader をビルド（FTCS_KEY_INDEX デモ）
make clean      # 成果物を削除
```

コンパイラ: `gcc -Wall -Wextra -std=c11 -Iinclude`

## プロジェクト構成

```
include/ftcs.h          # 公開ヘッダ（型定義・マクロ・API すべて）
src/ftcs_parser.c       # ファイルパーサ / レコードセット / 主キー検索
src/ftcs_core.c         # CLI フレームワーク (ftcs_main)
src/ftcs_shm.c          # 共有メモリ（現在はスタブ）
example/
  sample_struct.h       # ユーザ定義構造体のサンプル（FTCS_KEY_FIELD）
  sample_mapping.c      # マッピング定義 + main()
  sample_data.txt       # サンプルデータ（ID フィールドあり）
example2/
  sensor_struct.h       # ID メンバを持たない構造体（FTCS_KEY_INDEX）
  sensor_loader.c       # index_field_name="ID" 設定の main()
  sensor_data.txt       # 1ベース ID フィールドつきセンサーデータ
libftcs.a               # ビルド成果物（静的ライブラリ）
```

## コーディング規約

- C11 (`-std=c11`)
- コンパイラ警告: `-Wall -Wextra`（警告はエラーとして扱う）
- 公開APIのプレフィックス: `ftcs_`
- マッピング定義には必ず `FTCS_MAPPING_BEGIN(name, struct_type)` / `FTCS_FIELD(member, fname)` / `FTCS_MAPPING_END()` マクロを使用
- `FTCS_FIELD` は C11 `_Generic` でメンバ型を自動推論する（手動の型指定は不要）
- **1翻訳単位に 1 マッピングのみ**（`_ftcs_current_t` typedef の衝突を避けるため）
- 内部実装はヘッダに露出させない

## 主要な型とAPI

| シンボル | 定義場所 | 役割 |
|---|---|---|
| `ftcs_field_mapping_t` | `include/ftcs.h` | フィールドマッピングエントリ |
| `ftcs_parser_config_t` | `include/ftcs.h` | パーサ設定（コメント文字・セパレータ・主キー・主キーモード・インデックスフィールド名） |
| `ftcs_primary_key_mode_t` | `include/ftcs.h` | 主キーモード列挙型（`FTCS_KEY_FIELD` / `FTCS_KEY_INDEX`） |
| `ftcs_record_set_t` | `include/ftcs.h` | 動的配列のレコードセット |
| `ftcs_config_t` | `include/ftcs.h` | CLIフレームワーク設定 |
| `FTCS_MAPPING_BEGIN(name, type)` | `include/ftcs.h` | マッピングテーブル開始・`_ftcs_current_t` を typedef |
| `FTCS_FIELD(member, fname)` | `include/ftcs.h` | フィールドエントリ定義（型は `_Generic` で自動推論） |
| `FTCS_INFER_TYPE(member)` | `include/ftcs.h` | C11 `_Generic` によるメンバ型 → `FTCS_TYPE_*` 変換 |
| `FTCS_MAPPING_END()` | `include/ftcs.h` | マッピングテーブル終端 |
| `ftcs_parse_file()` | `src/ftcs_parser.c` | ファイル解析 → `ftcs_record_set_t *` |
| `ftcs_record_set_free()` | `src/ftcs_parser.c` | レコードセット解放 |
| `ftcs_find_by_key()` | `src/ftcs_parser.c` | 主キーフィールドによる線形探索（FTCS_KEY_FIELD 用） |
| `ftcs_find_by_index()` | `src/ftcs_parser.c` | 配列添え字による直接アクセス（FTCS_KEY_INDEX 用、O(1)） |
| `ftcs_main()` | `src/ftcs_core.c` | CLIエントリポイント (`-f`, `-d`, `-k`, `-h`) |
| `shm_addr_get()` | `src/ftcs_shm.c` | 共有メモリ取得（スタブ、常に NULL） |

## データ形式

```
# コメント行（# で始まる）
ID=42 NAME=TestItem VALUE=3.14
ID=7  NAME=Widget   VALUE=9.81
```

- 1行 = 1レコード
- スペース区切りの `KEY=VALUE` ペア
- `#` で始まる行はコメント

## 対応フィールド型

| マクロ | C型 |
|---|---|
| `FTCS_TYPE_INT` | `int` |
| `FTCS_TYPE_LONG` | `long` |
| `FTCS_TYPE_SHORT` | `short` |
| `FTCS_TYPE_FLOAT` | `float` |
| `FTCS_TYPE_DOUBLE` | `double` |
| `FTCS_TYPE_CHAR` | `char` |
| `FTCS_TYPE_STRING` | `char[]`（固定長配列） |

## 主キーモード

`ftcs_parser_config_t.primary_key_mode` で動作を切り替える。

| モード | 値 | 動作 |
|---|---|---|
| `FTCS_KEY_FIELD` | 0（デフォルト） | `primary_key` で指定したフィールド名で線形探索 |
| `FTCS_KEY_INDEX` | 1 | 0ベース添え字で配列に直接アクセス |

### FTCS_KEY_FIELD（主キー ON）

既存動作。`primary_key` に指定したフィールド名でレコードを線形探索する。
`-k <値>` にはフィールドの値を文字列で渡す。

```c
.parser_config = &(ftcs_parser_config_t){
    .comment_char = '#',
    .kv_separator = "=",
    .primary_key  = "ID",               /* 検索対象フィールド名 */
    /* primary_key_mode は省略 → FTCS_KEY_FIELD (0) */
},
```

### FTCS_KEY_INDEX（主キー OFF）

`-k <n>` で 0ベース添え字を指定し、その位置のレコードを返す。

**制約:**
- `-k` には非負整数のみ指定可能
- 主キー（位置指定フィールド）は構造体メンバに含まない
- `-k` の値がレコード数以上の場合はエラー

**`index_field_name` を指定した場合（位置指定フィールドあり）:**

データファイル内の 1ベース整数フィールドを読み取り、各レコードを
`array[value - 1]` の位置に配置する。ファイル内の出現順序は問わない。

```c
.parser_config = &(ftcs_parser_config_t){
    .comment_char     = '#',
    .kv_separator     = "=",
    .primary_key_mode = FTCS_KEY_INDEX,
    .index_field_name = "ID",  /* 1ベース位置フィールド名。構造体メンバには不要 */
},
```

データファイル例（出現順序不問）:
```
ID=3 LOCATION=ServerRoom TEMP=18.0 HUMIDITY=40.0
ID=1 LOCATION=RoomA      TEMP=22.5 HUMIDITY=60.0
ID=2 LOCATION=RoomB      TEMP=25.1 HUMIDITY=55.3
```
→ array[0]=RoomA, array[1]=RoomB, array[2]=ServerRoom の順に格納される。

**`index_field_name` を省略した場合（位置指定フィールドなし）:**

ファイルの行順に 0, 1, 2, … と順次格納する（`index_field_name = NULL`）。

## 注意事項

- `shm_addr_get()` は現在スタブで、常に `NULL` を返す。共有メモリ実装は将来予定。
- `ftcs_record_set_t` を使い終わったら必ず `ftcs_record_set_free()` で解放すること。
- `ftcs_find_by_key()` は線形探索のため、大量レコード時はパフォーマンスに注意。
- `ftcs_find_by_index()` は O(1) だがバウンドチェックあり。
- `index_field_name` を使う場合、IDが連続していない（飛び番）と間のスロットはゼロ初期化される。
