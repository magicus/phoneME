/*
 * @(#)stacks.h	1.117 06/10/10
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

#ifndef _INCLUDED_STACKS_H
#define _INCLUDED_STACKS_H

/****************************************************************************
 * CVMStack
 *
 * CVMStack is a C type for the four types of stacks we need:
 * interpreter stack, local roots stack, global roots stack, and jni
 * global refs stack.
 *
 * The stack is broken up into separate chunks. Initially there is only
 * one chunk, but as the stack grows more chunks may be added. If the
 * stack shrinks, chunks are not aggressively freed. Because of this,
 * currentStackChunk->next might not be NULL, but may in fact point
 * to a free chunk that can be used if that stack needs to grow again.
 ****************************************************************************/

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/stackwalk.h"

/*
 * Generic 32-bit wide "Java slot" definition. This type occurs
 * in Java locals, object fields, constant pools, and
 * operand stacks (as a CVMStackVal32).
 */
typedef union CVMSlotVal32 {
    CVMJavaVal32    j;     /* For "Java" values */
    CVMUint8*       a;     /* a return created by jsr or jsr_w */
} CVMSlotVal32;

/*
 * Generic 32-bit wide stack slot definition.
 */
union CVMStackVal32 {
    CVMJavaVal32    j;     /* For "Java" values */
    CVMSlotVal32    s;     /* any value from a "slot" or locals[] */
    CVMObjectICell  ref;   /* JNI refs, local roots, global roots */
    CVMStackVal32*  next;  /* An element of a free list */
};

struct CVMStackChunk {
    CVMStackChunk*  prev;          /* Previous chunk of this stack */
    CVMStackChunk*  next;          /* Next chunk of this stack */
    CVMStackVal32*  end_data;      /* address of end of this chunk */
    CVMStackVal32   data[1];       /* actual data  */
};


struct CVMStack {
    CVMStackChunk*   firstStackChunk; 
    CVMStackChunk*   currentStackChunk;
    CVMUint32        minStackChunkSize;/* minimum size for new chunk */
    CVMUint32        maxStackSize;     /* max space used in all chunks */
    CVMUint32        stackSize;        /* Total space used in all chunks */
    CVMFrame*        volatile currentFrame;     /* Current stack frame */
    /* Stack chunk pointers. Both may not be needed and can
     * always be calculated. stackChunkEnd saves us an indirection on
     * method calls. stackChunkStart might not be of much value.
    */
    CVMStackVal32*   stackChunkStart;  /* &currentStackChunk->data[0] */
    CVMStackVal32*   stackChunkEnd;    /* currentStackChunk->end_data */
};

/*
 * Types of stack frames we need to know about.
 */
typedef enum {
#ifdef CVM_DEBUG_ASSERTS
    CVM_FRAMETYPE_NONE,
#endif
    CVM_FRAMETYPE_JAVA,
    CVM_FRAMETYPE_TRANSITION,
    CVM_FRAMETYPE_FREELIST,
    CVM_FRAMETYPE_LOCALROOT,
    CVM_FRAMETYPE_GLOBALROOT,
    CVM_FRAMETYPE_CLASSTABLE,
#ifdef CVM_JIT
    CVM_FRAMETYPE_COMPILED,
#endif
    CVM_NUM_FRAMETYPES
} CVMFrameType;

typedef enum {
    CVM_FRAMEFLAG_ARTIFICIAL = 0x1,
    CVM_FRAMEFLAG_EXCEPTION = 0x2
} CVMFrameFlags;

extern CVMFrameGCScannerFunc * const CVMframeScanners[CVM_NUM_FRAMETYPES];

/****************************************************************************
 * CVMFrame
 *
 * CVMFrame is the common base class for all frames used to keep track of
 * local roots, java method calls, jni method calls, and frames created
 * by JNI PushLocalFrame. The last frame is always pointed to by the
 * owning stacks CVMStack.currentFrame field. It is not possible for
 * a frame to automatically know it's owning stack, but it can be deduced
 * from the frame type.
 ****************************************************************************/

struct CVMFrame {
    CVMFrame*             prevX;      /* previous frame: access using macros*/
    CVMUint8		  type;		/* CVMFrameType */
    CVMUint8		  flags;	/* CVMFrameFlags */
    /* topOfStack is used by CVMLocalRootsFrame to allocate the next
     * local root (localRoots field), by CVMJavaFrame to push and pop
     * operands (opstack field), and by CVMJNIFrame to allocate new
     * local refs (localRefs field).
     */
    CVMStackVal32*   volatile topOfStack; /* Pointer to top of stack.
					     This is the first free slot. */
    CVMMethodBlock*  volatile mb;         /* the method we are executing */
};

struct CVMFreelistFrame {
    CVMFrame          frame;

    CVMUint32         inUse;      /* Number of items in use under topOfStack */
    CVMStackVal32*    freeList;   /* A pointer to the free list */
    CVMStackVal32     vals[1];    /* Frame contents */
};

