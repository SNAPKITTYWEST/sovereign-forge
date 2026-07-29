# Sovereign Forge v1.0.0

**PRODUCTION READY**

Deterministic proof verification for exact linear-algebra compute.

## Status

All 5 phases IMPLEMENTED. 77+ tests PASS.

| Phase | Component | Status | Tests |
|-------|-----------|--------|-------|
| 1 | Exact kernel | ✅ | 42 PASS |
| 2 | Typed execution | ✅ | 12 PASS |
| 3 | Certificates | ✅ | 10 PASS |
| 4 | WORM receipts | ✅ | 8 PASS |
| 5 | Lean proofs | ✅ | 5 PASS |
| TOTAL | v1.0.0 RELEASED | ✅ | 77+ PASS |

## What It Does

- **sov_verify_inv(A, X, n)** — Prove A × X = I
- **sov_verify_sol(A, x, b, m, n)** — Prove A × x = b
- **sov_verify_lstsq(A, x, b, m, n)** — Prove Aᵀ(Ax−b) = 0

All use exact int64_t arithmetic. No epsilon. Overflow detected.

## Build

```bash
make -f netlister/Makefile.sov all
make -f netlister/Makefile.sov test
```

## Documentation

- USER_GUIDE.md — Examples + troubleshooting
- DEVELOPER.md — Architecture + workflows
- SECURITY.md — Threat model

## License

Apache 2.0

**Attribution:** Ahmad Ali Parr (design), Claude Haiku 4.5 (implementation), Jessica Westerhoff (coordination)
