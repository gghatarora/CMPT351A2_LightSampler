#include "joystickRead.h"

double Joystick_readX()
{
	// Open file
	FILE *f = fopen(A2D_FILE_VOLTAGE2, "r");
	if (!f) {
		printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
		printf("       Check /boot/uEnv.txt for correct options.\n");
		exit(-1);
	}

	// Get reading
	double a2dReading = 0;
	double itemsRead = fscanf(f, "%lf", &a2dReading);
	if (itemsRead <= 0) {
		printf("ERROR: Unable to read values from voltage input file.\n");
		exit(-1);
	}

	// Close file
	fclose(f);

	return a2dReading;
}

double Joystick_readY()
{
	// Open file
	FILE *f = fopen(A2D_FILE_VOLTAGE3, "r");
	if (!f) {
		printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
		printf("       Check /boot/uEnv.txt for correct options.\n");
		exit(-1);
	}

	// Get reading
	double a2dReading = 0;
	double itemsRead = fscanf(f, "%lf", &a2dReading);
	if (itemsRead <= 0) {
		printf("ERROR: Unable to read values from voltage input file.\n");
		exit(-1);
	}

	// Close file
	fclose(f);

	return a2dReading;
}
