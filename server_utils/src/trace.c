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

#include <trace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

static int stopWork;
static ism_threadh_t workThread;
extern pthread_mutex_t trc_lock;
extern char * ism_common_getTraceBackupDestination();
extern int ism_common_getTraceBackup();
extern int ism_common_getTraceBackupCount();

/* Maximum size of the backlog (to-do list) */
#define WORK_TABLE_SIZE      100

/* Maximum number of offloading errors before moving the file to "failed" list */
#define MAX_ERROR_COUNT      10

#define COMPRESSED_SUFFIX    ".gz"

/* Types of actions for trace processor */
typedef enum {
    imaTRC_None              = 0,
    imaTRC_Compress          = 1,
    imaTRC_SendOverSSH       = 2,
    imaTRC_SendOverFTP       = 3
} imaTraceProcessorActionType_t;

static int errorCount = 0;

static const int WAIT_ON_FAILURE = 1000000;        // Wait time in usec
static const int SLEEP_FREQUENCY = 10;             // How many errors to skip before pausing

ism_common_list * ism_trace_work_table = NULL;  /* List of pending work */
static pthread_mutex_t * workTableLock;
static pthread_cond_t * workAvailable;
static int maxWorkTableSize = WORK_TABLE_SIZE;

static void * processWork(void * arg, void * context, int value);
static int runProcessTrace(imaTraceProcessorActionType_t action,
        const char * fileName, int maxFiles, const char * destination, const char * move);
static void removeFile(const char * fileName);

/**
 * Invokes the work thread for processing trace file backups.
 */
XAPI void ism_trace_startWorker(void) {
    stopWork = 0;

    workTableLock = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,21),sizeof(pthread_mutex_t));
    workAvailable = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,22),sizeof(pthread_cond_t));

    pthread_mutex_init(workTableLock, NULL);
    pthread_cond_init(workAvailable, NULL);

    ism_trace_work_table = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,23),1, sizeof(ism_common_list));
    ism_common_list_init(ism_trace_work_table, 0, NULL);

    ism_common_startThread(&workThread, processWork, NULL, NULL, 0, ISM_TUSAGE_NORMAL, 0, "trcwork",
            "Trace compressor-offloader");
}

/**
 * Stops the work thread for processing trace file backups.
 */
void ism_trace_stopWorker(void) {
    stopWork = 1;

    pthread_mutex_lock(workTableLock);
    pthread_cond_signal(workAvailable);
    pthread_mutex_unlock(workTableLock);

    ism_common_joinThread(workThread, NULL);

    pthread_mutex_destroy(workTableLock);
    pthread_cond_destroy(workAvailable);
    ism_common_free(ism_memory_utils_misc,workTableLock);
    ism_common_free(ism_memory_utils_misc,workAvailable);
    ism_common_free(ism_memory_utils_misc,ism_trace_work_table);
}

/**
 * Work thread that processes incoming files.
 * @param arg        Not used
 * @param context    Not used
 * @param value     Not used
 * @return NULL
 */
