#ifndef CUBE_DATA_TRANSMISSION_H
#define CUBE_DATA_TRANSMISSION_H

#include "cube_hardware.h"

typedef enum
{
    CUBE_OK,
    CUBE_DISCONNECTED,
    CUBE_ERROR_TIMEOUT,
    CUBE_ERROR_DESERIALIZATION,
    CUBE_ERROR_ACKNOWLEDGE_TIMEOUT,
    CUBE_ERROR_ACKNOWLEDGE_RECEIVE_TIMEOUT,
    CUBE_ERROR_ACKNOWLEDGE_FINISH_TIMEOUT,
    CUBE_ERROR_CLOCK_TIMEOUT,
} cube_status_t;

void cube_set_side_idle(cube_side_t cube_side);
cube_status_t cube_receive_data(cube_side_t cube_side, uint8_t *data, uint32_t max_length, uint32_t *length_received);
void cube_send_data(cube_side_t cube_side, uint8_t *data, uint32_t length);
cube_status_t cube_init_data_transfer(cube_side_t cube_side);
uint8_t cube_is_connected(cube_side_t cube_side);
cube_status_t cube_handle_disconnect_or_communication_request(cube_side_t cube_side);

#endif // CUBE_DATA_TRANSMISSION_H