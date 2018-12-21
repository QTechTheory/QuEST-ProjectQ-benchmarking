/** @file 
 * Applies a random circuit
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <sys/time.h>

#include "RandomCircuit.h"
#include "../../QuEST_GPU/qubits.h"

const long double SQRT2 = 	1.41421356237309504880168872420969807856967187537694;

enum gateID {X, Y, T, H, C, None};


int getRandomInteger(int min, int max) {
	return min + rand()/(RAND_MAX/(max - min + 1) + 1);
}


enum gateID pickRandomGate(enum gateID prevGate) {
	
	int choice = getRandomInteger(0,1);
	switch(prevGate) {
		case T:  return (choice)? X:Y;
		case X:  return (choice)? Y:T;
		case Y:  return (choice)? T:X;
		default: break;
	}
	
	choice = getRandomInteger(0,2);
	if (choice == 0)
		return X;
	if (choice == 1)
		return Y;
	return T;
}


long double applyRandomCircuit(MultiQubit qubits, QuESTEnv env, int circuitDepth) {
	
	// calls functions:    GPU:
	// initStatePlus		[X]
	// controlPhaseGate	[X]
	// tGate				[ ] *replaced: see below
	// rotateQubit			[X]
	// syncQuESTEnv		[X]
	
	// tGate = {{1, 0}, {0, e^(i pi/4)}}
	//       ~ e^(-i pi/8) tGate
	//       = {{a, b}, {-b, a*}}
	// where a=exp(-i pi/8), b=0
	// so tGate ~ rotateQubit(exp(-i pi/8), 0)
	// in Cartesian form:
	// exp(-i pi/8) = 0.9238795325112867 - 0.3826834323650898 I
	
	/*
	 * PREPARE DATA STRUCTURES
	 */
	 
	// remembers previous gates
	int numQubits = qubits.numQubits;
	int *hasTGate = (int *) malloc(numQubits * sizeof(int));
	enum gateID *prevGates = (enum gateID *) malloc(numQubits * sizeof(enum gateID));
	enum gateID *prevSings = (enum gateID *) malloc(numQubits * sizeof(enum gateID));
	enum gateID *currGates = (enum gateID *) malloc(numQubits * sizeof(enum gateID));
	int i;
	for (i=0; i < numQubits; i++) {
		hasTGate[i] = 0;
		prevGates[i] = None;
		prevSings[i] = T;
		currGates[i] = H;
	}
	
	// arguments for X^1/2 and Y^1/2 qubit rotations
	Complex alphaXY; 
	alphaXY.real = 1/SQRT2; alphaXY.imag = 0;
	Complex betaX, betaY;
	betaX.real = 0; 		betaX.imag = -1/SQRT2;
	betaY.real = 1/SQRT2; 	betaY.imag = 0;
	
	// arguments for tGate rotation
	Complex alphaT, betaT;
	alphaT.real = 0.9238795325112867;
	alphaT.imag = -0.3826834323650898;
	betaT.real = 0; betaT.imag = 0;
	
	/*
	 * PERFORM RANDOM GATE ALGORITHM
	 */
	 
	// start timing execution
	struct timeval timeInstance;
	gettimeofday(&timeInstance, NULL);
	long double startTime = (
		timeInstance.tv_sec + 
		(long double) timeInstance.tv_usec/pow(10,6));
	
	// populate all states
	initStatePlus(&qubits);
	
	int cStartInd = -1;
	int depth;
	for (depth=0; depth < circuitDepth; depth++) {
		
		// update gates
		int i;
		for (i=0; i < numQubits; i++) {
			prevGates[i] = currGates[i];
			currGates[i] = None;
		}
		
		// apply CZs
		cStartInd += 1;
		cStartInd %= 3;
		int n;
		for (n=cStartInd; n<numQubits-1; n+=3) {
			controlPhaseGate(qubits, n, n+1);
			currGates[n] = currGates[n+1] = C;
		}
		
		// iterate only the not-C-gated qubits
		for (n=0; n < numQubits; n++) {
			
			if (currGates[n] != None || prevGates[n] != C)
				continue;
			
			// apply TGate gates
			if (!hasTGate[n]) {
				//tGate(qubits, n);
				rotateQubit(qubits, n, alphaT, betaT);
				hasTGate[n] = 1;
				prevSings[n] = T;
				currGates[n] = T;
			}
			// appply random gates
			else {
				enum gateID gate = pickRandomGate(prevSings[n]);
				prevSings[n] = gate;
				currGates[n] = gate;
				if (gate == T)
					// tGate(qubits, n);
                                        rotateQubit(qubits, n, alphaT, betaT);
				if (gate == X)
					rotateQubit(qubits, n, alphaXY, betaX);
				if (gate == Y)
					rotateQubit(qubits, n, alphaXY, betaY);
			}
		}
	}
	
	/* SYNCH QUEST OFF GPU */
	syncQuESTEnv(env);
	
	// stop timing execution
	gettimeofday(&timeInstance, NULL);
	long double endTime = (
		timeInstance.tv_sec + 
		(long double) timeInstance.tv_usec/pow(10,6));
	
	
	/*
	 * FREE MEMORY
	 */
	
	free(hasTGate);
	free(prevGates);
	free(prevSings);
	free(currGates);
	
	return endTime - startTime;
}


int mainRandomCircuitPrivate (int narg, char *varg[]) {
	
	
	/*
	 * GET PARAMETERS
	 */
	 
	// RandomCircuit rSeed, numQubits, circDepth
	if (narg != 4) {
		printf("ERROR: Call as RandomCircuit seed "
			   "numQubits circuitDepth\n");
		return 1;
	}
	
	int rSeed = atoi(varg[1]);
	int numQubits = atoi(varg[2]);
	int circuitDepth = atoi(varg[3]);
	
	
	/*
	 * SIMULATE RANDOM CIRCUIT
	 */
	
	// ensure circuit is random
	srand(rSeed);
	 
	// load QuEST, allocate qubits
	QuESTEnv env;
	initQuESTEnv(&env);
	MultiQubit qubits; 
	createMultiQubit(&qubits, numQubits, env);
	
	// time application of random circuit
	long double duration = applyRandomCircuit(qubits, env, circuitDepth);
	printf(
		"took %Lfs to apply circuit of depth %d on %d qubits\n",
		duration, circuitDepth, numQubits);
	
	
	/*
	 * FREE MEMORY
	 */
	
	destroyMultiQubit(qubits, env); 
	closeQuESTEnv(env);
	return 0;
}
