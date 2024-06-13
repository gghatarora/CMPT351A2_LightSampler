#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define A2D_FILE_VOLTAGE2  "/sys/bus/iio/devices/iio:device0/in_voltage2_raw" //X
#define A2D_FILE_VOLTAGE3  "/sys/bus/iio/devices/iio:device0/in_voltage3_raw" //Y
#define A2D_VOLTAGE_REF_V  1.8
#define A2D_MAX_READING    4095

//Read joystick X/Y position
double Joystick_readX();
double Joystick_readY();