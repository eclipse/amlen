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

//****************************************************************************
/// @file  test_utils_security.h
/// @brief Utility functions for testing security
//****************************************************************************
#ifndef __ISM_TEST_UTILS_SECURITY_H_DEFINED
#define __ISM_TEST_UTILS_SECURITY_H_DEFINED

#include <stdbool.h>
#include <stdint.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "engine.h"
#include "security.h"
#include "transport.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L
//Initialise OpenSSL - in the product the equivalent is ism_ssl_init()
//which ism_transport_init calls
static inline void sslInit(void)
{
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);
}

// Term OpenSSL - equivalent of ism_ssl_term()
static inline void sslTerm(void)
{
    EVP_cleanup();
    ERR_free_strings();
}
#else
#define sslInit()
#define sslTerm()
#endif

// From authorization.c, a local representation of the start of the ism_Security_t
// structure.
typedef struct mock_ismSecurity_t
{
    int                type;
    ism_transport_t *  transport;
    pthread_spinlock_t lock;
    int                reValidatePolicy;
    ExpectedMsgRate_t  ExpectedMsgRate;
    uint32_t           InFlightMsgLimit;
    uint32_t           ClientStateExpiry;
    /*...*/
} mock_ismSecurity_t;

int test_fake_security_configCallback(char * object,
                                      char * name,
                                      ism_prop_t * props,
                                      ism_ConfigChangeType_t flag);

//****************************************************************************
/// @brief  Release memory used by fake security routines
///
/// @param[in]     enabled  Whether fake validation routines should be enabled
//****************************************************************************
void test_freeFakeSecurityData(void);

//****************************************************************************
/// @brief  Save the fake security configuration to a props array
///
/// @param[in,out] props  Array of 50 properties to store the result
//****************************************************************************
void test_saveFakeSecurityData(ism_prop_t **props);

//****************************************************************************
/// @brief  Load the fake security configuration from a saved props array
///
/// @param[in]     props  Array of 50 properties to load
//****************************************************************************
void test_loadFakeSecurityData(ism_prop_t **props);

//****************************************************************************
/// @brief  Create a mock-up of a connection security context for the engine
///
/// @param[in,out] ppListener    Address of pointer to listener to use / create
/// @param[in,out] ppTransport   Address of pointer to transport to use / create
/// @param[out]    ppSecContext  Address of pointer to receive context
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_createSecurityContext(ism_listener_t **ppListener,
                                   ism_transport_t **ppTransport,
                                   ismSecurity_t **ppSecContext);

//****************************************************************************
/// @brief  Add a security policy to the security component
///
/// @param[in]     policyType    One of
///                                 TopicPolicy
///                                 QueuePolicy
///                                 SubscriptionPolicy
/// @param[in]     inputBuffer   JSON formatted character string representing policy
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_addSecurityPolicy(char *policyType, char *inputBuffer);

//****************************************************************************
/// @internal
///
/// @brief  Fake up admin deletion of policy
///
/// @param[in]     UID  Unique identifier of the policy to delete
/// @param[in]     Name Name of the policy to delete
///
/// @remark This does not remove the policy from an contexts to which it was
///         attached.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_deleteSecurityPolicy(char *UID, char *Name);

//****************************************************************************
/// @brief  Add a named messaging policy to the listener associated with this
///         security context.
///
/// @param[in,out] pSecContext  The security context to update
/// @param[in]     policy       The name of the policy to add
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_addPolicyToSecContext(ismSecurity_t *pSecContext, char *policy);

//****************************************************************************
/// @brief  Destroy the mock-up of a security context, and any other storage
//          used for it.
///
/// @param[in]     pListener    Pointer to previously created listener
/// @param[in]     pTransport   Pointer to previously created transport
/// @param[in]     pSecContext  Pointer to previously created context
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_destroySecurityContext(ism_listener_t *pListener,
                                    ism_transport_t *pTransport,
                                    ismSecurity_t *pSecContext);

#endif //end ifndef __ISM_TEST_UTILS_SECURITY_H_DEFINED
