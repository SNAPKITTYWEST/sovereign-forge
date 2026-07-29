# Sovereign Stack Machine: Instruction Semantics

**Version:** 1.0.0  
**Last Updated:** 2026-07-29  
**Authority:** Ahmad Ali Parr + Jessica (ARCHITECT phase)

---

## Stack Type System

### ValType (Discriminated Union)

```lean4
inductive ValType where
  | Scalar : ValType          -- int64_t
  | Vec (n : ℕ) : ValType    -- n-element vector
  | Mat (m n : ℕ) : ValType  -- m×n matrix
  | Proof (p : String) : ValType  -- Ghost type for proof witnesses
```

**Stack**: `StackType := List ValType` (top of stack = head of list)

### Numeric Domain

```
Domain ::= I64       -- Exact 64-bit signed integer
         | Rational  -- Exact rational (not used in core ISA, reserved)
         | F64       -- IEEE 754 double (not in core ISA)
```

Core ISA: **I64 only**. No implicit coercions.

---

## Instruction Typing Rules

### Format

```
Γ ⊢ INSTR : Γ → Γ' | Obligations | Errors

where:
  Γ = input stack type
  Γ' = output stack type
  Obligations = generated proof obligations
  Errors = failure modes (preconditions violated)
```

---

## Arithmetic Instructions

### `Push x : int64_t`

**Precondition:** None (always valid)

**Typing Rule:**
```
Γ ⊢ Push x : Γ → Scalar :: Γ
```

**Semantics:**
- Push immediate value `x` onto stack
- No obligations

**Example:**
```
Before:  [Mat 3 3]
Push 42:
After:   [Scalar, Mat 3 3]
```

---

### `Dup`

**Precondition:** Stack non-empty

**Typing Rule:**
```
Γ = h :: t ⊢ Dup : h :: h :: t → h :: h :: t
```

**Semantics:**
- Duplicate top-of-stack element
- Preserves type

**Error:** Stack underflow

**Example:**
```
Before:  [Scalar]
Dup:
After:   [Scalar, Scalar]
```

---

### `Swap`

**Precondition:** Stack has at least 2 elements

**Typing Rule:**
```
Γ = h1 :: h2 :: t ⊢ Swap : h2 :: h1 :: t
```

**Semantics:**
- Exchange top two elements
- Preserves types independently

**Error:** Stack underflow

**Example:**
```
Before:  [Scalar, Mat 3 3]
Swap:
After:   [Mat 3 3, Scalar]
```

---

### `Add`

**Precondition:** Top two elements have compatible shape

**Typing Rules:**

```
Case 1 (Scalar + Scalar):
Γ = Scalar :: Scalar :: t ⊢ Add : Scalar :: t

Case 2 (Vec n + Vec n):
Γ = Vec n :: Vec n :: t ⊢ Add : Vec n :: t

Case 3 (Mat m n + Mat m n):
Γ = Mat m n :: Mat m n :: t ⊢ Add : Mat m n :: t
```

**Semantics:**
- Pop two elements
- Compute element-wise addition (int64_t)
- Push result

**Obligations:**
- `ADD_BOUNDS`: Check no int64_t overflow in any element

**Errors:**
- Shape mismatch (e.g., `Vec 3 + Vec 4`)
- Overflow in any element

**Example:**
```
Before:  [Scalar(5), Scalar(3), Vec 2]
Add:
After:   [Scalar(8), Vec 2]
```

---

## Matrix Instructions

### `MatPush m n`

**Precondition:** None

**Typing Rule:**
```
Γ ⊢ MatPush m n : Mat m n :: Γ
```

**Semantics:**
- Push uninitialized matrix descriptor
- Placeholder for later `MatFill`

**Note:** This instruction is compile-time only; it doesn't allocate heap memory. The shape `m n` is a static constant.

---

### `MatFill`

**Precondition:** Stack contains `m×n` scalars followed by matrix descriptor

**Typing Rule:**
```
Γ = Mat m n :: Scalar :: ... :: Scalar :: t  (m*n scalars)
    ⊢ MatFill : Mat m n :: t
```

**Semantics:**
- Pop `m*n` scalars from stack (row-major order)
- Fill matrix with popped values
- Push populated matrix descriptor

**Errors:**
- Insufficient scalars on stack
- Shape mismatch

---

### `MatMul`

**Precondition:** Top two elements are matrices with compatible inner dimension

**Typing Rule:**
```
Γ = Mat m n :: Mat n k :: t
  ⊢ MatMul : Mat m k :: t  |  [MATMUL_SHAPE, MATMUL_BOUNDS]
```

**Obligations:**
- `MATMUL_SHAPE`: `A.cols == B.rows` (compile-time, asserted at runtime)
- `MATMUL_BOUNDS`: No overflow in any element of `A * B`

