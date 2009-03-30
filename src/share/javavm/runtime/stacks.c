/*
 * @(#)stacks.c	1.90 06/10/10
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
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/stacks.h"
#include "javavm/include/stackwalk.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/common_exceptions.h"

#include "javavm/include/clib.h"

CVMBool
CVMinitStack(CVMExecEnv *ee, CVMStack* s,
	     CVMUint32 initialStackSize, CVMUint32 maxStackSize,
	     CVMUint32 minStackChunkSize, CVMUint32 initialFrameCapacity,
	     CVMFrameType frameType)
{
    CVMFrame* initialFrame;

    s->minStackChunkSize = minStackChunkSize;
    s->maxStackSize = maxStackSize;
    s->stackSize = 0;

    /* Initialize stack chunk pointers to NULL before calling CVMexpandStack */
    s->firstStackChunk = s->currentStackChunk = NULL;
 
    /* Also initialize stack chunk begin and end pointers to NULL.
       These are now no longer updated by the call to CVMexpandStack,
       below, but are tested by the call to CVMensureCapacity which is
       contained in the PushFrame call after it, leading to a read
       from uninitialized memory (caught with dbx's memory checking;
       "help check".) */
    s->stackChunkStart = s->stackChunkEnd = NULL;

    /* Add the first chunk to the stack */
    if (CVMexpandStack(ee, s, initialStackSize, CVM_FALSE, CVM_FALSE) == NULL){
	return CVM_FALSE;  /* exception NOT thrown */
    }

    /*
     * This is going to end up being the previous frame of the initial frame
     */
    initialFrame = 0;

    /* Now push a singleton frame on */
    CVMpushFrame(ee, s, initialFrame, s->stackChunkStart,
		 initialFrameCapacity, 0, frameType, NULL, CVM_TRUE, {}, {});

    /* CVMpushFrame can't fail because we ensured enough space. */
    CVMassert(initialFrame != 0);

    /* Make topOfStack point just beyond the frame structure. */
    initialFrame->topOfStack =
	(CVMStackVal32*)initialFrame + initialFrameCapacity;
    s->currentFrame = initialFrame;
    
    return CVM_TRUE;
}

void
CVMdestroyStack(CVMStack* s)
{
    CVMStackChunk *c = s->firstStackChunk;
    while (c != NULL) {
	CVMStackChunk *n = c->next;
	free(c);
	c = n;
    }
#ifdef CVM_DEBUG
    s->firstStackChunk = NULL;
    s->currentStackChunk = NULL;
    s->currentFrame = NULL;
    s->stackChunkStart = NULL;
    s->stackChunkEnd = NULL;
#endif
}

CVMBool
CVMinitGCRootStack(CVMExecEnv *ee, CVMStack* s, CVMUint32 initialStackSize, 
		   CVMUint32 maxStackSize, CVMUint32 minStackChunkSize,
		   CVMFrameType frameType)
{
    CVMFreelistFrame* initialFrame;
    CVMBool result;

    if (CVMglobals.unlimitedGCRoots) {
        maxStackSize = 0xffffffff;
    }
    result = CVMinitStack(ee, s, initialStackSize, maxStackSize, 
			  minStackChunkSize, CVMfreelistFrameCapacity,
			  frameType);

    if (!result) {
	return CVM_FALSE; /* exception NOT thrown */
    }

    initialFrame = (CVMFreelistFrame*)s->currentFrame;
    CVMfreelistFrameInit(initialFrame, CVM_FALSE);
    return CVM_TRUE;
}

void
CVMdestroyGCRootStack(CVMStack* s)
{
    CVMdestroyStack(s);
}

#ifdef CVM_DEBUG_DUMPSTACK

static void 
CVMdumpChunk(CVMStackChunk* chunk)
{
    CVMconsolePrintf("\tStack chunk:    0x%x\n", chunk);
    CVMconsolePrintf("\tChunk size:     0x%x\n",
		     (chunk->end_data - &chunk->data[0]));
    CVMconsolePrintf("\tChunk data:     0x%x\n", chunk->data);
    CVMconsolePrintf("\tChunk end data: 0x%x\n", chunk->end_data);
    CVMconsolePrintf("\tChunk prev:     0x%x\n", chunk->prev);
    CVMconsolePrintf("\tChunk next:     0x%x\n", chunk->next);
}

