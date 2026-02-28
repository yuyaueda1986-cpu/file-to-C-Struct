#ifndef SENSOR_STRUCT_H
#define SENSOR_STRUCT_H

/**
 * @brief センサーの1計測レコードを表す構造体
 *
 * ID フィールドを持たず、配列の添字（FTCS_KEY_INDEX）でレコードを識別する。
 */
typedef struct {
    char  location[32]; /**< 計測場所の名称 */
    float temperature;  /**< 温度 [℃] */
    float humidity;     /**< 湿度 [%] */
} sensor_t;

#endif /* SENSOR_STRUCT_H */
