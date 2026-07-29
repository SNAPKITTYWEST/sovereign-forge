/*
 * sov_types.c -- Type System Implementation
 * FORGE Phase 2: Complete Type Inference Engine
 *
 * Implements:
 * - Stack push/pop/peek operations with underflow detection
 * - Forward type inference engine with per-instruction type rules
 * - Shape unification for matrix/vector operations
 * - Obligation generation on type constraints
 */

#include "sov_types.h"
#include "src/obligations/sov_obligations.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * ============================================================================
 * TYPE ENVIRONMENT MANAGEMENT
 * ============================================================================
 */

TypeEnv *sov_tyenv_new(void)
{
    TypeEnv *env = (TypeEnv *)malloc(sizeof(TypeEnv));
    if (env) {
        memset(env, 0, sizeof(TypeEnv));
        env->num_vars = 0;
        env->var_names = (char **)malloc(64 * sizeof(char *));
        env->var_types = (ValType *)malloc(64 * sizeof(ValType));
        env->var_shapes = (Shape *)malloc(64 * sizeof(Shape));
    }
    return env;
}

void sov_tyenv_free(TypeEnv *env)
{
    if (env) {
        if (env->var_names) {
            for (uint32_t i = 0; i < env->num_vars; i++) {
                free(env->var_names[i]);
            }
            free(env->var_names);
        }
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
    if (!env || !name) return -1;

    if (env->num_vars >= 64) return -1; /* Capacity exceeded */

    size_t name_len = strlen(name) + 1;
    env->var_names[env->num_vars] = (char *)malloc(name_len);
    if (!env->var_names[env->num_vars]) return -1;

    strcpy(env->var_names[env->num_vars], name);
    env->var_types[env->num_vars] = type;
    env->var_shapes[env->num_vars].rows = rows;
    env->var_shapes[env->num_vars].cols = cols;
    env->num_vars++;

    return 0;
}

/*
 * ============================================================================
 * STACK OPERATIONS
 * ============================================================================
 */

Stack *sov_stack_new(void)
{
    Stack *stack = (Stack *)malloc(sizeof(Stack));
    if (stack) {
        memset(stack, 0, sizeof(Stack));
        stack->values = (StackValue *)malloc(256 * sizeof(StackValue));
        if (!stack->values) {
            free(stack);
            return NULL;
        }
    }
    return stack;
}

void sov_stack_free(Stack *stack)
{
    if (stack) {
        if (stack->values) {
            for (size_t i = 0; i < stack->depth; i++) {
                if (stack->values[i].is_owned && stack->values[i].data) {
                    free(stack->values[i].data);
                }
            }
            free(stack->values);
        }
        free(stack);
    }
}

/* Push: Add value to top of stack */
int sov_stack_push(Stack *stack,
                   ValType type,
                   size_t rows,
                   size_t cols,
                   void *data,
                   bool is_owned)
{
    if (!stack) return -1;
    if (stack->depth >= 256) return -1; /* Stack overflow */

    StackValue *val = &stack->values[stack->depth];
    val->type = type;
    val->shape.rows = rows;
    val->shape.cols = cols;
    val->data = data;
    val->is_owned = is_owned;
    stack->depth++;

    return 0;
}

/* Pop: Remove and return top value (caller owns the returned StackValue) */
StackValue *sov_stack_pop(Stack *stack)
{
    if (!stack || stack->depth == 0) return NULL;

    stack->depth--;
    StackValue *val = (StackValue *)malloc(sizeof(StackValue));
    if (!val) return NULL;

    memcpy(val, &stack->values[stack->depth], sizeof(StackValue));
    return val;
}

/* Peek: Return top value without removing (borrowed reference) */
StackValue *sov_stack_peek(Stack *stack)
{
    if (!stack || stack->depth == 0) return NULL;
    return &stack->values[stack->depth - 1];
}

/*
 * ============================================================================
 * SHAPE UNIFICATION
 * ============================================================================
 */

bool sov_shape_unify(Shape s1, Shape s2)
{
    return (s1.rows == s2.rows && s1.cols == s2.cols);
}

/*
 * ============================================================================
 * INSTRUCTION DECODING AND TYPE INFERENCE
 * ============================================================================
 */

/* Instruction opcodes for stack machine */
typedef enum {
    OP_PUSH_SCALAR = 0x01,      /* PUSH_SCALAR(value: i64) */
    OP_PUSH_VECTOR = 0x02,      /* PUSH_VECTOR(n: u32, data: [i64; n]) */
    OP_PUSH_MATRIX = 0x03,      /* PUSH_MATRIX(m: u32, n: u32, data: [i64; m*n]) */
    OP_DUP = 0x04,              /* DUP: γ ⊢ τ → γ,τ */
    OP_SWAP = 0x05,             /* SWAP: γ,τ₁,τ₂ → γ,τ₂,τ₁ */
    OP_POP = 0x06,              /* POP: γ,τ → γ */
    OP_ADD = 0x07,              /* ADD: scalars or vectors */
    OP_SUB = 0x08,              /* SUB */
    OP_MATMUL = 0x09,           /* MATMUL: (m×n)*(n×p) → m×p */
    OP_VERIFY_INV = 0x0A,       /* VERIFY_INV: Obligation generation */
    OP_VERIFY_SOL = 0x0B,       /* VERIFY_SOL */
    OP_VERIFY_LSTSQ = 0x0C,     /* VERIFY_LSTSQ */
    OP_HALT = 0xFF,             /* End of program */
} OpCode;

/* Infer type effect for single instruction */
typedef struct {
    bool error;
    char error_msg[256];
    Stack *output_stack;
    ObligationSet *obligations;
} InferStep;

static InferStep infer_instruction(Stack *stack,
                                   const uint8_t *program,
                                   size_t pc,
                                   size_t *out_pc,
                                   TypeEnv *env __attribute__((unused)),
                                   ObligationSet *obls)
{
    InferStep result = {0};
    result.output_stack = NULL;
    result.obligations = obls;

    if (pc >= 1000000) { /* Sanity check */
        result.error = true;
        snprintf(result.error_msg, sizeof(result.error_msg), "PC overflow");
        return result;
    }

    uint8_t op = program[pc];
    *out_pc = pc + 1;

    switch (op) {
    case OP_PUSH_SCALAR: {
        /* Read i64 value (simplified: skip for type inference) */
        *out_pc = pc + 9; /* opcode + 8-byte value */
        if (sov_stack_push(stack, VAL_SCALAR, 1, 1, NULL, false) != 0) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "PUSH_SCALAR: stack overflow");
        }
        break;
    }

    case OP_PUSH_VECTOR: {
        /* Read u32 length (simplified) */
        if (pc + 5 > 1000000) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "PUSH_VECTOR: malformed opcode");
            return result;
        }
        uint32_t n = *(uint32_t *)(program + pc + 1);
        *out_pc = pc + 5 + (n * 8);
        if (sov_stack_push(stack, VAL_VECTOR, 1, n, NULL, false) != 0) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "PUSH_VECTOR: stack overflow");
        }
        break;
    }

    case OP_PUSH_MATRIX: {
        /* Read m, n dimensions */
        if (pc + 9 > 1000000) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "PUSH_MATRIX: malformed opcode");
            return result;
        }
        uint32_t m = *(uint32_t *)(program + pc + 1);
        uint32_t n = *(uint32_t *)(program + pc + 5);
        *out_pc = pc + 9 + (m * n * 8);
        if (sov_stack_push(stack, VAL_MATRIX, m, n, NULL, false) != 0) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "PUSH_MATRIX: stack overflow");
        }
        break;
    }

    case OP_DUP: {
        /* γ,τ → γ,τ,τ */
        StackValue *top = sov_stack_peek(stack);
        if (!top) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "DUP: stack underflow");
            return result;
        }
        if (sov_stack_push(stack, top->type, top->shape.rows, top->shape.cols,
                          NULL, false) != 0) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "DUP: stack overflow");
        }
        break;
    }

    case OP_SWAP: {
        /* γ,τ₁,τ₂ → γ,τ₂,τ₁ */
        if (stack->depth < 2) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "SWAP: insufficient stack depth (need 2, have %llu)",
                     (unsigned long long)stack->depth);
            return result;
        }
        StackValue temp = stack->values[stack->depth - 1];
        stack->values[stack->depth - 1] = stack->values[stack->depth - 2];
        stack->values[stack->depth - 2] = temp;
        break;
    }

    case OP_POP: {
        /* γ,τ → γ */
        if (stack->depth == 0) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "POP: stack underflow");
            return result;
        }
        stack->depth--;
        break;
    }

    case OP_ADD: {
        /* ADD: (Scalar, Scalar) → Scalar | (Vec n, Vec n) → Vec n */
        if (stack->depth < 2) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "ADD: insufficient stack depth");
            return result;
        }

        StackValue *b = &stack->values[stack->depth - 1];
        StackValue *a = &stack->values[stack->depth - 2];

        if (a->type == VAL_SCALAR && b->type == VAL_SCALAR) {
            stack->depth--;
            /* Result is scalar, keep it on stack */
        } else if (a->type == VAL_VECTOR && b->type == VAL_VECTOR) {
            if (!sov_shape_unify(a->shape, b->shape)) {
                result.error = true;
                snprintf(result.error_msg, sizeof(result.error_msg),
                         "ADD: vector shape mismatch [%llu] vs [%llu]",
                         (unsigned long long)a->shape.cols, (unsigned long long)b->shape.cols);
                return result;
            }
            stack->depth--;
            /* Result vector remains on stack */
        } else {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "ADD: type mismatch (need compatible scalars or vectors)");
            return result;
        }
        break;
    }

    case OP_SUB: {
        /* SUB: same constraints as ADD */
        if (stack->depth < 2) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "SUB: insufficient stack depth");
            return result;
        }

        StackValue *b = &stack->values[stack->depth - 1];
        StackValue *a = &stack->values[stack->depth - 2];

        if (a->type == VAL_SCALAR && b->type == VAL_SCALAR) {
            stack->depth--;
        } else if (a->type == VAL_VECTOR && b->type == VAL_VECTOR) {
            if (!sov_shape_unify(a->shape, b->shape)) {
                result.error = true;
                snprintf(result.error_msg, sizeof(result.error_msg),
                         "SUB: vector shape mismatch");
                return result;
            }
            stack->depth--;
        } else {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "SUB: type mismatch");
            return result;
        }
        break;
    }

    case OP_MATMUL: {
        /* MATMUL: (m×n) * (n×p) → (m×p) */
        if (stack->depth < 2) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "MATMUL: insufficient stack depth");
            return result;
        }

        StackValue *B = &stack->values[stack->depth - 1];
        StackValue *A = &stack->values[stack->depth - 2];

        if (A->type != VAL_MATRIX || B->type != VAL_MATRIX) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "MATMUL: both operands must be matrices");
            return result;
        }

        /* A is m×n, B is n×p: check inner dimensions match */
        if (A->shape.cols != B->shape.rows) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "MATMUL: inner dimension mismatch (%llu != %llu)",
                     (unsigned long long)A->shape.cols, (unsigned long long)B->shape.rows);
            return result;
        }

        /* Result is m×p, replace stack top with result type */
        stack->values[stack->depth - 2].shape.cols = B->shape.cols;
        stack->depth--;
        break;
    }

    case OP_VERIFY_INV: {
        /* VERIFY_INV: pop n×n matrix A, generate OBL_KIND_INV */
        if (stack->depth < 1) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "VERIFY_INV: stack underflow");
            return result;
        }

        StackValue *A = &stack->values[stack->depth - 1];
        if (A->type != VAL_MATRIX || A->shape.rows != A->shape.cols) {
            result.error = true;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "VERIFY_INV: requires square matrix");
            return result;
        }

        if (obls) {
            int32_t obl_id = sov_obset_add_inv(obls, NULL, A->shape.rows,
                                               (uint32_t)pc, (uint32_t)(*out_pc));
            if (obl_id < 0) {
                result.error = true;
                snprintf(result.error_msg, sizeof(result.error_msg),
                         "VERIFY_INV: obligation generation failed");
                return result;
            }
        }

        stack->depth--;
        break;
    }

    case OP_HALT: {
        *out_pc = pc + 1;
        break;
    }

    default: {
        result.error = true;
        snprintf(result.error_msg, sizeof(result.error_msg),
                 "Unknown opcode: 0x%02x at pc=%llu", op, (unsigned long long)pc);
        return result;
    }
    }

    result.output_stack = stack;
    return result;
}

