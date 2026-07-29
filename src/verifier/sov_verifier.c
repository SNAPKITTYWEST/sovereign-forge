/*
 * sov_verifier.c -- Sovereign Stack Machine Verifier Implementation
 *
 * Matrix verification with exact int64_t arithmetic and overflow detection.
 *
 * FORGE Phase 2
 * License: Apache 2.0 + AGPL 3.0
 */

#include "sov_verifier.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

VerifyResult sov_matrix_vec_mult(const int64_t *A,
                                  const int64_t *x,
                                  int64_t *result,
                                  size_t m,
                                  size_t n)
{
    if (!A || !x || !result) {
        return VER_NULL_INPUT;
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

VerifyResult sov_matrix_mult(const int64_t *A,
                              const int64_t *B,
                              int64_t *result,
                              size_t m,
                              size_t n,
                              size_t p)
{
    if (!A || !B || !result) {
        return VER_NULL_INPUT;
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
                            const int64_t *X,
                            size_t n)
{
    if (!A || !X) {
        return VER_NULL_INPUT;
    }

    if (n == 0) {
        return VER_DIMENSION_MISMATCH;
    }

    int64_t *product = (int64_t *)malloc(n * n * sizeof(int64_t));
    if (!product) {
        return VER_FAIL;
    }

    VerifyResult res = sov_matrix_mult(A, X, product, n, n, n);
    if (res != VER_OK) {
        free(product);
        return res;
    }

    bool is_identity = true;
    for (size_t i = 0; i < n && is_identity; i++) {
        for (size_t j = 0; j < n; j++) {
            int64_t expected = (i == j) ? 1 : 0;
            if (product[i * n + j] != expected) {
                is_identity = false;
                break;
            }
        }
    }

    free(product);
    return is_identity ? VER_OK : VER_FAIL;
}

VerifyResult sov_verify_sol(const int64_t *A,
                            const int64_t *x,
                            const int64_t *b,
                            size_t m,
                            size_t n)
{
    if (!A || !x || !b) {
        return VER_NULL_INPUT;
    }

    if (m == 0 || n == 0) {
        return VER_DIMENSION_MISMATCH;
    }

    int64_t *product = (int64_t *)malloc(m * sizeof(int64_t));
    if (!product) {
        return VER_FAIL;
    }

    VerifyResult res = sov_matrix_vec_mult(A, x, product, m, n);
    if (res != VER_OK) {
        free(product);
        return res;
    }

    bool equal = sov_matrix_equal(product, b, m);

    free(product);
    return equal ? VER_OK : VER_FAIL;
}

VerifyResult sov_verify_lstsq(const int64_t *A,
                              const int64_t *x,
                              const int64_t *b,
                              size_t m,
                              size_t n)
{
    if (!A || !x || !b) {
        return VER_NULL_INPUT;
    }

    if (m == 0 || n == 0) {
        return VER_DIMENSION_MISMATCH;
    }

    int64_t *ax = (int64_t *)malloc(m * sizeof(int64_t));
    if (!ax) {
        return VER_FAIL;
    }

    VerifyResult res = sov_matrix_vec_mult(A, x, ax, m, n);
    if (res != VER_OK) {
        free(ax);
        return res;
    }

    int64_t *residual = (int64_t *)malloc(m * sizeof(int64_t));
    if (!residual) {
        free(ax);
        return VER_FAIL;
    }

    for (size_t i = 0; i < m; i++) {
        if (__builtin_sub_overflow(ax[i], b[i], &residual[i])) {
            free(ax);
            free(residual);
            return VER_OVERFLOW;
        }
    }

    int64_t *result = (int64_t *)malloc(n * sizeof(int64_t));
    if (!result) {
        free(ax);
        free(residual);
        return VER_FAIL;
    }

    for (size_t i = 0; i < n; i++) {
        result[i] = 0;
        for (size_t j = 0; j < m; j++) {
            int64_t aji = A[j * n + i];
            int64_t rj = residual[j];

            res = checked_muladd(&result[i], aji, rj);
            if (res != VER_OK) {
                free(ax);
                free(residual);
                free(result);
                return res;
            }
        }
    }

    bool all_zero = true;
    for (size_t i = 0; i < n; i++) {
        if (result[i] != 0) {
            all_zero = false;
            break;
        }
    }

    free(ax);
    free(residual);
    free(result);

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
    default:
        return "UNKNOWN";
    }
}
