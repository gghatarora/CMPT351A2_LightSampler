// sampler.h
// Module to sample light levels in the background (thread).
// It provides access to the raw samples and then deletes them.
#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#define A2D_FILE_VOLTAGE1  "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define A2D_VOLTAGE_REF_V  1.8
#define A2D_MAX_READING    4095
#define SAMPLE_BUFF_SIZE 2000

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    double sampleInV;
    long long timestampInNanoS;
} samplerDatapoint_t;

// Begin/end the background thread which samples light levels.
void Sampler_startSampling(void);
void Sampler_stopSampling(void);

// our running thread.  Performs sampling logic.
void* Sampler_storeSample(void);

// Helper for min/max interval.  Computes the difference in timestamps between two samples.
double Sampler_computeIntervalInMS(int index);

//Compute min/max/average intervals
//Average is computed using exponential smoothing
double Sampler_minInterval();
double Sampler_maxInterval();
double Sampler_avgInterval();

//Compute min/max/average sample voltages
double Sampler_computeAvgVolt(samplerDatapoint_t* buff, double currentAvg);
double Sampler_computeMinVolt(samplerDatapoint_t* buff);
double Sampler_computeMaxVolt(samplerDatapoint_t* buff);


//compute number of dips
//Use threshold of 0.1V below average voltage, and do not count another dip until after voltage level is above the threshold again.
//Use hysteresis of 0.3V to nullify noise and prevent retriggering.
int Sampler_computeDips(samplerDatapoint_t* buff, double average);


// Get a copy of the samples in the sample history, removing values
// from our history here.
// Returns a newly allocated array and sets `length` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: function provides both data and size to ensure consistency.
samplerDatapoint_t* Sampler_extractAllValues(int *length);

// Returns how many valid samples are currently in the history.
int Sampler_getNumSamplesInHistory();

// Get the total number of light level samples taken so far.
long long Sampler_getNumSamplesTaken(void);



#endif
