/*
 * test_ftcs.cpp
 * Google Test suite for libftcs (ftcs_parser.c / ftcs_core.c)
 *
 * Note: FTCS_FIELD / FTCS_INFER_TYPE use C11 _Generic which is not available
 * in C++.  Mapping tables are therefore defined manually (offsetof + explicit
 * type tags) without relying on the convenience macros.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstddef>

extern "C" {
#include "ftcs.h"
}

/* ─────────────────────────────────────────────────────────────
 * Test structs
 * ───────────────────────────────────────────────────────────── */

typedef struct {
    int    id;
    char   name[64];
    double value;
} sample_t;

typedef struct {
    char  location[32];
    float temperature;
    float humidity;
} sensor_t;

typedef struct {
    int    ival;
    long   lval;
    short  sval;
    float  fval;
    double dval;
    char   cval;
    char   strval[32];
} all_types_t;

/* ─────────────────────────────────────────────────────────────
 * Mapping tables (defined manually – C++ cannot use _Generic)
 * ───────────────────────────────────────────────────────────── */

static const ftcs_field_mapping_t sample_mapping[] = {
    { "ID",    offsetof(sample_t, id),    sizeof(int),      FTCS_TYPE_INT    },
    { "NAME",  offsetof(sample_t, name),  sizeof(char[64]), FTCS_TYPE_STRING },
    { "VALUE", offsetof(sample_t, value), sizeof(double),   FTCS_TYPE_DOUBLE },
    { nullptr, 0, 0, FTCS_TYPE_INT }
};

static const ftcs_field_mapping_t sensor_mapping[] = {
    { "LOCATION", offsetof(sensor_t, location),    sizeof(char[32]), FTCS_TYPE_STRING },
    { "TEMP",     offsetof(sensor_t, temperature), sizeof(float),    FTCS_TYPE_FLOAT  },
    { "HUMIDITY", offsetof(sensor_t, humidity),    sizeof(float),    FTCS_TYPE_FLOAT  },
    { nullptr, 0, 0, FTCS_TYPE_INT }
};

static const ftcs_field_mapping_t all_types_mapping[] = {
    { "IVAL",   offsetof(all_types_t, ival),   sizeof(int),      FTCS_TYPE_INT    },
    { "LVAL",   offsetof(all_types_t, lval),   sizeof(long),     FTCS_TYPE_LONG   },
    { "SVAL",   offsetof(all_types_t, sval),   sizeof(short),    FTCS_TYPE_SHORT  },
    { "FVAL",   offsetof(all_types_t, fval),   sizeof(float),    FTCS_TYPE_FLOAT  },
    { "DVAL",   offsetof(all_types_t, dval),   sizeof(double),   FTCS_TYPE_DOUBLE },
    { "CVAL",   offsetof(all_types_t, cval),   sizeof(char),     FTCS_TYPE_CHAR   },
    { "STRVAL", offsetof(all_types_t, strval), sizeof(char[32]), FTCS_TYPE_STRING },
    { nullptr, 0, 0, FTCS_TYPE_INT }
};

/* ─────────────────────────────────────────────────────────────
 * Parser configs
 * ───────────────────────────────────────────────────────────── */

static const ftcs_parser_config_t sample_cfg = {
    '#', "=", "ID", FTCS_KEY_FIELD, nullptr
};
static const ftcs_parser_config_t all_types_cfg = {
    '#', "=", nullptr, FTCS_KEY_FIELD, nullptr
};
static const ftcs_parser_config_t sensor_index_field_cfg = {
    '#', "=", nullptr, FTCS_KEY_INDEX, "ID"
};
static const ftcs_parser_config_t sensor_sequential_cfg = {
    '#', "=", nullptr, FTCS_KEY_INDEX, nullptr
};

/* ─────────────────────────────────────────────────────────────
 * Helper: resolve a path inside test/data/
 * ───────────────────────────────────────────────────────────── */
static std::string data(const char *name)
{
    return std::string(TEST_DATA_DIR) + "/" + name;
}

