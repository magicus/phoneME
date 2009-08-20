/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sched.h>
#include <shared_memory_md.h>

/*  Directory for Temporary Lock files */
static char* tempDirLocation = "/tmp/";
static char* shmemPrefix = "shmem_lock_";

/************************ Helper functions ***************************/
static char *sharedMemory_createFullName(char* name) {
    char *shmem_key_name = malloc(strlen(tempDirLocation) + strlen(shmemPrefix) + strlen(name)+1);
    strcpy(shmem_key_name, tempDirLocation);
    strcat(shmem_key_name, shmemPrefix);
    strcat(shmem_key_name, name);
    return shmem_key_name;
}
static void sharedMemory_destroyFullName(char* name) {
    if (name != NULL) {
        free(name);
    }
}

static key_t sharedMemory_createKey(char* name) {
    int fd;
    key_t key;
    
    fd = open(name, O_CREAT);
    if (fd != -1) {
        close(fd);
    }
    key = ftok(name, 'a'); 
    if (key == -1) {
        perror("IPC error: ftok"); exit(1);        
    }
    return key; 
}

static key_t sharedMemory_getKey(char* name) {
    return ftok(name, 'a'); 
}

static void sharedMemory_removeKey(char* name) {
    remove(name);
}

/************************ Platform implementation ***************************/

/* create a new file mapping and open the shared memory */
int sharedMemory_create (SharedMemory_md* sm, char* name, int size){
    int shmid, semid; 
    struct shmid_ds shmds; 
    int mode = IPC_CREAT |0777;

    sm->full_name = sharedMemory_createFullName(name);
    if (sm->full_name == NULL) {
        return -1;
    }
    sm->key = sharedMemory_createKey(sm->full_name);
    if ((shmid = shmget(sm->key, size + sizeof(int), IPC_CREAT )) == -1){
        return -1;
    } 
    if ((shmctl(shmid, IPC_STAT, &shmds)) == -1){
        return -1;
    } 

    shmds.shm_perm.mode =  shmds.shm_perm.mode | O_RDWR;

    sm->dataBuffer = (char *)shmat(shmid, NULL, 0);
    *((int *)sm->dataBuffer) = size;
    sm->shmid = shmid;

    /* create semaphore to access shared memory */
    if ((semid = semget(sm->key, 1, mode)) != -1)
    {
        union semun {
                    int val;
                    struct semid_ds *buf;
                    unsigned short  *array;
        } arg;
        arg.val = 0;
        semctl( semid, 0, SETVAL, arg);
        sm->semid = semid;
    } else {
        return -1;
    }
    return 0; 
}  


/* 
 * maps the shared buffer to a *char and init the function pointers 
 * of the sharedMemory struct 
 */ 
int sharedMemory_open(SharedMemory_md* sm, char *name)  {
    int shmid, semid; 
    int size=10;
    struct shmid_ds shmds; 
    int mode = 0777;
    char *full_name;
    struct sembuf sbuf = {0,1,0};
    sm->full_name = NULL;
    full_name = sharedMemory_createFullName(name);
    if (full_name == NULL) {
        return -1;
    }
    
    sm->key = sharedMemory_getKey(full_name);
    sharedMemory_destroyFullName(full_name);
    
    if ((shmid = shmget(sm->key, 0, 0)) == -1) {
        return -1;
    } 
    
    if ((shmctl(shmid, IPC_STAT, &shmds)) == -1) {
        return -1;
    } 

    shmds.shm_perm.mode =  shmds.shm_perm.mode | O_RDWR;
    sm->shmid = shmid;

    sm->dataBuffer = (char *)shmat(shmid, NULL, 0);
    
    /* create semaphore to access shared memory */
    if ((semid = semget(sm->key, 1, mode)) != -1){
        sm->semid = semid;
    } else {
        return -1;
    }
    return 0; 
}

/* Purpose: Get total size of shared memory
   Returns: Size of shared memory
*/
int sharedMemory_getSize(SharedMemory_md *sm) {
    return *((int *)sm->dataBuffer);
}

/* Purpose: Get total size of shared memory
   Returns: Size of shared memory
*/
char *sharedMemory_getAddress(SharedMemory_md *sm) {
    return sm->dataBuffer + sizeof(int);
}

/* Purpose: Close shared memory. 
   Returns: none
*/
void sharedMemory_close(SharedMemory_md *sm) {
    if (sm->dataBuffer != NULL) {
        shmdt(sm->dataBuffer); 
        close(sm->shmid);
    }
    
    if (sm->full_name) {
        free(sm->full_name);
    }
    free(sm); 
}

/* Purpose: Destroy shared memory. 
   Returns: none
*/
void sharedMemory_destroy(SharedMemory_md *sm) {

    if (sm->dataBuffer != NULL) {
        shmdt(sm->dataBuffer); 
        shmctl(sm->shmid, IPC_RMID, NULL);
        close(sm->shmid);
        semctl(sm->semid, 0, IPC_RMID);
    }
    
    if (sm->full_name) {
        sharedMemory_removeKey(sm->full_name);
        sharedMemory_destroyFullName(sm->full_name);
    }
    
    free(sm); 
}

/* Purpose: Lock shared memory access. 
   Returns: none
*/
void sharedMemory_lock(SharedMemory_md *sm) {
    struct sembuf sbuf[2] = {{0,0,0},{0,1,0}} ;
    semop(sm->semid, &sbuf[0], 2);
}

/* Purpose: Unlock shared memory access. 
   Returns: none
*/
void sharedMemory_unlock(SharedMemory_md *sm) {
    struct sembuf sbuf[1] = {0,-1,0};
    semop(sm->semid, &sbuf[0], 1);
    sched_yield();
}
