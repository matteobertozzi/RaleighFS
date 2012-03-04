/*
 *   Copyright 2011 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <raleighfs/object/memcache.h>

#include <zcl/streamslice.h>
#include <zcl/byteslice.h>
#include <zcl/messageq.h>
#include <zcl/snprintf.h>
#include <zcl/iopoll.h>
#include <zcl/memcmp.h>
#include <zcl/memcpy.h>
#include <zcl/strtol.h>
#include <zcl/chunkq.h>
#include <zcl/socket.h>
#include <zcl/debug.h>
#include <zcl/rpc.h>

#define __tokcmp(client, token, mem, memlen)                                  \
    z_chunkq_memcmp(&((client)->rdbuffer), (token)->offset, mem, memlen)

#define __command_cmp(client, token, name, len)                               \
    z_chunkq_compare(&((client)->rdbuffer), (token)->offset, name, len,       \
                     (z_chunkq_compare_t)z_strncasecmp)

#define MAX_TOKENS                              25
#define COMMAND_TOKEN                           0

#define REDIS_REQ_MULTIBULK                     1
#define REDIS_REQ_INLINE                        1

typedef int (*redis_func_t)  (z_rpc_client_t *client,
                              int method_code,
                              const z_chunkq_extent_t *tokens,
                              unsigned int ntokens,
                              unsigned int nline);

struct redis_command {
    const char *    name;
    unsigned int    length;
    int             code;
    int             rmline;
    redis_func_t    func;
};

#define __redis_message_alloc(client, type)                   \
    z_message_alloc((client)->user_data, type)

static int __redis_message_send (z_rpc_client_t *client,
                                 z_message_t *msg,
                                 z_message_func_t callback)
{
    z_byte_slice_t obj_name;
    z_byte_slice(&obj_name, ":memcache:", 10);
    return(z_message_send(msg, Z_SLICE(&obj_name), callback, client));
}

static void __complete_set (void *user_data, z_message_t *msg) {
    z_rpc_client_t *client = Z_RPC_CLIENT(user_data);

    switch (z_message_state(msg)) {
        case RALEIGHFS_MEMCACHE_STORED:
             z_rpc_write(client, "+OK\r\n", 5);
            break;
        default:
             z_rpc_write(client, "-ERR todo\r\n", 11);
            break;
    }

    z_message_free(msg);
}

static int __process_set (z_rpc_client_t *client,
                          int method_code,
                          const z_chunkq_extent_t *tokens,
                          unsigned int ntokens,
                          unsigned int nline)
{
    const z_chunkq_extent_t *value;
    const z_chunkq_extent_t *key;
    z_message_stream_t stream;
    z_message_t *msg;

    if (ntokens != 3)
        return(-1);

    key = &(tokens[1]);
    value = &(tokens[2]);

    /* Send SET Operation Command! */
    msg = __redis_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(Z_STREAM(&stream), key->length);
    z_stream_write_uint32(Z_STREAM(&stream), value->length);
    z_stream_write_uint32(Z_STREAM(&stream), 0);
    z_stream_write_uint32(Z_STREAM(&stream), 0);
    z_stream_write_uint64(Z_STREAM(&stream), 0);
    z_stream_write_chunkq(Z_STREAM(&stream), key->chunkq, key->offset, key->length);
    z_stream_write_chunkq(Z_STREAM(&stream), value->chunkq, value->offset, value->length);
    return(__redis_message_send(client, msg, __complete_set));
}

