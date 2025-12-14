#include "cube.h"

#define DT1_PORT GPIOB
#define DT1_PIN LL_GPIO_PIN_7
#define DT2_PORT GPIOC
#define DT2_PIN LL_GPIO_PIN_1
#define DR1_PORT GPIOA
#define DR1_PIN LL_GPIO_PIN_5
#define DR2_PORT GPIOA
#define DR2_PIN LL_GPIO_PIN_6
#define DB1_PORT GPIOA
#define DB1_PIN LL_GPIO_PIN_1
#define DB2_PORT GPIOA
#define DB2_PIN LL_GPIO_PIN_0
#define DL1_PORT GPIOB
#define DL1_PIN LL_GPIO_PIN_3
#define DL2_PORT GPIOB
#define DL2_PIN LL_GPIO_PIN_4

#define HIGH 1
#define LOW 0

#define TIMEOUT_LIMIT 10000
#define COMMUNICATION_DELAY 10 // works with 1, but better be safe

static inline void delay_cycles(volatile uint32_t cycles)
{
  while (cycles-- > 0)
    ;
}

uint8_t cube_data_buffer[DATA_BUFFER_SIZE];

static cube_data_callback_t data_callback = NULL;
static cube_connected_callback_t connected_callback = NULL;
static cube_disconnected_callback_t disconnected_callback = NULL;
static cube_error_callback_t error_callback = NULL;

/**
 * Set the callback that is called when data is received from another cube.
 */
void cube_set_data_callback(cube_data_callback_t callback)
{
  data_callback = callback;
}

/**
 * Set the callback that is called when a cube is connected.
 */
void cube_set_connected_callback(cube_connected_callback_t callback)
{
  connected_callback = callback;
}

/**
 * Set the callback that is called when a cube is disconnected.
 */
void cube_set_disconnected_callback(cube_disconnected_callback_t callback)
{
  disconnected_callback = callback;
}

/**
 * Set the callback that is called when an error occurs during communication.
 */
void cube_set_error_callback(cube_error_callback_t callback)
{
  error_callback = callback;
}

static inline GPIO_TypeDef *cube_side_to_port1(cube_side_t cube_side)
{
  switch (cube_side)
  {
  case CUBE_TOP:
    return DT1_PORT;
  case CUBE_RIGHT:
    return DR1_PORT;
  case CUBE_BOTTOM:
    return DB1_PORT;
  case CUBE_LEFT:
    return DL1_PORT;
  }
}
static inline uint32_t cube_side_to_pin1(cube_side_t cube_side)
{
  switch (cube_side)
  {
  case CUBE_TOP:
    return DT1_PIN;
  case CUBE_RIGHT:
    return DR1_PIN;
  case CUBE_BOTTOM:
    return DB1_PIN;
  case CUBE_LEFT:
    return DL1_PIN;
  }
}
static inline GPIO_TypeDef *cube_side_to_port2(cube_side_t cube_side)
{
  switch (cube_side)
  {
  case CUBE_TOP:
    return DT2_PORT;
  case CUBE_RIGHT:
    return DR2_PORT;
  case CUBE_BOTTOM:
    return DB2_PORT;
  case CUBE_LEFT:
    return DL2_PORT;
  }
}
static inline uint32_t cube_side_to_pin2(cube_side_t cube_side)
{
  switch (cube_side)
  {
  case CUBE_TOP:
    return DT2_PIN;
  case CUBE_RIGHT:
    return DR2_PIN;
  case CUBE_BOTTOM:
    return DB2_PIN;
  case CUBE_LEFT:
    return DL2_PIN;
  }
}

static void init_data_pin(GPIO_TypeDef *gpio_port, uint32_t gpio_pin)
{
  LL_GPIO_SetOutputPin(gpio_port, gpio_pin);
  LL_GPIO_InitTypeDef g = {0};
  g.Pin = gpio_pin;
  g.Mode = LL_GPIO_MODE_OUTPUT;
  g.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  g.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  g.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(gpio_port, &g);
}

static void enable_all_clocks()
{
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
}

