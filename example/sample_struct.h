#ifndef SAMPLE_STRUCT_H
#define SAMPLE_STRUCT_H

/**
 * @brief サンプルレコード1件を表す構造体
 */
typedef struct {
    int    id;       /**< レコードの識別子 */
    char   name[64]; /**< レコードの名称 */
    double value;    /**< レコードに関連付けられた数値 */
} sample_t;

#endif /* SAMPLE_STRUCT_H */
