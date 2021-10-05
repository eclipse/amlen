/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

#ifndef __LUSi_DEFINED
#define __LUSi_DEFINED

#include "hashFunction.h"

typedef struct
{
   int                 numHashValues;  /* Number of hash values to use in lookup */
   mcc_hash_HashType_t hashType;     /* Hash function used by this BFSet       */
} mcc_hash_t ; 

#endif
