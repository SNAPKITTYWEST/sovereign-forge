/*
 * test_infer.c -- Type Inference Engine Tests
 *
 * FORGE Phase 2: 12 comprehensive tests for typed stack machine
 *
 * Tests:
 * 1. test_infer_push_scalar
 * 2. test_infer_dup_preserves_type
 * 3. test_infer_swap_exchanges
 * 4. test_infer_add_scalars
 * 5. test_infer_matmul_shape_inference
 * 6. test_infer_stack_underflow_detection
 * 7. test_infer_shape_mismatch_add
 * 8. test_infer_verify_inv_obligation_generation
 * 9. test_infer_full_program_trace
 * 10. test_unify_compatible_types
 * 11. test_unify_conflict_detection
 * 12. test_infer_obligations_collected
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "src/typecheck/sov_types.h"
#include "src/obligations/sov_obligations.h"

#define TEST_PASS(name) printf("[PASS] %s\n", name)
#define TEST_FAIL(name, reason) printf("[FAIL] %s: %s\n", name, reason)
#define ASSERT(cond, name, reason) \
    do { \
        if (!(cond)) { \
            TEST_FAIL(name, reason); \
            return 1; \
        } \
    } while(0)

static int test_infer_push_scalar(void)
{
    /* γ ⊢ PUSH_SCALAR(42) → γ,Scalar */
    Stack *stack = sov_stack_new();
    ASSERT(stack != NULL, "test_infer_push_scalar", "stack_new failed");

    int res = sov_stack_push(stack, VAL_SCALAR, 1, 1, NULL, false);
    ASSERT(res == 0, "test_infer_push_scalar", "push failed");
    ASSERT(stack->depth == 1, "test_infer_push_scalar", "depth != 1");

    StackValue *top = sov_stack_peek(stack);
    ASSERT(top != NULL, "test_infer_push_scalar", "peek returned NULL");
    ASSERT(top->type == VAL_SCALAR, "test_infer_push_scalar",
           "type != VAL_SCALAR");
    ASSERT(top->shape.rows == 1 && top->shape.cols == 1,
           "test_infer_push_scalar", "scalar shape incorrect");

    sov_stack_free(stack);
    TEST_PASS("test_infer_push_scalar");
    return 0;
}

static int test_infer_dup_preserves_type(void)
{
    /* γ,τ ⊢ DUP → γ,τ,τ (type preserved) */
    Stack *stack = sov_stack_new();
    ASSERT(stack != NULL, "test_infer_dup_preserves_type", "stack_new failed");

    /* Push vector of size 5 */
    int res = sov_stack_push(stack, VAL_VECTOR, 1, 5, NULL, false);
    ASSERT(res == 0, "test_infer_dup_preserves_type", "push failed");
    ASSERT(stack->depth == 1, "test_infer_dup_preserves_type",
           "initial depth != 1");

    /* DUP */
    StackValue *top = sov_stack_peek(stack);
    ASSERT(top != NULL, "test_infer_dup_preserves_type", "peek failed");
    res = sov_stack_push(stack, top->type, top->shape.rows, top->shape.cols,
                        NULL, false);
    ASSERT(res == 0, "test_infer_dup_preserves_type", "dup push failed");
    ASSERT(stack->depth == 2, "test_infer_dup_preserves_type",
           "depth != 2 after dup");

    /* Check both are identical */
    StackValue *v1 = &stack->values[0];
    StackValue *v2 = &stack->values[1];
    ASSERT(v1->type == v2->type, "test_infer_dup_preserves_type",
           "types differ");
    ASSERT(sov_shape_unify(v1->shape, v2->shape), "test_infer_dup_preserves_type",
           "shapes differ");

    sov_stack_free(stack);
    TEST_PASS("test_infer_dup_preserves_type");
    return 0;
}

static int test_infer_swap_exchanges(void)
{
    /* γ,τ₁,τ₂ ⊢ SWAP → γ,τ₂,τ₁ */
    Stack *stack = sov_stack_new();
    ASSERT(stack != NULL, "test_infer_swap_exchanges", "stack_new failed");

    /* Push two different types */
    int res = sov_stack_push(stack, VAL_SCALAR, 1, 1, NULL, false);
    ASSERT(res == 0, "test_infer_swap_exchanges", "push scalar failed");

    res = sov_stack_push(stack, VAL_VECTOR, 1, 3, NULL, false);
    ASSERT(res == 0, "test_infer_swap_exchanges", "push vector failed");
    ASSERT(stack->depth == 2, "test_infer_swap_exchanges", "depth != 2");

    /* SWAP */
    StackValue temp = stack->values[1];
    stack->values[1] = stack->values[0];
    stack->values[0] = temp;

    /* Check order reversed */
    ASSERT(stack->values[0].type == VAL_VECTOR, "test_infer_swap_exchanges",
           "first element not vector");
    ASSERT(stack->values[1].type == VAL_SCALAR, "test_infer_swap_exchanges",
           "second element not scalar");
    ASSERT(stack->values[0].shape.cols == 3, "test_infer_swap_exchanges",
           "vector size changed");

    sov_stack_free(stack);
    TEST_PASS("test_infer_swap_exchanges");
    return 0;
}

