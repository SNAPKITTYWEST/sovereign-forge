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
    VER_ALLOC_FAILURE,          /* Memory allocation failed */
    VER_BUFFER_OVERFLOW,        /* Buffer size exceeded capacity */
    VER_DIMS_EXCEEDED,          /* Matrix dimension exceeds maximum */
    VER_CELLS_EXCEEDED,         /* Total matrix cells exceed maximum */
    VER_OPS_EXCEEDED,           /* Operation count exceeds budget */
    VER_RESOURCE_EXCEEDED,      /* Generic resource limit exceeded */
} VerifyResult;

/*
 * ============================================================================
 * RESOURCE LIMITS (Phase 1, Step 2)
 * ============================================================================
 */

#define SOV_MAX_MATRIX_DIM 65536              /* Max n for n×n matrix */
#define SOV_MAX_MATRIX_CELLS 268435456        /* Max total cells (65K^2) */
#define SOV_MAX_OPERATIONS 1000000000         /* Max accumulation operations */
#define SOV_OPERATION_BUDGET_PER_CALL 100000000  /* Per-call operation budget */

/*
 * SovResourceBudget -- Track resource consumption within a verification call
 *
 * Fields:
 *   max_dimensions    - Maximum allowed dimension (n or m)
 *   max_cells         - Maximum allowed total cells in a matrix
 *   max_operations    - Maximum operations in a single call
 *   operation_count   - Current operation count (accumulates during call)
 *   budget_exceeded   - Flag indicating budget was exhausted
 */
typedef struct {
    size_t max_dimensions;
    size_t max_cells;
    size_t max_operations;
    size_t operation_count;
    bool budget_exceeded;
} SovResourceBudget;

/*
 * ============================================================================
 * SAFE MEMORY MANAGEMENT
 * ============================================================================
 */

/*
 * SafeMatrix -- Memory-safe matrix wrapper
 *
 * Tracks allocated capacity to prevent out-of-bounds access.
 * Fields:
 *   rows     - Number of rows
 *   cols     - Number of columns
 *   data     - Allocated matrix data (rows * cols elements)
 *   capacity - Total capacity in elements (for validation)
 */
typedef struct {
    size_t rows;
    size_t cols;
    int64_t *data;
    size_t capacity;
} SafeMatrix;

/*
 * safe_alloc_matrix -- Allocate a matrix with overflow checking
 *
 * Allocates a rows x cols matrix of int64_t values.
 * Checks for overflow in rows * cols * sizeof(int64_t) before allocation.
 *
 * Parameters:
 *   rows - Number of rows
 *   cols - Number of columns
 *
 * Returns:
 *   Pointer to SafeMatrix on success
 *   NULL if allocation failed or overflow detected
 */
SafeMatrix* safe_alloc_matrix(size_t rows, size_t cols);

/*
 * safe_free_matrix -- Free a SafeMatrix allocated by safe_alloc_matrix
 *
 * Parameters:
 *   m - Pointer to SafeMatrix to free (safe to call with NULL)
 */
void safe_free_matrix(SafeMatrix *m);

/*
 * validate_matrix_buffer -- Validate caller-provided buffer dimensions
 *
 * Checks that declared_rows * declared_cols does not exceed actual_capacity.
 * Used to validate buffers passed from callers that don't track capacity.
 *
 * Parameters:
 *   data                - Pointer to buffer (checked for NULL)
 *   declared_rows       - Declared number of rows
 *   declared_cols       - Declared number of columns
 *   actual_capacity     - Actual number of elements in buffer
 *
 * Returns:
 *   VER_OK              - Dimensions valid
 *   VER_NULL_INPUT      - data pointer is NULL
 *   VER_BUFFER_OVERFLOW - declared_rows * declared_cols > actual_capacity
 *   VER_OVERFLOW        - Multiplication overflowed
 */
VerifyResult validate_matrix_buffer(
    const int64_t *data,
    size_t declared_rows,
    size_t declared_cols,
    size_t actual_capacity
);

/*
 * ============================================================================
 * RESOURCE VALIDATION FUNCTIONS
 * ============================================================================
 */

/*
 * sov_check_dimensions -- Validate that dimensions do not exceed maximum
 *
 * Checks that both n and m are <= SOV_MAX_MATRIX_DIM.
 *
 * Parameters:
 *   n, m - Matrix dimensions to check
 *
 * Returns:
 *   VER_OK            - Dimensions are valid
 *   VER_DIMS_EXCEEDED - Either dimension exceeds maximum
 */
VerifyResult sov_check_dimensions(size_t n, size_t m);