static void __complete_get (void *user_data, z_message_t *msg) {
    z_rpc_client_t *client = Z_RPC_CLIENT(user_data);
    z_message_stream_t req_stream;
    z_message_stream_t res_stream;
    z_stream_slice_t value;
    z_stream_slice_t key;
    char text_number[16];
    char buffer[512];
    uint64_t unused64;
    uint32_t unused32;
    uint32_t klength;
    uint32_t vlength;
    uint64_t number;
    uint32_t state;
    int n;

    z_message_response_stream(msg, &res_stream);
    z_message_request_stream(msg, &req_stream);
    z_stream_read_uint32(Z_STREAM(&req_stream), &unused32);

    z_stream_read_uint32(Z_STREAM(&req_stream), &klength);
    z_stream_slice(&key, Z_STREAM(&req_stream), req_stream.offset, klength);

    z_stream_read_uint32(Z_STREAM(&res_stream), &state);
    if (state == RALEIGHFS_MEMCACHE_NOT_FOUND) {
        n = z_snprintf(buffer, sizeof(buffer), "$-1");
        z_rpc_write(client, buffer, n);
    } else {
        z_stream_read_uint32(Z_STREAM(&res_stream), &unused32);
        z_stream_read_uint32(Z_STREAM(&res_stream), &unused32);
        z_stream_read_uint64(Z_STREAM(&res_stream), &unused64);

        if (state == RALEIGHFS_MEMCACHE_NUMBER) {
            z_stream_read_uint64(Z_STREAM(&res_stream), &number);
            n = z_u64tostr(number, text_number, sizeof(text_number), 10);
            vlength = n;
            n = z_snprintf(buffer, sizeof(buffer), "$%u4d\r\n", vlength);
            z_rpc_write(client, buffer, n);
            z_rpc_write(client, text_number, vlength);
        } else {
            z_stream_read_uint32(Z_STREAM(&res_stream), &vlength);
            z_stream_slice(&value, Z_STREAM(&res_stream), res_stream.offset, vlength);
            z_stream_seek(Z_STREAM(&res_stream), res_stream.offset + vlength);

            n = z_snprintf(buffer, sizeof(buffer), "$%u4d\r\n", vlength);
            z_rpc_write(client, buffer, n);
            z_rpc_write_slice(client, Z_SLICE(&value));
        }
    }
    z_rpc_write(client, "\r\n", 2);

    z_message_free(msg);
}

static int __process_get (z_rpc_client_t *client,
                          int method_code,
                          const z_chunkq_extent_t *tokens,
                          unsigned int ntokens,
                          unsigned int nline)
{
    const z_chunkq_extent_t *key;
    z_message_stream_t stream;
    z_message_t *msg;

    if (ntokens != 2)
        return(-1);

    key = &(tokens[1]);

    /* Send GET Operation Command! */
    msg = __redis_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(Z_STREAM(&stream), 1);
    z_stream_write_uint32(Z_STREAM(&stream), key->length);
    z_stream_write_chunkq(Z_STREAM(&stream), key->chunkq, key->offset, key->length);
    return(__redis_message_send(client, msg, __complete_get));
}

static int __process_ping (z_rpc_client_t *client,
                           int method_code,
                           const z_chunkq_extent_t *tokens,
                           unsigned int ntokens,
                           unsigned int nline)
{
    return(z_rpc_write(client, "+PONG\r\n", 7));
}

static int __process_quit (z_rpc_client_t *client,
                           int method_code,
                           const z_chunkq_extent_t *tokens,
                           unsigned int ntokens,
                           unsigned int nline)
{
    z_rpc_write(client, "+OK\r\n", 5);
    return(-1);
}

static struct redis_command __redis_commands[] = {
    { "set",        3, RALEIGHFS_MEMCACHE_SET, 1, __process_set },
    { "get",        3, RALEIGHFS_MEMCACHE_GET, 1, __process_get },

    { "ping",       4, 0,                      1, __process_ping },
    { "quit",       4, 0,                      1, __process_quit },
    { NULL,         0, 0,                      1, NULL },
};

/* ============================================================================
 *  Redis Protocol Methods
 */
static int __redis_bind (const void *hostname, const void *service) {
    int socket;

    if ((socket = z_socket_tcp_bind(hostname, service, NULL)) < 0)
        return(-1);

    Z_PRINT_DEBUG(" - Redis Interface up and running on %s:%s...\n",
                  (const char *)hostname, (const char *)service);

    return(socket);
}