#define CVMfreelistFrameInit(frame_, mbIsAlreadyInited_) \
{                                                        \
    if (!mbIsAlreadyInited_) {                           \
        ((CVMFrame *)(frame_))->mb = 0;                  \
    }                                                    \
    (frame_)->inUse = 0;                                 \
    (frame_)->freeList = 0;                              \
}

/*
 * These are the capacities to ensure for a base frame and a
 * freelisting frame
 */
#define CVMbaseFrameCapacity \
    (sizeof(CVMFrame) / sizeof(CVMStackVal32))

#define CVMfreelistFrameCapacity  \
    (sizeof(CVMFreelistFrame) / sizeof(CVMStackVal32) - 1)

/****************************************************************************
 * Stack and Frame API's
 ****************************************************************************/

/* Many of the macros take arguments that could easily have been retrieved
 * from other arguments. This is done for performance because most of the
 * arguments are already be cached in registers. Also, some of the macros
 * don't set all fields in the stack and frame that they could set. This
 * is also done for performance to allow the interpreter to delay flushing
 * the value from a local to the stack or field only when a gc is needed.
 */


/*
 * Expand stack 's' to contain capacity words, and return a pointer to the
 * new area. If unsuccessful, returns NULL and throws an exception if the
 * throwException argument is true.
 *
 * If justChecking == TRUE, we are simply checking whether we have
 * this capacity or not. 
 */
extern 
CVMStackVal32* CVMexpandStack(CVMExecEnv* ee, CVMStack* s, 
			      CVMUint32 capacity,
			      CVMBool throwException,
			      CVMBool justChecking);

/*
 * Macros for dealing with frames that need special handling when they
 * are popped.
 *
 * We need a cheap way of finding out if we are popping the
 * first frame in a stack chunk, or if we are popping a frame that spans
 * multiple stack chunks. Both of these situations require special handling
 * by popFrame so information in the CVMStack can be updated. We use
 * the low bit in frame->prev to indicate that frame requires special
 * handling by popFrame. This is a cheap place for the flag because we
 * need to load frame->prev anyway when popping frame. This bit referred
 * to as the SPECIAL bit. 
 *
 * We also need a flag to indicate if we are returning from a compiled
 * frame to another compiled frame, allowing for quick compiled ->
 * compiled returns. We could look at the frame 'type' field, but it
 * is cheaper to use a bit in the frame->prev field since it is being
 * loaded anyway. This is referred to as the SLOW bit and is false if
 * compiled code can do a quick pop of the frame and then return to
 * another compiled method. Only compiled frames ever have SLOW=0, but
 * both compiled and java frames can have SLOW=1. All frames that need
 * special handling because of chunk boundary related issues as
 * described above have SLOW=1.
 *
 * SPECIAL has different meaning based on the value of SLOW. For
 * SLOW=0, it indicates if the compiled frame has had its fixup done
 * (CVMJITfixupFrames).  For SLOW=1 it indicates if the frame needs
 * special handling because of chunk boundary issues as described
 * above.
 *
 * SLOW=0 SPECIAL=0 compiled -> compiled return, needs CVMJITfixupFrames
 * SLOW=0 SPECIAL=1 compiled -> compiled return, CVMJITfixupFrames done
 * SLOW=1 SPECIAL=0 return must be handled by interpreter (usually means
 *                  returning to a non-compiled method)
 * SLOW=1 SPECIAL=1 return must be handled by interpreter and frame needs 
 *                  special handling (ie. first frame in chunk)
 */

#define CVM_FRAME_MASK_SPECIAL		(1 << 0)
#define CVM_FRAME_MASK_SLOW		(1 << 1)
#define CVM_FRAME_MASK_ALL		((1 << 2) - 1)

/* Clear low bit in CVMFrame* */
/* 
 * make CVM ready to run on 64 bit platforms
 * 
 * frame_ is used as an address
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMframeUnmasked(frame_) \
    ((CVMFrame*)((CVMAddr)(frame_) & ~CVM_FRAME_MASK_ALL))

/* Set low bit in CVMFrame* */
/* 
 * make CVM ready to run on 64 bit platforms
 * 
 * frame_ is used as an address
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMframeMasked(frame_, mask_) \
    ((CVMFrame*)((CVMAddr)(frame_) | (mask_)))

#ifdef CVM_JIT
/* 
 * make CVM ready to run on 64 bit platforms
 * 
 * frame_ is used as an address
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMframeMaskBitsAreCorrect(frame_) \
    ((((CVMAddr)((frame_))->prevX) & CVM_FRAME_MASK_ALL) != 0)
#else
#define CVMframeMaskBitsAreCorrect(frame_) \
    CVM_TRUE
#endif

#define CVMassertFrameMaskBitsAreCorrect(frame_) \
    CVMassert(CVMframeMaskBitsAreCorrect((frame_)))

/* Get prevous frame without bit being set. Used to iterate over frames */
#define CVMframePrev(frame_) \
    (CVMassertFrameMaskBitsAreCorrect(frame_), \
     CVMframeUnmasked((frame_)->prevX))