/*
 * Dump data in the interval [endLower, startHigher]
 */
static void
CVMdumpData(CVMStackVal32* startHigher, CVMStackVal32* endLower)
{
    CVMStackVal32* ptr;
    for (ptr = startHigher - 1; ptr >= endLower; ptr--) {
	CVMconsolePrintf("\t\t[0x%x] = (0x%8.8x) %d\n",
			 ptr, (*ptr).j.i, (*ptr).j.i);
    }
}

CVMStackChunk*
CVMdumpFrame(CVMFrame* frame, CVMStackChunk* startChunk,
	     CVMBool verbose, CVMBool includeData)
{
    CVMStackChunk* chunk      = startChunk;
    CVMStackVal32* topOfStack = frame->topOfStack;
    char*          type;
    char           buf[256];
    CVMUint8*	   pc = 0;
    CVMUint32      frameSize;

    /* get the type of frame */
    switch (frame->type) {
    case CVM_FRAMETYPE_TRANSITION:
	CVMassert(CVMframeIsTransition(frame));
	type = "Transition Frame";
	frameSize = CVM_TRANSITIONFRAME_SIZE / sizeof(CVMStackVal32*);
	pc = CVMframePc(frame);
	break;
    case CVM_FRAMETYPE_JAVA:
	CVMassert(CVMframeIsJava(frame));
	type = "Java Frame";
	frameSize = CVM_JAVAFRAME_SIZE / sizeof(CVMStackVal32*);
	pc = CVMframePc(frame);
	break;
#if 0
    case CVM_FRAMETYPE_JNI:
#endif
    case CVM_FRAMETYPE_FREELIST:
	CVMassert(CVMframeIsFreelist(frame));
	if (frame->mb != NULL) {
	    type = "JNI Frame";
	    frameSize = CVMJNIFrameCapacity;
	} else {
	    type = "Free List Frame";
	    frameSize = CVMfreelistFrameCapacity;
	}
	break;
#ifdef CVM_JIT
    case CVM_FRAMETYPE_COMPILED:
	CVMassert(CVMframeIsCompiled(frame));
	type = "Compiled Frame";
	frameSize = CVM_COMPILEDFRAME_SIZE / sizeof(CVMStackVal32*);
	pc = CVMcompiledFramePC(frame);
	break;
#endif
    default:
	frameSize = CVMbaseFrameCapacity;
	type = "<unknown frame type>";
    }

    /* get the very verbose name of the method */
    if (frame->mb != NULL) {
	CVMframe2string(frame, buf, buf + sizeof(buf));
    } else {
	sprintf(buf, "%s", "(JNI Local Frame)");
    }

    if (!verbose) {
	CVMconsolePrintf("  %-16s  %s\n", type, buf);
	if (includeData) {
	    CVMconsolePrintf("call CVMdumpFrame(0x%x, 0x%x, 1, 1)\n", 
			     frame, chunk);
	}
    } else {
	CVMconsolePrintf("\tFrame:   0x%x\n", frame);
	CVMconsolePrintf("\tprev:    0x%x\n", CVMframePrev(frame));
	CVMconsolePrintf("\tTos:     0x%x\n", frame->topOfStack);
	CVMconsolePrintf("\tType:    %s\n",   type);
	if (frame->mb != 0) {
	    CVMconsolePrintf("\tName:    %s\n", buf);
	}
	if (pc != 0) {
	    CVMconsolePrintf("\tPC:      0x%x\n", pc);
	}
	CVMconsolePrintf("\tspecial: %x\n",
			 CVMframeNeedsSpecialHandling(frame->prevX));
	if (frame->flags != 0) {
	    CVMconsolePrintf("\tflags: %x\n", frame->flags);
	}
	
	CVMconsolePrintf("\tContents:\n");
	CVMconsolePrintf("\t    Chunk 0x%x (0x%x-0x%x)\n", chunk,
			 &chunk->data[0], chunk->end_data);
	/*
	 * dumpData and handle frames spanning multiple stack chunks
	 */
	while(!CVMaddressInStackChunk(frame, chunk)) {
	    if (includeData) {
		CVMdumpData(topOfStack, &chunk->data[0]);
	    }
	    chunk = chunk->prev;
	    topOfStack = chunk->end_data;
	    CVMconsolePrintf("\t    Chunk 0x%x (0x%x-0x%x)\n",
			     chunk, &chunk->data[0], chunk->end_data);
	}
	if (includeData) {
	    CVMdumpData(topOfStack, (CVMStackVal32*)(frame) + frameSize);
	}

	CVMconsolePrintf("\n");
    }

    if ((CVMStackVal32*)frame == chunk->data) {
	chunk = chunk->prev;
    }
    return chunk;
}