/* ═════════════════════════════════════════════════════════════
 * Group 1: ftcs_parse_file — 引数バリデーション
 * ═════════════════════════════════════════════════════════════ */

TEST(ParseFile, NullFilepath)
{
    EXPECT_EQ(nullptr, ftcs_parse_file(nullptr, &sample_cfg,
                                       sample_mapping, sizeof(sample_t)));
}

TEST(ParseFile, NullConfig)
{
    EXPECT_EQ(nullptr, ftcs_parse_file(data("basic.txt").c_str(), nullptr,
                                       sample_mapping, sizeof(sample_t)));
}

TEST(ParseFile, NullMapping)
{
    EXPECT_EQ(nullptr, ftcs_parse_file(data("basic.txt").c_str(), &sample_cfg,
                                       nullptr, sizeof(sample_t)));
}

TEST(ParseFile, NonexistentFile)
{
    EXPECT_EQ(nullptr, ftcs_parse_file("/no/such/file.txt", &sample_cfg,
                                       sample_mapping, sizeof(sample_t)));
}

/* ═════════════════════════════════════════════════════════════
 * Group 2: ftcs_parse_file — 基本パース (FTCS_KEY_FIELD)
 * ═════════════════════════════════════════════════════════════ */

class ParseBasic : public ::testing::Test {
protected:
    ftcs_record_set_t *rs = nullptr;
    void SetUp() override {
        rs = ftcs_parse_file(data("basic.txt").c_str(), &sample_cfg,
                             sample_mapping, sizeof(sample_t));
        ASSERT_NE(nullptr, rs);
    }
    void TearDown() override { ftcs_record_set_free(rs); }
};

TEST_F(ParseBasic, RecordCount)
{
    EXPECT_EQ(3u, rs->count);
}

TEST_F(ParseBasic, FirstRecord)
{
    const sample_t *r = static_cast<const sample_t *>(rs->records);
    EXPECT_EQ(42, r[0].id);
    EXPECT_STREQ("TestItem", r[0].name);
    EXPECT_DOUBLE_EQ(3.14, r[0].value);
}

TEST_F(ParseBasic, SecondRecord)
{
    const sample_t *r = static_cast<const sample_t *>(rs->records);
    EXPECT_EQ(7, r[1].id);
    EXPECT_STREQ("Widget", r[1].name);
    EXPECT_DOUBLE_EQ(9.81, r[1].value);
}

TEST_F(ParseBasic, ThirdRecord)
{
    const sample_t *r = static_cast<const sample_t *>(rs->records);
    EXPECT_EQ(100, r[2].id);
    EXPECT_STREQ("Gadget", r[2].name);
    EXPECT_DOUBLE_EQ(2.718, r[2].value);
}

/* ═════════════════════════════════════════════════════════════
 * Group 3: ftcs_parse_file — コメント行・空行スキップ
 * ═════════════════════════════════════════════════════════════ */

TEST(ParseSkip, CommentsAndEmptyLines)
{
    ftcs_record_set_t *rs = ftcs_parse_file(data("comments_empty.txt").c_str(),
                                            &sample_cfg, sample_mapping,
                                            sizeof(sample_t));
    ASSERT_NE(nullptr, rs);
    EXPECT_EQ(2u, rs->count);

    const sample_t *r = static_cast<const sample_t *>(rs->records);
    EXPECT_EQ(1, r[0].id);
    EXPECT_STREQ("Alpha", r[0].name);
    EXPECT_EQ(2, r[1].id);
    EXPECT_STREQ("Beta", r[1].name);

    ftcs_record_set_free(rs);
}

/* ═════════════════════════════════════════════════════════════
 * Group 4: ftcs_parse_file — 全フィールド型
 * ═════════════════════════════════════════════════════════════ */