/* Set the "special handling" bit */
#define CVMsetFrameNeedsSpecialHandlingBit(frame_) \
    (frame_) = CVMframeMasked((frame_), CVM_FRAME_MASK_SPECIAL)

/* Check if the frame needs special handling in popFrame */
#ifndef CVM_JIT
/* 
 * make CVM ready to run on 64 bit platforms
 * 
 * frame_ is used as an address
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMframeNeedsSpecialHandling(frame_) \
    (((CVMAddr)(frame_) & CVM_FRAME_MASK_SPECIAL) != 0)
#else
/* 
 * make CVM ready to run on 64 bit platforms
 * 
 * frame_ is used as an address
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMframeNeedsSpecialHandling(frame_) \
    (((CVMAddr)(frame_) & CVM_FRAME_MASK_ALL) ==	\
    (CVM_FRAME_MASK_SLOW | CVM_FRAME_MASK_SPECIAL))
#endif

/* 
 *   CVMBool 
 *   CVMcheckCapacity(CVMStack* s2_, CVMStackVal32* tos2,
 *                    CVMUint32 capacity2)
 *
 *   Checks if stack s2_ with current top of stack tos2_, can ensure
 *   a capacity of capacity2_. Return true if it can.
 */
#define CVMcheckCapacity(s2_, tos2_, capacity2_)		\
    (((CVMUint32)capacity2_) <= (CVMUint32)((s2_)->stackChunkEnd - (tos2_)))

/* 
 * Checks if the stack can ensure the specified capacity. It will attempt to
 * grow the stack if necessary. Returns CVM_TRUE if it can. Otherwise it
 * throws an exception and returns CVM_FALSE.
 */
extern CVMBool
CVMensureCapacity(CVMExecEnv* ee, CVMStack* stack, int capacity);

/*
 *  void
 *  CVMpushFrame(CVMExecEnv* ee,
 *      CVMStack* s_, CVMFrame* cur_frame_, CVMStackVal32* tos_,
 *      CVMUint32 capacity_, CVMUint32 frameOffset_,
 *      CVMFrameType frameType_, CVMMethodBlock* methodBlock_,
 *	commit_, failAction_, expandAction_);
 *
 *  Push frame to stack s_, with current frame cur_frame_, current top
 *  of stack tos_, desired capacity_ in number of words, and a frame
 *  offset of frameOffset_ in number of words. cur_frame_ gets back
 *  a pointer to the new frame. The frame offset determines the
 *  'distance' from the previous frame's top of stack to the allocated
 *  CVMFrame. It is used to make room for java local variables.
 *
 *  On exit, cur_frame_ is set to the new frame. If the push failed (due
 *  to the failure of an expansion request) cur_frame_ is set to 0.
 *
 *  Must be called while gcUnsafe, but will become safe if the stack
 *  needs to be expanded.
 * 
 *  tos_ is not updated because java frames need to set the location of
 *  the new frame's local variables to the old tos minus the arg size.
 *
 *  frameType_ is a CVMFrameType in order to scan roots in
 *  this frame at GC time.
 *
 *  commit_ is a boolean that determines if (s_)->currentFrame will
 *  be updated.
 *
 *  failAction_ contains statements to executed if the frame cannot
 *  be allocated. It may contain a goto. CVMpushFrame will also throw 
 *  an exception and set cur_frame_ to NULL if unsuccessful.
 *
 *  expandAction_ contains statement to be executed only if the stack
 *  was successfully expanded. It is used by the interpreter to copy
 *  method arguments. It may contain a goto.
 */
#define CVMassertPushFrameHasProperGCSafety(ee, s)			\
    CVMassert(CVMD_isgcUnsafe(ee) || (s)->currentFrame == NULL)

#ifdef CVM_JIT
/* Purpose: Indicate that we have to use the slow method of returning to the
            previous frame.  An example of this is a transition from compiled
            code to interpreted code. */
#define CVMsetPrevSlow(f, p)	\
        ((f)->prevX = CVMframeMasked((p), CVM_FRAME_MASK_SLOW))
#else
#define CVMsetPrevSlow(f, p)	\
        ((f)->prevX = (p))
#endif

/* NOTE: In JVMPI, it is possible to suspend a thread at any arbitrary point.
    After this suspension, the profiler agent may request call traces.  Hence,
    the stack data must always appear to be consistent.  Thus frame->
    must be initialized before the frame is "pushed" onto the stack.
    A NULL methodblock pointer will tell JVMPI that the frame
    should be ignored.
*/

#ifdef CVM_JVMTI
#define clearLockInfo(x)				\
    if (CVMframeIsJava(x)) {				\
	CVMgetJavaFrame((x))->jvmtiLockInfo = NULL;	\
    }
#else
#define clearLockInfo(x)
#endif