**Semantics:**
- Pop matrices `A` (m×n) and `B` (n×k)
- Compute `C = A × B` (m×k)
- Push result
- Use exact int64_t arithmetic with overflow checking

**Errors:**
- Shape mismatch
- Overflow

---

### `MatTrans`

**Precondition:** Top element is matrix

**Typing Rule:**
```
Γ = Mat m n :: t ⊢ MatTrans : Mat n m :: t
```

**Semantics:**
- Pop matrix `A` (m×n)
- Compute transpose `A^T` (n×m)
- Push result

**No obligations** (shape-preserving)

---

### `MatAug` (Augment/Concatenate)

**Precondition:** Top two elements are matrices with same row count

**Typing Rule:**
```
Γ = Mat m n2 :: Mat m n1 :: t
  ⊢ MatAug : Mat m (n1 + n2) :: t
```

**Semantics:**
- Pop `B` (m×n2) and `A` (m×n1)
- Compute horizontally concatenated `[A | B]` (m × (n1+n2))
- Push result

**Errors:**
- Row mismatch

---

## Gauss-Jordan Row Operations

### `RowSwap`

**Precondition:** Row indices in bounds

**Typing Rule:**
```
Γ = Scalar :: Scalar :: Mat m n :: t
  ⊢ RowSwap : Mat m n :: t  |  [ROW_OP_BOUNDS]
```

**Obligations:**
- `ROW_OP_BOUNDS`: `i < m ∧ j < m`

**Semantics:**
- Pop row indices `j`, `i` and matrix `M` (m×n)
- Swap rows i and j in-place
- Push modified matrix

---

### `RowScale`

**Precondition:** Row index in bounds, scalar non-zero

**Typing Rule:**
```
Γ = Scalar :: Scalar :: Mat m n :: t
  ⊢ RowScale : Mat m n :: t  |  [ROW_OP_BOUNDS, PIVOT_NONZERO]
```

**Obligations:**
- `ROW_OP_BOUNDS`: `i < m`
- `PIVOT_NONZERO`: `k ≠ 0`

**Semantics:**
- Pop scalar `k`, row index `i`, matrix `M`
- Multiply row i by scalar `k` in-place
- Push modified matrix

---

### `RowAddMul`

**Precondition:** Row indices in bounds, scalar arbitrary

**Typing Rule:**
```
Γ = Scalar :: Scalar :: Scalar :: Mat m n :: t
  ⊢ RowAddMul : Mat m n :: t  |  [ROW_OP_BOUNDS]
```

**Obligations:**
- `ROW_OP_BOUNDS`: `src < m ∧ dst < m`

**Semantics:**
- Pop scalar `k`, destination row `dst`, source row `src`, matrix `M`
- Compute `M[dst] += k * M[src]` (element-wise)
- Overflow checked
- Push modified matrix

---

## High-Level Macros (Compile-Time Expansion)

### `Inv` (Matrix Inversion)

**Precondition:** Input is square matrix, determinant ≠ 0 (proof required)

**Typing Rule:**
```
Γ = Proof("Invertible") :: Mat n n :: t
  ⊢ Inv : Mat n n :: t  |  [INV_OK]
```

**Obligation:**
- `INV_OK`: Proof that `Det(A) ≠ 0`

**Semantics (Compile-Time Expansion):**
1. Push identity matrix I (n×n)
2. `MatAug` → `[A | I]` (n × 2n)
3. Gauss-Jordan elimination (micro-ops unrolled for fixed n)
4. Extract right half → `A^(-1)`
5. Push result

