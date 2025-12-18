#include "sk6812_effects.h"

/**
 * Show a binary code from 0-255 on the SK6812 LEDs for debugging.
 * Each bit corresponds to one LED pixel (LSB = pixel 0).
 * @param code The binary code to show (0-255)
 */
void sk6812_show_binary_code(uint8_t code)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (code & (1 << i))
        {
            sk6812_set_pixel(i, 1, 1, 1);
        }
        else
        {
            sk6812_set_pixel(i, 0, 0, 0);
        }
    }
    sk6812_show(1);
}
