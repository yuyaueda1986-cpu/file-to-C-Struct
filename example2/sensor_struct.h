#ifndef SENSOR_STRUCT_H
#define SENSOR_STRUCT_H

/* Sensor reading record.
 * No index/ID field — records are accessed by array position (FTCS_KEY_INDEX). */
typedef struct {
    char  location[32];
    float temperature;
    float humidity;
} sensor_t;

#endif /* SENSOR_STRUCT_H */