static void init_all_pins()
{
  init_data_pin(DT1_PORT, DT1_PIN);
  init_data_pin(DT2_PORT, DT2_PIN);
  init_data_pin(DR1_PORT, DR1_PIN);
  init_data_pin(DR2_PORT, DR2_PIN);
  init_data_pin(DB1_PORT, DB1_PIN);
  init_data_pin(DB2_PORT, DB2_PIN);
  init_data_pin(DL1_PORT, DL1_PIN);
  init_data_pin(DL2_PORT, DL2_PIN);
}

/**
 * Initializes all data pins.
 */
void cube_hardware_init()
{
  enable_all_clocks();
  init_all_pins();
}

/**
 * Sets all D1 pins to output low and all D2 pins to input mode, waiting for further instructions.
 */
void cube_set_idle()
{
  // Set all D1 pins to output low
  LL_GPIO_ResetOutputPin(DT1_PORT, DT1_PIN);
  LL_GPIO_ResetOutputPin(DR1_PORT, DR1_PIN);
  LL_GPIO_ResetOutputPin(DB1_PORT, DB1_PIN);
  LL_GPIO_ResetOutputPin(DL1_PORT, DL1_PIN);
  LL_GPIO_SetPinMode(DT1_PORT, DT1_PIN, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinMode(DR1_PORT, DR1_PIN, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinMode(DB1_PORT, DB1_PIN, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinMode(DL1_PORT, DL1_PIN, LL_GPIO_MODE_OUTPUT);

  // Set all D2 pins to input mode and high
  LL_GPIO_SetOutputPin(DT2_PORT, DT2_PIN);
  LL_GPIO_SetOutputPin(DR2_PORT, DR2_PIN);
  LL_GPIO_SetOutputPin(DB2_PORT, DB2_PIN);
  LL_GPIO_SetOutputPin(DL2_PORT, DL2_PIN);
  LL_GPIO_SetPinMode(DT2_PORT, DT2_PIN, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(DR2_PORT, DR2_PIN, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(DB2_PORT, DB2_PIN, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(DL2_PORT, DL2_PIN, LL_GPIO_MODE_INPUT);
}

static inline uint8_t read_data_pin(GPIO_TypeDef *gpio_port, uint32_t gpio_pin)
{
  return (LL_GPIO_IsInputPinSet(gpio_port, gpio_pin)) ? HIGH : LOW;
}

/**
 * Check if a cube is connected by reading the corresponding D2 pin.
 * If the pin is LOW, the cube is connected.
 * @return 1 if connected, 0 if not connected
 */
uint8_t cube_is_connected(cube_side_t cube_side)
{
  switch (cube_side)
  {
  case CUBE_TOP:
    return read_data_pin(DT2_PORT, DT2_PIN) == LOW;
  case CUBE_RIGHT:
    return read_data_pin(DR2_PORT, DR2_PIN) == LOW;
  case CUBE_BOTTOM:
    return read_data_pin(DB2_PORT, DB2_PIN) == LOW;
  case CUBE_LEFT:
    return read_data_pin(DL2_PORT, DL2_PIN) == LOW;
  }
}

/**
 * Receive a single byte from another cube using bit-banging on the data pins with one clock and one data line.
 * @param data_port GPIO port of the data line
 * @param data_pin GPIO pin of the data line
 * @param clock_port GPIO port of the clock line
 * @param clock_pin GPIO pin of the clock line
 * @param data Pointer to store the received data
 * @return The status code, one of CUBE_OK, CUBE_ERROR_TIMEOUT
 */
static inline cube_status_t cube_receive_byte(GPIO_TypeDef *data_port, uint32_t data_pin,
                                              GPIO_TypeDef *clock_port, uint32_t clock_pin,
                                              uint8_t *data)
{
  *data = 0;
  for (int i = 0; i < 8; i++)
  {
    uint32_t timeout = TIMEOUT_LIMIT;
    // Wait for clock line to go low
    while (read_data_pin(clock_port, clock_pin) == HIGH && timeout-- > 0)
      ;
    if (timeout == 0)
      return CUBE_ERROR_TIMEOUT;

    // Read data line
    *data <<= 1;
    if (read_data_pin(data_port, data_pin) == HIGH)
    {
      *data |= 0x01;
    }

    // Wait for clock line to go high
    timeout = TIMEOUT_LIMIT;
    while (read_data_pin(clock_port, clock_pin) == LOW && timeout-- > 0)
      ;
    if (timeout == 0)
      return CUBE_ERROR_TIMEOUT;
  }
  return CUBE_OK;
}

/**
 * Receive data from another cube using bit-banging on the data pins with one clock and one data line.
 * @param cube_side The side of the cube to communicate with
 * @param data Pointer to store the received data
 * @param max_length Maximum number of bytes to receive
 * @param length_received Pointer to store the actual length of received data
 * @return The status code, one of CUBE_OK, CUBE_ERROR_TIMEOUT
 */
cube_status_t cube_receive_data(cube_side_t cube_side, uint8_t *data, uint32_t max_length, uint32_t *length_received)
{
  GPIO_TypeDef *data_port = cube_side_to_port1(cube_side);
  uint32_t data_pin = cube_side_to_pin1(cube_side);
  GPIO_TypeDef *clock_port = cube_side_to_port2(cube_side);
  uint32_t clock_pin = cube_side_to_pin2(cube_side);
  // First receive the length of the data
  uint8_t total_length = 0;
  cube_status_t cube_status = cube_receive_byte(data_port, data_pin, clock_port, clock_pin, &total_length);
  if (cube_status != CUBE_OK)
  {
    return cube_status;
  }
  uint32_t length = total_length;
  uint32_t copy_length = (total_length < max_length) ? total_length : max_length;
  for (uint32_t i = 0; i < copy_length; i++)
  {
    cube_status = cube_receive_byte(data_port, data_pin, clock_port, clock_pin, &data[i]);
    if (cube_status != CUBE_OK)
    {
      return cube_status;
    }
  }
  // If the sender announced more bytes than we can store, discard the remaining bytes to keep the bus in sync.
  for (uint32_t i = copy_length; i < total_length; i++)
  {
    uint8_t discard;
    cube_status = cube_receive_byte(data_port, data_pin, clock_port, clock_pin, &discard);
    if (cube_status != CUBE_OK)
    {
      return cube_status;
    }
  }
  if (length_received)
  {
    *length_received = copy_length;
  }
  return CUBE_OK;
}

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
    delay_cycles(COMMUNICATION_DELAY);
    LL_GPIO_SetOutputPin(clock_port, clock_pin);
    delay_cycles(COMMUNICATION_DELAY);
  }
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
 * Check if the cube wants to start communication by checking if the D1 pin is low.
 * @return The status code, one of CUBE_OK, CUBE_ERROR_TIMEOUT, CUBE_DISCONNECTED
 */
static cube_status_t cube_handle_disconnect_or_communication_request(cube_side_t cube_side)
{
  GPIO_TypeDef *port1 = cube_side_to_port1(cube_side);
  uint32_t pin1 = cube_side_to_pin1(cube_side);
  GPIO_TypeDef *port2 = cube_side_to_port2(cube_side);
  uint32_t pin2 = cube_side_to_pin2(cube_side);
  uint8_t state = read_data_pin(port1, pin1);
  uint32_t timeout = TIMEOUT_LIMIT;

  // Check if the cube wants to start communication by checking if the D1 pin is low.
  LL_GPIO_SetOutputPin(port1, pin1);
  LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_INPUT);
  if (read_data_pin(port1, pin1) == HIGH)
  {
    return CUBE_DISCONNECTED;
  }
  // Communication request detected, acknowledge a communication request by setting the D2 pin low.
  LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(port2, pin2);

  // Wait for the other cube to receive the acknowledge by checking for the D1 pin to go high again.
  while (state == LOW && timeout-- > 0)
  {
    state = read_data_pin(port1, pin1);
  }
  if (state == LOW)
  {
    return CUBE_ERROR_TIMEOUT;
  }
  // Set D2 high again to finish the acknowledge process by setting it as input.
  LL_GPIO_SetOutputPin(port2, pin2);
  LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_INPUT);
  return CUBE_OK;
}

/**
 * Initialize a data transfer to another cube and send 4 bytes of data.
 * Returns the data that is received from the other cube.
 * @param cube_side The side of the cube to communicate with
 * @return The status code, one of CUBE_OK, CUBE_ERROR_TIMEOUT
 */
cube_status_t cube_init_data_transfer(cube_side_t cube_side)
{
  GPIO_TypeDef *port1 = cube_side_to_port1(cube_side);
  uint32_t pin1 = cube_side_to_pin1(cube_side);
  GPIO_TypeDef *port2 = cube_side_to_port2(cube_side);
  uint32_t pin2 = cube_side_to_pin2(cube_side);
  uint8_t state;
  uint32_t timeout;

  // Ask another cube for communication by setting the corresponding D1 as input and the D2 pin low
  LL_GPIO_ResetOutputPin(port2, pin2);
  LL_GPIO_SetOutputPin(port1, pin1);
  LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_INPUT);

  // Wait for acknowledge from the other cube by checking for the D1 pin to go low
  state = read_data_pin(port1, pin1);
  timeout = TIMEOUT_LIMIT;
  while (state == HIGH && timeout-- > 0)
  {
    state = read_data_pin(port1, pin1);
  }
  if (state == HIGH)
  {
    cube_set_idle();
    return CUBE_ERROR_TIMEOUT;
  }
  // Set D2 high again to finish the acknowledge process
  LL_GPIO_SetOutputPin(port2, pin2);

  // Wait for the other cube to finish the acknowledge by checking for the D1 pin to go high again
  state = read_data_pin(port1, pin1);
  timeout = TIMEOUT_LIMIT;
  while (state == LOW && timeout-- > 0)
  {
    state = read_data_pin(port1, pin1);
  }
  if (state == LOW)
  {
    cube_set_idle();
    return CUBE_ERROR_TIMEOUT;
  }

  // Prepare pins for data transfer
  LL_GPIO_SetOutputPin(port1, pin1);
  LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_OUTPUT);

  return CUBE_OK;
}

static uint8_t connected_cubes = 0;

void cube_init()
{
  cube_hardware_init();
  cube_set_idle();
  LL_mDelay(10); // Wait a bit for other cubes to power up
}

/**
 * Called when a cube is connected.
 */
static inline void cube_handle_connection(cube_side_t cube_side)
{
  if (connected_callback)
  {
    connected_callback(cube_side);
  }
  connected_cubes |= cube_side;
}

/**
 * Called when a cube is disconnected.
 */
static inline void cube_handle_disconnection(cube_side_t cube_side)
{
  if (disconnected_callback)
  {
    disconnected_callback(cube_side);
  }
  connected_cubes &= ~cube_side;
}

/**
 * Called when a communication error occurs.
 */
static inline void cube_handle_communication_error(cube_side_t cube_side, cube_status_t status)
{
  if (error_callback)
  {
    error_callback(cube_side, status);
  }
}

/**
 * Called when communication is started and data is expected to be received.
 */
static inline void cube_handle_communication_started(cube_side_t cube_side)
{
  uint32_t length = 0;
  cube_status_t cube_status = cube_receive_data(cube_side, cube_data_buffer, DATA_BUFFER_SIZE, &length);
  if (cube_status == CUBE_OK)
  {
    if (data_callback)
      data_callback(cube_side, cube_data_buffer, length);
  }
  else
  {
    cube_handle_communication_error(cube_side, cube_status);
  }
}

/**
 * Called when data disconnection is detected. The cube is either disconnected or wants to start communication.
 */
static inline void cube_handle_data_disconnection(cube_side_t cube_side)
{
  cube_status_t cube_status = cube_handle_disconnect_or_communication_request(cube_side);
  if (cube_status == CUBE_DISCONNECTED)
  {
    cube_handle_disconnection(cube_side);
  }
  else if (cube_status == CUBE_ERROR_TIMEOUT)
  {
    cube_handle_communication_error(cube_side, cube_status);
  }
  else if (cube_status == CUBE_OK)
  {
    cube_handle_communication_started(cube_side);
  }
  cube_set_idle();
}

/**
 * Main loop to be called periodically to handle cube connections and data transfers.
 */
void cube_loop()
{
  for (cube_side_t cube_side = CUBE_TOP; cube_side <= CUBE_LEFT; cube_side <<= 1)
  {
    uint8_t is_newly_connected = !(connected_cubes & cube_side) && cube_is_connected(cube_side);
    uint8_t is_data_disconnected = (connected_cubes & cube_side) && !cube_is_connected(cube_side);
    if (is_newly_connected)
    {
      cube_handle_connection(cube_side);
    }
    else if (is_data_disconnected)
    {
      cube_handle_data_disconnection(cube_side);
    }
  }
}