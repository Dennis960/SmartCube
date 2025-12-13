#include "sk6812.h"
#include "data.h"

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

int main(void)
{
  enable_bor();
  SystemClock_Config();
  sk6812_init(GPIOB, LL_GPIO_PIN_2, 4); // Initialize LED API
  sk6812_clear();
  sk6812_show(1);
  data_init();
  data_set_idle();

  int brightness = 255;

  while (1)
  {
    data_read_dt1() ? sk6812_set_pixel(0, 0, 0, 0) : sk6812_set_pixel(0, 0, brightness, 0);
    data_read_dr1() ? sk6812_set_pixel(1, 0, 0, 0) : sk6812_set_pixel(1, 0, brightness, 0);
    data_read_db1() ? sk6812_set_pixel(2, 0, 0, 0) : sk6812_set_pixel(2, 0, brightness, 0);
    data_read_dl1() ? sk6812_set_pixel(3, 0, 0, 0) : sk6812_set_pixel(3, 0, brightness, 0);
    sk6812_show(0);
  }
}

// // Timer code, might be useful later
// RCC->APBENR2 |= RCC_APBENR2_TIM1EN; // Enable TIM1 clock
// TIM1->PSC = 0;                      // Prescaler (divide clock)
// TIM1->ARR = 0xFFFFFFFF;             // Auto-reload value (max count)
// TIM1->CNT = 0;

// // Enable counter
// TIM1->CR1 |= TIM_CR1_CEN;

// // Force update to load PSC and ARR immediately
// TIM1->EGR = TIM_EGR_UG;
// uint32_t start = TIM1->CNT;
// __NOP();
// uint32_t end = TIM1->CNT;
// uint32_t diff = end - start;
