# Simulation Verification Checks

## Critical Measurements

### 1. Skew Budget

```
Measurement: V(op_gate_X) rising edge crossing VTH
Requirement: max_skew < PW / 4
```

If bus RC delay skews the pulse arrival, downstream cells see a narrowed or attenuated pulse — resulting in logic metastability.

### 2. Voltage Droop (IR Drop)

```
Measurement: Peak V(op_gate) at farthest cell vs nearest cell
Requirement: V_peak > VTH_next_stage
```

If multiple cells conduct simultaneously, the shared bus current draw causes IR drop. Worst-case is all cells conducting at once.

### 3. Threshold Noise Margin

```
Measurement: Monte Carlo sweep on VTH_NOISE (σ = 20 mV)
Requirement: Zero false triggers on ROFF paths
```

Process variation in threshold voltages can cause blocked cells to partially conduct. Verify op_gate stays below 0.1 V on blocked paths across all Monte Carlo runs.

## Pass/Fail Criteria

| Check | Metric | Pass | Fail |
|-------|--------|------|------|
| Skew | Δt_skew | < 125 ps | >= 125 ps |
| Droop | V_peak | > 0.6 V | <= 0.6 V |
| Noise | V_off_blocked | < 0.1 V | >= 0.1 V |
| Jitter | σ_J | < 5 ps RMS | >= 5 ps RMS |

## Simulation Manifest Schema

```json
{
  "simulation": {
    "topology": "three_cell_mesh",
    "simulator": "ngspice",
    "timestamp": "2026-07-04T22:00:00Z",
    "results": {
      "skew_02": "45 ps",
      "v_peak_cell_0": "1.72 V",
      "v_peak_cell_1": "1.68 V",
      "v_peak_cell_2": "1.61 V",
      "v_off_blocked": "0.03 V",
      "passed": true
    }
  }
}
```
