#include "hardware.h"

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

/**
 * Initializes the hardware, including HAL, brown out reset, and system clock.
 */
void hardware_init(void)
{
    HAL_StatusTypeDef status = HAL_Init();
    enable_bor();
    SystemClock_Config();
}