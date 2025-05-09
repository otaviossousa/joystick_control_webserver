#ifndef JOYSTICK_H
#define JOYSTICK_H

#define JOYSTICK_X 26 
#define JOYSTICK_Y 27 

void init_joystick();
float read_joystick_x();
float read_joystick_y();
const char* get_joystick_direction(float x, float y);

#endif