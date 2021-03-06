/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "api/s2n.h"
#include "stuffer/s2n_stuffer.h"

#include <assert.h>
#include <sys/param.h>
#include <cbmc_proof/cbmc_utils.h>
#include <cbmc_proof/make_common_datastructures.h>
#include <cbmc_proof/proof_allocators.h>

void s2n_stuffer_write_harness() {
    /* Non-deterministic inputs. */
    struct s2n_stuffer *stuffer = cbmc_allocate_s2n_stuffer();
    __CPROVER_assume(s2n_stuffer_is_valid(stuffer));
    struct s2n_blob *blob = cbmc_allocate_s2n_blob();
    __CPROVER_assume(s2n_blob_is_valid(blob));
    uint32_t index;

    /* Non-deterministically set initialized (in s2n_mem) to true. */
    if(nondet_bool()) {
        s2n_mem_init();
    }

    /* Save previous state from stuffer. */
    struct s2n_stuffer old_stuffer = *stuffer;

    /* Store a byte from the stuffer that wont be overwritten to compare if the write succeeds. */
    __CPROVER_assume(index < stuffer->blob.size);
    if(__CPROVER_overflow_plus(old_stuffer.write_cursor, blob->size)) {
        __CPROVER_assume(index < old_stuffer.write_cursor);
    } else {
        __CPROVER_assume(index < old_stuffer.write_cursor || index >= old_stuffer.write_cursor + blob->size);
    }
    uint8_t untouched_byte = stuffer->blob.data[index];

    /* Store a byte from the blob to compare. */
    struct s2n_blob old_blob = *blob;
    struct store_byte_from_buffer old_byte_from_blob;
    save_byte_from_blob(blob, &old_byte_from_blob);

    /* Operation under verification. */
    if (s2n_stuffer_write(stuffer, blob) == S2N_SUCCESS) {
        assert(stuffer->write_cursor == old_stuffer.write_cursor + blob->size);
        assert(stuffer->blob.data[index] == untouched_byte);
        assert(stuffer->high_water_mark == MAX(old_stuffer.write_cursor + blob->size, old_stuffer.high_water_mark));
        assert(s2n_stuffer_is_valid(stuffer));
    } else {
        assert(stuffer->write_cursor == old_stuffer.write_cursor);
        assert(stuffer->high_water_mark == old_stuffer.high_water_mark);
    }
    assert(stuffer->read_cursor == old_stuffer.read_cursor);
    assert_blob_equivalence(blob, &old_blob, &old_byte_from_blob);
}