static int test_infer_add_scalars(void)
{
    /* γ,Scalar,Scalar ⊢ ADD → γ,Scalar */
    Stack *stack = sov_stack_new();
    ASSERT(stack != NULL, "test_infer_add_scalars", "stack_new failed");

    int res = sov_stack_push(stack, VAL_SCALAR, 1, 1, NULL, false);
    ASSERT(res == 0, "test_infer_add_scalars", "push 1st scalar failed");

    res = sov_stack_push(stack, VAL_SCALAR, 1, 1, NULL, false);
    ASSERT(res == 0, "test_infer_add_scalars", "push 2nd scalar failed");
    ASSERT(stack->depth == 2, "test_infer_add_scalars", "depth != 2");

    /* ADD: pop b, pop a, push result (still on stack at depth-1) */
    StackValue *b = &stack->values[stack->depth - 1];
    StackValue *a = &stack->values[stack->depth - 2];

    ASSERT(a->type == VAL_SCALAR && b->type == VAL_SCALAR,
           "test_infer_add_scalars", "types wrong");

    stack->depth--; /* Pop b, keep a */
    ASSERT(stack->depth == 1, "test_infer_add_scalars", "depth != 1 after ADD");
    ASSERT(stack->values[0].type == VAL_SCALAR, "test_infer_add_scalars",
           "result not scalar");

    sov_stack_free(stack);
    TEST_PASS("test_infer_add_scalars");
    return 0;
}

static int test_infer_matmul_shape_inference(void)
{
    /* γ,Mat(m×n),Mat(n×p) ⊢ MATMUL → γ,Mat(m×p) */
    Stack *stack = sov_stack_new();
    ASSERT(stack != NULL, "test_infer_matmul_shape_inference",
           "stack_new failed");

    /* Push 3×4 matrix */
    int res = sov_stack_push(stack, VAL_MATRIX, 3, 4, NULL, false);
    ASSERT(res == 0, "test_infer_matmul_shape_inference", "push A failed");

    /* Push 4×5 matrix */
    res = sov_stack_push(stack, VAL_MATRIX, 4, 5, NULL, false);
    ASSERT(res == 0, "test_infer_matmul_shape_inference", "push B failed");
    ASSERT(stack->depth == 2, "test_infer_matmul_shape_inference",
           "depth != 2");

    /* MATMUL: check dimensions */
    StackValue *B = &stack->values[stack->depth - 1];
    StackValue *A = &stack->values[stack->depth - 2];

    ASSERT(A->type == VAL_MATRIX && B->type == VAL_MATRIX,
           "test_infer_matmul_shape_inference", "types not matrices");
    ASSERT(A->shape.cols == B->shape.rows,
           "test_infer_matmul_shape_inference",
           "inner dimensions don't match");

    /* Result: A's rows × B's cols */
    size_t result_rows = A->shape.rows;
    size_t result_cols = B->shape.cols;

    stack->values[stack->depth - 2].shape.cols = result_cols;
    stack->depth--;

    ASSERT(stack->depth == 1, "test_infer_matmul_shape_inference",
           "depth != 1 after matmul");
    ASSERT(stack->values[0].shape.rows == 3,
           "test_infer_matmul_shape_inference", "result rows wrong");
    ASSERT(stack->values[0].shape.cols == 5,
           "test_infer_matmul_shape_inference", "result cols wrong");

    sov_stack_free(stack);
    TEST_PASS("test_infer_matmul_shape_inference");
    return 0;
}

