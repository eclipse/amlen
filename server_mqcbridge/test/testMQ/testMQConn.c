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

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../server_ismc/src/ismc_p.h"
#include "../src/mqcInternal.h"

static int process_args(int argc, char * * argv);
static int process_config(const char * name,mqcRuleQM_t * pRuleQM);
static void syntax_help(void);
static int allocQMRule(mqcRuleQM_t * pRuleQM);
static int trclvl = -1;
static char * trcfile = NULL;

static char * g_configfile = NULL;
static char * g_configfile_default = "./testMQConnConfig.cfg";


int main(int argc, char * * argv) {
// process arguments to get qmgr, etc.
	mqcRuleQM_t * pRuleQM;

	int rc = 0;
	mqcRule_t * pRule;
	printf("About to process arguments\n");

	rc = process_args(argc, argv);
	if (rc != 0) {
		return 1;
	}
	if (g_configfile == NULL) {
		g_configfile = g_configfile_default;
	}
	printf("About to allocate QM rule\n");

	pRuleQM = mqc_malloc(sizeof(mqcRuleQM_t));
	pRuleQM->index = 1;
	if (pRuleQM == NULL) {
		mqc_allocError("malloc", sizeof(mqcRuleQM_t), errno);
		return 1;
	}
	if (allocQMRule(pRuleQM)) {
		return 1;
	}
	pRuleQM->pQM->pChannelName = (char*) malloc(1024);
	pRule = (mqcRule_t *) malloc(sizeof(mqcRule_t));
	pRuleQM->pRule=pRule;
	printf("About to process configFile %s\n", g_configfile);

	rc = process_config(g_configfile,pRuleQM);
	if (rc != 0) {
		return 1;
	}
//process g_configfile
// create ism structures
// resolve any build issues with linking ism libraries
// call each mqapi connection method once.
// call each mqapi disconnect method once.

	printf("About to connect Publisher\n");
	rc= mqc_connectPublishQM(pRuleQM);
	if (rc != 0) {
		printf("Could not connect Publisher\n");
		return 1;
	}
	printf("About to connect Subscriber\n");
	rc= mqc_connectSubscribeQM(pRuleQM);
	if (rc != 0) {
		printf("Could not connect Subscriber\n");
		return 1;
	}

	printf("Sleeping for 60 seconds to allow verification of connection establishment\n");
	sleep(60);
	printf("About to disconnect Publisher\n");
	rc= mqc_disconnectPublishQM(pRuleQM);
	if (rc != 0) {
		printf("Could not disconnect Publisher\n");
		return 1;
	}
	printf("About to disconnect Subscriber\n");
	rc= mqc_disconnectSubscribeQM(pRuleQM);
	if (rc != 0) {
		printf("Could not disconnect Subscriber\n");
		return 1;
	}

	return 0;
}

static int process_args(int argc, char * * argv) {
	int config_found = 1;
	int i;
	for (i = 1; i < argc; i++) {
		char * argp = argv[i];
		if (argp[0] == '-') {
			if (argp[1] >= '0' && argp[1] <= '9') {
				trclvl = argp[1] - '0';
			} else if (argp[1] == 'h' || argp[1] == 'H' || argp[1] == '?') {
				config_found = 0;
				break;
			} else if (argp[1] == 't') {
				if (i + 1 < argc) {
					argp = argv[++i];
					trcfile = argp;
				}
			} else {
				fprintf(stderr, "Unknown switch found: %s\n", argp);
				config_found = 0;
				break;
			}
		} else {
			if (g_configfile == NULL) {
				g_configfile = argp;
				config_found = 1;
			} else {
				fprintf(stderr, "Extra parameter found: %s\n", argp);
				config_found = 0;
			}
		}
	}

	if (!config_found) {
		syntax_help();
	}
	return 0;
}

/*
 * Syntax help
 */