TEST(ParseAllTypes, FieldValues)
{
    ftcs_record_set_t *rs = ftcs_parse_file(data("all_types.txt").c_str(),
                                            &all_types_cfg, all_types_mapping,
                                            sizeof(all_types_t));
    ASSERT_NE(nullptr, rs);
    ASSERT_EQ(1u, rs->count);

    const all_types_t *r = static_cast<const all_types_t *>(rs->records);
    EXPECT_EQ(42,           r->ival);
    EXPECT_EQ(1234567890L,  r->lval);
    EXPECT_EQ((short)32767, r->sval);
    EXPECT_FLOAT_EQ(1.5f,   r->fval);
    EXPECT_DOUBLE_EQ(3.14159, r->dval);
    EXPECT_EQ('Z',          r->cval);
    EXPECT_STREQ("Hello",   r->strval);

    ftcs_record_set_free(rs);
}

/* ═════════════════════════════════════════════════════════════
 * Group 5: FTCS_KEY_INDEX + index_field_name (順不同配置)
 * ═════════════════════════════════════════════════════════════ */

class ParseIndexField : public ::testing::Test {
protected:
    ftcs_record_set_t *rs = nullptr;
    void SetUp() override {
        rs = ftcs_parse_file(data("index_field.txt").c_str(),
                             &sensor_index_field_cfg, sensor_mapping,
                             sizeof(sensor_t));
        ASSERT_NE(nullptr, rs);
    }
    void TearDown() override { ftcs_record_set_free(rs); }
};

TEST_F(ParseIndexField, RecordCount) { EXPECT_EQ(3u, rs->count); }

TEST_F(ParseIndexField, Index0IsRoomA)
{
    const sensor_t *r = static_cast<const sensor_t *>(rs->records);
    EXPECT_STREQ("RoomA", r[0].location);
    EXPECT_FLOAT_EQ(22.5f, r[0].temperature);
    EXPECT_FLOAT_EQ(60.0f, r[0].humidity);
}

TEST_F(ParseIndexField, Index1IsRoomB)
{
    const sensor_t *r = static_cast<const sensor_t *>(rs->records);
    EXPECT_STREQ("RoomB", r[1].location);
    EXPECT_FLOAT_EQ(25.1f, r[1].temperature);
    EXPECT_FLOAT_EQ(55.3f, r[1].humidity);
}

TEST_F(ParseIndexField, Index2IsServerRoom)
{
    const sensor_t *r = static_cast<const sensor_t *>(rs->records);
    EXPECT_STREQ("ServerRoom", r[2].location);
    EXPECT_FLOAT_EQ(18.0f, r[2].temperature);
    EXPECT_FLOAT_EQ(40.0f, r[2].humidity);
}

/* ═════════════════════════════════════════════════════════════
 * Group 6: FTCS_KEY_INDEX + index_field_name=NULL (順次モード)
 * ═════════════════════════════════════════════════════════════ */

TEST(ParseSequential, OrderPreserved)
{
    ftcs_record_set_t *rs = ftcs_parse_file(data("sequential.txt").c_str(),
                                            &sensor_sequential_cfg,
                                            sensor_mapping, sizeof(sensor_t));
    ASSERT_NE(nullptr, rs);
    EXPECT_EQ(3u, rs->count);

    const sensor_t *r = static_cast<const sensor_t *>(rs->records);
    EXPECT_STREQ("Alpha", r[0].location);
    EXPECT_STREQ("Beta",  r[1].location);
    EXPECT_STREQ("Gamma", r[2].location);

    ftcs_record_set_free(rs);
}

/* ═════════════════════════════════════════════════════════════
 * Group 7: FTCS_KEY_INDEX エラーケース
 * ═════════════════════════════════════════════════════════════ */

TEST(ParseIndexError, IndexZeroRejected)
{
    /* ID=0 は 1-based 制約違反 → NULL が返る */
    ftcs_record_set_t *rs = ftcs_parse_file(data("bad_index.txt").c_str(),
                                            &sensor_index_field_cfg,
                                            sensor_mapping, sizeof(sensor_t));
    EXPECT_EQ(nullptr, rs);
}

TEST(ParseIndexError, MissingIndexField)
{
    /* index_field_name が必須なのにフィールドがない行 → NULL が返る */
    ftcs_record_set_t *rs = ftcs_parse_file(
        data("missing_index_field.txt").c_str(),
        &sensor_index_field_cfg, sensor_mapping, sizeof(sensor_t));
    EXPECT_EQ(nullptr, rs);
}