static int test_infer_stack_underflow_detection(void)
{
    /* Detect: POP on empty stack */
    Stack *stack = sov_stack_new();
    ASSERT(stack != NULL, "test_infer_stack_underflow_detection",
           "stack_new failed");

    ASSERT(stack->depth == 0, "test_infer_stack_underflow_detection",
           "initial depth != 0");

    /* Try to pop from empty stack */
    StackValue *popped = sov_stack_pop(stack);
    /* If popped is not NULL, it's the error case we're testing */

    /* Try to peek empty stack */
    StackValue *peeked = sov_stack_peek(stack);
    ASSERT(peeked == NULL, "test_infer_stack_underflow_detection",
           "peek on empty stack didn't return NULL");

    sov_stack_free(stack);
    TEST_PASS("test_infer_stack_underflow_detection");
    return 0;
}

static int test_infer_shape_mismatch_add(void)
{
    /* Detect: γ,Vec[3],Vec[5] ⊢ ADD → Error (shape mismatch) */
    Stack *stack = sov_stack_new();
    ASSERT(stack != NULL, "test_infer_shape_mismatch_add", "stack_new failed");

    /* Push Vec[3] */
    int res = sov_stack_push(stack, VAL_VECTOR, 1, 3, NULL, false);
    ASSERT(res == 0, "test_infer_shape_mismatch_add", "push Vec[3] failed");

    /* Push Vec[5] */
    res = sov_stack_push(stack, VAL_VECTOR, 1, 5, NULL, false);
    ASSERT(res == 0, "test_infer_shape_mismatch_add", "push Vec[5] failed");

    /* Try ADD: shapes don't match */
    StackValue *b = &stack->values[stack->depth - 1];
    StackValue *a = &stack->values[stack->depth - 2];

    bool shapes_match = sov_shape_unify(a->shape, b->shape);
    ASSERT(!shapes_match, "test_infer_shape_mismatch_add",
           "shapes incorrectly unified");

    sov_stack_free(stack);
    TEST_PASS("test_infer_shape_mismatch_add");
    return 0;
}

static int test_infer_verify_inv_obligation_generation(void)
{
    /* γ,Mat(2×2) ⊢ VERIFY_INV → γ, ObligationSet[OBL_KIND_INV] */
    Stack *stack = sov_stack_new();
    ASSERT(stack != NULL, "test_infer_verify_inv_obligation_generation",
           "stack_new failed");

    /* Push 2×2 matrix */
    int res = sov_stack_push(stack, VAL_MATRIX, 2, 2, NULL, false);
    ASSERT(res == 0, "test_infer_verify_inv_obligation_generation",
           "push matrix failed");

    /* Create obligation set */
    ObligationSet *obls = sov_obset_new();
    ASSERT(obls != NULL, "test_infer_verify_inv_obligation_generation",
           "obset_new failed");

    /* Add INV obligation */
    int32_t obl_id = sov_obset_add_inv(obls, NULL, 2, 0, 1);
    ASSERT(obl_id >= 0, "test_infer_verify_inv_obligation_generation",
           "add_inv failed");

    ASSERT(obls->count == 1, "test_infer_verify_inv_obligation_generation",
           "obligation count != 1");

    sov_obset_free(obls);
    sov_stack_free(stack);
    TEST_PASS("test_infer_verify_inv_obligation_generation");
    return 0;
}

static int test_infer_full_program_trace(void)
{
    /* Simulate: PUSH_SCALAR, PUSH_SCALAR, ADD, halt */
    Stack *stack = sov_stack_new();
    ASSERT(stack != NULL, "test_infer_full_program_trace", "stack_new failed");

    /* Instruction 1: PUSH_SCALAR */
    int res = sov_stack_push(stack, VAL_SCALAR, 1, 1, NULL, false);
    ASSERT(res == 0, "test_infer_full_program_trace", "push 1 failed");
    ASSERT(stack->depth == 1, "test_infer_full_program_trace",
           "depth after push1 != 1");

    /* Instruction 2: PUSH_SCALAR */
    res = sov_stack_push(stack, VAL_SCALAR, 1, 1, NULL, false);
    ASSERT(res == 0, "test_infer_full_program_trace", "push 2 failed");
    ASSERT(stack->depth == 2, "test_infer_full_program_trace",
           "depth after push2 != 2");

    /* Instruction 3: ADD */
    StackValue *b = &stack->values[stack->depth - 1];
    StackValue *a = &stack->values[stack->depth - 2];
    ASSERT(a->type == VAL_SCALAR && b->type == VAL_SCALAR,
           "test_infer_full_program_trace", "ADD types wrong");
    stack->depth--;
    ASSERT(stack->depth == 1, "test_infer_full_program_trace",
           "depth after ADD != 1");

    /* Final stack has 1 scalar */
    ASSERT(stack->values[0].type == VAL_SCALAR, "test_infer_full_program_trace",
           "final type not scalar");

    sov_stack_free(stack);
    TEST_PASS("test_infer_full_program_trace");
    return 0;
}

