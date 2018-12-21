#!/bin/bash

#SBATCH --job-name RC_GPU1
#SBATCH --output=RC_GPU1_output.txt
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --gres=gpu:1             ##### numgpus

##SBATCH --time=01:30:00
##SBATCH --array=0-57
##SBATCH --partition=gpu

## TEST
#SBATCH --time=5:00:00
#SBATCH --array=0-1
#SBATCH --partition=gpu

module purge
module load gpu/cuda
export OMP_NUM_THREADS=16

##qubits_values=($( seq 1 1 29 ))
##depth_values=( 100 1 )
##
##trial=${SLURM_ARRAY_TASK_ID}
##qubits=${qubits_values[$(( trial % ${#qubits_values[@]} ))]}
##trial=$(( trial / ${#qubits_values[@]} ))
##depth=${depth_values[$(( trial % ${#depth_values[@]} ))]}

qubits=$(( 28 + ${SLURM_ARRAY_TASK_ID} ))
depth=100


numgpus=1
threads=16
repetitions=50
platform="ARC_devel"
framework="QuEST_GPU"
filename="benchmark_results/${platform}_${framework}/threads${threads}/numGPUs${numgpus}/depth${depth}/qubits${qubits}.txt"

mkdir -p $filename   ## makes dir and file
rm -r $filename      ## removes file, keeps dir

./RandomCircuitBenchmarker $platform $framework $filename $threads $depth $qubits $repetitions
