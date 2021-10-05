/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#define MAX_LEN 32
struct region {        /* Defines "structure" of shared memory */
    int len;
    char buf[MAX_LEN];
};
struct region *rptr;
int fd;


main(){
/* Create shared memory object and set its size */
long counter=0;

fd = shm_open("/myregion2", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
if (fd == -1)
    /* Handle error */;


if (ftruncate(fd, sizeof(struct region)) == -1)
    /* Handle error */;


/* Map shared memory object */


rptr = mmap(NULL, sizeof(struct region),
       PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
if (rptr == MAP_FAILED)
    /* Handle error */;

counter=1;
while(counter<10000000) {
printf("read %llu\n", *((long long*)rptr->buf)) ;
((long long*)rptr->buf)[0]=counter;
printf("set %llu\n", *((long long*)rptr->buf)) ;
counter=counter+1;
printf("expect not = %d\n", counter);
while ( *((long long*)rptr->buf) < counter) {
    printf(".");
    sleep(1);
}
counter=counter+1;
}

printf("done.");



}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#define MAX_LEN 32
struct region {        /* Defines "structure" of shared memory */
    int len;
    char buf[MAX_LEN];
};
struct region *rptr;
int fd;


main(){
/* Create shared memory object and set its size */
long counter=0;
shm_unlink("/myregion2");

fd = shm_open("/myregion2", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
if (fd == -1)
    /* Handle error */;


if (ftruncate(fd, sizeof(struct region)) == -1)
    /* Handle error */;


/* Map shared memory object */


rptr = mmap(NULL, sizeof(struct region),
       PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
if (rptr == MAP_FAILED)
    /* Handle error */;

printf("read %llu\n", *((long long*)rptr->buf)) ;
counter=1;
while(counter<10000000) {
printf("expect not = %d\n", counter);
while ( *((long long*)rptr->buf) < counter) {
    printf(".");
    sleep(1);
}

counter=counter+1;
printf("read %llu\n", *((long long*)rptr->buf)) ;
((long long*)rptr->buf)[0]=counter;
printf("set %llu\n", *((long long*)rptr->buf)) ;
counter=counter+1;
}

printf("now %llu\n", *((long long*)rptr->buf)) ;

printf("done\n");

shm_unlink("/myregion2");

}
