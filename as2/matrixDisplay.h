#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <time.h>

#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"

#define I2C_DEVICE_ADDRESS 0x70
#define REG_DIRA 0x21
#define REG_DIRB 0x81

typedef struct {
    unsigned char rows[8];
} Digits;

//Initialize necessary pins, i2c BUS, and turn on the LED display.
int initI2cBus(char* bus, int address);

//write to i2c registers (used for initI2cBus)
void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value);

//Read from i2c registers
unsigned char readI2cReg(int i2cFileDesc, unsigned char regAddr);

//Display two-digit integer (0 to 99)
//If less than 10, then display with a zero in front (e.g. 9 = 09, 5 = 05, etc.)
void displayInteger(int i2cFile, int value);

//Display float (0.0 to 9.9)
void displayFloat(int i2cFile, double value);

