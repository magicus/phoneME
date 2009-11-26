/*
 * @(#)timestamp.c	1.11 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */

#include "javavm/include/defs.h"
#include "javavm/include/globals.h"
#include "javavm/include/timestamp.h"
#include "javavm/include/utils.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/porting/time.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/ansi/time.h"

/*
 * This is a generic linked list implementation. It is used to store
 * the time-stamps records. 
 */
typedef struct ListType {
    struct ListNode* headNodePtr;
    struct ListNode* tailNodePtr;
    int numberOfElements;
} List;

typedef struct  ListIteratorType{
    struct ListNode* currentNodePtr;
} ListIterator;

typedef struct ListNode {
    void* dataPtr;
    struct ListNode* nextNodePtr;
} ListNode;

List* timeStampList = NULL;


/*
 * P.S. The implementation of list if not multi-thread safe. 
 * Caller of this should take care to lock the list if required.
 * List implemenation starts here!
 */

/*
 * Initializes the list pointers. The list has two node pointers
 * one points to the head of the list and other is the end of 
 * the list. Also keep a count on the number of elements in the 
 * list.
 */
static void 
listInit(List* list) 
{  
    CVMassert(list != NULL);
    list->headNodePtr = NULL;
    list->tailNodePtr = NULL;
    list->numberOfElements = 0;
}

/*
 * Destroys the list and nodes. 
 * Traverse the list and free the memory allocated by the
 * nodes and memory allocated by the data within the nodes.
 * This is done on the assumption that the contents of each
 * list node, passed through listAddLast() are still 
 * malloc and supposed to be free'ed as we free the nodes.
 */
static void
listAndNodesDestroy(List* list)
{
    ListNode* temp;
    while (list->headNodePtr != NULL) {
        CVMassert(list->numberOfElements > 0);
        CVMassert((list->numberOfElements != 1) ||
                  (list->headNodePtr == list->tailNodePtr));
        temp = list->headNodePtr;
        list->headNodePtr = temp->nextNodePtr;
        temp->nextNodePtr = NULL;
        free(temp->dataPtr);
        temp->dataPtr = NULL;
        free(temp);
        list->numberOfElements--;
    }
    CVMassert(list->numberOfElements == 0);
    listInit(list);
}


/*
 * Inserts an element at the end of the list
 */
static CVMBool 
listAddLast(CVMExecEnv* ee, List* list, void* newDataPtr) 
{
    ListNode* newNodePtr = (ListNode*) malloc(sizeof(ListNode));
    if (newNodePtr == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
        return CVM_FALSE; 
    }
    newNodePtr->dataPtr = newDataPtr;
    newNodePtr->nextNodePtr = NULL;

    if (list->headNodePtr == NULL) {
        CVMassert(list->tailNodePtr == NULL);
        CVMassert(list->numberOfElements == 0);
        list->headNodePtr = newNodePtr;
    }
    else {
        CVMassert(list->tailNodePtr != NULL);
        CVMassert(list->numberOfElements > 0);
        list->tailNodePtr->nextNodePtr = newNodePtr;
    }
    list->tailNodePtr = newNodePtr;
    list->numberOfElements++;
    return CVM_TRUE; 
}
 
/*
 * Returns the pointer to the list itereator
 */
static ListIterator* 
listGetIterator(CVMExecEnv* ee, List* list) {
    ListIterator* iterator = (ListIterator*) malloc(sizeof(ListIterator));
    if (iterator == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
        return NULL;
    }
    CVMassert(list != NULL);
    iterator->currentNodePtr = list->headNodePtr;
    return iterator;
}

/*
 * Iterates the list from the beginning towards the end.
 * Returns the pointer to the data element for the current
 * node and resets the pointer to the current node.
 */
static void*
listNext(ListIterator* iterator) 
{
    void* retValPtr;
    CVMassert(iterator != NULL);
    if (iterator->currentNodePtr == NULL) {
        return NULL;
    }
    retValPtr = iterator->currentNodePtr->dataPtr;
    iterator->currentNodePtr = iterator->currentNodePtr->nextNodePtr;
    return retValPtr;
}

/* Utility functions to record the time stamp in the beginning 
 * and end and print the recorded time stamps
 */

