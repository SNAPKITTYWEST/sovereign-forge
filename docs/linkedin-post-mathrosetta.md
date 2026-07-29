# LinkedIn Post: MathRosetta ProofCore

**MathRosetta just shipped ProofCore — a proof pipeline that refuses to lie.**

Most proof assistants let you write `sorry` and move on. ProofCore doesn't.

The architecture:

```
LaTeX / Lean 4 / Isabelle input
  → MathIR (normalized AST)
  → ProofCore IR (universal intermediate representation)
  → 7 backend emitters:
      • Isabelle/HOL
      • Lean 4
      • Coq
      • Idris 2
      • SMT-LIB
      • LaTeX report
      • APL semantic trace
  → Placeholder scan (forbidden token detection)
  → Checker verification (P-time)
  → WORM receipt (hash-chained audit trail)
```

**The key constraint: the emitter is never allowed to call its own output "verified."**

Status lifecycle:
- `Emitted` → `PlaceholderDetected` → `CheckerPassed` → `WORMSealed` → `ExternallyVerified`

Only external checkers (isabelle build, lake build) can promote to `ExternallyVerified`. The emitter itself is structurally forbidden from self-certifying.

**Forbidden tokens per backend:**
- Isabelle: `sorry`, `oops`, `admit`, `axiomatization`
- Lean 4: `sorry`, `admit`, `Admitted`
- Coq: `Admitted`, `admit`, `Axiom`
- Idris 2: `primitive`, `believe_me`

Every proof output includes a `PlaceholderScan` — if any forbidden token is detected, status stays at `PlaceholderDetected` and the WORM receipt reflects this.

**Why this matters:**

Proof engineering has an honesty problem. Tools let you write incomplete proofs and call them done. ProofCore makes the proof status a first-class citizen — you can't ship without the receipt, and the receipt reflects reality.

🔗 MathRosetta: https://github.com/SNAPKITTYWEST/mathrosetta
🔗 Proof manifest: https://github.com/SNAPKITTYWEST/mathrosetta/tree/master/proofs

#FormalMethods #ProofEngineering #Lean4 #IsabelleHOL #MathRosetta #BuildInPublic
