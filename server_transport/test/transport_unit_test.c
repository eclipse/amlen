/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#include <transport_unit_test.h>
#include <transport.h>

/*
 * Array that carries all the transport tests for APIs to CUnit framework.
 */
CU_TestInfo ISM_transport_tests[] = {
    { "--- Testing ism_transport_newTransport ---", testNewTransport },
    { "--- Testing protocol registration      ---", testProtocolRegistration },
    { "--- Testing transport memory allocation---", testAllocBytes },
    { "--- Testing close connection           ---", testCloseConnection },
    CU_TEST_INFO_NULL
};

int protocolHandler1(ism_transport_t * transport);
int protocolHandler2(ism_transport_t * transport);
int protocolHandler3(ism_transport_t * transport);
int protocolHandler4(ism_transport_t * transport);

int framer1(ism_transport_t * transport, char * buf, int buflen);
int framer2(ism_transport_t * transport, char * buf, int buflen);
int framer3(ism_transport_t * transport, char * buf, int buflen);

/**
 * Test transport instance creation.
 */
void testNewTransport(void) {
    const int TOBJ_INIT_SIZE = 1536;
    ism_listener_t * listener = calloc(1, sizeof(ism_listener_t) + sizeof(ism_endstat_t));
    listener->stats = (ism_endstat_t *)(listener+1);
    listener->port = 12345;
    listener->name = "test";

    ism_transport_t *transport = ism_transport_newTransport(listener, 0, 0);
    CU_ASSERT((transport != NULL) && (transport->suballoc.size == (TOBJ_INIT_SIZE-sizeof(ism_transport_t))) && (transport->suballoc.pos == 0));
    CU_ASSERT((transport != NULL) && (transport->state == ISM_TRANST_Opening));
    CU_ASSERT((transport != NULL) && (transport->tobj == NULL));
    CU_ASSERT((transport != NULL) && (transport->listener = listener));
    ism_transport_freeTransport(transport);

    transport = ism_transport_newTransport(NULL, 512, 0);
    CU_ASSERT((transport != NULL) && (transport->suballoc.size == (TOBJ_INIT_SIZE-sizeof(ism_transport_t))) && (transport->suballoc.pos == 512));
    CU_ASSERT((transport != NULL) && (transport->state == ISM_TRANST_Opening));
    CU_ASSERT((transport != NULL) && (transport->tobj != NULL));
    ism_transport_freeTransport(transport);

    transport = ism_transport_newTransport(NULL, 2*TOBJ_INIT_SIZE, 1);
    CU_ASSERT((transport != NULL) && (transport->suballoc.size > 0) && (transport->suballoc.pos == 2*TOBJ_INIT_SIZE));
    CU_ASSERT((transport != NULL) && (transport->state == ISM_TRANST_Opening));
    CU_ASSERT((transport != NULL) && (transport->tobj != NULL));
    ism_transport_freeTransport(transport);

}

/**
 * Test protocol registration (ism_transport_registerProtocol
 * and ism_transport_findProtocol).
 *
 * Register 1-4 protocol handlers and test find routines.
 */
void testProtocolRegistration(void) {
    ism_listener_t * listener = calloc(1, sizeof(ism_listener_t) + sizeof(ism_endstat_t));
    listener->stats = (ism_endstat_t *)(listener+1);
    listener->port = 12345;
    listener->name = "test";
    ism_transport_t *transport = ism_transport_newTransport(listener, 128, 0);

    ism_transport_registerProtocol(NULL, protocolHandler1);
    transport->protocol = "prot1";
    CU_ASSERT((ism_transport_findProtocol(transport) == 0));
    transport->protocol = "prot2";
    CU_ASSERT(ism_transport_findProtocol(transport) != 0);

    ism_transport_registerProtocol(NULL, protocolHandler3);
    CU_ASSERT(ism_transport_findProtocol(transport) != 0);
    transport->protocol = "prot3";
    CU_ASSERT((ism_transport_findProtocol(transport) == 0));
    transport->protocol = "prot1";
    CU_ASSERT((ism_transport_findProtocol(transport) == 0));

    ism_transport_registerProtocol(NULL, protocolHandler2);
    ism_transport_registerProtocol(NULL, protocolHandler4);
    transport->protocol = "prot1";
    CU_ASSERT((ism_transport_findProtocol(transport) == 0));
    transport->protocol = "prot2";
    CU_ASSERT((ism_transport_findProtocol(transport) == 0));
    transport->protocol = "prot3";
    CU_ASSERT((ism_transport_findProtocol(transport) == 0));
    transport->protocol = "prot4";
    CU_ASSERT((ism_transport_findProtocol(transport) == 0));

    ism_transport_freeTransport(transport);
}


/**
 * Test ism_transport_allocBytes function.
 */
