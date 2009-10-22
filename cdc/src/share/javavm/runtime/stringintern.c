/*
 * @(#)stringintern.c	1.38 06/10/10
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
#include "javavm/export/jni.h"
#include "native/common/jni_util.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/localroots.h"
#include "generated/offsets/java_lang_String.h"
#include "javavm/include/string_impl.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/globals.h"

#undef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))

/*
    #define CVMInternSegmentSize	943 / * or 2003, choose one. * /

    struct CVMInternSegment {
	    struct CVMInternSegment * next;
	    CVMUint32	capacity;
	    CVMUint32	load;
	    CVMUint32	maxLoad;
	    CVMStringICell	data[capacity];
	    CVMUint8	refCount[capacity];
    } CVMInternSegment;

	/ *
	// count == 255 means unused	 (CVMInternUnused)
	// count == 254 means sticky	 (CVMInternSticky)
	// count == 253 means deleted!!	 (CVMInternDeleted)
	* /
*/

/*
 * Acceptable segment sizes.
 * All must be relatively prime to the secondary hash, h1,
 * and, as a bonus, should be relatively prime to 37, used
 * in the initial hash value computation
 * Since 1 <= h1 <= 16, these sizes cannot contain the
 * factors 2, 3, 5, 7, 11, or 13.
 */
static const int CVMinternSegmentSizes[] = {
	943,	/* 41*23 */
	1511,	/* prime */
	2147,   /* 113 * 19 */
	3233,   /* 61 * 53 */
	4891,   /* 73 * 67 */
	7313,   /* 103 * 71 */
	10961,  /* 113 * 97 */
	15943   /* 149 * 107 */
};

#define CVM_INTERN_INITIAL_SEGMENT_SIZE_IDX	0
#define CVM_INTERN_MAX_SEGMENT_SIZE_IDX		7

#ifdef CVM_DEBUG

/*
 * CALL ONLY FROM DEBUGGER: NO LOCKING!!
 */

static void
CVMInternValidateSegment( struct CVMInternSegment * segp, CVMBool verbose ) {
    int capacity, i;
    CVMUint8 *  refCount;
    CVMStringICell * data;
    int nUnused, nSticky, nDeleted, nNoRef, nRef, nBad;
    CVMBool cellIsNull;

    /* do trivial consistancy checks first */
    if ( segp->nextp == NULL ){
	CVMconsolePrintf("Intern segment 0x%x->nextp == NULL\n", segp );
    } else if ( verbose ){
	CVMconsolePrintf("Intern segment 0x%x->nextp != NULL (OK)\n", segp );
    }
    if ( (capacity = segp->capacity) < CVM_INTERN_INITIAL_SEGMENT_SIZE_IDX ){
	/* could be ok for very small romized set, so don't panic! */
	CVMconsolePrintf("Intern segment 0x%x->capacity == %d\n", segp, segp->capacity );
    } else if ( verbose ){
	CVMconsolePrintf("Intern segment 0x%x->capacity == %d (OK)\n", segp, segp->capacity );
    }
    if ( segp->load > capacity ){
	CVMconsolePrintf("Intern segment 0x%x->load == %d > capacity == %d\n", segp,
	    segp->load, capacity );
    } else if (verbose){
	CVMconsolePrintf("Intern segment 0x%x->load == %d (OK)\n", segp, segp->load );
    }
    if ( segp->maxLoad > (((segp->capacity*65)/100)+1) ){
	CVMconsolePrintf("Intern segment 0x%x->maxLoad == %d\n", segp, segp->maxLoad );
    } else if ( verbose ){
	CVMconsolePrintf("Intern segment 0x%x->maxLoad == %d (OK)\n", segp, segp->maxLoad );
    }

    /* now look at all the data. */
    nUnused = nSticky = nDeleted = nNoRef = nRef = nBad = 0;
    refCount = CVMInternRefCount( segp );
    data     = segp->data;
    for ( i = 0; i < capacity; i++ ){
	cellIsNull = CVMID_icellIsNull( &data[i] );
	switch ( refCount[i] ){
	case CVMInternUnused:
	    nUnused += 1;
	    if ( !cellIsNull ){
		CVMconsolePrintf("Intern segment 0x%x->data[%d] Unused but not null\n", segp, i );
		nBad += 1;
	    }
	    break;
	case CVMInternSticky:
	    nSticky += 1;
	    if ( cellIsNull ){
		CVMconsolePrintf("Intern segment 0x%x->data[%d] Sticky but null\n", segp, i );
		nBad += 1;
	    }
	    break;
	case CVMInternDeleted:
	    nDeleted += 1;
	    if ( !cellIsNull ){
		CVMconsolePrintf("Intern segment 0x%x->data[%d] Deleted but not null\n", segp, i );
		nBad += 1;
	    }
	    break;
	default:
	    if ( refCount[i] == 0 )
		nNoRef += 1;
	    else
		nRef += 1;
	    if ( cellIsNull ){
		CVMconsolePrintf("Intern segment 0x%x->data[%d] null with ref count %d\n", segp, i, refCount[i] );
		nBad += 1;
	    }
	    break;
	}
    }
    if ( verbose ){
	CVMconsolePrintf("Intern segment 0x%x has:\n", segp );
	CVMconsolePrintf("    %d unused cells\n", nUnused );
	CVMconsolePrintf("    %d sticky cells\n", nSticky );
	CVMconsolePrintf("    %d deleted cells\n", nDeleted );
	CVMconsolePrintf("    %d 0-ref-count cells\n", nNoRef );
	CVMconsolePrintf("    %d non-0-ref-count cells\n", nRef );
    }
    if ( segp->load != ( nSticky+nDeleted+nNoRef+nRef ) ){
	CVMconsolePrintf("Intern segment 0x%x->load == %d, should be %d\n",
	    segp->load, ( nSticky+nDeleted+nNoRef+nRef ) );
    }
    if ( nBad != 0 ){
	CVMconsolePrintf("Intern segment 0x%x has %d bad cells\n", segp, nBad );
    }
}

