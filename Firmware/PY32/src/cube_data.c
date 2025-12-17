#include "cube_data.h"

#define READ_OR_FAIL(ptr, remaining, out)   \
    do                                      \
    {                                       \
        if ((remaining) < sizeof(out))      \
            goto fail;                      \
        memcpy(&(out), (ptr), sizeof(out)); \
        (ptr) += sizeof(out);               \
        (remaining) -= sizeof(out);         \
    } while (0)

/**
 * Deserializes a cube data packet from a byte buffer.
 * @param data The input byte buffer.
 * @param length The length of the input buffer.
 * @return A pointer to the deserialized cube_data_packet_t, or NULL on failure.
 *        The caller is responsible for freeing the returned packet.
 */
cube_data_packet_t *deserialize_cube_data(const uint8_t *data, uint32_t length)
{
    if (!data || length < 1)
        return NULL;

    const uint8_t *ptr = data;
    uint32_t remaining = length;

    cube_data_packet_t *packet = calloc(1, sizeof(*packet));
    if (!packet)
        return NULL;

    /* --- Read type --- */
    uint8_t raw_type;
    READ_OR_FAIL(ptr, remaining, raw_type);
    packet->type = (data_type_t)raw_type;

    /* --- Parse payload --- */
    switch (packet->type)
    {
    case DATA_TYPE_HALL_SENSOR_DATA:
        READ_OR_FAIL(ptr, remaining, packet->data.hall_data.value);
        break;

    case DATA_TYPE_LED_COLOR:
        READ_OR_FAIL(ptr, remaining, packet->data.led_data.pixels);
        break;

    case DATA_TYPE_REQUEST_HALL_SENSOR_DATA:
        /* no payload */
        break;

    default:
        goto fail;
    }

    /* --- Reject trailing garbage --- */
    if (remaining != 0)
        goto fail;

    return packet;
fail:
    free(packet);
    return NULL;
}

/**
 * Serializes a cube data packet into a byte buffer.
 * @param packet The cube data packet to serialize.
 * @param out_length Pointer to store the length of the output buffer.
 * @return A pointer to the serialized byte buffer, or NULL on failure.
 *        The caller is responsible for freeing the returned buffer.
 */
uint8_t *serialize_cube_data(const cube_data_packet_t *packet, uint32_t *out_length)
{
    if (!packet || !out_length)
        return NULL;

    uint32_t length = 1; // type byte

    switch (packet->type)
    {
    case DATA_TYPE_HALL_SENSOR_DATA:
        length += sizeof(packet->data.hall_data.value);
        break;

    case DATA_TYPE_LED_COLOR:
        length += sizeof(packet->data.led_data.pixels);
        break;

    case DATA_TYPE_REQUEST_HALL_SENSOR_DATA:
        break;

    default:
        return NULL;
    }

    uint8_t *buffer = malloc(length);
    if (!buffer)
        return NULL;

    uint8_t *ptr = buffer;

    /* --- Write type --- */
    uint8_t raw_type = (uint8_t)packet->type;
    memcpy(ptr, &raw_type, sizeof(raw_type));
    ptr += sizeof(raw_type);

    /* --- Write payload --- */
    switch (packet->type)
    {
    case DATA_TYPE_HALL_SENSOR_DATA:
        memcpy(ptr,
               &packet->data.hall_data.value,
               sizeof(packet->data.hall_data.value));
        break;

    case DATA_TYPE_LED_COLOR:
        memcpy(ptr,
               packet->data.led_data.pixels,
               sizeof(packet->data.led_data.pixels));
        break;

    case DATA_TYPE_REQUEST_HALL_SENSOR_DATA:
        break;

    default:
        free(buffer);
        return NULL;
    }

    *out_length = length;
    return buffer;
}

/**
 * Determines if a given data type expects a response.
 * @param type The data type to check.
 * @return 1 if a response is expected, 0 otherwise.
 */
uint8_t data_type_expects_response(data_type_t type)
{
    switch (type)
    {
    case DATA_TYPE_REQUEST_HALL_SENSOR_DATA:
        return 1;

    case DATA_TYPE_HALL_SENSOR_DATA:
    case DATA_TYPE_LED_COLOR:
        return 0;

    default:
        return 0;
    }
}