static void
CVMdumpFrames(CVMStack* s, CVMBool verbose, CVMBool includeData,
	      CVMInt32 frameLimit)
{
    CVMStackWalkContext c;
    CVMFrame* frame;

    CVMstackwalkInit(s, &c);
    frame = c.frame;
    do {
	CVMdumpFrame(frame, c.chunk, verbose, includeData);
	CVMstackwalkPrev(&c);
	frame = c.frame;
	frameLimit--;
    } while (frame != 0 && frameLimit != 0);
}

/*
 * Dump out stack information, including backtrace. frameLimit limits the
 * number of frames that are traced. A limit of 0 dumps all frames.
 */

void
CVMdumpStack(CVMStack* s, CVMBool verbose, CVMBool includeData,
	     CVMInt32 frameLimit)
{
    CVMStackChunk* chunk;

    if (verbose) {
	CVMconsolePrintf("*** Dumping stack 0x%x\n", s);
	CVMconsolePrintf("    Stack info\n");
	CVMconsolePrintf("\tstack size:    0x%x\n", s->stackSize);
	CVMconsolePrintf("\tchunk start:   0x%x\n", s->stackChunkStart);
	CVMconsolePrintf("\tchunk end:     0x%x\n", s->stackChunkEnd);
	CVMconsolePrintf("\tcurrent chunk: 0x%x\n", s->currentStackChunk);

	/* First dump chunk information */
	for (chunk = s->firstStackChunk; chunk != NULL; chunk = chunk->next) {
	    CVMconsolePrintf("\n    Chunk 0x%x\n", chunk);
	    CVMdumpChunk(chunk);
	}
	CVMconsolePrintf("\n    Frames:\n");
    }
    CVMdumpFrames(s, verbose, includeData, frameLimit);
}

#endif

/*
 * CVMexpandStack()
 *
 * -Caller may be gcsafe or unsafe, but the only caller that is allowed
 *  to be gcsafe is CVMinitStack, becaues it is called before gc knows
 *  about thread so it won't attempt to scan the stack. CVMexpandStack() 
 *  will become gcSafe if it needs to allocate memory or if it throws 
 *  an exception.
 * -Returns NULL if unsuccessful.
 * -If throwException is true, then throws OutOfMemoryError if out of 
 *  memory or StackOverflowError if stack would exceed its preset limit.
 *
 * IMPORTANT: If justChecking == false, then the caller must not become
 * gcsafe until after making sure that stack->currentFrame->topOfStack is in
 * the new stack chunk returned. Otherwise CVMwalkRefsInGCRootFrame() will
 * cause gc to crash because it always expects
 * stack->currentFrame->topOfStack to be in the stack's current chunk.
 */