/*
 * sov_check_matrix_cells -- Validate that matrix does not exceed cell limit
 *
 * Checks that rows * cols <= SOV_MAX_MATRIX_CELLS.
 * Includes overflow checking.
 *
 * Parameters:
 *   rows, cols - Matrix dimensions
 *
 * Returns:
 *   VER_OK           - Cell count is within limits
 *   VER_CELLS_EXCEEDED - Total cells exceed maximum
 *   VER_OVERFLOW     - Multiplication overflowed
 */
VerifyResult sov_check_matrix_cells(size_t rows, size_t cols);

/*
 * sov_add_operation_cost -- Track operation budget and check if exceeded
 *
 * Adds cost to the operation count in budget. Sets budget_exceeded if
 * operation_count would exceed max_operations.
 *
 * Parameters:
 *   budget - Pointer to SovResourceBudget struct
 *   cost   - Number of operations to add (typically m*n*k for multiplication)
 *
 * Returns:
 *   VER_OK          - Operation added within budget
 *   VER_OPS_EXCEEDED - Would exceed operation budget
 *   VER_NULL_INPUT  - budget pointer is NULL
 */
VerifyResult sov_add_operation_cost(SovResourceBudget *budget, size_t cost);

/*
 * sov_init_resource_budget -- Initialize a resource budget to defaults
 *
 * Parameters:
 *   budget - Pointer to SovResourceBudget struct to initialize
 *
 * Returns:
 *   VER_OK or VER_NULL_INPUT
 */
VerifyResult sov_init_resource_budget(SovResourceBudget *budget);

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
 * Enforces resource limits for dimension and operation budget.
 *
 * MEMORY SAFETY: Caller must provide A_len and X_len to indicate actual
 * buffer sizes. These are validated against n*n before computation.
 *
 * Parameters:
 *   A         - n x n matrix (row-major order, n*n elements)
 *   A_len     - Actual size of buffer A (in elements)
 *   X         - n x n matrix (row-major order, n*n elements)
 *   X_len     - Actual size of buffer X (in elements)
 *   n         - Matrix dimension (n x n)
 *   budget    - Optional resource budget (if NULL, defaults are used)
 *
 * Returns:
 *   VER_OK                    - A*X = I verified exactly
 *   VER_FAIL                  - A*X != I
 *   VER_OVERFLOW              - Arithmetic overflow or dimension overflow
 *   VER_DIMENSION_MISMATCH    - Inconsistent dimensions
 *   VER_NULL_INPUT            - Null pointer
 *   VER_BUFFER_OVERFLOW       - Buffer size validation failed
 *   VER_ALLOC_FAILURE         - Temporary buffer allocation failed
 *   VER_DIMS_EXCEEDED         - Dimension exceeds limit
 *   VER_CELLS_EXCEEDED        - Matrix cells exceed limit
 *   VER_OPS_EXCEEDED          - Operation budget exceeded
 */
VerifyResult sov_verify_inv(const int64_t *A,
                            size_t A_len,
                            const int64_t *X,
                            size_t X_len,
                            size_t n,
                            SovResourceBudget *budget);

/*
 * sov_verify_sol -- Verify that A*x = b (exact linear system solution)
 *
 * Given m x n matrix A, vector x (n elements), and vector b (m elements),
 * verify that A*x = b exactly.
 * Enforces resource limits for dimension and operation budget.
 *
 * MEMORY SAFETY: Caller must provide buffer lengths for validation.
 *
 * Parameters:
 *   A         - m x n matrix (row-major order, m*n elements)
 *   A_len     - Actual size of buffer A (in elements)
 *   x         - Solution vector (n elements)
 *   x_len     - Actual size of buffer x (in elements)
 *   b         - Right-hand side vector (m elements)
 *   b_len     - Actual size of buffer b (in elements)
 *   m, n      - Matrix dimensions
 *   budget    - Optional resource budget (if NULL, defaults are used)
 *
 * Returns:
 *   VER_OK                    - A*x = b verified exactly
 *   VER_FAIL                  - A*x != b
 *   VER_OVERFLOW              - Arithmetic overflow or dimension overflow
 *   VER_DIMENSION_MISMATCH    - Inconsistent dimensions
 *   VER_NULL_INPUT            - Null pointer
 *   VER_BUFFER_OVERFLOW       - Buffer size validation failed
 *   VER_ALLOC_FAILURE         - Temporary buffer allocation failed
 *   VER_DIMS_EXCEEDED         - Dimension exceeds limit
 *   VER_CELLS_EXCEEDED        - Matrix cells exceed limit
 *   VER_OPS_EXCEEDED          - Operation budget exceeded
 */
