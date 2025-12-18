#include "cube_connection.h"

#define ANNOUNCEMENT_TIMEOUT 10000 // Wait for a response to the announcement
#define HANDSHAKE_TIMEOUT 20       // Timeout for each step in the handshake process

static cube_connection_error_callback_t error_callback = NULL;

void cube_set_connection_error_callback(cube_connection_error_callback_t callback)
{
    error_callback = callback;
}

/**
 * Called when a connection error occurs.
 * @param cube_side The side of the cube where the error occurred
 * @param status The error status code, one of CUBE_CONNECTION_ERROR_ANNOUNCE_TIMEOUT, CUBE_CONNECTION_ERROR_ACKNOWLEDGE_PRESENCE_TIMEOUT, CUBE_CONNECTION_ERROR_INIT_HANDSHAKE_TIMEOUT, CUBE_CONNECTION_ERROR_ACKNOWLEDGE_HANDSHAKE_TIMEOUT, CUBE_CONNECTION_ERROR_COMPLETE_HANDSHAKE_TIMEOUT
 */
static inline void cube_handle_connection_error(cube_side_t cube_side, cube_connection_status_t status)
{
    if (error_callback)
    {
        error_callback(cube_side, status);
    }
}

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

/**
 * Announce presence and check if a cube is connected at the given side by doing a handshake.
 * @param cube_side The side of the cube to check the connection for
 * @return 1 if connected, 0 if not connected
 */
uint8_t cube_connection_announce_presence(cube_side_t cube_side)
{
    GPIO_TypeDef *port1 = cube_side_to_port1(cube_side);
    uint32_t pin1 = cube_side_to_pin1(cube_side);
    GPIO_TypeDef *port2 = cube_side_to_port2(cube_side);
    uint32_t pin2 = cube_side_to_pin2(cube_side);

    // Announce presence by setting D1 low and D2 as input
    LL_GPIO_SetOutputPin(port2, pin2);
    LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_INPUT);
    LL_GPIO_ResetOutputPin(port1, pin1);
    LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_OUTPUT);

    // Wait for presence acknowledgment by checking for D2 to go low
    if (!wait_for_pin_state(port2, pin2, LOW, ANNOUNCEMENT_TIMEOUT))
    {
        cube_set_side_disconnected(cube_side);
        cube_handle_connection_error(cube_side, CUBE_CONNECTION_ERROR_ANNOUNCE_TIMEOUT);
        return 0;
    }
    // Set D1 high as input to initiate the handshake
    LL_GPIO_SetOutputPin(port1, pin1);
    LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_INPUT);
    // Wait for D1 to go low which acknowledges the handshake
    if (!wait_for_pin_state(port1, pin1, LOW, HANDSHAKE_TIMEOUT))
    {
        cube_set_side_disconnected(cube_side);
        cube_handle_connection_error(cube_side, CUBE_CONNECTION_ERROR_INIT_HANDSHAKE_TIMEOUT);
        return 0;
    }
    // Complete the handshake by setting D2 low again
    LL_GPIO_ResetOutputPin(port2, pin2);
    LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_OUTPUT);
    // Wait for D1 to go high again which completes the handshake
    if (!wait_for_pin_state(port1, pin1, HIGH, HANDSHAKE_TIMEOUT))
    {
        cube_set_side_disconnected(cube_side);
        cube_handle_connection_error(cube_side, CUBE_CONNECTION_ERROR_COMPLETE_HANDSHAKE_TIMEOUT);
        return 0;
    }
    // Go into idle state
    cube_set_side_idle(cube_side);
    return 1;
}

/**
 * Respond to an announcement from another cube by doing a handshake.
 * @param cube_side The side of the cube to respond to the announcement for
 * @return 1 if the connection was successful, 0 otherwise
 */
uint8_t cube_connection_respond_to_announcement(cube_side_t cube_side)
{
    GPIO_TypeDef *port1 = cube_side_to_port1(cube_side);
    uint32_t pin1 = cube_side_to_pin1(cube_side);
    GPIO_TypeDef *port2 = cube_side_to_port2(cube_side);
    uint32_t pin2 = cube_side_to_pin2(cube_side);

    // Acknowledge presence by setting D1 low and D2 high as input
    LL_GPIO_SetOutputPin(port2, pin2);
    LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_INPUT);
    LL_GPIO_ResetOutputPin(port1, pin1);
    LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_OUTPUT);

    // Wait for handshake initiation by checking for D2 to go high
    if (!wait_for_pin_state(port2, pin2, HIGH, HANDSHAKE_TIMEOUT))
    {
        cube_set_side_disconnected(cube_side);
        cube_handle_connection_error(cube_side, CUBE_CONNECTION_ERROR_ACKNOWLEDGE_PRESENCE_TIMEOUT);
        return 0;
    }

    // Acknowledge the handshake by setting D2 low and D1 as input
    LL_GPIO_ResetOutputPin(port2, pin2);
    LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(port1, pin1);
    LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_INPUT);

    // Wait for the other cube to complete the handshake by checking for D1 to go low
    if (!wait_for_pin_state(port1, pin1, LOW, HANDSHAKE_TIMEOUT))
    {
        cube_set_side_disconnected(cube_side);
        cube_handle_connection_error(cube_side, CUBE_CONNECTION_ERROR_ACKNOWLEDGE_HANDSHAKE_TIMEOUT);
        return 0;
    }

    // Complete the handshake by going into idle state
    cube_set_side_idle(cube_side);
    return 1;
}
