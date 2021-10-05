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

#ifndef __TRACE_DEFINED
#define __TRACE_DEFINED

#include <ismutil.h>

/**
 * Default maximum number of trace files
 */
#define MAX_TRACE_FILES 3

extern ism_common_list * ism_trace_work_table;

typedef struct ism_trace_work_entry_t {
	int    type;             // 0 - trace, 1 - log
	char * fileName;
	int    retryCount;
} ism_trace_work_entry_t;

/**
 * Invokes the work thread for processing trace file backups.
 */
extern void ism_trace_startWorker(void);

/**
 * Stops the work thread for processing trace file backups.
 */
extern void ism_trace_stopWorker(void);

/**
 * Add an entry (with filename) to the trace work list.
 *
 * @param entry A pointer to the new work list entry.
 */
extern void ism_trace_add_work(ism_trace_work_entry_t * entry);

#endif
