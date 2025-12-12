#include "sk6812.h"

/* MCU-specific */
#ifndef PY32F002Bx5
#define PY32F002Bx5
#endif
#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "py32f0xx.h"
#include "py32f0xx_hal.h"
#include "py32f002b_ll_bus.h"
#include "py32f002b_ll_rcc.h"
#include "py32f002b_ll_system.h"
#include "py32f002b_ll_gpio.h"
#include "py32f002b_ll_utils.h"

void Error_Handler(void);
static void SystemClock_Config(void);

#define DATA_TOP_1_PORT GPIOB
#define DATA_TOP_1_PIN LL_GPIO_PIN_7
#define DATA_TOP_2_PORT GPIOC
#define DATA_TOP_2_PIN LL_GPIO_PIN_1
#define DATA_RIGHT_1_PORT GPIOA
#define DATA_RIGHT_1_PIN LL_GPIO_PIN_5
#define DATA_RIGHT_2_PORT GPIOA
#define DATA_RIGHT_2_PIN LL_GPIO_PIN_6
#define DATA_BOTTOM_1_PORT GPIOA
#define DATA_BOTTOM_1_PIN LL_GPIO_PIN_1
#define DATA_BOTTOM_2_PORT GPIOA
#define DATA_BOTTOM_2_PIN LL_GPIO_PIN_0
#define DATA_LEFT_1_PORT GPIOB
#define DATA_LEFT_1_PIN LL_GPIO_PIN_3
#define DATA_LEFT_2_PORT GPIOB
#define DATA_LEFT_2_PIN LL_GPIO_PIN_4

uint32_t led_rainbow(uint8_t position)
{
  position = 255 - position;
  if (position < 85)
  {
    return ((255 - position * 3) << 16) | (0 << 8) | (position * 3);
  }
  else if (position < 170)
  {
    position -= 85;
    return (0 << 16) | (position * 3 << 8) | (255 - position * 3);
  }
  else
  {
    position -= 170;
    return (position * 3 << 16) | (255 - position * 3 << 8) | (0);
  }
}

static void init_data_pin(GPIO_TypeDef *gpio_port, uint32_t gpio_pin)
{
  switch ((uint32_t)gpio_port)
  {
  case (uint32_t)GPIOA:
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    break;
  case (uint32_t)GPIOB:
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
    break;
  case (uint32_t)GPIOC:
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
    break;
  default:
    return;
  }

  LL_GPIO_InitTypeDef g = {0};
  g.Pin = gpio_pin;
  g.Mode = LL_GPIO_MODE_OUTPUT;
  g.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  g.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  g.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(gpio_port, &g);

  LL_GPIO_ResetOutputPin(gpio_port, gpio_pin);
}

static uint8_t on = 0;

static void toggle_all_pins(void)
{
  if (on)
  {
    DATA_TOP_1_PORT->BRR = DATA_TOP_1_PIN;
    DATA_TOP_2_PORT->BRR = DATA_TOP_2_PIN;
    DATA_RIGHT_1_PORT->BRR = DATA_RIGHT_1_PIN;
    DATA_RIGHT_2_PORT->BRR = DATA_RIGHT_2_PIN;
    DATA_BOTTOM_1_PORT->BRR = DATA_BOTTOM_1_PIN;
    DATA_BOTTOM_2_PORT->BRR = DATA_BOTTOM_2_PIN;
    DATA_LEFT_1_PORT->BRR = DATA_LEFT_1_PIN;
    DATA_LEFT_2_PORT->BRR = DATA_LEFT_2_PIN;
    on = 0;
  }
  else
  {
    DATA_TOP_1_PORT->BSRR = DATA_TOP_1_PIN;
    DATA_TOP_2_PORT->BSRR = DATA_TOP_2_PIN;
    DATA_RIGHT_1_PORT->BSRR = DATA_RIGHT_1_PIN;
    DATA_RIGHT_2_PORT->BSRR = DATA_RIGHT_2_PIN;
    DATA_BOTTOM_1_PORT->BSRR = DATA_BOTTOM_1_PIN;
    DATA_BOTTOM_2_PORT->BSRR = DATA_BOTTOM_2_PIN;
    DATA_LEFT_1_PORT->BSRR = DATA_LEFT_1_PIN;
    DATA_LEFT_2_PORT->BSRR = DATA_LEFT_2_PIN;
    on = 1;
  }
}

static void init_all_pins(void)
{
  init_data_pin(DATA_TOP_1_PORT, DATA_TOP_1_PIN);
  init_data_pin(DATA_TOP_2_PORT, DATA_TOP_2_PIN);
  init_data_pin(DATA_RIGHT_1_PORT, DATA_RIGHT_1_PIN);
  init_data_pin(DATA_RIGHT_2_PORT, DATA_RIGHT_2_PIN);
  init_data_pin(DATA_BOTTOM_1_PORT, DATA_BOTTOM_1_PIN);
  init_data_pin(DATA_BOTTOM_2_PORT, DATA_BOTTOM_2_PIN);
  init_data_pin(DATA_LEFT_1_PORT, DATA_LEFT_1_PIN);
  init_data_pin(DATA_LEFT_2_PORT, DATA_LEFT_2_PIN);
}

/**
 * Enables brown out reset at 3.1-3.2V
 * The device will reset when VDD drops below this threshold (to prevent hanging)
 */
static void enable_bor(void)
{
  FLASH_OBProgramInitTypeDef OBInitCfg;

  /* Read current option bytes */
  HAL_FLASH_OBGetConfig(&OBInitCfg);

  if ((OBInitCfg.USERConfig & (OB_BOR_ENABLE | OB_BOR_LEVEL_3p1_3p2)) != (OB_BOR_ENABLE | OB_BOR_LEVEL_3p1_3p2))
  {
    HAL_FLASH_Unlock();    /* Unlock FLASH */
    HAL_FLASH_OB_Unlock(); /* Unlock Option Bytes */

    /* Prepare only BOR configuration */
    OBInitCfg.OptionType = OPTIONBYTE_USER;
    OBInitCfg.USERType = OB_USER_BOR_EN | OB_USER_BOR_LEV;

    /* Enable BOR with level 3.1-3.2V and iwdg */
    OBInitCfg.USERConfig = OB_BOR_ENABLE | OB_BOR_LEVEL_3p1_3p2;

    /* Program the option bytes */
    HAL_FLASH_OBProgram(&OBInitCfg);

    HAL_FLASH_Lock();    /* Lock FLASH */
    HAL_FLASH_OB_Lock(); /* Lock Option Bytes */

    /* Launch reset to apply */
    HAL_FLASH_OB_Launch();
  }
}

int main(void)
{
  enable_bor();
  SystemClock_Config();
  sk6812_init(GPIOB, LL_GPIO_PIN_2, 4); // Initialize LED API
  init_all_pins();

  while (1)
  {
    for (uint8_t pos = 0; pos < 256; pos++)
    {
      for (int i = 0; i < 4; i++)
      {
        uint8_t offset_pos = (pos + (i * 64)) % 256;
        uint32_t color = led_rainbow(offset_pos);
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;

        sk6812_set_pixel(i, r, g, b);
      }
      sk6812_show();
      LL_mDelay(10);
    }
  }
}

static void SystemClock_Config(void)
{
  LL_RCC_HSI_Enable();
  while (!LL_RCC_HSI_IsReady())
  {
  }

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSISYS);
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS)
  {
  }

  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

  LL_Init1msTick(24000000);
  LL_SetSystemCoreClock(24000000);
}
