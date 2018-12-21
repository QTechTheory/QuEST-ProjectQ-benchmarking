#!/bin/env bash

#SBATCH --array=0-109
#SBATCH --job-name=projqThreads
#SBATCH --output=output.txt
#SBATCH --mem=60GB
#SBATCH --time=0-100:0:0
#SBATCH --nodes=1
#SBATCH --cpus-per-task=16
#SBATCH --reservation=nqit

depth_values=( 10 20 30 40 50 60 70 80 90 100 )
threads_values=( 3 5 6 7 9 10 11 12 13 14 15 )

trial=${SLURM_ARRAY_TASK_ID}
threads=${threads_values[$(( trial % ${#threads_values[@]} ))]}
trial=$(( trial / ${#threads_values[@]} ))
depth=${depth_values[$(( trial % ${#depth_values[@]} ))]}

source ../../prep.sh
export OMP_NUM_THREADS=$threads
export OMP_PROC_BIND=spread

qubits=30
reps=10
platform="ARC"
framework="ProjectQ"
filename="benchmark_results/${platform}_${framework}/threads${threads}/depth${depth}/qubits${qubits}.txt"

python random_circuit_benchmarker.py $platform $framework $filename $threads $depth $qubits $reps