/* ═════════════════════════════════════════════════════════════
 * Group 8: ftcs_find_by_key
 * ═════════════════════════════════════════════════════════════ */

class FindByKey : public ::testing::Test {
protected:
    ftcs_record_set_t *rs = nullptr;
    void SetUp() override {
        rs = ftcs_parse_file(data("basic.txt").c_str(), &sample_cfg,
                             sample_mapping, sizeof(sample_t));
        ASSERT_NE(nullptr, rs);
    }
    void TearDown() override { ftcs_record_set_free(rs); }
};

TEST_F(FindByKey, FindExistingIntKey)
{
    const void *rec = ftcs_find_by_key(rs, sample_mapping, "ID", "42",
                                       sizeof(sample_t));
    ASSERT_NE(nullptr, rec);
    const sample_t *s = static_cast<const sample_t *>(rec);
    EXPECT_EQ(42, s->id);
    EXPECT_STREQ("TestItem", s->name);
}

TEST_F(FindByKey, FindSecondRecord)
{
    const void *rec = ftcs_find_by_key(rs, sample_mapping, "ID", "7",
                                       sizeof(sample_t));
    ASSERT_NE(nullptr, rec);
    EXPECT_EQ(7, static_cast<const sample_t *>(rec)->id);
}

TEST_F(FindByKey, FindThirdRecord)
{
    const void *rec = ftcs_find_by_key(rs, sample_mapping, "ID", "100",
                                       sizeof(sample_t));
    ASSERT_NE(nullptr, rec);
    EXPECT_EQ(100, static_cast<const sample_t *>(rec)->id);
}

TEST_F(FindByKey, KeyNotFound)
{
    EXPECT_EQ(nullptr, ftcs_find_by_key(rs, sample_mapping, "ID", "999",
                                        sizeof(sample_t)));
}

TEST_F(FindByKey, NullRecordSet)
{
    EXPECT_EQ(nullptr, ftcs_find_by_key(nullptr, sample_mapping, "ID", "42",
                                        sizeof(sample_t)));
}

TEST_F(FindByKey, NullMapping)
{
    EXPECT_EQ(nullptr, ftcs_find_by_key(rs, nullptr, "ID", "42",
                                        sizeof(sample_t)));
}

TEST_F(FindByKey, NullKeyName)
{
    EXPECT_EQ(nullptr, ftcs_find_by_key(rs, sample_mapping, nullptr, "42",
                                        sizeof(sample_t)));
}

TEST_F(FindByKey, NullKeyValue)
{
    EXPECT_EQ(nullptr, ftcs_find_by_key(rs, sample_mapping, "ID", nullptr,
                                        sizeof(sample_t)));
}

TEST_F(FindByKey, UnknownFieldName)
{
    /* マッピングに存在しないフィールド名 → NULL */
    EXPECT_EQ(nullptr, ftcs_find_by_key(rs, sample_mapping, "NOSUCHFIELD",
                                        "42", sizeof(sample_t)));
}

/* ═════════════════════════════════════════════════════════════
 * Group 9: ftcs_find_by_index
 * ═════════════════════════════════════════════════════════════ */

class FindByIndex : public ::testing::Test {
protected:
    ftcs_record_set_t *rs = nullptr;
    void SetUp() override {
        rs = ftcs_parse_file(data("sequential.txt").c_str(),
                             &sensor_sequential_cfg,
                             sensor_mapping, sizeof(sensor_t));
        ASSERT_NE(nullptr, rs);
    }
    void TearDown() override { ftcs_record_set_free(rs); }
};

TEST_F(FindByIndex, Index0)
{
    const void *rec = ftcs_find_by_index(rs, "0", sizeof(sensor_t));
    ASSERT_NE(nullptr, rec);
    EXPECT_STREQ("Alpha", static_cast<const sensor_t *>(rec)->location);
}

