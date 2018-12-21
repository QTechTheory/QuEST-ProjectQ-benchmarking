
from projectq.ops import All, H, T, Rx, Ry, Z, C, Measure

from math import pi
import random, time


# prepare gate set
Xroot, Yroot = Rx(pi/2), Ry(pi/2)
Xroot.tex_str = lambda: '$X^{1/2}$'
Yroot.tex_str = lambda: '$Y^{1/2}$'
def pick_gate(prev_gate):
    gates = [Xroot, Yroot, T]          #!!! does paper contain error??
    gates.remove(prev_gate)            # "cycle" selectively includes/excludes C gates
    return random.choice(gates)


def apply_random_circuit(qubits, circ_depth, engine):

    # prepare structures
    num_qubits = len(qubits)
    has_toffo = [False]*num_qubits
    prev_gate = [None]*num_qubits
    prev_sing = [T]*num_qubits
    curr_gate = [H]*num_qubits
    c_start_i = -1

    # don't include init-state in timing
    engine.flush()
    engine.backend.set_wavefunction([1], qubits)
    engine.backend.collapse_wavefunction(qubits, [0]*num_qubits)
    engine.flush()

    # start timing
    start_time = time.time()

     # apply random circuit algorithm
    All(H) | qubits

    for _ in range(circ_depth):

        prev_gate = curr_gate
        curr_gate = [False]*num_qubits
        
        # apply CZs
        c_start_i += 1
        c_start_i %= 3
        for n in range(c_start_i, num_qubits-1, 3):
            C(Z) | (qubits[n], qubits[n+1])
            curr_gate[n] = curr_gate[n+1] = C

        # iterate ungated qubits
        for n in range(num_qubits):
            if curr_gate[n] or prev_gate[n] != C:
                continue

            # apply toffoli gates
            if not has_toffo[n]:
                T | qubits[n]
                has_toffo[n] = True
                prev_sing[n] = T
                curr_gate[n] = T

            # apply random gates
            else:
                gate = pick_gate(prev_sing[n])
                gate | qubits[n]
                prev_sing[n] = gate
                curr_gate[n] = gate

    # apply gates
    engine.flush()

    # ensure simulator empties cache
    engine.backend._simulator.run()

    # stop timing
    end_time = time.time()
    
    return end_time - start_time



def plot_random_circuit_or_distributions(num_qubits, draw_flag):

    import sys; sys.path.append('..')
    from qtools import MainDrawEngine, save_data, add_data
    import matplotlib.pyplot as plt

    engine = MainDrawEngine(draw_flag)
    qubits = engine.allocate_qureg(num_qubits)
    engine.flush()

    # prepare strucs
    binary_nums = [
        format(n, '0%db'%num_qubits)[::-1]
        for n in range(2**num_qubits)
    ]

    if not draw_flag:
        
        distributions = []

        # apply random circuits of varying depths
        for circ_depth in [10**n for n in range(4)]:
            engine.set_wavefunction([1] + [0]*(2**num_qubits - 1), qubits)
            engine.flush()
            apply_random_circuit(qubits, circ_depth)
            engine.flush()

            # record data
            distributions.append(
                list(engine.get_probability(n, qubits) for n in binary_nums))
            plt.plot(
                range(2**num_qubits),
                distributions[-1],
                label='depth %d' % circ_depth)

        Measure | qubits

        plt.xlabel('state')
        plt.ylabel('probability of state')
        plt.title('output distribution of random universal quantum circuits')
        plt.legend()
        plt.show()

    if draw_flag:
        circ_depth = 10
        apply_random_circuit(qubits, circ_depth)
        Measure|qubits
        engine.draw()


def repeatedly_time_random_circuit(engine, num_qubits, circ_depth, num_reps):

    # prepare structures
    qubits = engine.allocate_qureg(num_qubits)
    durationSum = 0
    durationSquaredSum = 0

    # repeat simulation, collecting sums
    for i in range(num_reps):
        dur = apply_random_circuit(qubits, circ_depth, engine)
        durationSum += dur
        durationSquaredSum += dur**2

    # compute average and variance
    avgDur = durationSum / num_reps
    varDur = durationSquaredSum / num_reps - avgDur**2

    Measure | qubits
    return (avgDur, varDur)


def time_random_circuit_over_num_qubits(min_num_qb, max_num_qb, circ_depth, num_reps, filename):

    import sys; sys.path.append('..')
    from qtools import MainDrawEngine, save_data, add_data
    engine = MainDrawEngine()

    # prepare headers
    save_data(filename, ['numQubits','circDepth','numReps','avgDur(s)','varDur(s^2)'])

    # repeat simulation
    for num_qb in range(min_num_qb, max_num_qb+1):
        print("%d/%d qubits" % (num_qb, max_num_qb))

        # measure duration and variance
        avgDur, varDur = repeatedly_time_random_circuit(engine, num_qb, circ_depth, num_reps)

        # append measurements to file
        add_data(filename, [num_qb, circ_depth, num_reps, avgDur, varDur])
    

if __name__ == '__main__':

    if len(sys.argv) != 6:
        print("ERROR call as random_circuit.py minNumQubits maxNumQubits circuitDepth " +
              "numRepetitions filename")
        exit()

    # get params
    minQB, maxQB, circDepth, numReps = map(int, sys.argv[1:5])
    filename = sys.argv[5]

    # begin simulation
    time_random_circuit_over_num_qubits(minQB, maxQB, circDepth, numReps, filename)