CVMStackVal32* 
CVMexpandStack(CVMExecEnv* ee, CVMStack* s, 
	       CVMUint32 capacity, CVMBool throwException,
	       CVMBool justChecking)
{
    CVMStackChunk* chunk = s->currentStackChunk;
    CVMStackChunk* next  = (chunk == NULL ? NULL : chunk->next);
    CVMStackChunk* after = NULL;
    CVMStackVal32* data;

    /* The only time we should be gcSafe is during thread creation. */
    CVMassert(CVMD_isgcUnsafe(ee) || s->currentFrame == NULL);

    if (capacity < s->minStackChunkSize) {
	/* We should at least allocate this many words */
	capacity = s->minStackChunkSize;
    }

    while ((next != NULL) && 
	   ((CVMUint32)(next->end_data - next->data) < capacity)) {
	/* Not enough room. Unlink and free this chunk. A new one 
	   will be put in its place */
	after = next->next;
	chunk->next = after;
	if (after != NULL) {
	    after->prev = chunk;
	}
	s->stackSize -= (CVMUint32)(next->end_data - next->data);
	free(next);
	next = after;
    }

    if (next == NULL) {
	CVMUint32      size;
	CVMUint32      newStackSize;
	/* 
	 * Either the size of 'next' wasn't enough, or there was no 'next'.
	 * Must allocate new chunk. Take sizeof(data[1]) field into account.
	 */
	size = sizeof(CVMStackChunk) + 
	    sizeof(CVMStackVal32) * (capacity - 1);

	newStackSize = s->stackSize + capacity;
	if (newStackSize > s->maxStackSize) {
	    if (throwException) {
		CVMthrowStackOverflowError(ee, NULL);
	    }
	    return NULL;
	}
	if (CVMD_isgcSafe(ee)) {
	    next = (CVMStackChunk*)malloc(size);
	} else {
	    CVMD_gcSafeExec(ee, {
		next = (CVMStackChunk*)malloc(size);
	    });
	}
	if (next == NULL) {
	    if (throwException) {
		CVMthrowOutOfMemoryError(ee, NULL);
	    }
	    return NULL;
	}
	s->stackSize = newStackSize;

	/* Link the new chunk in */
	next->prev  = chunk;
	next->next  = after;
	if (chunk == NULL) {
	    s->firstStackChunk = next;
	} else {
	    chunk->next = next;
	}
	if (after != NULL) {
	    after->prev = next;
	}

        /* Set the capacity of the newly allocated chunk: */
        next->end_data       = &next->data[capacity];
    }

    data                 = &next->data[0];

    /*
     * Now we should see if we need to update the stack data
     * structure or not. If we are 'justChecking', we should make
     * sure adequate space exists, but we don't update the stack data
     * structure pointers. The next time CVMexpandStack is called with
     * justChecking == FALSE, that's when we commit this space.  
     */
    if (!justChecking) {
	s->currentStackChunk = next;
	s->stackChunkStart   = data;
        s->stackChunkEnd     = next->end_data;
    }

    return data;
}


/* 
 * Checks if the stack can ensure the specified capacity. It will attempt to
 * grow the stack if necessary. Returns CVM_TRUE if it can. Otherwise it
 * throws and exception and returns CVM_FALSE.
 */
CVMBool
CVMensureCapacity(CVMExecEnv* ee, CVMStack* stack, int capacity)
{
    CVMFrame*      currentFrame = stack->currentFrame;
    CVMStackVal32* topOfStack = currentFrame->topOfStack;

    if (CVMcheckCapacity(stack, topOfStack, capacity)) {
	return CVM_TRUE;
    } else {
	/* Must be unsafe to call CVMexpandStack. */
	CVMD_gcUnsafeExec(ee, {
	    CVMexpandStack(ee, stack, capacity, CVM_TRUE, CVM_TRUE);
	});
	return !CVMlocalExceptionOccurred(ee);
    }
}

#ifdef CVM_JIT
/* Purpose: Checks to see if the current frame can be replaced with a new
            frame of the specified capacity.  This function will try to
            expand the stack if necessary. */
CVMBool
CVMframeEnsureReplacementWillFit(CVMExecEnv *ee, CVMStack *stack,
                                 CVMFrame *currentFrame,
                                 CVMUint16 newCapacity)
{
    CVMStackVal32 *prevTopOfStack;
    prevTopOfStack = (CVMStackVal32 *)CVMframeLocals(currentFrame);
    if (CVMcheckCapacity(stack, prevTopOfStack, newCapacity)) {
        return CVM_TRUE;
    } else {
        CVMStackVal32 *space;
        space = CVMexpandStack(ee, stack, newCapacity, CVM_FALSE, CVM_TRUE);
        if (space != NULL) {
            return CVM_TRUE;
        }
    }
    return CVM_FALSE;
}

