#include "data.h"

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

// clang-format off
#define HIGH 1
#define LOW 0
#define digitalWrite(port, pin, value) \
  do { \
    if (value) { \
      (port)->BSRR = (pin); \
    } else { \
      (port)->BRR = (pin); \
    } \
  } while (0)
/**
 * The opposite data pin is the pin on the opposite side of the cube.
 * E.g. opposite of DT1 is DB2.
 */
#define OPPOSITE_DATA_PIN(port, pin) \
  ((port) == DT1_PORT && (pin) == DT1_PIN ? (DB2_PORT, DB2_PIN) : \
   (port) == DT2_PORT && (pin) == DT2_PIN ? (DB1_PORT, DB1_PIN) : \
    (port) == DR1_PORT && (pin) == DR1_PIN ? (DL2_PORT, DL2_PIN) : \
    (port) == DR2_PORT && (pin) == DR2_PIN ? (DL1_PORT, DL1_PIN) : \
    (port) == DB1_PORT && (pin) == DB1_PIN ? (DT2_PORT, DT2_PIN) : \
    (port) == DB2_PORT && (pin) == DB2_PIN ? (DT1_PORT, DT1_PIN) : \
    (port) == DL1_PORT && (pin) == DL1_PIN ? (DR2_PORT, DR2_PIN) : \
    /* else */                               (DR1_PORT, DR1_PIN))
/**
 * The next data pin in clockwise order when looking at the cube face.
 */
#define CLOCKWISE_DATA_PIN(port, pin) \
  ((port) == DT1_PORT && (pin) == DT1_PIN ? (DR1_PORT, DR1_PIN) : \
   (port) == DT2_PORT && (pin) == DT2_PIN ? (DR2_PORT, DR2_PIN) : \
   (port) == DR1_PORT && (pin) == DR1_PIN ? (DB1_PORT, DB1_PIN) : \
   (port) == DR2_PORT && (pin) == DR2_PIN ? (DB2_PORT, DB2_PIN) : \
   (port) == DB1_PORT && (pin) == DB1_PIN ? (DL1_PORT, DL1_PIN) : \
   (port) == DB2_PORT && (pin) == DB2_PIN ? (DL2_PORT, DL2_PIN) : \
   (port) == DL1_PORT && (pin) == DL1_PIN ? (DT1_PORT, DT1_PIN) : \
   /* else */                               (DT2_PORT, DT2_PIN))
/**
 * The next data pin in counterclockwise order when looking at the cube face.
 */
#define COUNTERCLOCKWISE_DATA_PIN(port, pin) \
  ((port) == DT1_PORT && (pin) == DT1_PIN ? (DL1_PORT, DL1_PIN) : \
   (port) == DT2_PORT && (pin) == DT2_PIN ? (DL2_PORT, DL2_PIN) : \
   (port) == DR1_PORT && (pin) == DR1_PIN ? (DT1_PORT, DT1_PIN) : \
   (port) == DR2_PORT && (pin) == DR2_PIN ? (DT2_PORT, DT2_PIN) : \
   (port) == DB1_PORT && (pin) == DB1_PIN ? (DR1_PORT, DR1_PIN) : \
   (port) == DB2_PORT && (pin) == DB2_PIN ? (DR2_PORT, DR2_PIN) : \
   (port) == DL1_PORT && (pin) == DL1_PIN ? (DB1_PORT, DB1_PIN) : \
   /* else */                               (DB2_PORT, DB2_PIN))
/**
 * The neighboring data pin on the same face.
 */
#define NEIGHBOR_DATA_PIN(port, pin) \
  ((port) == DT1_PORT && (pin) == DT1_PIN ? (DT2_PORT, DT2_PIN) : \
   (port) == DT2_PORT && (pin) == DT2_PIN ? (DT1_PORT, DT1_PIN) : \
   (port) == DR1_PORT && (pin) == DR1_PIN ? (DR2_PORT, DR2_PIN) : \
   (port) == DR2_PORT && (pin) == DR2_PIN ? (DR1_PORT, DR1_PIN) : \
   (port) == DB1_PORT && (pin) == DB1_PIN ? (DB2_PORT, DB2_PIN) : \
   (port) == DB2_PORT && (pin) == DB2_PIN ? (DB1_PORT, DB1_PIN) : \
   (port) == DL1_PORT && (pin) == DL1_PIN ? (DL2_PORT, DL2_PIN) : \
   /* else */                               (DL1_PORT, DL1_PIN))
// clang-format on

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
void data_init()
{
    enable_all_clocks();
    init_all_pins();
}

/**
 * Sets all D1 pins to input mode and all D2 pins to output low, waiting for further instructions.
 */
void data_set_idle()
{
    // Set all D1 pins to input mode
    LL_GPIO_SetPinMode(DT1_PORT, DT1_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(DR1_PORT, DR1_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(DB1_PORT, DB1_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(DL1_PORT, DL1_PIN, LL_GPIO_MODE_INPUT);

    // Set all D2 pins to output low
    LL_GPIO_SetPinMode(DT2_PORT, DT2_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_ResetOutputPin(DT2_PORT, DT2_PIN);
    LL_GPIO_SetPinMode(DR2_PORT, DR2_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_ResetOutputPin(DR2_PORT, DR2_PIN);
    LL_GPIO_SetPinMode(DB2_PORT, DB2_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_ResetOutputPin(DB2_PORT, DB2_PIN);
    LL_GPIO_SetPinMode(DL2_PORT, DL2_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_ResetOutputPin(DL2_PORT, DL2_PIN);
}

static inline uint8_t read_data_pin(GPIO_TypeDef *gpio_port, uint32_t gpio_pin)
{
    return (LL_GPIO_IsInputPinSet(gpio_port, gpio_pin)) ? 1 : 0;
}

uint8_t data_read_dt1()
{
    return read_data_pin(DT1_PORT, DT1_PIN);
}
uint8_t data_read_dr1()
{
    return read_data_pin(DR1_PORT, DR1_PIN);
}
uint8_t data_read_db1()
{
    return read_data_pin(DB1_PORT, DB1_PIN);
}
uint8_t data_read_dl1()
{
    return read_data_pin(DL1_PORT, DL1_PIN);
}
