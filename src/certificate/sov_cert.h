/*
 * sov_cert.h -- Sovereign Stack Machine Certificate Structure
 *
 * Implements RFC 8949 CBOR-compatible proof certificates with SHA-256 sealing.
 * NO floating-point tolerances. NO unverifiable claims. All int64_t with overflow detection.
 *
 * FORGE Phase 2: Proof Certificate Generator
 *
 * Build: part of libsov_forge.a
 * License: Apache 2.0 + AGPL 3.0
 */

#ifndef SOV_CERT_H
#define SOV_CERT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ============================================================================
 * CERTIFICATE STRUCTURES
 * ============================================================================
 */

/* Obligation kinds from verification policy */
typedef enum {
    OB_INV_OK,      /* Invariant verification required: A*X = I */
    OB_SOLVE_OK,    /* Linear system solution required: A*x = b */
    OB_LSTSQ_OK,    /* Least squares solution required: A^T(Ax-b) = 0 */
    OB_TYPE_OK,     /* Type inference obligation */
    OB_PROP_OK,     /* Property holds at location */
} ObligationKind;

/* Single obligation with witness slot */
typedef struct {
    uint32_t id;                /* Obligation ID (0, 1, 2, ...) */
    ObligationKind kind;        /* Type of obligation */
    uint32_t start_pc;          /* Start program counter */
    uint32_t end_pc;            /* End program counter */

    /* Obligation-specific parameters */
    union {
        struct {
            int64_t *matrix_a;  /* n x n matrix for invariant */
            size_t n;           /* Matrix dimension */
        } inv_params;
        struct {
            int64_t *matrix_a;  /* m x n matrix for solve */
            int64_t *vector_b;  /* Right-hand side vector */
            size_t m, n;        /* Dimensions */
        } solve_params;
        struct {
            int64_t *matrix_a;  /* m x n for least squares */
            int64_t *vector_b;  /* Residual vector */
            size_t m, n;        /* Dimensions */
        } lstsq_params;
    } params;

    /* Witness (filled by verifier) */
    int64_t *witness;           /* Witness data: solution, inverse, etc. */
    size_t witness_len;         /* Witness length in elements */
    bool witness_filled;        /* True if verifier filled this */
} Obligation;

/* Obligation set for a program */
typedef struct {
    Obligation *obligations;    /* Array of obligations */
    size_t count;               /* Number of obligations */
} ObligationSet;

/* Proof certificate structure (RFC 8949 CBOR-compatible) */
typedef struct {
    uint32_t version;           /* Schema version (1) */
    uint64_t timestamp;         /* UNIX timestamp when generated */

    /* Program identification */
    uint8_t program_hash[32];   /* SHA-256 of program bytecode */
    uint32_t program_size;      /* Size in bytes */

    /* Stack machine state */
    int64_t *initial_stack;     /* Initial stack values */
    size_t initial_stack_len;

    int64_t *final_stack;       /* Final stack values */
    size_t final_stack_len;

    /* Type environment */
    uint8_t *type_env;          /* Serialized type environment */
    size_t type_env_len;

    /* Obligations */
    ObligationSet obligations;

    /* Evidence chain */
    uint8_t *evidence_chain;    /* CBOR-serialized evidence */
    size_t evidence_chain_len;

    /* Canonical JSON representation (for hashing) */
    uint8_t *canonical_json;    /* Canonical JSON bytes */
    size_t canonical_json_len;

    /* Certificate hash (SHA-256 of canonical_json) */
    uint8_t cert_hash[32];      /* SHA-256 digest */
} ProofCertificate;

/*
 * ============================================================================
 * WORM RECEIPT STRUCTURES
 * ============================================================================
 */

typedef enum {
    RECEIPT_SUCCESS,            /* Verification succeeded */
    RECEIPT_FAILURE,            /* Verification failed */
    RECEIPT_TIMEOUT,            /* Verification timed out */
    RECEIPT_INVALID_INPUT,      /* Malformed input */
} ReceiptOutcome;

typedef struct {
    uint64_t timestamp;         /* UNIX timestamp */
    uint8_t certificate_hash[32];  /* SHA-256 of certificate */
    uint8_t program_hash[32];   /* SHA-256 of program */
    uint8_t machine_id[32];     /* Machine identity (Blake3) */

    ReceiptOutcome outcome;     /* Success/failure */

    /* Ed25519 signature (64 bytes) */
    uint8_t signature[64];      /* Signature of (cert_hash || program_hash || machine_id || outcome) */
    uint8_t pubkey[32];         /* Ed25519 public key */

    /* Optional: reason for failure */
    char *failure_reason;       /* Human-readable failure reason */
    size_t failure_reason_len;
} WormReceipt;

/*
 * ============================================================================
 * CERTIFICATE API
 * ============================================================================
 */

/* Create a new empty certificate */
ProofCertificate *sov_cert_new(void);

/* Free a certificate and all its data */
void sov_cert_free(ProofCertificate *cert);

/* Set program identification */
void sov_cert_set_program(ProofCertificate *cert,
                          const uint8_t *program_hash,
                          uint32_t program_size);

/* Set initial/final stacks */
int sov_cert_set_stacks(ProofCertificate *cert,
                        const int64_t *init_stack,
                        size_t init_len,
                        const int64_t *final_stack,
                        size_t final_len);

/* Add an obligation to the certificate */
int sov_cert_add_obligation(ProofCertificate *cert,
                            ObligationKind kind,
                            uint32_t start_pc,
                            uint32_t end_pc);

/* Compute canonical JSON representation */
int sov_cert_canonicalize(ProofCertificate *cert);

/* Compute SHA-256 certificate hash */
int sov_cert_hash(ProofCertificate *cert);

/* Serialize to CBOR bytes */
int sov_cert_serialize_cbor(ProofCertificate *cert,
                            uint8_t **out_bytes,
                            size_t *out_len);

/* Deserialize from CBOR bytes */
ProofCertificate *sov_cert_deserialize_cbor(const uint8_t *bytes,
                                             size_t len);

/*
 * ============================================================================
 * WORM RECEIPT API
 * ============================================================================
 */

/* Create a new WORM receipt */
WormReceipt *sov_receipt_new(void);

/* Free a receipt */
void sov_receipt_free(WormReceipt *receipt);

/* Sign and seal a receipt with Ed25519 */
int sov_receipt_seal(WormReceipt *receipt,
                     const uint8_t *secret_key,  /* 32-byte Ed25519 secret */
                     ProofCertificate *cert,
                     const uint8_t *program_hash,
                     ReceiptOutcome outcome);

/* Verify a receipt signature */
int sov_receipt_verify(const WormReceipt *receipt);

/* Serialize receipt to JSON */
int sov_receipt_to_json(const WormReceipt *receipt,
                        uint8_t **out_json,
                        size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* SOV_CERT_H */
