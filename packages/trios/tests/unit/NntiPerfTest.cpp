/**
//@HEADER
// ************************************************************************
//
//                   Trios: Trilinos I/O Support
//                 Copyright 2011 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//Questions? Contact Ron A. Oldfield (raoldfi@sandia.gov)
//
// *************************************************************************
//@HEADER
 */
/*
 * NntiPerfTest.c
 *
 *  Created on: May 16, 2014
 *      Author: thkorde
 */

#include "Trios_nnti.h"
#include "Trios_logger.h"
#include "Trios_timer.h"
#include "Trios_nnti_fprint_types.h"

#include "Teuchos_CommandLineProcessor.hpp"
#include "Teuchos_StandardCatchMacros.hpp"
#include "Teuchos_oblackholestream.hpp"

#include <unistd.h>

#include <mpi.h>


int nprocs, nclients, nservers;
int rank, client_rank;


NNTI_transport_t trans_hdl;
NNTI_peer_t      server_hdl;
char             url[NNTI_URL_LEN];

NNTI_buffer_t       queue_mr;
NNTI_buffer_t       client_ack_mr;
NNTI_buffer_t       server_ack_mr;
NNTI_buffer_t       send_mr;

NNTI_buffer_t       get_src_mr;
NNTI_buffer_t       get_dst_mr;

NNTI_buffer_t       put_src_mr;
NNTI_buffer_t       put_dst_mr;

#define WR_COUNT 2
NNTI_result_t        err;

int one_kb=1024;
int one_mb=1024*1024;
int four_mb=4*1024*1024;

log_level nntiperf_debug_level = LOG_UNDEFINED;

#define MAX_SENDS_PER_CLIENT 10
#define MAX_GETS_PER_CLIENT 10
#define MAX_PUTS_PER_CLIENT 10

//#define MAX_RDMA_XFER_SIZE 4194304

static int buffer_pack(void *input, void **output, int32_t *output_size, xdrproc_t pack_func)
{
	XDR pack_xdrs;

	*output_size = xdr_sizeof(pack_func, input);
	*output=malloc(*output_size);
    xdrmem_create(&pack_xdrs, (caddr_t)*output, *output_size, XDR_ENCODE);
    pack_func(&pack_xdrs, input);

    return(0);
}

static int buffer_pack_free(void *input, int32_t input_size, xdrproc_t free_func)
{
	XDR free_xdrs;

    xdrmem_create(&free_xdrs, (caddr_t)input, input_size, XDR_FREE);
    free_func(&free_xdrs, input);

    return(0);
}

static int buffer_unpack(void *input, int32_t input_size, void *output, xdrproc_t unpack_func)
{
	XDR unpack_xdrs;

    xdrmem_create(&unpack_xdrs, (caddr_t)input, input_size, XDR_DECODE);
    unpack_func(&unpack_xdrs, output);

    return(0);
}

