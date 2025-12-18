#include "cube_hardware.h"

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

GPIO_TypeDef *cube_side_to_port1(cube_side_t cube_side)
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
uint32_t cube_side_to_pin1(cube_side_t cube_side)
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
GPIO_TypeDef *cube_side_to_port2(cube_side_t cube_side)
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
uint32_t cube_side_to_pin2(cube_side_t cube_side)
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

static void init_data_pin(GPIO_TypeDef *gpio_port, uint32_t gpio_pin, uint8_t input_mode)
{
    LL_GPIO_SetOutputPin(gpio_port, gpio_pin);
    LL_GPIO_InitTypeDef g = {0};
    g.Pin = gpio_pin;
    g.Mode = input_mode ? LL_GPIO_MODE_INPUT : LL_GPIO_MODE_OUTPUT;
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
    init_data_pin(DT1_PORT, DT1_PIN, 0);
    init_data_pin(DT2_PORT, DT2_PIN, 1);
    init_data_pin(DR1_PORT, DR1_PIN, 0);
    init_data_pin(DR2_PORT, DR2_PIN, 1);
    init_data_pin(DB1_PORT, DB1_PIN, 0);
    init_data_pin(DB2_PORT, DB2_PIN, 1);
    init_data_pin(DL1_PORT, DL1_PIN, 0);
    init_data_pin(DL2_PORT, DL2_PIN, 1);
}

/**
 * Initializes all data pins used by the cube communication system.
 */
void cube_hardware_init()
{
    enable_all_clocks();
    init_all_pins();
}