/*
 * CALL ONLY FROM DEBUGGER: NO LOCKING!!
 */

void
CVMInternValidateAllSegments( CVMBool verbose ) {
    CVMInternSegment * curSeg, *nextSeg;
    nextSeg = (CVMInternSegment*)&CVMInternTable; /* cast away const */
    do {
	curSeg = nextSeg;
	CVMInternValidateSegment( curSeg, verbose );
	nextSeg = CVMInternNext(curSeg);
    } while ( nextSeg != NULL );
}

/*
 * CALL ONLY FROM DEBUGGER!!
 * Not a good user of the memory interface!
 * Not suitable for any general purpose whatsoever.
 * Assumes that we really have ASCII strings.
 */

static void
CVMInternPrintString( CVMStringICell *icellp ){
    struct java_lang_String *stringp = *(struct java_lang_String**)icellp;
    struct CVMArrayOfChar   *datap;
    int    i, n, off;
#undef  MAXINTERNPRINTLENGTH
#define MAXINTERNPRINTLENGTH 20
    char   c[MAXINTERNPRINTLENGTH+1];

    if ( stringp == NULL ){
	CVMconsolePrintf("<null>");
	return;
    }
    n = stringp->count;
    off = stringp->offset;
    datap = *(CVMArrayOfChar**)&(stringp->value);
    CVMconsolePrintf("(%d)", n );
    if ( n > MAXINTERNPRINTLENGTH )
	n = MAXINTERNPRINTLENGTH;
    for ( i = 0; i < n ; i ++ ){
	c[i] = (char)(datap->elems[off++]);
    }
    c[i] = 0;
    CVMconsolePrintf("%s", c );
    if ( n < stringp->count )
	CVMconsolePrintf("...");
}

/*
 * CALL ONLY FROM DEBUGGER: NO LOCKING!!
 * ASSUMES SEGMENT VALIDATED ALREADY!
 */

void
CVMInternPrintSegment( struct CVMInternSegment * segp ) {
    int capacity, i;
    CVMUint8 *  refCount;
    CVMStringICell * data;

    capacity = segp->capacity;

    /* now look at all the data. */
    refCount = CVMInternRefCount( segp );
    data     = segp->data;

    CVMconsolePrintf("Printing all Strings in 0x%x\n", segp );

    for ( i = 0; i < capacity; i++ ){
	switch ( refCount[i] ){
	case CVMInternUnused:
	case CVMInternDeleted:
	    break;
	case CVMInternSticky:
	    CVMconsolePrintf("    S    ");
	    CVMInternPrintString( &data[i] );
	    CVMconsolePrintf("\n");
	    break;
	default:
	    CVMconsolePrintf("%4d    ", refCount[i] );
	    CVMInternPrintString( &data[i] );
	    CVMconsolePrintf("\n");
	    break;
	}
    }
}

#endif /* CVM_DEBUG */

typedef CVMBool (*helperFunction)( CVMExecEnv* ee,  CVMStringICell * candidate, void * stuff);
typedef void    (*consumerFunction)( CVMExecEnv* ee,  CVMStringICell * stringCell, CVMUint8 * refCell, void * stuff);

/* all our forwards for all our helper functions here */
static CVMInternSegment * allocateNewSegment();

static CVMBool
internJavaCompare( CVMExecEnv* ee,  CVMStringICell * candidate, void * stuff);
static CVMBool
internJavaProduce( CVMExecEnv *ee, CVMStringICell * target, void * stuff );
static void
internJavaConsume( CVMExecEnv* ee,  CVMStringICell * stringCell, CVMUint8 * refCell, void * stuff);

static CVMBool
internUTFCompare( CVMExecEnv* ee,  CVMStringICell * candidate, void * stuff);
static CVMBool
internUTFProduce( CVMExecEnv *ee, CVMStringICell * target, void * stuff );
static void
internUTFConsume( CVMExecEnv* ee,  CVMStringICell * stringCell, CVMUint8 * refCell, void * stuff);

/*
 * Max chars to look at when computing simple string hash value.
 * This constant is known by JavaCodeCompact.
 */
#undef  MAX_HASH_LENGTH
#define MAX_HASH_LENGTH		16

/*
 * If gc latency is of concern, you may want to reduce these buffer sizes.
 * They affect the maximum amount of time spent gc-unsafe while copying
 * characters from the heap to a local buffer.
 * Note that PREFIX_BUFFER_SIZE MUST BE > MAX_HASH_LENGTH, which is a constant
 * of the implementation shared with the preLoader!
 */

