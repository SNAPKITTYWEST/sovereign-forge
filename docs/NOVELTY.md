# Novelty Claim

## The Prolog -> Analog Netlist Bridge

The research contribution is the **compilation of logical topology facts into analog filter meshes** whose validity is verified by physical simulation rather than logical inference.

| Prior Art | This Work |
|-----------|-----------|
| Software-defined networking (SDN) | Topology-defined computation |
| FPGA reconfiguration | Fixed analog mesh, environment-configured |
| Neuromorphic computing (spiking) | Voltage-threshold gating, no learning |
| Logic-in-memory | No memory — conduction only |
| Formal verification (model checking) | Analog simulation (SPICE-level) |

## Distinguishing Properties

1. **Zero stored state.** Latch states are purely physical (capacitive/flux). No registers, no flip-flops, no memory elements.

2. **No instruction cycle.** Σ(t) pulse propagation *is* the computation. There is no fetch-decode-execute.

3. **Environment as computer.** Environmental observables are not inputs to an algorithm — they *configure the conductive paths* that define the algorithm.

4. **Algorithm is layout.** Trace length matching, RC constants, and antenna geometry *are* the program. Crosstalk and phase skew *are* the bugs.

5. **P-time verification.** Any proposed topology can be verified in polynomial time via analog simulation. Finding a correct topology remains NP-hard.

## Relationship to Existing Frameworks

| Model | State | Fetch | Program | Coordination |
|-------|-------|-------|---------|-------------|
| Turing Machine | Tape | Head | Table | — |
| Neural Network | Weights | Fwd pass | Training | — |
| Cellular Automaton | Grid | Rule | Table | Neighbor |
| **Exo-Synchronicity** | **None** | **Wavefront** | **Wiring** | **Physics** |
