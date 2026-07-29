# Sovereign Stack Machine: Verification Policy

**Version:** 1.0.0

---

## Core Principle: EXACT INTEGER ARITHMETIC ONLY

- **Datatype:** `int64_t` (two's complement)
- **Tolerance:** Zero (bitwise exact equality)
- **Overflow:** Detected and fails verification
- **No floating-point** in core verifiers

---

## Verifier Specifications

### 1. `VerifyInv(A, X)` — Matrix Inverse Check

**Obligation:** `INV_OK`

**Check:**
```c
∀ i,j ∈ [0, n):
  if i == j then (A * X)[i,j] == 1
  else (A * X)[i,j] == 0
```

**Arithmetic:** `int64_t` with overflow detection

**Tolerance:** **ZERO** — exact equality required

**Implementation:**
```c
bool sov_verify_inv(const Mat* A, const Mat* X) {
  if (A->rows != X->cols || A->cols != X->rows) return false;
  
  Mat* P = matmul_checked(A, X);  // Overflow → return false
  if (!P) return false;
  
  for (int i = 0; i < A->rows; i++) {
    for (int j = 0; j < A->rows; j++) {
      int64_t expected = (i == j) ? 1 : 0;
      if (P->data[i * A->rows + j] != expected) return false;
    }
  }
  free(P);
  return true;
}
```

---

### 2. `VerifySol(A, x, b)` — Linear Solution Check

**Obligation:** `SOLVE_OK`

**Check:**
```c
∀ i ∈ [0, m):
  (A * x)[i] == b[i]
```

**Arithmetic:** `int64_t` exact

**Tolerance:** **ZERO**

**Implementation:**
```c
bool sov_verify_sol(const Mat* A, const Mat* x, const Mat* b) {
  if (A->cols != x->rows || x->cols != 1) return false;
  if (A->rows != b->rows || b->cols != 1) return false;
  
  Mat* y = matmul_checked(A, x);
  if (!y) return false;
  
  for (int i = 0; i < A->rows; i++) {
    if (y->data[i] != b->data[i]) return false;
  }
  free(y);
  return true;
}
```

---

### 3. `VerifyLstsq(A, x, b)` — Least-Squares Check

**Obligation:** `LSTSQ_OK`

**Check (Normal Equations):**
```c
∀ i ∈ [0, n):
  (Aᵀ * (A*x - b))[i] == 0
```

**Arithmetic:** `int64_t` exact

**Tolerance:** **ZERO**

**Implementation:**
```c
bool sov_verify_lstsq(const Mat* A, const Mat* x, const Mat* b) {
  if (A->cols != x->rows || x->cols != 1) return false;
  if (A->rows != b->rows || b->cols != 1) return false;
  if (A->rows < A->cols) return false;  // m ≥ n required
  
  // r = A*x - b
  Mat* Ax = matmul_checked(A, x);
  if (!Ax) return false;
  
  Mat* r = matsub_checked(Ax, b);  // Overflow → return false
  free(Ax);
  if (!r) return false;
  
  // AT_r = Aᵀ * r
  Mat* AT = mattrans(A);
  Mat* AT_r = matmul_checked(AT, r);
  free(AT);
  free(r);
  if (!AT_r) return false;
  
  // Check: AT_r == 0 (all elements)
  for (int i = 0; i < A->cols; i++) {
    if (AT_r->data[i] != 0) return false;
  }
  free(AT_r);
  return true;
}
```

---

### 4. `MatMul` — Shape & Bounds Checks

**Obligations:** `MATMUL_SHAPE`, `MATMUL_BOUNDS`

**Checks:**
```c
1. A.cols == B.rows  (compile-time via type; runtime assert)
2. ∀ i,j,k: |A[i,k]| < 2^63 AND |B[k,j]| < 2^63 (pre-check)
3. ∀ i,j: |∑_k A[i,k]*B[k,j]| < 2^63 (per-element post-check)
```

**Rationale:** Pre-check detects obvious overflows; post-check catches accumulation.

---

## Verification Modes

### STRICT Mode

**Policy:**
- Accept only exact integer verifications
- Reject any claim involving floating-point, tolerance, or approximation
- All obligations must pass

**Usage:** Hardware verification, formal methods, critical systems

---

### BOUNDED Mode

**Policy:**
- Accept floating-point verifications with declared tolerance policy
- Tolerance must be explicit (not inferred)
- Must state: absolute ε, relative δ, norm type

**Tolerance Formula (default):**
```
residual ≤ max(ε_abs, ε_rel * ||b||)
```

**Usage:** Numerical algorithms where exact arithmetic is infeasible

---

### AUDIT Mode

**Policy:**
- Recompute all supported operations independently
- Collect all failures (don't abort on first)
- Emit full diagnostic evidence

**Usage:** Security review, compliance audit, forensics

---

## Obligation Categorization

| Obligation | Check | Verifier | Domain | Tolerance |
|-----------|-------|----------|--------|-----------|
| `INV_OK` | `A*X==I` | VerifyInv | I64 | Zero |
| `SOLVE_OK` | `A*x==b` | VerifySol | I64 | Zero |
| `LSTSQ_OK` | `Aᵀ(Ax-b)==0` | VerifyLstsq | I64 | Zero |
| `MATMUL_SHAPE` | dims match | Compile-time | — | — |
| `MATMUL_BOUNDS` | no overflow | MatMul check | I64 | — |
| `ROW_OP_BOUNDS` | index in range | Runtime assert | — | — |
| `PIVOT_NONZERO` | k ≠ 0 | Runtime assert | I64 | — |

---

## Overflow Handling

**Detection:** Use `__builtin_add_overflow`, `__builtin_mul_overflow` (GCC/Clang)

**Action:** Return verification status `OVERFLOW`, include in receipt

**Never:** Wrap, saturate, or silently promote to a larger type

---

## Error Recording

**Each verification failure records:**
```json
{
  "obligation_id": <int>,
  "status": "FAIL",
  "details": {
    "check_type": "<INV_OK|SOLVE_OK|...>",
    "expected": <expected_value_or_range>,
    "actual": <actual_value_or_vector>,
    "max_error": <numeric_value_or_null>,
    "location": { "start_pc": <int>, "end_pc": <int> }
  }
}
```

---

## Performance Targets (Informational)

| Operation | n | Budget |
|-----------|---|--------|
| `VerifyInv` | ≤16 | 100 µs |
| `VerifySol` | ≤16 | 50 µs |
| `VerifyLstsq` | m≤64, n≤16 | 200 µs |

**Note:** Targets are informational. Correctness takes precedence over performance.

---

**END VERIFICATION POLICY**