/* Purpose: Deletes the specified chunk in the given stack. */
void
CVMstackDeleteChunk(CVMStack *stack, CVMStackChunk *chunk)
{
#ifdef CVM_DEBUG_ASSERTS
    /* Make sure the specified chunk is in the specified stack: */
    {
        CVMStackChunk *current = stack->currentStackChunk;
        CVMBool chunkFoundInStack = CVM_FALSE;
        while (!chunkFoundInStack && (current != NULL)) {
            if (current == chunk) {
                chunkFoundInStack =  CVM_TRUE;
            } else {
                current = current->prev;
            }
        }
        CVMassert(chunkFoundInStack);
    }
#endif

    /* Unlink the chunk from the stack chunk list: */
    if (chunk->next != NULL) {
        chunk->next->prev = chunk->prev;
    }
    if (chunk->prev == NULL) {
        stack->firstStackChunk = chunk->next;
    } else {
        chunk->prev->next = chunk->next;
    }

    /* Release the chunk and indicate the release in the stack size: */
    stack->stackSize -= (CVMUint32)(chunk->end_data - chunk->data);
    free(chunk);
}

#endif /* CVM_JIT */

#ifdef CVM_JVMTI
/* Purpose: Deletes the last chunk in the given stack. */
CVMBool
CVMstackDeleteLastChunk(CVMStack *stack, CVMStackChunk *chunk)
{
#ifdef CVM_DEBUG_ASSERTS
    /* Make sure the specified chunk is NOT in the specified stack: */
    {
        CVMStackChunk *current = stack->currentStackChunk;
        CVMBool chunkFoundInStack = CVM_FALSE;
        while (!chunkFoundInStack && (current != NULL)) {
            if (current == chunk) {
                chunkFoundInStack =  CVM_TRUE;
            } else {
                current = current->prev;
            }
        }
        CVMassert(!chunkFoundInStack);
    }
    CVMassert(stack->currentFrame->topOfStack <=
              stack->stackChunkEnd &&
              stack->currentStackChunk->next != NULL &&
              stack->currentStackChunk->next->next == NULL);
#endif
    if (stack->currentFrame->topOfStack >
        stack->stackChunkEnd ||
        stack->currentStackChunk->next == NULL ||
        stack->currentStackChunk->next != chunk ||
        stack->currentStackChunk->next->next != NULL) {
        return CVM_FALSE;
    }
    /* Unlink the chunk from the stack chunk list: */
    chunk->prev->next = chunk->next;

    /* Release the chunk and indicate the release in the stack size: */
    stack->stackSize -= (CVMUint32)(chunk->end_data - chunk->data);
    free(chunk);
    return CVM_TRUE;
}

void
CVMstackEnableReserved(CVMStack *curStack)
{
    curStack->maxStackSize += curStack->minStackChunkSize;

}

void
CVMstackDisableReserved(CVMStack *curStack)
{
    if (curStack->currentStackChunk->next != NULL) {
        if (CVMstackDeleteLastChunk(curStack,
                                    curStack->currentStackChunk->next)) {
            curStack->maxStackSize -= curStack->minStackChunkSize;
        }
    }
}

#endif

CVMBool
CVMCstackCheckSize(CVMExecEnv* ee, CVMUint32 redzone, 
		   char *func_name, CVMBool exceptionThrow)
{
    if (!CVMthreadStackCheck(CVMexecEnv2threadID(ee), redzone)) {
#ifdef CVM_DEBUG
        /*
         * Avoid a potential recursion problem here since
         * CVMdebugPrintf() leads to another stack check.
         */
        printf("C stack overflow in \"%s\".\n", func_name);
#endif
        if (exceptionThrow) {
	    if (CVMD_isgcUnsafe(ee)) {
        	CVMgcUnsafeThrowLocalExceptionNoDumpStack(ee, 
		    CVMID_icellDirect(ee, CVMglobals.preallocatedStackOverflowError));
	    }
	    else {
        	CVMgcSafeThrowLocalExceptionNoDumpStack(ee, 
		    CVMglobals.preallocatedStackOverflowError);
	    }
        }
        return CVM_FALSE;
    } else {
	return CVM_TRUE;
    }
}