void testAllocBytes(void) {
    ism_transport_t *transport;

    /*
     * Check memory allocation above 848 bytes (0x800 - 1200) to force new allocation
     */
    const int SIZE1 = 855;
    transport = ism_transport_newTransport(NULL, 512, 0);
    ism_transport_allocBytes(transport, SIZE1, 0);
    CU_ASSERT((transport->suballoc.next != NULL) && (transport->suballoc.next->pos == SIZE1));
    CU_ASSERT((transport->suballoc.next != NULL) && (transport->suballoc.next->size == (2048 -sizeof(struct suballoc_t))));
    CU_ASSERT((transport->suballoc.next != NULL) && (transport->suballoc.next->next == NULL));
    ism_transport_freeTransport(transport);
    sleep(1);
    /*
     * Check memory allocation below 848 bytes with additional small and large allocations
     */
    const int SIZE2 = 800;
    const int SIZE3 = 10;
    transport = ism_transport_newTransport(NULL, 512, 0);
    ism_transport_allocBytes(transport, SIZE2, 0);      // New allocation with available 976 bytes
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.next->pos == SIZE2));
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.next->size == (1024 -sizeof(struct suballoc_t))));
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.next->next == NULL));

    ism_transport_allocBytes(transport, SIZE3, 0);      // Fit into existing allocation
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.pos == (512 + SIZE3)));
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.next->size == (1024 -sizeof(struct suballoc_t))));
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.next->next == NULL));

    ism_transport_allocBytes(transport, 2*SIZE2, 0);    // Force new allocation
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.next->pos == SIZE2));
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.next->size == (1024 -sizeof(struct suballoc_t))));
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.next->next != NULL) &&
              (transport->suballoc.next->next->pos == 2*SIZE2));
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.next->next != NULL) &&
              (transport->suballoc.next->next->size == (2048 -sizeof(struct suballoc_t))));
    CU_ASSERT((transport->suballoc.next != NULL) &&
              (transport->suballoc.next->next != NULL) &&
              (transport->suballoc.next->next->next == NULL));

    ism_transport_freeTransport(transport);
}


int ism_testSelect(ism_transport_t * transport, const char * clientid, const char * userid, const char * client_addr, const char * endpoint);
/**
 * Test ism_transport_allocBytes function.
 */
void testCloseConnection(void) {
    ism_transport_t * transport;
    transport = ism_transport_newTransport(NULL, 512, 0);
    transport->endpoint_name = "fred";
    transport->userid = "sam";
    transport->name   = "tom";
    transport->client_addr = "1.2.3.4";

    CU_ASSERT(ism_testSelect(transport, "tom", NULL, NULL, NULL) == 1);
    CU_ASSERT(ism_testSelect(transport, "to*", NULL, NULL, NULL) == 1);
    CU_ASSERT(ism_testSelect(transport, "*o*", NULL, NULL, NULL) == 1);
    CU_ASSERT(ism_testSelect(transport, "sam", NULL, NULL, NULL) == 0);
    CU_ASSERT(ism_testSelect(transport, "tom", "sam", NULL, NULL) == 1);
    CU_ASSERT(ism_testSelect(transport, "tom", "sam", NULL, NULL) == 1);
    CU_ASSERT(ism_testSelect(transport, NULL,  "sa*", NULL, NULL) == 1);
    CU_ASSERT(ism_testSelect(transport, NULL,  "*am", NULL, "fred") == 1);
    CU_ASSERT(ism_testSelect(transport, NULL,  NULL, NULL, "*re*") == 1);
    CU_ASSERT(ism_testSelect(transport, NULL,  NULL, "1.2.3.4", "fred") == 1);
    CU_ASSERT(ism_testSelect(transport, NULL,  NULL, "1*", "fred") == 1);
}

/**
 * Dummy protocol handler responding to protocol "prot1"
 */
int protocolHandler1(ism_transport_t * transport) {
    if (!strcmpi(transport->protocol, "prot1")) {
        return 0;
    }

    return 1;
}

/**
 * Dummy protocol handler responding to protocol "prot2"
 */
int protocolHandler2(ism_transport_t * transport) {
    if (!strcmpi(transport->protocol, "prot2")) {
        return 0;
    }

    return 1;
}

/**
 * Dummy protocol handler responding to protocol "prot3"
 */
int protocolHandler3(ism_transport_t * transport) {
    if (!strcmpi(transport->protocol, "prot3")) {
        return 0;
    }

    return 1;
}

/**
 * Dummy protocol handler responding to protocol "prot4"
 */
int protocolHandler4(ism_transport_t * transport) {
    if (!strcmpi(transport->protocol, "prot4")) {
        return 0;
    }

    return 1;
}

/**
 * Dummy framer responding to handshake "frame1" in first 6 bytes of the input buffer.
 * @param transport  The transport object
 * @param buffer     The buffer containing the handshake
 * @param buflen     The length of the buffer
 * @return The number of bytes used if >= 0, of a return code if negative
 */
int framer1(ism_transport_t * transport, char * buf, int buflen) {
    const char * HANDSHAKE = "frame1";
    const int HANDSHAKE_LEN = strlen(HANDSHAKE);

    if (buflen >= HANDSHAKE_LEN && memcmp(buf, HANDSHAKE, HANDSHAKE_LEN) == 0) {
        return HANDSHAKE_LEN;
    } else {
        return -1;
    }
}

/**
 * Dummy framer responding to handshake "_frame2" in first 7 bytes of the input buffer.
 * @param transport  The transport object
 * @param buffer     The buffer containing the handshake
 * @param buflen     The length of the buffer
 * @return The number of bytes used if >= 0, of a return code if negative
 */
int framer2(ism_transport_t * transport, char * buf, int buflen) {
    const char * HANDSHAKE = "_frame2";
    const int HANDSHAKE_LEN = strlen(HANDSHAKE);

    if (buflen >= HANDSHAKE_LEN && memcmp(buf, HANDSHAKE, HANDSHAKE_LEN) == 0) {
        return HANDSHAKE_LEN;
    } else {
        return -1;
    }
}

/**
 * Dummy framer responding to handshake "__frame3" in first 8 bytes of the input buffer.
 * @param transport  The transport object
 * @param buffer     The buffer containing the handshake
 * @param buflen     The length of the buffer
 * @return The number of bytes used if >= 0, of a return code if negative
 */
int framer3(ism_transport_t * transport, char * buf, int buflen) {
    const char * HANDSHAKE = "__frame3";
    const int HANDSHAKE_LEN = strlen(HANDSHAKE);

    if (buflen >= HANDSHAKE_LEN && memcmp(buf, HANDSHAKE, HANDSHAKE_LEN) == 0) {
        return HANDSHAKE_LEN;
    } else {
        return -1;
    }
}
