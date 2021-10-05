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
#include "storeLockFile.h"
#include <assert.h>
#include <signal.h>
#include <sys/wait.h>

#define FILECREATION_COUNT 5

#include "stubFuncs.c"

int test_failed=0;
int child_done=0;
int main (void)
{ 
  int rc = 0;
  int filelock_fd=-1;
  int filelock_fd2=-1;
  char *datadir = "/tmp";
  char *filename = "unittest_server1";
  
  char filename_buf[1024];
  int count=0;
  int status=0;
  
  
  pid_t childpid; /* variable to store the child's pid */
  
  
  rc = ism_store_checkStoreLock(datadir, filename, &filelock_fd, 00644);
  if(filelock_fd==-1 || rc ==-1){
    //Fail the test
    printf("The creation of the store lock file failed.\n");
    assert(0);
  }
  rc= ism_store_removeLockFile(datadir, filename, filelock_fd);
  if(rc==-1){
    printf("Failed to remove the store lock file.\n");
    assert(0);
  }
  
  /*Test Creating multiple lock store files with same name*/
  for(count=0; count< FILECREATION_COUNT; count++)
  {
    /* now create new process */
      childpid = fork();
      
      if (childpid >= 0) /* fork succeeded */
      {
        /*Use child to create the file with the same name as parent. 
        Do the check at the parent. */
          if (childpid == 0) /* fork() returns 0 to the child process */
          {
            printf("\tChild Process: %d\n", getpid());
            
                
        rc = ism_store_checkStoreLock(datadir, filename, &filelock_fd, 00644);
        if(rc ==-1){
          //Fail the test
          printf("The creation of the store lock file failed.\n");
          assert(0);
        } 
        
        child_done=1;
        
        /*Sleep more than parent to give the parent the chance to 
        create the file and verify the result*/
        sleep(4);
        
              exit(0); /* child exits with user-provided return code */
          }
          else /* fork() returns new pid to the parent process */
          {
            printf("\tParent Process: %d\n", getpid());
              /*Sleep so the child have the chance to create the file.*/
              sleep(2);
        rc = ism_store_checkStoreLock(datadir, filename, &filelock_fd2, 00644);
        if(rc !=-1){
          //Fail the test
          printf("The creation of the store lock supposed to fail.\n");
          assert(0);
        }
        rc = ism_store_removeLockFile(datadir, filename, filelock_fd);
        if(rc==-1){
          printf("Failed to remove the store lock file.\n");
          assert(0);
        }
            
            wait(&status);
                  
          }
      }
  }
  
  
  
  /*Test Creating multiple lock store files*/
  for(count=0; count< FILECREATION_COUNT; count++)
  {
    sprintf(filename_buf, "unittest_server%d", count);
    rc = ism_store_checkStoreLock(datadir,(const char *) &filename_buf, &filelock_fd, 00644);
    if(filelock_fd==-1 || rc ==-1){
      //Fail the test
      printf("The creation of the store lock file failed");
      assert(0);
    }
    
    rc = ism_store_removeLockFile(datadir,(const char*) &filename_buf, filelock_fd);
    if(rc==-1){
      printf("Failed to remove the store lock file.\n");
      assert(0);
    }
  }
  
  printf("test passed.\n");
  return 0;
  
}
