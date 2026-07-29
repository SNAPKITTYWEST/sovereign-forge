/*
 * sov_verifier.c -- Sovereign Stack Machine Verifier Implementation
 *
 * Matrix verification with exact int64_t arithmetic and overflow detection.
 * Memory safety: safe allocation with overflow checking, buffer validation.
 *
 * FORGE Phase 2
 * License: Apache 2.0 + AGPL 3.0
 */

#include "sov_verifier.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/*
 * ============================================================================
 * SAFE MEMORY MANAGEMENT FUNCTIONS
 * ============================================================================
 */

SafeMatrix* safe_alloc_matrix(size_t rows, size_t cols)
{
    /* Check for overflow in rows * cols */
    if (rows > 0 && cols > SIZE_MAX / rows) {
        return NULL;
    }

    size_t nelems = rows * cols;

    /* Check for overflow in nelems * sizeof(int64_t) */
    if (nelems > 0 && nelems > SIZE_MAX / sizeof(int64_t)) {
        return NULL;
    }

    size_t data_size = nelems * sizeof(int64_t);

    /* Allocate the SafeMatrix struct */
    SafeMatrix *m = (SafeMatrix *)malloc(sizeof(SafeMatrix));
    if (!m) {
        return NULL;
    }

    /* Allocate the data buffer */
    if (nelems > 0) {
        m->data = (int64_t *)malloc(data_size);
        if (!m->data) {
            free(m);
            return NULL;
        }
    } else {
        m->data = NULL;
    }

    m->rows = rows;
    m->cols = cols;
    m->capacity = nelems;

    return m;
}

void safe_free_matrix(SafeMatrix *m)
{
    if (!m) {
        return;
    }
    if (m->data) {
        free(m->data);
    }
    free(m);
}

VerifyResult validate_matrix_buffer(
    const int64_t *data,
    size_t declared_rows,
    size_t declared_cols,
    size_t actual_capacity)
{
    if (!data) {
        return VER_NULL_INPUT;
    }

    /* Check for overflow in declared_rows * declared_cols */
    if (declared_rows > 0 && declared_cols > SIZE_MAX / declared_rows) {
        return VER_OVERFLOW;
    }

    size_t required_size = declared_rows * declared_cols;

    /* Check if required size exceeds actual capacity */
    if (required_size > actual_capacity) {
        return VER_BUFFER_OVERFLOW;
    }

    return VER_OK;
}

/*
 * ============================================================================
 * RESOURCE LIMIT CHECKING FUNCTIONS (Phase 1, Step 2)
 * ============================================================================
 */

VerifyResult sov_check_dimensions(size_t n, size_t m)
{
    if (n > SOV_MAX_MATRIX_DIM || m > SOV_MAX_MATRIX_DIM) {
        return VER_DIMS_EXCEEDED;
    }
    return VER_OK;
}

VerifyResult sov_check_matrix_cells(size_t rows, size_t cols)
{
    /* Check for overflow in rows * cols */
    if (rows > 0 && cols > SIZE_MAX / rows) {
        return VER_OVERFLOW;
    }

    size_t total_cells = rows * cols;

    /* Check if total cells exceed limit */
    if (total_cells > SOV_MAX_MATRIX_CELLS) {
        return VER_CELLS_EXCEEDED;
    }

    return VER_OK;
}

VerifyResult sov_add_operation_cost(SovResourceBudget *budget, size_t cost)
{
    if (!budget) {
        return VER_NULL_INPUT;
    }

    /* Check for overflow in operation_count + cost */
    if (cost > 0 && budget->operation_count > SIZE_MAX - cost) {
        budget->budget_exceeded = true;
        return VER_OPS_EXCEEDED;
    }

    budget->operation_count += cost;

    /* Check if we've exceeded the budget */
    if (budget->operation_count > budget->max_operations) {
        budget->budget_exceeded = true;
        return VER_OPS_EXCEEDED;
    }

    return VER_OK;
}

VerifyResult sov_init_resource_budget(SovResourceBudget *budget)
{
    if (!budget) {
        return VER_NULL_INPUT;
    }

    budget->max_dimensions = SOV_MAX_MATRIX_DIM;
    budget->max_cells = SOV_MAX_MATRIX_CELLS;
    budget->max_operations = SOV_OPERATION_BUDGET_PER_CALL;
    budget->operation_count = 0;
    budget->budget_exceeded = false;

    return VER_OK;
}

