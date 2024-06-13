#include "sampler.h"
#include "matrixDisplay.h"
#include "joystickRead.h"

static samplerDatapoint_t samplesBuff[SAMPLE_BUFF_SIZE];

static pthread_t samplerThread;
static pthread_mutex_t samplerMutex;
static int numSamples = 0;
static int samplesTaken = 0;
static long long lastSampleTimestamp = 0;
double average = 0;
int numberOfDips = 0;

static void sleepForMs(long long delayInMs)
{
 const long long NS_PER_MS = 1000 * 1000;
 const long long NS_PER_SECOND = 1000000000;
 long long delayNs = delayInMs * NS_PER_MS;
 int seconds = delayNs / NS_PER_SECOND;
 int nanoseconds = delayNs % NS_PER_SECOND;
 struct timespec reqDelay = {seconds, nanoseconds};
 nanosleep(&reqDelay, (struct timespec *) NULL);
}

static long long getTimeInNanoS(void)
{
 struct timespec spec;
 clock_gettime(CLOCK_REALTIME, &spec);
 long long nanoSeconds = spec.tv_nsec;
 return nanoSeconds;
}

static long long getTimeInS(void)
{
 struct timespec spec;
 clock_gettime(CLOCK_REALTIME, &spec);
 long long seconds = spec.tv_sec;
 return seconds;
}

static int userButton()
{
    
    int userBtn;
    FILE *pFile = fopen("/sys/class/gpio/gpio72/value", "r");
    if (pFile == NULL) {
        printf("ERROR: Unable to read file.");
        exit(1);
    }

    //fscanf will store integer that was read from the pFile into address of userBtn
    if(fscanf(pFile, "%d", &userBtn) != 1) { 
        fclose(pFile);
        exit(1);
    }

    fclose(pFile);
    return userBtn;

}

double PhotoRes_read(){
    // Open file
	FILE *f = fopen(A2D_FILE_VOLTAGE1, "r");
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
	a2dReading = ((double)a2dReading / A2D_MAX_READING) * A2D_VOLTAGE_REF_V;
	return a2dReading;
}

samplerDatapoint_t* Sampler_extractAllValues(int *length){
	
	pthread_mutex_lock(&samplerMutex);
	*length = numSamples;
	samplerDatapoint_t *extractedVal = malloc(sizeof(samplerDatapoint_t)*(*length));
	for(int i = 0; i < *length; i++){
		extractedVal[i] = samplesBuff[i];
	}
	numSamples = 0; //reset the number of samples to zero
	pthread_mutex_unlock(&samplerMutex);

	return extractedVal;
}

int Sampler_getNumSamplesInHistory(){
	
	pthread_mutex_lock(&samplerMutex);
	int totNumSamples = numSamples;
	pthread_mutex_unlock(&samplerMutex);
	
	return totNumSamples;
}

long long Sampler_getNumSamplesTaken(void){
	pthread_mutex_lock(&samplerMutex);
	long long numSamplesTaken = samplesTaken;
	pthread_mutex_unlock(&samplerMutex);
	
	return numSamplesTaken;
}

//Begin thread.
void Sampler_startSampling(void){
	pthread_mutex_lock(&samplerMutex);
	pthread_create(&samplerThread, NULL, (void *(*)(void *))&Sampler_storeSample, NULL);
	pthread_mutex_unlock(&samplerMutex);
}

//Stop thread, and wait for it to finish running.
void Sampler_stopSampling(void){
	pthread_mutex_lock(&samplerMutex);
    pthread_cancel(samplerThread);
    pthread_join(samplerThread, NULL);
    pthread_mutex_unlock(&samplerMutex);
}

