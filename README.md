# Sovereign Forge

<p align="center">
  <strong>Deterministic proof verification for exact linear-algebra compute.</strong>
</p>

<p align="center">
  Exact arithmetic · Overflow detection · Typed obligations · Canonical certificates · WORM receipts
</p>

---

## What This Is

Sovereign Forge is a compact verification kernel for checking mathematical witnesses produced by stack machines, numerical runtimes, compilers, and autonomous agents.

**Current status:** Exact verifier kernel is implemented and tested. Typed execution, certificate, and WORM layers are under development.

---

## The Verification Problem

Most compute systems ask you to trust:
- the runtime
- the machine  
- the library
- the model
- the operator

Sovereign Forge separates computation from verification:

<p align="center">
  <img
    src="docs/assets/sovereign-forge-kernel.svg"
    alt="Sovereign Forge exact proof verification kernel"
    width="100%" />
</p>


```text
UNTRUSTED COMPUTE
  Result + Witness
      ↓
SOVEREIGN FORGE KERNEL
  • Validate dimensions
  • Recompute invariant
  • Detect overflow
  • Emit proof outcome
      ↓
  CERTIFICATE + RECEIPT
```

---

## Status Matrix

| Component | Status | Notes |
|-----------|--------|-------|
| **Exact matrix verifier** | ✅ Working | VerifyInv, VerifySol, VerifyLstsq |
| **Overflow-checked arithmetic** | ✅ Working | `__builtin_*_overflow` on all ops |
| **Conformance tests** | ✅ Passing | 11/11 PASS |
| **Adversarial tests** | ✅ Passing | 31/31 PASS, 21/21 attacks detected |
| **Lean proofs** | ✅ Available | 8 theorems PROVED, zero sorry |
| **Typed stack execution** | 🔄 Stubbed | Push/pop/peek currently return NULL |
| **Type inference** | 🔄 Stubbed | Returns NULL, no shape unification |
| **Proof obligations** | 🔄 Stubbed | Not implemented |
| **CBOR certificates** | 🔄 Stubbed | Not implemented |
| **Receipt signing** | 🔄 Stubbed | Not implemented |
| **WORM receipts** | 🔄 Stubbed | Not implemented |

**Honest statement:** The exact verifier core is production-usable as an experimental library. The complete typed-execution, certificate, and WORM pipeline is not yet released as a stable interface.

---

## What Works Now

### Exact arithmetic verification

The kernel operates over `int64_t` with:
- **no floating-point epsilon**
- **no hidden tolerance**
- **no approximate equality**
- **no silent wraparound**

Every multiplication, addition, subtraction is checked for overflow.

### Inverse verification

```c
verify_inv(A, X)  // Check: A × X = I
```

### Linear-system verification

```c
verify_sol(A, x, b)  // Check: A × x = b
```

### Least-squares verification

```c
verify_lstsq(A, x, b)  // Check: Aᵀ(Ax − b) = 0
```

---

## Architecture

```text
┌────────────────────────────────────────────┐
│   WORM Receipt Layer (STUBBED/PLANNED)     │
├────────────────────────────────────────────┤
│  Proof Certificates (STUBBED/IN DEVELOPMENT)
├────────────────────────────────────────────┤
│  Proof Obligations (STUBBED/IN DEVELOPMENT)
├────────────────────────────────────────────┤
│  Typed Execution (STUBBED/IN DEVELOPMENT)
├────────────────────────────────────────────┤
│  Exact Verification Kernel (✅ WORKING)    │
│  VerifyInv · VerifySol · VerifyLstsq      │
│  Overflow checked · Deterministic          │
└────────────────────────────────────────────┘
```

Only the bottom layer is complete and directly usable.

---

## Build & Test

### Requirements

- C compiler with `__builtin_*_overflow` support
- GNU Make
- 64-bit platform

### Compile

```bash
make -f netlister/Makefile.sov
```

### Run tests

```bash
make -f netlister/Makefile.sov test
# Output: 42/42 PASS (11 conformance + 31 adversarial)
```

---

## API (Experimental)

```c
typedef enum {
  VER_PASS = 0,
  VER_FAIL = 1,
  VER_OVERFLOW = 2,
  VER_SHAPE_MISMATCH = 3,
  VER_NULL_INPUT = 4,
  VER_ALLOC_FAILURE = 5
} VerifyResult;

VerifyResult sov_verify_inv(const int64_t *A, const int64_t *X, size_t n);
VerifyResult sov_verify_sol(const int64_t *A, const int64_t *x, const int64_t *b, size_t m, size_t n);
VerifyResult sov_verify_lstsq(const int64_t *A, const int64_t *x, const int64_t *b, size_t m, size_t n);
```

Row-major matrix layout. Caller responsible for buffer sizes.

---

## Production Blockers (P0)

Before any production deployment:

1. **Allocation overflow** — `malloc(n * n * sizeof(int64_t))` can wrap
2. **No buffer-length validation** — Pointers alone don't prove array bounds
3. **No resource bounds** — Remote size can exhaust CPU/memory
4. **Certificate/WORM are stubs** — No actual hashing, serialization, signing
5. **Type inference not implemented** — Programs cannot yet be validated

---

## Roadmap to v1.0.0

### v0.1.0: Kernel hardening
- [ ] Buffer-size overflow protection
- [ ] Allocation-failure errors
- [ ] ASan/UBSan clean
- [ ] Fuzz testing harness

### v0.2.0: Typed execution
- [ ] Stack push/pop/peek implementation
- [ ] Type inference engine
- [ ] Shape unification

### v0.3.0: Proof artifacts  
- [ ] Obligation generation
- [ ] Canonical CBOR serialization
- [ ] Deterministic hashing

### v0.4.0: Provenance
- [ ] Ed25519 signing
- [ ] WORM receipt chaining
- [ ] Replay protection

### v1.0.0: Formal refinement
- [ ] Lean-to-C verification
- [ ] Reproducible builds
- [ ] External security audit

---

## Security Properties

**Currently provides:**
- Deterministic outcomes
- Exact equality checks
- Arithmetic-overflow detection
- Small audit surface

**Not yet provided:**
- Buffer-length validation
- Resource bounds
- Cryptographic sealing
- Hardware attestation
- ABI versioning

---

## Repository Layout

```
src/verifier/       ✅ Exact kernel (STABLE)
src/typecheck/      🔄 Type system (STUBBED)
src/obligations/    🔄 Obligations (STUBBED)
src/certificate/    🔄 Certificates (STUBBED)

spec/               ✅ Frozen Phase 1 specifications
proofs/lean4/       ✅ Stack-machine proofs (8 PROVED)

tests/
  conformance/      ✅ 11/11 PASS
  adversarial/      ✅ 31/31 PASS

netlister/          📋 Experimental extraction
veriloga/           📋 Experimental mixed-signal
docs/               Architecture & audit material
```

---

## License

Apache 2.0

---

## Core Principle

> Compute may be untrusted. Verification must be small enough to inspect.

---

**Sovereign Forge**  
Exact verification kernel for linear-algebra witnesses.

*Cracked by Ahmad Ali Parr. Implemented by Claude Haiku 4.5. Validated by Opus 5.*
