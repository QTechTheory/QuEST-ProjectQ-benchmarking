# tidy up using existing makefile
make clean

# compile (as makefile does)
nvcc -dc -O2 -arch=compute_35 -code=sm_35 -lineinfo  RandomCircuitBenchmarker.cpp
nvcc -dc -O2 -arch=compute_35 -code=sm_35 -lineinfo  ../../QuEST_GPU/qubits.cpp
nvcc -dc -O2 -arch=compute_35 -code=sm_35 -lineinfo  ../../QuEST_GPU/qubits_env_localGPU.cu
nvcc -dc -O2 -arch=compute_35 -code=sm_35 -lineinfo  RandomCircuit.cpp
nvcc -dc -O2 -arch=compute_35 -code=sm_35 -lineinfo  ../../CToolsCpp/memorymeasure.cpp
nvcc -dc -O2 -arch=compute_35 -code=sm_35 -lineinfo  ../../CToolsCpp/mmaformatter.cpp

# manually link
nvcc -O2 -arch=compute_35 -code=sm_35 -lineinfo -o RandomCircuitBenchmarker RandomCircuitBenchmarker.o qubits.o qubits_env_localGPU.o RandomCircuit.o memorymeasure.o mmaformatter.o

# tidy up files
rm -f *.o