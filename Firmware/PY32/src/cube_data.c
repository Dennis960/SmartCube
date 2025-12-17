#include "cube_data.h"

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

    uint32_t payload_size = data_type_payload_size(packet->type);

    uint32_t length = 1 + payload_size; // 1 byte for type
    uint8_t *buffer = malloc(length);
    if (!buffer)
        return NULL;

    buffer[0] = (uint8_t)packet->type;

    if (payload_size > 0)
    {
        const void *payload = &packet->data;
        memcpy(buffer + 1, payload, payload_size);
    }

    *out_length = length;
    return buffer;
}

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

    data_type_t type = (data_type_t)data[0];
    uint32_t payload_size = data_type_payload_size(type);
    uint32_t expected_length = 1 + payload_size;

    if (length != expected_length)
        return NULL; // invalid length

    cube_data_packet_t *packet = (cube_data_packet_t *)malloc(sizeof(cube_data_packet_t));
    if (!packet)
        return NULL;

    packet->type = type;
    const uint8_t *ptr = data + 1;

    memcpy(&packet->data, ptr, payload_size);

    return packet;
}