#undef  PREFIX_BUFFER_SIZE
#define PREFIX_BUFFER_SIZE	100
#undef  ITERATION_BUFFER_SIZE
#define ITERATION_BUFFER_SIZE	50

/*
 * This intern function is shared by the the function that
 * takes a java.lang.String as an argument (called by Java code)
 * and the one that take a UTF8 string as an argument
 * (called by the linker). In order to allow us to inline
 * a simple (or usually, all) comparison, we require that the
 * string's prefix be provided us in a buffer. The length provided
 * (in bufferLength) must be MIN( fullStringLength, PREFIX_BUFFER_SIZE ).
 * 
 * If the string is actually longer, the compare function will be called
 * to finish comparing the candidate to the input data.
 * compareData will be passed along to it.
 *
 * If the string is not alread in the table, the function produceData
 * will be called to (if necessary) construct the String, and fill
 * the slot in the table.
 *
 * a note on reference counts:
 * 255:	unused slot, unreferenced
 * 254: sticky object. Immortal. ROMized or very popular
 * 253: used slot, but was deleted from table when reference count
 *	went down. Needed to make hashing work!
 * [1, 252]: produced by class loader from UTF8 string. We trust
 *	that class UNloading will cause these to be decremented.
 */


static void
internInner(
    CVMExecEnv *	ee,
    CVMJavaChar 	buffer[],
    CVMSize		bufferLength,
    CVMSize		fullLength,
    helperFunction 	compare,
    helperFunction 	produce,
    consumerFunction	consume,
    void *		callbackData
){

    CVMUint32		h, h1;
    CVMSize 		i, j, n;
    CVMSize		slotWithOpening=0;
    CVMSize		capacity;
    CVMUint8		refCount;
    CVMUint8*		refArray;
    CVMInternSegment*   curSeg,
		    *	nextSeg,
		    *   segWithOpening = NULL;
    CVMStringICell  *	candidate;


    /* calculate hash code.
     * This is similar to the one in java.lang.String, but this
     * is just a coincidence. Make sure that the ROMizer uses the
     * same one! (in order to make this easier, we will mask off
     * the high order bit.)
     */
    n = MIN(bufferLength, MAX_HASH_LENGTH);
    h = 0;
    for ( i = 0; i < n; i++ ){
	h = (h*37) + buffer[i];
    }
    h &= ~0x80000000;
    h1 = (h&15)+1;

    /*
     * Look through the segments until we either find a match.
     * or run out of places to look. Keep track of any deleted
     * cells we find along the way, because if we have to
     * do an insertion, we'd prefer to insert in one of them.
     */

    /* too bad we have to lock ourselves against mutation */
    CVMsysMutexLock(ee, &CVMglobals.internLock );
    
    nextSeg = (CVMInternSegment*)&CVMInternTable; /* cast away const */
    do {
	curSeg = nextSeg;
	capacity = curSeg->capacity;
	refArray = CVMInternRefCount(curSeg);
	i = h % capacity; /* this is where we start looking */

	while ( (refCount=refArray[i]) != CVMInternUnused ){
	    if ( refCount == CVMInternDeleted ){
		if ( segWithOpening==NULL){
		    segWithOpening = curSeg;
		    slotWithOpening = i;
		}
	    } else {
		CVMJavaInt candidateLength;	
		CVMJavaChar candidateData[PREFIX_BUFFER_SIZE];

		candidate = &(curSeg->data[i]);
		CVMID_fieldReadInt( ee, candidate,
		    CVMoffsetOfjava_lang_String_count,
		    candidateLength );
		if ( candidateLength == fullLength ){
		    /* look at a few of the characters. */
		    if ( fullLength > 0 ){
			/* note:
			 * The following is highly suspect: it gets
			 * the array pointer and the offset from the
			 * string object, then extracts some characters
			 * from that array. BUGS BE HERE.
			 * Furthermore, I copied this several places below.
			 * A bug for one is a bug for all!
			 */
			CVMD_gcUnsafeExec(ee,{
			    CVMObject *     stringDirect;
			    CVMObject *     theChars;
			    CVMJavaInt      offset;

			    stringDirect = CVMID_icellDirect(ee, candidate);
			    CVMD_fieldReadInt( stringDirect, 
				CVMoffsetOfjava_lang_String_offset,
				offset );
			    CVMD_fieldReadRef( stringDirect, 
				CVMoffsetOfjava_lang_String_value,
				theChars );
			    CVMD_arrayReadBodyChar( candidateData, (CVMArrayOfChar*)theChars, offset, bufferLength );
			});
			/* now that that trauma is over, do the short
			 * data compare. 
			 */
			for ( j = 0; j < bufferLength; j++ ){
			    if ( candidateData[j] != buffer[j] )
				goto comparisonFailure;
			}
		    }
		    /*
		     * So far so good
		     */
		    if ( (bufferLength==fullLength) || ((*compare)( ee,  candidate, callbackData ) ) ){
			/* DEBUG CVMconsolePrintf(" >FOUND<"); */
			goto found;
		    }
		}
	    }
	comparisonFailure:
	    i += h1;
	    if ( i >= capacity )
		i -= capacity;
	    /* continue looking in this segment */
	}
	/*
	 * Did not find it in this segment.
	 * Either go on to the next segment, or consider
	 * inserting a new String.
	 */
	nextSeg = CVMInternNext(curSeg);
    } while ( nextSeg != NULL );
    /*
     * String not found.
     * Insert it.
     * If we found a deleted slot, fill it.
     * Otherwise, add at curSeg->data[i], which is *candidate
     * If we do the latter, this segment might get too full,
     * so we might have to allocate a new one.
     */
    if ( segWithOpening == NULL ) {
	/* insert in current segment */
	if ( curSeg->load > curSeg->maxLoad ){
	    CVMInternSegment * newSeg;
	    if ( (CVMInternNext(curSeg) = newSeg = allocateNewSegment()) == NULL ){
		/*
		 * running out of memory. fail here by returning NULL
		 */
		candidate = NULL;
		goto failure;
	    }
	    curSeg = newSeg;
	    i = h % curSeg->capacity;
	}
	curSeg->load += 1;
    } else {
	/* Insert in some slot we looked at before
	 * (though possibly in the current segment )
	 * (Don't increase the load factor for this segment, as it
	 * was not decremented when the slot was vacated.)
	 */
	curSeg = segWithOpening;
	i = slotWithOpening;
    }

    /*
     * All insertions get an initial refCount of 0. If we have
     * incrementRefcount == TRUE, then we increment it in 'found:'.
     */
    CVMInternRefCount(curSeg)[i] = 0;
    candidate = &(curSeg->data[i]);

    (*produce)( ee, candidate, callbackData );

found:
    (*consume)( ee,  candidate, &CVMInternRefCount(curSeg)[i], callbackData);

    /* Indicate that we have interned another string: */
    CVMglobals.gcCommon.stringInternedSinceLastGC = CVM_TRUE;

failure:

    CVMsysMutexUnlock(ee, &CVMglobals.internLock );

}

