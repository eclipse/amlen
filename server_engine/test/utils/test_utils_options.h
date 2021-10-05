/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
/// @file  test_utils_options.h
/// @brief Options manipulation functions / macros
//****************************************************************************
#ifndef __ISM_TEST_UTILS_OPTIONS_H_DEFINED
#define __ISM_TEST_UTILS_OPTIONS_H_DEFINED

#include "engineInternal.h"

#define testDEFAULT_PROTOCOL_ID 99

// Return the default consumer options expected based on the subOptions specified
static inline uint32_t test_getDefaultConsumerOptions(uint32_t subOptions)
{
    if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE) != 0)
    {
        return ismENGINE_CONSUMER_OPTION_ACK;
    }
    else if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) == ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE)
    {
        return ismENGINE_CONSUMER_OPTION_NONE;
    }

    return ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;
}
static inline uint32_t test_getDefaultConsumerOptionsFromQType(ismQueueType_t type)
{
    if (type == multiConsumer)
    {
        return ismENGINE_CONSUMER_OPTION_ACK;
    }
    else if (type == intermediate)
    {
        return ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;
    }

    return ismENGINE_CONSUMER_OPTION_NONE;
}
#endif //end ifndef __ISM_TEST_UTILS_OPTIONS_H_DEFINED
