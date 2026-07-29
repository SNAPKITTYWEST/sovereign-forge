# Architecture

## Five-Layer Stack

```
Layer 5: WORM Provenance                  worm/receipts/
Layer 4: Simulation Verification          Spectre / Xyce / NGSpice
Layer 3: Verilog-A Mesh                   exo_cell, sigma_source, lossy_bus_segment
Layer 2: Prolog -> Netlist Compiler       netlister/parser.py + emit_veriloga.py
Layer 1: Prolog Topology Specification    binds/2, valid_operator/2, env_state/2
```

## Key Components

### Σ(t) Pulse Generator
Global analog pulse train (10 MHz default) with configurable jitter (σ_J = 5 ps RMS) and source impedance (ZOUT = 50 Ω). Drives the bus distribution network.

### Lossy Bus
Pi-model RC segments (R_PER_UM = 0.1 Ω/μm, C_PER_UM = 0.2 fF/μm, L_PER_UM = 0.2 pH/μm) modeling on-chip interconnect parasitics. Default segment length = 500 μm.

### Exo Cell
Voltage-controlled conductance switch:
- P-path: conducts when port voltage > VTH
- PN-path: conducts when port voltage < VTH
- RON = 50 Ω, ROFF = 1 TΩ
- CIN = 2 fF input loading on Σ bus
- VTH_NOISE = 20 mV (Monte Carlo process variation)

### Netlister
Reads Prolog KB, validates topology, emits structural Verilog-A with correct instance names, bus topology, and environment stimulus.