void syntax_help(void) {
	fprintf(stderr, "mqcbridge  options  config_file\n\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "    -?         = Show this help\n");
	fprintf(stderr, "    -0 ... -9  = Set the trace level. 0=trace off\n");
	fprintf(stderr, "    -t fname   = Set the trace file name\n");
	fprintf(stderr, "\nconfig_file:\n");
	fprintf(stderr,
			"    The config file contains a list of keyword=value pairs of\n");
	fprintf(stderr, "    configuration values.\n");
	exit(1);
}
int process_config(const char * name,mqcRuleQM_t * pRuleQM) {
	FILE * f;
	char linebuf[2048];
	char * line;
	char * keyword;
	char * value;
	char * conname;
	char * hostname;
	char * port;
//	char* hello;
	mqcQM_t * pQM;
	int lineCount = 0;
	int lineLength = 0;

	printf("Entered process config");

	conname = (char*) malloc(265);
	hostname = (char*) malloc(260);
	port = (char*) malloc(5);
	keyword = (char*) malloc(100);
	value = (char*) malloc(1024);
//	linebuf = (char*)malloc(2048);
	pQM = pRuleQM->pQM;
//	printf("About to test pRuleQM and children\n");
//	printf("is pRuleQM Null? %d\n", pRuleQM == NULL);
//	printf("is pRuleQM->pQM Null? %d\n", pRuleQM->pQM == NULL);
//	printf("is pQM after Null? %d\n", pQM == NULL);
//	printf("is pQM-pChannelName Null? %d", (pQM->pChannelName == NULL));

	memset(conname, 0, 265);
	memset(hostname, 0, 260);
	memset(port, 0, 5);
	memset(keyword, 0, 100);
	memset(value, 0, 1024);
//	memset(conname,0,sizeof(conname));

	if (name == NULL)
		return 0;

	f = fopen(name, "rb");
	if (!f) {
		printf("Config file not found: %s\n", name);
		return 1;
	}

	/* Create a subscription descriptor for this topic string      */

	line = fgets(linebuf, sizeof(linebuf), f);
	if (feof(f)) {
		if (lineCount == 0) {
			printf("Error reading configuration file %s. It was empty\n", name);
			exit(EXIT_FAILURE); /* Genuine problem detected. */
		}
	} else if (ferror(f)) {
		printf("read error occured\n");
		exit(EXIT_FAILURE); /* Genuine problem detected. */
	}
	while (line) {
		lineLength = strlen(line);
		if (line[lineLength - 1] == '\n') {
			line[--lineLength] = 0;
		}
		lineCount += 1;
		printf("line = %s .\n", line);
		keyword = strtok(line, "=");
		printf("keyword = %s .\n", keyword);
		value = strtok(NULL, "=");
		printf("value = %s .\n", value);

		if (keyword && keyword[0] != '*' && keyword[0] != '#') {
//			value = ism_common_getToken(more, " =\t\r\n", "\r\n", &more);
			for (int i = 0; keyword[i]; i++) {
				keyword[i] = tolower(keyword[i]);
			}
			if (0 == strcmp(keyword, "host")) {
				strcpy(hostname, value);
				printf("hostname = %s .\n", hostname);
			} else if (0 == strcmp(keyword, "port")) {
				strcpy(port, value);
				printf("port = %s .\n", port);
			} else if (0 == strcmp(keyword, "qm")) {
				if (pQM->pQMName== NULL) {
					pQM->pQMName= (char*) malloc(1024);
				}
				strcpy(pQM->pQMName, value);
				printf("queue manager name = %s .\n", pQM->pQMName);
			} else if (0 == strcmp(keyword, "channel")) {
				if (pQM->pChannelName == NULL) {
					pQM->pChannelName = (char*) malloc(1024);
				}
				strcpy(pQM->pChannelName, value);
				printf("channel name = %s .\n", pQM->pChannelName);
			}
//			else if (0==strcmp()) {
//				char test='a';
//				printf("hello = %s .\n");
//			}
			else if (0 == strcmp(keyword, "username")) {
				if (pQM->pChannelUserName == NULL) {
					pQM->pChannelUserName = (char*) malloc(1024);
				}
				strcpy(pQM->pChannelUserName ,value);
				printf("channel username= %s .\n", pQM->pChannelUserName);
			} else if (0 == strcmp(keyword, "password")) {
				if (pQM->pChannelPassword == NULL) {
					pQM->pChannelPassword = (char*) malloc(1024);
				}
				pQM->pChannelPassword = strcpy(pQM->pChannelPassword,value);
				printf("channel password = %s .\n", pQM->pChannelPassword);
			}
			else {
				printf("Keyword  %s not found\n", keyword);
			}
		}
		line = fgets(linebuf, sizeof(linebuf), f);
	}
	printf("hostname = %s .\n", hostname);
	printf("port = %s .\n", port);
	printf("channel name = %s .\n", pQM->pChannelName);
	sprintf(conname, "%s(%s)", hostname, port);
	printf("conname = %s .\n", conname);
	if (pQM->pConnectionName == NULL) {
		pQM->pConnectionName = (char*) malloc(1024);
	}
	strcpy(pQM->pConnectionName, conname);

	printf("channel username = %s .\n", pQM->pChannelUserName);
	printf("channel password = %s .\n", pQM->pChannelPassword);
	fclose(f);
	printf("finished processing config. \n");
	return 0;
}
/* ********************************************************************
 *
 * String functions
 *
 **********************************************************************/

/*
 * Tokenize a string with separate leading and trailing characters.
 */
char * ism_common_getToken(char * from, const char * leading,
		const char * trailing, char * * more) {
	char * ret;
	if (!from)
		return NULL;
	while (*from && strchr(leading, *from))
		from++;
	if (!*from) {
		if (more)
			*more = NULL;
		return NULL;
	}
	ret = from;
	while (*from && !strchr(trailing, *from))
		from++;
	if (*from) {
		*from = 0;
		if (more)
			*more = from + 1;
	} else {
		if (more)
			*more = NULL;
	}
	return ret;
}
/* Provide wrapper functions for malloc and free to provide a single  */
/* location to inject logging or tracing for memory allocation        */
/* problems.                                                          */

inline void *mqc_malloc(size_t size) {
	void *pTemp = malloc(size);

	/* if (pTemp != NULL) */
	/* { */
	/*   __sync_fetch_and_add(&mqcMQConnectivity.currentAllocatedMemory, malloc_usable_size(pTemp)); */
	/* } */
	return pTemp;
}

inline void mqc_free(void *ptr) {
	/* __sync_fetch_and_sub(&mqcMQConnectivity.currentAllocatedMemory, malloc_usable_size(ptr)); */
	free(ptr);
}
int mqc_allocError(char * api, int len, int syserrno) {
	printf("Could not allocate object/n");
	return 1;
}
int allocQMRule(mqcRuleQM_t * pRuleQM) {
	/* config */
	char * pDescription = (char*) malloc(1024);
	char * pQMName = (char*) malloc(40);
	char * pChannelUserName = (char*) malloc(1024);;
	char * pChannelPassword = (char*) malloc(1024);
	char * pConnectionName = (char*) malloc(1024);
	char * pChannelName = (char*) malloc(1024);
	char * pSSLCipherSpec = (char*) malloc(1024);
	/* internal */
	struct ismc_manrec_t indexRecord;
	mqcQM_t * pQM;

	if (pRuleQM->pQM == NULL) {
		pRuleQM->pQM = mqc_malloc(sizeof(mqcQM_t));
	}
	if (pRuleQM->pQM == NULL) {
		mqc_allocError("malloc", sizeof(mqcQM_t), errno);
		return 1;
	}
	pQM = pRuleQM->pQM;

	pQM->BatchSize = 1;
	pQM->pConnectionName = pConnectionName;
	pQM->pChannelName = pChannelName;
	pQM->flags = 0;
	pQM->index = 0;
	pQM->indexRecord = indexRecord;
	pQM->lengthOfQMName = 0;
	pQM->pDescription = pDescription;
	pQM->pName = "testQM";
	pQM->pNext = NULL;
	pQM->pQMName = pQMName;
	pQM->pSSLCipherSpec = pSSLCipherSpec;
	pQM->platformType = 3;
	pQM->recoveryTimestamp = 0;
//	pQM->xa_recoverMutex = xa_recoverMutex;
	pQM->pChannelUserName = pChannelUserName;
	pQM->pChannelPassword = pChannelPassword;

	return 0;
}