VerifyResult sov_matrix_vec_mult_safe(
    const int64_t *A,
    size_t A_len,
    const int64_t *x,
    size_t x_len,
    int64_t *result,
    size_t result_len,
    size_t m,
    size_t n)
{
    /* Validate buffer sizes */
    VerifyResult res = validate_matrix_buffer(A, m, n, A_len);
    if (res != VER_OK) {
        return res;
    }

    res = validate_matrix_buffer(x, 1, n, x_len);
    if (res != VER_OK) {
        return res;
    }

    res = validate_matrix_buffer(result, 1, m, result_len);
    if (res != VER_OK) {
        return res;
    }

    /* Call unsafe version (now we know buffers are valid) */
    return sov_matrix_vec_mult(A, x, result, m, n);
}

/*
 * ============================================================================
 * ARITHMETIC HELPER
 * ============================================================================
 */

static VerifyResult checked_muladd(int64_t *c, int64_t a, int64_t b)
{
    int64_t product;
    if (__builtin_mul_overflow(a, b, &product)) {
        return VER_OVERFLOW;
    }
    if (__builtin_add_overflow(*c, product, c)) {
        return VER_OVERFLOW;
    }
    return VER_OK;
}

VerifyResult sov_matrix_vec_mult_with_budget(const int64_t *A,
                                               const int64_t *x,
                                               int64_t *result,
                                               size_t m,
                                               size_t n,
                                               SovResourceBudget *budget)
{
    if (!A || !x || !result) {
        return VER_NULL_INPUT;
    }

    /* Track operation cost: m * n operations */
    if (budget) {
        if (m > 0 && n > SIZE_MAX / m) {
            return VER_OVERFLOW;
        }
        size_t total_ops = m * n;
        VerifyResult res = sov_add_operation_cost(budget, total_ops);
        if (res != VER_OK) {
            return res;
        }
    }

    for (size_t i = 0; i < m; i++) {
        result[i] = 0;
        for (size_t j = 0; j < n; j++) {
            int64_t a = A[i * n + j];
            int64_t xj = x[j];

            VerifyResult res = checked_muladd(&result[i], a, xj);
            if (res != VER_OK) {
                return res;
            }
        }
    }

    return VER_OK;
}

VerifyResult sov_matrix_vec_mult(const int64_t *A,
                                  const int64_t *x,
                                  int64_t *result,
                                  size_t m,
                                  size_t n)
{
    return sov_matrix_vec_mult_with_budget(A, x, result, m, n, NULL);
}

VerifyResult sov_matrix_mult_with_budget(const int64_t *A,
                                           const int64_t *B,
                                           int64_t *result,
                                           size_t m,
                                           size_t n,
                                           size_t p,
                                           SovResourceBudget *budget)
{
    if (!A || !B || !result) {
        return VER_NULL_INPUT;
    }

    /* Track operation cost: m * n * p operations */
    if (budget) {
        if (m > 0 && n > SIZE_MAX / m) {
            return VER_OVERFLOW;
        }
        size_t mn = m * n;
        if (mn > 0 && p > SIZE_MAX / mn) {
            return VER_OVERFLOW;
        }
        size_t total_ops = mn * p;
        VerifyResult res = sov_add_operation_cost(budget, total_ops);
        if (res != VER_OK) {
            return res;
        }
    }

    for (size_t i = 0; i < m; i++) {
        for (size_t j = 0; j < p; j++) {
            result[i * p + j] = 0;

            for (size_t k = 0; k < n; k++) {
                int64_t a = A[i * n + k];
                int64_t b = B[k * p + j];

                VerifyResult res = checked_muladd(&result[i * p + j], a, b);
                if (res != VER_OK) {
                    return res;
                }
            }
        }
    }

    return VER_OK;
}

VerifyResult sov_matrix_mult(const int64_t *A,
                              const int64_t *B,
                              int64_t *result,
                              size_t m,
                              size_t n,
                              size_t p)
{
    return sov_matrix_mult_with_budget(A, B, result, m, n, p, NULL);
}

VerifyResult sov_matrix_transpose(const int64_t *A,
                                   int64_t *result,
                                   size_t m,
                                   size_t n)
{
    if (!A || !result) {
        return VER_NULL_INPUT;
    }

    for (size_t i = 0; i < m; i++) {
        for (size_t j = 0; j < n; j++) {
            result[j * m + i] = A[i * n + j];
        }
    }

    return VER_OK;
}