VerifyResult sov_verify_sol(const int64_t *A,
                            size_t A_len,
                            const int64_t *x,
                            size_t x_len,
                            const int64_t *b,
                            size_t b_len,
                            size_t m,
                            size_t n,
                            SovResourceBudget *budget);

/*
 * sov_verify_lstsq -- Verify that A^T(Ax-b) = 0 (exact least squares)
 *
 * Given m x n matrix A, vector x (n elements), vector b (m elements),
 * verify that the normal equations hold exactly: A^T(Ax-b) = 0.
 * Enforces resource limits for dimension and operation budget.
 *
 * This checks: for all i in [0, n):
 *   sum_j A[j][i] * (A[j][k] * x[k] - b[j]) = 0
 *
 * MEMORY SAFETY: Caller must provide buffer lengths for validation.
 *
 * Parameters:
 *   A         - m x n matrix (row-major order, m*n elements)
 *   A_len     - Actual size of buffer A (in elements)
 *   x         - Solution vector (n elements)
 *   x_len     - Actual size of buffer x (in elements)
 *   b         - Right-hand side vector (m elements)
 *   b_len     - Actual size of buffer b (in elements)
 *   m, n      - Matrix dimensions
 *   budget    - Optional resource budget (if NULL, defaults are used)
 *
 * Returns:
 *   VER_OK                    - A^T(Ax-b) = 0 verified exactly
 *   VER_FAIL                  - Normal equations don't hold
 *   VER_OVERFLOW              - Arithmetic overflow or dimension overflow
 *   VER_DIMENSION_MISMATCH    - Inconsistent dimensions
 *   VER_NULL_INPUT            - Null pointer
 *   VER_BUFFER_OVERFLOW       - Buffer size validation failed
 *   VER_ALLOC_FAILURE         - Temporary buffer allocation failed
 *   VER_DIMS_EXCEEDED         - Dimension exceeds limit
 *   VER_CELLS_EXCEEDED        - Matrix cells exceed limit
 *   VER_OPS_EXCEEDED          - Operation budget exceeded
 */
VerifyResult sov_verify_lstsq(const int64_t *A,
                              size_t A_len,
                              const int64_t *x,
                              size_t x_len,
                              const int64_t *b,
                              size_t b_len,
                              size_t m,
                              size_t n,
                              SovResourceBudget *budget);

/*
 * ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================
 */

/* Compute A*x into result vector (m elements) with resource budget tracking
 * Returns VER_OK on success, VER_OVERFLOW on overflow, VER_OPS_EXCEEDED if budget exhausted
 */
VerifyResult sov_matrix_vec_mult_with_budget(const int64_t *A,
                                              const int64_t *x,
                                              int64_t *result,
                                              size_t m,
                                              size_t n,
                                              SovResourceBudget *budget);

/* Compute A*x into result vector (m elements)
 * Returns VER_OK on success, VER_OVERFLOW on overflow
 */
VerifyResult sov_matrix_vec_mult(const int64_t *A,
                                  const int64_t *x,
                                  int64_t *result,
                                  size_t m,
                                  size_t n);

/* Compute A*B into result matrix (m x p) with resource budget tracking
 * A is m x n, B is n x p
 * Returns VER_OK on success, VER_OVERFLOW on overflow, VER_OPS_EXCEEDED if budget exhausted
 */
VerifyResult sov_matrix_mult_with_budget(const int64_t *A,
                                          const int64_t *B,
                                          int64_t *result,
                                          size_t m,
                                          size_t n,
                                          size_t p,
                                          SovResourceBudget *budget);

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

/*
 * sov_matrix_vec_mult_safe -- Safe matrix-vector multiply with buffer validation
 *
 * Like sov_matrix_vec_mult but validates buffer sizes first.
 *
 * Parameters:
 *   A       - m x n matrix
 *   A_len   - Actual size of A buffer
 *   x       - n-element vector
 *   x_len   - Actual size of x buffer
 *   result  - m-element result vector
 *   result_len - Actual size of result buffer
 *   m, n    - Matrix dimensions
 *
 * Returns:
 *   VER_OK on success, VER_BUFFER_OVERFLOW if buffers too small, VER_OVERFLOW on arithmetic overflow
 */
VerifyResult sov_matrix_vec_mult_safe(
    const int64_t *A,
    size_t A_len,
    const int64_t *x,
    size_t x_len,
    int64_t *result,
    size_t result_len,
    size_t m,
    size_t n
);

#ifdef __cplusplus
}
#endif

#endif /* SOV_VERIFIER_H */
