#ifndef CUBE_DATA_H
#define CUBE_DATA_H
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef enum
{
    DATA_TYPE_REQUEST_HALL_SENSOR_DATA,
    DATA_TYPE_HALL_SENSOR_DATA,
    DATA_TYPE_LED_COLOR,
} data_type_t;

typedef struct
{
    float value;
} hall_sensor_data_t;
typedef struct
{
    uint8_t pixels[4][3]; // 4 pixels, RGB each
} led_color_data_t;
typedef struct
{
    data_type_t type; // type of the data
    union
    {
        hall_sensor_data_t hall_data;
        led_color_data_t led_data;
    } data; // payload
} cube_data_packet_t;

cube_data_packet_t *deserialize_cube_data(const uint8_t *data, uint32_t length);
uint8_t *serialize_cube_data(const cube_data_packet_t *packet, uint32_t *out_length);
uint8_t data_type_expects_response(data_type_t type);
#endif // CUBE_DATA_H