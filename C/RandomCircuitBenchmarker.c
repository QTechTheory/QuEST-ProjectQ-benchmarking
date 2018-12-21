/** @file 
 * Benchmarks the random circuit algorithm
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#include "RandomCircuit.h"
#include "../../QuEST/QuEST/qubits.h"
#include "../../CTools/memorymeasure.h"
#include "../../CTools/mmaformatter.h"


const char* INPUT_KEYS[] = {
	"platform", "framework", "filename",
	"numThreads", "circuitDepth", "numQubits", "numRepetitions"
};
const char* OUTPUT_KEYS[] = {
	"durations", "normErrors",
	"currRealMemories", "peakRealMemories",
	"currVirtMemories", "peakVirtMemories"
};


void repeatedlyTimeRandomCircuit(
	QuESTEnv env, int numQubits, 
	int circuitDepth, int numRepetitions,
	double *avgDuration, double *varDuration) {
	
	// prepare qubits
	MultiQubit qubits;
	createMultiQubit(&qubits, numQubits, env);
	
	// repeat algorithm, recording duration
	long double duration;
	double durationSum = 0;
	double durationSquaredSum = 0;
	for (int i=0; i < numRepetitions; i++) {
		duration = applyRandomCircuit(qubits, circuitDepth);
		durationSum += duration;
		durationSquaredSum += pow(duration,2);
	}
		
	// compute average and variance
	*avgDuration = durationSum / numRepetitions;
	*varDuration = (durationSquaredSum / numRepetitions
					 - pow(*avgDuration, 2));
	
	// free qubits
	destroyMultiQubit(qubits, env);
}


void timeRandomCircuitOverNumQubits(
	int minNumQubits, int maxNumQubits, 
	int circuitDepth, int numRepetitions,
	char *filename) {
	
	// prepare simulator
	QuESTEnv env;
	initQuESTEnv(&env);
	
	// write headers to file
	FILE *file = fopen(filename, "w");
	fprintf(file, "numQubits,circDepth,numReps,avgDur(s),varDur(s^2)\n");
	fclose(file);

	// repeat simulation, changing the number of qubits
	double avgDur, varDur;
	for (int numQubits=minNumQubits; numQubits<=maxNumQubits; numQubits++) {
		
		printf("%d/%d qubits\n", numQubits, maxNumQubits);
		
		// measure duration and varinace
		repeatedlyTimeRandomCircuit(
			env, numQubits, circuitDepth, numRepetitions,
			&avgDur, &varDur
		);
		
		// append measurements to file
		file = fopen(filename, "a");
		fprintf(
			file,
			"%d,%d,%d,%f,%f\n",
			numQubits,circuitDepth,numRepetitions,avgDur,varDur
		);
		fclose(file);
	}
	
	// free memory
	closeQuESTEnv(env);
}


int mainOld(int narg, char* varg[]) {
	
	// RCB minQB, maxQB, circDepth, numReps, filename
	if (narg != 6) {
		printf(
			"ERROR! call as RandomCircuitBenchmarker "
			"minNumQubits maxNumQubits circuitDepth " 
			"numRepetitions filename\n"
		);
		return 1;
	}
	
	timeRandomCircuitOverNumQubits(
		atoi(varg[1]), atoi(varg[2]), atoi(varg[3]), atoi(varg[4]), 
		varg[5]
	);
	
	return 0;
}


void writeInputsToFile(
	FILE* file,
	char* platform,
	char* framework,
	char* filename,
	int numThreads,
	int circuitDepth,
	int numQubits,
	int numRepetitions,
	unsigned long rSeed) {
	
	writeStringToDict(file, "platform", platform);
	writeStringToDict(file, "framework", framework);
	writeStringToDict(file, "filename", filename);
	writeIntToDict(file, "numThreads", numThreads);
	writeIntToDict(file, "circuitDepth", circuitDepth);
	writeIntToDict(file, "numQubits", numQubits);
	writeIntToDict(file, "numRepetitions", numRepetitions);
	fprintf(file, "\"randomSeed\" -> %lu,\n", rSeed);
}


unsigned long hashParams(
	char* platform,
	char* framework,
	char* filename,
	int numThreads,
	int circuitDepth,
	int numQubits,
	int numRepetitions) {
	
	unsigned long hash = 5381;
	int c;
	while ((c = *platform++))
		hash = ((hash << 5) + hash) + c;
	while ((c = *framework++))
		hash = ((hash << 5) + hash) + c;
	while ((c = *filename++))
		hash = ((hash << 5) + hash) + c;
		
	hash = ((hash << 5) + hash) + numThreads;
	hash = ((hash << 5) + hash) + circuitDepth;
	hash = ((hash << 5) + hash) + numQubits;
	hash = ((hash << 5) + hash) + numRepetitions;
	return hash;
}


void mytestfunc(unsigned long *a) {
	*a = 20;
	
	*a *= 4;
}


int main(int narg, char* varg[]) {
	
	
	/*
	 * prepare simulation parameters
	 */
	 
	// check all cmd arguments were passed
	int numInpKeys = sizeof(INPUT_KEYS)/sizeof(char*);
	if (narg-1 != numInpKeys) {
		printf("Call as ./RandomCircuitBenchmarker");
		for (int i=0; i < numInpKeys; i++)
			printf(" %s", INPUT_KEYS[i]);
		return EXIT_FAILURE;
	}
	
	// read in cmd args, int'ing some of them
	char* platform		= varg[1];
	char* framework		= varg[2];
	char* filename		= varg[3];
	int numThreads		= atoi(varg[4]);
	int circuitDepth		= atoi(varg[5]);
	int numQubits		= atoi(varg[6]);
	int numRepetitions	= atoi(varg[7]);
	
	// generate a random seed from hashing params
	unsigned long rSeed = hashParams(
		platform, framework, filename,
		numThreads, circuitDepth, numQubits, numRepetitions);
	
	// create the output file (to indicate launch)
	FILE* file = openDictWrite(filename);
	writeInputsToFile(file, 
		platform, framework, filename, 
		numThreads, circuitDepth, numQubits, numRepetitions, rSeed);
	closeDictWrite(file);
	
	// display simulation params
	printf("Simulating random circuit with params:\n");
	printf("platform:\t%s\n", platform);
	printf("framework:\t%s\n", framework);
	printf("filename:\t%s\n", filename);
	printf("numThreads:\t%d\n", numThreads);
	printf("circuitDepth:\t%d\n", circuitDepth);
	printf("numQubits:\t%d\n", numQubits);
	printf("numRepetitions:\t%d\n", numRepetitions);
	printf("randomSeed:\t%lu\n", rSeed);
	
	
	/*
	 * prepare quantum simulator and data structutes
	 */
	
	// seed RandomCircuits
	srand(rSeed);
	
	// construct simulator
	QuESTEnv env;
	initQuESTEnv(&env);
	MultiQubit qubits;
	createMultiQubit(&qubits, numQubits, env);
	
	double *durations  = malloc(numRepetitions * sizeof(double));
	double *normErrors = malloc(numRepetitions * sizeof(double));
	
	int memArrSize = numRepetitions * sizeof(unsigned long);
	unsigned long *currRealMems  = malloc(memArrSize);
	unsigned long *peakRealMems  = malloc(memArrSize);
	unsigned long *currVirtMems  = malloc(memArrSize);
	unsigned long *peakVirtMems  = malloc(memArrSize);
	
	// clear arrays (so that on sys that don't support memory measure.
	// the output is less confusing)
	for (int i=0; i < numRepetitions; i++) {
		currRealMems[i] = currVirtMems[i] = 0;
		peakRealMems[i] = peakVirtMems[i] = 0;
	}
	
	
	/* 
	 * simulate random circuit and collect data
	 */
	 	 
	for (int trial=0; trial < numRepetitions; trial++) {
		
		durations[trial] = (double) applyRandomCircuit(qubits, circuitDepth);
		normErrors[trial] = 1 - calcTotalProbability(qubits);
		getMemory(
			&currRealMems[trial], &peakRealMems[trial],
			&currVirtMems[trial], &peakVirtMems[trial]);
		
		printf("dur: %fs, error: %2.e,  ",
			   durations[trial], normErrors[trial]);
		printf("realMem: %lub (peak: %lub), ",
			   currRealMems[trial], peakRealMems[trial]);
		printf("virtMem: %lub (peak: %lub)\n",
			   currVirtMems[trial], peakVirtMems[trial]);
	}
	
	
	/*
	 * write collected data to file
	 */
	file = openDictWrite(filename);
	writeInputsToFile(file, 
		platform, framework, filename, 
		numThreads, circuitDepth, numQubits, numRepetitions, rSeed);
	writeDoubleArrToDict(file, "durations", durations, numRepetitions, 5);
	writeDoubleArrToDict(file, "normErrors", normErrors, numRepetitions, 5);
	writeUnsignedLongArrToDict(file, "currRealMemories", currRealMems, numRepetitions);
	writeUnsignedLongArrToDict(file, "peakRealMemories", peakRealMems, numRepetitions);
	writeUnsignedLongArrToDict(file, "currVirtMemories", currVirtMems, numRepetitions);
	writeUnsignedLongArrToDict(file, "peakVirtMemories", peakVirtMems, numRepetitions);
	closeDictWrite(file);
	
	
	/* 
	 * free memory 
	 */
	 
	destroyMultiQubit(qubits, env);
	closeQuESTEnv(env);
	
	
	free(durations);
	free(normErrors);
	free(currRealMems);
	free(peakRealMems);
	free(currVirtMems);
	free(peakVirtMems);
	
	return EXIT_SUCCESS;
}