void* Sampler_storeSample(void){
	double average = 0; 
	while(1){
		//get start time and sample from the photo resistor.
		long long startTime = getTimeInS();
		double sample = PhotoRes_read();
		
		pthread_mutex_lock(&samplerMutex);
		//Critical section: we write to buffer and change number of samples.
		if(numSamples < SAMPLE_BUFF_SIZE){ 
			//if index is not at the end of the buffer continue to add to it
			samplesBuff[numSamples].sampleInV = sample;
			samplesBuff[numSamples].timestampInNanoS = getTimeInNanoS();
			numSamples++;
			samplesTaken++;
			lastSampleTimestamp = samplesBuff[numSamples].timestampInNanoS;
			average = Sampler_computeAvgVolt(samplesBuff, average);
		}
		if(numSamples == SAMPLE_BUFF_SIZE){ //if index reaches the end of the buffer, flush and reset index back to the start (Might not need this if statement)
			printf("buffer FULL, refilling\n");
			numSamples = 0;
		}

		pthread_mutex_unlock(&samplerMutex);
		
		sleepForMs(1);

		//Every second, we output, min/max interval, average interval, min/max sample voltage, average voltage, num of dips, and num of samples.
		if((getTimeInS()-startTime) == 1){
			int totalSamples = Sampler_getNumSamplesInHistory();
			int dips = Sampler_computeDips(samplesBuff, average);

			double aveInterval = Sampler_avgInterval();
			double minInterval = Sampler_minInterval();
			double maxInterval = Sampler_maxInterval();

			double minVolt = Sampler_computeMinVolt(samplesBuff);
			double maxVolt = Sampler_computeMaxVolt(samplesBuff);

			samplerDatapoint_t* samples = Sampler_extractAllValues(&numSamples);

			printf("Interval ms (%.3lf, %.3lf) avg=%.3lf   Samples V (%.3lf, %.3lf) avg=%.3lf   # Dips:   %d   # Samples:    %d\n", minInterval, maxInterval, aveInterval, minVolt, maxVolt, average, dips, totalSamples);
			startTime = getTimeInS();
			numberOfDips = dips;
			free(samples);
		}
	}
}

double Sampler_computeIntervalInMS(int index){
	double interval = 0;

	//if the index is at the first sample in the buffer, compare it with the previous second's, last sample.
	if(index == 0){
		interval = abs(samplesBuff[index].timestampInNanoS - lastSampleTimestamp);
	}

	//else compare the current index's timestamp to the previous index's timestamp. subtract.
	else{
		interval = abs(samplesBuff[index].timestampInNanoS - samplesBuff[index-1].timestampInNanoS);
	}
	
	//Convert to MS
	interval = ((interval)/(1e+6)); 

	return interval;
}

double Sampler_avgInterval(){
	//if less than two samples, we just skip.
	if(numSamples < 2){
		return 0.0;
	}

	//Compute average using exponential smoothing.
	double average = 0;
	double alpha = 0.999;

	for(int i = 0; i < numSamples; i++){
		average = (alpha*Sampler_computeIntervalInMS(i)) + ((1-alpha)*average);
	}

	return average;
}

double Sampler_minInterval(){
	//if theres no samples, theres no interval. 
	if(numSamples == 0){
		return 0.0;
	}

	//set minimum to be the interval between the first sample of this second and the last sample of the last second.
	//Iterate through all samples, and if there is a smaller interval, update the min.
	double min = Sampler_computeIntervalInMS(0);
	for(int i = 1; i < numSamples; i++){
		if(min > Sampler_computeIntervalInMS(i)){
			min = Sampler_computeIntervalInMS(i);
		}
	}

	return min;
}

double Sampler_maxInterval(){
	//if theres no samples, theres no interval. 
	if(numSamples == 0){
		return 0.0;
	}

	//set maximum to be negative.
	//Iterate through all samples, and if there is a bigger interval, update the max.
	double max = -1;
	for(int i = 1; i < numSamples; i++){
		if(max < Sampler_computeIntervalInMS(i)){
			max = Sampler_computeIntervalInMS(i);
		}
	}

	return max;
}

double Sampler_computeAvgVolt(samplerDatapoint_t* buff, double currentAvg){
	//represents the cumulative average off ALL samples taken, not just the ones in current history
	double alpha = 0.999;

	currentAvg = (alpha*buff[numSamples-1].sampleInV) + ((1-alpha)*(currentAvg)); //using exponential smoothing
	
	return currentAvg;
}

