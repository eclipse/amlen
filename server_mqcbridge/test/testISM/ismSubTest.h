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

/* Include file for subsciber test program at the ISM end           */

#include <ismPubSubTest.h>

/* Publication Target Descriptor */
typedef struct _imbt_publish_target_t
{
   struct _imbt_publish_target_t *pNextPublishTarget;
   MQTTClient_deliveryToken deliveryToken;
   int messageCount;
   char* pTopicName;
   size_t topicNameLength;
   size_t topicNameSize;
} imbt_publish_target_t;