/*
 * this struct keeps the timestamps records and the corresponding locations
 */
typedef struct timeStampRecords {
    char* loc;
    int position;
    int cpuTime; /* time in milli-second */
    long wallClockTime; /* time in milli-second */
} timeStampRecords;

/* This function records the CPU time at the location (str),
 * passed an argument and updates the timestampList.
 * To prevent the timestampList to be corrputed by another  
 * thread we put the lock on the list, and after the list is  
 * updated, we unlock the list. 
 */
static long getWallClkTime();

static CVMBool
timeStampRecord(CVMExecEnv* ee, 
		const char* str, int position, long wallClkTime)
{
    timeStampRecords* valPtr;
    valPtr = (timeStampRecords*) malloc(sizeof(timeStampRecords));
    if (valPtr == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
        return CVM_FALSE;
    }  
    /* if the location string passes is null then we will
     * keep the timeStampRecords->loc as null and while printing the
     * the time stamps, will put auto count.
     */
    if (str != NULL) {
        valPtr->loc = strdup(str);
        if (valPtr->loc == NULL) {
            free(valPtr);
            return CVM_FALSE;
        }
    }
    else {
        valPtr->loc = NULL;
    }
    valPtr->position = position;
    valPtr->cpuTime = clock();
    /* convert the time from micro-sec to milli-sec */
    valPtr->cpuTime /= 1000;
    valPtr->wallClockTime = wallClkTime;
    
    CVMsysMutexLock(ee, &CVMglobals.timestampListLock);
    /* insert the timeStamp record in the list */
    CVMassert(timeStampList != NULL);
    if (!listAddLast(ee, timeStampList, valPtr)) {
	CVMsysMutexUnlock(ee, &CVMglobals.timestampListLock);
        /* insertion in the list failed... */
        free(valPtr->loc);
        free(valPtr);
        return CVM_FALSE;
    }
    CVMsysMutexUnlock(ee, &CVMglobals.timestampListLock);
    return CVM_TRUE;
}

/* this function initializes the timestampList and 
 * is called once at the beginning of VM creation.
 */
static CVMBool
timeStampStart(CVMExecEnv* ee, long wallClockTime)
{
    CVMassert(timeStampList == NULL);
    timeStampList = (List*) malloc(sizeof(List));
    if (timeStampList == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
        return CVM_FALSE;
    }
    listInit(timeStampList);
    return timeStampRecord(ee, "Starting", -1, wallClockTime);
}

/* This function prints the timeStamp records. Iterates the 
 * timestampList and prints the timeStampRecords.
 */
static CVMBool
timeStampPrint(CVMExecEnv* ee)
{
    ListIterator* fowardIterator;
    timeStampRecords* value;
    int prevTime = 0;
    long prevWallclockTime = 0;
    int locationCount = 0;
    fprintf(stdout, 
            "------------------------------------------------"
            "-------------------\n"
            "Location\tCPU Time (ms)\tCPU Time (ms)\tClock Time (ms)"
            "\tClock time\n"
            "        \tused by the  \tdiff between \telapsed        "
            "\tdiff (ms) \n"
            "        \tprogram      \tlocations\n");
    
    CVMsysMutexLock(ee, &CVMglobals.timestampListLock);
    fowardIterator = listGetIterator(ee, timeStampList);
    if (fowardIterator == NULL) {
	CVMsysMutexUnlock(ee, &CVMglobals.timestampListLock);
        return CVM_FALSE;
    }
    while ((value = (timeStampRecords*)listNext(fowardIterator)) != NULL) {
        if ((value->loc == NULL) && (value->position == -1)) {
            fprintf(stdout,
                    "auto# %d \t%d  \t\t%d \t\t%ld \t\t%ld\n",
                    ++locationCount,
                    (value->cpuTime), 
                    (value->cpuTime) - prevTime, 
                    (value->wallClockTime),
                    (value->wallClockTime) - prevWallclockTime);            
        }
        else if (value->position != -1) {
            fprintf(stdout,
                    "%10d \t%d  \t\t%d \t\t%ld \t\t%ld\n",
                    value->position,
                    (value->cpuTime), 
                    (value->cpuTime) - prevTime, 
                    (value->wallClockTime),
                    (value->wallClockTime) - prevWallclockTime);   
        }
        else {
            fprintf(stdout,
                    "%s \t%d  \t\t%d \t\t%ld \t\t%ld\n", 
                    value->loc, 
                    (value->cpuTime), 
                    (value->cpuTime)- prevTime, 
                    (value->wallClockTime),
                    (value->wallClockTime) - prevWallclockTime);            
        }
        prevTime = value->cpuTime;
        prevWallclockTime = value->wallClockTime; 
    }
    fprintf(stdout, "----------------------------------------------------"
                     "---------------\n");
    free(fowardIterator);
    CVMsysMutexUnlock(ee, &CVMglobals.timestampListLock);
    return CVM_TRUE;
}

