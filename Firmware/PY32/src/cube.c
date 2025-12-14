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
#define ECHO_DELAY 1000

static inline void delay_cycles(volatile uint32_t cycles)
{
  while (cycles-- > 0)
    ;
}

cube_state_t cube_state = CUBE_STATE_INIT;
uint8_t cube_is_master = 0;

static void set_cube_state(cube_state_t new_state)
{
  if (cube_state == CUBE_STATE_ERROR)
    // cube can not leave error state without a reset
    return;
  cube_state = new_state;
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
  LL_GPIO_InitTypeDef g = {0};
  g.Pin = gpio_pin;
  g.Mode = LL_GPIO_MODE_OUTPUT;
  g.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  g.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  g.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(gpio_port, &g);

  LL_GPIO_ResetOutputPin(gpio_port, gpio_pin);
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
  LL_GPIO_SetPinMode(DT1_PORT, DT1_PIN, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(DT1_PORT, DT1_PIN);
  LL_GPIO_SetPinMode(DR1_PORT, DR1_PIN, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(DR1_PORT, DR1_PIN);
  LL_GPIO_SetPinMode(DB1_PORT, DB1_PIN, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(DB1_PORT, DB1_PIN);
  LL_GPIO_SetPinMode(DL1_PORT, DL1_PIN, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(DL1_PORT, DL1_PIN);

  // Set all D2 pins to input mode
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
static uint8_t cube_is_connected(cube_side_t cube_side)
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
 * Receive a single 32-bit word from another cube using bit-banging on the data pins with one clock and one data line.
 * @param data_port GPIO port of the data line
 * @param data_pin GPIO pin of the data line
 * @param clock_port GPIO port of the clock line
 * @param clock_pin GPIO pin of the clock line
 * @param data Pointer to store the received data
 * @return 0 on success, 1 on timeout
 */
static inline uint32_t cube_receive_word(GPIO_TypeDef *data_port, uint32_t data_pin,
                                         GPIO_TypeDef *clock_port, uint32_t clock_pin,
                                         uint32_t *data)
{
  for (int i = 0; i < 32; i++)
  {
    uint32_t timeout = TIMEOUT_LIMIT;
    // Wait for clock line to go low
    while (read_data_pin(clock_port, clock_pin) == HIGH && timeout-- > 0)
      ;
    if (timeout == 0)
      return 1; // Timeout occurred

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
      return 1; // Timeout occurred
  }
  return 0;
}

/**
 * Receive data from another cube using bit-banging on the data pins with one clock and one data line.
 * @param cube_side The side of the cube to communicate with
 * @param data Pointer to store the received data
 * @param max_length Maximum number of words to receive
 * @return The length of the received data in words. 0 if an error occurred.
 */
uint32_t cube_receive_data(cube_side_t cube_side, uint32_t *data, uint32_t max_length)
{
  GPIO_TypeDef *data_port = cube_side_to_port1(cube_side);
  uint32_t data_pin = cube_side_to_pin1(cube_side);
  GPIO_TypeDef *clock_port = cube_side_to_port2(cube_side);
  uint32_t clock_pin = cube_side_to_pin2(cube_side);
  // First receive the length of the data
  uint32_t length = 0;
  uint32_t err = cube_receive_word(data_port, data_pin, clock_port, clock_pin, &length);
  if (err)
  {
    return 0;
  }
  for (int i = 0; i < length && i < max_length; i++)
  {
    uint32_t err = cube_receive_word(data_port, data_pin, clock_port, clock_pin, &data[i]);
    if (err)
    {
      return 0;
    }
  }
  return length;
}

static inline void cube_send_word(GPIO_TypeDef *data_port, uint32_t data_pin,
                                  GPIO_TypeDef *clock_port, uint32_t clock_pin,
                                  uint32_t data)
{
  for (int i = 0; i < 32; i++)
  {
    // Set data line
    if (data & 0x80000000)
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
 * @param length Length of the data in words
 */
void cube_send_data(cube_side_t cube_side, uint32_t *data, uint32_t length)
{
  GPIO_TypeDef *data_port = cube_side_to_port2(cube_side);
  uint32_t data_pin = cube_side_to_pin2(cube_side);
  GPIO_TypeDef *clock_port = cube_side_to_port1(cube_side);
  uint32_t clock_pin = cube_side_to_pin1(cube_side);
  LL_GPIO_SetPinMode(clock_port, clock_pin, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinMode(data_port, data_pin, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetOutputPin(clock_port, clock_pin);
  LL_GPIO_ResetOutputPin(data_port, data_pin);
  cube_send_word(data_port, data_pin, clock_port, clock_pin, length);
  for (int i = 0; i < length; i++)
  {
    cube_send_word(data_port, data_pin, clock_port, clock_pin, data[i]);
  }

  LL_GPIO_SetOutputPin(data_port, data_pin);
  // LL_GPIO_SetOutputPin(clock_port, clock_pin); // clock is already high
  LL_GPIO_SetPinMode(data_port, data_pin, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinMode(clock_port, clock_pin, LL_GPIO_MODE_INPUT);
}

/**
 * Check if the cube wants to start communication by checking if the D1 pin is low.
 * @return 0 if communication request detected, 1 if the cube is disconnected, 2 if an error occurred
 */
static uint8_t cube_handle_disconnect_or_communication_request(cube_side_t cube_side)
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
    return 1; // Cube is disconnected
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
    set_cube_state(CUBE_STATE_SOFT_ERROR);
    return 2; // Timeout occurred, treat as disconnected
  }
  // Set D2 high again to finish the acknowledge process by setting it as input.
  LL_GPIO_SetOutputPin(port2, pin2);
  LL_GPIO_SetPinMode(port2, pin2, LL_GPIO_MODE_INPUT);
  return 0;
}

/**
 * Initialize a data transfer to another cube and send 4 bytes of data.
 * Returns the data that is received from the other cube.
 * @param cube_side The side of the cube to communicate with
 * @return 1 if an error occurred, 0 otherwise
 */
uint32_t cube_init_data_transfer(cube_side_t cube_side)
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
    set_cube_state(CUBE_STATE_ERROR);
    return 1; // Timeout occurred
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
    set_cube_state(CUBE_STATE_ERROR);
    return 1; // Timeout occurred
  }

  // Prepare pins for data transfer
  LL_GPIO_SetOutputPin(port1, pin1);
  LL_GPIO_SetPinMode(port1, pin1, LL_GPIO_MODE_OUTPUT);

  return 0; // Success
}

static void cube_handle_connected(cube_side_t cube_side)
{
}

static void cube_handle_disconnected(cube_side_t cube_side)
{
}

static uint8_t connected_cubes = 0;

void cube_init()
{
  cube_hardware_init();
  cube_set_idle();
  LL_mDelay(10); // Wait a bit for other cubes to power up

  uint8_t found_cube = 0;
  for (cube_side_t cube_side = CUBE_TOP; cube_side <= CUBE_LEFT; cube_side <<= 1)
  {
    if (cube_is_connected(cube_side))
    {
      found_cube = 1;
      break;
    }
  }
  if (!found_cube)
  {
    // TODO: implement communication with esp32 power supply cube so this will never happen and be an error
    // For now, when this happens, this cube will just be the master cube
    // set_cube_state(CUBE_STATE_ERROR);
    cube_is_master = 1;
  }
  else
  {
    cube_is_master = 0;
  }
}

/**
 * Main loop to handle cube connections and data transfers.
 * @param data Pointer to store received data
 * @param length Length of data to receive in words
 */
void cube_loop(uint32_t *data, uint32_t length)
{
  for (cube_side_t cube_side = CUBE_TOP; cube_side <= CUBE_LEFT; cube_side <<= 1)
  {
    if (!(connected_cubes & cube_side) && cube_is_connected(cube_side))
    {
      cube_handle_connected(cube_side);
      connected_cubes |= cube_side;
    }
    else if ((connected_cubes & cube_side) && !cube_is_connected(cube_side))
    {
      // Either the cube was disconnected or wants to start a new communication
      uint8_t err = cube_handle_disconnect_or_communication_request(cube_side);
      if (err == 1)
      {
        cube_handle_disconnected(cube_side);
        connected_cubes &= ~cube_side;
      }
      else if (err == 2)
      {
        // An error occurred during communication setup
      }
      else
      {
        cube_receive_data(cube_side, data, length);
        // TODO: handle received data
        // For now, just echo back the received data after a short delay
        delay_cycles(ECHO_DELAY);
        cube_send_data(cube_side, data, length);
        cube_set_idle();
      }
    }
  }
}