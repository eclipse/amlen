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


/* If there are are experimental versions of these functions available, then
   use them */
#ifndef MQBRIDGE_EXTRA_SRC
#include "mqcInternal.h"

int mqc_SPINotify(mqcRuleQM_t * pRuleQM,
                  MQHCONN  hConn,
                  MQHOBJ   hObj,
                  MQBYTE24 ConnectionId,
                  PMQLONG  pCompCode,
                  PMQLONG  pReason)
{
    int rc = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    /* We need to figure out how to wake up the thing listening on
       hobj - don't think that's possible at the mo - pcf stop conn? */

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

void mqc_SPISetProdId(mqcRuleQM_t *pRuleQM,
                      PMQLONG  pCompCode,
                      PMQLONG  pReason)
{
    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    /* I don't think we can currently set the prod id */

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

#endif
