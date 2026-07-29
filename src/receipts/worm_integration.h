/*
  WORM Integration: Seal verification results to append-only ledger

  After verification completes, optionally append a WORM receipt to the
  distributed ledger for immutable audit trail. Receipts include the
  verification result, witness commitment, and signature.
*/

#ifndef SOVEREIGN_WORM_INTEGRATION_H
#define SOVEREIGN_WORM_INTEGRATION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  WORM_OK = 0,
  WORM_ERR_NULL_INPUT = 1,
  WORM_ERR_INIT_FAILURE = 2,
  WORM_ERR_APPEND_FAILURE = 3,
  WORM_ERR_SIGNATURE_FAILURE = 4,
  WORM_ERR_RECORD_NOT_CREATED = 5,
} WormError;

/*
  worm_append_verification_receipt

  Append a verification result to the WORM ledger as a signed receipt.

  Parameters:
    writer_id:    Machine identity (32 bytes, binary)
    stream_id:    Verification stream ID (32 bytes, binary)
    witness_hash: SHA-256 of witness (32 bytes)
    result_code:  Verification result (VER_PASS, VER_FAIL, etc.)
    private_key:  Ed25519 private key (32 bytes)

  Returns:
    WORM_OK on success
    WormError code on failure
*/
WormError worm_append_verification_receipt(
    const uint8_t *writer_id,     // machine identity
    const uint8_t *stream_id,     // verification stream
    const uint8_t *witness_hash,  // payload commitment
    int32_t result_code,          // verification result
    const uint8_t *private_key    // Ed25519 key
);

/*
  worm_get_ledger_sequence

  Query the current sequence number in the verification ledger.
  Used to check if a receipt was successfully appended.

  Returns:
    Sequence number (>=0) on success
    -1 on failure
*/
int64_t worm_get_ledger_sequence(void);

#ifdef __cplusplus
}
#endif

#endif // SOVEREIGN_WORM_INTEGRATION_H
