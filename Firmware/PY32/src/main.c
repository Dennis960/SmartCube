#include "sk6812.h"
#include "cube.h"

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

uint32_t led_data[4] = {0x00010100, 0, 0, 0};
uint32_t data_to_send[4] = {0x00000001, 0x00000100, 0x00010000, 0x00010001}; // blue, green, red, purple
uint32_t counter = 0;

void show_color()
{
  counter++;
  counter %= 400;
  if (counter > 200)
  {
    sk6812_clear();
    sk6812_show(1);
    return;
  }
  for (uint16_t i = 0; i < 4; i++)
  {
    uint32_t color = led_data[i];
    uint8_t red = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8) & 0xFF;
    uint8_t blue = color & 0xFF;
    sk6812_set_pixel(i, red, green, blue);
  }
  sk6812_show(1);
}

/**
 * Callback when data is received from another cube.
 * @param cube_side The side of the cube that sent the data
 * @param data Pointer to the received data
 * @param length Length of the received data
 */
void cube_data_received_callback(cube_side_t cube_side, uint32_t *data, uint32_t length)
{
  // Handle received data
  memcpy(led_data, data, 4 * sizeof(uint32_t));
  cube_send_data(cube_side, led_data, length);
}

/**
 * Callback when an error occurs during communication.
 * @param cube_side The side of the cube where the error occurred
 * @param cube_status The status code, one of CUBE_ERROR_TIMEOUT, CUBE_DISCONNECTED
 */
void cube_error_callback(cube_side_t cube_side, cube_status_t cube_status)
{
  // Handle error
}

/**
 * Callback when a cube is connected.
 * @param cube_side The side of the cube that was connected
 */
void cube_connected_callback(cube_side_t cube_side)
{
  // Handle cube connection
}

/**
 * Callback when a cube is disconnected.
 * @param cube_side The side of the cube that was disconnected
 */
void cube_disconnected_callback(cube_side_t cube_side)
{
  // Handle cube disconnection
}

int main(void)
{
  HAL_StatusTypeDef status = HAL_Init();
  enable_bor();
  SystemClock_Config();
  sk6812_init(GPIOB, LL_GPIO_PIN_2, 4); // Initialize LED API
  sk6812_clear();
  sk6812_show(1);

  cube_init();

  for (cube_side_t cube_side = CUBE_TOP; cube_side <= CUBE_LEFT; cube_side <<= 1)
  {
    if (!cube_is_connected(cube_side))
    {
      continue;
    }
    cube_status_t status = cube_init_data_transfer(cube_side);
    if (status != CUBE_OK)
    {
      cube_error_callback(cube_side, status);
      continue;
    }
    cube_send_data(cube_side, data_to_send, 4);
    uint32_t length = 0;
    status = cube_receive_data(cube_side, led_data, 4, &length);
  }

  cube_set_data_callback(cube_data_received_callback);
  cube_set_error_callback(cube_error_callback);
  cube_set_connected_callback(cube_connected_callback);
  cube_set_disconnected_callback(cube_disconnected_callback);

  while (1)
  {
    cube_loop();
    show_color();
  }
}
