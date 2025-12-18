#include "cube_data_transmission.h"

#define RECEIVE_FIRST_BIT_TIMEOUT 100000  // Longer timeout for the first bit to allow for initial delays
#define FIRST_ACKNOWLEDGE_TIMEOUT 1000000 // Timeout for waiting for acknowledge signal (around 300 ms)
#define ACKNOWLEDGE_TIMEOUT 10            // Timeout for waiting for acknowledge line changes
#define CLOCK_CYCLE_DELAY 5               // works with 1, but better be safe
#define CLOCK_SIGNAL_TIMEOUT 20           // Timeout for waiting for clock line changes, has to be bigger than CLOCK_CYCLE_DELAY

static inline void delay_cycles(volatile uint32_t cycles)
{
    while (cycles-- > 0)
        ;
}

/**
 * Receive a single byte from another cube using bit-banging on the data pins with one clock and one data line.
 * @param data_port GPIO port of the data line
 * @param data_pin GPIO pin of the data line
 * @param clock_port GPIO port of the clock line
 * @param clock_pin GPIO pin of the clock line
 * @param data Pointer to store the received data
 * @return The status code, one of CUBE_DATA_TRANSMISSION_OK, CUBE_DATA_TRANSMISSION_ERROR_CLOCK_TIMEOUT
 */
static inline cube_data_transmission_status_t cube_receive_byte(GPIO_TypeDef *data_port, uint32_t data_pin,
                                                                GPIO_TypeDef *clock_port, uint32_t clock_pin,
                                                                uint8_t *data, uint32_t timeout)
{
    *data = 0;
    for (int i = 0; i < 8; i++)
    {
        // Wait for clock line to go low
        while (read_data_pin(clock_port, clock_pin) == HIGH && timeout-- > 0)
            ;
        if (timeout == 0)
            return CUBE_DATA_TRANSMISSION_ERROR_CLOCK_TIMEOUT;
        timeout = CLOCK_SIGNAL_TIMEOUT;

        // Read data line
        *data <<= 1;
        if (read_data_pin(data_port, data_pin) == HIGH)
        {
            *data |= 0x01;
        }

        // Wait for clock line to go high
        while (read_data_pin(clock_port, clock_pin) == LOW && timeout-- > 0)
            ;
        if (timeout == 0)
            return CUBE_DATA_TRANSMISSION_ERROR_CLOCK_TIMEOUT;
        timeout = CLOCK_SIGNAL_TIMEOUT;
    }
    return CUBE_DATA_TRANSMISSION_OK;
}

/**
 * Send a single byte to another cube using bit-banging on the data pins with one clock and one data line.
 * @param data_port GPIO port of the data line
 * @param data_pin GPIO pin of the data line
 * @param clock_port GPIO port of the clock line
 * @param clock_pin GPIO pin of the clock line
 * @param data The data byte to send
 */
static inline void cube_send_byte(GPIO_TypeDef *data_port, uint32_t data_pin,
                                  GPIO_TypeDef *clock_port, uint32_t clock_pin,
                                  uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        // Set data line
        if (data & 0x80)
        {
            LL_GPIO_SetOutputPin(data_port, data_pin);
        }
        else
        {
            LL_GPIO_ResetOutputPin(data_port, data_pin);
        }
        data <<= 1;

        // Pulse clock line (active low)
        LL_GPIO_ResetOutputPin(clock_port, clock_pin);
        // Small delay to ensure the other cube can read the data
        delay_cycles(CLOCK_CYCLE_DELAY);
        LL_GPIO_SetOutputPin(clock_port, clock_pin);
        delay_cycles(CLOCK_CYCLE_DELAY);
    }
}

/**
 * Receive data from another cube using bit-banging on the data pins with one clock and one data line.
 * @param cube_side The side of the cube to communicate with
 * @param data Pointer to store the received data
 * @param max_length Maximum number of bytes to receive
 * @param length_received Pointer to store the actual length of received data
 * @return The status code, one of CUBE_DATA_TRANSMISSION_OK, CUBE_DATA_TRANSMISSION_ERROR_CLOCK_TIMEOUT
 */
cube_data_transmission_status_t cube_receive_data(cube_side_t cube_side, uint8_t *data, uint32_t max_length, uint32_t *length_received)
{
    GPIO_TypeDef *data_port = cube_side_to_port1(cube_side);
    uint32_t data_pin = cube_side_to_pin1(cube_side);
    GPIO_TypeDef *clock_port = cube_side_to_port2(cube_side);
    uint32_t clock_pin = cube_side_to_pin2(cube_side);
    // First receive the length of the data
    uint8_t total_length = 0;
    cube_data_transmission_status_t status = cube_receive_byte(data_port, data_pin, clock_port, clock_pin, &total_length, RECEIVE_FIRST_BIT_TIMEOUT);
    if (status != CUBE_DATA_TRANSMISSION_OK)
    {
        return status;
    }
    uint32_t length = total_length;
    uint32_t copy_length = (total_length < max_length) ? total_length : max_length;
    for (uint32_t i = 0; i < copy_length; i++)
    {
        status = cube_receive_byte(data_port, data_pin, clock_port, clock_pin, &data[i], CLOCK_SIGNAL_TIMEOUT);
        if (status != CUBE_DATA_TRANSMISSION_OK)
        {
            return status;
        }
    }
    // If the sender announced more bytes than we can store, discard the remaining bytes to keep the bus in sync.
    for (uint32_t i = copy_length; i < total_length; i++)
    {
        uint8_t discard;
        status = cube_receive_byte(data_port, data_pin, clock_port, clock_pin, &discard, CLOCK_SIGNAL_TIMEOUT);
        if (status != CUBE_DATA_TRANSMISSION_OK)
        {
            return status;
        }
    }
    if (length_received)
    {
        *length_received = copy_length;
    }
    return CUBE_DATA_TRANSMISSION_OK;
}

