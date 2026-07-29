/*
 * sov_verifier.h -- Sovereign Stack Machine Verifier
 *
 * Matrix verification engines for invariants, linear systems, and least squares.
 * ALL operations use int64_t with overflow detection via __builtin_*_overflow.
 * NO floating-point arithmetic. ZERO tolerance for numerical error.
 *
 * Per verification-policy.md:
 * - VerifyInv: A*X = I (exact match only)
 * - VerifySol: A*x = b (exact match only)
 * - VerifyLstsq: A^T(Ax-b) = 0 (exact match only)
 *
 * FORGE Phase 2: Matrix Verifier Engine
 *
 * Build: part of libsov_forge.a
 * License: Apache 2.0 + AGPL 3.0
 */

#ifndef SOV_VERIFIER_H
#define SOV_VERIFIER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ============================================================================
 * VERIFICATION RESULTS
 * ============================================================================
 */

typedef enum {
    VER_OK,                     /* Verification passed */
    VER_FAIL,                   /* Verification failed */
    VER_OVERFLOW,               /* Arithmetic overflow detected */
    VER_DIMENSION_MISMATCH,     /* Matrix dimensions don't match */
    VER_NULL_INPUT,             /* Null pointer input */
    VER_SINGULAR,               /* Matrix is singular (for inversion) */
} VerifyResult;

/*
 * ============================================================================
 * VERIFICATION ENGINES
 * ============================================================================
 */

/*
 * sov_verify_inv -- Verify that A*X = I (exact invariant)
 *
 * Given n x n matrix A and n x n matrix X, verify A*X = I exactly.
 * Uses int64_t arithmetic with overflow detection.
 *
 * Parameters:
 *   A         - n x n matrix (row-major order, n*n elements)
 *   X         - n x n matrix (row-major order, n*n elements)
 *   n         - Matrix dimension
 *
 * Returns:
 *   VER_OK                    - A*X = I verified exactly
 *   VER_FAIL                  - A*X != I
 *   VER_OVERFLOW              - Arithmetic overflow during computation
 *   VER_DIMENSION_MISMATCH    - Inconsistent dimensions
 *   VER_NULL_INPUT            - Null pointer
 */
VerifyResult sov_verify_inv(const int64_t *A,
                            const int64_t *X,
                            size_t n);

/*
 * sov_verify_sol -- Verify that A*x = b (exact linear system solution)
 *
 * Given m x n matrix A, vector x (n elements), and vector b (m elements),
 * verify that A*x = b exactly.
 *
 * Parameters:
 *   A         - m x n matrix (row-major order, m*n elements)
 *   x         - Solution vector (n elements)
 *   b         - Right-hand side vector (m elements)
 *   m, n      - Matrix dimensions
 *
 * Returns:
 *   VER_OK                    - A*x = b verified exactly
 *   VER_FAIL                  - A*x != b
 *   VER_OVERFLOW              - Arithmetic overflow during computation
 *   VER_DIMENSION_MISMATCH    - Inconsistent dimensions
 *   VER_NULL_INPUT            - Null pointer
 */
VerifyResult sov_verify_sol(const int64_t *A,
                            const int64_t *x,
                            const int64_t *b,
                            size_t m,
                            size_t n);

/*
 * sov_verify_lstsq -- Verify that A^T(Ax-b) = 0 (exact least squares)
 *
 * Given m x n matrix A, vector x (n elements), vector b (m elements),
 * verify that the normal equations hold exactly: A^T(Ax-b) = 0.
 *
 * This checks: for all i in [0, n):
 *   sum_j A[j][i] * (A[j][k] * x[k] - b[j]) = 0
 *
 * Parameters:
 *   A         - m x n matrix (row-major order, m*n elements)
 *   x         - Solution vector (n elements)
 *   b         - Right-hand side vector (m elements)
 *   m, n      - Matrix dimensions
 *
 * Returns:
 *   VER_OK                    - A^T(Ax-b) = 0 verified exactly
 *   VER_FAIL                  - Normal equations don't hold
 *   VER_OVERFLOW              - Arithmetic overflow detected
 *   VER_DIMENSION_MISMATCH    - Inconsistent dimensions
 *   VER_NULL_INPUT            - Null pointer
 */
VerifyResult sov_verify_lstsq(const int64_t *A,
                              const int64_t *x,
                              const int64_t *b,
                              size_t m,
                              size_t n);

/*
 * ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================
 */

/* Compute A*x into result vector (m elements)
 * Returns VER_OK on success, VER_OVERFLOW on overflow
 */
VerifyResult sov_matrix_vec_mult(const int64_t *A,
                                  const int64_t *x,
                                  int64_t *result,
                                  size_t m,
                                  size_t n);

/* Compute A*B into result matrix (m x p)
 * A is m x n, B is n x p
 * Returns VER_OK on success, VER_OVERFLOW on overflow
 */
VerifyResult sov_matrix_mult(const int64_t *A,
                              const int64_t *B,
                              int64_t *result,
                              size_t m,
                              size_t n,
                              size_t p);

/* Transpose matrix: A is m x n, result is n x m
 * Returns VER_OK on success
 */
VerifyResult sov_matrix_transpose(const int64_t *A,
                                   int64_t *result,
                                   size_t m,
                                   size_t n);

/* Check if all elements equal */
bool sov_matrix_equal(const int64_t *A,
                       const int64_t *B,
                       size_t nelems);

/* Convert result enum to string */
const char *sov_verify_result_to_string(VerifyResult r);

#ifdef __cplusplus
}
#endif

#endif /* SOV_VERIFIER_H */