#define CVMpushFrame(ee_, s_, cur_frame_, tos_, capacity_,		\
                     frameOffset_, frameType_, mb_, commit_,		\
		     failAction_, expandAction_)			\
{									\
    CVMFrame*       cf_    = (CVMFrame*)(cur_frame_);			\
									\
    /* The only time we should be gcSafe is during thread creation. */  \
    CVMassertPushFrameHasProperGCSafety(ee_, s_);              		\
									\
    if (CVMcheckCapacity((s_), (tos_), (capacity_))) {			\
	(cur_frame_)       = (CVMFrame*)&tos_[(frameOffset_)];		\
	CVMsetPrevSlow(cur_frame_, cf_);				\
        (cur_frame_)->type = frameType_;				\
        (cur_frame_)->mb  = mb_;					\
        (cur_frame_)->flags = 0;					\
	clearLockInfo(cur_frame_);					\
	if (commit_) {							\
	    (s_)->currentFrame    = (cur_frame_);			\
	}								\
    } else {								\
        CVMStackVal32* space_;						\
	space_ = CVMexpandStack(ee_, s_, (capacity_),			\
	    CVM_TRUE, !commit_);					\
        if (space_ == 0) {						\
            (cur_frame_) = 0;	/* do this before failAction_ */	\
            failAction_;						\
        } else {							\
	    CVMsetFrameNeedsSpecialHandlingBit(cf_);			\
	    (cur_frame_)       = (CVMFrame*)&space_[(frameOffset_)];	\
	    CVMsetPrevSlow(cur_frame_, cf_);				\
            (cur_frame_)->type = frameType_;				\
	    (cur_frame_)->mb  = mb_;					\
	    (cur_frame_)->flags = 0;					\
	    clearLockInfo(cur_frame_);					\
            expandAction_;     /* do this after setting cur_frame_ */	\
	    if (commit_) {						\
		(s_)->currentFrame    = (cur_frame_);			\
	    }								\
        }								\
    }									\
}

#ifdef CVM_JIT
/* Purpose: Checks to see if the current frame can be replaced with a new
            frame of the specified capacity.  This function will try to
            expand the stack if necessary. */
extern CVMBool
CVMframeEnsureReplacementWillFit(CVMExecEnv *ee, CVMStack *stack,
                                 CVMFrame *currentFrame,
                                 CVMUint16 newCapacity);

/* Purpose: Replace the specified frame with a new frame while preserving
            the old frame information.  Currently, this is only used for the
            JIT OSR (On-Stack Replacement) feature.
*/
#define CVMreplaceFrame(ee_, s_, cur_frame_, oldFrameSize_,             \
                        oldFrameOffset_, newCapacity_, newFrameOffset_, \
                        newFrameType_, mb_, commit_,                    \
                        failAction_, expandAction_)                     \
{									\
    CVMFrame *cf_ = (CVMFrame*)(cur_frame_);                            \
    CVMStackVal32 *tos_ =                                               \
        (CVMStackVal32 *)(cur_frame_) - (oldFrameOffset_);              \
    CVMassert(CVMD_isgcUnsafe(ee_));                                    \
    if (CVMcheckCapacity((s_), (tos_), (newCapacity_))) {               \
        /* Compute the address of the new frame: */                     \
        (cur_frame_)       = (CVMFrame*)&tos_[(newFrameOffset_)];       \
        /* Copy the old frame info to the new frame: */                 \
        memmove((cur_frame_), cf_, (oldFrameSize_));                    \
        /* NOTE: The previousFrame pointer remains unchanged. */        \
        /* Set the new frame type: */                                   \
        (cur_frame_)->type = newFrameType_;                             \
        /* NOTE: The frame mb pointer should remain unchanged: */       \
        CVMassert((cur_frame_)->mb == (mb_));                           \
	if (commit_) {							\
	    (s_)->currentFrame    = (cur_frame_);			\
	}								\
    } else {								\
        CVMStackVal32* space_;						\
	space_ = CVMexpandStack((ee_), (s_), (newCapacity_), CVM_TRUE,  \
	                        CVM_FALSE);                             \
        if (space_ == 0) {						\
            (cur_frame_) = 0;	/* do this before failAction_ */	\
            failAction_;						\
        } else {							\
            /* Get the previousFrame pointer and mark as being */       \
            /* going across a chunk boundary: */                        \
            CVMFrame *prev_ = cf_->prevX;                               \
                                                                        \
            CVMsetFrameNeedsSpecialHandlingBit(prev_);                  \
            (cur_frame_) = (CVMFrame*)&space_[(newFrameOffset_)];       \
                                                                        \
            /* Copy the old frame info to the new frame: */             \
            memcpy((cur_frame_), cf_, (oldFrameSize_));                 \
                                                                        \
            CVMsetPrevSlow(cur_frame_, prev_);                          \
            /* Set the new frame type: */                               \
            (cur_frame_)->type = (newFrameType_);                       \
            /* NOTE: The frame mb pointer should remain unchanged: */   \
            CVMassert((cur_frame_)->mb == (mb_));                       \
            expandAction_;     /* do this after setting cur_frame_ */	\
	    if (commit_) {						\
		(s_)->currentFrame    = (cur_frame_);			\
	    }								\
        }								\
    }									\
}

#endif