double Sampler_computeMinVolt(samplerDatapoint_t* buff){
	//if theres no samples, theres no voltage readings. 
	if(numSamples == 0){
		return 0.0;
	}

	//set minimum to be the first sample.
	//Iterate through all samples, and if there is a lower voltage, update the min.
	double min = buff[0].sampleInV;
	for(int i = 1; i < numSamples; i++){
		if(min > buff[i].sampleInV){
			min = buff[i].sampleInV;
		}
	}

	return min;
}

double Sampler_computeMaxVolt(samplerDatapoint_t* buff){
	//if theres no samples, theres no voltage readings. 
	if(numSamples == 0){
		return 0.0;
	}
	
	//set maximum to be the first sample.
	//Iterate through all samples, and if there is a higher voltage, update the max.
	double max = buff[0].sampleInV;
	for(int i = 1; i < numSamples; i++){
		if(max < buff[i].sampleInV){
			max = buff[i].sampleInV;
		}
	}

	return max;
}


int Sampler_computeDips(samplerDatapoint_t* buff, double average){
	double lower_threshold = (average - 0.1) - 0.3;
	int checkDip = 0; //if a dip happens, it is set to 1, and it only resets to zero if it goes back above the threshold.
	int count = 0;

	for(int i = 0; i < numSamples-1; i++){
		//if the voltage drops below the threshold for a single sample then we have a dip. AND if check dip is 0, that means we are not already in a dip.
		if(((buff[i].sampleInV < lower_threshold) && (buff[i+1].sampleInV > lower_threshold)) && !checkDip){ 
			//increment # dips, and set checkDip to 1 to indicate we are in a dip.
			count++;
			checkDip = 1;
		}
		//if the voltage is above the threshold AND if check dip is 1, that means we are in a dip.
		//we set checkDip to zero, to indicate we are no longer in a dip.
		else if((buff[i].sampleInV > lower_threshold) && checkDip){
			checkDip = 0;
		}
	}
	return count;
}




int main(){	
	//initialize
	int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

	// Start sampling thread
    Sampler_startSampling();
	
	while(1){
		double readingX = Joystick_readX();
        double readingY = Joystick_readY();

		//Joystick is CENTER
		if((readingX < (3*(A2D_MAX_READING/4))) && ((readingX > (A2D_MAX_READING/4))) && (readingY < (3*(A2D_MAX_READING/4))) && ((readingY > (A2D_MAX_READING/4)))){
			displayInteger(i2cFileDesc, numberOfDips);
			sleepForMs(500);
		}

		//Joystick is DOWN: display min voltage on LED matrix
		if(readingX >= (3*(A2D_MAX_READING/4))){
			double minVolt = Sampler_computeMinVolt(samplesBuff);
			displayFloat(i2cFileDesc, minVolt);
            sleepForMs(500);
        }

		//Joystick is UP: display max voltage on LED matrix
        if(readingX <= (A2D_MAX_READING/4)){
			double maxVolt = Sampler_computeMaxVolt(samplesBuff);
			displayFloat(i2cFileDesc, maxVolt);
            sleepForMs(500);
        }

		//Joystick is LEFT: display min interval on LED matrix
        if(readingY >= (3*(A2D_MAX_READING/4))){
			double minInterval = Sampler_minInterval();
			displayFloat(i2cFileDesc, minInterval);
            sleepForMs(500);
        }

		//Joystick is RIGHT: display max interval on LED matrix
        if(readingY <= (A2D_MAX_READING/4)){
			double maxInterval = Sampler_maxInterval();
			displayFloat(i2cFileDesc, maxInterval);
            sleepForMs(500);
        }

		//if button is pressed, exit
		if(!userButton()){
			printf("User Button pressed. SHUTTING DOWN.\n");
			break;
		}


	}
    Sampler_stopSampling();
	close(i2cFileDesc);

	int numSamplesInHistory = Sampler_getNumSamplesInHistory();
	
	//Print out the # samples left in recent history (# samples left in our buffer) and the total number of samples taken over the course of the program running.
	printf("Number of samples in History (Current samples buffer): %d\n", numSamplesInHistory);    
	printf("Number of samples Taken over entire run time: %lld\n", Sampler_getNumSamplesTaken()); 

    return 0;
}
