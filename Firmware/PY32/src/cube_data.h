#ifndef CUBE_DATA_H
#define CUBE_DATA_H
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef enum
{
    DATA_TYPE_REQUEST_HALL_SENSOR_DATA,
    DATA_TYPE_REQUEST_POSITION_DATA,
    DATA_TYPE_HALL_SENSOR_DATA,
    DATA_TYPE_LED_COLOR,
    DATA_TYPE_POSITION_DATA
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
    int8_t origin_x;
    int8_t origin_y;
    int8_t x;
    int8_t y;
} position_data_t;
typedef struct
{
    data_type_t type; // type of the data
    union
    {
        hall_sensor_data_t hall_data;
        led_color_data_t led_data;
        position_data_t position_data;
    } data; // payload
} cube_data_packet_t;

inline uint32_t data_type_payload_size(data_type_t type)
{
    switch (type)
    {
    case DATA_TYPE_HALL_SENSOR_DATA:
        return sizeof(hall_sensor_data_t);
    case DATA_TYPE_LED_COLOR:
        return sizeof(led_color_data_t);
    case DATA_TYPE_POSITION_DATA:
        return sizeof(position_data_t);
    case DATA_TYPE_REQUEST_HALL_SENSOR_DATA:
    case DATA_TYPE_REQUEST_POSITION_DATA:
    default:
        return 0; // no payload
    }
}

/**
 * Determines if a given data type expects a response.
 * @param type The data type to check.
 * @return 1 if a response is expected, 0 otherwise.
 */
inline uint8_t data_type_expects_response(data_type_t type)
{
    switch (type)
    {
    case DATA_TYPE_REQUEST_HALL_SENSOR_DATA:
    case DATA_TYPE_REQUEST_POSITION_DATA:
        return 1;

    case DATA_TYPE_HALL_SENSOR_DATA:
    case DATA_TYPE_LED_COLOR:
    case DATA_TYPE_POSITION_DATA:
    default:
        return 0;
    }
}

cube_data_packet_t *deserialize_cube_data(const uint8_t *data, uint32_t length);
uint8_t *serialize_cube_data(const cube_data_packet_t *packet, uint32_t *out_length);
#endif // CUBE_DATA_H