static CVMInternSegment *
allocateNewSegment(){
    CVMInternSegment *	seg;
    size_t		dataSize = CVMinternSegmentSizes[CVMglobals.stringInternSegmentSizeIdx];
    size_t		roundedRefSize;
    size_t		allocationBytes;

    /* 
     * Make sure that 'nextp' is correctly aligned for pointer size access.
     */
    roundedRefSize = (dataSize+(sizeof(void*)-1))&~(sizeof(void*)-1);
    allocationBytes = sizeof(CVMInternSegment)+(dataSize*sizeof(CVMStringICell))
		      + roundedRefSize;
    /*
     * since sizeof CVMInternSegment had 1 CVMStringICell in it, I don't think
     * we need to account for the extra next pointer cell as well!
     */

    seg = (CVMInternSegment *)calloc( allocationBytes, 1 );
    if ( seg == NULL ) return NULL;
    seg->capacity = (CVMUint32)dataSize;
    seg->load  = 0;
    /*
     * make sure maxLoad is even on dynamically allocated
     * segments.
     */
    seg->maxLoad = (CVMUint32)(( dataSize*65)/100)&~1;
    seg->nextp = (CVMInternSegment**)&(CVMInternRefCount(seg)[roundedRefSize]);
    CVMInternNext(seg)     = NULL;

    memset( CVMInternRefCount(seg), CVMInternUnused, seg->capacity );

    /** Compute size of next allocation */
    if (++CVMglobals.stringInternSegmentSizeIdx > CVM_INTERN_MAX_SEGMENT_SIZE_IDX){
	CVMglobals.stringInternSegmentSizeIdx = CVM_INTERN_MAX_SEGMENT_SIZE_IDX;
    }

    return seg;
}

struct stringInternData{
    CVMStringICell *	  srcString;
    CVMArrayOfCharICell * body;
    CVMInt32	          offset;
    int		  	  length;
    int		  	  bufferLength;
    CVMJavaChar 	  buffer[PREFIX_BUFFER_SIZE];
    JNIEnv *		  jniEnv;
    jobject		  result;
};

/*
 * The JNI entry point: java.lang.String.intern.
 * by way of JVM_InternString
 */
