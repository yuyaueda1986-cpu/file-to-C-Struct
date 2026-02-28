# CLAUDE.md

> 目的: モダンなC言語開発環境における、保守性・可読性の高いコードを書くための規約集。
> 「優れた名前こそ最高のドキュメントである」考えで「良い物語を語る」コードを目指す。

---

## プロジェクト概要

このファイルはClaudeがコードを生成・編集する際に従うべきルールと規約を定義します。

- **言語**: C11（`-std=c11`）
- **ターゲット**: Linux x86_64
- **ビルドシステム**: make
- **テストフレームワーク**: Google Test（gtest）

---

## ビルド・ツール

### ビルドコマンド

```bash
# ビルド
make

# クリーンビルド
make clean && make

# テスト実行
make test

# 静的解析
make lint        # clang-tidy

# フォーマット
make format      # clang-format
```

### コンパイラフラグ

```makefile
CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Werror
CFLAGS += -g -O2
CFLAGS += -D_POSIX_C_SOURCE=200809L
```

### ドキュメント生成
- **Doxygen** を使用してドキュメントを生成する
- すべての関数・構造体・変数に Doxygen 形式のコメントを付与する
- コメントは **日本語** で記述する

```c
/**
 * @brief 2つの整数を加算する
 * @param a 加算する整数の左辺値
 * @param b 加算する整数の右辺値
 * @return 加算結果
 */
int add(int a, int b);
```

### テスト
- **Google Test (gtest)** を使用して単体テストを記述する
- テストファイルは `test/` ディレクトリに `test_<モジュール名>.cpp` の命名規則で配置する
- ハードウェア依存部分はモック（`test/mocks/`）を使用する

```cpp
// test/test_sensor.cpp の例
#include <gtest/gtest.h>

extern "C" {
#include "sensor.h"  // C ヘッダーを C++ から呼ぶ
}

TEST(SensorTest, ReadReturnsValidRange) {
    float val = read_adc(0);
    EXPECT_GE(val, 0.0f);
    EXPECT_LE(val, 3.3f);  // VCC = 3.3V のため上限は 3.3V
}
```

---

## ディレクトリ構成

```
project/
├── src/           # ソースファイル (.c)
├── include/       # ヘッダーファイル (.h)
├── test/          # Google Test によるテストコード (.cpp)
├── docs/          # Doxygen 生成ドキュメント出力先
├── build/         # ビルド成果物（git管理外）
├── Makefile
├── Doxyfile       # Doxygen 設定ファイル
└── CLAUDE.md
```

---

## コーディング規約

### 命名規則

「優れた名前こそ最高のドキュメントである」という原則のもと、以下のルールを守ること。

| 対象 | 規則 | 例 |
|---|---|---|
| 変数・関数 | snake_case | `read_sensor`, `buf_size` |
| 定数・マクロ | ALL_CAPS | `MAX_BUF_SIZE`, `TIMEOUT_MS` |
| 構造体タグ | snake_case | `struct sensor_data` |
| typedef 型名 | snake_case + `_t` | `sensor_data_t` |

```c
// ✅ 良い例
int user_count = 0;
void calculate_total_price(void);
typedef struct order_manager order_manager_t;

// ❌ 悪い例
int userCount = 0;
void calculateTotalPrice();
typedef struct OrderManager OrderManager;
```

### 変数名・関数名の長さ

- 原則 **2〜9文字** を目標とする
  - **第一優先**: 読者に意図が伝わること
  - **第二優先**: 出現頻度が高い変数は短く
- **1文字を許可するケース**（慣習的に意味が自明なもののみ）:
  - ループカウンタ: `i`, `j`, `k`
  - 座標: `x`, `y`, `z`
  - 汎用文字: `c`, `n`
- 10文字以上になる場合は略語を検討する
  - 使用可能な略語: `len`, `buf`, `cnt`, `idx`, `ptr`, `tmp`, `err`, `max`, `min`

```c
// Good
int   len;        // 文字列長
int   retry_cnt;  // リトライ回数
float avg_temp;   // 平均温度
void  read_adc(); // ADC値読み取り

// Bad
int   l;                          // 何の長さか不明
int   the_number_of_retry_count;  // 冗長すぎる
float t;                          // 温度か時間か不明
```

### 関数の長さ
- **1関数あたり200行以内** に収める
- 200行を超える場合は適切に関数を分割する

---

## コードスタイル

### 波括弧（ブレース）

`if` / `else if` / `else` / `for` / `while` の本体が **1行であっても必ず `{}` を使用する**。

```c
// ✅ 良い例
if (is_valid) {
    process();
}

// ❌ 悪い例
if (is_valid)
    process();
```

### ネストの深いコード

- `if` / `for` のネストが **4段以上** になる箇所には、処理全体の意図を説明するコメントを必ず追記する
- コメントはネストブロックの**直前**に記述し、「なぜこの構造が必要か」を説明する
- ネストが4段以上になる場合は、まず**関数分割でネストを減らせないか**を検討すること

```c
// パケット種別ごとにチャンネルを走査し、有効なエントリのみ更新する。
// 外側2重ループはデバイス×パケット種別、内側2重ループはチャンネル×サンプル。
for (int dev = 0; dev < dev_cnt; dev++) {
    for (int pkt = 0; pkt < PKT_TYPE_NUM; pkt++) {
        for (int ch = 0; ch < CH_NUM; ch++) {
            for (int s = 0; s < sample_cnt; s++) {
                if (entry[dev][pkt][ch][s].valid) {
                    update_entry(&entry[dev][pkt][ch][s]);
                }
            }
        }
    }
}
```

