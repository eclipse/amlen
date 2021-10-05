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
 * error.c
 */

#include <ismc_p.h>
#include <stdarg.h>

/*
 * Set up the thread local storage to keep the last error and
 * make sure all places which set a return code call ismc_setError().
 */
#define ISM_MAX_ERROR_MESSAGE_LENGTH 			4096
#define ISM_NO_ERROR							"No error message available"


/*
 * Set an error with variable replacement data.
 *
 * @param rc  		An error code
 * @param reason 	An error description (format string as in sprintf)
 * @return An error code
 */
int ismc_setError(int rc, const char * reason, ...) {
    /* TEMP - trace reason and variable data */
    if (rc != ISMRC_TimeOut)
    {
        va_list args;
        char errorMessage[ISM_MAX_ERROR_MESSAGE_LENGTH];
        va_start(args, reason);
        vsnprintf(errorMessage, sizeof(errorMessage), reason, args);
        va_end(args);
        TRACE(8, "ismc_setError: %s\n", errorMessage);
    }
    /* END TEMP */

    ism_common_setError(rc);
	return rc;
}


/*
 * Get the last error in this thread.
 * The reason string of the last error is filled in the supplied buffer.
 * @param errbuf  The buffer to return the error reason string
 * @param len     The length of the error buffer.
 * @return The last return code
 */
int ismc_getLastError(char * errbuf, int len) {
    int rc = ism_common_getLastError();
    ism_common_getErrorString(rc, errbuf, len);
	return rc;
}


/*
 * Get error string.
 * The reason string of the last error is filled in the supplied buffer.
 * Since no replacement data is available, any message with replacement data is
 * returned as a Java message format string.
 * @param rc      The return code to show the reason for
 * @param errbuf  The buffer to return the error reason string
 * @param len     The length of the error buffer.
 * @return  A pointer to the buffer with the error message.
 */
const char * ismc_getErrorString(int rc, char * errbuf, int len) {
    return ism_common_getErrorString(rc, errbuf, len);
}