bool sov_matrix_equal(const int64_t *A,
                       const int64_t *B,
                       size_t nelems)
{
    if (!A || !B) {
        return false;
    }

    for (size_t i = 0; i < nelems; i++) {
        if (A[i] != B[i]) {
            return false;
        }
    }

    return true;
}

VerifyResult sov_verify_inv(const int64_t *A,
                            size_t A_len,
                            const int64_t *X,
                            size_t X_len,
                            size_t n,
                            SovResourceBudget *budget)
{
    if (!A || !X) {
        return VER_NULL_INPUT;
    }

    if (n == 0) {
        return VER_DIMENSION_MISMATCH;
    }

    /* Initialize budget if not provided */
    SovResourceBudget default_budget;
    if (!budget) {
        VerifyResult res = sov_init_resource_budget(&default_budget);
        if (res != VER_OK) {
            return res;
        }
        budget = &default_budget;
    }

    /* Check resource limits */
    VerifyResult res = sov_check_dimensions(n, n);
    if (res != VER_OK) {
        return res;
    }

    res = sov_check_matrix_cells(n, n);
    if (res != VER_OK) {
        return res;
    }

    /* Validate input buffer sizes */
    res = validate_matrix_buffer(A, n, n, A_len);
    if (res != VER_OK) {
        return res;
    }

    res = validate_matrix_buffer(X, n, n, X_len);
    if (res != VER_OK) {
        return res;
    }

    /* Safely allocate temporary product matrix */
    SafeMatrix *product = safe_alloc_matrix(n, n);
    if (!product) {
        return VER_ALLOC_FAILURE;
    }

    res = sov_matrix_mult_with_budget(A, X, product->data, n, n, n, budget);
    if (res != VER_OK) {
        safe_free_matrix(product);
        return res;
    }

    bool is_identity = true;
    for (size_t i = 0; i < n && is_identity; i++) {
        for (size_t j = 0; j < n; j++) {
            int64_t expected = (i == j) ? 1 : 0;
            if (product->data[i * n + j] != expected) {
                is_identity = false;
                break;
            }
        }
    }

    safe_free_matrix(product);
    return is_identity ? VER_OK : VER_FAIL;
}

VerifyResult sov_verify_sol(const int64_t *A,
                            size_t A_len,
                            const int64_t *x,
                            size_t x_len,
                            const int64_t *b,
                            size_t b_len,
                            size_t m,
                            size_t n,
                            SovResourceBudget *budget)
{
    if (!A || !x || !b) {
        return VER_NULL_INPUT;
    }

    if (m == 0 || n == 0) {
        return VER_DIMENSION_MISMATCH;
    }

    /* Initialize budget if not provided */
    SovResourceBudget default_budget;
    if (!budget) {
        VerifyResult res = sov_init_resource_budget(&default_budget);
        if (res != VER_OK) {
            return res;
        }
        budget = &default_budget;
    }

    /* Check resource limits */
    VerifyResult res = sov_check_dimensions(m, n);
    if (res != VER_OK) {
        return res;
    }

    res = sov_check_matrix_cells(m, n);
    if (res != VER_OK) {
        return res;
    }

    /* Validate input buffer sizes */
    res = validate_matrix_buffer(A, m, n, A_len);
    if (res != VER_OK) {
        return res;
    }

    res = validate_matrix_buffer(x, 1, n, x_len);
    if (res != VER_OK) {
        return res;
    }

    res = validate_matrix_buffer(b, 1, m, b_len);
    if (res != VER_OK) {
        return res;
    }

    /* Safely allocate temporary product vector */
    SafeMatrix *product = safe_alloc_matrix(1, m);
    if (!product) {
        return VER_ALLOC_FAILURE;
    }

    res = sov_matrix_vec_mult_with_budget(A, x, product->data, m, n, budget);
    if (res != VER_OK) {
        safe_free_matrix(product);
        return res;
    }

    bool equal = sov_matrix_equal(product->data, b, m);

    safe_free_matrix(product);
    return equal ? VER_OK : VER_FAIL;
}