static int __redis_accept (z_rpc_server_t *server) {
    struct sockaddr_storage address;
    char ip[INET6_ADDRSTRLEN];
    int csock;

    if ((csock = z_socket_tcp_accept(Z_IOPOLL_ENTITY_FD(server), NULL)) < 0)
        return(-1);

    z_socket_tcp_set_nodelay(csock);

    z_socket_address(csock, &address);
    z_socket_str_address(ip, INET6_ADDRSTRLEN, (struct sockaddr *)&address);
    Z_PRINT_DEBUG(" - Redis Accept %d %s.\n", csock, ip);

    return(csock);
}

static int __redis_connected (z_rpc_client_t *client) {
    z_message_source_t *source;

    if ((source = z_message_source_alloc((client)->server->user_data)) == NULL)
        return(1);

    client->user_data = source;
    return(0);
}

static void __redis_disconnected (z_rpc_client_t *client) {
    z_message_source_free(client->user_data);
}

static int __redis_process_command (z_rpc_client_t *client,
                                    const z_chunkq_extent_t *tokens,
                                    unsigned int ntokens,
                                    unsigned int request_length)
{
    struct redis_command *p;
    int result;

    for (p = __redis_commands; p->name != NULL; ++p) {
        if (p->length != tokens[COMMAND_TOKEN].length)
            continue;

        if (__command_cmp(client, &(tokens[COMMAND_TOKEN]), p->name, p->length))
            continue;

        z_iopoll_entity_reset_flags(client);
        if ((result = p->func(client, p->code, tokens, ntokens, request_length)))
            return(result);

        if (p->rmline)
            z_chunkq_remove(&(client->rdbuffer), request_length);

        return(0);
    }

    z_chunkq_remove(&(client->rdbuffer), request_length);
    z_rpc_write(client, "+ERROR\r\n", 8);
    return(0);
}

static int __redis_process_multibulk (z_rpc_client_t *client) {
    z_chunkq_extent_t tokens[MAX_TOKENS];
    unsigned int ntokens;
    unsigned int nline;
    uint32_t nargs;
    uint32_t size;
    long index;

    if (!(nline = z_rpc_has_line(client, 0)))
        return(1);

    index = z_chunkq_indexof(&(client->rdbuffer), 0, "*", 1) + 1;
    if (!z_chunkq_u32(&(client->rdbuffer), index, nline - index, 10, &nargs))
        return(2);

    if (nargs > MAX_TOKENS) {
        Z_PRINT_DEBUG("Too much tokens %u\n", nargs);
        return(-1);
    }

    ntokens = 0;
    while (nargs--) {
        nline++;
        index = z_chunkq_indexof(&(client->rdbuffer), nline, "$", 1) + 1;
        if (!(nline = z_rpc_has_line(client, nline)))
            return(3);

        if (!z_chunkq_u32(&(client->rdbuffer), index, nline - index, 10, &size))
            return(4);

        nline++;
        z_chunkq_extent_set(&(tokens[ntokens]), &(client->rdbuffer),
                            nline, size);

        if (!(nline = z_rpc_has_line(client, nline)))
            return(5);

        ntokens++;
    }

    return(__redis_process_command(client, tokens, ntokens, nline + 1));
}

static int __redis_process_inline (z_rpc_client_t *client) {
    z_chunkq_extent_t tokens[MAX_TOKENS];
    unsigned int ntokens;
    unsigned int nline;

    if (!(nline = z_rpc_has_line(client, 0)))
        return(1);

    nline++;
    ntokens = z_rpc_ntokenize(client, tokens, MAX_TOKENS, 0, nline);

    return(__redis_process_command(client, tokens, ntokens, nline));
}

static int __redis_process (z_rpc_client_t *client) {
    z_chunkq_extent_t token;

    if (!z_rpc_tokenize(client, &token, 0))
        return(1);

    if (!__tokcmp(client, &token, "*", 1))
        return(__redis_process_multibulk(client));
    else
        return(__redis_process_inline(client));

    return(1);
}

z_rpc_protocol_t redis_protocol = {
    .bind           = __redis_bind,
    .accept         = __redis_accept,
    .connected      = __redis_connected,
    .disconnected   = __redis_disconnected,
    .process        = __redis_process,
    .process_line   = NULL,
};