JNIEXPORT jobject JNICALL
CVMstringIntern(JNIEnv *env, jstring thisString){
    CVMExecEnv *ee;
    struct stringInternData d;

    if ( CVMID_icellIsNull( thisString ) ){
	return (jobject)&CVMID_nullICell; /* don't bother me */
    }
    /*
     * Extract from the Java string:
     * the length, underlying char array & offset
     * and the first few characters (for fast comparing)
     * This is very similar to the code we use above to look at the
     * comparand.
     */
    ee = CVMjniEnv2ExecEnv(env);
    d.jniEnv = env;
    d.result = NULL;
    CVMID_localrootBegin( ee );{
	CVMID_localrootDeclare( CVMArrayOfCharICell, body );
	d.srcString = (CVMStringICell*)thisString;
	d.body = body;
	CVMD_gcUnsafeExec(ee,{
	    CVMObject * 	stringDirect;
	    CVMObject * 	theChars;
	    stringDirect = CVMID_icellDirect(ee, thisString);
	    CVMD_fieldReadInt( stringDirect, 
		CVMoffsetOfjava_lang_String_offset,
		d.offset );
	    CVMD_fieldReadInt( stringDirect, 
		CVMoffsetOfjava_lang_String_count,
		d.length );
	    CVMD_fieldReadRef( stringDirect, 
		CVMoffsetOfjava_lang_String_value,
		theChars );
	    CVMID_icellSetDirect( ee, body, (CVMArrayOfChar*)theChars );
	    d.bufferLength = MIN(d.length, PREFIX_BUFFER_SIZE);
	    if ( d.bufferLength > 0 )
		CVMD_arrayReadBodyChar( d.buffer, (CVMArrayOfChar*)theChars, d.offset, d.bufferLength );
	});

	/*DEBUG
	     {
		char printbuffer[ PREFIX_BUFFER_SIZE+1 ];
	       int q;
	       for ( q = 0; q < d.bufferLength; q++ )
		    printbuffer[q] = d.buffer[q];
	       printbuffer[q] = 0;
	       CVMconsolePrintf("Interning Java String beginning with %s", printbuffer );
	     }
	*/

	/*
	 * need the local root to the char array live across this call.
	 */
	internInner( ee,  d.buffer, d.bufferLength, d.length,
			internJavaCompare, internJavaProduce, internJavaConsume, &d );
    } CVMID_localrootEnd();
    /*DEBUG CVMconsolePrintf("... at cell %x\n", d.result );*/
    /*
     * detect the out-of-memory problem and throw.
     * (copied from jvm.c)
     */

    if ( d.result==NULL ){
	CVMthrowOutOfMemoryError(ee, "interning Java String");
    }

    return (jobject)d.result;
}

/*
 * do full compare. Assume that the first bufferLength things have already
 * been found equal. To speed up the process(?), we will extract characters
 * a few at a time ( few > 1 ) and compare them.
 * (We already know the string is "long", else we wouldn't be here.)
 *
 * Setup: extract array body and offset from candidate.
 * Then:
 *	load a buffer of characters from each array.
 *	compare until done or until different.
 *
 * By the way: DO NOT change anything in dp. If this compare fails, it will
 * be used again!
 */

static CVMBool
internJavaCompare( CVMExecEnv* ee,  CVMStringICell * candidate, void * stuff){
     struct stringInternData* dp = (struct stringInternData *)stuff;
     CVMBool 		returnValue;
     CVMJavaInt		candidateOffset;
     CVMJavaInt		ourOffset;
     CVMJavaChar	candidateBuffer[ ITERATION_BUFFER_SIZE ];
     CVMJavaChar	ourBuffer[ ITERATION_BUFFER_SIZE ];
     int		thisLength;
     int		lengthRemaining;

    CVMID_localrootBegin( ee );{
	CVMID_localrootDeclare( CVMArrayOfCharICell, candidateBody );

	CVMID_fieldReadInt( ee, candidate, CVMoffsetOfjava_lang_String_offset,
		candidateOffset );
	CVMID_fieldReadRef( ee, candidate, 
		CVMoffsetOfjava_lang_String_value,
		(CVMObjectICell*)candidateBody );
	returnValue = CVM_TRUE;
	/* take it as a given that the first d.bufferLength
	 * characters are equal. They need not be inspected again
	 */
	candidateOffset += dp->bufferLength;
	ourOffset = dp->offset + dp->bufferLength;
	lengthRemaining = dp->length - dp->bufferLength;

	while( (lengthRemaining > 0) && ( returnValue == CVM_TRUE) ){
	    int i;
	    thisLength = MIN( lengthRemaining, ITERATION_BUFFER_SIZE );
	    CVMD_gcUnsafeExec(ee,{
		CVMArrayOfChar* theChars;

		theChars = CVMID_icellDirect(ee, candidateBody);
		CVMD_arrayReadBodyChar( candidateBuffer, theChars, candidateOffset, thisLength );

		theChars = CVMID_icellDirect(ee, dp->body);
		CVMD_arrayReadBodyChar( ourBuffer, theChars, ourOffset, thisLength );
	    });
	    for ( i = 0; i < thisLength; i++ ){
		if ( candidateBuffer[i] != ourBuffer[i] ){
		    returnValue = CVM_FALSE;
		    break;
		}
	    }
	    lengthRemaining -= thisLength;
	    candidateOffset += thisLength;
	    ourOffset       += thisLength;
	}
    } CVMID_localrootEnd();
    return returnValue;
}

static CVMBool
internJavaProduce( CVMExecEnv *ee, CVMStringICell * target, void * stuff ){
    struct stringInternData* dp = (struct stringInternData *)stuff;
    CVMID_icellAssign( ee, target, dp->srcString );
    return CVM_TRUE;
}

static void
internJavaConsume( CVMExecEnv* ee,  CVMStringICell * stringCell, CVMUint8 * refCell, void * stuff){
    struct stringInternData* dp = (struct stringInternData *)stuff;
    dp->result = (*(dp->jniEnv))->NewLocalRef( dp->jniEnv, (jobject) stringCell );
}


/*
 * This is the entry point called by the class loader, when it has
 * a UTF8 string representation of a String to intern. It doesn't
 * require actually constructing the java.lang.String object until
 * we know it is required.
 */