VerifyResult sov_verify_lstsq(const int64_t *A,
                              size_t A_len,
                              const int64_t *x,
                              size_t x_len,
                              const int64_t *b,
                              size_t b_len,
                              size_t m,
                              size_t n,
                              SovResourceBudget *budget)
{
    if (!A || !x || !b) {
        return VER_NULL_INPUT;
    }

    if (m == 0 || n == 0) {
        return VER_DIMENSION_MISMATCH;
    }

    /* Initialize budget if not provided */
    SovResourceBudget default_budget;
    if (!budget) {
        VerifyResult res = sov_init_resource_budget(&default_budget);
        if (res != VER_OK) {
            return res;
        }
        budget = &default_budget;
    }

    /* Check resource limits */
    VerifyResult res = sov_check_dimensions(m, n);
    if (res != VER_OK) {
        return res;
    }

    res = sov_check_matrix_cells(m, n);
    if (res != VER_OK) {
        return res;
    }

    /* Validate input buffer sizes */
    res = validate_matrix_buffer(A, m, n, A_len);
    if (res != VER_OK) {
        return res;
    }

    res = validate_matrix_buffer(x, 1, n, x_len);
    if (res != VER_OK) {
        return res;
    }

    res = validate_matrix_buffer(b, 1, m, b_len);
    if (res != VER_OK) {
        return res;
    }

    /* Safely allocate temporary vectors */
    SafeMatrix *ax = safe_alloc_matrix(1, m);
    if (!ax) {
        return VER_ALLOC_FAILURE;
    }

    res = sov_matrix_vec_mult_with_budget(A, x, ax->data, m, n, budget);
    if (res != VER_OK) {
        safe_free_matrix(ax);
        return res;
    }

    SafeMatrix *residual = safe_alloc_matrix(1, m);
    if (!residual) {
        safe_free_matrix(ax);
        return VER_ALLOC_FAILURE;
    }

    for (size_t i = 0; i < m; i++) {
        if (__builtin_sub_overflow(ax->data[i], b[i], &residual->data[i])) {
            safe_free_matrix(ax);
            safe_free_matrix(residual);
            return VER_OVERFLOW;
        }
    }

    SafeMatrix *result = safe_alloc_matrix(1, n);
    if (!result) {
        safe_free_matrix(ax);
        safe_free_matrix(residual);
        return VER_ALLOC_FAILURE;
    }

    /* Track operations for A^T * residual: m * n operations */
    res = sov_add_operation_cost(budget, m * n);
    if (res != VER_OK) {
        safe_free_matrix(ax);
        safe_free_matrix(residual);
        safe_free_matrix(result);
        return res;
    }

    for (size_t i = 0; i < n; i++) {
        result->data[i] = 0;
        for (size_t j = 0; j < m; j++) {
            int64_t aji = A[j * n + i];
            int64_t rj = residual->data[j];

            res = checked_muladd(&result->data[i], aji, rj);
            if (res != VER_OK) {
                safe_free_matrix(ax);
                safe_free_matrix(residual);
                safe_free_matrix(result);
                return res;
            }
        }
    }

    bool all_zero = true;
    for (size_t i = 0; i < n; i++) {
        if (result->data[i] != 0) {
            all_zero = false;
            break;
        }
    }

    safe_free_matrix(ax);
    safe_free_matrix(residual);
    safe_free_matrix(result);

    return all_zero ? VER_OK : VER_FAIL;
}

const char *sov_verify_result_to_string(VerifyResult r)
{
    switch (r) {
    case VER_OK:
        return "OK";
    case VER_FAIL:
        return "FAIL";
    case VER_OVERFLOW:
        return "OVERFLOW";
    case VER_DIMENSION_MISMATCH:
        return "DIMENSION_MISMATCH";
    case VER_NULL_INPUT:
        return "NULL_INPUT";
    case VER_SINGULAR:
        return "SINGULAR";
    case VER_ALLOC_FAILURE:
        return "ALLOC_FAILURE";
    case VER_BUFFER_OVERFLOW:
        return "BUFFER_OVERFLOW";
    case VER_DIMS_EXCEEDED:
        return "DIMS_EXCEEDED";
    case VER_CELLS_EXCEEDED:
        return "CELLS_EXCEEDED";
    case VER_OPS_EXCEEDED:
        return "OPS_EXCEEDED";
    case VER_RESOURCE_EXCEEDED:
        return "RESOURCE_EXCEEDED";
    default:
        return "UNKNOWN";
    }
}
