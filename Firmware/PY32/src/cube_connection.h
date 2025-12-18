#ifndef CUBE_CONNECTION_H
#define CUBE_CONNECTION_H

#include "cube_hardware.h"

typedef enum
{
    CUBE_CONNECTION_OK,
    CUBE_CONNECTION_ERROR_ANNOUNCE_TIMEOUT,
    CUBE_CONNECTION_ERROR_ACKNOWLEDGE_PRESENCE_TIMEOUT,
    CUBE_CONNECTION_ERROR_INIT_HANDSHAKE_TIMEOUT,
    CUBE_CONNECTION_ERROR_ACKNOWLEDGE_HANDSHAKE_TIMEOUT,
    CUBE_CONNECTION_ERROR_COMPLETE_HANDSHAKE_TIMEOUT,
} cube_connection_status_t;

typedef void (*cube_connection_error_callback_t)(cube_side_t cube_side, cube_connection_status_t error_code);

/**
 * Set the pins of the given cube side to disconnected state (both high, d1 as output, d2 as input).
 * @param cube_side The side of the cube to set as disconnected
 */
static inline void cube_set_side_disconnected(cube_side_t cube_side)
{
    GPIO_TypeDef *port1 = cube_side_to_port1(cube_side);
    uint32_t pin1 = cube_side_to_pin1(cube_side);
    GPIO_TypeDef *port2 = cube_side_to_port2(cube_side);
    uint32_t pin2 = cube_side_to_pin2(cube_side);

    LL_GPIO_SetOutputPin(port1, pin1);
    LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(port2, pin2);
    LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_INPUT);
}

void cube_set_connection_error_callback(cube_connection_error_callback_t callback);
uint8_t cube_connection_announce_presence(cube_side_t cube_side);
uint8_t cube_connection_respond_to_announcement(cube_side_t cube_side);

#endif // CUBE_CONNECTION_H
