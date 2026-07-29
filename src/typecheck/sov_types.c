/*
 * sov_types.c -- Type System Implementation
 * FORGE Phase 2 (stub for compilation)
 */

#include "sov_types.h"
#include <stdlib.h>
#include <string.h>

TypeEnv *sov_tyenv_new(void)
{
    TypeEnv *env = (TypeEnv *)malloc(sizeof(TypeEnv));
    if (env) {
        memset(env, 0, sizeof(TypeEnv));
    }
    return env;
}

void sov_tyenv_free(TypeEnv *env)
{
    if (env) {
        free(env->var_names);
        free(env->var_types);
        free(env->var_shapes);
        free(env);
    }
}

int sov_tyenv_add_var(TypeEnv *env,
                      const char *name,
                      ValType type,
                      size_t rows,
                      size_t cols)
{
    if (!env) return -1;
    return 0;
}

Stack *sov_stack_new(void)
{
    Stack *stack = (Stack *)malloc(sizeof(Stack));
    if (stack) {
        memset(stack, 0, sizeof(Stack));
        stack->values = (StackValue *)malloc(64 * sizeof(StackValue));
    }
    return stack;
}

void sov_stack_free(Stack *stack)
{
    if (stack) {
        free(stack->values);
        free(stack);
    }
}

int sov_stack_push(Stack *stack,
                   ValType type,
                   size_t rows,
                   size_t cols,
                   void *data,
                   bool is_owned)
{
    if (!stack) return -1;
    return 0;
}

StackValue *sov_stack_pop(Stack *stack)
{
    if (!stack || stack->depth == 0) return NULL;
    return NULL;
}

StackValue *sov_stack_peek(Stack *stack)
{
    if (!stack || stack->depth == 0) return NULL;
    return NULL;
}

InferResult *sov_infer_program(const uint8_t *program_bytes,
                               size_t program_len,
                               Stack *initial_stack,
                               TypeEnv *env)
{
    return NULL;
}

void sov_infer_free(InferResult *result)
{
    if (result) {
        free(result->obligation_ids);
        free(result->error_msg);
        free(result);
    }
}

bool sov_shape_unify(Shape s1, Shape s2)
{
    return (s1.rows == s2.rows && s1.cols == s2.cols);
}

void sov_type_print(ValType t, Shape s)
{
    /* Stub */
}