/* This function must be called while destroying the VM 
 * because it does the clean up of the memory allocated 
 * during these operations.
 */
static CVMBool
timeStampFinishup(CVMExecEnv* ee, long wallClockTime)
{
    if (!timeStampRecord(ee, "Terminated", -1, wallClockTime)) {
        return CVM_FALSE;
    }
    if (CVMglobals.timeStampEnabled) {
	fprintf(stdout, "Time Stamps data at VM termination-\n");
	if (!timeStampPrint(ee)) {
	    return CVM_FALSE;
	}
    }

    /* Free up location records before feeing up list */
    {
        ListIterator* fowardIterator;
        timeStampRecords* value;
        CVMsysMutexLock(ee, &CVMglobals.timestampListLock);
        fowardIterator = listGetIterator(ee, timeStampList);
        if (fowardIterator != NULL) {
            while ((value = (timeStampRecords*)listNext(fowardIterator))
                   != NULL)
            {
                if (value->loc != NULL) {
                    free(value->loc);
                    value->loc = NULL;
                }
            }
            free(fowardIterator);
        }
        CVMsysMutexUnlock(ee, &CVMglobals.timestampListLock);
    }

    listAndNodesDestroy(timeStampList);
    free(timeStampList);
    timeStampList = NULL;
    return CVM_TRUE;
}

/* 
 * get time elapsed since first wall clock time has been recorded
 */
static long
getWallClkTime() 
{
    /* Here we assume that 'long' is big enough to keep wall-clock
     * time for a typical execution of an application
     */
    return CVMlong2Int((CVMlongSub(CVMtimeMillis(), 
				   CVMglobals.firstWallclockTime)));
}

/* 
 * Public functions 
 */

/* 
 * Initializes the 'firstWallclockTime' of CVMglobals
 * with -1
 */
void
CVMtimeStampWallClkInit()
{
    /*  the '-1' value for firstWallclockTime indicates that
     * it has not been initialized yet
     */
    CVMglobals.firstWallclockTime = CVMint2Long(-1);
}

/*
 * Time stamp record after VM initialization
 */
CVMBool
CVMtimeStampStart(CVMExecEnv* ee) 
{
    CVMassert(CVMlongEq(CVMglobals.firstWallclockTime, CVMint2Long(-1)));
    CVMglobals.firstWallclockTime = CVMtimeMillis();
    return timeStampStart(ee, 0);
}

/*
 * Time stamp record at VM termination
 */
CVMBool
CVMtimeStampFinishup(CVMExecEnv* ee) 
{
    return timeStampFinishup(ee, getWallClkTime());
}

/*
 * This is a public function, for time stamp recording.
 * The loc and pos identifies the location 
 * to record the time. You can pass this loc as NULL also.
 * the pos can not be -1. 
 */
CVMBool 
CVMtimeStampRecord(CVMExecEnv* ee, const char* loc, int pos) 
{
    if (CVMglobals.timeStampEnabled) {
	return timeStampRecord(ee, loc, pos, getWallClkTime());
    } else {
	return CVM_FALSE;
    }
}

/*
 * This function prints the time stamp records.
 */
CVMBool
CVMtimeStampPrint(CVMExecEnv* ee) 
{
    if (CVMglobals.timeStampEnabled) {
	return timeStampPrint(ee);
    } else {
	return CVM_FALSE;
    }
}