---

## コメント規約

> 原則: **「良い物語を語る」**
> コードは *what*（何をしているか）を語る。コメントは *why*（なぜそうするか）を語る。
> この二つが揃って、初めてコードは読者に物語を伝える。

### 基本ルール

- コメントは**日本語**で書く
- Doxygen コメント（`/** */`）を関数・構造体に使用する
- インラインコメントは `//` を使用する
- コメントは**コードの複製ではなく補完**である
  - コードを読めば分かることを繰り返さない
  - コードから読み取れない意図・背景・制約を伝える

```c
// Bad（コードの複製）
i++;  // i をインクリメントする

// Good（コードの補完）
i++;  // 先頭要素はヘッダーのため読み飛ばす
```

### Why を語る

以下のような問いに答えるコメントが「良いコメント」である。

- なぜこのアルゴリズムを選んだか
- なぜこの定数値なのか
- なぜ一見おかしく見える実装をしているのか
- どんな前提条件・制約のもとで動くコードか

```c
// Bad（what の繰り返し）
timeout_ms = 200;  // タイムアウトを200msに設定する

// Good（why を説明）
timeout_ms = 200;  // センサーの応答仕様上限が180ms のため、余裕を持たせた値
```

```c
// Good（一見奇妙な実装の理由を説明）
val &= ~0x01;  // LSB はハードウェアの都合で常に不定のため、読み捨てる
```

### 区切りとしての役割

良いコメントは句読点のように、コードのまとまりを区切る。
処理のブロックが切り替わる箇所にコメントを置き、読者に「段落」を感じさせる。

```c
// --- 入力値の検証 ---
if (len == 0 || buf == NULL) {
    return ERR_INVALID_ARG;
}

// --- バッファを初期化してデータを読み込む ---
memset(buf, 0, len);
read_sensor(buf, len);

// --- チェックサムを検証して結果を返す ---
if (!verify_checksum(buf, len)) {
    return ERR_CHECKSUM;
}
return OK;
```

### コメントを書くべき場所

| 場面 | 書くべき内容 |
|---|---|
| 変数・関数の宣言 | その意味・役割を日本語で簡潔に |
| 定数・マジックナンバー | その値の根拠・出典 |
| アルゴリズムの選択 | なぜこの手法か |
| ネスト4段以上の複雑なブロック | 処理全体の意図と構造の説明 |
| 一見おかしく見える実装 | 意図的である旨と理由 |

### 書いてはいけないコメント

```c
// Bad: コードを日本語に翻訳しただけ
for (int i = 0; i < len; i++) {  // len 回ループする

// Bad: 自明すぎる
is_ready = true;  // is_ready を true にする

// Bad: コードと矛盾した嘘コメント
retry_cnt = 5;  // 3回リトライする  ← 実際は5回
```

> 嘘のコメントはコメントがないより有害である。
> コードを修正したら、必ずコメントも一緒に見直すこと。

### 宣言時コメントの書き方

```c
int   retry_max;   // 通信失敗時の最大リトライ回数
float vref;        // ADC 基準電圧 [V]
bool  is_ready;    // 初期化完了フラグ（true: 使用可能）
```

```c
/**
 * @brief ユーザーの注文リストを処理する構造体
 */
typedef struct order_processor {
    int retry_count; /**< 処理失敗時のリトライ回数 */
} order_processor_t;

/**
 * @brief 注文を処理してデータベースに保存する
 * @param proc 処理対象のorder_processor
 * @param order_id 処理対象の注文ID
 * @return 処理が成功した場合は 0、失敗した場合は -1
 */
int process_order(order_processor_t *proc, int order_id);
```

---

## Static関数の配置ルール

### 原則：トップダウン（概要→詳細）

static 関数は**呼び出す側を上、呼び出される側を下**に配置する。
読者がファイルの先頭から「概要→詳細」の順で物語を読めるようにするためである。

### 前方宣言：ファイル先頭にまとめて宣言する

前方宣言の一覧はそのファイルの「関数の目次」として機能する。

```c
// --- 関数宣言（目次） ---
static int fna(void);   // 上位処理
static int fnb(void);   // 中位処理
static int fnc(void);   // 末端処理

// --- 関数定義（概要→詳細の順） ---
static int main(void) { return fna(); }
static int fna(void)  { return fnb(); }
static int fnb(void)  { return fnc(); }
static int fnc(void)  { return 0; }
```

コンパイルエラーを避けるためだけにボトムアップ順に並べない。
「コンパイラのために書かれたコード」ではなく「読者のために書かれたコード」を目指す。

---

## 一般原則

> コードは書く回数より**読まれる回数の方が圧倒的に多い**。
> すべての判断基準は「コンパイラが通るか」ではなく「読者が自然に読めるか」である。

- `malloc` / `free` を使う場合は必ず対になっていることをコメントで明示する
- マジックナンバーは必ず名前付き定数（`#define` または `const`）にする
- 新規ファイルを追加した場合は `Makefile` の対象も必ず更新する
- コードを修正したら、関連するコメントも必ず見直す

---

## チェックリスト（コード生成時）

- [ ] C11 の機能・構文を使用しているか
- [ ] すべての識別子が snake_case か
- [ ] 関数が 200 行以内か
- [ ] `if` / `else` に必ず `{}` が付いているか
- [ ] 関数・変数に日本語の Doxygen コメントがあるか
- [ ] テストコードが gtest で記述されているか
