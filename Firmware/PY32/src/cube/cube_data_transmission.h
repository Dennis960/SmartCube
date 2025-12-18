#ifndef CUBE_DATA_TRANSMISSION_H
#define CUBE_DATA_TRANSMISSION_H

#include "cube_hardware.h"

typedef enum
{
    CUBE_DATA_TRANSMISSION_OK,
    CUBE_DATA_TRANSMISSION_DISCONNECTED,
    CUBE_DATA_TRANSMISSION_ERROR_TIMEOUT,
    CUBE_DATA_TRANSMISSION_ERROR_DESERIALIZATION,
    CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_TIMEOUT,
    CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_RECEIVE_TIMEOUT,
    CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_FINISH_TIMEOUT,
    CUBE_DATA_TRANSMISSION_ERROR_CLOCK_TIMEOUT,
} cube_data_transmission_status_t;

/**
 * For the given cube_side, sets all D1 pins to output low and all D2 pins to input mode, waiting for further instructions.
 * @param cube_side The side of the cube to set idle
 */
static inline void cube_set_side_idle(cube_side_t cube_side)
{
    LL_GPIO_ResetOutputPin(cube_side_to_port1(cube_side), cube_side_to_pin1(cube_side));
    LL_GPIO_SetPinMode(cube_side_to_port1(cube_side), cube_side_to_pin1(cube_side), LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(cube_side_to_port2(cube_side), cube_side_to_pin2(cube_side));
    LL_GPIO_SetPinMode(cube_side_to_port2(cube_side), cube_side_to_pin2(cube_side), LL_GPIO_MODE_INPUT);
}

cube_data_transmission_status_t cube_receive_data(cube_side_t cube_side, uint8_t *data, uint32_t max_length, uint32_t *length_received);
void cube_send_data(cube_side_t cube_side, uint8_t *data, uint32_t length);
cube_data_transmission_status_t cube_init_data_transfer(cube_side_t cube_side);
uint8_t cube_is_connected(cube_side_t cube_side);
cube_data_transmission_status_t cube_handle_disconnect_or_communication_request(cube_side_t cube_side);

#endif // CUBE_DATA_TRANSMISSION_H
