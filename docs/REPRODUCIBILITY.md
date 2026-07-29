# Reproducibility

## Prerequisites

- Python 3.10+
- SWI-Prolog (for Prolog validation queries)
- NGSpice / Spectre / Xyce (for analog simulation)
- pytest

## Steps

### 1. Validate Prolog topology

```bash
swipl -q -s logic/prolog/topology.pl -s logic/prolog/schema.pl
```

Expected output — no errors, facts load cleanly.

### 2. Generate Verilog-A netlist

```bash
python netlister/emit_veriloga.py --spec logic/prolog/examples/three_cell_mesh.pl --output veriloga/exo_mesh_tb.va
```

Expected output — netlister prints parsed facts and validation pass.

### 3. Run Python tests

```bash
python -m pytest tests/ -v
```

Expected result — **8/8 tests passing**.

### 4. Run analog simulation

```bash
# NGSpice
cd simulations/ngspice
ngspice -b ../../veriloga/exo_mesh_tb.va
```

Or with the simulation runner:

```bash
bash scripts/run_sim.sh ngspice
```

## Expected v0.1 Results

| Step | Check | Expected |
|------|-------|----------|
| 1 | Prolog topology validates | Facts load, all ports bound |
| 2 | Netlister emits mesh | 3-cell testbench generated |
| 3 | Python tests pass | 8/8 passing |
| 4 | Simulator produces traces | Skew, droop, threshold-margin waveforms |

## Simulation Metrics (Pass Criteria)

| Metric | Threshold |
|--------|-----------|
| Skew (cell 0 vs cell 2) | < 125 ps |
| Voltage droop (farthest cell) | V_peak > 0.6 V |
| False trigger (blocked path) | V_off < 0.1 V |
| Monte Carlo noise margin | Zero false triggers across σ = 20 mV |
