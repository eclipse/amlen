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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

static int ism_store_unlink(const char *name);
static int ism_store_access(const char *name, const char *mode);
static int ism_store_mkdir(const char *name, const char *mode);

/*
 * Common file remove with UTF-8 filename
 */
static int ism_store_unlink(const char *name)
{
   int rc=0;
   rc = unlink(name);
   return rc;
}

/*
 * Common access with a UTF-8 file name
 */
static int ism_store_access(const char *name, const char *mode)
{
   int imode = 0;

   char * uname;
   if (strchr(mode, 'x'))
      imode |= X_OK;
   if (strchr(mode, 'r'))
      imode |= R_OK;
   if (strchr(mode, 'w'))
      imode |= W_OK;
    
   uname = (char *)name;
    
   return access(uname, imode);
}

/*
 * Common mkdir with a UTF-8 file name
 */
static int ism_store_mkdir(const char *name, const char *mode)
{
   char *uname;
   int imode = 0775;
   int unixrc;

   if (mode)
   {
      imode = strtoul(mode, NULL, 8);
   }
    
   uname = (char *)name;
    
   unixrc = mkdir(uname, imode);
   if (unixrc == 0)
   {
      // chmod to ensure the user's umask doesn't prevent creation with correct mode
      (void)chmod(uname, imode);
   }
   return unixrc;
}

/*
* Check and set the store lock.
*/
int ism_store_checkStoreLock(const char *datadir, const char *filename, int *filelock_fd, mode_t file_mode)
{
   struct flock filelock;
   char *lockname = alloca(strlen(datadir) + strlen(filename) + 16);
   int rc=0;
   int fd;
   char pidstr[1024];

   filelock.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
   filelock.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
   filelock.l_start  = 0;        /* Offset from l_whence         */
   filelock.l_len    = 0;        /* length, 0 = to EOF           */
   filelock.l_pid    = getpid();
	
   if (ism_store_access(datadir, "rwx"))
   {
      rc = ism_store_mkdir(datadir, "777");
      if (rc==-1)
      {
         TRACE(1, "Failed to create the store lock file directory %s. Error: (code=%d) %s\n", datadir, errno, strerror(errno));
         return -1;
      }
      else
      {
         TRACE(5, "The store lock file directory %s is created.\n", datadir);
      }
   }
	
   if (rc!=-1)
   {
      // Construct the name of the lock file
      strcpy(lockname, datadir);
      strcat(lockname, filename);

      if ((fd = open(lockname, O_WRONLY|O_CREAT|O_CLOEXEC, file_mode)) == -1)
      {
         if (errno != EEXIST)
         {
            TRACE(1,"Failed to open the store lock file %s. Error: (code=%d) %s\n", lockname, errno, strerror(errno));
            rc = -1;
         }
      }
   }

   if (rc!=-1)
   {
      *filelock_fd = fd;
      if (fcntl(*filelock_fd, F_SETLK, &filelock) == -1)
      {
         if (errno == EACCES || errno == EAGAIN)
         {
            TRACE(1,"The store lock file %s is locked by another process.\n", lockname);
         }
         else
         {
            TRACE(1,"Failed to lock the store lock file %s. Error: (code=%d) %s\n", lockname, errno, strerror(errno));
         }
         rc =-1;
      }
   }

   if (rc!=-1)
   {
      sprintf(pidstr, "%d", getpid());
      int len = write(fd, &pidstr, strlen(pidstr));
      TRACE(5,"The store lock file %s is locked (fd=%d). Written %d bytes\n", lockname, *filelock_fd,len);
   }
   else
   {
      TRACE(1,"Failed to lock the store lock file %s. Error: (code=%d) %s\n", lockname, errno, strerror(errno));
      if (filelock_fd!=NULL)
      {
         *filelock_fd = -1;
      }
   }
   		
   return rc;
}

int ism_store_closeLockFile(const char *datadir, const char *filename, const int filelock_fd)
{
   struct flock fl;
   int rc=0;
   char *lockname = alloca(strlen(datadir) + strlen(filename) + 16);
	 
   strcpy(lockname, datadir);
   strcat(lockname, filename);
 	
   /*Unlock the file*/
   fl.l_type = F_UNLCK;
   fl.l_whence = SEEK_SET;
   fl.l_start = 0;
   fl.l_len = 0;
   fl.l_pid    = getpid();
   if (fcntl(filelock_fd, F_SETLK, &fl) == -1)
   {
      TRACE(1,"Failed to unlock the store lock file %s (fd=%d).  Error: (code=%d) %s\n", lockname, filelock_fd, errno, strerror(errno));
   }
   rc = close(filelock_fd);
	
   if (rc==-1)
   {
      TRACE(1,"Failed to close the store lock file %s. Error: (code=%d) %s\n", lockname, errno, strerror(errno));
   }
   else
   {
      TRACE(5,"The store lock file %s is closed (fd=%d).\n", lockname, filelock_fd);
   }
    
   return rc;
}

int ism_store_removeLockFile(const char *datadir, const char *filename, const int filelock_fd)
{
   int rc=0;
   char *lockname = alloca(strlen(datadir) + strlen(filename) + 16);

   strcpy(lockname, datadir);
   strcat(lockname, filename);

   ism_store_closeLockFile(datadir, filename, filelock_fd);
   // Remove the file
   rc = ism_store_unlink(lockname);
   if (rc==-1)
   {
      TRACE(1,"Failed to unlink the store lock file %s. Error: (code=%d) %s\n", lockname, errno, strerror(errno));
   }
   else
   {
      TRACE(5,"The store lock file %s is removed\n", lockname);
   }

   return rc;
}
