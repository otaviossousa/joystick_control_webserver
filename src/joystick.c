#include "lib/joystick.h"
#include "hardware/adc.h"
#include <math.h>

void init_joystick() {
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
}

float read_joystick_x() {
    adc_select_input(1); // Canal ADC correspondente ao pino X
    return adc_read() * (3.3f / (1 << 12)); // Conversão para tensão
}

float read_joystick_y() {
    adc_select_input(0); // Canal ADC correspondente ao pino Y
    return adc_read() * (3.3f / (1 << 12)); // Conversão para tensão
}

const char* get_joystick_direction(float x, float y) {
    if (y > 2.5 && x > 2.5) return "Nordeste ↗"; 
    if (y > 2.5 && x < 0.8) return "Noroeste ↖"; 
    if (y < 0.8 && x > 2.5) return "Sudeste ↘"; 
    if (y < 0.8 && x < 0.8) return "Sudoeste ↙"; 
    if (y > 2.5) return "Norte ↑";               
    if (y < 0.8) return "Sul ↓";               
    if (x > 2.5) return "Leste →";               
    if (x < 0.8) return "Oeste ←";                 
    return "Centro •";                           
}