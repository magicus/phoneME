/*
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
#include "RingBuffer-linux.h"
#include <assert.h>
#include <string.h>
#include "mni.h"

static int idCount = 0;

#define RB_START_SIZE   200000  //bytes
#define RB_MAX_SIZE     2000000  //bytes

void initRB()
{
	// Could create a custom heap here. But currently unused
}

void deinitRB()
{
	// Could create a custom heap here. But currently unused
}


RB *createRB(int bs)
{
    RB *r = NULL;
	
    r = MNI_CALLOC(1, sizeof(RB));
    assert(r!=NULL);
	
    r->buf = MNI_CALLOC(1, bs);
    assert(r->buf != NULL);
	
    r->bufSize = bs;
    r->head = 0;
    r->tail = 0;
	
    r->id = idCount++;
	
    assert(0 == pthread_mutex_init(&r->cs, NULL));
	
	
    return r;
}



void destroyRB(RB *rb)
{
    MNI_FREE(rb->buf);
    pthread_mutex_destroy(&rb->cs);               
    MNI_FREE(rb);
}


static int spaceAvail(RB *rb)
{
	if(rb->head == rb->tail) return rb->bufSize;
	if(rb->tail < rb->head) return (rb->head - rb->tail);
	if(rb->tail > rb->head) return (rb->bufSize - rb->tail) + rb->head;
	
	return 0;
}



int pushRB(RB *rb, char *data, int dataLen)
{
	int r = 0;
	
	assert(0 == pthread_mutex_lock(&rb->cs));
	/*
	{
	char msg[256];
	sprintf(msg, "PUSHRB<%d\n", rb->id); 
	}
	*/
	if(dataLen < spaceAvail(rb))
	{
		if( (rb->tail + dataLen) < rb->bufSize )
		{
			memcpy( rb->buf+rb->tail, data, dataLen );
			rb->tail += dataLen;
		}
		else 
		{
			int len1 = rb->bufSize - rb->tail;
			int len2 = dataLen - len1;
			
			memcpy(rb->buf+rb->tail, data, len1);
			memcpy(rb->buf, data+len1, len2);
			
			rb->tail = len2;
		}
		r = dataLen;
	}
	/*
	{
	char msg[256];
	sprintf(msg, "PUSHRB>%d\n", rb->id); 
	}
	*/
	assert(0 == pthread_mutex_unlock(&rb->cs));
	
	return r;
}


static int length(RB *rb)
{
    if(rb->head == rb->tail) return 0;
    if(rb->head < rb->tail) return (rb->tail - rb->head);
    if(rb->head > rb->tail) return (rb->bufSize - rb->head) + rb->tail;
	
    return 0;
}


int popRB(RB *rb, char *data, int dataLen)
{
	int r = 0;
	int len;
	
	assert(0 == pthread_mutex_lock(&rb->cs));
	/*
	{
	char msg[256];
	sprintf(msg, "POPRB<-%d\n", rb->id); 
	}
	*/
	len = length(rb);
	
	if(dataLen > len) dataLen = len;
	
	if(dataLen > 0)
	{
		if((rb->head + dataLen) < rb->bufSize)
		{
			memcpy(data, rb->buf + rb->head, dataLen);
			rb->head += dataLen;
		}
		else 
		{
			int len1 = rb->bufSize - rb->head;
			int len2 = dataLen - len1;
			
			memcpy(data, rb->buf+rb->head, len1);
			memcpy(data+len1, rb->buf, len2);
			
			rb->head = len2;
		}
		
		r = dataLen;
	}
	
	/*
	{
	char msg[256];
	sprintf(msg, "POPRB>-%d\n", rb->id); 
	}
	*/
	
	assert(0 == pthread_mutex_unlock(&rb->cs));
	
	return r;
}

