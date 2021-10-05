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

/*
 * pxactivity.c
 */

#include "pxactivity.h"
#include "ActivityTracker.h"

std::unique_ptr<ism_proxy::ActivityTracker> tracker = NULL;

extern "C" {
extern int pxactEnabled;
//=== from pxcluster.h ========================================================

/*
 * This is declared only in the unit test, and is used to clear these
 * variables when doing repeated back to back tests of the C-API.
 */
extern void ism_pxactivity_test_destroy()
{
    tracker.release();
}

XAPI int ism_pxactivity_init(void)
{
    if (tracker)
    {
        TRACE(2, "Warning: %s already initialized, ignoring. rc=%d\n", __FUNCTION__, ISMRC_Error);
        return ISMRC_OK;
    }

	tracker.reset(new ism_proxy::ActivityTracker());
    if (tracker)
    {
        return ISMRC_OK;
    }
    else
    {
    	TRACE(1, "Error: %s failed to allocate object, rc=%d\n", __FUNCTION__, ISMRC_AllocateError);
        return ISMRC_AllocateError;
    }
}

XAPI uint8_t ism_pxactivity_is_started()
{
    if (tracker)
    {
        return static_cast<uint8_t>(tracker->isStarted());
    }
    else
    {
        return 0;
    }
}


XAPI PXACT_STATE_TYPE_t ism_pxactivity_get_state()
{
    if (tracker)
    {
        return tracker->getState();
    }
    else
    {
        return PXACT_STATE_TYPE_NONE;
    }
}

XAPI int ism_pxactivity_startMessaging()
{
    if (tracker)
    {
        int rc = tracker->startMessaging();
        TRACE(8, "%s Exit: rc=%d\n", __FUNCTION__,rc);
        return rc;
    }
    else
    {
        TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
                return ISMRC_NullPointer;
    }

}

XAPI uint8_t ism_pxactivity_is_messaging_started()
{
    if (tracker)
    {
        return static_cast<uint8_t>(tracker->isMessagingStarted());
    }
    else
    {
        return 0;
    }
}


XAPI void ism_pxactivity_ConfigActivityDB_Init(ismPXActivity_ConfigActivityDB_t *pConfigAffinityDB)
{
   if (pConfigAffinityDB)
   {
       // cppcheck-suppress memsetClassFloat
       memset(pConfigAffinityDB,0,sizeof(ismPXActivity_ConfigActivityDB_t));
       pConfigAffinityDB->memoryLimitPercentOfTotal = 0; //unlimited
   }
}

XAPI int ism_pxactivity_ConfigActivityDB_Set(ismPXActivity_ConfigActivityDB_t *pConfigAffinityDB)
{
    if (tracker)
    {
        int rc = tracker->configure(pConfigAffinityDB);
        TRACE(8, "%s Exit: rc=%d\n", __FUNCTION__,rc);
        return rc;
    }
    else
    {
        TRACE(3, "Error: %s failed, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }
}

XAPI int ism_pxactivity_start(void)
{
	if (tracker)
	{
		int rc = tracker->start();
        TRACE(8, "%s Exit: rc=%d\n", __FUNCTION__,rc);
		return rc;
	}
	else
	{
		TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}
}

XAPI int ism_pxactivity_stop()
{
    if (tracker)
    {
        int rc = tracker->stop();
        TRACE(8, "%s Exit: rc=%d\n", __FUNCTION__,rc);
        return rc;
    }
    else
    {
        TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }
}

XAPI int ism_pxactivity_term(void)
{

	if (tracker)
	{
		int rc = tracker->close(true);
		TRACE(8, "%s Exit: rc=%d\n", __FUNCTION__,rc);
		return rc;
	}
	else
	{
		TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}
}

XAPI int ism_pxactivity_Client_Activity(
		const ismPXACT_Client_t* pClient)
{

	if (tracker)
	{
		int rc = tracker->clientActivity(pClient);
		TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
		return rc;
	}
	else
	{
		TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}
}

XAPI int ism_pxactivity_Client_Connectivity(
		const ismPXACT_Client_t* pClient,
		const ismPXACT_ConnectivityInfo_t* pConnInfo)
{

	if (tracker)
	{
		int rc = tracker->clientConnectivity(pClient, pConnInfo);
		TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
		return rc;
	}
	else
	{
		TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
		return ISMRC_NullPointer;
	}
}

XAPI int ism_pxactivity_MonitoringPolicy_set(PXACT_CLIENT_TYPE_t client_type, int policy)
{
    if (tracker)
    {
        tracker->setClientTypePolicy(client_type, policy);
        TRACE(8, "%s Exit: rc=%d\n", __FUNCTION__,ISMRC_OK);
        return ISMRC_OK;
    }
    else
    {
        TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }
}

XAPI int ism_pxactivity_Stats_get(ismPXACT_Stats_t* pStats)
{
	if(!pxactEnabled)
	{
		TRACE(9, "%s Client Activity is disabled\n", __FUNCTION__);
		return 0;
	}
    if (tracker)
    {
        int rc = tracker->getStats(pStats);
        TRACE(8, "%s Exit: rc=%d\n", __FUNCTION__,rc);
        return rc;
    }
    else
    {
		TRACE(1, "Error: %s failed, rc=%d\n", __FUNCTION__, ISMRC_NullPointer);
		return ISMRC_NullPointer;

	}
}
}