/*
 *  void
 *  CVMpopFrame(CVMStack* s_, CVMFrame* cur_frame_);
 *
 *  Pop topmost frame from stack s_ with current frame
 *  cur_frame. cur_frame_ gets pointer to resulting new topmost frame.  
 *
 *  If the prev bit of the popped frame is tagged, this means one or both of
 *  two things: The frame spanned multiple stack chunks and/or 
 *  the frame is the first one in the current chunk. To handle these cases, 
 *  first find the chunk that the popped frame points to. If the new top 
 *  frame is not on the same chunk, update again.
 */

/* NOTE: In JVMPI or JVMTI, it is possible to suspend a thread at any arbitrary
    point.
    After this suspension, the profiler agent may request call traces.  Hence,
    the stack data must always appear to be consistent.  The
    CVMframePresetCurrentFrame() macro below is needed to ensure that the stack
    frame is popped first before its chunk is "popped".  Otherwise, it may
    appear that the currentFrame is not located in any of the stack chunks.
*/
#if defined(CVM_JVMPI) || defined(CVM_JVMTI)

#define CVMframePresetCurrentFrame(frame_, value_) { (frame_) = (value_); }
#define CVMframeSetCurrentStackChunk(s_, chunk_) {      \
    CVMStackChunk * volatile *currentStackChunkPtr_;    \
    currentStackChunkPtr_ = &(s_)->currentStackChunk;   \
    *currentStackChunkPtr_ = chunk_;                    \
}

#else /* !(CVM_JVMPI || CVM_JVMTI)*/

#define CVMframePresetCurrentFrame(frame_, value_)
#define CVMframeSetCurrentStackChunk(s_, chunk_) { \
    (s_)->currentStackChunk = chunk_;              \
}
#endif /* CVM_JVMPI || CVM_JVMTI */

#define CVMpopFrameSpecial(s_, cur_frame_, specialAction_)		    \
{									    \
    CVMframeGetPreviousUpdateChunk((s_)->currentStackChunk, (cur_frame_), { \
        /* We need to adjust the frame pointer before popping off the */    \
        /* top chunk to make sure that we stay consistent: */               \
        CVMframePresetCurrentFrame((s_)->currentFrame, (prev_));            \
        CVMframeSetCurrentStackChunk((s_), chunk_);                         \
        (s_)->stackChunkStart = chunk_->data;                               \
        (s_)->stackChunkEnd = chunk_->end_data;                             \
        specialAction_							    \
    });                                                                     \
    /* We need to set the current frame pointer anyway because the above */ \
    /* presetting won't necessarily happen in every case: */                \
    (s_)->currentFrame = (cur_frame_);                                      \
}

#define CVMpopFrame(s_, cur_frame_)	\
	CVMpopFrameSpecial((s_), (cur_frame_), {/* no special action */})

/*
 * Common to any code that wants to get to the previous frame,
 * and track stack chunks in the process.
 *
 * cur_chunk_     -- The last chunk of the current frame
 * cur_frame_     -- The current frame
 * updateAction_  -- The action to perform if we had to travel chunks
 *
 */
#define CVMframeGetPreviousUpdateChunk(cur_chunk_,			\
				       cur_frame_,			\
				       updateAction_)			\
{									\
    CVMFrame* prev_ = (cur_frame_)->prevX;				\
    CVMassertFrameMaskBitsAreCorrect(cur_frame_);                       \
    if (CVMframeNeedsSpecialHandling(prev_)) {				\
	/*								\
	 * Either curFrame spanned multiple stack			\
	 * chunks or it is the first frame in				\
	 * chunk. In any case, we must update the			\
	 * new current chunk to refer to last chunk of			\
	 * previous frame.						\
	 */								\
        CVMStackChunk* chunk_   = (cur_chunk_);				\
	/*								\
	 * First bring the chunk to the chunk of			\
	 * the current frame						\
	 */								\
        while(!CVMaddressInStackChunk((cur_frame_), chunk_)) {		\
            chunk_ = chunk_->prev;					\
        }								\
	prev_ = CVMframeUnmasked(prev_);				\
	/*								\
	 * Now 'chunk' points to the starting chunk			\
	 * of curFrame.  If this frame was the				\
	 * first in the current chunk, the previous			\
	 * frame's last chunk is the previous				\
	 * chunk.							\
	 */								\
        if ((prev_ != NULL) && 						\
	    !CVMaddressInStackChunk(prev_->topOfStack, chunk_)) {	\
	    chunk_ = chunk_->prev;					\
	}								\
        updateAction_;							\
    } else {								\
	/*								\
	 * Mask off any other frame flags 				\
	 * This can be removed if there are no other flags.		\
	 */								\
	prev_ = CVMframeUnmasked(prev_);				\
    }									\
    cur_frame_ = prev_;							\
}

/*
 * push/pop GC root frame pushes empty frames that will contain GC roots
 * like JNI locals and globals, CVM local and global roots.
 *
 * If unsuccessful, throws an exception and sets curr_frame3_ to NULL.
 */
