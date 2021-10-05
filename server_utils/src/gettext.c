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

#include <ismutil.h>
#include <log.h>
#include <locale.h>
#include <stdarg.h>

/* External function declarations */
int ism_common_initLocale(ism_prop_t * props);
const char * ism_common_getLocale(void);


static FILE * getConsoleOutputFile(ISM_LOGLEV level) {
    switch (level) {
    case ISM_LOGLEV_CRIT:
    case ISM_LOGLEV_ERROR:
    case ISM_LOGLEV_WARN:
        return stderr;
    case ISM_LOGLEV_NOTICE:
    case ISM_LOGLEV_INFO:
    default:
        return stdout;
    }
}

/*
 * Retrieve a message from the message catalog and do substitutions.
 */
int main(int argc, const char *argv[]) {
	int rc = 0;
	const char * msgID;
	const char * level;
	char * locale = getenv("IMA_LOCALE");
	char * catpath = getenv("IMA_USE_CATALOG_DIR");

	int replcount = argc - 3;
	if (replcount < 0) {
		return 2;
	}

	msgID = argv[1];
	if (msgID == NULL) {
		return 3;
	}
	level = argv[2];
	if (level == NULL) {
		return 4;
	}

	ISM_LOGLEV logLevel;
	rc = ism_log_getISMLogLevel(level, &logLevel);
	if (rc) {
		return 5;
	}
	FILE * ofile = getConsoleOutputFile(logLevel);
	
	ism_prop_t * props = NULL;
	if (locale) {
	    ism_field_t f;
	    f.type = VT_String;
	    f.val.s = locale;
	    props = ism_common_newProperties(1);
	    ism_common_setProperty(props, "Locale", &f);
	}
	rc = ism_common_initLocale(props);
   	ism_common_freeProperties(props);
    if (rc) {
		return 6;
	}

	char msgx[1024*8];
	const char * msgf;
	char * buf;
	int    buflen = 256;
	int    inheap = 0;
	int    mlen;
	int    bytes_needed;
	const char *    repl[64];
	int i=0;

	/*
	 * Get the messasge
	 */
	if ( catpath && *catpath != '\0' ) {
		msgf = ism_common_getMessageByLocaleAndCatalogPath((const char *)catpath, msgID, ism_common_getLocale(), msgx, sizeof(msgx), &mlen);
	} else {
	    msgf = ism_common_getMessageByLocale(msgID, ism_common_getLocale(), msgx, sizeof(msgx), &mlen);
	}

	/* If cannot find the message, report error. */
	if (msgf) {
		buflen += strlen(msgf);
	} else {
		return 1;
	}

	/*
	 * Create the buffer and build the log message
	 */
	if (buflen < 8192) {
		buf = alloca(buflen);
	} else {
		buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,215),buflen);
		inheap = 1;
	}

	/*Getting the replacement data*/
	for (i=0; i < replcount; i++) {
		repl[i] = argv[i + 3];
	}

	bytes_needed = ism_common_formatMessage(buf, buflen, msgf, repl, replcount);

	if (bytes_needed > buflen) {
		  if (inheap)
				buf = ism_common_realloc(ISM_MEM_PROBE(ism_memory_utils_misc,216),buf, bytes_needed);
		  else {
				if (bytes_needed < 8192 - buflen) {
					buf = alloca(bytes_needed);
				} else {
					buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,217),bytes_needed);
					inheap = 1;
				}
		  }
		  buflen = bytes_needed;
		  ism_common_formatMessage(buf, buflen, msgf, repl, replcount);
	}

	/* Print out to console */
	fprintf(ofile, "%s\n", buf);

	if (inheap)
		ism_common_free(ism_memory_utils_misc,buf);

	return rc;
}

