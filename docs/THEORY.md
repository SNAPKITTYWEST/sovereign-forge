# Exo-Synchronicity: Theory

## The Filter Mesh Theorem

**Definition.** Let Σ(t) be a global analog pulse train with frequency f, pulse width PW, and jitter σ_J. Let P = {p_1, ..., p_n} be physical ports, each bound to an environmental observable. Let L = {ℓ_1, ..., ℓ_n} be latches with polarity π ∈ {P, PN} and threshold VTH.

A **filter mesh** M = (P, L, W) consists of ports, latches, and a wiring diagram W (directed edges from P × L to operator gates G).

**Theorem.** Operator g ∈ G fires on Σ(t_i) iff there exists a conducting path from Σ(t_i) to g through M under environmental state E(t_i) = {V(p_1, t_i), ..., V(p_n, t_i)}. The mesh computes a function F: E → 2^G without stored state, instruction fetch, or sequential flow. Computation is bounded only by analog propagation delay.

## Corollaries

### Corollary 1: No State
The mesh has zero memory. Latch state is purely a function of instantaneous E(t) and fixed wiring W. There is no distinction between program and data — both are topology.

### Corollary 2: Polarity Duality
Operator activation is a monotone Boolean function of threshold crossings:

```
activate(g) iff AND_i (π(p_i) = P -> V(p_i) > VTH) AND (π(p_i) = PN -> V(p_i) < VTH)
```

### Corollary 3: Swarm Coherence
For cells c_i, c_j in the same Σ(t) domain, skew Δt = |t_rise(c_i) - t_rise(c_j)| is bounded by:

```
Δt ≤ t_prop_max + σ_J + RC_mismatch
```

No consensus protocol is required — physics guarantees synchronization.

## Open Problems

1. **Topology Synthesis.** Given activation table T: E → 2^G, find minimal mesh M. (NP-hard, reduction from MONOTONE-SAT.)

2. **Skew Budget Closure.** Given M and Δt_max, determine if routing exists within area constraints. (NP-complete, reduction from GRID-EMBEDDING.)

3. **Impedance Matching.** Given cells with varying CIN, find placement minimizing reflection-induced false triggers. (P, dynamic programming on tree topologies.)