void * processWork(void * arg, void * context, int value) {
    ism_trace_work_entry_t * entry;
    int rc;
    ism_common_list * failedFiles = NULL;  /* List of files that failed to transfer */
    char * fileName;

    failedFiles = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,27),1, sizeof(ism_common_list));
    ism_common_list_init(failedFiles, 1, NULL);

    int maxFiles = 0;
    char * traceDestination = NULL;
    imaTraceProcessorActionType_t action;

    while (!stopWork) {
        pthread_mutex_lock(workTableLock);
        while (ism_common_list_size(ism_trace_work_table) == 0) {
            pthread_cond_wait(workAvailable, workTableLock);
            if(stopWork){
                break;
            }
        }

        maxFiles = ism_common_getTraceBackupCount();

        action = imaTRC_None;

        int backupMode;

        pthread_mutex_lock(&trc_lock);
        backupMode = ism_common_getTraceBackup();
        pthread_mutex_unlock(&trc_lock);

        switch (backupMode) {
        case 1:
            action = imaTRC_Compress;
            break;
        case 2:
        {
            /*
             * If SCP or FTP destination is specified, use it.
             * Otherwise default to compressing files only.
             */
            action = imaTRC_Compress;

            if (traceDestination) {
                ism_common_free(ism_memory_utils_misc,traceDestination);
            }
            traceDestination = ism_common_getTraceBackupDestination();

            if (traceDestination) {
                if (strstr(traceDestination, "scp://") == traceDestination) {
                    action = imaTRC_SendOverSSH;
                } else if (strstr(traceDestination, "ftp://") == traceDestination) {
                    action = imaTRC_SendOverFTP;
                }
            }

            break;
        }
        case 0:
            /* BEAM suppression: fall thru */
        default:
            action = imaTRC_None;
            break;
        }

        rc = ism_common_list_remove_head(ism_trace_work_table, (void **)&entry);
        pthread_mutex_unlock(workTableLock);
        if (rc == 0) {
            if (entry->type == 1) {
                // Ignore, not supported
            } else {
                /*
                 * This is a trace file.
                 *
                 * If retry count exceeds the threshold, just compress the file
                 * and try to transfer it when the next transfer succeeds.
                 *
                 * If action is compress or none, reset the total error count.
                 */
                if (action <= imaTRC_Compress) {
                    errorCount = 0;
                }

                if (action == imaTRC_None) {
                    ism_common_free(ism_memory_utils_misc,entry->fileName);
                    ism_common_free(ism_memory_utils_misc,entry);

                    continue;
                }

                if (entry->retryCount >= MAX_ERROR_COUNT) {
                    char * compressedFile = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,31),strlen(entry->fileName) + sizeof(COMPRESSED_SUFFIX));

                    action = imaTRC_Compress;
                    sprintf(compressedFile, "%s"COMPRESSED_SUFFIX, entry->fileName);
                    TRACE(5, "Adding %s to the list of files we failed to offload\n", compressedFile);
                    ism_common_list_insert_tail(failedFiles, compressedFile);
                    if (ism_common_list_size(failedFiles) > maxFiles) {
                        if (ism_common_list_remove_head(failedFiles, (void **)&fileName) == 0) {
                            TRACE(5, "The list of files we failed to offload is too long, dropping %s\n", fileName);
                            removeFile(fileName);
                            ism_common_free(ism_memory_utils_misc,fileName);
                        }
                    }

                }

                rc = runProcessTrace(action, entry->fileName,
                        maxFiles, traceDestination?traceDestination:"", NULL);
                if (action > imaTRC_Compress) {
                    if (rc) {
                        // Transfer failed, increment the error count and schedule for retry - wait 1 second
                        errorCount++;
                        entry->retryCount++;
                        TRACE(1, "Transfer failed for %s, increase the retry count to %d, total entries: %d, total errors: %d\n",
                                entry->fileName, entry->retryCount, ism_common_list_size(ism_trace_work_table), errorCount);

                        pthread_mutex_lock(workTableLock);
                        ism_common_list_insert_tail(ism_trace_work_table, entry);
                        pthread_mutex_unlock(workTableLock);

                        // Pause every 10 errors
                        if (errorCount % SLEEP_FREQUENCY == 0) {
                            ism_common_sleep(WAIT_ON_FAILURE);
                        }

                        continue;
                    } else {
                        /*
                         * Transfer succeeded, clear error counts for all existing entries and
                         * try to transfer all files that failed to transfer before and are now
                         * compressed.
                         */
                        ism_common_listIterator iter;

                        ism_common_free(ism_memory_utils_misc,entry->fileName);
                        ism_common_free(ism_memory_utils_misc,entry);

                        TRACE(5, "Transfer succeeded, reset retry and error counts and resend any failed files\n");

                        errorCount = 0;

                        pthread_mutex_lock(workTableLock);
                        ism_common_list_iter_init(&iter, ism_trace_work_table);

                        while(ism_common_list_iter_hasNext(&iter)) {
                            ism_common_list_node * node = ism_common_list_iter_next(&iter);

                            entry = (ism_trace_work_entry_t *)node->data;
                            TRACE(8, "Resetting retry count for %s\n", entry->fileName);
                            entry->retryCount = 0;
                        }
                        ism_common_list_iter_destroy(&iter);
                        pthread_mutex_unlock(workTableLock);

                        rc = ism_common_list_remove_head(failedFiles, (void **)&fileName);
                        if (rc == 0) {
                            rc = runProcessTrace(action, entry->fileName,
                                    maxFiles, traceDestination?traceDestination:"", "move");
                            TRACE(8, "Resent %s: rc=%d\n", fileName, rc);
                            ism_common_free(ism_memory_utils_misc,fileName);
                        }
                    }
                } else {
                    ism_common_free(ism_memory_utils_misc,entry->fileName);
                    ism_common_free(ism_memory_utils_misc,entry);
                }
            }
        }
    };
    return NULL;
}

