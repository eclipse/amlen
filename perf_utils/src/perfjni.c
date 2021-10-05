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
#include <stdarg.h>
#include <stdio.h>
#include <sched.h>
#include <pthread.h>
#include <string.h>
#include <jni.h>
#include <ismutil.h>

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
  ism_common_initUtil();
  return JNI_VERSION_1_4;
}

/* Set thread affinity from JMSBenchTest
 * For each byte in the processor map, if the byte is non-zero the associated processor
 * is available for scheduling this thread.
 * @param cpu_id  The CPU to bind the current thread to
 */
JNIEXPORT void JNICALL Java_JMSBenchTest_setAffinity(JNIEnv *env, jclass clazz, jint cpu_id) {
	int   i;
	char cpus[128];
	cpu_set_t cpu_set;

	CPU_ZERO(&cpu_set);
	pthread_t tid = pthread_self();

	/* Set affinity of this thread */
	memset(cpus, 0, sizeof(cpus));
	if (cpu_id < sizeof(cpus))
		cpus[cpu_id] = 1;

	for (i=0; i<sizeof(cpus); i++) {
	    if (cpus[i])
	        CPU_SET(i, &cpu_set);
	}
	pthread_setaffinity_np(tid, sizeof(cpu_set), &cpu_set);
}

JNIEXPORT jdouble JNICALL Java_JMSBenchTest_readTSC(JNIEnv *env, jclass clazz) {
	return ism_common_readTSC();
}