#define CVMpushGCRootFrameUnsafe(ee3_, s3_, curr_frame3_,		\
				 capacity3_, frameType_)		\
{									\
    CVMpushFrame((ee3_), (s3_), (curr_frame3_),				\
		 ((curr_frame3_)->topOfStack),				\
	         (capacity3_), 0, frameType_, NULL, CVM_TRUE, {}, {});	\
    if ((curr_frame3_) != 0) {						\
        (curr_frame3_)->topOfStack =					\
    	(CVMStackVal32*)(curr_frame3_) + (capacity3_);			\
    }									\
    CVMassert((curr_frame3_) != 0);					\
}

#define CVMpushGCRootFrame(ee3_, s3_, curr_frame3_, capacity3_, frameType_) \
{									    \
    CVMD_gcUnsafeExec((ee3_), {						    \
        CVMpushGCRootFrameUnsafe(ee3_, s3_, curr_frame3_,		    \
				 capacity3_, frameType_)		    \
    });									    \
    CVMassert((curr_frame3_) != 0);					    \
}

#define CVMpopGCRootFrameUnsafe(ee3_, s3_, curr_frame3_)	\
{								\
    CVMassert(CVMD_isgcUnsafe(ee3_));				\
    CVMpopFrame((s3_), (curr_frame3_));				\
}

#define CVMpopGCRootFrame(ee3_, s3_, curr_frame3_)  \
{                                                   \
    CVMD_gcUnsafeExec((ee3_), {                     \
        CVMpopFrame((s3_), (curr_frame3_));         \
    });                                             \
}

/* Purpose: Gets the current chunk in the stack. */
#define CVMstackGetCurrentChunk(s_)    ((s_)->currentStackChunk)

#ifdef CVM_JIT
/* Purpose: Deletes the specified chunk in the given stack. */
extern void
CVMstackDeleteChunk(CVMStack *stack, CVMStackChunk *chunk);
#endif

#ifdef CVM_JVMTI
/* Purpose: Deletes the last chunk in the given stack. */
extern CVMBool
CVMstackDeleteLastChunk(CVMStack *stack, CVMStackChunk *chunk);
extern void
CVMstackEnableReserved(CVMStack *curStack);
extern void
CVMstackDisableReserved(CVMStack *curStack);
#endif

/* 
 * Walk the frames in a given stack and perform frameAction on each. 
 */
#define CVMstackWalkAllFrames(stack, frameAction) 	       	\
{								\
    CVMStackWalkContext swc;					\
    CVMFrame*           frame;					\
    CVMStackChunk*      chunk;					\
    								\
    CVMstackwalkInit(stack, &swc);				\
    frame = swc.frame;						\
    chunk = swc.chunk;						\
    do {							\
        frameAction;						\
	CVMstackwalkPrev(&swc);					\
	frame = swc.frame;					\
	chunk = swc.chunk;					\
    } while (frame != 0);					\
}

#define CVMstackScanRoots0(ee, stack, callback, data, gcOpts,	\
			   framePreAction, framePostAction)	\
    CVMstackWalkAllFrames(stack, {				\
	framePreAction						\
	(*CVMframeScanners[frame->type])(ee, frame, chunk,	\
			callback, data, gcOpts);		\
	framePostAction						\
    });

#define CVMstackScanRoots(ee, stack, callback, data, gcOpts)	\
    CVMstackScanRoots0(ee, stack, callback, data, gcOpts, {}, {})

/*
 * Scan each root in a freelisting GC root frame.
 */
#define CVMstackScanGCRootFrameRoots(stack, callback, data)	  	  \
    CVMstackWalkAllFrames(stack, {		  	  	 	  \
	CVMwalkRefsInGCRootFrame(frame, ((CVMFreelistFrame*)frame)->vals, \
				 chunk, callback, data, CVM_TRUE);	  \
    });

/* 
 * Walk the references in a given GC root frame. Call 'callback_' on
 * each reference discovered. The GC root frame may or may not have
 * free lists, as indicated by 'hasFreeLists_'.
 */
#define CVMwalkRefsInGCRootFrame(frame_,       				 \
                                 rootData_,				 \
				 chunk_, 				 \
				 callback_, 				 \
				 data_,					 \
				 hasFreeLists_)				 \
{									 \
    CVMStackVal32* tos_ = (frame_)->topOfStack;				 \
									 \
    /*									 \
     * First scan all roots across multiple chunks until we get to the	 \
     * chunk the frame is on.						 \
     */									 \
    while (!CVMaddressInStackChunk(frame_, chunk_)) {			 \
        CVMwalkRefsInGCRootFrameSegment(tos_, (chunk_)->data,		 \
					callback_, data_, hasFreeLists_);\
	(chunk_) = (chunk_)->prev;					 \
	tos_ = (chunk_)->end_data;					 \
    }									 \
    /*									 \
     * Now we are in the first chunk of the frame. Scan from the start	 \
     * of the root area to the top of stack (or the end of chunk, 	 \
     * depending on whether the previous loop executed or not).		 \
     */									 \
    CVMwalkRefsInGCRootFrameSegment(tos_, rootData_,			 \
				    callback_, data_, hasFreeLists_);	 \
    /* %comment rt001 */						 \
}