/**
 * Send data to another cube using bit-banging on the data pins with one clock and one data line.
 *
 * Needs to be called after cube_init_data_transfer().
 *
 * @param cube_side The side of the cube to communicate with
 * @param data Pointer to the data to send
 * @param length Length of the data in bytes
 */
void cube_send_data(cube_side_t cube_side, uint8_t *data, uint32_t length)
{
    GPIO_TypeDef *data_port = cube_side_to_port2(cube_side);
    uint32_t data_pin = cube_side_to_pin2(cube_side);
    GPIO_TypeDef *clock_port = cube_side_to_port1(cube_side);
    uint32_t clock_pin = cube_side_to_pin1(cube_side);
    LL_GPIO_SetPinMode(clock_port, clock_pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(data_port, data_pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(clock_port, clock_pin);
    LL_GPIO_ResetOutputPin(data_port, data_pin);
    cube_send_byte(data_port, data_pin, clock_port, clock_pin, length);
    for (uint32_t i = 0; i < length; i++)
    {
        cube_send_byte(data_port, data_pin, clock_port, clock_pin, data[i]);
    }

    LL_GPIO_SetOutputPin(data_port, data_pin);
    // LL_GPIO_SetOutputPin(clock_port, clock_pin); // clock is already high
    LL_GPIO_SetPinMode(data_port, data_pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(clock_port, clock_pin, LL_GPIO_MODE_INPUT);
}

/**
 * Initialize a data transfer to another cube and send 4 bytes of data.
 * Returns the data that is received from the other cube.
 * @param cube_side The side of the cube to communicate with
 * @return The status code, one of CUBE_DATA_TRANSMISSION_OK, CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_TIMEOUT, CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_FINISH_TIMEOUT
 * 
 * Note: Cube has to be in idle mode before calling this function.
 */
cube_data_transmission_status_t cube_init_data_transfer(cube_side_t cube_side)
{
    GPIO_TypeDef *port1 = cube_side_to_port1(cube_side);
    uint32_t pin1 = cube_side_to_pin1(cube_side);
    GPIO_TypeDef *port2 = cube_side_to_port2(cube_side);
    uint32_t pin2 = cube_side_to_pin2(cube_side);

    // Ask another cube for communication by setting the corresponding D1 as input and the D2 pin low
    LL_GPIO_ResetOutputPin(port2, pin2);
    LL_GPIO_SetOutputPin(port1, pin1);
    LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_INPUT);

    // Wait for acknowledge from the other cube by checking for the D1 pin to go low
    if (!wait_for_pin_state(port1, pin1, LOW, FIRST_ACKNOWLEDGE_TIMEOUT))
    {
        cube_set_side_idle(cube_side);
        return CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_TIMEOUT;
    }
    // Set D2 high again to finish the acknowledge process
    LL_GPIO_SetOutputPin(port2, pin2);

    // Wait for the other cube to finish the acknowledge by checking for the D1 pin to go high again
    if (!wait_for_pin_state(port1, pin1, HIGH, ACKNOWLEDGE_TIMEOUT))
    {
        cube_set_side_idle(cube_side);
        return CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_FINISH_TIMEOUT;
    }

    // Prepare pins for data transfer
    LL_GPIO_SetOutputPin(port1, pin1);
    LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_OUTPUT);

    return CUBE_DATA_TRANSMISSION_OK;
}

/**
 * Check if a cube is connected by reading the corresponding D2 pin.
 * If the pin is LOW, the cube is connected.
 * @return 1 if connected, 0 if not connected
 */
uint8_t cube_is_connected(cube_side_t cube_side)
{
    return read_data_pin(cube_side_to_port2(cube_side), cube_side_to_pin2(cube_side)) == LOW;
}

/**
 * Check if the cube wants to start communication by checking if the D1 pin is low.
 * @return The status code, one of CUBE_DATA_TRANSMISSION_OK, CUBE_DATA_TRANSMISSION_DISCONNECTED, CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_RECEIVE_TIMEOUT
 */
cube_data_transmission_status_t cube_handle_disconnect_or_communication_request(cube_side_t cube_side)
{
    GPIO_TypeDef *port1 = cube_side_to_port1(cube_side);
    uint32_t pin1 = cube_side_to_pin1(cube_side);
    GPIO_TypeDef *port2 = cube_side_to_port2(cube_side);
    uint32_t pin2 = cube_side_to_pin2(cube_side);

    // Check if the cube wants to start communication by checking if the D1 pin is low.
    LL_GPIO_SetOutputPin(port1, pin1);
    LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_INPUT);
    if (read_data_pin(port1, pin1) == HIGH)
    {
        return CUBE_DATA_TRANSMISSION_DISCONNECTED;
    }
    // Communication request detected, acknowledge a communication request by setting the D2 pin low.
    LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_ResetOutputPin(port2, pin2);

    // Wait for the other cube to receive the acknowledge by checking for the D1 pin to go high again.
    if (!wait_for_pin_state(port1, pin1, HIGH, ACKNOWLEDGE_TIMEOUT))
    {
        return CUBE_DATA_TRANSMISSION_ERROR_ACKNOWLEDGE_RECEIVE_TIMEOUT;
    }
    // Set D2 high again to finish the acknowledge process by setting it as input.
    LL_GPIO_SetOutputPin(port2, pin2);
    LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_INPUT);
    return CUBE_DATA_TRANSMISSION_OK;
}
