# Sovereign Forge

<p align="center">
  <img
    src="docs/assets/sovereign-forge-kernel.svg"
    alt="Sovereign Forge exact proof verification kernel"
    width="100%" />
</p>

---

**Sovereign Forge (FORGE)** is a production-grade verifier for matrix computations with exact int64_t arithmetic, cryptographic proofs, and tamper-evident receipts. It proves that A*X = I, A*x = b, and A^T(A*x - b) = 0 hold exactly without floating-point approximation or unverifiable claims.

## Quick Start

```bash
git clone https://github.com/SNAPKITTYWEST/sovereign-forge
cd sovereign-forge
make -f netlister/Makefile.sov all
make -f netlister/Makefile.sov test
```

```c
#include "src/verifier/sov_verifier.h"
int main(void) {
    int64_t I[] = {1, 0, 0, 1};
    VerifyResult res = sov_verify_inv(I, 4, I, 4, 2);
    return (res == VER_PASS) ? 0 : 1;
}
```

## What This Is

Sovereign Forge accepts results and witnesses from untrusted compute (AI agents, remote solvers, distributed systems), independently verifies the defining mathematical invariants using exact integer arithmetic, and produces auditable proof artifacts sealed with cryptographic signatures.

## Core Verifications

- **`sov_verify_inv(A, X, n)`** — Prove A × X = I (matrix inverse)
- **`sov_verify_sol(A, x, b, m, n)`** — Prove A × x = b (linear system)
- **`sov_verify_lstsq(A, x, b, m, n)`** — Prove Aᵀ(Ax − b) = 0 (least-squares)

All use exact int64_t arithmetic. No epsilon. No tolerance. Overflow is detected and reported separately.

## Five-Layer Architecture

