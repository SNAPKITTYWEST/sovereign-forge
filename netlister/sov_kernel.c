// sov_kernel.c — Sovereign Stack Machine Kernel
// Bare-metal C ABI (<10KB, zero dependencies, no libleanrt)
// Linked via @[extern] from Lean 4 proof terms
// Compiles to: freestanding binary, WebAssembly, Verilog-AMS RTL

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// ═══ 1. DATA LAYOUT (Matches Lean Vector ℤ n) ═══
// Lean's Vector ℤ n becomes C's int64_t array.
// Vector = { int64_t* data; size_t len; }
typedef struct {
    int64_t* data;
    size_t len;
} SovVector;

// ═══ 2. INSTRUCTION TYPES (Matches Lean Instr) ═══
typedef enum {
    SOV_PUSH = 0,
    SOV_DUP = 1,
    SOV_SWAP = 2,
    SOV_ADD = 3
} SovInstrTag;

typedef struct {
    SovInstrTag tag;
    int64_t imm;  // Immediate for PUSH
} SovInstr;

// ═══ 3. MATRIX KERNELS (Unrolled, Constant, Pure Functions) ═══
// No closures. No heap. Just pointer arithmetic.

// Push: v'[0] = x; v'[i+1] = v[i]  (height n → n+1)
// Precondition: v_out has capacity >= n+1
static inline void sov_push_kernel(int64_t* v_out, const int64_t* v_in, size_t n, int64_t x) {
    v_out[0] = x;
    for (size_t i = 0; i < n; ++i) {
        v_out[i + 1] = v_in[i];
    }
}

// Dup: v'[0] = v[0]; v'[1] = v[0]; v'[i+1] = v[i]  (height n → n+1)
// Precondition: n >= 1 (guaranteed by ValidStack)
static inline void sov_dup_kernel(int64_t* v_out, const int64_t* v_in, size_t n) {
    v_out[0] = v_in[0];  // Duplicate top
    v_out[1] = v_in[0];
    for (size_t i = 1; i < n; ++i) {
        v_out[i + 1] = v_in[i];
    }
}

// Swap: v'[0] = v[1]; v'[1] = v[0]; v'[i] = v[i]  (height n → n, n >= 2)
// Precondition: n >= 2 (guaranteed by ValidStack)
static inline void sov_swap_kernel(int64_t* v, size_t n) {
    (void)n;  // Suppress unused parameter warning
    int64_t tmp = v[0];
    v[0] = v[1];
    v[1] = tmp;
}

// Add: v'[0] = v[0] + v[1]; v'[i] = v[i+1]  (height n → n-1, n >= 2)
// Precondition: n >= 2 (guaranteed by ValidStack)
static inline void sov_add_kernel(int64_t* v_out, const int64_t* v_in, size_t n) {
    v_out[0] = v_in[0] + v_in[1];  // Add top two
    for (size_t i = 1; i < n - 1; ++i) {
        v_out[i] = v_in[i + 1];
    }
}

// ═══ 4. EXECUTION KERNEL (execProg) ═══
// Precondition: prog is ValidStack-proved at compile time.
// Stack grows UP: v[0] = TOP (Lean convention), v[len-1] = BOTTOM.
// This function modifies the vector in-place and updates its length.
//
// Return value: Final height after execution.
size_t sov_exec_prog(const SovInstr* prog, size_t prog_len, SovVector* stack) {
    int64_t* v = stack->data;
    size_t n = stack->len;

    for (size_t pc = 0; pc < prog_len; ++pc) {
        const SovInstr* ins = &prog[pc];

        switch (ins->tag) {
            case SOV_PUSH: {
                // Shift right, insert at position 0
                // v[i] -> v[i+1] for all i in [0, n)
                for (size_t i = n; i > 0; --i) {
                    v[i] = v[i - 1];
                }
                v[0] = ins->imm;
                n++;
                break;
            }

            case SOV_DUP: {
                // Shift right, duplicate top at position 0 and 1
                for (size_t i = n; i > 0; --i) {
                    v[i] = v[i - 1];
                }
                v[0] = v[1];  // After shift, v[1] is the old top
                n++;
                break;
            }

            case SOV_SWAP: {
                // Swap top two elements
                // n >= 2 is guaranteed by ValidStack proof
                int64_t tmp = v[0];
                v[0] = v[1];
                v[1] = tmp;
                break;
            }

            case SOV_ADD: {
                // Pop two, push sum
                // v'[0] = v[0] + v[1]
                // v'[i] = v[i+1] for i in [1, n-2]
                // n >= 2 is guaranteed by ValidStack proof
                v[1] = v[0] + v[1];
                for (size_t i = 0; i < n - 1; ++i) {
                    v[i] = v[i + 1];
                }
                n--;
                break;
            }
        }
    }

    stack->len = n;
    return n;
}