struct utfInternData{
    const CVMUtf8 *  	  srcString;
    CVMSize		  srcLength;
    CVMSize		  srcPosition;
    CVMSize		  targetLength;
    CVMSize		  bufferLength;
    CVMJavaChar 	  buffer[PREFIX_BUFFER_SIZE];
    CVMStringICell *	  allocatedString;
    CVMArrayOfCharICell * allocatedArray;
    CVMStringICell *	  result;
};

CVMStringICell *
CVMinternUTF(
    CVMExecEnv *     ee,
    const CVMUtf8 *  src
){
    struct utfInternData d;
    if ( src == NULL ){
	return NULL; /* don't bother me */
    }
    /*
     * Extract from the UTF string:
     * the lengths ( both UTF length and Unicode length),
     * and the first few characters (for fast comparing)
     */
    d.srcString    = src;
    d.srcLength    = strlen( src );
    d.targetLength = CVMutfLength( src );
    d.bufferLength = MIN(d.targetLength, PREFIX_BUFFER_SIZE);
    d.srcPosition = CVMutfCountedCopyIntoCharArray( src, d.buffer, d.bufferLength );
    d.result = NULL;

    /*DEBUG
      {
     	char printbuffer[ PREFIX_BUFFER_SIZE+1 ];
     	int q;
     	for ( q = 0; q < d.bufferLength; q++ )
     	    printbuffer[q] = d.buffer[q];
     	printbuffer[q] = 0;
     	CVMconsolePrintf("Interning UTF beginning with %s", printbuffer );
      }
    */

    CVMID_localrootBegin(ee){
	/*
	 * Pre-allocated the string memory, as we will likely
	 * need it and don't want to unlock this table in order
	 * to allocate later.
	 */
	CVMID_localrootDeclare(CVMStringICell, allocatedString);
	CVMID_localrootDeclare(CVMArrayOfCharICell, allocatedArray);

	CVMID_allocNewInstance( 
		ee, (CVMClassBlock*)CVMsystemClass(java_lang_String), allocatedString);
	CVMID_allocNewArray(
		ee, CVM_T_CHAR, 
		(CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_CHAR],
		(CVMJavaInt)d.targetLength, (CVMObjectICell*)allocatedArray);

	if (CVMID_icellIsNull(allocatedString) || CVMID_icellIsNull(allocatedArray)) {
	    /* null result if anything failed. */
	    goto failure;
	}

	d.allocatedString = allocatedString;
	d.allocatedArray  = allocatedArray;
	/*
	 * call the intern-er with this information
	 */
	internInner( ee,  d.buffer, d.bufferLength, d.targetLength,
			    internUTFCompare, internUTFProduce, internUTFConsume, &d );
	/* DEBUG  CVMconsolePrintf("... at cell %x\n", d.result );*/
failure: ;
	/*
	 * At this point, the local roots for allocatedString and allocatedArray
	 * vanish. Either these objects are in the table, or they become
	 * garbage.
	 */
    }CVMID_localrootEnd();
    return d.result;
}

/*
 * do full compare. Assume that the first bufferLength things have already
 * been found equal. To speed up the process(?), we will extract characters
 * a few at a time ( few > 1 ) and compare them.
 * (We already know the string is "long", else we wouldn't be here.)
 *
 * Setup: extract array body and offset from candidate.
 * Then:
 *	load a buffer of characters from each array.
 *	compare until done or until different.
 *
 * By the way: DO NOT change anything in dp. If this comapare fails, it will
 * be used again!
 */

static CVMBool
internUTFCompare( CVMExecEnv* ee,  CVMStringICell * candidate, void * stuff){
    struct utfInternData* dp = (struct utfInternData *)stuff;
    CVMBool 		returnValue;
    CVMSize		candidateOffset;
    const CVMUtf8 *	srcPos;
    CVMJavaChar		candidateBuffer[ ITERATION_BUFFER_SIZE ];
    CVMJavaChar		ourBuffer[ ITERATION_BUFFER_SIZE ];
    int			thisLength;
    int			lengthRemaining;

    srcPos = dp->srcString+dp->srcPosition;

    CVMID_localrootBegin( ee );{
	CVMID_localrootDeclare( CVMArrayOfCharICell, candidateBody );

	CVMID_fieldReadInt( ee, candidate, CVMoffsetOfjava_lang_String_offset,
		candidateOffset );
	CVMID_fieldReadRef( ee, candidate, 
		CVMoffsetOfjava_lang_String_value,
		(CVMObjectICell*)candidateBody );
	returnValue = CVM_TRUE;
	/* take it as a given that the first dp->bufferLength
	 * characters are equal. They need not be inspected again
	 */
	candidateOffset += dp->bufferLength;
	lengthRemaining = (int)(dp->targetLength - dp->bufferLength);

	while( (lengthRemaining > 0) && ( returnValue == CVM_TRUE) ){
	    int i;
	    thisLength = MIN( lengthRemaining, ITERATION_BUFFER_SIZE );
	    CVMD_gcUnsafeExec(ee,{
		CVMArrayOfChar* theChars;

		theChars = CVMID_icellDirect(ee, candidateBody);
		CVMD_arrayReadBodyChar( candidateBuffer, theChars, (CVMJavaInt)candidateOffset, thisLength );
	    });
	    srcPos += CVMutfCountedCopyIntoCharArray( srcPos, ourBuffer, thisLength);
	    for ( i = 0; i < thisLength; i++ ){
		if ( candidateBuffer[i] != ourBuffer[i] ){
		    returnValue = CVM_FALSE;
		    break;
		}
	    }
	    lengthRemaining -= thisLength;
	    candidateOffset += thisLength;
	}
    } CVMID_localrootEnd();
    return returnValue;
}

