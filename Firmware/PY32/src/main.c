#include "sk6812.h"

/* MCU-specific */
#ifndef PY32F002Bx5
#define PY32F002Bx5
#endif
#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#include "py32f0xx.h"
#include "py32f002b_ll_bus.h"
#include "py32f002b_ll_rcc.h"
#include "py32f002b_ll_system.h"
#include "py32f002b_ll_gpio.h"
#include "py32f002b_ll_utils.h"

void Error_Handler(void);
static void SystemClock_Config(void);

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

int main(void)
{
  SystemClock_Config();
  sk6812_init(GPIOB, LL_GPIO_PIN_2, 4); // Initialize LED API

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
