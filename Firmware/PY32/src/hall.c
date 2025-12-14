#include "hall.h"

#define HALL_PORT GPIOA
#define HALL_PIN LL_GPIO_PIN_4

/**
 * Initialize the analog pin for the hall sensor.
 */
void hall_init()
{
    /* Enable clocks */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_ADC1);

    /* PA4 as analog */
    LL_GPIO_SetPinMode(HALL_PORT, HALL_PIN, LL_GPIO_MODE_ANALOG);

    /* Reset & calibrate ADC */
    LL_ADC_Reset(ADC1);
    LL_ADC_StartCalibration(ADC1);
    while (LL_ADC_IsCalibrationOnGoing(ADC1))
        ;

    /* ADC basic config */
    LL_ADC_SetClock(ADC1, LL_ADC_CLOCK_SYNC_PCLK_DIV2);
    LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_12B);
    LL_ADC_SetDataAlignment(ADC1, LL_ADC_DATA_ALIGN_RIGHT);
    LL_ADC_SetSamplingTimeCommonChannels(
        ADC1, LL_ADC_SAMPLINGTIME_41CYCLES_5);

    /* Software trigger, single conversion */
    LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_SOFTWARE);
    LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_SINGLE);

    /* PA4 channel */
    LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_2);

    /* Enable ADC */
    LL_ADC_Enable(ADC1);
    LL_mDelay(1);
}

/**
 * Read the value from the hall sensor once and return it scaled to 0-1.
 */
float hall_read()
{
    /* Calibration constants */
    const float ref_voltage = 5.0f;
    const float min_voltage = 2.58f;
    const float max_voltage = 2.62f;
    const float adc_max = 4095.0f;  /* 12-bit ADC */

    /* Start conversion */
    LL_ADC_REG_StartConversion(ADC1);

    /* Wait for end of conversion */
    while (!LL_ADC_IsActiveFlag_EOC(ADC1))
        ;

    /* Clear EOC flag */
    LL_ADC_ClearFlag_EOC(ADC1);

    /* Read 12-bit result and scale to 0-1 */
    int raw = LL_ADC_REG_ReadConversionData12(ADC1);

    /* Convert ADC counts to voltage */
    float voltage = (raw / adc_max) * ref_voltage;

    /* Scale from min-max voltage range to 0-1 */
    float scaled = (voltage - min_voltage) / (max_voltage - min_voltage);

    /* Clamp to 0-1 range */
    if (scaled < 0.0f)
        scaled = 0.0f;
    if (scaled > 1.0f)
        scaled = 1.0f;

    return scaled;
}