/*
 * ============================================================================
 * PROGRAM INFERENCE ENGINE
 * ============================================================================
 */

InferResult *sov_infer_program(const uint8_t *program_bytes,
                               size_t program_len,
                               Stack *initial_stack,
                               TypeEnv *env)
{
    InferResult *result = (InferResult *)malloc(sizeof(InferResult));
    if (!result) return NULL;

    memset(result, 0, sizeof(InferResult));

    if (!program_bytes || program_len == 0 || !initial_stack) {
        result->error_msg = (char *)malloc(256);
        if (result->error_msg) {
            strcpy(result->error_msg, "Invalid program or stack");
        }
        return result;
    }

    /* Create working stack copy */
    Stack *work_stack = sov_stack_new();
    if (!work_stack) {
        result->error_msg = (char *)malloc(256);
        if (result->error_msg) {
            strcpy(result->error_msg, "Stack allocation failed");
        }
        return result;
    }

    /* Copy initial stack */
    for (size_t i = 0; i < initial_stack->depth; i++) {
        if (sov_stack_push(work_stack,
                          initial_stack->values[i].type,
                          initial_stack->values[i].shape.rows,
                          initial_stack->values[i].shape.cols,
                          NULL, false) != 0) {
            result->error_msg = (char *)malloc(256);
            if (result->error_msg) {
                strcpy(result->error_msg, "Stack copy failed");
            }
            sov_stack_free(work_stack);
            return result;
        }
    }

    /* Create obligation set */
    ObligationSet *obls = sov_obset_new();
    if (!obls) {
        result->error_msg = (char *)malloc(256);
        if (result->error_msg) {
            strcpy(result->error_msg, "Obligation set allocation failed");
        }
        sov_stack_free(work_stack);
        return result;
    }

    /* Execute type inference */
    size_t pc = 0;
    while (pc < program_len && program_bytes[pc] != OP_HALT) {
        size_t next_pc = pc;
        InferStep step = infer_instruction(work_stack, program_bytes, pc,
                                           &next_pc, env, obls);

        if (step.error) {
            result->error_msg = (char *)malloc(512);
            if (result->error_msg) {
                snprintf(result->error_msg, 512, "Type inference failed at PC %llu: %s",
                         (unsigned long long)pc, step.error_msg);
            }
            sov_obset_free(obls);
            sov_stack_free(work_stack);
            return result;
        }

        pc = next_pc;
        if (pc > program_len) {
            result->error_msg = (char *)malloc(256);
            if (result->error_msg) {
                strcpy(result->error_msg, "Instruction stream overflow");
            }
            sov_obset_free(obls);
            sov_stack_free(work_stack);
            return result;
        }
    }

    /* Success: capture final stack and obligations */
    result->final_stack = work_stack;
    result->num_obligations = obls->count;
    if (obls->count > 0) {
        result->obligation_ids = (uint32_t *)malloc(obls->count * sizeof(uint32_t));
        if (result->obligation_ids) {
            for (size_t i = 0; i < obls->count; i++) {
                result->obligation_ids[i] = obls->items[i].id;
            }
        }
    }

    sov_obset_free(obls);
    return result;
}

void sov_infer_free(InferResult *result)
{
    if (result) {
        if (result->final_stack) {
            sov_stack_free(result->final_stack);
        }
        if (result->obligation_ids) {
            free(result->obligation_ids);
        }
        if (result->error_msg) {
            free(result->error_msg);
        }
        free(result);
    }
}

void sov_type_print(ValType t, Shape s)
{
    switch (t) {
    case VAL_SCALAR:
        printf("Scalar");
        break;
    case VAL_VECTOR:
        printf("Vec[%llu]", (unsigned long long)s.cols);
        break;
    case VAL_MATRIX:
        printf("Mat(%llu x %llu)", (unsigned long long)s.rows, (unsigned long long)s.cols);
        break;
    case VAL_PROOF:
        printf("Proof");
        break;
    default:
        printf("Unknown");
    }
}