**Errors:**
- Non-square matrix
- Missing invertibility proof
- Gauss-Jordan fails (shouldn't happen if proof valid)

---

### `Solve` (Linear System A*x = b)

**Precondition:** Matrix A invertible (proof required)

**Typing Rule:**
```
Γ = Proof("Invertible") :: Mat n 1 :: Mat n n :: t
  ⊢ Solve : Mat n 1 :: t  |  [SOLVE_OK]
```

**Obligation:**
- `SOLVE_OK`: Proof that `Det(A) ≠ 0`

**Semantics:**
1. Use `Inv` to compute `A^(-1)`
2. `MatMul` to compute `x = A^(-1) * b`
3. Push result

---

### `Lstsq` (Least-Squares A*x ≈ b)

**Precondition:** Matrix A has full column rank (proof required)

**Typing Rule:**
```
Γ = Proof("FullColumnRank") :: Mat m 1 :: Mat m n :: t (m ≥ n)
  ⊢ Lstsq : Mat n 1 :: t  |  [LSTSQ_OK]
```

**Obligation:**
- `LSTSQ_OK`: Proof that `Rank(A) = n` (equivalently, `A^T*A` invertible)

**Semantics (Normal Equations):**
1. Compute `A^T` via `MatTrans`
2. Compute `A^T * A` via `MatMul`
3. Compute `A^T * b` via `MatMul`
4. Solve `(A^T*A) * x = A^T*b` via `Solve`
5. Push result

---

## Verifier Instructions

### `VerifyInv`

**Precondition:** Top two elements are matrices (same shape)

**Typing Rule:**
```
Γ = Mat n n :: Mat n n :: t
  ⊢ VerifyInv : Proof("InvOK") :: t  |  [INV_OK]
```

**Obligation:**
- `INV_OK`: Verify `A * X == I` exactly (int64_t)

**Semantics:**
1. Pop matrices `X` and `A` (both n×n)
2. Compute product `P = A * X`
3. Check: `P[i,j] == (1 if i==j else 0)` for all i,j
4. If all checks pass: push `Proof("InvOK")`
5. If any check fails: abort with failure record

**No overflow tolerance:** Overflow in `A * X` → verification fails

---

### `VerifySol`

**Precondition:** Top three elements are matrix, vector, vector

**Typing Rule:**
```
Γ = Mat n 1 :: Mat n n :: Mat n 1 :: t
  ⊢ VerifySol : Proof("SolOK") :: t  |  [SOLVE_OK]
```

**Obligation:**
- `SOLVE_OK`: Verify `A * x == b` exactly

**Semantics:**
1. Pop `b` (n×1), `A` (n×n), `x` (n×1)
2. Compute `y = A * x`
3. Check: `y[i] == b[i]` for all i
4. If all checks pass: push `Proof("SolOK")`
5. If any check fails: abort

---

### `VerifyLstsq`

**Precondition:** Three matrices on stack

**Typing Rule:**
```
Γ = Mat n 1 :: Mat m n :: Mat m 1 :: t (m ≥ n)
  ⊢ VerifyLstsq : Proof("LstsqOK") :: t  |  [LSTSQ_OK]
```

**Obligation:**
- `LSTSQ_OK`: Verify `A^T * (A*x - b) == 0` exactly (normal equations)

**Semantics:**
1. Pop `b` (m×1), `A` (m×n), `x` (n×1)
2. Compute residual `r = A*x - b` (m×1)
3. Compute `A^T * r` (n×1)
4. Check: `(A^T*r)[i] == 0` for all i
5. If all checks pass: push `Proof("LstsqOK")`
6. If any check fails: abort

---

## Stack Transition Examples

### Example 1: 2×2 Inversion

```
Program: [Push A_data..., MatPush 2 2, MatFill, Proof(...), Inv, VerifyInv]

Stack Trace:
  []
  [Scalar] (first element of A)
  [Scalar, Scalar] (second element)
  [Scalar, Scalar, Scalar, Scalar] (4 elements = 2×2)
  [Mat 2 2] (after MatFill)
  [Proof("Invertible"), Mat 2 2] (after Proof)
  [Mat 2 2] (after Inv expansion, A^(-1))
  [Proof("InvOK")] (after VerifyInv succeeds)
```

### Example 2: 3×3 Solve

```
Program: [Push A_data..., MatPush 3 3, MatFill, 
          Push b_data..., MatPush 3 1, MatFill,
          Proof(...), Solve, VerifySol]

Stack Trace (abbreviated):
  []
  [Mat 3 3] (after first MatFill)
  [Mat 3 1, Mat 3 3] (after second MatFill)
  [Proof("Invertible"), Mat 3 1, Mat 3 3] (after Proof)
  [Mat 3 1] (after Solve)
  [Proof("SolOK")] (after VerifySol succeeds)
```

---

## Error Handling

### Type Errors (Compile-Time)

Caught by `ValidStack` inference:
- Stack underflow
- Shape mismatch (e.g., `Add` on incompatible dimensions)
- Invalid opcode

### Verification Failures (Runtime)

Triggered by `VerifyInv`, `VerifySol`, `VerifyLstsq`:
- Overflow during computation
- Incorrect result (e.g., `A*X ≠ I`)
- Symbolic shape resolution failure

**Action:** Abort execution, emit FAIL receipt

---

## Canonical Encoding

**Program representation for hashing:**
- Instruction opcodes as single byte: `Push=0x00, Dup=0x01, Swap=0x02, Add=0x03, ...`
- Immediate values as big-endian int64_t
- Matrix dimensions as uint32_t (m then n)
- Row-major layout for matrix data

**Example:** `[Push 10, MatPush 2 2, MatFill]`
```
0x00 0x0000000a          // Push 10
0x14 0x00000002 0x00000002  // MatPush 2 2
0x15                      // MatFill
```

---

**END INSTRUCTION SEMANTICS**