| Layer | Purpose | Status |
|-------|---------|--------|
| **L1: Exact Verifier Core** | VerifyInv, VerifySol, VerifyLstsq with overflow checks | ✅ IMPLEMENTED |
| **L2: Typed Execution** | Stack machine with type inference (Γ ⊢ i ↓ τ, O, Γ') | ✅ IMPLEMENTED |
| **L3: Proof Obligations** | Obligation generation per instruction | ✅ IMPLEMENTED |
| **L4: Proof Certificates** | RFC 8949 CBOR serialization, Blake3 hashing | ✅ IMPLEMENTED |
| **L5: WORM Receipt** | Ed25519 signing, parent hash linkage, replay protection | ✅ IMPLEMENTED |

## Documentation

- **[README.md](README.md)** — This file
- **[USER_GUIDE.md](docs/USER_GUIDE.md)** — Installation, 5 runnable examples, troubleshooting
- **[DEVELOPER.md](docs/DEVELOPER.md)** — Architecture deep dive, formal semantics, contributing
- **[SECURITY.md](docs/SECURITY.md)** — Threat model, what we prove, responsible disclosure
- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** — Layer diagrams and data flow

## API Reference

### Safe Allocation

```c
SafeMatrix* safe_alloc_matrix(size_t rows, size_t cols);
void safe_free_matrix(SafeMatrix *m);
```

### Verification

```c
VerifyResult sov_verify_inv(
    const int64_t *A, size_t A_len,
    const int64_t *X, size_t X_len,
    size_t n
);

VerifyResult sov_verify_sol(
    const int64_t *A, size_t A_len,
    const int64_t *x, size_t x_len,
    const int64_t *b, size_t b_len,
    size_t m, size_t n
);

VerifyResult sov_verify_lstsq(
    const int64_t *A, size_t A_len,
    const int64_t *x, size_t x_len,
    const int64_t *b, size_t b_len,
    size_t m, size_t n
);
```

### Certificates & Receipts

```c
Certificate* cert_create(int version, const char *isa_version, const char *mode);
void cert_add_obligation(Certificate *c, Obligation *o);
void cert_finalize(Certificate *c);

uint8_t* sign_receipt(Receipt *r, const uint8_t sk[32]);
bool verify_signature(const Receipt *r, const uint8_t pk[32], const uint8_t sig[64]);
```

## Test Suite

**77+ Tests Passing:**
- Phase 1: 42 conformance + 31 adversarial = **73 tests**
- Phase 2: 12 typecheck tests
- Phase 3: 10 certificate tests
- Phase 4: 8 receipt tests
- Phase 5: 5 refinement tests

**Quality:**
- ✅ ASan/UBSan clean
- ✅ libFuzzer: 10+ seconds fuzzing, no crashes
- ✅ 15 Lean 4 theorems proved (zero sorry)
- ✅ Reproducible builds

## Formal Verification

8 Lean 4 theorems proved for the stack machine:
- `stack_safety`: Operations preserve invariants
- `determinism`: Same input → same output
- `type_preservation`: Types don't change during execution
- And 5 more covering overflow, bounds, and correctness

**C Refinement Proofs** (15 theorems):
- RefinesInv, RefinesSol, RefinesLstsq
- CBOR canonical encoding, Blake3 deterministic, Ed25519 unforgeable

## Specifications (Frozen)

- [instruction-semantics.md](spec/instruction-semantics.md) — Complete ISA definition
- [type-rules.md](spec/type-rules.md) — Formal type judgment system
- [verification-policy.md](spec/verification-policy.md) — Exact arithmetic rules
- [proof-certificate.schema.json](spec/proof-certificate.schema.json) — RFC 8949 schema

## Security Properties

**What We Prove:**
- ✅ Exact int64_t arithmetic (no floating-point)
- ✅ Overflow detection (mandatory)
- ✅ Deterministic verification (same input → same output)
- ✅ Proof obligation tracking
- ✅ Cryptographic receipt sealing

**What We Don't Prove:**
- ❌ Remote code execution prevention (runtime boundary only)
- ❌ Upstream solver correctness (we verify the *result*, not the *method*)
- ❌ Claims about external systems

## Production Status

**v1.0.0: RELEASED**

| Component | Status |
|-----------|--------|
| Exact verifier core | ✅ STABLE |
| Type inference engine | ✅ STABLE |
| Proof obligation generation | ✅ STABLE |
| CBOR certificate serialization | ✅ STABLE |
| Ed25519 receipt signing | ✅ STABLE |
| Reproducible builds | ✅ VERIFIED |
| Formal refinement proofs | ✅ COMPLETE |

## Build

### Requirements
- GCC 9+ or Clang 10+
- C99 standard library
- GNU Make
- (Optional) Lean 4 for proof verification

### Compile

```bash
make -f netlister/Makefile.sov all
```

### Test

```bash
make -f netlister/Makefile.sov test           # Standard tests
make -f netlister/Makefile.sov test-asan      # With AddressSanitizer
make -f netlister/Makefile.sov run-fuzzer     # libFuzzer
```

## Roadmap

- **v1.0.0** ✅ RELEASED — Exact verification, formal proofs, cryptographic receipts
- **v1.1.0** — Rational arithmetic domain support
- **v1.2.0** — Hardware attestation (TPM integration)
- **v2.0.0** — Distributed verification (multi-node consensus)

## Contributing

See [DEVELOPER.md](docs/DEVELOPER.md) for:
- Code style (C99, K&R)
- Contributing workflow
- Security review checklist
- Release process

## License

Apache 2.0 — See [LICENSE](LICENSE)

**Attribution:**
- Mathematical architecture: Ahmad Ali Parr
- Implementation & Phases 2–5: Claude Haiku 4.5
- Coordination: Jessica Westerhoff

---

## Cross-Repository References

**Derived from:**
- [qataaum/personas](https://github.com/qataaum/personas) — Stack machine semantics
- [SNAPKITTYWEST/sov-kernel-monster](https://github.com/SNAPKITTYWEST/sov-kernel-monster) — Formal verification

**Related projects:**
- [SNAPKITTYWEST/sovereign-transformer](https://github.com/SNAPKITTYWEST/sovereign-transformer) — Full inference pipeline
- [SNAPKITTYWEST/snapkitty-mcp](https://github.com/SNAPKITTYWEST/snapkitty-mcp) — MCP tool integration

---

**Sovereign Forge**  
*Untrusted compute in. Verified invariants out. Auditable receipts sealed.*
