
from projectq          import MainEngine  # to simulate quantum computer
from projectq.backends import Simulator   # to give simulator the seed and gate-fuse flag
from projectq.ops      import Measure     # to deallocate qubits before exiting

from random_circuit import apply_random_circuit # to generate random circuits
from memory_measure import get_memory           # to track memory usage
from outputter      import save_to_mma          # to save as a Mathematica file

import random   # to control random seed in projectq and random_circuit
import sys      # to get cmd args



# cmd arguments to pass
INPUT_KEYS = [
    'platform', 'framework', 'filename',                         # strings
    'numThreads', 'circuitDepth', 'numQubits', 'numRepetitions'  # ints
]

# data keys in mma output
OUTPUT_KEYS = [
    'durations',
    'currRealMemories', 'peakRealMemories',
    'currVirtMemories', 'peakVirtMemories',
    'normErrors'
]



if __name__ == '__main__':

    # we use a dictionary comprehension available in py2.7+
    assert sys.version_info >= (2, 7)
    

    '''
    prepare simulation parameters
    '''

    # check all cmd arguments were passed
    if len(sys.argv)-1 != len(INPUT_KEYS):
        print('Call as python random_circuit_benchmarker.py ' +
              ' '.join(INPUT_KEYS))
        exit()

    # read in cmd args, int'ing some of them
    params = {}
    for ind, key in enumerate(INPUT_KEYS):
        params[key] = sys.argv[ind+1]
    for key in INPUT_KEYS[3:]:
        params[key] = int(params[key])

    # create the output file (to indicate launch)
    save_to_mma(params, params['filename'])
    
    # display simulation params
    print("Simulating random circuit with params:\n" +
          '\n'.join('%s:\t%s' % (key, str(params[key])) for key in INPUT_KEYS))


    '''
    prepare quantum simulator
    '''

    # seed based on passed params (limit to C max int size)
    rseed = hash(params.values()) % 4294967295
    random.seed(rseed)

    # use all compilers and gate-fuse only on multicores
    engine = MainEngine(backend=Simulator(
        rnd_seed=rseed,
        gate_fusion=(params['numThreads']>1) ))
    qubits = engine.allocate_qureg(params['numQubits'])
    engine.flush()


    '''
    simulate random circuit and collect data
    '''

    # maps output key name to list of data
    output_data = {key:[] for key in OUTPUT_KEYS}

    # repeatedly simulate random circuits
    for rep in range(params['numRepetitions']):

        # record time to apply circuit (resets psi0 to |0> initially)
        dur = apply_random_circuit(qubits, params['circuitDepth'], engine)

        # record memory usage
        mem = get_memory()

        # get wavefunc norm (without overloading memory)
        norm = sum(engine.backend.get_probability(bit, qubits[0:1])
                   for bit in ['0', '1'])

        # record data
        output_data['durations'].append(dur)
        output_data['currRealMemories'].append(mem['VmRSS'])
        output_data['currVirtMemories'].append(mem['VmSize'])
        output_data['peakRealMemories'].append(mem['VmHWM'])
        output_data['peakVirtMemories'].append(mem['VmPeak'])
        output_data['normErrors'].append(1 - norm)

        # display data to logs
        print(("dur: %fs, error: %2.e, " +
               "realMem: %lub (peak: %lub), " +
               "virtMem: %lub (peak: %lub)") % (
                dur, 1-norm,
                mem['VmRSS'], mem['VmHWM'], mem['VmSize'], mem['VmPeak']))

        # write so-far data to file (in case numRepetitions is very large)
        save_to_mma(
            {**params, **output_data},
            params['filename'],
            key_order = INPUT_KEYS + OUTPUT_KEYS)

    # measure qubits to allow deallocation
    Measure | qubits
    del qubits
    
