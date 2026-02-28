# libftcs — Google Test 試験仕様・結果

## 試験環境

| 項目 | 内容 |
|---|---|
| OS | Linux (WSL2, kernel 6.6.87.2-microsoft-standard-WSL2) |
| コンパイラ | g++ (C++17) / gcc (C11) |
| テストフレームワーク | Google Test 1.14.0 (`libgtest-dev`) |
| テストバイナリ | `test/test_ftcs` |
| ビルドコマンド | `make test` |

---

## テスト構成

```
test/
├── test_ftcs.cpp        # テストコード（C++17 + gtest）
└── data/
    ├── basic.txt            # 3 レコード（int/string/double）
    ├── all_types.txt        # 全フィールド型 1 レコード
    ├── comments_empty.txt   # コメント行・空行を含む 2 レコード
    ├── index_field.txt      # 順不同 ID付き 3 レコード（FTCS_KEY_INDEX）
    ├── sequential.txt       # 3 レコード（順次モード、FTCS_KEY_INDEX）
    ├── bad_index.txt        # ID=0 の不正データ（エラーケース）
    └── missing_index_field.txt  # index_field_name フィールドが欠如した行
```

> **注意:** `FTCS_FIELD` / `FTCS_INFER_TYPE` マクロは C11 `_Generic` を使用するため
> C++ コンパイラでは利用不可。テストコード内のマッピングテーブルは
> `offsetof` と明示的な型タグ (`FTCS_TYPE_*`) で手動定義している。

### テストコードの構成方針

`test_ftcs.cpp` はライブラリ本体と同じコーディング規約に従って記述されている。

| 規約 | 内容 |
|---|---|
| コメント | すべて日本語。セクション区切りに `══` 罫線を使用 |
| `data()` ヘルパー | ファイル先頭に前方宣言、定義はテストグループの後（トップダウン配置） |
| Doxygen | `data()` ヘルパーに `@brief` / `@param` / `@return` を付与 |
| 波括弧 | `if` の本体が1行でも `{}` を付ける（`ShmCopy` テスト内の切り詰め処理など） |

```
/* ── 関数宣言（目次） ── */
static std::string data(const char *name);

/* グループ1〜11: テストケース群（data() の呼び出し側） */
TEST(...)  { ... data("basic.txt") ... }
...

/* ── ヘルパー（定義は末尾） ── */
/** @brief test/data/ 内のパスを解決する */
static std::string data(const char *name) { ... }
```

---

## 試験一覧と結果

### Group 1: `ftcs_parse_file` — 引数バリデーション（4 件）

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `ParseFile.NullFilepath` | `filepath = NULL` を渡す | `NULL` が返る | PASS |
| `ParseFile.NullConfig` | `config = NULL` を渡す | `NULL` が返る | PASS |
| `ParseFile.NullMapping` | `mapping = NULL` を渡す | `NULL` が返る | PASS |
| `ParseFile.NonexistentFile` | 存在しないファイルパス | `NULL` が返る | PASS |

---

### Group 2: `ftcs_parse_file` — 基本パース / FTCS_KEY_FIELD（4 件）

テストデータ: `test/data/basic.txt`（3 レコード: ID=42, 7, 100）

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `ParseBasic.RecordCount` | レコード数が正しいか | `count == 3` | PASS |
| `ParseBasic.FirstRecord` | 先頭レコードの各フィールド | `id=42, name="TestItem", value=3.14` | PASS |
| `ParseBasic.SecondRecord` | 2番目レコード | `id=7, name="Widget", value=9.81` | PASS |
| `ParseBasic.ThirdRecord` | 3番目レコード | `id=100, name="Gadget", value=2.718` | PASS |

---

### Group 3: `ftcs_parse_file` — コメント行・空行スキップ（1 件）

テストデータ: `test/data/comments_empty.txt`（コメント/空行を含む 2 レコード）

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `ParseSkip.CommentsAndEmptyLines` | `#` 行・空行が除外される | `count == 2`, `id=1,2` の順 | PASS |

---

### Group 4: `ftcs_parse_file` — 全フィールド型（1 件）

テストデータ: `test/data/all_types.txt`（int/long/short/float/double/char/string 各 1 フィールド）

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `ParseAllTypes.FieldValues` | 全型が正しく変換される | `ival=42, lval=1234567890, sval=32767, fval=1.5, dval=3.14159, cval='Z', strval="Hello"` | PASS |

---

### Group 5: `FTCS_KEY_INDEX` + `index_field_name`（順不同配置）（4 件）

