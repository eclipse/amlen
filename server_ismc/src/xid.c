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

/*
 * @file xid.c
 * Implement the XA transaction support
 */
#include <ismc_p.h>
#include <ismc.h>
#include <engine.h>

int ismc_startGlobalTransaction(ismc_session_t * session, ism_xid_t * xid) {
    action_t * act;
    int rc = checkAndLockSession(session);

    TRACE(7, ">>> %s, session: %p\n", __func__, session);

    if (!rc && session->transacted != ISMC_GLOBAL_TRANSACTION) {
        rc = ismc_setError(ISMRC_ObjectNotValid, "The session must be created as globally transacted");
    }
    if (!rc && session->globalTransaction) {
        rc = ismc_setError(ISMRC_ObjectNotValid, "The session already has an unprepared global transaction");
    }
    if (!rc && !xid) {
    	rc = ismc_setError(ISMRC_NullPointer, "The XID must not be NULL");
    }
    if (!rc) {
        act = ismc_newAction(session->connect, session, Action_startGlobalTransaction);
        /* val0 = XID */
        ism_protocol_putXidValue(&act->buf, xid);
        ism_protocol_putIntValue(&act->buf, ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT);
        act->hdr.hdrcount = 2;

        rc = ismc_request(act, 1);
        ismc_freeAction(act);

        if (!rc) {
        	session->globalTransaction = 1;
        }
    }

    unlockSession(session);

    TRACE(7, "<<< %s\n", __func__);

    return rc;
}

int ismc_endGlobalTransaction(ismc_session_t * session) {
    action_t * act;
    int rc = checkAndLockSession(session);

    TRACE(7, ">>> %s, session: %p\n", __func__, session);

    if (!rc && session->transacted != ISMC_GLOBAL_TRANSACTION) {
        rc = ismc_setError(ISMRC_ObjectNotValid, "The session must be created as globally transacted");
    }
    if (!rc) {
        act = ismc_newAction(session->connect, session, Action_endGlobalTransaction);
        ism_protocol_putIntValue(&act->buf, ismENGINE_END_TRANSACTION_OPTION_DEFAULT);

        ismc_writeAckSqns(act, session, NULL);
        act->hdr.hdrcount++;		// Account for the flag value in the first position

        rc = ismc_request(act, 1);
        ismc_freeAction(act);

        if (!rc) {
            session->globalTransaction = 0;
        }
    }

    unlockSession(session);

    TRACE(7, "<<< %s\n", __func__);

    return rc;
}

static int ismc_processXAAction(ismc_session_t * session, ism_xid_t * xid, int action, int *flag) {
    action_t * act;
    int rc = checkAndLockSession(session);

    TRACE(7, ">>> %s, session: %p %d\n", __func__, session, action);

    if (!rc && session->transacted != ISMC_GLOBAL_TRANSACTION) {
        rc = ismc_setError(ISMRC_ObjectNotValid, "The session must be created as globally transacted");
    }
    if (!rc) {
        act = ismc_newAction(session->connect, session, action);
        /* val0 = XID */
        ism_protocol_putXidValue(&act->buf, xid);
        act->hdr.hdrcount = 1;
        if(flag) {
            ism_protocol_putBooleanValue(&act->buf,(*flag != 0));
            act->hdr.hdrcount = 2;
        }

        rc = ismc_request(act, 1);
        ismc_freeAction(act);
    }

    unlockSession(session);

    TRACE(7, "<<< %s\n", __func__);

    return rc;
}

int ismc_prepareGlobalTransaction(ismc_session_t * session, ism_xid_t * xid) {
    return ismc_processXAAction(session,xid,Action_prepareGlobalTransaction,NULL);
}
int ismc_rollbackGlobalTransaction(ismc_session_t * session, ism_xid_t * xid) {
    return ismc_processXAAction(session,xid,Action_rollbackGlobalTransaction,NULL);
}
int ismc_commitGlobalTransaction(ismc_session_t * session, ism_xid_t * xid, int onePhase) {
    return ismc_processXAAction(session,xid,Action_commitGlobalTransaction,&onePhase);
}
/**
 * Request an array of the XIDs known to the ISM Server to be returned
 * in the buffer provided. Count defines the number of XID items that
 * would fit in the buffer. This function call establishes a cursor into
 * the XID table so repeated calls may be required in order that all of
 * the known XIDs are returned. A flags value of TMSTARTRSCAN will cause
 * the cursor to be reset to the start of the list before the list of XIDs
 * is returned. A flags value of TMENDRSCAN will cause the cursor to be reset
 * after the list of XIDs is returned. Both flags values may be specified
 * concurrently on a single invocation.
 *
 * @param session
 * @param xidBuffer  An array of XID items
 * @param count      The number of entries that fit in xidBuffer
 * @param flags
 * @return Number of entries placed in the xidBuffer or -1, if error
 */
int ismc_recoverGlobalTransactions(ismc_session_t * session, ism_xid_t * xidBuffer, int count, int flags) {
    action_t * act;
    ism_field_t field;
    int xidsReturned = 0;
    int rc = checkAndLockSession(session);

    TRACE(7, ">>> %s, session: %p\n", __func__, session);

    if (!rc) {
        act = ismc_newAction(session->connect, session, Action_recoverGlobalTransactions);
        /* val0 = flags */
        ism_protocol_putIntValue(&act->buf, flags);
        /* val1 = max entries */
        ism_protocol_putIntValue(&act->buf, count);
        act->hdr.hdrcount = 2;

        rc = ismc_request(act, 1);
        if (!rc) {
        	/* The header field contains the count of XIDs */
        	ism_protocol_getObjectValue(&act->buf, &field);
        	if (field.type == VT_Integer) {
        		xidsReturned = field.val.i;

        		if (xidsReturned <= count) {

        			/* The content of the message is a byte array that contains the list of
        			 * handles and associated data. */
        			ism_protocol_getObjectValue(&act->buf, &field);
        			if (field.type != VT_Null) {
        				concat_alloc_t map = { 0 };
        				int i;

        				map.len = field.len;
        				map.buf = field.val.s;
        				map.pos = 0;
        				map.used = field.len;

        				for (i = 0; i < xidsReturned; i++) {
        					/* XID value */
        					ism_protocol_getObjectValue(&map, &field);
        					ism_common_toXid(&field, &xidBuffer[i]);
        				}
        			} else if (xidsReturned > 0) {
        				rc = ismc_setError(ISMRC_BadClientData,
        						"List of XIDs is incorrect (rc=%d).", rc);
        			}
        		} else {
        			rc = ismc_setError(ISMRC_BadClientData,
        					"Returned more XIDs (%d) than requested (%d).",
        					xidsReturned, count);
        		}
        	} else {
        		rc =  ismc_setError(ISMRC_BadClientData,
        				"XID count is missing (rc=%d).", rc);
        	}
        }

        ismc_freeAction(act);
    }

    unlockSession(session);

    TRACE(7, "<<< %s\n", __func__);

    return (rc == 0)?xidsReturned:-1;
}
