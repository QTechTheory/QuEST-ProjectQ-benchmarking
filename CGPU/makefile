#======================================================================#
#                                                                      #
#      Makefile -- build the qubit function library                    #
#                                                                      #
#======================================================================#

#
# --- COMMON CONFIG
#

# COMPILER options: GNU, INTEL
CC = nvcc
EXE = RandomCircuitBenchmarker
MY_FILE_NAME = RandomCircuitBenchmarker
USE_MPI=0
USE_OPENMP=1
QUEST_DIR = ../../QuEST_GPU
CTOOLS_DIR = ../../CToolsCpp

#
# --- compiler
#

# suppressing deprecated GP target warning thing
CFLAGS	   = -dc
CLFLAGS	   = -O2 -arch=compute_35 -code=sm_35 -lineinfo


#
# --- targets
#
OBJ = $(MY_FILE_NAME).o qubits.o qubits_env_localGPU.o
OBJ += RandomCircuit.o
OBJ += memorymeasure.o
OBJ += mmaformatter.o


#
# --- rules
#
%.o: %.cu
	$(CC) $(CFLAGS) $(CLFLAGS) $(CFLAGS_OMP) $<
%.o: %.cpp
	$(CC) $(CFLAGS) $(CLFLAGS) $(CFLAGS_OMP) $<

%.o: $(QUEST_DIR)/%.cu
	$(CC) $(CFLAGS) $(CLFLAGS) $(CFLAGS_OMP) $<
%.o: $(QUEST_DIR)/%.cpp
	$(CC) $(CFLAGS) $(CLFLAGS) $(CFLAGS_OMP) $<

%.o: $(CTOOLS_DIR)/%.cpp
	$(CC) $(CFLAGS) $(CLFLAGS) $(CFLAGS_OMP) $<

#
# --- build
#
default:	$(EXE)

$(EXE):		$(OBJ)
		$(CC) $(CFLAGS) $(CFLAGS_OMP) -o $(EXE) $(OBJ)

.PHONY:		clean veryclean
clean:
		/bin/rm -f *.o $(EXE)
veryclean:	clean
		/bin/rm -f *.h~ *.c~ makefile~
