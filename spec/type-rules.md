# Sovereign Stack Machine: Type Rules (Formal)

**Version:** 1.0.0

---

## Judgment Forms

```
Γ ⊢ p ↓ (τ, O, Γ')

where:
  Γ = input stack type
  p = program (instruction sequence)
  τ = type of computed value (if any)
  O = generated obligations
  Γ' = output stack type
```

---

## Forward Type Inference (Deterministic)

```
Algorithm: infer(p: List Instr, Γ₀: StackType) → (Γ_out: StackType, O: List Obligation) ∪ Error
```

**Base Case:** Empty program
```
Γ ⊢ [] ↓ (unit, [], Γ)
```

**Inductive Step:**
```
Γ ⊢ i ↓ (τ, O_i, Γ')    Γ' ⊢ p ↓ (τ', O_p, Γ'')
————————————————————————————————————————————————————
  Γ ⊢ (i :: p) ↓ (τ', O_i ++ O_p, Γ'')
```

---

## Individual Instruction Rules

### `Push x`
```
Γ ⊢ Push x ↓ (int64_t, [], Scalar :: Γ)
```

### `Dup`
```
h ∈ Γ    Γ = h :: t
———————————————————
Γ ⊢ Dup ↓ (unit, [], h :: h :: t)
```

**Error:** Stack empty

### `Swap`
```
Γ = h₁ :: h₂ :: t
———————————————————
Γ ⊢ Swap ↓ (unit, [], h₂ :: h₁ :: t)
```

**Error:** Fewer than 2 elements

### `Add`
```
Case 1: Scalar + Scalar
Γ = Scalar :: Scalar :: t
———————————————————————————
Γ ⊢ Add ↓ (unit, [AddBounds], Scalar :: t)

Case 2: Vec n + Vec n
Γ = Vec n :: Vec n :: t
———————————————————————————
Γ ⊢ Add ↓ (unit, [VecAddBounds n], Vec n :: t)

Case 3: Mat m n + Mat m n
Γ = Mat m n :: Mat m n :: t
——————————————————————————————
Γ ⊢ Add ↓ (unit, [MatAddBounds m n], Mat m n :: t)
```

**Error:** Shape mismatch

### `MatPush m n`
```
————————————————————————
Γ ⊢ MatPush m n ↓ (unit, [], Mat m n :: Γ)
```

### `MatFill`
```
Γ = Mat m n :: Scalar :: ... :: Scalar :: t    (m*n scalars)
————————————————————————————————————————————————
Γ ⊢ MatFill ↓ (unit, [MatFillBounds m n], Mat m n :: t)
```

**Error:** Insufficient scalars, or shape mismatch

### `MatMul`
```
Γ = Mat m n :: Mat n k :: t
————————————————————————————————————————————
Γ ⊢ MatMul ↓ (unit, [MatMulShape m n k, MatMulBounds m n k], Mat m k :: t)
```

**Obligations:**
- `MatMulShape m n k`: Assert n values match (compile-time, runtime check)
- `MatMulBounds m n k`: No overflow in any element

**Error:** Shape mismatch

### `MatTrans`
```
Γ = Mat m n :: t
——————————————————
Γ ⊢ MatTrans ↓ (unit, [], Mat n m :: t)
```

### `MatAug`
```
Γ = Mat m n₂ :: Mat m n₁ :: t
————————————————————————————————
Γ ⊢ MatAug ↓ (unit, [MatAugShape m n₁ n₂], Mat m (n₁ + n₂) :: t)
```

**Error:** Row mismatch

### `Inv`
```
Γ = Proof("Invertible") :: Mat n n :: t
———————————————————————————————————————————
Γ ⊢ Inv ↓ (unit, [InvBounds n], Mat n n :: t)
```

**Obligation:** `InvBounds n` — check no overflow in Gauss-Jordan steps

**Error:** Non-square, or missing proof

### `Solve`
```
Γ = Proof("Invertible") :: Mat n 1 :: Mat n n :: t
———————————————————————————————————————————————————
Γ ⊢ Solve ↓ (unit, [SolveBounds n], Mat n 1 :: t)
```

### `Lstsq`
```
Γ = Proof("FullColumnRank") :: Mat m 1 :: Mat m n :: t    (m ≥ n)
———————————————————————————————————————————————————————————
Γ ⊢ Lstsq ↓ (unit, [LstsqBounds m n], Mat n 1 :: t)
```

### `VerifyInv`
```
Γ = Mat n n :: Mat n n :: t
————————————————————————————
Γ ⊢ VerifyInv ↓ (unit, [VerifyInvObligation n], Proof("InvOK") :: t)
```

**Obligation:** `VerifyInvObligation n` — verify A*X == I

### `VerifySol`
```
Γ = Mat n 1 :: Mat n n :: Mat n 1 :: t
——————————————————————————————————————
Γ ⊢ VerifySol ↓ (unit, [VerifySolObligation n], Proof("SolOK") :: t)
```

### `VerifyLstsq`
```
Γ = Mat n 1 :: Mat m n :: Mat m 1 :: t    (m ≥ n)
——————————————————————————————————————————————————
Γ ⊢ VerifyLstsq ↓ (unit, [VerifyLstsqObligation m n], Proof("LstsqOK") :: t)
```

---

## Unification Algorithm

**For symbolic dimensions in branching code (not in core ISA):**

```
unify(τ₁: ValType, τ₂: ValType) → (Substitution ∪ Conflict)

Base cases:
  unify(Scalar, Scalar) = {}
  unify(Vec n₁, Vec n₂) = if n₁ == n₂ then {} else Conflict("Vec dimension mismatch")
  unify(Mat m₁ n₁, Mat m₂ n₂) = if m₁==m₂ && n₁==n₂ then {} else Conflict(...)
  unify(τ₁, τ₂) when τ₁ ≠ τ₂ = Conflict("Type mismatch")
```

**Apply substitution:**
```
apply(σ: Substitution, Γ: StackType) : StackType
```

---

## Completeness & Soundness

**Completeness:** If a program is valid (no underflow, correct shapes), `infer` succeeds and produces `Γ'`.

**Soundness:** If `infer(p, Γ)` succeeds with `(Γ', O)`, then execution of `p` starting with stack type `Γ` produces stack type `Γ'` and only generates obligations in `O`.

**Proof:** By induction on program length and instruction structure. Each instruction rule preserves type safety and obligation correctness.

---

**END TYPE RULES**
