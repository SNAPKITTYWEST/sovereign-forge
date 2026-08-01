# DSSSL + Verilog Relational Synthesis Pipeline

Relational hole-filling across two layers:
1. **Verilog hardware**: RTL state machine evaluates the SMT invariant in silicon
2. **DSSSL software**: style sheet fills holes via unification + grove construction rules

Expression: `10 + (?x * 5) = 20`  →  `?x = 2`

---

## Files

| File | Role |
|------|------|
| `synthesis.dsl` | ISO 10179 DSSSL style sheet — synthesis kernel + construction rules |
| `grove.sgml` | SGML input grove with hole `?x` |
| `refinement_eval.v` | Verilog RTL — SMT invariant checker, IDLE→EVAL→CHECK FSM |
| `dsssl-verilog-synth` | CLI — runs Stage 1 (Verilog) then Stage 2 (DSSSL) |
| `Makefile` | `make all`, `make verilog`, `make dsssl` |

---

## Run

```bash
# Full pipeline (Verilog + DSSSL)
./dsssl-verilog-synth --verbose

# Verilog only (requires iverilog)
make verilog
# iverilog -o sim refinement_eval.v && vvp sim

# DSSSL + JS kernel
make dsssl-js

# Install iverilog
brew install icarus-verilog          # Mac
sudo apt install iverilog            # Linux
```

---

## Expected output

```
[STAGE 1] Hardware SMT Refinement Check
  -> ?x = 1 | 10 + (1*5) = 15 | UNSAT
  -> ?x = 2 | 10 + (2*5) = 20 | SAT    <-- solution
  -> ?x = 3 | 10 + (3*5) = 25 | UNSAT
  ...

[STAGE 2] DSSSL Grove Synthesis
<SYNTHESIZED-GROVE STATUS="VERIFIED">
  <SYNTHESIZED-INT RESOLVED-FROM="?x">2</SYNTHESIZED-INT>
</SYNTHESIZED-GROVE>

[+] Hole ?x = 2. Grove verified. 10 + (2 * 5) = 20.
```

---

## Architecture position

```
grove.sgml              SGML input with holes
  ↓
synthesis.dsl           DSSSL: unify-hole + construction rules (ISO 10179)
  ↓
dsssl-kernel.mjs        JS runtime (snapkitty-clojure-lisp-bridge)
  ↓
refinement_eval.v       Verilog RTL: hardware SMT oracle
  ↓
netlister/sov_kernel.c  C kernel linked from Lean proofs (sovereign-forge)
  ↓
proofs/lean4/           Formal verification (sovereign-forge)
```

Omega = TRUST AND CODE
