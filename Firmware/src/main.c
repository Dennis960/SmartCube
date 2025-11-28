// #include "main.h"
#include "py32f0xx_hal.h"
#include <py32f002bx5.h>
#include <py32f002b_hal_gpio.h>

static void APP_GpioConfig(void);

int main(void)
{
  HAL_Init();

  APP_GpioConfig();

  while (1)
  {
    HAL_Delay(250);

    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
  }
}

static void APP_GpioConfig(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void APP_ErrorHandler(void)
{
  while (1)
  {
  }
}
