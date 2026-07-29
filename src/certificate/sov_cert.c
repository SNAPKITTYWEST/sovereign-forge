/*
 * sov_cert.c -- Proof Certificate Implementation
 * FORGE Phase 2 (stub for compilation)
 */

#include "sov_cert.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

ProofCertificate *sov_cert_new(void)
{
    ProofCertificate *cert = (ProofCertificate *)malloc(sizeof(ProofCertificate));
    if (cert) {
        memset(cert, 0, sizeof(ProofCertificate));
        cert->version = 1;
        cert->timestamp = (uint64_t)time(NULL);
    }
    return cert;
}

void sov_cert_free(ProofCertificate *cert)
{
    if (cert) {
        free(cert->initial_stack);
        free(cert->final_stack);
        free(cert->type_env);
        free(cert->evidence_chain);
        free(cert->canonical_json);
        if (cert->obligations.obligations) {
            free(cert->obligations.obligations);
        }
        free(cert);
    }
}

void sov_cert_set_program(ProofCertificate *cert,
                          const uint8_t *program_hash,
                          uint32_t program_size)
{
    if (cert && program_hash) {
        memcpy(cert->program_hash, program_hash, 32);
        cert->program_size = program_size;
    }
}

int sov_cert_set_stacks(ProofCertificate *cert,
                        const int64_t *init_stack,
                        size_t init_len,
                        const int64_t *final_stack,
                        size_t final_len)
{
    if (!cert) return -1;
    
    cert->initial_stack = (int64_t *)malloc(init_len * sizeof(int64_t));
    if (!cert->initial_stack) return -1;
    memcpy(cert->initial_stack, init_stack, init_len * sizeof(int64_t));
    cert->initial_stack_len = init_len;
    
    cert->final_stack = (int64_t *)malloc(final_len * sizeof(int64_t));
    if (!cert->final_stack) return -1;
    memcpy(cert->final_stack, final_stack, final_len * sizeof(int64_t));
    cert->final_stack_len = final_len;
    
    return 0;
}

int sov_cert_add_obligation(ProofCertificate *cert,
                            ObligationKind kind,
                            uint32_t start_pc,
                            uint32_t end_pc)
{
    if (!cert) return -1;
    return 0;  /* Stub */
}

int sov_cert_canonicalize(ProofCertificate *cert)
{
    if (!cert) return -1;
    return 0;  /* Stub */
}

int sov_cert_hash(ProofCertificate *cert)
{
    if (!cert) return -1;
    return 0;  /* Stub */
}

int sov_cert_serialize_cbor(ProofCertificate *cert,
                            uint8_t **out_bytes,
                            size_t *out_len)
{
    if (!cert) return -1;
    return 0;  /* Stub */
}

ProofCertificate *sov_cert_deserialize_cbor(const uint8_t *bytes,
                                             size_t len)
{
    return NULL;  /* Stub */
}

WormReceipt *sov_receipt_new(void)
{
    WormReceipt *receipt = (WormReceipt *)malloc(sizeof(WormReceipt));
    if (receipt) {
        memset(receipt, 0, sizeof(WormReceipt));
        receipt->timestamp = (uint64_t)time(NULL);
    }
    return receipt;
}

void sov_receipt_free(WormReceipt *receipt)
{
    if (receipt) {
        free(receipt->failure_reason);
        free(receipt);
    }
}

int sov_receipt_seal(WormReceipt *receipt,
                     const uint8_t *secret_key,
                     ProofCertificate *cert,
                     const uint8_t *program_hash,
                     ReceiptOutcome outcome)
{
    if (!receipt) return -1;
    return 0;  /* Stub */
}

int sov_receipt_verify(const WormReceipt *receipt)
{
    if (!receipt) return -1;
    return 0;  /* Stub */
}

int sov_receipt_to_json(const WormReceipt *receipt,
                        uint8_t **out_json,
                        size_t *out_len)
{
    if (!receipt) return -1;
    return 0;  /* Stub */
}