static CVMBool
internUTFProduce( CVMExecEnv *ee, CVMStringICell * target, void * stuff ){
    struct utfInternData* dp = (struct utfInternData *)stuff;
    /* These are all for the character transfer loop, if necessary */
    int	 nChars;	/* characters this iteration */
    int  charsLeft;	/* Unicode chars left to transfer */
    int  destPos;	/* destination index of this transfer */
    const char * srcPos; /* UTF src pointer for this transfer */
    CVMJavaChar	transferBuffer[ ITERATION_BUFFER_SIZE ];

    /* Finish turning the UTF string into a Java String.
     * Its storage has already been allocated, but none of the slots
     * has been filled in. Construct the String object, then
     * fill in the Java char array from the buffer and the UTF representation
     * Most of this is stolen from CVMnewString in utils.c
     */
    CVMID_fieldWriteRef(ee, dp->allocatedString, 
			CVMoffsetOfjava_lang_String_value,
			(CVMObjectICell*)(dp->allocatedArray));
    CVMID_fieldWriteInt(ee, dp->allocatedString,
			CVMoffsetOfjava_lang_String_offset,
			0);
    CVMID_fieldWriteInt(ee, dp->allocatedString, 
			CVMoffsetOfjava_lang_String_count,
			(CVMJavaInt)dp->targetLength);
    /*
     * Shovel characters from here to there. 
     * Start with the prefix buffer.
     * After that, do no more than ITERATION_BUFFER_SIZE at a time,
     * the size of which can be tuned to modify gc-latency
     */
    CVMD_gcUnsafeExec(ee, {
	CVMArrayOfChar* charArray =
	    CVMID_icellDirect(ee, dp->allocatedArray);
	CVMD_arrayWriteBodyChar(dp->buffer, charArray, 0, dp->bufferLength);
    });
    if ( (charsLeft = (int)(dp->targetLength-dp->bufferLength)) > 0 ){
	/* there's more */
	destPos = (int)dp->bufferLength;
	srcPos  = dp->srcString+dp->srcPosition;
	while (charsLeft > 0 ){
	    nChars  = MIN(charsLeft, ITERATION_BUFFER_SIZE);
	    srcPos += CVMutfCountedCopyIntoCharArray(
			srcPos, transferBuffer, nChars );
	    CVMD_gcUnsafeExec(ee, {
		CVMArrayOfChar* charArray =
		    CVMID_icellDirect(ee, dp->allocatedArray);
		CVMD_arrayWriteBodyChar(transferBuffer, charArray, destPos, nChars );
	    });
	    destPos += nChars;
	    charsLeft -= nChars;
	}
    }
    /*
     * Finally, assign reference to the String object to the
     * result location.
     */
    CVMID_icellAssign( ee, target, dp->allocatedString );

    return CVM_TRUE;
}

static void
internUTFConsume( CVMExecEnv* ee,  CVMStringICell * stringCell, CVMUint8 * refCell, void * stuff){
    struct utfInternData* dp = (struct utfInternData *)stuff;
    int n = *refCell;
    if ( n != CVMInternSticky ){
	/* Don't even try to touch alread-sticky counts */
	n  += 1;
	if ( n >= CVMInternDeleted ) n = CVMInternSticky;
	*refCell = (CVMUint8)n;
    }
    dp->result = stringCell;
}


static CVMUint8*
findRefCount( CVMStringICell * targetcell ){
    /*
     * determine which segment this is in
     * determine its offset.
     * calculate the address of its corresponding reference count cell.
     */
    CVMInternSegment * thisSeg = (CVMInternSegment*)&CVMInternTable; /* cast away const */
    while ( thisSeg != NULL ){
	CVMStringICell * data = &(thisSeg->data[0]);
	if ( (data <= targetcell) && ( targetcell < &data[thisSeg->capacity] ) ){
	    int i;
	    i = (int)(targetcell - data);
	    return &( CVMInternRefCount(thisSeg)[i] );
	}
	thisSeg = CVMInternNext(thisSeg);
    }
    /* oops didn't find! */
    return NULL;
}
    

/*
 * Just because the classloader doesn't reference a cell
 * doesn't make it dead. Leave that for the garbage collector to decide!
 */
void
CVMinternDecRef(
    CVMExecEnv *     ee,
    CVMStringICell * target
){
    CVMUint8* rcp = findRefCount( target );
    unsigned  rcount = *rcp;
    if ( rcount < CVMInternDeleted ){
	CVMsysMutexLock(ee, &CVMglobals.internLock );
	rcount = *rcp;
	if ( rcount < CVMInternDeleted ){
	    if ( rcount > 0 ){ /* == 0 is a very unnatural condition */
		*rcp = rcount-1;
	    }
	}
	CVMsysMutexUnlock(ee, &CVMglobals.internLock );
    }
}

