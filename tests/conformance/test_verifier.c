/*
 * test_verifier.c -- Conformance Tests for Sovereign Stack Machine Verifier
 *
 * FORGE Phase 2 Conformance
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "src/verifier/sov_verifier.h"

#define TEST_PASS(name) printf("[PASS] %s\n", name)
#define TEST_FAIL(name, reason) printf("[FAIL] %s: %s\n", name, reason)
#define ASSERT(cond, name, reason) do { if (!(cond)) { TEST_FAIL(name, reason); return 1; } } while(0)

static int test_verify_inv_identity_2x2(void)
{
    int64_t I[] = {1, 0, 0, 1};
    VerifyResult res = sov_verify_inv(I, I, 2);
    ASSERT(res == VER_OK, "test_verify_inv_identity_2x2", sov_verify_result_to_string(res));
    TEST_PASS("test_verify_inv_identity_2x2");
    return 0;
}

static int test_verify_inv_inverse_2x2(void)
{
    int64_t A[] = {1, 1, 0, 1};
    int64_t A_inv[] = {1, -1, 0, 1};
    VerifyResult res = sov_verify_inv(A, A_inv, 2);
    ASSERT(res == VER_OK, "test_verify_inv_inverse_2x2", sov_verify_result_to_string(res));
    TEST_PASS("test_verify_inv_inverse_2x2");
    return 0;
}

static int test_verify_inv_fail_not_inverse(void)
{
    int64_t A[] = {1, 2, 3, 4};
    int64_t B[] = {5, 6, 7, 8};
    VerifyResult res = sov_verify_inv(A, B, 2);
    ASSERT(res == VER_FAIL, "test_verify_inv_fail_not_inverse", sov_verify_result_to_string(res));
    TEST_PASS("test_verify_inv_fail_not_inverse");
    return 0;
}

static int test_verify_sol_2x2_system(void)
{
    int64_t A[] = {2, 0, 0, 2};
    int64_t x[] = {2, 3};
    int64_t b[] = {4, 6};
    VerifyResult res = sov_verify_sol(A, x, b, 2, 2);
    ASSERT(res == VER_OK, "test_verify_sol_2x2_system", sov_verify_result_to_string(res));
    TEST_PASS("test_verify_sol_2x2_system");
    return 0;
}

static int test_verify_sol_overdetermined(void)
{
    int64_t A[] = {1, 0, 0, 1, 1, 1};
    int64_t x[] = {1, 1};
    int64_t b[] = {1, 1, 2};
    VerifyResult res = sov_verify_sol(A, x, b, 3, 2);
    ASSERT(res == VER_OK, "test_verify_sol_overdetermined", sov_verify_result_to_string(res));
    TEST_PASS("test_verify_sol_overdetermined");
    return 0;
}

static int test_verify_sol_fail_wrong_solution(void)
{
    int64_t A[] = {2, 0, 0, 2};
    int64_t x[] = {1, 1};
    int64_t b[] = {4, 6};
    VerifyResult res = sov_verify_sol(A, x, b, 2, 2);
    ASSERT(res == VER_FAIL, "test_verify_sol_fail_wrong_solution", sov_verify_result_to_string(res));
    TEST_PASS("test_verify_sol_fail_wrong_solution");
    return 0;
}

static int test_verify_lstsq_perfect_system(void)
{
    int64_t A[] = {1, 0, 0, 1};
    int64_t x[] = {1, 2};
    int64_t b[] = {1, 2};
    VerifyResult res = sov_verify_lstsq(A, x, b, 2, 2);
    ASSERT(res == VER_OK, "test_verify_lstsq_perfect_system", sov_verify_result_to_string(res));
    TEST_PASS("test_verify_lstsq_perfect_system");
    return 0;
}

static int test_verify_lstsq_overdetermined_exact(void)
{
    int64_t A[] = {1, 0, 0, 1, 1, 1};
    int64_t x[] = {1, 1};
    int64_t b[] = {1, 1, 2};
    VerifyResult res = sov_verify_lstsq(A, x, b, 3, 2);
    ASSERT(res == VER_OK, "test_verify_lstsq_overdetermined_exact", sov_verify_result_to_string(res));
    TEST_PASS("test_verify_lstsq_overdetermined_exact");
    return 0;
}

static int test_verify_lstsq_fail_not_solution(void)
{
    int64_t A[] = {1, 0, 0, 1};
    int64_t x[] = {2, 3};
    int64_t b[] = {1, 2};
    VerifyResult res = sov_verify_lstsq(A, x, b, 2, 2);
    ASSERT(res == VER_FAIL, "test_verify_lstsq_fail_not_solution", sov_verify_result_to_string(res));
    TEST_PASS("test_verify_lstsq_fail_not_solution");
    return 0;
}

static int test_overflow_detection(void)
{
    int64_t A[] = {INT64_MAX, 0, 0, 1};
    int64_t x[] = {2, 1};
    int64_t result[2];
    VerifyResult res = sov_matrix_vec_mult(A, x, result, 2, 2);
    ASSERT(res == VER_OVERFLOW, "test_overflow_detection", sov_verify_result_to_string(res));
    TEST_PASS("test_overflow_detection");
    return 0;
}

static int test_null_input_handling(void)
{
    int64_t A[] = {1, 0, 0, 1};
    VerifyResult res = sov_verify_inv(NULL, A, 2);
    ASSERT(res == VER_NULL_INPUT, "test_null_input_handling", sov_verify_result_to_string(res));
    TEST_PASS("test_null_input_handling");
    return 0;
}

int main(void)
{
    int failed = 0;

    printf("=== Sovereign Stack Machine Verifier Conformance Tests ===\n\n");

    printf("[TEST GROUP] Matrix Inversion (A*X = I)\n");
    failed += test_verify_inv_identity_2x2();
    failed += test_verify_inv_inverse_2x2();
    failed += test_verify_inv_fail_not_inverse();
    printf("\n");

    printf("[TEST GROUP] Linear System Solution (A*x = b)\n");
    failed += test_verify_sol_2x2_system();
    failed += test_verify_sol_overdetermined();
    failed += test_verify_sol_fail_wrong_solution();
    printf("\n");

    printf("[TEST GROUP] Least Squares (A^T(Ax-b) = 0)\n");
    failed += test_verify_lstsq_perfect_system();
    failed += test_verify_lstsq_overdetermined_exact();
    failed += test_verify_lstsq_fail_not_solution();
    printf("\n");

    printf("[TEST GROUP] Error Handling\n");
    failed += test_overflow_detection();
    failed += test_null_input_handling();
    printf("\n");

    if (failed == 0) {
        printf("=== ALL TESTS PASSED ===\n");
    } else {
        printf("=== %d TESTS FAILED ===\n", failed);
    }

    return failed;
}
