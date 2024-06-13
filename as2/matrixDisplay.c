#include "matrixDisplay.h"

static Digits integers[11] = {

		{{0b00000000, 0b01110000, 0b01010000, 0b01010000, 0b01010000, 0b01010000, 0b01010000, 0b01110000}}, // 0
		{{0b00000000, 0b01110000, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b01100000, 0b00100000}}, // 1
		{{0b00000000, 0b01110000, 0b01000000, 0b01000000, 0b00100000, 0b00010000, 0b01010000, 0b00100000}}, // 2
		{{0b00000000, 0b01110000, 0b00010000, 0b00010000, 0b01110000, 0b00010000, 0b00010000, 0b01110000}}, // 3
		{{0b00000000, 0b00010000, 0b00010000, 0b00010000, 0b01110000, 0b01010000, 0b01010000, 0b01010000}}, // 4
		{{0b00000000, 0b01110000, 0b00010000, 0b00010000, 0b01110000, 0b01000000, 0b01000000, 0b01110000}}, // 5
		{{0b00000000, 0b01110000, 0b01010000, 0b01010000, 0b01110000, 0b01000000, 0b01000000, 0b01110000}}, // 6
		{{0b00000000, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b00010000, 0b00010000, 0b01110000}}, // 7
		{{0b00000000, 0b01110000, 0b01010000, 0b01010000, 0b01110000, 0b01010000, 0b01010000, 0b01110000}}, // 8
		{{0b00000000, 0b01110000, 0b00010000, 0b00010000, 0b01110000, 0b01010000, 0b01010000, 0b01110000}}, // 9
		{{0b00000100, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000}}  // .  

};

int initI2cBus(char* bus, int address)
{
	//configure pins
	system("config-pin P9_17 i2c");
	system("config-pin P9_18 i2c");

	int i2cFileDesc = open(bus, O_RDWR);
	if (i2cFileDesc < 0) {
		printf("I2C DRV: Unable to open bus for read/write (%s)\n", bus);
		perror("Error is:");
		exit(-1);
	}

	int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
	if (result < 0) {
		perror("Unable to set I2C device to slave address.");
		exit(-1);
	}

	//turn on LED Matrix
	writeI2cReg(i2cFileDesc, REG_DIRA, 0x00);
	writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);
	
	return i2cFileDesc;
}

void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
{
	unsigned char buff[2];
	buff[0] = regAddr;
	buff[1] = value;
	int res = write(i2cFileDesc, buff, 2);

	if (res != 2) {
		perror("Unable to write i2c register");
		exit(-1);
	}
}

unsigned char readI2cReg(int i2cFileDesc, unsigned char regAddr)
{
	// To read a register, must first write the address
	int res = write(i2cFileDesc, &regAddr, sizeof(regAddr));
	if (res != sizeof(regAddr)) {
		perror("Unable to write i2c register.");
		exit(-1);
	}

	// Now read the value and return it
	char value = 0;
	res = read(i2cFileDesc, &value, sizeof(value));
	if (res != sizeof(value)) {
		perror("Unable to read i2c register");
		exit(-1);
	}
	return value;
}

void displayInteger(int i2cFile, int value){
	
	//clip if outside boundaries
	if(value < 0){
		value = 0;
	}
	else if(value > 99){
		value = 99;
	}
    
    // get the tens digit
    int tens = value / 10;

	// get the ones digit
    int ones = value % 10;

    // put digits into a single array
	unsigned char arr[17]; //16 bytes total, every second one outputs to the LED display.
	arr[0] = 0x00; 
	int j = 0;
    for (int i = 1; i < 17; i = i+2) {
        arr[i]= integers[tens].rows[j] | ((integers[ones].rows[j] >> 5) | (integers[ones].rows[j] << (8 - 5)));
		j++;
    }
    j = 0;

    // Write the physical frame to the LED matrix
	int res = write(i2cFile, arr, sizeof(arr));

	if (res != sizeof(arr)) {
		perror("Unable to write i2c register");
		exit(-1);
	}

	
}

void displayFloat(int i2cFile, double value){

	//clip if outside boundaries	
	if(value < 0.0){
		value = 0.0;
	}
	else if(value > 9.9){
		value = 9.9;
	}

	//get the ones digit (left of decimal)
	int ones = (int) value;

	//get the tenths digit (right of the decimal)
	int tenths = (int) ((value-ones)*10);

	// put digits into a single array
	unsigned char arr[17]; //16 bytes total, every second one outputs to the LED display.
	arr[0] = 0x00;
	arr[1] = integers[10].rows[0];
	int j = 1;
    for (int i = 3; i < 17; i = i+2) {
        arr[i]= integers[ones].rows[j] | ((integers[tenths].rows[j] >> 5) | (integers[tenths].rows[j] << (8 - 5)));
		j++;
    }
    j = 0;

	//write to physical frame
	int res = write(i2cFile, arr, sizeof(arr));

	if (res != sizeof(arr)) {
		perror("Unable to write i2c register");
		exit(-1);
	}

}