void client(void) {
    NNTI_result_t rc=NNTI_OK;
    NNTI_status_t rdma_status;
    NNTI_status_t send_status;
    NNTI_status_t client_ack_status;
    char *c_ptr;
    void    *packed=NULL;
    int32_t  packed_size=0;

    double op_timer;

    Teuchos::oblackholestream blackhole;
    std::ostream &out = ( rank == 1 ? std::cout : blackhole );

    NNTI_connect(&trans_hdl, url, 5000, &server_hdl);

    char *send_buf=(char *)malloc(NNTI_REQUEST_BUFFER_SIZE);
    memset(send_buf, 0, NNTI_REQUEST_BUFFER_SIZE);
    NNTI_register_memory(&trans_hdl, send_buf, NNTI_REQUEST_BUFFER_SIZE, 1, NNTI_SEND_SRC, NULL, &send_mr);

    char *client_ack_buf=(char *)malloc(NNTI_REQUEST_BUFFER_SIZE);
    memset(client_ack_buf, 0, NNTI_REQUEST_BUFFER_SIZE);
    NNTI_register_memory(&trans_hdl, client_ack_buf, NNTI_REQUEST_BUFFER_SIZE, 1, NNTI_RECV_DST, NULL, &client_ack_mr);

    char *get_dst_buf=(char *)malloc(four_mb);
    memset(get_dst_buf, 0, four_mb);
    NNTI_register_memory(&trans_hdl, get_dst_buf, four_mb, 1, NNTI_GET_DST, NULL, &get_dst_mr);

    char *put_src_buf=(char *)malloc(four_mb);
    memset(put_src_buf, 0, four_mb);
    NNTI_register_memory(&trans_hdl, put_src_buf, four_mb, 1, NNTI_PUT_SRC, NULL, &put_src_mr);

    /*
     * Phase 1 - exchange buffer handles
     */
    buffer_pack(&client_ack_mr, &packed, &packed_size, (xdrproc_t)&xdr_NNTI_buffer_t);
    if (packed_size > NNTI_REQUEST_BUFFER_SIZE) {
        log_error(nntiperf_debug_level, "buffer_pack() says encoded NNTI_buffer_t is larger than NNTI_REQUEST_BUFFER_SIZE");
    	MPI_Abort(MPI_COMM_WORLD, -10);
    }

    // send the server the recv_mr so it can send back it's ack_mr
    memcpy(send_buf, packed, packed_size);

    buffer_pack_free(packed, packed_size, (xdrproc_t)&xdr_NNTI_buffer_t);

    rc=NNTI_send(&server_hdl, &send_mr, NULL);
    if (rc != NNTI_OK) {
        log_error(nntiperf_debug_level, "NNTI_send() returned an error: %d", rc);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    rc=NNTI_wait(&send_mr, NNTI_SEND_SRC, 5000, &send_status);
    if (rc != NNTI_OK) {
        log_error(nntiperf_debug_level, "NNTI_wait() returned an error: %d", rc);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    // wait for the server to send back it's recv_mr
    rc=NNTI_wait(&client_ack_mr, NNTI_RECV_DST, -1, &client_ack_status);

    char *ptr=(char*)client_ack_status.start+client_ack_status.offset;

    memcpy(&packed_size, ptr, sizeof(packed_size));
    ptr += sizeof(packed_size);
    memcpy(packed, ptr, packed_size);
    ptr += packed_size;

    buffer_unpack(packed, packed_size, &server_ack_mr, (xdrproc_t)&xdr_NNTI_buffer_t);

    memcpy(&packed_size, ptr, sizeof(packed_size));
    ptr += sizeof(packed_size);
    memcpy(packed, ptr, packed_size);
    ptr += packed_size;

    buffer_unpack(packed, packed_size, &get_src_mr, (xdrproc_t)&xdr_NNTI_buffer_t);

    memcpy(&packed_size, ptr, sizeof(packed_size));
    ptr += sizeof(packed_size);
    memcpy(packed, ptr, packed_size);
    ptr += packed_size;

    buffer_unpack(packed, packed_size, &put_dst_mr, (xdrproc_t)&xdr_NNTI_buffer_t);

//    fprint_NNTI_buffer(logger_get_file(), "server_ack_mr",
//            "received server ack hdl", &server_ack_mr);
//    fprint_NNTI_buffer(logger_get_file(), "get_src_mr",
//            "received get src hdl", &get_src_mr);
//    fprint_NNTI_buffer(logger_get_file(), "put_dst_mr",
//            "received put dst hdl", &put_dst_mr);

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 2 - test sync request performance
     */
    op_timer=trios_get_time();
    for (int i=0;i<MAX_SENDS_PER_CLIENT;i++) {
        rc=NNTI_send(&server_hdl, &send_mr, NULL);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_send() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        rc=NNTI_wait(&send_mr, NNTI_SEND_SRC, 1000, &send_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << " sync requests per second == " << MAX_SENDS_PER_CLIENT/op_timer << std::endl;

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 3 - test async request performance
     */
    op_timer=trios_get_time();
    for (int i=0;i<MAX_SENDS_PER_CLIENT;i++) {
        rc=NNTI_send(&server_hdl, &send_mr, NULL);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_send() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    for (int i=0;i<MAX_SENDS_PER_CLIENT;i++) {
        rc=NNTI_wait(&send_mr, NNTI_SEND_SRC, 1000, &send_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << "async requests per second == " << MAX_SENDS_PER_CLIENT/op_timer << std::endl;

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 4 - test sync get performance
     */
    // warm up the pipes
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_get(&get_src_mr, client_rank*four_mb, four_mb, &get_dst_mr, 0);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_get() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        rc=NNTI_wait(&get_dst_mr, NNTI_GET_DST, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }

    op_timer=trios_get_time();
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_get(&get_src_mr, client_rank*four_mb, one_kb, &get_dst_mr, 0);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_get() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        rc=NNTI_wait(&get_dst_mr, NNTI_GET_DST, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << " sync get (1 KB transfer) == " << (double)(MAX_GETS_PER_CLIENT*one_kb)/one_mb/op_timer << " MBps" << std::endl;

    op_timer=trios_get_time();
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_get(&get_src_mr, client_rank*four_mb, one_mb, &get_dst_mr, 0);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_get() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        rc=NNTI_wait(&get_dst_mr, NNTI_GET_DST, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << " sync get (1 MB transfer) == " << (double)(MAX_GETS_PER_CLIENT*one_mb)/one_mb/op_timer << " MBps" << std::endl;

    op_timer=trios_get_time();
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_get(&get_src_mr, client_rank*four_mb, four_mb, &get_dst_mr, 0);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_get() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        rc=NNTI_wait(&get_dst_mr, NNTI_GET_DST, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << " sync get (4 MB transfer) == " << (double)(MAX_GETS_PER_CLIENT*four_mb)/one_mb/op_timer << " MBps" << std::endl;

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 5 - test async get performance
     */
    // warm up the pipes
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_get(&get_src_mr, client_rank*four_mb, four_mb, &get_dst_mr, 0);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_get() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_wait(&get_dst_mr, NNTI_GET_DST, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }

    op_timer=trios_get_time();
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_get(&get_src_mr, client_rank*four_mb, one_kb, &get_dst_mr, 0);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_get() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_wait(&get_dst_mr, NNTI_GET_DST, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << "async get (1 KB transfer) == " << (double)(MAX_GETS_PER_CLIENT*one_kb)/one_mb/op_timer << " MBps" << std::endl;

    op_timer=trios_get_time();
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_get(&get_src_mr, client_rank*four_mb, one_mb, &get_dst_mr, 0);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_get() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_wait(&get_dst_mr, NNTI_GET_DST, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << "async get (1 MB transfer) == " << (double)(MAX_GETS_PER_CLIENT*one_mb)/one_mb/op_timer << " MBps" << std::endl;

    op_timer=trios_get_time();
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_get(&get_src_mr, client_rank*four_mb, four_mb, &get_dst_mr, 0);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_get() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    for (int i=0;i<MAX_GETS_PER_CLIENT;i++) {
        rc=NNTI_wait(&get_dst_mr, NNTI_GET_DST, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << "async get (4 MB transfer) == " << (double)(MAX_GETS_PER_CLIENT*four_mb)/one_mb/op_timer << " MBps" << std::endl;

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 6 - test sync put performance
     */
    op_timer=trios_get_time();
    for (int i=0;i<MAX_PUTS_PER_CLIENT;i++) {
        rc=NNTI_put(&put_src_mr, 0, one_kb, &put_dst_mr, client_rank*four_mb);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_put() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        rc=NNTI_wait(&put_src_mr, NNTI_PUT_SRC, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << " sync put (1 KB transfer) == " << (double)(MAX_PUTS_PER_CLIENT*one_kb)/one_mb/op_timer << " MBps" << std::endl;

    op_timer=trios_get_time();
    for (int i=0;i<MAX_PUTS_PER_CLIENT;i++) {
        rc=NNTI_put(&put_src_mr, 0, one_mb, &put_dst_mr, client_rank*four_mb);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_put() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        rc=NNTI_wait(&put_src_mr, NNTI_PUT_SRC, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << " sync put (1 MB transfer) == " << (double)(MAX_PUTS_PER_CLIENT*one_mb)/one_mb/op_timer << " MBps" << std::endl;

    op_timer=trios_get_time();
    for (int i=0;i<MAX_PUTS_PER_CLIENT;i++) {
        rc=NNTI_put(&put_src_mr, 0, four_mb, &put_dst_mr, client_rank*four_mb);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_put() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        rc=NNTI_wait(&put_src_mr, NNTI_PUT_SRC, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << " sync put (4 MB transfer) == " << (double)(MAX_PUTS_PER_CLIENT*four_mb)/one_mb/op_timer << " MBps" << std::endl;

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 7 - test async put performance
     */
    op_timer=trios_get_time();
    for (int i=0;i<MAX_PUTS_PER_CLIENT;i++) {
        rc=NNTI_put(&put_src_mr, 0, one_kb, &put_dst_mr, client_rank*four_mb);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_put() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    for (int i=0;i<MAX_PUTS_PER_CLIENT;i++) {
        rc=NNTI_wait(&put_src_mr, NNTI_PUT_SRC, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << "async put (1 KB transfer) == " << (double)(MAX_PUTS_PER_CLIENT*one_kb)/one_mb/op_timer << " MBps" << std::endl;

    op_timer=trios_get_time();
    for (int i=0;i<MAX_PUTS_PER_CLIENT;i++) {
        rc=NNTI_put(&put_src_mr, 0, one_mb, &put_dst_mr, client_rank*four_mb);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_put() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    for (int i=0;i<MAX_PUTS_PER_CLIENT;i++) {
        rc=NNTI_wait(&put_src_mr, NNTI_PUT_SRC, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << "async put (1 MB transfer) == " << (double)(MAX_PUTS_PER_CLIENT*one_mb)/one_mb/op_timer << " MBps" << std::endl;

    op_timer=trios_get_time();
    for (int i=0;i<MAX_PUTS_PER_CLIENT;i++) {
        rc=NNTI_put(&put_src_mr, 0, four_mb, &put_dst_mr, client_rank*four_mb);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_put() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    for (int i=0;i<MAX_PUTS_PER_CLIENT;i++) {
        rc=NNTI_wait(&put_src_mr, NNTI_PUT_SRC, 1000, &rdma_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() did not return NNTI_EDROPPED: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }
    op_timer=trios_get_time()-op_timer;
    out << "async put (4 MB transfer) == " << (double)(MAX_PUTS_PER_CLIENT*four_mb)/one_mb/op_timer << " MBps" << std::endl;

    MPI_Barrier(MPI_COMM_WORLD);


    NNTI_unregister_memory(&send_mr);
    free(send_buf);

    NNTI_unregister_memory(&client_ack_mr);
    free(client_ack_buf);

    NNTI_unregister_memory(&get_dst_mr);
    free(get_dst_buf);

    NNTI_unregister_memory(&put_src_mr);
    free(put_src_buf);

    return;
}

void server(void)
{
    NNTI_result_t rc=NNTI_OK;
    NNTI_status_t queue_status;
    NNTI_status_t send_status;
    NNTI_status_t server_ack_status;
    char *c_ptr;
    void    *packed=NULL;
    int32_t  packed_size=0;


    int num_elements=nclients+(4*nclients*MAX_SENDS_PER_CLIENT);
    char *queue_buf=(char *)malloc(num_elements*NNTI_REQUEST_BUFFER_SIZE);
    memset(queue_buf, 0, num_elements*NNTI_REQUEST_BUFFER_SIZE);
    NNTI_register_memory(&trans_hdl, queue_buf, NNTI_REQUEST_BUFFER_SIZE, num_elements, NNTI_RECV_QUEUE, NULL, &queue_mr);

    char *send_buf=(char *)malloc(NNTI_REQUEST_BUFFER_SIZE);
    memset(send_buf, 0, NNTI_REQUEST_BUFFER_SIZE);
    NNTI_register_memory(&trans_hdl, send_buf, NNTI_REQUEST_BUFFER_SIZE, 1, NNTI_SEND_SRC, NULL, &send_mr);

    char *server_ack_buf=(char *)malloc(NNTI_REQUEST_BUFFER_SIZE);
    memset(server_ack_buf, 0, NNTI_REQUEST_BUFFER_SIZE);
    NNTI_register_memory(&trans_hdl, server_ack_buf, NNTI_REQUEST_BUFFER_SIZE, 1, NNTI_RECV_DST, NULL, &server_ack_mr);

    char *get_src_buf=(char *)malloc(nclients*four_mb);
    memset(get_src_buf, 0, nclients*four_mb);
    NNTI_register_memory(&trans_hdl, get_src_buf, nclients*four_mb, 1, NNTI_GET_SRC, NULL, &get_src_mr);

    char *put_dst_buf=(char *)malloc(nclients*four_mb);
    memset(put_dst_buf, 0, nclients*four_mb);
    NNTI_register_memory(&trans_hdl, put_dst_buf, nclients*four_mb, 1, NNTI_PUT_DST, NULL, &put_dst_mr);


    /*
     * Phase 1 - exchange buffers handles
     */
    // wait for the client to send it's recv_mr
    NNTI_wait(&queue_mr, NNTI_RECV_QUEUE, -1, &queue_status);

    c_ptr=(char*)queue_status.start+queue_status.offset;
    buffer_unpack(c_ptr, queue_status.length, &client_ack_mr, (xdrproc_t)&xdr_NNTI_buffer_t);

//    fprint_NNTI_buffer(logger_get_file(), "client_ack_mr",
//            "received client ack hdl", &client_ack_mr);

    // send our server_ack_mr, get_src_mr and put_dst_mr back to the client
    buffer_pack(&server_ack_mr, &packed, &packed_size, (xdrproc_t)&xdr_NNTI_buffer_t);
    if (packed_size > NNTI_REQUEST_BUFFER_SIZE) {
        log_error(nntiperf_debug_level, "buffer_pack() says encoded NNTI_buffer_t is larger than NNTI_REQUEST_BUFFER_SIZE");
    	MPI_Abort(MPI_COMM_WORLD, -10);
    }

    char *ptr=send_buf;
    memcpy(ptr, &packed_size, sizeof(packed_size));
    ptr += sizeof(packed_size);
    memcpy(ptr, packed, packed_size);
    ptr += packed_size;

    buffer_pack_free(packed, packed_size, (xdrproc_t)&xdr_NNTI_buffer_t);

    buffer_pack(&get_src_mr, &packed, &packed_size, (xdrproc_t)&xdr_NNTI_buffer_t);
    if (packed_size > NNTI_REQUEST_BUFFER_SIZE) {
        log_error(nntiperf_debug_level, "buffer_pack() says encoded NNTI_buffer_t is larger than NNTI_REQUEST_BUFFER_SIZE");
    	MPI_Abort(MPI_COMM_WORLD, -10);
    }

    memcpy(ptr, &packed_size, sizeof(packed_size));
    ptr += sizeof(packed_size);
    memcpy(ptr, packed, packed_size);
    ptr += packed_size;

    buffer_pack_free(packed, packed_size, (xdrproc_t)&xdr_NNTI_buffer_t);

    buffer_pack(&put_dst_mr, &packed, &packed_size, (xdrproc_t)&xdr_NNTI_buffer_t);
    if (packed_size > NNTI_REQUEST_BUFFER_SIZE) {
        log_error(nntiperf_debug_level, "buffer_pack() says encoded NNTI_buffer_t is larger than NNTI_REQUEST_BUFFER_SIZE");
    	MPI_Abort(MPI_COMM_WORLD, -10);
    }

    memcpy(ptr, &packed_size, sizeof(packed_size));
    ptr += sizeof(packed_size);
    memcpy(ptr, packed, packed_size);
    ptr += packed_size;

    buffer_pack_free(packed, packed_size, (xdrproc_t)&xdr_NNTI_buffer_t);

    rc=NNTI_send(&queue_status.src, &send_mr, &client_ack_mr);
    if (rc != NNTI_OK) {
        log_error(nntiperf_debug_level, "NNTI_send() returned an error: %d", rc);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    rc=NNTI_wait(&send_mr, NNTI_SEND_SRC, 5000, &send_status);
    if (rc != NNTI_OK) {
        log_error(nntiperf_debug_level, "NNTI_wait() returned an error: %d", rc);
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 2 - client sends sync requests
     */
    for (int i=0;i<nclients*MAX_SENDS_PER_CLIENT;i++) {
        rc=NNTI_wait(&queue_mr, NNTI_RECV_QUEUE, 1000, &queue_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 3 - client sends async requests
     */
    for (int i=0;i<nclients*MAX_SENDS_PER_CLIENT;i++) {
        rc=NNTI_wait(&queue_mr, NNTI_RECV_QUEUE, 1000, &queue_status);
        if (rc != NNTI_OK) {
            log_error(nntiperf_debug_level, "NNTI_wait() returned an error: %d", rc);
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 4 - client does sync gets
     */
    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 5 - client does async gets
     */
    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 6 - client does sync puts
     */
    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Phase 7 - client does async puts
     */
    MPI_Barrier(MPI_COMM_WORLD);


    NNTI_unregister_memory(&queue_mr);
    free(queue_buf);

    NNTI_unregister_memory(&send_mr);
    free(send_buf);

    NNTI_unregister_memory(&server_ack_mr);
    free(server_ack_buf);

    NNTI_unregister_memory(&get_src_mr);
    free(get_src_buf);

    NNTI_unregister_memory(&put_dst_mr);
    free(put_dst_buf);

    return;
}

int main(int argc, char *argv[])
{
    char logname[1024];

    bool success=true;


    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    nservers=1;
    nclients=nprocs-nservers;
    client_rank=rank-1;

    sprintf(logname, "nntiperf.%03d.log", rank);
    logger_init(LOG_ERROR, NULL);

    NNTI_init(NNTI_DEFAULT_TRANSPORT, NULL, &trans_hdl);

    if (rank==0) {
        NNTI_get_url(&trans_hdl, url, NNTI_URL_LEN);
    }

    MPI_Bcast(&url[0], NNTI_URL_LEN, MPI_CHAR, 0, MPI_COMM_WORLD);

    log_debug(nntiperf_debug_level, "NNTI perfermance server url is %s", url);

    if (rank==0) {
        server();
    } else {
        client();
    }

    NNTI_fini(&trans_hdl);

    MPI_Finalize();

    logger_fini();

    Teuchos::oblackholestream blackhole;
    std::ostream &out = ( rank == 1 ? std::cout : blackhole );
    if (success)
        out << "\nEnd Result: TEST PASSED" << std::endl;
    else
        out << "\nEnd Result: TEST FAILED" << std::endl;

    return (success ? 0 : 1 );
//    return 0;
}