/*
 * This is a helper macro for CVMwalkRefsInGCRootFrame(). It walks a
 * contiguous segment of roots. It calls 'callback' only on the non-NULL
 * roots.
 *
 * It is also used as the implementation of CVMwalkContiguousRefs()
 * (see below)
 */
#define CVMwalkRefsInGCRootFrameSegment(higherAddr_, 			     \
					lowerAddr_,			     \
					callback_,			     \
					data_,				     \
					hasFreeLists_)			     \
{									     \
    CVMStackVal32* refPtr_;						     \
    for (refPtr_ = (higherAddr_) - 1; refPtr_ >= (lowerAddr_); refPtr_--) {  \
	/* 
	 * referent_ is pointer (what can easily be seen from the fact that  \
	 * refPtr is cast to a pointer to a pointer as formal parameter    \
	 * to callback_). Although referent_ isn't dereferenced it should    \
	 * be of the type CVMAddr which is 4 byte on 32 bit platforms and 8  \
	 * 8 byte on 64 bit platforms to make sure that all address bits     \
	 * get checked in "referent_ != 0".				     \
	 */								     \
        CVMAddr referent_ = *((CVMAddr*)refPtr_);			     \
        if (referent_ != 0) {						     \
	    if (hasFreeLists_) {					     \
		/*							     \
		 * Not a free list element. Walk it.    		     \
		 */							     \
		if ((referent_ & 0x1) == 0) {	 	     		     \
		    (*callback_)((CVMObject**)refPtr_, data_);		     \
		}							     \
	    } else {							     \
		(*callback_)((CVMObject**)refPtr_, data_);     		     \
	    }								     \
        }								     \
    }								 	     \
}

/*
 * Walk numRefs_ contiguous references starting at addr refsAddr_. Call
 * 'callback_' on each non-NULL reference found.
 */
#define CVMwalkContiguousRefs(refsAddr_, numRefs_, callback_, data_) 	      \
    CVMwalkRefsInGCRootFrameSegment((CVMStackVal32*)((refsAddr_)+(numRefs_)), \
				    (CVMStackVal32*)(refsAddr_),	      \
				    (callback_), (data_),		      \
				    CVM_FALSE)

#define CVMwalkOneICell(refICellPtr, callback, data)		\
{								\
    CVMObject* referent = *((CVMObject**)refICellPtr);		\
    if (referent != 0) {					\
	(*callback)((CVMObject**)refICellPtr, data);		\
    }								\
}



/*
 * CVMStackVal32*
 * CVMframeAllocFromTop(CVMExecEnv* ee_, CVMStack* s_, CVMFrame* cur_frame_,
 *                      CVMStackVal32* result_);
 *
 * Allocates a slot from the top of the specified CVMFrame.
 *
 * If unsuccessful, sets result_ to NULL. NO EXCEPTION IS THROWN.
 */
#define CVMframeAllocFromTop(ee_, s_, cur_frame_, result_)		\
{									\
    CVMFrame*      cf_    = (CVMFrame*)(cur_frame_);			\
    CVMStackVal32* tos_   = cf_->topOfStack;				\
    CVMStackVal32* space_;						\
									\
    /* We might end up expanding the stack. Assert GC-safety */	        \
    CVMassert(CVMD_isgcSafe(ee_));					\
									\
    if (CVMcheckCapacity((s_), (tos_), 1)) {				\
        space_ = (tos_);						\
        CVMID_icellSetNull(&space_->ref);				\
        (result_)         = space_;					\
        cf_->topOfStack   = space_ + 1;					\
    } else {								\
        CVMD_gcUnsafeExec(ee_, {					\
            space_ = CVMexpandStack(ee_, (s_), 1,			\
				    CVM_FALSE, CVM_FALSE);		\
            if (space_ == 0) {						\
                (result_) = 0;						\
            } else {							\
                CVMID_icellSetNull(&space_->ref);			\
	        (result_)         = space_;				\
	        cf_->topOfStack   = space_ + 1;				\
                if (tos_ != space_) {					\
                    CVMsetFrameNeedsSpecialHandlingBit(cf_->prevX);	\
               }							\
            }								\
        });								\
    }									\
}

/*
 * CVMStackVal32*
 * CVMframeAlloc(CVMExecEnv* ee_, CVMStack* s_, CVMFreelistFrame* cur_frame_,
 *               CVMStackVal32* result_);
 *
 * Allocates a slot from the CVMFreelistFrame. If a slot is on the free
 * list then it is allocated there. Otherewise it is allocated from
 * the top of the frame. Slots on the free list have their low bit set
 * so it needs to be cleared before returning the slot.
 *
 * If unsuccessful, sets result_ to NULL. NO EXCEPTION IS THROWN.
 */