// ═══ 5. VERIFICATION HELPER (Plasma Gate) ═══
// Deterministic verifier matching ValidStack predicate
// Returns final height, or -1 if invalid
int64_t sov_verify_program(const SovInstr* prog, size_t prog_len, int64_t init_height) {
    int64_t h = init_height;

    for (size_t pc = 0; pc < prog_len; ++pc) {
        switch (prog[pc].tag) {
            case SOV_PUSH:
                h += 1;
                break;
            case SOV_DUP:
                if (h < 1) return -1;  // Would underflow
                h += 1;
                break;
            case SOV_SWAP:
                if (h < 2) return -1;  // Would underflow
                break;
            case SOV_ADD:
                if (h < 2) return -1;  // Would underflow
                h -= 1;
                break;
        }
    }

    return h;
}

// ═══ 6. DETERMINISTIC RECEIPTING (WORM Integration) ═══
// Hash bytecode stream for provenance chain
typedef struct {
    uint64_t hash_lo;
    uint64_t hash_hi;
    int64_t final_height;
    uint64_t timestamp;
} SovProgramReceipt;

// Stub hash: Replace with SHA-256 in production
// For now, a simple deterministic hash (FNV-1a 64-bit)
static uint64_t sov_hash_bytecode(const SovInstr* prog, size_t prog_len) {
    uint64_t hash = 0xcbf29ce484222325ULL;  // FNV offset basis
    const uint8_t* bytes = (const uint8_t*)prog;
    size_t total_bytes = prog_len * sizeof(SovInstr);

    for (size_t i = 0; i < total_bytes; ++i) {
        hash ^= bytes[i];
        hash *= 0x100000001b3ULL;  // FNV prime
    }

    return hash;
}

SovProgramReceipt sov_seal_program(
    const SovInstr* prog, size_t prog_len,
    int64_t init_height, uint64_t timestamp) {
    SovProgramReceipt receipt;

    // Verify program
    int64_t final_h = sov_verify_program(prog, prog_len, init_height);
    if (final_h < 0) {
        receipt.final_height = -1;
        receipt.hash_lo = 0;
        receipt.hash_hi = 0;
        receipt.timestamp = 0;
        return receipt;
    }

    // Hash bytecode
    uint64_t h = sov_hash_bytecode(prog, prog_len);
    receipt.hash_lo = h;
    receipt.hash_hi = (h >> 32) ^ (final_h & 0xFFFFFFFF);  // Mix in height

    receipt.final_height = final_h;
    receipt.timestamp = timestamp;

    return receipt;
}

// ═══ 7. EXAMPLE: Test Harness ═══
// To compile as standalone binary:
// gcc -O3 -std=c99 -ffreestanding -nostdlib sov_kernel.c -o kernel.elf
// Or with libc:
// gcc -O3 -std=c99 sov_kernel.c -o kernel.elf

#ifdef SOV_STANDALONE_TEST

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    // Example: [Push 10, Push 20, Dup, Add]
    SovInstr prog[] = {
        {SOV_PUSH, 10},
        {SOV_PUSH, 20},
        {SOV_DUP, 0},
        {SOV_ADD, 0}
    };
    size_t prog_len = 4;

    // Verify
    int64_t final_h = sov_verify_program(prog, prog_len, 0);
    printf("Verify: final_height=%lld\n", final_h);
    if (final_h < 0) {
        printf("INVALID PROGRAM\n");
        return 1;
    }

    // Seal with receipt
    SovProgramReceipt receipt = sov_seal_program(prog, prog_len, 0, 1234567890ULL);
    printf("Receipt: hash_lo=0x%016llx, timestamp=%llu\n", receipt.hash_lo, receipt.timestamp);

    // Execute
    int64_t* stack_mem = (int64_t*)malloc((final_h + 1) * sizeof(int64_t));
    SovVector stack = {stack_mem, 0};

    size_t result_height = sov_exec_prog(prog, prog_len, &stack);
    printf("Execute: result_height=%zu\n", result_height);

    if (result_height > 0) {
        printf("Top-of-stack: %lld\n", stack.data[0]);
    }

    free(stack_mem);
    return 0;
}

#endif  // SOV_STANDALONE_TEST