TEST_F(FindByIndex, Index1)
{
    const void *rec = ftcs_find_by_index(rs, "1", sizeof(sensor_t));
    ASSERT_NE(nullptr, rec);
    EXPECT_STREQ("Beta", static_cast<const sensor_t *>(rec)->location);
}

TEST_F(FindByIndex, Index2)
{
    const void *rec = ftcs_find_by_index(rs, "2", sizeof(sensor_t));
    ASSERT_NE(nullptr, rec);
    EXPECT_STREQ("Gamma", static_cast<const sensor_t *>(rec)->location);
}

TEST_F(FindByIndex, OutOfRange)
{
    EXPECT_EQ(nullptr, ftcs_find_by_index(rs, "3", sizeof(sensor_t)));
}

TEST_F(FindByIndex, NegativeIndex)
{
    EXPECT_EQ(nullptr, ftcs_find_by_index(rs, "-1", sizeof(sensor_t)));
}

TEST_F(FindByIndex, NonIntegerKey)
{
    EXPECT_EQ(nullptr, ftcs_find_by_index(rs, "abc", sizeof(sensor_t)));
}

TEST_F(FindByIndex, NullRecordSet)
{
    EXPECT_EQ(nullptr, ftcs_find_by_index(nullptr, "0", sizeof(sensor_t)));
}

TEST_F(FindByIndex, NullKeyValue)
{
    EXPECT_EQ(nullptr, ftcs_find_by_index(rs, nullptr, sizeof(sensor_t)));
}

/* ═════════════════════════════════════════════════════════════
 * Group 10: ftcs_record_set_free — 安全性
 * ═════════════════════════════════════════════════════════════ */

TEST(RecordSetFree, NullIsNoOp)
{
    /* クラッシュしないこと */
    ftcs_record_set_free(nullptr);
}

TEST(RecordSetFree, ValidPointerFreed)
{
    ftcs_record_set_t *rs = ftcs_parse_file(data("basic.txt").c_str(),
                                            &sample_cfg, sample_mapping,
                                            sizeof(sample_t));
    ASSERT_NE(nullptr, rs);
    ftcs_record_set_free(rs); /* クラッシュ・二重解放がなければ OK */
}

/* ═════════════════════════════════════════════════════════════
 * Group 11: shm コピーロジック検証
 * (ftcs_core.c の memcpy 処理と同等のロジックをここで直接テスト)
 * ═════════════════════════════════════════════════════════════ */

TEST(ShmCopy, FullCopyToBuffer)
{
    ftcs_record_set_t *rs = ftcs_parse_file(data("basic.txt").c_str(),
                                            &sample_cfg, sample_mapping,
                                            sizeof(sample_t));
    ASSERT_NE(nullptr, rs);

    sample_t shm_buf[3] = {};
    size_t shm_size = sizeof(shm_buf);
    size_t bytes = rs->count * rs->struct_size;
    if (bytes > shm_size) bytes = shm_size;
    memcpy(shm_buf, rs->records, bytes);

    EXPECT_EQ(42,  shm_buf[0].id);
    EXPECT_STREQ("TestItem", shm_buf[0].name);
    EXPECT_EQ(7,   shm_buf[1].id);
    EXPECT_EQ(100, shm_buf[2].id);

    ftcs_record_set_free(rs);
}

TEST(ShmCopy, TruncatedWhenBufferSmall)
{
    /* shm 領域が 1 レコード分しかない場合、先頭レコードのみコピーされる */
    ftcs_record_set_t *rs = ftcs_parse_file(data("basic.txt").c_str(),
                                            &sample_cfg, sample_mapping,
                                            sizeof(sample_t));
    ASSERT_NE(nullptr, rs);

    sample_t shm_buf[3] = {};
    size_t shm_size = sizeof(sample_t); /* 1 レコード分のみ */
    size_t bytes = rs->count * rs->struct_size;
    if (bytes > shm_size) bytes = shm_size;
    memcpy(shm_buf, rs->records, bytes);

    EXPECT_EQ(42, shm_buf[0].id);
    /* 2 番目以降はゼロ初期化のまま */
    EXPECT_EQ(0,  shm_buf[1].id);

    ftcs_record_set_free(rs);
}
