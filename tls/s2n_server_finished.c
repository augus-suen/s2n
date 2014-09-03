/*
 * Copyright 2014 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <stdint.h>

#include "tls/s2n_connection.h"
#include "tls/s2n_tls.h"

#include "stuffer/s2n_stuffer.h"

#include "utils/s2n_safety.h"

int s2n_server_finished_recv(struct s2n_connection *conn, const char **err)
{
    uint8_t *our_version;
    int length = S2N_TLS_FINISHED_LEN;
    our_version = conn->handshake.server_finished;

    if (conn->handshake.rsa_failed == 1) {
        *err = "RSA validation failed";
        return -1;
    }

    if (conn->actual_protocol_version == S2N_SSLv3) {
        length = S2N_SSL_FINISHED_LEN;
    }

    uint8_t *their_version = s2n_stuffer_raw_read(&conn->handshake.io, length, err);
    notnull_check(their_version);

    if (!s2n_constant_time_equals(our_version, their_version, length)) {
        *err = "Invalid finished message received";
        return -1;
    }

    conn->handshake.next_state = HANDSHAKE_OVER;

    return 0;
}

int s2n_server_finished_send(struct s2n_connection *conn, const char **err)
{
    uint8_t *our_version;
    int length = S2N_TLS_FINISHED_LEN;

    our_version = conn->handshake.server_finished;

    if (conn->actual_protocol_version == S2N_SSLv3) {
        length = S2N_SSL_FINISHED_LEN;
    }

    GUARD(s2n_stuffer_write_bytes(&conn->handshake.io, our_version, length, err));

    /* Zero the sequence number */
    GUARD(s2n_zero_sequence_number(conn->pending.server_sequence_number, err));

    /* Update the pending state to active, and point the client at the active state */
    memcpy_check(&conn->active, &conn->pending, sizeof(conn->active));
    conn->client = &conn->active;

    conn->handshake.next_state = HANDSHAKE_OVER;

    return 0;
}