/*
 * CVMscanInternedStrings is used to scan the intern table,
 * calling the func on every live (non-null, non-deleted) entry.
 *
 * Used by GC.
 * Assumes intern table already locked, so it is safe to iterate
 * without further locking.
 */
void
CVMscanInternedStrings( CVMRefCallbackFunc func, void * data ){
    CVMInternSegment * thisSeg = (CVMInternSegment*)&CVMInternTable; /* cast away const */

    /* Skip the first segment because it only contains ROMized strings which
       does not need to be scanned because they will never be moved nor
       collected: */
    thisSeg = CVMInternNext(thisSeg);

    while ( thisSeg != NULL ){
	int i;
	int thisSegCapacity = thisSeg->capacity;
	CVMUint8 * refArray = CVMInternRefCount( thisSeg );
	for ( i=0 ; i < thisSegCapacity ; i ++ ){
	    CVMStringICell * cellp;
	    switch ( refArray[i] ){
	    case CVMInternUnused:
	    case CVMInternDeleted:
		continue;
	    }
	    cellp = &(thisSeg->data[i]);
	    CVMassert( !CVMID_icellIsNull( cellp ) );
	    /* here for sticky, 0 and none of the above non-zero */

            /* NOTE: All ROmized strings are constant pool strings, and hence
               will only appear in the first segment which we skip above.
               Hence, the strings we see here should not be ROMized: */
            CVMassert(!CVMobjectIsInROM(*((CVMObject**)cellp)));
            func( (CVMObject**)cellp, data);
	}
	thisSeg = CVMInternNext(thisSeg);
    }
}

/*
 * CVMdeleteUnreferencedStrings called during garbage collection:
 * iterate over all the intern table entries. For any that have a 0
 * reference count, discover if they have Java references by calling
 * queryStringReferenced. If not, then the String will be deleted from
 * the intern table.
 *
 * Assumes intern table already locked, so it is safe to change
 * entries.
 */

void
CVMinternProcessNonStrong(CVMRefLivenessQueryFunc isLive,
			  void* isLiveData,
			  CVMRefCallbackFunc transitiveScanner,
			  void* transitiveScannerData)
{
    CVMInternSegment * thisSeg = (CVMInternSegment*)&CVMInternTable; /* cast away const */

    /* Skip the first segment because it only contains ROMized strings which
       does not need to be scanned because they will never be moved nor
       collected.  NOTE: The refCount for ROMized strings whose
       refCount is always sticky anyway: */
    thisSeg = CVMInternNext(thisSeg);

    while ( thisSeg != NULL ){
	int i;
	int thisSegCapacity = thisSeg->capacity;
	CVMUint8 * refArray = CVMInternRefCount( thisSeg );
	for ( i=0 ; i < thisSegCapacity ; i ++ ){
	    CVMStringICell * cellp;
	    if ( refArray[i] != 0 ){
		/*
		 * already deleted,
		 * or still unused,
		 * or sticky
		 * or having a non-zero ref count so we know
		 * they're live
		 */
		continue;
	    }
	    /* here only for a non-null cell with a 0 reference count */
	    /* find out what we are supposed to do */
	    cellp = &(thisSeg->data[i]);
	    CVMassert( !CVMID_icellIsNull( cellp ) );
	    if ( !isLive( (CVMObject**)cellp, isLiveData ) ){
		/*  DEAD cell in the table. Delete it */
		refArray[i] = CVMInternDeleted;
		CVMID_icellSetNull( cellp );
	    }
	}
	thisSeg = CVMInternNext(thisSeg);
    }
    /*
     * Now that we've weeded out the 'unwanted', make the rest alive
     * This is the only part of interned string scanning that can
     * potentially cause an undo.
     */
    /* NOTE: interned strings with a non-zero reference count can be kept
       alive even though the GC root scan shows no reference to it.  Have
       to tell GC to keep it alive: */
    CVMscanInternedStrings(transitiveScanner, transitiveScannerData);
}


/*
 * Roll back scanned interned strings
 */
void
CVMinternRollbackHandling(CVMExecEnv* ee,
			  CVMGCOptions* gcOpts,
			  CVMRefCallbackFunc rootRollbackFunction,
			  void* rootRollbackData)
{
    /*
     * Scan the undeleted parts of the intern string table
     * and roll each back.
     */
    CVMscanInternedStrings(rootRollbackFunction, rootRollbackData);
}
			  
void
CVMinternInit(){
    CVMglobals.stringInternSegmentSizeIdx = CVM_INTERN_INITIAL_SEGMENT_SIZE_IDX;
}

/*
 * When destroying the VM, give back dynamically allocated resources.
 * Java objects will be dealt with elsewhere, so just give back the
 * intern table segments here.
 * Preallocated segments have an odd maxLoad.
 * Dynamically allocated segments have an even maxLoad.
 */
void
CVMinternDestroy(){
    CVMInternSegment * thisSeg;
    CVMInternSegment * nextSeg = (CVMInternSegment*)&CVMInternTable; /* cast away const */
    while ( nextSeg != NULL ){
	thisSeg = nextSeg;
	nextSeg = CVMInternNext(thisSeg);
	if ( !(thisSeg->maxLoad & 1) ){
	    free( thisSeg );
	}
    }
}

