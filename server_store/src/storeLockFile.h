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

#include <ismutil.h>

/*
* Check and set the store lock.
*/
int ism_store_checkStoreLock(const char *datadir, const char *filename, int *filelock_fd, mode_t file_mode);

/*
* Unlink or remove the store lock file
*/
int ism_store_removeLockFile(const char *datadir, const char *filename, const int filelock_fd);

/**
* Close the store lock file. This will not remove the file from the file system.
*/
int ism_store_closeLockFile(const char *datadir, const char *filename, const int filelock_fd);