/**
 * Removes the file by name. If the name is a symlink,
 * delete the actual file too.
 * @param fileName - The name of the file to remove.
 */
void removeFile(const char * fileName) {
    struct stat buf;

    if (lstat(fileName, &buf) == 0) {
        if (S_ISLNK(buf.st_mode)) {
            char * name = alloca(buf.st_size + 1);
            ssize_t cnt = readlink(fileName, name, buf.st_size + 1);
            if (cnt > 0) {
                name[buf.st_size] = 0;
                remove(name);
            }
        }
        remove(fileName);
        TRACE(1, "Deleted file %s\n", fileName);
    }
}

int runProcessTrace(imaTraceProcessorActionType_t action,
        const char * fileName, int maxFiles, const char * destination, const char * move) {
    TRACE(5, "Executing process_trace script: %d %s %d %s %s\n",
            action, fileName, maxFiles, destination, (move != NULL)?move:"");

    char buf1[10];
    char buf2[10];
    int st;
    pid_t pid;
    sprintf(buf1, "%d", action);
    sprintf(buf2, "%d", maxFiles);

    pid = vfork();
    if (pid < 0) {
        TRACE(1, "Could not vfork process to run the script\n");
        return 1;
    }
    if (pid == 0) {
        execl(IMA_SVR_INSTALL_PATH "/bin/process_trace.sh", "process_trace.sh",
                buf1, fileName, buf2, destination, move, NULL);
        int urc = errno;
        TRACE(1, "Unable to run process_trace.sh: errno=%d error=%s\n", urc, strerror(urc));
        _exit(1);
    }
    waitpid(pid, &st, 0);
    int rc = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

    TRACE(8, "Executed process_trace script: %d %s %d %s %s with return code %d\n",
            action, fileName, maxFiles, destination, (move != NULL)?move:"", rc);

    return rc;
}

/**
 * Add an entry (with filename) to the trace work list.
 *
 * @param entry A pointer to the new work list entry.
 */
void ism_trace_add_work(ism_trace_work_entry_t * entry) {
    pthread_mutex_lock(workTableLock);
    ism_common_list_insert_tail(ism_trace_work_table, entry);
    if (ism_common_list_size(ism_trace_work_table) > maxWorkTableSize) {
        if (ism_common_list_remove_head(ism_trace_work_table, (void **)&entry) == 0) {
            /*
             * Remove file and free up the entry.
             * Check for symlink first and remove the actual file too.
             */
            removeFile(entry->fileName);
            ism_common_free(ism_memory_utils_misc,entry->fileName);
            ism_common_free(ism_memory_utils_misc,entry);
        }
    }

    pthread_cond_signal(workAvailable);
    pthread_mutex_unlock(workTableLock);
}