テストデータ: `test/data/index_field.txt`（ID=3,1,2 の順で記述）

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `ParseIndexField.RecordCount` | レコード数が正しいか | `count == 3` | PASS |
| `ParseIndexField.Index0IsRoomA` | ID=1 が `array[0]` に配置される | `location="RoomA"` | PASS |
| `ParseIndexField.Index1IsRoomB` | ID=2 が `array[1]` に配置される | `location="RoomB"` | PASS |
| `ParseIndexField.Index2IsServerRoom` | ID=3 が `array[2]` に配置される | `location="ServerRoom"` | PASS |

---

### Group 6: `FTCS_KEY_INDEX` + `index_field_name=NULL`（順次モード）（1 件）

テストデータ: `test/data/sequential.txt`（3 レコードを出現順に格納）

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `ParseSequential.OrderPreserved` | 行順に `array[0..2]` へ格納される | `Alpha, Beta, Gamma` の順 | PASS |

---

### Group 7: `FTCS_KEY_INDEX` エラーケース（2 件）

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `ParseIndexError.IndexZeroRejected` | `ID=0`（1-based 制約違反）の行 | `NULL` が返る | PASS |
| `ParseIndexError.MissingIndexField` | `index_field_name` に指定したフィールドが行に存在しない | `NULL` が返る | PASS |

---

### Group 8: `ftcs_find_by_key`（9 件）

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `FindByKey.FindExistingIntKey` | `ID=42` で検索 | 正しいレコードへのポインタ | PASS |
| `FindByKey.FindSecondRecord` | `ID=7` で検索 | 正しいレコードへのポインタ | PASS |
| `FindByKey.FindThirdRecord` | `ID=100` で検索 | 正しいレコードへのポインタ | PASS |
| `FindByKey.KeyNotFound` | 存在しない `ID=999` | `NULL` が返る | PASS |
| `FindByKey.NullRecordSet` | `rs = NULL` を渡す | `NULL` が返る | PASS |
| `FindByKey.NullMapping` | `mapping = NULL` を渡す | `NULL` が返る | PASS |
| `FindByKey.NullKeyName` | `primary_key_name = NULL` を渡す | `NULL` が返る | PASS |
| `FindByKey.NullKeyValue` | `key_value = NULL` を渡す | `NULL` が返る | PASS |
| `FindByKey.UnknownFieldName` | マッピングに存在しないフィールド名 | `NULL` が返る | PASS |

---

### Group 9: `ftcs_find_by_index`（8 件）

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `FindByIndex.Index0` | インデックス `0` を取得 | `location="Alpha"` | PASS |
| `FindByIndex.Index1` | インデックス `1` を取得 | `location="Beta"` | PASS |
| `FindByIndex.Index2` | インデックス `2` を取得 | `location="Gamma"` | PASS |
| `FindByIndex.OutOfRange` | `count == 3` に対し `"3"` を指定 | `NULL` が返る | PASS |
| `FindByIndex.NegativeIndex` | `"-1"` を指定 | `NULL` が返る | PASS |
| `FindByIndex.NonIntegerKey` | `"abc"` を指定 | `NULL` が返る | PASS |
| `FindByIndex.NullRecordSet` | `rs = NULL` を渡す | `NULL` が返る | PASS |
| `FindByIndex.NullKeyValue` | `key_value = NULL` を渡す | `NULL` が返る | PASS |

---

### Group 10: `ftcs_record_set_free` — 安全性（2 件）

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `RecordSetFree.NullIsNoOp` | `NULL` を渡しても無害 | クラッシュなし | PASS |
| `RecordSetFree.ValidPointerFreed` | 有効なポインタを解放 | クラッシュ・メモリエラーなし | PASS |

---

### Group 11: shm コピーロジック検証（2 件）

`ftcs_core.c` の `memcpy` 処理と同等のロジックをテストコード内で直接検証。

| テスト名 | 試験内容 | 期待値 | 結果 |
|---|---|---|---|
| `ShmCopy.FullCopyToBuffer` | 全レコードをバッファへコピー | `id=42, 7, 100` がバッファに反映 | PASS |
| `ShmCopy.TruncatedWhenBufferSmall` | バッファが 1 レコード分のみ | 先頭レコードのみコピー、残りはゼロ | PASS |

---

## 総合結果

```
[==========] 38 tests from 11 test suites ran.
[  PASSED  ] 38 tests.
[  FAILED  ] 0 tests.
```

**全 38 件 PASSED / 失敗 0 件**

---

## 実行方法

```bash
# ライブラリ + テストバイナリのビルドと実行
make test

# テストバイナリを単体で実行（フィルタや詳細オプションも利用可）
./test/test_ftcs
./test/test_ftcs --gtest_filter="FindByKey.*"
./test/test_ftcs --gtest_list_tests
```