/* 
 * make CVM ready to run on 64 bit platforms
 * 
 * CVMframeAlloc()
 * (*slot_).next is a native pointer
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMframeAlloc(ee_, s_, cur_frame_, result_)			   \
{									   \
    CVMStackVal32*  slot_  = (cur_frame_)->freeList;			   \
									   \
    /* We might end up expanding the stack. Assert GC-safety */		   \
    CVMassert(CVMD_isgcSafe(ee_));					   \
    if (slot_ != 0) {							   \
	(cur_frame_)->freeList = 					   \
            (CVMStackVal32*)((CVMAddr)((*slot_).next) & ~1); 		   \
        CVMID_icellSetNull(&slot_->ref);				   \
	(result_) = slot_;						   \
        (cur_frame_)->inUse++;						   \
    } else {								   \
	CVMframeAllocFromTop(ee_, (s_), (cur_frame_), (result_));  	   \
        if (result_ != NULL) {						   \
           (cur_frame_)->inUse++;					   \
        }								   \
    }									   \
}

/*
 * void
 * CVMframeFree(CVMFreelistFrame* cur_frame_, CVMStackVal32* slot_);
 *
 * Puts slot_ on the free list of cur_frame_. The low bit of the new
 * free list item is set to indicate that it is a freed item. This is done
 * so when gc scans the frame, it can tell which slots are in use and
 * which slots are free.
 */
/* 
 * make CVM ready to run on 64 bit platforms
 * 
 * CVMframeFree():
 * (slot_)->next is a native pointer (see union CVMStackVal32)
 * freeList is cast to a pointer (CVMStackVal32*)
 *
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMframeFree(cur_frame_, slot_)					   \
{									   \
    CVMStackVal32*  freeList_ = (cur_frame_)->freeList;			   \
									   \
    CVMassert(slot_ != NULL);						   \
    if (((CVMAddr)(slot_)->next & 1) != 0) { /* check if already free */ \
	CVMdebugPrintf(("CVMframeFree: 0x%x is already free\n", slot_));     \
        CVMassert(CVM_FALSE);						   \
    } else {								   \
	(cur_frame_)->freeList = slot_;					   \
	(slot_)->next = (CVMStackVal32*)((CVMAddr)freeList_ | 1);	   \
	(cur_frame_)->inUse--;						   \
    }									   \
}

/*
 * Is the address at the start of the given stack chunk?
 */
#define CVMaddressIsAtStartOfChunk(address_, chunk_) \
    ((CVMStackVal32*)(address_) == &(chunk_)->data[0])

/*
 * Is the address in the given stack chunk?
 */
#define CVMaddressInStackChunk(address_, chunk_)		\
    (((CVMStackVal32*)(address_) <= (chunk_)->end_data) &&	\
     ((CVMStackVal32*)(address_) >= &(chunk_)->data[0]))

/*
 * Initialize a stack before first use, and destroy after use.
 * If CVMinitStack is unsuccessful, it will return CVM_FALSE. No
 * exception is thrown.
 */
extern CVMBool
CVMinitStack(CVMExecEnv *ee, CVMStack* s,
	     CVMUint32 initialStackSize, CVMUint32 maxStackSize,
	     CVMUint32 minStackChunkSize, CVMUint32 initialFrameCapacity,
	     CVMFrameType frameType);

extern void
CVMdestroyStack(CVMStack* stack);

/*
 * Initialize a GC root stack with a singleton frame
 * If CVMinitGCRootStack is unsuccessful, it will return CVM_FALSE. An
 * exception is NOT thrown.
 */
extern CVMBool
CVMinitGCRootStack(CVMExecEnv *ee, CVMStack* s, CVMUint32 initialStackSize, 
		   CVMUint32 maxStackSize, CVMUint32 minStackChunkSize,
		   CVMFrameType frameType);

extern void
CVMdestroyGCRootStack(CVMStack* stack);

/*
 * Dump stack or frame contents for debugging
 */
#ifdef CVM_DEBUG_DUMPSTACK
extern void
CVMdumpStack(CVMStack* s, CVMBool verbose, CVMBool includeData,
	     CVMInt32 frameLimit);

extern CVMStackChunk*
CVMdumpFrame(CVMFrame* frame, CVMStackChunk* startChunk,
	     CVMBool verbose, CVMBool includeData);
#endif

/*
 * Check C stack size availability in the recursive function and
 * throw an exception.
 */
extern CVMBool
CVMCstackCheckSize(CVMExecEnv *ee, CVMUint32 redzone, char *func_name, CVMBool exceptionThrow);

/*
 * A callback function for each frame discovered in CVMscanStack()
 */
typedef void (*CVMFrameCallbackFunc)(CVMFrame *thisFrame);

/* Scans the in-use part of the stack and calls frameCallback for each
 * discovered frame. This should be a function since it's big and
 * slow anyway. 
 */
extern void 
CVMscanStack(CVMStack* stack, CVMFrameCallbackFunc frameCallback);

/* Scans the in-use part of the stack for garbage collection. This
 * should be a function since it's big and slow anyway.  
 */
extern void 
CVMscanStackForGC(CVMStack* stack);

#endif /* _INCLUDED_STACKS_H */
