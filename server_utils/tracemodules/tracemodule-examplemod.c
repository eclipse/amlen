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

#include <stdio.h>
#include <stdbool.h>
#include "ismutil.h"

//
//In order to compile this example module: copy to server_utils/src then add to server_utils/Makefile:
//
//libtracemodule-examplemod-FILES = tracemodule-examplemod.c
//
//LIB-TARGETS += $(LIBDIR)/libtracemodule-examplemod$(SO)
//$(LIBDIR)/libtracemodule-examplemod$(SO): $(call objects, $(libtracemodule-examplemod-FILES))
//	echo label=$(BUILD_LABEL) id=$(BUILD_ID) version=$(ISM_VERSION) FIPS_BUILD=$(FIPS_BUILD) CC=$(CC) FIPSLD_CC=$(FIPSLD_CC)
//	$(call make-c-library)
//
//DEBUG-LIB-TARGETS += $(DEBUG_LIBDIR)/libtracemodule-examplemod$(SO)
//$(DEBUG_LIBDIR)/libtracemodule-examplemod$(SO): $(call debug-objects, $(libtracemodule-examplemod-FILES))
//	$(call debug-make-c-library)
//
//COVERAGE-LIB-TARGETS += $(COVERAGE_LIBDIR)/libtracemodule-examplemod$(SO)
//$(COVERAGE_LIBDIR)/libtracemodule-examplemod$(SO): $(call coverage-objects, $(libtracemodule-examplemod-FILES))
//	$(call coverage-make-c-library)

FILE *fp = NULL;

int ism_initTraceModule(  ism_prop_t *props
		                , char *errorBuffer
		                , int errorBuffSize
		                , int *trclevel )
{
	snprintf(errorBuffer, errorBuffSize, "");

	fp = fopen("/ima/logs/tracemodule.log", "w");

	if (!fp) {
		snprintf(errorBuffer, errorBuffSize, "Unable to open log file\n");
		return 10;
	}

	return 0;
}

void ism_insertTrace(int level, int opt, const char * file,
                                           int lineno, const char * fmt, ...)
{
    fprintf(fp, "INSERTTRACE\n");
}

void ism_insertTraceData(const char * label, int option, const char * file,
                            int where, void * vbuf, int buflen, int maxlen)
{
    fprintf(fp, "INSERTTRACEX\n");
}

//Optional... if this isn't in the trace module a default implementation
//is used which calls the trace functions with the error
void ism_insertSetError(ism_rc_t rc, const char * filename, int where)
{
	fprintf(fp, "SET\n");
}

//Optional... if this isn't in the trace module a default implementation
//is used which calls the trace functions with the error
void ism_insertSetErrorData(ism_rc_t rc, const char * filename, int where, const char * fmt, ...)
{
	fprintf(fp, "SETX\n");
}

//Optional, called whenever properties are updated...
void ism_insertCfgUpdate(ism_prop_t *props)
{
	fprintf(fp, "Cfg updated\n");
}
