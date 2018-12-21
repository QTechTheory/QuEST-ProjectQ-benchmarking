/** @file 
 * Benchmarks the random circuit algorithm
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#include "RandomCircuit.h"
#include "../../QuEST_GPU/qubits.h"
#include "../../CToolsCpp/memorymeasure.h"
#include "../../CToolsCpp/mmaformatter.h"


const char* INPUT_KEYS[] = {
	"platform", "framework", "filename",
	"numThreads", "circuitDepth", "numQubits", "numRepetitions"
};
const char* OUTPUT_KEYS[] = {
	"durations", "normErrors",
	"currRealMemories", "peakRealMemories",
	"currVirtMemories", "peakVirtMemories"
};



void writeInputsToFile(
	FILE* file,
	const char* platform,
	const char* framework,
	const char* filename,
	int numThreads,
	int circuitDepth,
	int numQubits,
	int numRepetitions,
	unsigned long rSeed) {
	
	writeStringToAssoc(file, "platform", platform);
	writeStringToAssoc(file, "framework", framework);
	writeStringToAssoc(file, "filename", filename);
	writeIntToAssoc(file, "numThreads", numThreads);
	writeIntToAssoc(file, "circuitDepth", circuitDepth);
	writeIntToAssoc(file, "numQubits", numQubits);
	writeIntToAssoc(file, "numRepetitions", numRepetitions);
	fprintf(file, "\"randomSeed\" -> %lu,\n", rSeed);
}


unsigned long hashParams(
	const char* platform,
	const char* framework,
	const char* filename,
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


int main(int narg, char* varg[]) {
	
	
	/*
	 * prepare simulation parameters
	 */
	 
	// check all cmd arguments were passed
	int numInpKeys = sizeof(INPUT_KEYS)/sizeof(char*);
	if (narg-1 != numInpKeys) {
		printf("Call as ./RandomCircuitBenchmarker");
		int i;
		for (i=0; i < numInpKeys; i++)
			printf(" %s", INPUT_KEYS[i]);
                printf("\n");
		return EXIT_FAILURE;
	}
	
	// read in cmd args, int'ing some of them
	char* platform		= varg[1];
	char* framework	= varg[2];
	char* filename		= varg[3];
	int numThreads		= atoi(varg[4]);
	int circuitDepth	= atoi(varg[5]);
	int numQubits		= atoi(varg[6]);
	int numRepetitions	= atoi(varg[7]);
	
	// generate a random seed from hashing params
	unsigned long rSeed = hashParams(
		platform, framework, filename,
		numThreads, circuitDepth, numQubits, numRepetitions);
	
	// create the output file (to indicate launch)
	FILE* file = openAssocWrite(filename);
	writeInputsToFile(file, 
		platform, framework, filename, 
		numThreads, circuitDepth, numQubits, numRepetitions, rSeed);
	closeAssocWrite(file);
	
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
	
	double *durations  = (double *) malloc(numRepetitions * sizeof(double));
	double *normErrors = (double *) malloc(numRepetitions * sizeof(double));
	
	int memArrSize = numRepetitions * sizeof(unsigned long);
	unsigned long *currRealMems  = (unsigned long *) malloc(memArrSize);
	unsigned long *peakRealMems  = (unsigned long *) malloc(memArrSize);
	unsigned long *currVirtMems  = (unsigned long *) malloc(memArrSize);
	unsigned long *peakVirtMems  = (unsigned long *) malloc(memArrSize);
	
	// clear arrays (so that on sys that don't support memory measure.
	// the output is less confusing)
	int i;
	for (i=0; i < numRepetitions; i++) {
		currRealMems[i] = currVirtMems[i] = 0;
		peakRealMems[i] = peakVirtMems[i] = 0;
	}
	
	
	/* 
	 * simulate random circuit and collect data
	 */
	int trial;
	for (trial=0; trial < numRepetitions; trial++) {
		
		durations[trial] = (double) applyRandomCircuit(qubits, env, circuitDepth);
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
	file = openAssocWrite(filename);
	writeInputsToFile(file, 
		platform, framework, filename, 
		numThreads, circuitDepth, numQubits, numRepetitions, rSeed);
	writeDoubleArrToAssoc(file, "durations", durations, numRepetitions, 5);
	writeDoubleArrToAssoc(file, "normErrors", normErrors, numRepetitions, 5);
	writeUnsignedLongArrToAssoc(file, "currRealMemories", currRealMems, numRepetitions);
	writeUnsignedLongArrToAssoc(file, "peakRealMemories", peakRealMems, numRepetitions);
	writeUnsignedLongArrToAssoc(file, "currVirtMemories", currVirtMems, numRepetitions);
	writeUnsignedLongArrToAssoc(file, "peakVirtMemories", peakVirtMems, numRepetitions);
	closeAssocWrite(file);
	
	
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
