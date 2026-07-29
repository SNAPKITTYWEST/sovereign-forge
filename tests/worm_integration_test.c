/*
  WORM Integration Test: Verify results sealed to append-only ledger

  Tests the integration between verification and WORM receipt appending.
  When a verification completes, the result is optionally appended to the
  WORM ledger as an immutable, signed receipt.
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../src/receipts/worm_integration.h"
#include "../src/verifier/sov_verifier.h"

/* Test: Append a verification receipt to WORM ledger */
int test_worm_append_receipt(void)
{
  printf("TEST: worm_append_verification_receipt\n");

  uint8_t writer_id[32], stream_id[32], witness_hash[32], private_key[32];
  memset(writer_id, 0xAA, 32);
  memset(stream_id, 0xBB, 32);
  memset(witness_hash, 0xCC, 32);
  memset(private_key, 0xDD, 32);

  WormError err = worm_append_verification_receipt(
      writer_id,
      stream_id,
      witness_hash,
      VER_OK,
      private_key
  );

  if (err != WORM_OK) {
    printf("FAIL: Expected WORM_OK, got %d\n", err);
    return 1;
  }

  printf("PASS: Receipt appended\n");
  return 0;
}

/* Test: Query ledger sequence after append */
int test_worm_query_sequence(void)
{
  printf("TEST: worm_get_ledger_sequence\n");

  int64_t seq = worm_get_ledger_sequence();
  if (seq < 0) {
    printf("FAIL: Expected sequence >= 0, got %lld\n", seq);
    return 1;
  }

  printf("PASS: Ledger sequence %lld\n", seq);
  return 0;
}

/* Test: Null input handling */
int test_worm_null_input(void)
{
  printf("TEST: worm_append_verification_receipt (null input)\n");

  uint8_t dummy[32];
  memset(dummy, 0, 32);

  WormError err = worm_append_verification_receipt(
      NULL,  /* writer_id */
      dummy,
      dummy,
      VER_OK,
      dummy
  );

  if (err != WORM_ERR_NULL_INPUT) {
    printf("FAIL: Expected WORM_ERR_NULL_INPUT, got %d\n", err);
    return 1;
  }

  printf("PASS: Null input rejected\n");
  return 0;
}

int main(void)
{
  printf("=== WORM Integration Tests ===\n\n");

  int failures = 0;
  failures += test_worm_append_receipt();
  failures += test_worm_query_sequence();
  failures += test_worm_null_input();

  printf("\n=== Summary ===\n");
  printf("Tests: 3\n");
  printf("Passed: %d\n", 3 - failures);
  printf("Failed: %d\n", failures);

  return failures > 0 ? 1 : 0;
}