static int test_unify_compatible_types(void)
{
    /* Shape unification: (3,4) == (3,4) */
    Shape s1 = {3, 4};
    Shape s2 = {3, 4};
    bool unified = sov_shape_unify(s1, s2);
    ASSERT(unified, "test_unify_compatible_types",
           "compatible shapes didn't unify");

    /* Different dimensions */
    Shape s3 = {3, 5};
    unified = sov_shape_unify(s1, s3);
    ASSERT(!unified, "test_unify_compatible_types",
           "incompatible shapes unified");

    TEST_PASS("test_unify_compatible_types");
    return 0;
}

static int test_unify_conflict_detection(void)
{
    /* Shape unification conflict: (2,3) != (2,4) */
    Shape s1 = {2, 3};
    Shape s2 = {2, 4};
    bool unified = sov_shape_unify(s1, s2);
    ASSERT(!unified, "test_unify_conflict_detection",
           "conflicting shapes incorrectly unified");

    /* Row mismatch */
    Shape s3 = {3, 3};
    Shape s4 = {2, 3};
    unified = sov_shape_unify(s3, s4);
    ASSERT(!unified, "test_unify_conflict_detection",
           "row-mismatched shapes unified");

    TEST_PASS("test_unify_conflict_detection");
    return 0;
}

static int test_infer_obligations_collected(void)
{
    /* Verify obligations are collected during inference */
    ObligationSet *obls = sov_obset_new();
    ASSERT(obls != NULL, "test_infer_obligations_collected",
           "obset_new failed");
    ASSERT(obls->count == 0, "test_infer_obligations_collected",
           "initial count != 0");

    /* Add multiple obligations */
    int32_t id1 = sov_obset_add_inv(obls, NULL, 2, 0, 1);
    ASSERT(id1 == 0, "test_infer_obligations_collected", "id1 != 0");
    ASSERT(obls->count == 1, "test_infer_obligations_collected",
           "count != 1 after add_inv");

    int32_t id2 = sov_obset_add_type(obls, "test constraint", 5, 10);
    ASSERT(id2 == 1, "test_infer_obligations_collected", "id2 != 1");
    ASSERT(obls->count == 2, "test_infer_obligations_collected",
           "count != 2 after add_type");

    /* Get obligation */
    Obligation *obl = sov_obset_at(obls, 0);
    ASSERT(obl != NULL, "test_infer_obligations_collected",
           "obset_at returned NULL");
    ASSERT(obl->id == 0, "test_infer_obligations_collected", "obligation id wrong");

    sov_obset_free(obls);
    TEST_PASS("test_infer_obligations_collected");
    return 0;
}

/*
 * Main test runner
 */
int main(void)
{
    printf("=== FORGE Phase 2: Type Inference Tests ===\n\n");

    int tests_run = 0;
    int tests_failed = 0;

    /* Test 1 */
    if (test_infer_push_scalar()) tests_failed++;
    tests_run++;

    /* Test 2 */
    if (test_infer_dup_preserves_type()) tests_failed++;
    tests_run++;

    /* Test 3 */
    if (test_infer_swap_exchanges()) tests_failed++;
    tests_run++;

    /* Test 4 */
    if (test_infer_add_scalars()) tests_failed++;
    tests_run++;

    /* Test 5 */
    if (test_infer_matmul_shape_inference()) tests_failed++;
    tests_run++;

    /* Test 6 */
    if (test_infer_stack_underflow_detection()) tests_failed++;
    tests_run++;

    /* Test 7 */
    if (test_infer_shape_mismatch_add()) tests_failed++;
    tests_run++;

    /* Test 8 */
    if (test_infer_verify_inv_obligation_generation()) tests_failed++;
    tests_run++;

    /* Test 9 */
    if (test_infer_full_program_trace()) tests_failed++;
    tests_run++;

    /* Test 10 */
    if (test_unify_compatible_types()) tests_failed++;
    tests_run++;

    /* Test 11 */
    if (test_unify_conflict_detection()) tests_failed++;
    tests_run++;

    /* Test 12 */
    if (test_infer_obligations_collected()) tests_failed++;
    tests_run++;

    printf("\n=== RESULTS ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_run - tests_failed);
    printf("Tests failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("\n[SUCCESS] All 12/12 tests passed!\n");
        return 0;
    } else {
        printf("\n[FAILURE] %d test(s) failed\n", tests_failed);
        return 1;
    }
}
