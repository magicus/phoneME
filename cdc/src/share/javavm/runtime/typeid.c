/*
 * @(#)typeid.c	1.114 06/10/10
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
#include "javavm/include/typeid.h"
#include "javavm/include/typeid_impl.h"
#include "javavm/include/globals.h"
#include "javavm/include/signature.h"
#include "javavm/include/utils.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/preloader.h"
#include "javavm/include/packages.h"
#include "javavm/include/clib.h"
#include "javavm/include/porting/system.h"

#define isTableEntry( i ) (CVMtypeidIsBigArray(i)||(((i)>CVMtypeidLastScalar)&&(i)<(1<<CVMtypeidArrayShift)))

/*
 * A limitation of the implementation
 * Since method sig table is private, no need to expose this.
 */
#define CVM_TYPEID_MAX_SIG 0xfffe


/**************************************************************
 * The same scheme of variable-length tables are used several
 * places: for memberNames, for both field and method types.
 * The same indexation idiom is used throughout. If this were Java
 * we'd use subclassing or something to share. Here, we use type casting.
 */

struct genericTableHeader {
    SEGMENT_HEADER( struct genericTableSegment )
};

struct genericTableSegment {
    SEGMENT_HEADER( struct genericTableSegment )
    char	genericData[1];
};

struct genericTableEntry {
    COMMON_TYPE_ENTRY_HEADER
};

#ifdef CVM_DEBUG
static struct idstat{
    int nNamesAdded;
    int nNamesDeleted;
    int nClassesAdded;
    int nClassesDeleted;
    int nArraysAdded;
    int nArraysDeleted;
    int nPkgAdded;
    int nPkgDeleted;
    int nMethodSigsAdded;
    int nMethodSigsDeleted;
    int nMethodFormsAdded;
    int nMethodFormsDeleted;
    int nMethodDetailsAdded;
    int nMethodDetailsDeleted;
}idstat = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static const char * const idstatName[] = {
    "member names added:",
    "           deleted:",
    "  class id's added:",
    "           deleted:",
    "  array id's added:",
    "           deleted:",
    "    packages added:",
    "           deleted:",
    " method sigs added:",
    "           deleted:",
    "  terse sigs added:",
    "           deleted:",
    " sig details added:",
    "           deleted:",
    NULL
};

void CVMtypeidPrintStats(){
    int i;
    const char * name;
    int * val = (int*)&idstat;
    CVMconsolePrintf("Name and Type ID info changed:\n");
    for ( i=0; (name = idstatName[i]) != NULL; i++ ){
	CVMconsolePrintf("    %s %d\n", name, val[i]);
    }
    memset( &idstat, 0, sizeof(idstat));
}

#endif

static void deletePackage( struct pkg * );
static int getSignatureInfo( struct methodTypeTableEntry *mp, CVMUint32 **formp, CVMTypeIDTypePart** detailp );

/*
 * We allocate segments bigger and bigger as we go.
 * Initial RAM allocation is modest.
 */
#define INITIAL_SEGMENT_SIZE	1000

#define ASSERT_LOCKED CVMassert(CVMreentrantMutexIAmOwner( (CVMgetEE()),  CVMsysMutexGetReentrantMutex(&CVMglobals.typeidLock)))

/*
 * If we run out of memory or of table space, we want to throw an exception.
 * However, this can only be done with the typeidLock unlocked. Bracket the
 * throwing by conditional unlock/lock calls. They will usually be necessary.
 * (Also, since these errors seldom occur, we won't bother passing ee around just
 * for this case, but instead fetch it: locally more expensive, globally cheaper.)
 * OBVIOUSLY, make sure that the table is in a consistent state before unlocking!
 */
static void
unlockThrowInternalError( const char * msg ){
    CVMExecEnv * ee   = CVMgetEE();
    CVMBool lockOwner = CVMsysMutexIAmOwner(ee, &CVMglobals.typeidLock );
    if ( lockOwner ){
	CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
    }
    CVMthrowInternalError( ee, msg );
    if ( lockOwner ){
	CVMsysMutexLock(ee, &CVMglobals.typeidLock );
    }
}

static void
unlockThrowOutOfMemoryError(){
    CVMExecEnv * ee   = CVMgetEE();
    CVMBool lockOwner = CVMsysMutexIAmOwner(ee, &CVMglobals.typeidLock );
    if ( lockOwner ){
	CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
    }
    CVMthrowOutOfMemoryError( ee, NULL );
    if ( lockOwner ){
	CVMsysMutexLock(ee, &CVMglobals.typeidLock );
    }
}

static void
unlockThrowNoClassDefFoundError(const char * name) {
    CVMExecEnv * ee = CVMgetEE();
    CVMBool lockOwner = CVMsysMutexIAmOwner(ee, &CVMglobals.typeidLock );
    if ( lockOwner ) {
        CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
    }
    CVMthrowNoClassDefFoundError( ee, name);
    if ( lockOwner) {
        CVMsysMutexLock(ee, &CVMglobals.typeidLock );
    }
}

/*
 * Little routines used in all the ToCString routines,
 * in an attempt not to overrun externally-allocated buffers.
 * (This was a macro, but that seems the wrong space/time tradeoff.)
 */
static void conditionalPutchar( char **chrp, int * lengthRemaining, char chr, CVMBool *success  )
{
    if ( *lengthRemaining > 1 ){ 
	*(*chrp)++ = chr; 
	*lengthRemaining -= 1; 
    } else { 
	**chrp = '\0'; 
	*success = CVM_FALSE; 
	*lengthRemaining = 0; 
    }
}

static void conditionalPutstring(
    char ** chrp, int * lengthRemaining, const char *string, int stringLength, CVMBool * success )
{
    int localLengthRemaining = *lengthRemaining;
    if (localLengthRemaining <= 1) {
	**chrp = '\0'; 
	*success = CVM_FALSE; 
	*lengthRemaining = 0; 
	return;
    }
    if ( stringLength < localLengthRemaining ){
	strncpy( *chrp, string, stringLength+1 );
	*chrp += stringLength;
	*lengthRemaining -= stringLength;
    } else {
	strncpy( *chrp, string, localLengthRemaining-1 );
	*chrp += localLengthRemaining-1;
	**chrp = '\0';
	*success = CVM_FALSE;
	*lengthRemaining = 0;
    }
}

#define BASETYPE_CASES \
	case CVM_TYPEID_VOID:  conditionalPutchar( &chp, &bufLength, 'V', &success ); break; \
	case CVM_TYPEID_INT:   conditionalPutchar( &chp, &bufLength, 'I', &success ); break; \
	case CVM_TYPEID_SHORT: conditionalPutchar( &chp, &bufLength, 'S', &success ); break; \
	case CVM_TYPEID_CHAR:  conditionalPutchar( &chp, &bufLength, 'C', &success ); break; \
	case CVM_TYPEID_LONG:  conditionalPutchar( &chp, &bufLength, 'J', &success ); break; \
	case CVM_TYPEID_BYTE:  conditionalPutchar( &chp, &bufLength, 'B', &success ); break; \
	case CVM_TYPEID_FLOAT: conditionalPutchar( &chp, &bufLength, 'F', &success ); break; \
	case CVM_TYPEID_DOUBLE:conditionalPutchar( &chp, &bufLength, 'D', &success ); break; \
	case CVM_TYPEID_BOOLEAN: conditionalPutchar( &chp, &bufLength, 'Z', &success ); break;


/*
 * Initialize the type Id system
 * Register some well-known typeID's 
 */
CVMBool
CVMtypeidInit( CVMExecEnv *ee )
{
    CVMglobals.typeIDscalarSegmentSize = INITIAL_SEGMENT_SIZE;
    CVMglobals.typeIDmethodTypeSegmentSize = INITIAL_SEGMENT_SIZE;
    CVMglobals.typeIDmemberNameSegmentSize = INITIAL_SEGMENT_SIZE;

    CVMglobals.initTid = 
	CVMtypeidLookupMethodIDFromNameAndSig(ee, "<init>", "()V");
    CVMglobals.clinitTid = 
	CVMtypeidLookupMethodIDFromNameAndSig(ee, "<clinit>", "()V");
    CVMglobals.finalizeTid = 
	CVMtypeidLookupMethodIDFromNameAndSig(ee, "finalize", "()V");
    CVMglobals.cloneTid = 
	CVMtypeidLookupMethodIDFromNameAndSig(ee, "clone", "()V");

#ifdef CVM_DEBUG_STACKTRACES
    CVMglobals.printlnTid =
	CVMtypeidLookupMethodIDFromNameAndSig(ee, "println", 
					   "(Ljava/lang/String;)V");
    CVMglobals.getCauseTid =
	CVMtypeidLookupMethodIDFromNameAndSig(ee, "getCause", 
                                              "()Ljava/lang/Throwable;");
#endif
#ifdef CVM_DUAL_STACK
    {
        const char *midpImplLoaderName = 
            "sun/misc/MIDPImplementationClassLoader";
        const char *midletLoaderName = 
            "sun/misc/MIDletClassLoader";
        CVMglobals.midpImplClassLoaderTid = CVMtypeidNewClassID(
            ee, midpImplLoaderName, strlen(midpImplLoaderName));
        CVMglobals.midletClassLoaderTid = CVMtypeidNewClassID(
            ee, midletLoaderName, strlen(midletLoaderName));
        if (CVMglobals.midpImplClassLoaderTid == CVM_TYPEID_ERROR ||
            CVMglobals.midletClassLoaderTid == CVM_TYPEID_ERROR) {
            return CVM_FALSE;
        }
    }
#endif
    return CVM_TRUE;
}

/*
 * A second stage of type ID initialization.
 * Register all pre-loaded packages using
 * CVMpackagesAddPackageEntry( pkgName, strlen(pkgName), "<preloaded>" )
 */
extern void
CVMtypeidRegisterPreloadedPackages(){
    struct pkg ** hashbucket;
    struct pkg *  pkgp;
    int i;
    size_t maxLength = 0;
    char * buffer;

    for ( i = 1; i < NPACKAGEHASH; i++ ){
	hashbucket = &CVM_pkgHashtable[ i ];
	for ( pkgp = *hashbucket; pkgp != NULL; pkgp = pkgp->next ){
	    size_t l = strlen( pkgp->pkgname );
	    if ( l > maxLength ) maxLength = l;
	}
    }
    buffer = (char *)malloc( maxLength + 2 );
    if ( buffer == NULL ) return;

    for ( i = 1; i < NPACKAGEHASH; i++ ){
	hashbucket = &CVM_pkgHashtable[ i ];
	for ( pkgp = *hashbucket; pkgp != NULL; pkgp = pkgp->next ){
	    if ( pkgp->pkgname[0] != '\0' ){
		size_t l = strlen( pkgp->pkgname );
		strncpy( buffer, pkgp->pkgname, l );
		buffer[l] = '/'; /* makes  CVMpackagesAddEntry happy */
		buffer[l+1] = '\0';
		CVMpackagesAddEntry( buffer, "<preloaded>" );
	    }
	}
    }
    free( buffer );
}

struct genericTableSegment;

static void  processSegmentedTable(
    struct genericTableSegment * thisSeg, 
    size_t			 entrySize,
    void   (*entryCallback)(void*),
    void   (*segmentCallback)(void*) );

/*
 * Callbacks for giving back memory.
 */
static void destroyScalarEntry( void * t ){
    struct scalarTableEntry * p = (struct scalarTableEntry *)t;
    if ( p->tag == CVM_TYPE_ENTRY_OBJ ){
	free( (void*)(p->value.className.classInPackage) ); /*cast away const */
	p->value.className.classInPackage = NULL;
    }
}

static void destroyMemberName( void * t ){
    struct memberName * p = (struct memberName*)t;
    free( (void*)(p->name) ); /*cast away const*/
    p->name = NULL;
}

static void destroySignature( void * t ){
    struct methodTypeTableEntry * p = (struct methodTypeTableEntry*) t;
    CVMTypeIDTypePart * detailp;
    int nDetail;
    nDetail = getSignatureInfo( p, NULL, &detailp );
    if ( nDetail > N_METHOD_INLINE_DETAILS){
	p->form.formp = NULL; /* dead for sure */
	free( detailp );
    }
}

static void destroySegment( void * ); /* see later */

/*
 * Give back memory. Leaves a mess, as we don't
 * neatly unlink things!
 */
void
CVMtypeidDestroy( ){
    /*
     * Destroy signatures first, as they depend
     * on forms for interpretation. Then do other tables.
     */
    struct sigForm *thisSig, *nextSig;
    struct pkg     *thisPkg, *nextPkg;
    int i;

    processSegmentedTable(
	(struct genericTableSegment*)&CVMMethodTypeTable,
	sizeof(struct methodTypeTableEntry),
	destroySignature, destroySegment );
    processSegmentedTable(
	(struct genericTableSegment*)&CVMFieldTypeTable,
	sizeof(struct scalarTableEntry),
	destroyScalarEntry, destroySegment );
    processSegmentedTable(
	(struct genericTableSegment*)&CVMMemberNames,
	sizeof(struct memberName),
	destroyMemberName, destroySegment );
    /*
     * fields are on a single linked list.
     * Assume that any with max ref count are in Rom.
     */
    nextSig = *CVMformTablePtr;
    while ( nextSig != NULL && (nextSig->refCount) != MAX_COUNT ){
	thisSig = nextSig;
	nextSig = thisSig->next;
	free( thisSig );
    }
    /*
     * packages are on multiple lists. There is an INROM flag for them.
     */
    for ( i = 0 ; i < NPACKAGEHASH; i++ ){
	nextPkg = CVM_pkgHashtable[i];
	while ( nextPkg != NULL ){
	    thisPkg = nextPkg;
	    nextPkg = thisPkg->next;
	    if ( thisPkg->state != TYPEID_STATE_INROM ){
		free( thisPkg );
	    }
	}
    }
}

/****************************************************************
 * A simple name hashing function.
 * It is used in a bunch of places for turning names into numbers
 * It must be EXACTLY the same as the one used in JavaCodeCompact to
 * build the initial type tables, else we'll never find the entries!
 */

static unsigned computeHash( const char * name, int l ){
    /*
     * Since suffix is more likely to be unique,
     * hash from the back end.
     */
    const unsigned char * s = (const unsigned char *)name;
    unsigned v = 0;
    int n = ( l > 7 ) ? 7 : l;
    s += l-n;
    while ( n--> 0 ){
	v = (v*19) + s[n] - '/';
    }
    return v;
}

static void  processTableSegment(
    struct genericTableSegment * thisSeg, 
    size_t			 entrySize,
    void   (*callback)(void*)
){
    int i, n;
    char *   thisEntry;
    n = thisSeg->nInSegment;
    if ( thisSeg->nextFree >= 0 )
	n = thisSeg->nextFree;
    thisEntry = &(thisSeg->genericData[0]);
    for ( i = 0; i < n; i++ ){
#ifndef NO_RECYCLE
	if ( ((struct genericTableEntry*)thisEntry)->refCount != 0 )
#endif
	    callback(thisEntry);
	thisEntry += entrySize;
    }
}

/*
 * Iterate over all dynamically-allocated objects in the table.
 * Used to give back memory!
 */
static void  processSegmentedTable(
    struct genericTableSegment * thisSeg, 
    size_t			 entrySize,
    void   (*entryCallback)(void*),
    void   (*segmentCallback)(void*)
) {
    struct genericTableSegment * nextSeg; 
    /* in all cases, the first segment is part of the preloaded
     * image and we want to skip it.
     * thus start at the second one.
     */
    nextSeg = *(thisSeg->next);
    while ( nextSeg != NULL ){
	thisSeg = nextSeg;
	nextSeg = *(thisSeg->next);
	processTableSegment( thisSeg, entrySize, entryCallback );
	segmentCallback( thisSeg );
    }
}

static void destroySegment( void * t){
    struct genericTableSegment *p = (struct genericTableSegment *)t;
    p->nFree = 0;
    p->nextFree = -1;
    p->next = 0;
    free( p );
}

/*
 * This has a NoCheck version for those rare occasions
 * that its ok to reference an entry with a zero refCount
 * (Could always distribute the assertion to all clients, but
 * that is more error-prone than this.)
 */
static void * indexSegmentedTable(
    CVMAddr		 indexVal,
    struct genericTableSegment **returnSeg,
    struct genericTableSegment * thisSeg, 
    size_t			 entrySize )
{
    struct genericTableEntry *   thisEntry;
    if (indexVal >= TYPEID_NOENTRY) {
        if (returnSeg != NULL) {
            *returnSeg = NULL; /* not a valid index, so return NULL */
        }
        return NULL;
    }
    CVMassert( indexVal >= thisSeg->firstIndex );
    indexVal -= thisSeg->firstIndex;
    while ( (thisSeg != NULL) && indexVal >= thisSeg->nInSegment ){
	indexVal -= thisSeg->nInSegment;
	thisSeg = *(thisSeg->next);
    }
    thisEntry = (struct genericTableEntry*) &(thisSeg->genericData[indexVal*entrySize]);
    CVMassert( thisEntry->refCount > 0 );
    if ( returnSeg != NULL ){
	*returnSeg = thisSeg;
    }
    return thisEntry;
}

#ifdef CVM_DEBUG_ASSERTS

static void * indexSegmentedTableNoCheck(
    CVMAddr		 indexVal,
    struct genericTableSegment **returnSeg,
    struct genericTableSegment * thisSeg, 
    size_t			 entrySize )
{
    struct genericTableEntry *   thisEntry;
    if (indexVal >= TYPEID_NOENTRY) {
        if (returnSeg != NULL) {
            *returnSeg = NULL; /* not a valid index, so return NULL */
        }
        return NULL;
    }
    CVMassert( indexVal >= thisSeg->firstIndex );
    indexVal -= thisSeg->firstIndex;
    while ( (thisSeg != NULL) && indexVal >= thisSeg->nInSegment ){
	indexVal -= thisSeg->nInSegment;
	thisSeg = *(thisSeg->next);
    }
    thisEntry = (struct genericTableEntry*) &(thisSeg->genericData[indexVal*entrySize]);
    /* CVMassert( thisEntry->refCount > 0 ); <== THIS IS THE DIFFERENCE */
    if ( returnSeg != NULL ){
	*returnSeg = thisSeg;
    }
    return thisEntry;
}
#else

#define indexSegmentedTableNoCheck indexSegmentedTable

#endif

/* The specializations of the above */

#define indexMethodEntry( index, segp ) \
    ((struct methodTypeTableEntry *)indexSegmentedTable( index, \
	segp, \
	(struct genericTableSegment*)&CVMMethodTypeTable, \
	sizeof (struct methodTypeTableEntry) ))

#define indexMethodEntryNoCheck( index, segp ) \
    ((struct methodTypeTableEntry *)indexSegmentedTableNoCheck( index, \
	segp, \
	(struct genericTableSegment*)&CVMMethodTypeTable, \
	sizeof (struct methodTypeTableEntry) ))

#define indexScalarEntry( index, segp ) \
    ((struct scalarTableEntry *)indexSegmentedTable( index, \
        segp, \
	(struct genericTableSegment*)&CVMFieldTypeTable, \
	sizeof (struct scalarTableEntry) ))

#define indexScalarEntryNoCheck( index, segp ) \
    ((struct scalarTableEntry *)indexSegmentedTableNoCheck( index, \
	segp, \
	(struct genericTableSegment*)&CVMFieldTypeTable, \
	sizeof (struct scalarTableEntry) ))

#define indexMemberName( index, segp ) \
    ((struct memberName *)indexSegmentedTable( index, \
	segp, \
	(struct genericTableSegment*)&CVMMemberNames, \
	sizeof (struct memberName) ))

#define indexMemberNameNoCheck( index, segp ) \
    ((struct memberName *)indexSegmentedTableNoCheck( index, \
	segp, \
	(struct genericTableSegment*)&CVMMemberNames, \
	sizeof (struct memberName) ))

/*
 * Remove an entry from an index'ed hash chain.
 * Also note it as free in its containing segment.
 */
static void
unlinkEntry(
    CVMTypeIDPart		oldCookie,
    CVMTypeIDPart		replacementCookie,
    CVMTypeIDPart *		linkp,
    struct genericTableSegment *thisTable,
    struct genericTableSegment *thisSeg,
    size_t			entrySize )
{
    CVMTypeIDPart   indexCookie;
    struct genericTableEntry * thisEntry;

    while ( (indexCookie = *linkp) != oldCookie){
	CVMassert( indexCookie != TYPEID_NOENTRY );
	thisEntry = (struct genericTableEntry*)
		indexSegmentedTable( indexCookie, NULL, thisTable, entrySize );
	linkp = & (thisEntry->nextIndex);
    }
    *linkp = replacementCookie;
    if ( thisSeg == NULL ){
	/* containing segment not actually known. Do one more index to get it */
	/* and since this entry is known to have a zero ref-count... */
	(void) indexSegmentedTableNoCheck( oldCookie, &thisSeg, thisTable, entrySize );
    }
    thisSeg->nFree += 1;
}

static void *
findFreeTableEntry(
    struct genericTableSegment *initialSeg,
    size_t entrySize,
    CVMUint32 *tableAllocationCount,
    CVMFieldTypeID * newIndex
){
    struct genericTableSegment*  nextSeg = initialSeg;
    struct genericTableSegment*  thisSeg;
    /*struct genericTableSegment** nextp;*/
    struct genericTableEntry *   thisEntry;
    struct genericTableEntry *	 endEntry;
    size_t			 allocationSize;
    size_t			 newIndexVal;

    ASSERT_LOCKED;
    do{
	thisSeg = nextSeg;
#ifdef NO_RECYCLE
	if ( thisSeg->nextFree >= 0 ) goto findFreeInSegment;
#else
	if ( thisSeg->nFree > 0 ) goto findFreeInSegment;
#endif
    }while( (nextSeg = *(thisSeg->next)) != NULL );
    /* no free entries. allocate new. */
    allocationSize = sizeof(struct genericTableHeader)
		+ *tableAllocationCount * entrySize
		+ sizeof(struct genericTableSegment*);
    nextSeg = (struct genericTableSegment*)malloc(allocationSize);
    if ( nextSeg == NULL ){
	/* malloc failure. Return appropriately null values. */
	/* Must return TYPEID_NOENTRY here -- not CVM_TYPEID_ERROR */
	*newIndex = TYPEID_NOENTRY;
	unlockThrowOutOfMemoryError();
	return NULL;
    }
    *(thisSeg->next) = nextSeg;
    nextSeg->firstIndex = thisSeg->firstIndex + thisSeg->nInSegment;
    nextSeg->nInSegment = nextSeg->nFree = *tableAllocationCount;
    nextSeg->nextFree = 0;
    thisSeg = nextSeg;
    /* new next-pointer cell follows the segment immediately */
    thisSeg->next = (struct genericTableSegment**) 
	(((char*)thisSeg)+allocationSize- sizeof(struct genericTableSegment*));
    *(thisSeg->next) = NULL;

    *tableAllocationCount = (*tableAllocationCount*3)/2;

findFreeInSegment:
    if ( thisSeg->nextFree >= 0 ){
	/* new allocation. just bump and go. */
	thisEntry = (struct genericTableEntry*)&(thisSeg->genericData[(thisSeg->nextFree++)*entrySize]);
	if ( thisSeg->nextFree >= thisSeg->nInSegment)
	    thisSeg->nextFree = -1; /* no more easy ones in this segment */
    }else{
	/* look for a free table entry the hard way */
	endEntry = (struct genericTableEntry*)&(thisSeg->genericData[(thisSeg->nInSegment)*entrySize]);
	for ( thisEntry = (struct genericTableEntry*)&(thisSeg->genericData[0]);
	      thisEntry<endEntry;
	      thisEntry = (struct genericTableEntry*)((char*)thisEntry+entrySize ) )
	{
	    if ( thisEntry->refCount == 0 )
		break;
	}
	CVMassert(thisEntry<endEntry);
    }
    /* thisEntry now points to a free one. */
    thisEntry->refCount = 1;
    thisSeg->nFree -= 1;
    newIndexVal = thisSeg->firstIndex
	    + ( (char*)thisEntry - (char*)&(thisSeg->genericData[0]) ) / entrySize;
    *newIndex = (CVMFieldTypeID)newIndexVal;
    return thisEntry;
}

#ifdef CVM_DEBUG

struct tableReferenceMap {
    int 	minIndex;
    int 	maxTableSize;
    CVMUint32*	bitMap;
    CVMBool	failure;
    const char* tableName;
    struct pkg* currentPackage; /* only sometimes interesting */
    int		nForms;
    int *	formRefCount;
};

typedef CVMBool (*checkSegmentedTableCallback)(
	CVMUint32 hashval,
	CVMUint32 indexval,
	void * thisEntry,
	void * thisSegment,
	struct tableReferenceMap * tmap );

static void
tableError(const char * tableName,
	int hashno,
	int next,
	struct genericTableEntry * thisEntry,
	CVMBool initialHashEntry,
	const char * errorType )
{
    CVMconsolePrintf("Error in typeid table %s:\n", tableName );
    CVMconsolePrintf("	%s ", errorType );
    if ( initialHashEntry ){
	CVMconsolePrintf(" in hashtable at entry %d\n", hashno );
    } else {
	CVMconsolePrintf(" in entry %d on hash chain %d\n", next, hashno );
	if ( thisEntry!= NULL){
	    CVMconsolePrintf("  0x%x -> next: %d, refCount %d\n", thisEntry, thisEntry->nextIndex, thisEntry->refCount );
	}
    }
}

/* determine size of table and allocate bit map*/

static void initializeReferenceMap(
    struct genericTableSegment * table,
    const char * name,
    struct tableReferenceMap * mp )
{
    struct genericTableSegment* thisSegment;
    mp->maxTableSize = 0;
    mp->minIndex = table->firstIndex;
    mp->tableName = name;
    /* CVMconsolePrintf("%s table segments:\n", name); */
    for ( thisSegment=table;
	*(thisSegment->next)!=NULL;
	thisSegment=*(thisSegment->next) ){
	    /*CVMconsolePrintf("    first %d, n %d, nextFree %d\n",
		thisSegment->firstIndex,
		thisSegment->nInSegment,
		thisSegment->nextFree
	    );*/
    }
    /*CVMconsolePrintf("    first %d, n %d, nextFree %d\n",
	thisSegment->firstIndex,
	thisSegment->nInSegment,
	thisSegment->nextFree
    );*/
    mp->maxTableSize = thisSegment->firstIndex + 
	((thisSegment->nextFree>= 0)
	    ? (thisSegment->nextFree)
	    : (thisSegment->nInSegment-1));
    mp->bitMap = (CVMUint32*)calloc( sizeof(CVMUint32), (mp->maxTableSize+31)>>5 );
    mp->failure = CVM_FALSE;
    /*CVMconsolePrintf("%s table [%d ..%d]\n", name, mp->minIndex, mp->maxTableSize-1);*/
}

#undef bitVal
#undef setBit
#define bitVal(n) (mp->bitMap[(n)>>5]&(1<<((n)&31)) )
#define setBit(n) (mp->bitMap[(n)>>5] |= (1<<((n)&31)) )

static void checkReferenceMap(
    struct genericTableSegment * table, 
    size_t			 entrySize,
    checkSegmentedTableCallback  checkUnreferencedData,
    struct tableReferenceMap * mp )
{
    unsigned int i;
    struct genericTableEntry *	tableEntry;
    if ( ! mp->failure ) {
	/* now make sure that all the unreferenced entries
	 * have a zero reference count.
	 * If there's already been a failure, the bit map is
	 * incomplete and will yield false errors.
	 */
	for ( i = mp->minIndex; i < mp->maxTableSize; i++ ){
	    if ( !bitVal( i ) ){
		struct genericTableSegment * thisSegment; 
		tableEntry = (struct genericTableEntry*)
		    indexSegmentedTableNoCheck( i, &thisSegment, table, entrySize );
		if ( tableEntry->refCount != 0 ){
		    tableError( mp->tableName, 0, i, tableEntry,
			CVM_FALSE, "non-zero refCount in unreferenced entry" );
		    mp->failure = CVM_TRUE;
		}
		if ( checkUnreferencedData != NULL ){
		    checkUnreferencedData( -1, i, tableEntry, thisSegment, mp );
		}
	    }
	}
    }
    free( mp->bitMap );
}

static CVMBool checkSegmentedTable(
    CVMTypeIDPart		 hashtabl[],
    size_t			 nHash,
    struct genericTableSegment * table, 
    size_t			 entrySize,
    checkSegmentedTableCallback  checkData,
    struct tableReferenceMap * mp )
{
    int 			hashno;
    CVMTypeIDPart		next;
    struct genericTableEntry *	thisEntry;
    struct genericTableEntry *	nextEntry;
    struct genericTableSegment* thisSegment;
    struct genericTableSegment* nextSegment;
    int				minIndex;
    int				maxTableSize;
    CVMBool			failure = CVM_FALSE;

    minIndex     = mp->minIndex;
    maxTableSize = mp->maxTableSize;
	
    /* follow all hash links */
    for ( hashno = 0; hashno < nHash; hashno++ ){
	next = hashtabl[hashno];
	thisEntry   = NULL;
	thisSegment = NULL;
	while ( next != TYPEID_NOENTRY ){
	    if ( (next < minIndex) || (next >= maxTableSize) ){
		tableError( mp->tableName, hashno, next, thisEntry,
		    thisSegment==NULL, "bad hash chain entry" );
		mp->failure = failure = CVM_TRUE;
		goto nextHashEntry;
	    }
	    nextEntry = (struct genericTableEntry*)
		indexSegmentedTableNoCheck( next, &nextSegment, table, entrySize );
	    if ( nextEntry == NULL ){
		tableError( mp->tableName, hashno, next, thisEntry,
		    thisSegment==NULL, "bad hash chain entry" );
		mp->failure = failure = CVM_TRUE;
		goto nextHashEntry;
	    }
	    thisEntry = nextEntry;
	    thisSegment = nextSegment;
	    if ( thisEntry->refCount == 0 ){
		tableError( mp->tableName, hashno, next, thisEntry,
		    thisSegment==NULL, "zero reference count" );
		mp->failure = failure = CVM_TRUE;
		goto nextHashEntry;
	    }
	    if ( bitVal( next ) ){
		tableError( mp->tableName, hashno, next, thisEntry,
		    thisSegment==NULL, "entry multiply listed" );
		mp->failure = failure = CVM_TRUE;
		goto nextHashEntry;
	    }
	    setBit( next );
	    if ( checkData != NULL ){
		checkData( hashno, next, thisEntry, thisSegment, mp );
	    }
	    next = thisEntry->nextIndex;
	}
	nextHashEntry:;
    }
    
    return failure;
}

/*
 * table-specific helper functions
 * just a little data-integrity checking.
 */
static CVMBool
liveName(
    CVMUint32 hashval,
    CVMUint32 indexval,
    void * thisEntry,
    void * thisSegment,
    struct tableReferenceMap * tmap )
{
    struct memberName * m = (struct memberName*)thisEntry;
    if ( m->name == NULL ){
	tableError("member name",
	hashval, indexval,
	(struct genericTableEntry *)thisEntry,
	CVM_FALSE, "null name pointer");
	return CVM_TRUE;
    }
    return CVM_FALSE;
}

static CVMBool
deadName(
    CVMUint32 hashval,
    CVMUint32 indexval,
    void * thisEntry,
    void * thisSegment,
    struct tableReferenceMap * tmap )
{
#ifndef NO_RECYCLE
    struct memberName * m = (struct memberName*)thisEntry;
    if ( m->name != NULL ){
	tableError("member name",
	    hashval, indexval,
	    (struct genericTableEntry *)thisEntry,
	    CVM_FALSE, "non-null name pointer in deleted entry");
	return CVM_TRUE;
    }
#endif
    return CVM_FALSE;
}

static CVMBool
liveScalar(
    CVMUint32 hashval,
    CVMUint32 indexval,
    void * thisEntry,
    void * thisSegment,
    struct tableReferenceMap * tmap )
{
    struct scalarTableEntry * sp = (struct scalarTableEntry*)thisEntry;
    struct scalarTableEntry * bp;

    CVMBool hadError = CVM_FALSE;
    switch (sp->tag){
    case CVM_TYPE_ENTRY_OBJ:
	if ( sp->value.className.classInPackage == NULL ){
	    tableError("field type",
		hashval, indexval,
		(struct genericTableEntry *)thisEntry,
		CVM_FALSE, "null name pointer in classname entry");
	    hadError = CVM_TRUE;
	}
	if ( sp->value.className.package != tmap->currentPackage ){
	    tableError("field type",
		hashval, indexval,
		(struct genericTableEntry *)thisEntry,
		CVM_FALSE, "classname entry doesn't point back to its containing package");
	    hadError = CVM_TRUE;
	}
	break;
    case CVM_TYPE_ENTRY_ARRAY:
	if ( (sp->value.bigArray.depth <= CVMtypeidMaxSmallArray )
	    || (sp->value.bigArray.depth > CVM_MAX_ARRAY_DIMENSIONS ) ){
	    tableError("field type",
		hashval, indexval,
		(struct genericTableEntry *)thisEntry,
		CVM_FALSE, "out-of-range depth in array entry");
	    hadError = CVM_TRUE;
	}
	bp = indexScalarEntryNoCheck(sp->value.bigArray.basetype, NULL);
	if ( bp == NULL ){
	    tableError("field type",
		hashval, indexval,
		(struct genericTableEntry *)thisEntry,
		CVM_FALSE, "bad base-type index in array entry");
	    hadError = CVM_TRUE;
	} else if ( bp->refCount == 0 ){
	    tableError("field type",
		hashval, indexval,
		(struct genericTableEntry *)thisEntry,
		CVM_FALSE, "array base type has 0 refcount");
	    hadError = CVM_TRUE;
	} else if ( bp->tag != CVM_TYPE_ENTRY_OBJ ){
	    tableError("field type",
		hashval, indexval,
		(struct genericTableEntry *)thisEntry,
		CVM_FALSE, "array base type is not class");
	    hadError = CVM_TRUE;
	}
	break;
    default:
	tableError("field type",
	    hashval, indexval,
	    (struct genericTableEntry *)thisEntry,
	    CVM_FALSE, "bad type tag in live entry");
	hadError = CVM_TRUE;
    }
    return hadError;
}

static CVMBool
deadScalar(
    CVMUint32 hashval,
    CVMUint32 indexval,
    void * thisEntry,
    void * thisSegment,
    struct tableReferenceMap * tmap )
{
#ifndef NO_RECYCLE
    struct scalarTableEntry * sp = (struct scalarTableEntry*)thisEntry;
    if ( sp->tag != CVM_TYPE_ENTRY_FREE ){
	tableError("field type",
	    hashval, indexval,
	    (struct genericTableEntry *)thisEntry,
	    CVM_FALSE, "bad type tag in dead entry");
	return CVM_TRUE;
    }
#endif
    return CVM_FALSE;
}

static CVMBool
liveMethod(
    CVMUint32 hashval,
    CVMUint32 indexval,
    void * thisEntry,
    void * thisSegment,
    struct tableReferenceMap * tmap )
{
    struct methodTypeTableEntry * m = (struct methodTypeTableEntry*)thisEntry;
    if ( (m->nParameters+2) > FORM_SYLLABLESPERDATUM){
	int formno = 0;
	struct sigForm *f, *thisForm;
	thisForm = m->form.formp;
	for ( f = *CVMformTablePtr; f!= NULL; f = f->next ){
	    if ( f == thisForm )
		break;
	    else
		formno+=1;
	}
	if ( f == NULL ){
	    tableError("method type",
		hashval, indexval,
		(struct genericTableEntry *)thisEntry,
		CVM_FALSE, "bad form pointer");
	    return CVM_TRUE;
	} else {
	    tmap->formRefCount[formno] += 1;
	}
    }
    return CVM_FALSE;
}

void CVMtypeidCheckTables(){
    struct pkg * pp;
    CVMUint32 i;
    struct tableReferenceMap tmap;
    struct sigForm * f;

    tmap.currentPackage = NULL;
    tmap.nForms = 0;
    tmap.formRefCount = NULL;

    /*
     * member name table
     */
    initializeReferenceMap(
	(struct genericTableSegment*)&CVMMemberNames,
	"member name", &tmap );

    checkSegmentedTable(
	CVMMemberNameHash, NMEMBERNAMEHASH,
	(struct genericTableSegment*)&CVMMemberNames,
	sizeof(struct memberName ),  liveName, &tmap );

    checkReferenceMap(
	(struct genericTableSegment*)&CVMMemberNames,
	sizeof(struct memberName ),  deadName, &tmap );

    /*
     * method type table table
     * part of the setup is to setup for validating
     * the form reference counting.
     */
    for ( f = *CVMformTablePtr; f!= NULL; f=f->next )
	tmap.nForms += 1;
    tmap.formRefCount = (int*)calloc( tmap.nForms, sizeof(int) );
    initializeReferenceMap(
	(struct genericTableSegment*)&CVMMethodTypeTable,
	"method type", &tmap );

    checkSegmentedTable(
	CVMMethodTypeHash, NMETHODTYPEHASH,
	(struct genericTableSegment*)&CVMMethodTypeTable,
	sizeof(struct methodTypeTableEntry ),
	liveMethod, &tmap );

    checkReferenceMap(
	(struct genericTableSegment*)&CVMMethodTypeTable,
	sizeof(struct methodTypeTableEntry),
	NULL, &tmap );
    /* make sure the form reference counts are right */
    for ( f = *CVMformTablePtr, i=0; f!= NULL; f=f->next, i++ ){
	if ( f->refCount == MAX_COUNT )
	    continue;
	if ( f->refCount != tmap.formRefCount[i] ){
	    CVMconsolePrintf("Error in typeid form table:\n" );
	    CVMconsolePrintf("	entry at 0x%x claims %d references, but %d found\n",
		f, f->refCount, tmap.formRefCount[i] );
#ifndef NO_RECYCLE
	} else if ( f->refCount == 0 ){
	    CVMconsolePrintf("Error in typeid form table:\n" );
	    CVMconsolePrintf("	unreferenced entry at 0x%x\n", f );
#endif
	}
    }
    free( tmap.formRefCount );
    tmap.formRefCount = NULL;

    /*
     * field table
     * which is accessed by way of the pkg table.
     */
    initializeReferenceMap(
	(struct genericTableSegment*)&CVMFieldTypeTable,
	"field type",
        &tmap );

    /* do null package, then do other packages */
    tmap.currentPackage = CVMnullPackage;
    checkSegmentedTable( CVMnullPackage->typeData,
	NCLASSHASH,
	(struct genericTableSegment*)&CVMFieldTypeTable,
	sizeof(struct scalarTableEntry),
	liveScalar, &tmap );
    for ( i = 0 ; i < NPACKAGEHASH; i++ ){
	for ( pp = CVM_pkgHashtable[i] ; pp != NULL; pp = pp-> next ){
	    tmap.currentPackage = pp;
	    checkSegmentedTable( pp->typeData,
		NCLASSHASH,
		(struct genericTableSegment*)&CVMFieldTypeTable,
		sizeof(struct scalarTableEntry),
		liveScalar, &tmap );
	}
    }

    checkReferenceMap(
	(struct genericTableSegment*)&CVMFieldTypeTable,
	sizeof(struct scalarTableEntry),
	deadScalar, &tmap );
}

typedef void (*diffSegmentedTableCallback)(
	CVMUint32 indexval,
	void *  thisEntry,
	CVMBool verbosity );


static void findDiffsInSegmentedTable(
    struct genericTableSegment * table, 
    size_t			 entrySize,
    diffSegmentedTableCallback   diffFn,
    CVMBool			 verbosity)
{
    int entryNo;
    struct genericTableEntry *thisEntry;
    for ( ; table != NULL ; table = *(table->next) ){
	int n = (table->nextFree>=0)?table->nextFree:table->nInSegment;
	entryNo = table->firstIndex;
	thisEntry = (struct genericTableEntry*)&(table->genericData[0]);
	for ( ; n > 0; n-- ){
	    if ((thisEntry->state&~TYPEID_STATE_INROM) != 0 ){
		/* report and reset */
		diffFn( entryNo, thisEntry, verbosity );
		thisEntry->state = 0;
	    }
	    entryNo += 1;
	    thisEntry = (struct genericTableEntry *)((char*)thisEntry + entrySize);
	}
    }
}

static CVMBool
diffPrefix( int indexval, int state, CVMBool verbosity)
{
    switch ( state ){
    case TYPEID_STATE_ADDED:
	CVMconsolePrintf("	index %d: added ", indexval );
	break;
    case TYPEID_STATE_DELETED:
	CVMconsolePrintf("	index %d: deleted ", indexval );
	break;
    case TYPEID_STATE_ADDED|TYPEID_STATE_DELETED:
	if ( verbosity ){
	    CVMconsolePrintf("	index %d: added and deleted ",
		indexval );
	} else {
	    return CVM_FALSE;
	}
	break;
    }
    return CVM_TRUE;
}

static void memberDiffs(
	CVMUint32 indexval,
	void *  thisEntry,
	CVMBool verbosity )
{
    struct memberName * m = (struct memberName *)thisEntry;
    const char * mname;
    if ( ! diffPrefix( indexval, m->state, verbosity ) ) return;
    mname = m->name;
    if ( mname == NULL ) mname = "<null>";
    CVMconsolePrintf("%s(%d)\n", mname, m->refCount);
}

static void fieldDiffs(
	CVMUint32 indexval,
	void *  thisEntry,
	CVMBool verbosity )
{
    struct scalarTableEntry * s = (struct scalarTableEntry *)thisEntry;
    CVMAddr basecookie;
    struct scalarTableEntry *b;

    if ( ! diffPrefix( indexval, s->state, verbosity ) ) return;

    switch ( s->tag ){
    case CVM_TYPE_ENTRY_OBJ:
	CVMconsolePrintf("class %s/%s(%d)", s->value.className.package->pkgname, s->value.className.classInPackage, s->refCount );
	break;
    case CVM_TYPE_ENTRY_ARRAY:
	CVMconsolePrintf("array depth %d of ", s->value.bigArray.depth );
	basecookie = s->value.bigArray.basetype;
	if ( isTableEntry( basecookie ) ){
	    b = indexScalarEntryNoCheck( basecookie, NULL );
	    CVMconsolePrintf("class %s/%s(%d)", b->value.className.package->pkgname, b->value.className.classInPackage, s->refCount );
	} else {
	    /* this cannot happen as they are all in ROM */
	    CVMconsolePrintf("simple type %d", basecookie );
	}
	break;
    default:
	CVMconsolePrintf("	unknown thing of tag %d", s->tag );
	break;
    }
    CVMconsolePrintf("\n" );
}

static void methodDiffs(
	CVMUint32 indexval,
	void *  thisEntry,
	CVMBool verbosity )
{
    struct methodTypeTableEntry * s = (struct methodTypeTableEntry *)thisEntry;

    if ( ! diffPrefix( indexval, s->state, verbosity ) ) return;

    CVMconsolePrintf("a method signature of %d parameters\n",
	s->nParameters );
}

static void formDiffs(
	struct sigForm *  thisEntry,
	CVMBool verbosity )
{
    switch ( thisEntry->state ){
    case TYPEID_STATE_ADDED:
	CVMconsolePrintf("	added ");
	break;
    case TYPEID_STATE_DELETED:
	CVMconsolePrintf("	deleted ");
	break;
    case TYPEID_STATE_ADDED|TYPEID_STATE_DELETED:
	if ( verbosity ){
	    CVMconsolePrintf("	added and deleted " );
	} else {
	    return;
	}
	break;
    }

    CVMconsolePrintf("a method form of %d parameters\n", thisEntry->nParameters);

}

void
CVMtypeidPrintDiffs( CVMBool verbose ){
    const char *title;
    struct sigForm * form;
#ifdef NO_RECYCLE
    if ( verbose ){
	title = "Name and Type ID diffs: additions, deletions, both (verbose)";
    }else {
	title = "Name and Type ID diffs: additions & deletions";
    }
#else
    title = "Name and Type ID diffs: additions only";
#endif
    CVMconsolePrintf("============ %s ============\n", title);

    CVMconsolePrintf("\nMember Name Table:\n");
    findDiffsInSegmentedTable( (struct genericTableSegment*)&CVMMemberNames,
	sizeof(struct memberName),
	memberDiffs, verbose );

    CVMconsolePrintf("\nField Type Table:\n");
    findDiffsInSegmentedTable( (struct genericTableSegment*)&CVMFieldTypeTable,
	sizeof(struct scalarTableEntry),
	fieldDiffs, verbose );

    CVMconsolePrintf("\nMethod Type Table:\n");
    findDiffsInSegmentedTable( (struct genericTableSegment*)&CVMMethodTypeTable,
	sizeof(struct methodTypeTableEntry),
	methodDiffs, verbose );
    CVMconsolePrintf("Method Form Table:\n");
    for ( form = *CVMformTablePtr; form != NULL; form = form->next ){
	if ( (form->state&~TYPEID_STATE_INROM) != 0 ){
	    formDiffs( form, verbose );
	    form->state = 0;
	}
    }
    CVMconsolePrintf("=========END %s DNE=========\n", title);

}

#endif


/**************************************************************
 * Member names are kept in a table. They are indexed.
 */

static struct memberName *
findFreeMemberEntry( CVMFieldTypeID * newIndex ){
    struct memberName * retval = (struct memberName *)findFreeTableEntry(
	(struct genericTableSegment *)&CVMMemberNames,
	sizeof (struct memberName),
	&CVMglobals.typeIDmemberNameSegmentSize,
	newIndex
	);
    if ((retval!=NULL) && (*newIndex > CVM_TYPEID_MAX_MEMBERNAME)){
	/*
	 * No malloc failure, but
	 * table overflow. Give it back and return a null.
	 */
	struct genericTableSegment *thisSeg;
	(void)indexMemberNameNoCheck( *newIndex, &thisSeg );
	thisSeg->nFree += 1;
	retval->refCount = 0;
	unlockThrowInternalError( "Combined number of method and field names exceeds vm limit." );
	*newIndex = TYPEID_NOENTRY;
	return NULL;
    }
    return retval;
}

/*
 * Unlink an unused membername from the table and release
 * its resources. This is called from CVMtypeidDisposeMembername
 * when a name's reference count reaches zero, or from 
 * CVMtypeidNew*IDFromNameAndSig when there is an allocation or
 * lookup error after a membername is created but while it still
 * has zero reference count and is unneeded.
 */
static void
deleteMemberEntry(
    struct memberName * thisEntry,
    CVMTypeIDNamePart thisCookie,
    struct memberNameTableSegment * thisSeg )
{
    /*
     * First, take it off the hash chain.
     * and add to the segment's free count.
     */
    unsigned 	hashVal = computeHash( thisEntry->name, (int)strlen(thisEntry->name) ) % NMEMBERNAMEHASH;
    CVMTypeIDNamePart * pCell = &CVMMemberNameHash[ hashVal ];
    ASSERT_LOCKED;
    CVMtraceTypeID(("Typeid: Deleting member name %s\n", thisEntry->name));
    unlinkEntry( thisCookie, thisEntry->nextIndex,
	    pCell, 
	    (struct genericTableSegment*)&CVMMemberNames, 
	    (struct genericTableSegment*)thisSeg, 
	    sizeof(struct memberName) );
#ifndef NO_RECYCLE
    /*
     * now delete the malloc'd memory.
     * Since the refCount is already zero, we're done.
     */
    free( (char*)(thisEntry->name) ); /* cast away const */
    thisEntry->name = NULL;
#endif
#ifdef CVM_DEBUG
    thisEntry->state |= TYPEID_STATE_DELETED;
    idstat.nNamesDeleted++;
#endif
}

static struct memberName *
referenceMemberName(
    CVMExecEnv * ee,
    const char * name, 
    CVMTypeIDNamePart *nameCookie, 
    CVMBool doInsertion
){
    unsigned 		hashVal = computeHash( name, (int)strlen(name) ) % NMEMBERNAMEHASH;
    CVMTypeIDNamePart *	hashbucket = &CVMMemberNameHash[ hashVal ];
    CVMTypeIDNamePart	thisIndex;
    struct memberName * thisName = NULL;

    for ( thisIndex = *hashbucket; thisIndex != TYPEID_NOENTRY; thisIndex = thisName->nextIndex ){
	const char * namestring;
	thisName = indexMemberName( thisIndex , NULL);
	namestring = thisName->name;
	/* Assert that we don't have a race with a client deleting the
	 * entry. This would be an indication of a client doing a lookup
	 * when it should be doing a new.
	 */
	CVMassert((thisName->refCount != 0) && (namestring != NULL));
	if (strcmp( namestring, name ) == 0 ){
	    /* found one */
	    if ( doInsertion ){
		conditionalIncRef(thisName);
	    }
	    break;
	}
    }
    if ( thisIndex==TYPEID_NOENTRY ){
	/* didn't find */
	/* if doInsertion, then add it in, else fail */
	thisName = NULL;
	if ( doInsertion ){
	    CVMFieldTypeID freeEntry;
	    char * newname;
	    size_t namelength = strlen(name);
	    ASSERT_LOCKED;
	    thisName = findFreeMemberEntry( &freeEntry );
	    newname = (char *)malloc( namelength+1 );
	    if ( (thisName==NULL) || (newname==NULL) ){
		if ( thisName != NULL ){
		    /*
		     * Just tell the segment it has one more free entry.
		     */
		    struct genericTableSegment *thisSeg;
		    (void)indexMemberNameNoCheck( freeEntry, &thisSeg );
		    thisSeg->nFree += 1;
		    thisName = NULL;
		    unlockThrowOutOfMemoryError(); /* malloc failed */
		} else {
		    if ( newname != NULL ){
			free( newname );
		    }
		    /* InternalError already thrown by findFreeMemberEntry() */
		}
		thisIndex = freeEntry = TYPEID_NOENTRY;
	    } else {
		thisIndex = freeEntry;
		strcpy( newname, name );
		newname[namelength] = '\0';
		thisName->name = newname;
		thisName->nextIndex = *hashbucket;
		thisName->refCount = 1;
		*hashbucket       = freeEntry;
		CVMtraceTypeID(("Typeid: New member name %s\n", newname));
#ifdef CVM_DEBUG
		thisName->state = TYPEID_STATE_ADDED;
		idstat.nNamesAdded++;
#endif
	    }
	}
    }
    if ( nameCookie != NULL )
	*nameCookie = thisIndex;
    return thisName;
}

CVMTypeID
CVMtypeidLookupMembername( CVMExecEnv * ee, const char * name ){
    CVMTypeIDNamePart thisIndex = TYPEID_NOENTRY;
    struct memberName * thisName;
    thisName = referenceMemberName( ee, name, &thisIndex, CVM_FALSE );
    if ( thisName == NULL )
	return CVM_TYPEID_ERROR;
    return CVMtypeidCreateTypeIDFromParts(thisIndex, 0);
}

CVMTypeID
CVMtypeidNewMembername( CVMExecEnv * ee, const char * name ){
    CVMTypeID thisIndex;
    CVMTypeIDNamePart thisCookie;
    struct memberName * thisName;
    CVMsysMutexLock(ee, &CVMglobals.typeidLock );
    thisName = referenceMemberName( ee, name, &thisCookie, CVM_TRUE );
    if ( thisName == NULL ){
	thisIndex = CVM_TYPEID_ERROR;
    } else {
	thisIndex = CVMtypeidCreateTypeIDFromParts(thisCookie, 0);
    }
    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
    return thisIndex;
}

CVMTypeID
CVMtypeidCloneMembername( CVMExecEnv *ee, CVMTypeID cookie ){
    struct memberName * 	   thisName;
    struct genericTableSegment *thisSeg;

    cookie >>= CVMtypeidNameShift; /* name part only! */
    thisName = indexMemberName( cookie, &thisSeg );

    CVMsysMutexLock(ee, &CVMglobals.typeidLock );
    conditionalIncRef(thisName);
    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );

    return cookie;
}


void
CVMtypeidDisposeMembername( CVMExecEnv *ee, CVMTypeID cookie ){
    struct memberName * 	   thisName;
    struct genericTableSegment *thisSeg;
    CVMTypeIDNamePart nameCookie = CVMtypeidGetNamePart(cookie);
    thisName = indexMemberName( nameCookie, &thisSeg );
    CVMsysMutexLock(ee, &CVMglobals.typeidLock );
    if ( thisName->refCount != MAX_COUNT ){
	if ( --(thisName->refCount) == 0 ){
	    deleteMemberEntry( thisName, nameCookie,
			       (struct memberNameTableSegment*)thisSeg );
	}
    }
    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
}

/****************************************************************************
 * type structures are allocated in arrays, called segments,
 * which are linked together.
 * They are allocated, reference counted, freed, and re-used.
 * Here is the management code.
 */


static struct scalarTableEntry *
findFreeScalarEntry( CVMFieldTypeID * newIndex ){
    struct scalarTableEntry * retval =  (struct scalarTableEntry *)findFreeTableEntry(
	(struct genericTableSegment *)&CVMFieldTypeTable,
	sizeof (struct scalarTableEntry),
	&CVMglobals.typeIDscalarSegmentSize,
	newIndex );
    if ((retval!=NULL) && (*newIndex > CVM_TYPEID_MAX_BASETYPE)){
	/*
	 * No malloc failure, but
	 * table overflow. Give it back and return a null.
	 */
	struct genericTableSegment *thisSeg;
	(void)indexScalarEntryNoCheck( *newIndex, &thisSeg );
	thisSeg->nFree += 1;
	retval->refCount = 0;
	unlockThrowInternalError( "Number of class names exceeds vm limit." );
	*newIndex = TYPEID_NOENTRY;
	return NULL;
    }
    return retval;
}

/*
 * Unlink an unused type entry from the table and release
 * its resources. This is called 
 * when an entry's reference count reaches zero, or 
 * when there is an allocation or
 * lookup error after one is created but while it still
 * has zero reference count and is unneeded.
 * This is for classes only. For big arrays, use
 * deleteArrayEntry.
 */
static void
deleteClassEntry(
    struct scalarTableEntry * thisEntry,
    CVMClassTypeID thisCookie,
    struct scalarTableSegment * thisSeg )
{
    /*
     * First, take it off the hash chain.
     * and add to the segment's free count.
     */
    unsigned 	 hashVal;
    struct pkg * thisPkg;
    CVMTypeIDTypePart * pCell;

    ASSERT_LOCKED;
    CVMassert( thisEntry->tag == CVM_TYPE_ENTRY_OBJ );
    thisCookie &= CVMtypeidBasetypeMask;
    thisPkg = thisEntry->value.className.package;
    hashVal = computeHash( thisEntry->value.className.classInPackage, (int)strlen(thisEntry->value.className.classInPackage) ) % NCLASSHASH;
    CVMtraceTypeID(("Typeid: Deleting class (%d ) %s/%s\n",
		    thisCookie, thisPkg->pkgname, thisEntry->value.className.classInPackage));
    pCell = &(thisPkg->typeData[ hashVal ]);
    unlinkEntry( (CVMTypeIDPart)thisCookie, thisEntry->nextIndex,
	    pCell, 
	    (struct genericTableSegment*)&CVMFieldTypeTable,
	    (struct genericTableSegment*)thisSeg,
	    sizeof(struct scalarTableEntry) );
#ifndef NO_RECYCLE
    /*
     * now delete the malloc'd memory and 
     * note in segment header.
     * The refCount is already zero.
     */
    free( (char*)(thisEntry->value.className.classInPackage) ); /* cast away const */
    thisEntry->value.className.classInPackage = NULL;
    thisEntry->tag = CVM_TYPE_ENTRY_FREE;
#endif
#ifdef CVM_DEBUG
    thisEntry->state |= TYPEID_STATE_DELETED;
    idstat.nClassesDeleted++;
#endif
    /* reference count the package, too */
    if ( thisPkg->refCount != MAX_COUNT ){
	if ( --(thisPkg->refCount) == 0 ){
	    deletePackage( thisPkg );
	}
    }
}

static void
deleteArrayEntry(
    struct scalarTableEntry * thisEntry,
    CVMTypeIDTypePart thisCookie,
    struct scalarTableSegment * thisSeg )
{
    struct scalarTableEntry * baseEntry;
    unsigned 	 hashVal;
    struct pkg * thisPkg;

    ASSERT_LOCKED;
    CVMassert( thisEntry->tag == CVM_TYPE_ENTRY_ARRAY );
    /*
     * First, take it off the hash chain.
     * and add to the segment's free count.
     * We are in the hash chain of the package of the base type
     * or the null package.
     */
    if ( CVMtypeidFieldIsRef(thisEntry->value.bigArray.basetype ) ){
	baseEntry = indexScalarEntry(thisEntry->value.bigArray.basetype, NULL);
	thisPkg = baseEntry->value.className.package;
	hashVal = computeHash( baseEntry->value.className.classInPackage, (int)strlen(baseEntry->value.className.classInPackage) ) % NCLASSHASH;
    } else {
	baseEntry = NULL;
	thisPkg = CVMnullPackage;
	hashVal = 0;
    }
    thisCookie &= CVMtypeidBasetypeMask;
    CVMtraceTypeID(("Typeid: Deleting array (%d ) %s/%s[%d]\n",
		    thisCookie, thisPkg->pkgname,
		    (baseEntry==NULL)?"<none>":baseEntry->value.className.classInPackage,
		    thisEntry->value.bigArray.depth ));
    unlinkEntry( thisCookie, thisEntry->nextIndex,
	    &(thisPkg->typeData[ hashVal ]),
	    (struct genericTableSegment*)&CVMFieldTypeTable,
	    (struct genericTableSegment*)thisSeg,
	    sizeof(struct scalarTableEntry) );
#ifndef NO_RECYCLE
    thisEntry->tag = CVM_TYPE_ENTRY_FREE;
#endif
    /*
     * All this entry contains is a reference to its baseclass.
     * DecRef that thing, if it is indeed a class.
     */
    if ( (baseEntry!=NULL) && (baseEntry->refCount != MAX_COUNT) ){
	if ( --(baseEntry->refCount) == 0 ){
	    deleteClassEntry( baseEntry, thisEntry->value.bigArray.basetype, NULL );
	}
    }
#ifdef CVM_DEBUG
    thisEntry->state |= TYPEID_STATE_DELETED;
    idstat.nArraysDeleted++;
#endif
    /* reference count the package, too */
    if ( thisPkg->refCount != MAX_COUNT ){
	if ( --(thisPkg->refCount) == 0 ){
	    deletePackage( thisPkg );
	}
    }
}

static void
deleteScalarTypeEntry(
    struct scalarTableEntry * thisEntry,
    CVMTypeIDTypePart thisCookie,
    struct scalarTableSegment * thisSeg )
{
    if ( thisEntry->tag == CVM_TYPE_ENTRY_ARRAY ){
	deleteArrayEntry( thisEntry, thisCookie, thisSeg );
    } else {
	deleteClassEntry( thisEntry, thisCookie, thisSeg );
    }
}

static void
decrefScalarTypeEntry( CVMTypeIDTypePart typeCookie ){
    struct scalarTableEntry*	thisType;
    struct genericTableSegment*	typeSeg;

    ASSERT_LOCKED;
    typeCookie &= CVMtypeidBasetypeMask; /* note BASE TYPE ONLY */
    if ( isTableEntry( typeCookie ) ){
	thisType = indexScalarEntry( typeCookie, &typeSeg );
	if ( thisType->refCount != MAX_COUNT ){
	    if ( --(thisType->refCount) == 0 ){
		deleteScalarTypeEntry( thisType, typeCookie,
				       (struct scalarTableSegment *)typeSeg);
	    }
	}
    }
}

/*******************************************************************
 * The basis of type-structure lookup is the package.
 * This introduces another layer of data-structure management.
 * Here it is.
 */

extern struct pkg * const CVMnullPackage;

static struct pkg *
lookupPkg(const char * pkgname, int namelength, CVMBool doInsertion){
    unsigned nameHash;
    struct pkg ** hashbucket;
    struct pkg *  pkgp;
    char	  firstchar;
    if ( namelength == 0 ) return CVMnullPackage;

    nameHash = computeHash( pkgname, namelength );
    hashbucket = &CVM_pkgHashtable[ nameHash%NPACKAGEHASH ];
    firstchar = pkgname[0];
    for ( pkgp = *hashbucket; pkgp != NULL; pkgp = pkgp->next ){
	/*
	 * compare this one with that one.
	 * Integer comparison is cheap, for an opener.
	 */
	const char * namestring = pkgp->pkgname;
	/* Assert that we don't have a race with a client deleting the
	 * entry. This would be an indication of a client doing a lookup
	 * when it should be doing a new.
	 */
	CVMassert((pkgp->refCount != 0) && (namestring != NULL));
	if ( namestring[0] != firstchar)
	    continue;
	if ( strncmp( namestring, pkgname, namelength ) != 0 )
	    continue;
	if (namestring[namelength] == '\0' )
	    break; /* we have a winner */
    }
    /*
     * Either off the end of the list, or found a winner
     */
    if ( doInsertion && (pkgp == NULL) ){
	/*
	 * Not there. Already locked.
	 * Add it.
	 * First allocate all the space in one single allocation
	 */
	char * newname;
	ASSERT_LOCKED;
	pkgp = (struct pkg *)calloc( 1, sizeof( struct pkg ) + namelength+1 );
	if ( pkgp == NULL ){
	    /* malloc failure */
	    unlockThrowOutOfMemoryError();
	} else {
	    int i;

	    /* now compute where we really want things within the allocation */
	    newname = (char *)( pkgp+1 ); /* C idiom in use here */

	    pkgp->pkgname = strncpy( newname, pkgname, namelength );
	    newname[namelength] = '\0';
	    pkgp->next    = *hashbucket;
	    /*
	     * Nothing in this hash table yet
	     */
	    for (i = 0; i < NCLASSHASH; i++) {
		pkgp->typeData[i] = TYPEID_NOENTRY;
	    }
	    *hashbucket = pkgp;
	    pkgp->refCount = 0; /* no references yet */
	    CVMtraceTypeID(("Typeid: New package %s\n", newname));
#ifdef CVM_DEBUG
	    pkgp->state = TYPEID_STATE_ADDED;
	    idstat.nPkgAdded++;
#else
	    pkgp->state = 0;
#endif
	}
    }
    return pkgp;
}

static void
deletePackage( struct pkg *pkgp ){
    struct pkg ** hashbucket;
    unsigned nameHash;
#ifdef CVM_DEBUG
    int i;
#endif
    
    CVMassert( pkgp->refCount == 0 );
    CVMtraceTypeID(("Typeid: Deleting package %s\n", pkgp->pkgname));
#ifdef CVM_DEBUG
    /* ensure hash table is empty */
    for ( i = 0 ; i < NCLASSHASH; i++ ){
	CVMassert( pkgp->typeData[i] == TYPEID_NOENTRY );
    }
    idstat.nPkgDeleted++;
#endif
    nameHash = computeHash( pkgp->pkgname, (int)strlen(pkgp->pkgname) );
    hashbucket = &CVM_pkgHashtable[ nameHash%NPACKAGEHASH ];
    while ( *hashbucket != pkgp ){
	CVMassert( *hashbucket != NULL ); /* seems redundant */
	hashbucket = &((*hashbucket)->next);
    }
    *hashbucket = pkgp->next; /* unlink from hash chain. */
    /*
     * Since the package and its name are allocated by a single calloc
     * call, this will free both at once.
     */
    free( pkgp );
}


/********************************************************
 * Many of the user operations on these are quite straightforward.
 * Here are a number of them.
 * The only hard part is parsing and lookup of signatures or type names.
 * That is saved until last.
 */



int
CVMtypeidGetArrayDepthX( CVMClassTypeID t ){
    struct scalarTableEntry* ep;
    t &= CVMtypeidTypeMask;
    if ( !isTableEntry(t) ){
	/* got here in error. deal with it. */
	return t>>CVMtypeidArrayShift;
    }
    ep = indexScalarEntry( t&CVMtypeidBasetypeMask, NULL );
    if ( ep->tag == CVM_TYPE_ENTRY_ARRAY ){
	return (int)ep->value.bigArray.depth;
    }
    /* non-array has a depth of 0 */
    return 0;

}

CVMClassTypeID
CVMtypeidGetArrayBasetypeX( CVMClassTypeID t ){
    struct scalarTableEntry* ep;
    t &= CVMtypeidTypeMask;
    if ( !isTableEntry(t) ){
	/* got here in error. deal with it. */
	return t&CVMtypeidBasetypeMask;
    }
    ep = indexScalarEntry( t&CVMtypeidBasetypeMask, NULL );
    if ( ep->tag == CVM_TYPE_ENTRY_ARRAY ){
	return ep->value.bigArray.basetype;
    }
    /* non-array has a bad basetype of 0? */
    return CVM_TYPEID_ERROR;
}

/***************************************************************
 *
 * Lookup a class, having already found its package.
 * Insert if appropriate. Return a pointer to the classname
 * or null if not found and we're not inserting.
 * also, set *classCookie to the index of the entry, classCookie!=0
 */
static struct scalarTableEntry *
lookupClass(
    const char *    classname,
    int             namelength,
    struct pkg *    pkgp,
    CVMBool         doInsertion,
    CVMFieldTypeID* classCookie
){
    unsigned nameHash = computeHash( classname, namelength );
    CVMTypeIDTypePart * hashbucket = &(pkgp->typeData[ nameHash%NCLASSHASH ]);
    struct scalarTableEntry *	classp = NULL;
    CVMFieldTypeID		classIndex;
    char			firstchar = classname[0];
    char			lChar;
    const char *		thisname;
    int				l = namelength;
    int				lm1;

    if (l > 255 ){
	l = 255;
    }
    lm1 = l - 1;
    lChar = classname[lm1];

    /* DEBUG printf( "looking for class %s in package %s\n",
	classname, pkgp->pkgname ); */
    for ( classIndex = *hashbucket;
	  classIndex != TYPEID_NOENTRY;
	  classIndex = classp->nextIndex
    ){
        classp = indexScalarEntry(classIndex, NULL);
	if ( classp->tag != CVM_TYPE_ENTRY_OBJ)
	    continue;
	thisname = classp->value.className.classInPackage;
	/* Assert that we don't have a race with a client deleting the
	 * entry. This would be an indication of a client doing a lookup
	 * when it should be doing a new.
	 */
	CVMassert((classp->refCount != 0) && (thisname != NULL));
	/* DEBUG printf("... compare to %s\n",thisname); */
	if (classp->nameLength != l)
	    continue;
	if ((thisname[0] != firstchar) || (thisname[lm1] != lChar))
	    continue;
	if ( strncmp( classname, thisname, namelength) != 0 )
	    continue;
	if ( thisname[namelength] == '\0' ) {
	    /* DEBUG printf("... found it!\n"); */
	    if ( doInsertion ){
		conditionalIncRef(classp);
	    }
	    break;
	}
    }
    /*
     * Either found an entry, or didn't
     */
    if ( classIndex == TYPEID_NOENTRY ){
	classp = NULL;
	if ( doInsertion ){
	    /*
	     * it really is absent and we want to 
	     * insert it.
	     */
	    char * newname;
	    ASSERT_LOCKED;
	    /* DEBUG printf("... inserting class %s\n", classname ); */
	    classp = findFreeScalarEntry( &classIndex );
	    newname = (char *)malloc( namelength+1);
	    if ( (classp==NULL) || (newname==NULL ) ){
		/* malloc failure */
		if ( classp!= NULL){
		    /*
		     * Give back the table entry. Actually, just tell the
		     * segment it has one more free entry.
		     */
		    struct genericTableSegment * thisSeg;
		    (void)indexScalarEntryNoCheck( classIndex, &thisSeg );
		    thisSeg->nFree += 1;
		    classp = NULL;
		    unlockThrowOutOfMemoryError(); /* must have been a malloc() failure */
		} else {
		    if ( newname != NULL ){
			free( newname );
		    }
		    /* InternalError already thrown by findFreeScalarEntry */
		}
		classIndex = CVM_TYPEID_ERROR;
	    } else {
		classp->tag = CVM_TYPE_ENTRY_OBJ;
		strncpy( newname, classname, namelength );
		newname[namelength] = '\0';
		classp->value.className.classInPackage = newname;

		classp->nextIndex = *hashbucket;
		classp->value.className.package = pkgp;
		classp->refCount = 1;
		classp->nameLength = l;
		*hashbucket       = classIndex;
		conditionalIncRef( pkgp );
		CVMtraceTypeID(("Typeid: New class (%d ) %s/%s\n",
				classIndex, pkgp->pkgname, newname));
#ifdef CVM_DEBUG
		classp->state = TYPEID_STATE_ADDED;
		idstat.nClassesAdded++;
#endif
	    }
	}
    }
    /* now have an entry    */
    if ( classCookie != NULL ) *classCookie = classIndex;

    if (classIndex == TYPEID_NOENTRY) {
	return NULL;
    }

    return classp;
}


static struct scalarTableEntry *
referenceClassName(
    const char * classname,
    int namelength,
    CVMBool doInsertion,
    CVMFieldTypeID *classCookie
){
    /*
     * Parse into class part and package part.
     * Package part may be null!
     */
    const char*		pkgEnd;
    const char*		classInPkg;
    int 		pkgLength;
    int			classInPkgLength;
    struct pkg *	pkgp;
    struct scalarTableEntry *
			classp;


    /* The following is equivalent to ... 
     * pkgEnd = (char*)memrchr(classname, '/', namelength);
     */
    for (pkgEnd = classname + namelength - 1; *pkgEnd != '/'; pkgEnd --){
	if (pkgEnd == classname){
	    pkgEnd = NULL;
	    break;
	}
    }
    /*
     * if the test clause of the "for" was false, then *pkgEnd == '/'
     * otherwise pkgEnd == NULL
     */
    CVMassert((pkgEnd == NULL) || (*pkgEnd == '/'));

    if (pkgEnd == NULL ){
	/* no separation, so the class name starts right at the beginning */
	pkgLength = 0;
	classInPkg = classname;
	classInPkgLength = namelength;
    } else {
	/* found a separating / */
	pkgLength =  pkgEnd - classname;
	classInPkg = pkgEnd + 1;
	classInPkgLength = namelength - pkgLength - 1;
    }
    pkgp = lookupPkg(classname, pkgLength, doInsertion);
    if ( pkgp == NULL ) return NULL;
    classp = lookupClass(classInPkg, classInPkgLength,
			 pkgp, doInsertion, classCookie);
    if (classp == NULL){
	/*
	 * See if this was to have been the first class
	 * in its package. If so, we should free the package.
	 */
	if (pkgp->refCount == 0){
	    deletePackage(pkgp);
	}
    }
    return classp;
}

/*
 * Lookup an array, having already found its package.
 * Insert if appropriate. Return a pointer to the entry
 * or null if not found and we're not inserting.
 * also, set *classCookie to the index of the entry, classCookie!=0
 */

static struct scalarTableEntry *
lookupArray(
    CVMFieldTypeID basetype,
    struct scalarTableEntry *baseEntry,
    int depth,
    struct pkg * pkgp,
    CVMBool doInsertion,
    CVMFieldTypeID *classCookie
){
    unsigned int basenameHash =
	    (baseEntry==NULL) ? 0
	    : computeHash(baseEntry->value.className.classInPackage,
			  (int)strlen(baseEntry->value.className.classInPackage) 
	    );
    CVMTypeIDTypePart * hashbucket = &(pkgp->typeData[ basenameHash%NCLASSHASH ]);
    CVMFieldTypeID   		arrayIndex;
    struct scalarTableEntry *	arrayp = NULL;

    /* DEBUG printf( "looking for array[%d of %x in package %s\n",
	depth, basetype, pkgp->pkgname ); */

    for ( arrayIndex = *hashbucket;
	arrayIndex != TYPEID_NOENTRY;
	arrayIndex = arrayp->nextIndex
    ){
        arrayp = indexScalarEntry(arrayIndex, NULL);
	if ( arrayp->tag != CVM_TYPE_ENTRY_ARRAY)
	    continue;
	if ( arrayp->value.bigArray.basetype==basetype && arrayp->value.bigArray.depth==depth ){
	    /*DEBUG printf("... found it!\n"); */
	    if ( doInsertion ){
		conditionalIncRef( arrayp );
	    }
	    break;
	}
    }
    /*
     * Either found an entry, or didn't
     */
    if ( arrayIndex == TYPEID_NOENTRY ){
	arrayp = NULL;
	arrayIndex = CVM_TYPEID_ERROR; /* probably redundant */
	if ( doInsertion ){
	    /*
	     * Not there, will add entry.
	     */
	    ASSERT_LOCKED;
	    /*DEBUG printf("... inserting class %s\n", classname ); */
	    arrayp = findFreeScalarEntry( &arrayIndex );
	    if ( arrayp != NULL ){
		arrayp->tag = CVM_TYPE_ENTRY_ARRAY;
		arrayp->refCount = 1;
		if ( baseEntry != NULL ){
		    conditionalIncRef(baseEntry);
		}
		arrayp->value.bigArray.basetype = basetype;
		arrayp->value.bigArray.depth    = depth;
		arrayp->nextIndex    = *hashbucket;
		*hashbucket = arrayIndex;
		conditionalIncRef( pkgp );
		CVMtraceTypeID(("Typeid: New array (%d ) %s/%s[%d]\n",
				arrayIndex, pkgp->pkgname,
				(baseEntry==NULL)?"<none>":baseEntry->value.className.classInPackage,
				depth));
#ifdef CVM_DEBUG
		arrayp->state = TYPEID_STATE_ADDED;
		idstat.nArraysAdded++;
#endif
	    }
	}
    }
    if ( classCookie != NULL ) *classCookie = arrayIndex;
    return arrayp;
}


CVMBool
CVMtypeidIsSameClassPackage( CVMClassTypeID classname1, CVMClassTypeID classname2 ){
    CVMFieldTypeID	comparand[2];
    struct pkg * 	pkg[2];
    int 		i;

    comparand[0] = classname1;
    comparand[1] = classname2;

    /* find base type cookie for each */
    /* this strips off any layers of arrays */
    for ( i = 0; i < 2; i++ ){
	if ( CVMtypeidIsBigArray(comparand[i]) ){
	    struct scalarTableEntry * arrayp;
	    arrayp = indexScalarEntry( comparand[i]&CVMtypeidBasetypeMask, NULL);
	    comparand[i]  = arrayp->value.bigArray.basetype;
	} else {
	    comparand[i] &= CVMtypeidBasetypeMask;
	}
    }
    /* find base type class entries for each ( if any ),
     * and the packages, too
     */
    for ( i = 0; i < 2; i++ ){
	if ( CVMtypeidIsPrimitive(comparand[i]) ){
	    pkg[i] = CVMnullPackage;
	} else {
	    struct scalarTableEntry *baseclass;
	    baseclass = indexScalarEntry( comparand[i] , NULL);
	    pkg[i] = baseclass->value.className.package;
	}
    }

    /* now that we have a package pointer, comparison is trivial */
    return pkg[0] == pkg[1];
}


/*
 * The public entry point for parsing a field signature.
 */
static struct scalarTableEntry *
referenceFieldSignature( CVMExecEnv * ee, const char * sig, int sigLength, CVMBool doInsertion, CVMFieldTypeID *retCookie ){
    CVMFieldTypeID thisCookie = CVM_TYPEID_ERROR;
    struct scalarTableEntry * thisEntry = NULL;
    int depth=0;
    CVMFieldTypeID basetype = CVM_TYPEID_ERROR;
    struct scalarTableEntry * baseEntry = NULL;
    struct pkg *	      basePackage = CVMnullPackage;
    const char *name = sig;

    while ( *sig == CVM_SIGNATURE_ARRAY && sigLength>0){
	depth += 1;
	sig += 1;
	sigLength -= 1;
    }
    if ( sigLength<= 0 ){
	goto returnSig;
    }

    switch ( *sig ){
    case CVM_SIGNATURE_VOID:
	/* technically illegal as a type, but we'll let it pass. */
	basetype =  CVM_TYPEID_VOID;
	break;
    case CVM_SIGNATURE_INT:
	basetype = CVM_TYPEID_INT;
	break;
    case CVM_SIGNATURE_SHORT:
	basetype = CVM_TYPEID_SHORT;
	break;
    case CVM_SIGNATURE_CHAR:
	basetype = CVM_TYPEID_CHAR;
	break;
    case CVM_SIGNATURE_LONG:
	basetype = CVM_TYPEID_LONG;
	break;
    case CVM_SIGNATURE_BYTE:
	basetype = CVM_TYPEID_BYTE;
	break;
    case CVM_SIGNATURE_FLOAT:
	basetype = CVM_TYPEID_FLOAT;
	break;
    case CVM_SIGNATURE_DOUBLE:
	basetype = CVM_TYPEID_DOUBLE;
	break;
    case CVM_SIGNATURE_BOOLEAN:
	basetype = CVM_TYPEID_BOOLEAN;
	break;
    case CVM_SIGNATURE_CLASS:
	sig += 1; /* skip the L */
	sigLength -= 2; /* discount the ; */
	baseEntry = referenceClassName( sig, sigLength, doInsertion, &basetype);

	if ( baseEntry== NULL ){
	    goto returnSig; /* error! */
	}
	basePackage = baseEntry->value.className.package;
	break;
    default:
	goto returnSig; /* error! */
    }
    if ( depth <= CVMtypeidMaxSmallArray ){
	thisEntry = baseEntry;
	thisCookie = (((CVMFieldTypeID) depth)<<CVMtypeidArrayShift)+basetype;
	/* goto returnSig; */
    } else {
	/*
	 * else its a big array
	 * lookupArray will reference count the underlying baseclass.
	 * but and the entry itself!
	 * At this point, the baseclass count may be TOO BIG, since
	 * referenceClassName (called above) also incremented it.
	 * Adjust downward. I promise it will never go below 1.
	 */
	if ( (thisEntry = lookupArray( basetype, baseEntry, depth, basePackage,
		doInsertion, &thisCookie ) ) != NULL
	){
	    thisCookie = CVMtypeidBigArray|thisCookie;
	    if ( doInsertion && (baseEntry!=NULL) && (baseEntry->refCount != MAX_COUNT) ){
		CVMtraceTypeID(("Typeid: field signature base entry adjustment to class %s\n", baseEntry->value.className.classInPackage));
		baseEntry->refCount -= 1;
		CVMassert( baseEntry->refCount != 0 );
	    }
	}
    }

returnSig:
    if ( thisEntry != NULL ){
	CVMassert( thisEntry->refCount != 0 );
    }
    if ( retCookie != NULL ){
	*retCookie = thisCookie;
    }
    if (thisCookie == CVM_TYPEID_ERROR && doInsertion 
            && !CVMexceptionOccurred(ee)) {
        unlockThrowNoClassDefFoundError(name);
    }
    return thisEntry;
}

/*
 * This is effectively a New routine, as it
 * DOES manipulate the reference counts!
 */

CVMClassTypeID
CVMtypeidIncrementArrayDepth(
    CVMExecEnv * ee,
    CVMClassTypeID base,
    int depthIncrement
){
    CVMFieldTypeID 	      arrayretval;
    struct scalarTableEntry * baseEntry;
    struct scalarTableEntry * arrayEntry;
    struct pkg *	      basePackage;
    CVMClassTypeID	      newDepth = CVMtypeidGetArrayDepth( base ) + depthIncrement;

    base = CVMtypeidGetArrayBasetype(base);
    if ( newDepth <= CVMtypeidMaxSmallArray ){
	if ( isTableEntry( base ) ){
	    base = CVMtypeidCloneClassID( ee, base );
	}
	return (newDepth<<CVMtypeidArrayShift)|base;
    }
    if ( CVMtypeidFieldIsRef( base ) ){
	baseEntry = indexScalarEntry( base , NULL);
	basePackage = baseEntry->value.className.package;
    } else {
	baseEntry   = NULL;
	basePackage = CVMnullPackage;
    }
    /*
     * look up the big array in the type table. We set
     * parameter doInsertion to TRUE for the case that an array of this
     * depth, of this base type has never been seen before.
     */
    CVMsysMutexLock(ee, &CVMglobals.typeidLock );
    arrayEntry = lookupArray( base, baseEntry, newDepth, basePackage,
	CVM_TRUE, &arrayretval );
    if ( arrayEntry == NULL ){
	arrayretval =  CVM_TYPEID_ERROR;
    } else {
	arrayretval |=  CVMtypeidBigArray;
    }
    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );

    return arrayretval;
}

CVMClassTypeID
CVMtypeidLookupClassID(
    CVMExecEnv * ee,
    const char * name,
    int nameLength
){
    CVMFieldTypeID retval = CVM_TYPEID_ERROR;
    struct scalarTableEntry * ep;
    if ( name[0] == CVM_SIGNATURE_ARRAY ){
	/* it starts with a [ so it is really an array */

	ep = referenceFieldSignature( ee, name, nameLength, CVM_FALSE, &retval );
    } else {
	/* It is a simple name */
	ep = referenceClassName( name, nameLength, CVM_FALSE, &retval );
	if ( ep == NULL ){
	    retval = CVM_TYPEID_ERROR;
	}
    }
    return retval;
}

CVMClassTypeID
CVMtypeidNewClassID(
    CVMExecEnv * ee,
    const char * name,
    int nameLength
){
    CVMFieldTypeID retval = CVM_TYPEID_ERROR;
    struct scalarTableEntry * ep;

    /* Detect an empty string */ 
    CVMassert(name[0] != '\0');

    CVMsysMutexLock(ee, &CVMglobals.typeidLock );
    if ( name[0] == CVM_SIGNATURE_ARRAY ){
	/* it starts with a [ so it is really an array */
	ep = referenceFieldSignature( ee, name, nameLength, CVM_TRUE, &retval );
    } else {
	/* It is a simple name */
	ep = referenceClassName( name, nameLength, CVM_TRUE, &retval );
	if ( ep == NULL ){
	    retval = CVM_TYPEID_ERROR;
	}
    }
    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
    return retval;
}

CVMMethodTypeID
CVMtypeidCloneClassID( CVMExecEnv *ee, CVMClassTypeID cookie ){
    CVMTypeIDTypePart typeCookie = (CVMTypeIDTypePart) (cookie&CVMtypeidBasetypeMask); /* note BASE TYPE ONLY */
    struct scalarTableEntry*	thisType;

    if ( isTableEntry( typeCookie ) ){
	CVMsysMutexLock(ee, &CVMglobals.typeidLock );
	thisType = indexScalarEntry( typeCookie, NULL );
	conditionalIncRef(thisType);
	CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
    }

    return cookie;
}

void
CVMtypeidDisposeClassID( CVMExecEnv *ee, CVMClassTypeID cookie ){
    CVMTypeIDTypePart typeCookie = (CVMTypeIDTypePart)(cookie&CVMtypeidBasetypeMask);

    if ( isTableEntry( typeCookie ) ){
	CVMsysMutexLock(ee, &CVMglobals.typeidLock );
	decrefScalarTypeEntry( (CVMTypeIDTypePart)cookie );
	CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
    }

}

/**********************************************************************/

/*
 * Sufficiently large forms are worth sharing.
 * Fortunately, there aren't many of them, so the lookup is primitive.
 */
static struct sigForm *
formLookup( CVMExecEnv *ee, CVMUint32 * datap, int nSyllables, CVMBool doInsertion ){
    struct sigForm * thisForm;
    int i;
    int nwords = FORM_DATAWORDS(nSyllables);
    int nParameters = nSyllables-2;
    for ( thisForm = *CVMformTablePtr; thisForm != NULL; thisForm = thisForm->next ){
	if ( thisForm->nParameters != nParameters )
	    continue;
	for ( i = 0; i < nwords; i++ ){
	    if ( datap[i] != thisForm->data[i] )
		goto notEqual;
	}
	/* seems to be the one */
	if ( doInsertion ){
	    conditionalIncRef( thisForm );
	}
	return thisForm;
    notEqual:; /* continue looking */
    }
    /* not found */
    if ( (thisForm==NULL) && doInsertion){
	ASSERT_LOCKED;
	thisForm = (struct sigForm *)malloc( sizeof(struct sigForm) + (nwords-1)*(sizeof(CVMUint32)) );
	if ( thisForm == NULL ){
	    unlockThrowOutOfMemoryError();
	} else {
	    thisForm->nParameters = nParameters;
	    for ( i = 0; i < nwords; i++ ){
		thisForm->data[i] = datap[i];
	    }
	    thisForm->refCount = 1;
	    thisForm->next = *CVMformTablePtr;
	    *CVMformTablePtr = thisForm;
#ifdef CVM_DEBUG
	    thisForm->state = TYPEID_STATE_ADDED;
	    idstat.nMethodFormsAdded++;
#endif
	}
    }
    return thisForm;
}

static struct methodTypeTableEntry *
findFreeMethodSigEntry( CVMMethodTypeID * newIndex ){
    struct methodTypeTableEntry * retval =  (struct methodTypeTableEntry *) findFreeTableEntry(
	(struct genericTableSegment *) &CVMMethodTypeTable,
	sizeof ( struct methodTypeTableEntry ),
	&CVMglobals.typeIDmethodTypeSegmentSize,
	newIndex );
    if ((retval!=NULL) && (*newIndex > CVM_TYPEID_MAX_SIG)){
	/*
	 * no malloc failure, but
	 * table overflow. Give it back and return a null.
	 */
	struct genericTableSegment *thisSeg;
	(void)indexMethodEntryNoCheck( *newIndex, &thisSeg );
	thisSeg->nFree += 1;
	retval->refCount = 0;
	unlockThrowInternalError( "Number of method signatures exceeds vm limit." );
	*newIndex = TYPEID_NOENTRY;
	return NULL;
    }
    return retval;
}

/*
 * Evaluation of methodTypeTableEntries:
 * The following function is needed to discover how many OBJ
 * things we have in a terse form, which determines where the
 * details array is (directly in details.data, or indirectly in
 * details.datap).
 * Parameters are:
 *	mp:	pointer to the methodTypeTableEntry of interest
 *	formp:  pointer to variable receiving the form-data pointer
 *	detailp: pointer to variable receiving the detail-pointer
 * Return value: 
 *	number of OBJ in the sig, including return value.
 */
static int
getSignatureInfo(
    struct methodTypeTableEntry *mp,
    CVMUint32** formp,
    CVMTypeIDTypePart** detailp )
{
    int	      nSyl = mp->nParameters+2;
    int	      nWords = FORM_DATAWORDS( nSyl );
    int       wordNo;
    CVMUint32 dataword;
    unsigned  syl;
    int       nObj = 0;
    CVMUint32 *formdata;

    formdata = (nSyl <= FORM_SYLLABLESPERDATUM)? & mp->form.formdata 
		: mp->form.formp->data;

    for ( wordNo = 0; wordNo < nWords; wordNo++ ){
	dataword = formdata[wordNo];
	while( (syl = dataword&0xf) != 0 ){
	    dataword >>= 4;
	    if (syl==CVM_TYPEID_OBJ)
		nObj += 1;
	}
    }
    if ( formp!= NULL ){
	*formp = formdata;
    }
    if ( detailp != NULL ){
	*detailp = (nObj <= N_METHOD_INLINE_DETAILS)
	    ?	mp->details.data : mp->details.datap;
    }
    return nObj;
}

/*
 * There are 4 subspecies of signatures:
 * 1) inline form, inline details
 * 2) inline form, out-of-line details,
 * 3) out-of-line form, inline details
 * 4) out-of-line form, out-of-line details.
 */
#define INLINE_FORM	0
#define OUTLINE_FORM	1
#define INLINE_DETAILS	0
#define OUTLINE_DETAILS	2

#ifdef CVM_TRACE
static const char * const signatureMatrix[] = {
    "inline form/inline detail",
    "outline form/inline detail",
    "inline form/outline detail",
    "outline form/outline detail"
};
#endif

static void
decrefForm( struct sigForm * formp ){
    if ( formp->refCount != MAX_COUNT ){
	if ( --(formp->refCount) == 0 ){
	    /*
	     * remove from linked list,
	     * then free the space.
	     */
	    struct sigForm ** formpp = CVMformTablePtr;
	    CVMassert( *formpp != NULL );
	    while ( *formpp != formp ){
		formpp = &((*formpp)->next);
		CVMassert( *formpp != NULL );
	    }
	    *formpp = formp->next;
#ifndef NO_RECYCLE
	    free( formp );
#else
#ifdef CVM_DEBUG
	    formp->state |= TYPEID_STATE_DELETED;
#endif
#endif
#ifdef CVM_DEBUG
	    idstat.nMethodFormsDeleted++;
#endif
	}
    }
}

static void
deleteMethodSignatureEntry(
    struct methodTypeTableEntry *   thisEntry,
    CVMTypeIDNamePart		    thisCookie,
    struct methodTypeTableSegment * thisSeg )
{
    int		nSyl;
    int		nDetail;
    int		sigSpecies;
    struct sigForm *formp;
    CVMUint32 *	formdata;
    CVMTypeIDTypePart* detailp;
    int		i;

    unsigned 	hashVal;
    /*
     * First, ascertain which species of signature we have here.
     * This determines how much work we have to do.
     */
    nDetail = getSignatureInfo( thisEntry, &formdata, &detailp );

    if ( (nSyl = thisEntry->nParameters+2) <= FORM_SYLLABLESPERDATUM){
	sigSpecies = INLINE_FORM;
	formp = NULL;
    } else {
	sigSpecies = OUTLINE_FORM;
	formp = thisEntry->form.formp;
    }

    if ( nDetail <= N_METHOD_INLINE_DETAILS ){
	sigSpecies |=  INLINE_DETAILS;
    } else {
	sigSpecies |=  OUTLINE_DETAILS;
    }
    CVMtraceTypeID(("Typeid: Deleting %s method signature at 0x%x\n",
	signatureMatrix[ sigSpecies ], thisEntry));

    /*
     * Now, take it off the hash chain.
     * and add to the segment's free count.
     */

    hashVal = ( formdata[0]<<16 ) + (nDetail>0 ? detailp[0] : 0 );
    hashVal %= NMETHODTYPEHASH;

    ASSERT_LOCKED;
    unlinkEntry( thisCookie, thisEntry->nextIndex,
	    &CVMMethodTypeHash[hashVal], 
	    (struct genericTableSegment*)&CVMMethodTypeTable,
	    (struct genericTableSegment*)thisSeg,
	    sizeof(struct methodTypeTableEntry) );
    /*
     * decRef the detail fields
     * COPIED from CVMtypeidDisposeFieldID
     * CONSIDER SUBROUTINIZING
     */
    for ( i = 0; i < nDetail; i++ ){
	decrefScalarTypeEntry( detailp[i] );
    }
    /*
     * The details array is not shared. Delete if external
     */
    if ( sigSpecies&OUTLINE_DETAILS ){
#ifndef NO_RECYCLE
	free( detailp );
#endif
#ifdef CVM_DEBUG
	idstat.nMethodDetailsDeleted++;
#endif
    }
    /*
     * The form structure, if external, is ref-counted.
     */
    if ( sigSpecies&OUTLINE_FORM ){
	decrefForm( formp );
    }

#ifdef CVM_DEBUG
    thisEntry->state |= TYPEID_STATE_DELETED;
    idstat.nMethodSigsDeleted++;
#endif
}

/* constants used by CVMtypeidMethodSignatureToType */
#define MAX_SIGITEMS	257
#define NLOCALDETAIL 6
#define NLOCALSYLLABLEWORDS 2
#define NLOCALSYLLABLES (NLOCALSYLLABLEWORDS*FORM_SYLLABLESPERDATUM)


static struct methodTypeTableEntry * 
lookupMethodSignature(
    CVMExecEnv *	ee,
    int			sigSpecies, /* combination of the above flags */
    int			nformwords,
    int			nSyllables,
    int			nDetail,
    CVMTypeIDTypePart*	hashbucket,
    CVMUint32 *		syllablep,
    CVMTypeIDTypePart*	detailp,
    CVMMethodTypeID *	methodCookie,
    CVMBool *		foundExistingEntry,
    CVMBool		doInsertion
){
    struct methodTypeTableEntry * thisSig;
    CVMMethodTypeID	thisSigNo;
    int			nParameters = nSyllables-2;

    /* DEBUG
	int i;
	CVMconsolePrintf("lookupMethodSignature: species %x, %d syllables, %d details\n", sigSpecies, nSyllables, nDetail );
	CVMconsolePrintf("	syllables:");
	for ( i = 0; i < nformwords; i++ ){
	    CVMconsolePrintf(" 0x%x", syllablep[0] );
	}
	if ( nDetail != 0 ){
	    CVMconsolePrintf("\n    details:");
	    for ( i = 0; i < nDetail; i++ ){
		if ( detailp[i] >= TYPEID_NOENTRY ){
		    CVMconsolePrintf(" ERROR" );
		} else {
		    CVMconsolePrintf(" %s",
			CVMtypeidFieldTypeToAllocatedCString( detailp[i] ) );
		}
	    }
	}
	CVMconsolePrintf("\n");

    */
    for ( thisSigNo = *hashbucket;
	thisSigNo != TYPEID_NOENTRY ;
	thisSigNo=thisSig->nextIndex
    ){
	CVMTypeIDTypePart *thisData;
	int	     detailword;
	thisSig = indexMethodEntry( thisSigNo , NULL);
	/*
	 * compare size of forms
	 */
	if ( thisSig->nParameters != nParameters )
	    continue;
	/*
	 * compare forms
	 */
	if (sigSpecies&OUTLINE_FORM){
	    CVMUint32 * thisForm = thisSig->form.formp->data;
	    int formword;
	    for ( formword = 0; formword < nformwords; formword++ ){
		if ( syllablep[formword] != thisForm[formword] )
		    goto continueSigLoop;
	    }
	} else {
	    if ( syllablep[0] != thisSig->form.formdata )
		goto continueSigLoop;
	}
	/*
	 * compare details.
	 * (if the forms are identical, so the number of details will be, too)
	 */
	thisData = (sigSpecies&OUTLINE_DETAILS) ? thisSig->details.datap : (thisSig->details.data );
	for ( detailword = 0; detailword < nDetail; detailword ++ ){
	    if ( detailp[detailword] != thisData[detailword] )
		goto continueSigLoop;
	}
	/*
	 * Everything compares equal. we have a winner
	 */
	*foundExistingEntry = CVM_TRUE;
	goto returnThisEntry;

    continueSigLoop:;
    }
    /*
     * Fell out of compare loop. Must make a new one and add it.
     * Or not.
     */
    thisSig = NULL;
    *foundExistingEntry = CVM_FALSE;

    if ( doInsertion ){
	/*
	 * is absent. Do all allocations first,
	 * then fill in all data.
	 */
	struct sigForm* externalForm  = NULL;
	CVMTypeIDTypePart*  externalDetail    = NULL;
	ASSERT_LOCKED;
	thisSig = findFreeMethodSigEntry( &thisSigNo );
	if ( thisSig == NULL ) goto mallocFailure;
	if ( sigSpecies&OUTLINE_FORM ){
	    externalForm = formLookup( ee, syllablep, nSyllables, CVM_TRUE );
	    if ( externalForm == NULL ) goto mallocFailure;
	}
	if ( sigSpecies&OUTLINE_DETAILS ){
	    externalDetail = (CVMTypeIDTypePart *)
		malloc( nDetail*sizeof(CVMTypeIDTypePart) );
	    if ( externalDetail == NULL ) {
		unlockThrowOutOfMemoryError();
		goto mallocFailure;
	    }
#ifdef CVM_DEBUG
	    idstat.nMethodDetailsAdded++;
#endif
	}
	/*
	 * all allocations done. finish building the data structures
	 * and linking this entry into the hash chain.
	 */
	thisSig->nParameters = nParameters;
	if ( sigSpecies&OUTLINE_FORM ){
	    thisSig->form.formp = externalForm;
	} else {
	    thisSig->form.formdata = syllablep[0];
	}
	if ( sigSpecies&OUTLINE_DETAILS ){
	    thisSig->details.datap = externalDetail;
	    memcpy( externalDetail, detailp, nDetail*sizeof(CVMTypeIDTypePart) );
	} else {
	    memcpy( thisSig->details.data, detailp, nDetail*sizeof(CVMTypeIDTypePart) );
	}
	thisSig->nextIndex = *hashbucket;
	thisSig->refCount = 0;
	*hashbucket = thisSigNo;
	CVMtraceTypeID(("Typeid: Creating %s method signature at 0x%x\n",
	    signatureMatrix[ sigSpecies ], thisSig));
#ifdef CVM_DEBUG
	idstat.nMethodSigsAdded++;
	thisSig->state |= TYPEID_STATE_ADDED;
#endif
	if ( CVM_FALSE ){
    mallocFailure:
	    /*
	     * This is where we end up only in the case
	     * of an allocation failure while trying to
	     * instantiate a new method type. Either the
	     * method sig table was full, or I could not
	     * allocate a new form, or I could not allocate
	     * a new external details list.
	     * See what is allocated, if anything, and 
	     * free it or un-ref-count it.
	     */
	    if ( thisSig!= NULL ){
		/* don't call deleteMethodSignatureEntry because
		 * it expects a fully-formed entry which is in the hash table.
		 * we don't have this. We just have a table entry to give back.
		 */
		struct genericTableSegment * thisSeg;
		(void)indexMethodEntryNoCheck( thisSigNo, &thisSeg );
		thisSeg->nFree += 1;
		thisSig = NULL;
	    }
	    if ( externalForm != NULL ){
		decrefForm( externalForm );
	    }
	    if ( externalDetail != NULL ){
		free( externalDetail );
	    }
	    thisSigNo = CVM_TYPEID_ERROR;
	}
    }

returnThisEntry:

    if ( thisSig!=NULL && doInsertion ){
	conditionalIncRef( thisSig );
    }
    if (methodCookie != NULL)
	*methodCookie = thisSigNo;
    return thisSig;
}

/*
 * Buffers used in referenceMethodSignature when the signatures get
 * too big to parse using the stack buffers. Make sure to acquire the
 * typeidLock before using them! This will hardly ever happen.
 * The macro is used in two places.
 */
static CVMTypeIDTypePart staticDetailBuffer[MAX_SIGITEMS];
static CVMUint32    staticSyllableBuffer[FORM_DATAWORDS(MAX_SIGITEMS)];

#define USE_STATIC_BUFFERS \
    if ( !doInsertion ){ \
	needToUnlock = CVM_TRUE; \
	CVMsysMutexLock(ee, &CVMglobals.typeidLock ); \
    } \
    detailp = staticDetailBuffer; \
    memcpy( detailp, localDetailBuffer, NLOCALDETAIL*sizeof(localDetailBuffer[0]) ); \
    \
    syllablep = staticSyllableBuffer; \
    memcpy( syllablep, localSyllableBuffer, NLOCALSYLLABLEWORDS*sizeof(localSyllableBuffer[0]) ); \
    memset( &syllablep[NLOCALSYLLABLEWORDS], 0, \
	sizeof(CVMUint32) * (FORM_DATAWORDS(MAX_SIGITEMS)-NLOCALSYLLABLEWORDS) );

static struct methodTypeTableEntry *
referenceMethodSignature(
    CVMExecEnv * ee,
    const char * sig,
    int sigLength,
    CVMTypeIDNamePart *nameCookie, 
    CVMBool doInsertion )
{
    CVMTypeIDTypePart localDetailBuffer[NLOCALDETAIL];
    CVMUint32	localSyllableBuffer[NLOCALSYLLABLEWORDS];
    const char*	subtypeStart;
    const char*	endSig;
    CVMTypeIDTypePart*	detailp;
    CVMUint32*	syllablep;
    int		nDetail = 0;
    int		nSyllables = 1; /* reserve one for return type! */
    CVMFieldTypeID thisSyllable;
    CVMFieldTypeID thisDetail;
    int		isReturnValue = 0;
    int		foundReturnValue = 0;
    int		sigFlavor, nformword;
    int		copyCtr;
    struct methodTypeTableEntry * thisSig = NULL;
    CVMBool	foundExistingEntry;
    CVMMethodTypeID	thisSigNo = TYPEID_NOENTRY;
    CVMUint32	hashBucket;
    CVMBool	needToUnlock = CVM_FALSE; /* set by USE_STATIC_BUFFERS */

    detailp = localDetailBuffer;
    syllablep = localSyllableBuffer;
    endSig = sig+sigLength;

    memset( localSyllableBuffer, 0, NLOCALSYLLABLEWORDS*sizeof(localSyllableBuffer[0]));
    if ( sig[0] != CVM_SIGNATURE_FUNC ) {
	if ( nameCookie != NULL )
	    *nameCookie = TYPEID_NOENTRY;
	return NULL;
    }
    sig += 1;
    sigLength -= 1;
    while ( sig < endSig ){
	if ( foundReturnValue ){
	    /* CVMconsolePrintf("referenceMethodSignature failing: excess post-return value\n"); */
	    goto parseFailure;
	}
	switch ( *sig ){
	case CVM_SIGNATURE_VOID:
	    if (!isReturnValue){
		/* CVMconsolePrintf("referenceMethodSignature failing: void parameter\n"); */
		goto parseFailure; /* no void parameters! */
	    }
	    thisSyllable = CVM_TYPEID_VOID;
	    break;
	case CVM_SIGNATURE_INT:
	    thisSyllable = CVM_TYPEID_INT;
	    break;
	case CVM_SIGNATURE_SHORT:
	    thisSyllable = CVM_TYPEID_SHORT;
	    break;
	case CVM_SIGNATURE_CHAR:
	    thisSyllable = CVM_TYPEID_CHAR;
	    break;
	case CVM_SIGNATURE_LONG:
	    thisSyllable = CVM_TYPEID_LONG;
	    break;
	case CVM_SIGNATURE_BYTE:
	    thisSyllable = CVM_TYPEID_BYTE;
	    break;
	case CVM_SIGNATURE_FLOAT:
	    thisSyllable = CVM_TYPEID_FLOAT;
	    break;
	case CVM_SIGNATURE_DOUBLE:
	    thisSyllable = CVM_TYPEID_DOUBLE;
	    break;
	case CVM_SIGNATURE_BOOLEAN:
	    thisSyllable = CVM_TYPEID_BOOLEAN;
	    break;
	case CVM_SIGNATURE_ARRAY:
	    subtypeStart = sig++;
	    while ( sig < endSig && *sig == CVM_SIGNATURE_ARRAY ){
		sig += 1;
	    }
	    if ( sig >= endSig ){
		/* bummer */
		/* CVMconsolePrintf("referenceMethodSignature failing: mal-formed array type\n"); */
		goto parseFailure; /* no void parameters! */
	    }
	    if  ( *sig == CVM_SIGNATURE_CLASS ){
		sig += 1;
		goto doClasstype;
	    }
	    goto doObjecttype;

	case CVM_SIGNATURE_CLASS:
	    subtypeStart = sig++;

	doClasstype:
	    while ( sig < endSig && *sig != CVM_SIGNATURE_ENDCLASS ){
		sig += 1;
	    }
	    if ( sig >= endSig ){
		/* bummer */
		/* CVMconsolePrintf("referenceMethodSignature failing: mal-formed class type\n"); */
		goto parseFailure; /* no void parameters! */
	    }

	doObjecttype:
	    /*
	     * Now, subtypeStart points at the initial [ or L
	     * and sig points at the last character of this parameter.
	     */
	    referenceFieldSignature( ee, subtypeStart, (int)(sig-subtypeStart+1), doInsertion, &thisDetail );
	    /*
	     * if isReturnValue, then detail gets inserted at the beginning of the detail
	     * array (yeucch), otherwise it gets appended at end.
	     * Combine this with the possibility of overflowing our local buffer and needing
	     * to use the static one, and we have 4 choices. 
	     * We will deal with this in a simple but ineffecient manner.
	     */
	    if ( nDetail == NLOCALDETAIL && detailp == localDetailBuffer ){
		/* local buffers inadequate, use static buffers */
		USE_STATIC_BUFFERS
	    }
	    if ( isReturnValue ){
		for (copyCtr = nDetail; copyCtr > 0; copyCtr--) {
		    detailp[copyCtr] = detailp[copyCtr-1];
		}
		nDetail++;
		detailp[0] = thisDetail;
	    } else {
		detailp[nDetail++] = thisDetail;
	    }
	    thisSyllable = CVM_TYPEID_OBJ;
	    break;

	case CVM_SIGNATURE_ENDFUNC:
	    /* if (isReturnValue) already, this is an error */
	    if (isReturnValue){
		/* CVMconsolePrintf("referenceMethodSignature failing: duplicate )\n"); */
		goto parseFailure; /* no void parameters! */
	    }
	    thisSyllable = CVM_TYPEID_ENDFUNC;
	    break;

	default:
	    goto parseFailure; /* unrecognized syllable */
	}
	sig += 1;

	if ( isReturnValue ){
	    syllablep[0] |= thisSyllable;
	    foundReturnValue = 1;
	} else {
	    if ( (nSyllables == NLOCALSYLLABLES) && (syllablep==localSyllableBuffer) ){
		/*
		 * About to overflow local buffer. 
		 */
		USE_STATIC_BUFFERS
	    } else if ( nSyllables >= MAX_SIGITEMS ){
		/*
		 * MAX_SIGITEMS = 257:
		 *	255 parameters (acutally less in many cases)
		 *	1   ENDFUNC
		 *	1   return value.
		 * Here we have too many:
		 *     syllable 0 is the return value
		 *     syllable 1 is the 1st parameter
		 *     ...
		 *     syllable n is the ENDFUNC marker
		 *         for n >0 && n <= 256
		 */
		/* CVMconsolePrintf("referenceMethodSignature failing: too many parameters\n"); */
		goto parseFailure;
	    }
	    syllablep[ nSyllables/FORM_SYLLABLESPERDATUM ] |= thisSyllable<<(4*(nSyllables%FORM_SYLLABLESPERDATUM));
	    nSyllables += 1;
	    if ( thisSyllable == CVM_TYPEID_ENDFUNC ){
		/* the next type is the return type */
		isReturnValue = 1;
	    }
	}
    }
    if ( ! foundReturnValue ){
		/* CVMconsolePrintf("referenceMethodSignature failing: no return value\n"); */
		goto parseFailure;
    }
    /*
     * if ( nSyllables > 20 ){
     *	CVMconsolePrintf("Whopper signature has %d syllables\n", nSyllables);
     * }
     */

    sigFlavor  = ( nSyllables <= FORM_SYLLABLESPERDATUM ) ? INLINE_FORM : OUTLINE_FORM;
    sigFlavor |= ( nDetail <= N_METHOD_INLINE_DETAILS ) ? INLINE_DETAILS : OUTLINE_DETAILS;
    nformword = FORM_DATAWORDS(nSyllables);

    /* compute hash value. index into hash table */
    hashBucket = ( syllablep[0]<<16 ) + (nDetail>0 ? detailp[0] : 0 );
    hashBucket %= NMETHODTYPEHASH;

    thisSig = lookupMethodSignature(
	ee, sigFlavor, nformword, nSyllables, nDetail,
	&CVMMethodTypeHash[hashBucket], syllablep, detailp,
	&thisSigNo, &foundExistingEntry, doInsertion );
    /*DEBUG
	if ( thisSig == NULL ){
	    CVMconsolePrintf("referenceMethodSignature failing: lookup failed\n");
	}
    */

    if ( foundExistingEntry && doInsertion ){
	/*
	 * when looking up the details, we bumped their ref counts
	 * needlessly. Undo that.
	 */
	int i;
	CVMtraceTypeID(("Typeid: Component adjustment for method signature at 0x%x\n",
	    thisSig));

	for( i = 0 ; i < nDetail ; i++ ){
	    decrefScalarTypeEntry( detailp[i] );
	}
    }
    if (needToUnlock){
	CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
    }
    if ( nameCookie != NULL )
	*nameCookie = thisSigNo;
    return thisSig;

parseFailure:
    /* Any input parse errors come here, to 
     * clean up before returning.
     */
    if ( doInsertion ){
	/*
	 * when looking up the details, we bumped their ref counts
	 * needlessly. Undo that.
	 */
	int i;
	for( i = 0 ; i < nDetail ; i++ ){
	    decrefScalarTypeEntry( detailp[i] );
	}
    }
    if (needToUnlock){
	CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
    }
    if ( nameCookie != NULL )
	*nameCookie = TYPEID_NOENTRY;
    return NULL;
}

/*
 * Convert type ID to string for printouts
 */
CVMBool
CVMtypeidMethodTypeToCString(CVMMethodTypeID t, char* buf, int bufLength )
{
    char * chp;
    CVMSigIterator sigIterator;
    int		i;
    CVMBool	success = CVM_TRUE;

    chp = buf;

    CVMtypeidGetSignatureIterator( t, &sigIterator );

    conditionalPutchar( &chp, &bufLength, '(', &success );
    while ( (i=CVM_SIGNATURE_ITER_NEXT( sigIterator ) ) != CVM_TYPEID_ENDFUNC ){
	switch ( i ){

	BASETYPE_CASES

	default: {
		int thisLength;
		CVMtypeidFieldTypeToCString( i, chp, bufLength);
		chp += (thisLength = (int)strlen(chp) );
		bufLength -=  thisLength;
		break;
	    }
	}
    }
    conditionalPutchar( &chp, &bufLength, ')', &success );
    if ( !success ) return CVM_FALSE; /* bail early if we've alread overrun */
    /* Return type */
    switch ( i=CVM_SIGNATURE_ITER_RETURNTYPE( sigIterator  ) ){

    BASETYPE_CASES

    default: {
	    int thisLength;
	    success = CVMtypeidFieldTypeToCString( i, chp, bufLength);
	    chp += (thisLength = (int)strlen(chp) );
	    bufLength -=  thisLength;
	    break;
	}
    }
    *chp++ = '\0'; /* there's always room for nil */
    return success;
}

char *
CVMtypeidMethodTypeToAllocatedCString( CVMMethodTypeID t ){
    int length = (int)CVMtypeidMethodTypeLength(t) + 1;
    char * buf = (char *)malloc( length );
    if ( buf == NULL )
	return NULL; /* malloc failure */
    CVMtypeidMethodTypeToCString(t, buf, length );
    return buf;
}

static CVMBool
CVMtypeidPrivateFieldTypeToCString(CVMFieldTypeID t, char* buf, int bufLength, CVMBool isField) {
    char * chp;
    CVMAddr depth;
    CVMBool success = CVM_TRUE;

    chp = buf;
    t &= CVMtypeidTypeMask;
    if ( CVMtypeidIsBigArray(t) ){
	struct scalarTableEntry * arrayp;
	arrayp = indexScalarEntry( t&CVMtypeidBasetypeMask, NULL);
	depth = arrayp->value.bigArray.depth;
	t     = arrayp->value.bigArray.basetype;
    } else {
	depth = t>>CVMtypeidArrayShift;
	t &= CVMtypeidBasetypeMask;
    }
    while ( depth-->0){
	conditionalPutchar( &chp, &bufLength, '[', &success );
	isField = CVM_TRUE;
    }
    switch( t ){
    case TYPEID_NOENTRY:
    case CVM_TYPEID_ERROR: conditionalPutstring( &chp, &bufLength, "ERROR", sizeof("ERROR")-1, &success );
			   break;

    BASETYPE_CASES

    default: {
	struct scalarTableEntry * basep = indexScalarEntry( t , NULL);
	if ( isField )
	    conditionalPutchar( &chp, &bufLength, 'L', &success );
	if ( (basep->value.className.package != NULL) &&
	     (basep->value.className.package->pkgname != NULL) &&
	     (basep->value.className.package->pkgname[0] != 0) ){
	    conditionalPutstring( &chp, &bufLength, basep->value.className.package->pkgname,
		(int)strlen(basep->value.className.package->pkgname), &success );
	    conditionalPutchar( &chp, &bufLength, '/', &success );
	}
	conditionalPutstring( &chp, &bufLength, basep->value.className.classInPackage, 
		(int)strlen(basep->value.className.classInPackage ), &success );
	if ( isField ){
	    conditionalPutchar( &chp, &bufLength, ';', &success );
	}
	}
    }
    *chp = '\0';
    return success;
}

CVMBool
CVMtypeidFieldTypeToCString(CVMFieldTypeID t, char* buf, int bufLength) {
    return CVMtypeidPrivateFieldTypeToCString(t, buf, bufLength, CVM_TRUE );
}

char *
CVMtypeidFieldTypeToAllocatedCString( CVMFieldTypeID t ){
    int    bufLength = (int)CVMtypeidFieldTypeLength( t ) +1;
    char * buf = (char *)malloc( bufLength );
    if ( buf == NULL )
	return NULL; /* malloc failure */
    CVMtypeidPrivateFieldTypeToCString(t, buf, bufLength, CVM_TRUE );
    return buf;
}


CVMBool
CVMtypeidClassNameToCString(CVMClassTypeID t, char* buf, int bufLength) {
    return CVMtypeidPrivateFieldTypeToCString(t, buf, bufLength, CVM_FALSE );
}

CVMBool
CVMtypeidMethodNameToCString(CVMMethodTypeID type, char*buf, int bufLength ){
    struct memberName * mn = indexMemberName( CVMtypeidGetNamePart(type), NULL);
    CVMBool success = CVM_TRUE;
    conditionalPutstring( &buf, &bufLength, mn->name, (int)strlen( mn->name ), &success );
    return success;
}

CVMBool
CVMtypeidFieldNameToCString(CVMFieldTypeID type, char*buf, int bufLength ){
    struct memberName * mn = indexMemberName( CVMtypeidGetNamePart(type) , NULL);
    CVMBool success = CVM_TRUE;
    conditionalPutstring( &buf, &bufLength, mn->name, (int)strlen( mn->name ), &success );
    return success;
}

char *
CVMtypeidMethodNameToAllocatedCString( CVMMethodTypeID type ){
    struct memberName * mn = indexMemberName( CVMtypeidGetNamePart(type), NULL);
    char * result = (char *)malloc( strlen( mn->name ) +1 );
    if ( result == NULL )
	return NULL; /* malloc failure */
    strcpy( result, mn->name );
    return result;
}

char *
CVMtypeidFieldNameToAllocatedCString( CVMFieldTypeID type ){
    return CVMtypeidMethodNameToAllocatedCString( (CVMMethodTypeID)type );
}

char *
CVMtypeidClassNameToAllocatedCString( CVMClassTypeID t ){
    int    bufLength = (int)CVMtypeidClassNameLength( t ) +1;
    char * buf = (char *)malloc( bufLength );
    if ( buf == NULL )
	return NULL; /* malloc failure */
    CVMtypeidPrivateFieldTypeToCString(t, buf, bufLength, CVM_FALSE );
    return buf;
}

size_t
CVMtypeidFieldTypeLength0(CVMFieldTypeID t, CVMBool isField) {
    CVMAddr depth;
    size_t length = 0;

    t &= CVMtypeidTypeMask;
    if ( CVMtypeidIsBigArray(t) ){
	struct scalarTableEntry * arrayp;
	arrayp = indexScalarEntry( t&CVMtypeidBasetypeMask, NULL);
	depth = arrayp->value.bigArray.depth;
	t     = arrayp->value.bigArray.basetype;
    } else {
	depth = t>>CVMtypeidArrayShift;
	t &= CVMtypeidBasetypeMask;
    }
    if ( depth>0){
	length += depth;
	isField = CVM_TRUE;
    }
    if ( CVMtypeidFieldIsRef( t ) ){
	struct scalarTableEntry * basep = indexScalarEntry( t , NULL);
	if ( (basep->value.className.package != NULL) &&
	     (basep->value.className.package->pkgname != NULL) &&
	     (basep->value.className.package->pkgname[0] != 0) ){
	    length +=  strlen( basep->value.className.package->pkgname ) + 1; /* include trailing slash */
	}
	length += strlen(basep->value.className.classInPackage );
	if (isField)
	    length += 2; /* leading L trailing ; */
	return length;
    } else {
	return length+1; /* one char for base name, and some number for the leading ['s */
    }
}

size_t
CVMtypeidMemberNameLength(CVMMethodTypeID type ) {
    struct memberName * mn = indexMemberName( CVMtypeidGetNamePart(type) , NULL);
    return strlen( mn->name );
}

size_t
CVMtypeidMethodTypeLength(CVMMethodTypeID t ){
    size_t	length;
    CVMSigIterator sigIterator;
    int		i;

    CVMtypeidGetSignatureIterator( t, &sigIterator );

    length = 2; /* one for ( and one for ) */
    while ( (i=CVM_SIGNATURE_ITER_NEXT( sigIterator ) ) != CVM_TYPEID_ENDFUNC ){
	if ( CVMtypeidFieldIsRef( i ) ){
	    length += CVMtypeidFieldTypeLength(i);
	} else {
	    length += 1;
	}
    }

    /* Return type */
    if ( CVMtypeidFieldIsRef(i=CVM_SIGNATURE_ITER_RETURNTYPE( sigIterator  ) ) ){
	length += CVMtypeidFieldTypeLength(i);
    } else {
	length += 1;
    }
    return length;
}


/*****************************************************************
 *
 * The public entry point for manipulating the name/type
 * pairs, both for methods and for data.
 */

/*
 * Make a type ID out of a method name and a signature
 * No locking, no reference counting, no assurance that it will
 * exist by the time it is returned to you.
 */
CVMMethodTypeID
CVMtypeidLookupMethodIDFromNameAndSig(
		CVMExecEnv *ee, const CVMUtf8* memberName,
		const CVMUtf8* memberSig)
{
    struct memberName * name;
    struct methodTypeTableEntry * sig;
    CVMTypeIDNamePart	nameCookie;
    CVMTypeIDTypePart	sigCookie;
    CVMMethodTypeID	retCookie;

    name = referenceMemberName( ee, memberName, &nameCookie, CVM_FALSE );
    if (name==NULL){
	/* no such name */
	return CVM_TYPEID_ERROR;
    }

    sig = referenceMethodSignature( ee, memberSig, (int)strlen(memberSig), &sigCookie, CVM_FALSE );
    if (sig==NULL){
	/* there was a parse or malloc failure. somewhere. */
	return CVM_TYPEID_ERROR;
    }
    retCookie = CVMtypeidCreateTypeIDFromParts(nameCookie, sigCookie);

    return retCookie;
}

CVMMethodTypeID
CVMtypeidNewMethodIDFromNameAndSig(
		CVMExecEnv *ee, const CVMUtf8* memberName,
		const CVMUtf8* memberSig)
{
    struct memberName * name;
    struct methodTypeTableEntry * sig;
    CVMTypeIDNamePart	nameCookie;
    CVMTypeIDTypePart	sigCookie;
    CVMMethodTypeID	result;

    CVMsysMutexLock(ee, &CVMglobals.typeidLock );
    name = referenceMemberName( ee, memberName, &nameCookie, CVM_TRUE );
    if (name==NULL){
	result = CVM_TYPEID_ERROR;
	goto exit;
    }

    sig = referenceMethodSignature( ee, memberSig, (int)strlen(memberSig), &sigCookie, CVM_TRUE );
    if (sig==NULL){
	/* there was a parse or malloc failure. somewhere. */
	/* delete anything that needs deleting */
	/* Since we call the reference functions with doInsertion == CVM_TRUE,
	 * a reference count of 1 means that an entry was just inserted by
	 * us, so we should delete it.
	 */
	if ( (name != NULL) && (name->refCount == 1) ){
	    name->refCount = 0;
	    deleteMemberEntry( name, nameCookie, NULL );
	}
	result = CVM_TYPEID_ERROR;
	goto exit;
    }
    result = CVMtypeidCreateTypeIDFromParts(nameCookie, sigCookie);
exit:
    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
    return result;
}


CVMMethodTypeID
CVMtypeidCloneMethodID( CVMExecEnv *ee, CVMMethodTypeID cookie ){
    CVMTypeIDNamePart nameCookie = CVMtypeidGetNamePart(cookie);
    CVMTypeIDTypePart typeCookie = CVMtypeidGetTypePart(cookie);
    struct memberName *			thisName;
    struct methodTypeTableEntry*	thisType;

    CVMsysMutexLock(ee, &CVMglobals.typeidLock );

    thisName = indexMemberName( nameCookie, NULL );
    conditionalIncRef(thisName);

    thisType = indexMethodEntry( typeCookie, NULL );
    conditionalIncRef(thisType);

    /*
	CVMconsolePrintf("typeidCloneMethodID of name 0x%x->refCount to %d\n", nameCookie, thisName->refCount);
	CVMconsolePrintf("               and  of type 0x%x->refCount to %d\n", typeCookie, thisType->refCount);
    */

    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );

    return cookie;
}


void
CVMtypeidDisposeMethodID( CVMExecEnv *ee, CVMMethodTypeID cookie ){
    CVMTypeIDNamePart nameCookie = CVMtypeidGetNamePart(cookie);
    CVMTypeIDTypePart typeCookie = CVMtypeidGetTypePart(cookie);
    struct memberName *			thisName;
    struct genericTableSegment*	        nameSeg;
    struct methodTypeTableEntry*	thisType;
    struct genericTableSegment *	typeSeg;

    CVMsysMutexLock(ee, &CVMglobals.typeidLock );

    thisName = indexMemberName( nameCookie, &nameSeg );
    if ( thisName->refCount != MAX_COUNT ){
	if ( --(thisName->refCount) == 0 ){
	    deleteMemberEntry( thisName, nameCookie,
			       (struct memberNameTableSegment*)nameSeg );
	}
    }

    thisType = indexMethodEntry( typeCookie, &typeSeg );
    if ( thisType->refCount != MAX_COUNT ){
	if ( --(thisType->refCount) == 0 ){
	    deleteMethodSignatureEntry( thisType, typeCookie,
					(struct methodTypeTableSegment *)
					typeSeg);
	}
    }

    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
}

/*
 * Make a type ID out of a field name and a signature
 */
CVMFieldTypeID
CVMtypeidLookupFieldIDFromNameAndSig( CVMExecEnv * ee, const CVMUtf8* memberName,
			 const CVMUtf8* memberSig)
{
    struct memberName * name;
    struct scalarTableEntry *type;
    CVMTypeIDNamePart	nameCookie;
    CVMFieldTypeID	sigCookie;
    CVMFieldTypeID	result;

    name = referenceMemberName( ee, memberName, &nameCookie, CVM_FALSE );
    if ( name==NULL ){
	/* no such name */
	return CVM_TYPEID_ERROR;
    }
    type = referenceFieldSignature( ee, memberSig, (int)strlen(memberSig), CVM_FALSE, &sigCookie );
    if (sigCookie>=TYPEID_NOENTRY){
	/* there was a malloc or lookup failure. somewhere. */
	/* any error has already been thrown */
	return CVM_TYPEID_ERROR;
    }
    result = CVMtypeidCreateTypeIDFromParts(nameCookie, sigCookie);
    return result;
}

CVMFieldTypeID
CVMtypeidNewFieldIDFromNameAndSig( CVMExecEnv * ee, const CVMUtf8* memberName,
			 const CVMUtf8* memberSig)
{
    struct memberName * name;
    struct scalarTableEntry *type;
    CVMTypeIDNamePart	nameCookie;
    CVMFieldTypeID	sigCookie;
    CVMFieldTypeID	result;

    CVMsysMutexLock(ee, &CVMglobals.typeidLock );
    name = referenceMemberName( ee, memberName, &nameCookie, CVM_TRUE );
    if ( name == NULL ){
	result = CVM_TYPEID_ERROR;
	goto exit;
    }
    type = referenceFieldSignature( ee, memberSig, (int)strlen(memberSig), CVM_TRUE, &sigCookie );
    if (sigCookie>=TYPEID_NOENTRY){
	/* there was a malloc failure in referenceFieldSignature */
	/* Since we call the reference functions with doInsertion == CVM_TRUE,
	 * a reference count of 1 means that an entry was just inserted by
	 * us, so we should delete it.
	 */
	if (name->refCount == 1){
	    name->refCount = 0;
	    deleteMemberEntry( name, nameCookie, NULL );
	}
	result = CVM_TYPEID_ERROR;
	goto exit;
    }
    result = CVMtypeidCreateTypeIDFromParts(nameCookie, sigCookie);

exit:
    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );

    return result;
}

CVMMethodTypeID
CVMtypeidCloneFieldID( CVMExecEnv *ee, CVMFieldTypeID cookie ){
    CVMTypeIDNamePart nameCookie = CVMtypeidGetNamePart(cookie);
    CVMTypeIDTypePart typeCookie = CVMtypeidGetTypePart(cookie); /* note BASE TYPE ONLY */
    struct memberName *		thisName;
    struct scalarTableEntry*	thisType;

    CVMsysMutexLock(ee, &CVMglobals.typeidLock );

    thisName = indexMemberName( nameCookie, NULL );
    conditionalIncRef(thisName);

    if ( isTableEntry( typeCookie ) ){
	thisType = indexScalarEntry( typeCookie, NULL );
	conditionalIncRef(thisType);
    }

    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );

    return cookie;
}


void
CVMtypeidDisposeFieldID( CVMExecEnv *ee, CVMFieldTypeID cookie ){
    CVMTypeIDNamePart nameCookie = CVMtypeidGetNamePart(cookie);
    CVMTypeIDTypePart typeCookie = CVMtypeidGetTypePart(cookie);
    struct memberName *			thisName;
    struct genericTableSegment*		nameSeg;

    CVMsysMutexLock(ee, &CVMglobals.typeidLock );

    thisName = indexMemberName( nameCookie, &nameSeg );
    if ( thisName->refCount != MAX_COUNT ){
	if ( --(thisName->refCount) == 0 ){
	    deleteMemberEntry( thisName, nameCookie,
			       (struct memberNameTableSegment*)nameSeg );
	}
    }

    if ( isTableEntry( typeCookie ) ){
	decrefScalarTypeEntry( typeCookie );
    }

    CVMsysMutexUnlock(ee, &CVMglobals.typeidLock );
}


/*****************************************************************/

char
CVMtypeidGetReturnType(CVMMethodTypeID type)
{
    struct methodTypeTableEntry * thisEntry;
    thisEntry = indexMethodEntry( type& CVMtypeidTypeMask , NULL);
    return ((thisEntry->nParameters<=(FORM_SYLLABLESPERDATUM-2)) ? (thisEntry->form.formdata) : (thisEntry->form.formp->data[0]) ) & 0xf;
}

/*
 * Returns true if the ID is a ref. This works for methods that return 
 * references.
 * CVMtypeidFieldIsRef is a macro.
 */

CVMBool
CVMtypeidMethodReturnsRef( CVMMethodTypeID type )
{
    return CVMtypeidGetReturnType( type ) == CVM_TYPEID_OBJ;
}

/*
 * Return information about a method signature's terse form.
 */
void
CVMtypeidGetTerseSignature( CVMMethodTypeID tid, CVMterseSig * tsp ){
    struct methodTypeTableEntry * mtp = indexMethodEntry( tid&CVMtypeidTypeMask , NULL);
    if ( (tsp->nParameters = mtp->nParameters) <= (FORM_SYLLABLESPERDATUM-2) ){
	tsp->datap = &(mtp->form.formdata);
    } else {
	tsp->datap = mtp->form.formp->data;
    }
}

void
CVMtypeidGetTerseSignatureIterator( CVMMethodTypeID tid, CVMterseSigIterator * tsp ){
    struct methodTypeTableEntry * mtp = indexMethodEntry( tid&CVMtypeidTypeMask , NULL);
    if ( (tsp->thisSig.nParameters = mtp->nParameters) <= (FORM_SYLLABLESPERDATUM-2) ){
	tsp->thisSig.datap = &(mtp->form.formdata);
    } else {
	tsp->thisSig.datap = mtp->form.formp->data;
    }
    tsp->word = 0;
    tsp->syllableInWord = 1; /* since 0 is return type */
}

void
CVMtypeidGetSignatureIterator( CVMMethodTypeID tid, CVMSigIterator* result ){
    struct methodTypeTableEntry * m;
    CVMUint32*  formdata;
    int	        nDetail;
    CVMTypeIDTypePart returnType;

    m = indexMethodEntry( tid&CVMtypeidTypeMask , NULL);
    /*
     * Have to look at the form to determine how many
     * details there are.
     */
    nDetail = getSignatureInfo( m, &formdata, &(result->parameterDetails) );

    CVMtypeidGetTerseSignatureIterator( tid, &(result->terseSig) );
    returnType = CVM_TERSE_ITER_RETURNTYPE( (result->terseSig) );
    if ( returnType == CVM_TYPEID_OBJ ){
	result->returnType = *(result->parameterDetails++);
    } else {
	result->returnType = returnType;
    }
}

/*
 * CVMtypeidGetArgsSize - returns the total number of words that the
 * method arguments occupy.
 *
 * WARNING: does not account for the "this" argument.
 */
CVMUint16
CVMtypeidGetArgsSize( CVMMethodTypeID methodTypeID )
{
    CVMUint32 argType;
    CVMterseSigIterator terseSig;
    CVMUint16 argsSize = 0;
    
    CVMtypeidGetTerseSignatureIterator(methodTypeID, &terseSig);
    do {
	argType = CVM_TERSE_ITER_NEXT(terseSig);
	switch (argType) {
	case CVM_TYPEID_ENDFUNC:
	    break;
	case CVM_TYPEID_LONG:
	case CVM_TYPEID_DOUBLE:
	    argsSize += 2;
	    break;
	case CVM_TYPEID_INT:
	case CVM_TYPEID_SHORT:
	case CVM_TYPEID_CHAR:
	case CVM_TYPEID_BYTE:
	case CVM_TYPEID_FLOAT:
	case CVM_TYPEID_BOOLEAN:
	case CVM_TYPEID_OBJ:
	    argsSize++;
	    break;
	default:
	    CVMassert(CVM_FALSE);
	}
    } while (argType != CVM_TYPEID_ENDFUNC);

    return argsSize;
}

#ifdef CVM_JIT
/*
 * Returns the total number of arguments that the method has.
 *
 * WARNING: does not account for the "this" argument.
 */
extern CVMUint16
CVMtypeidGetArgsCount( CVMMethodTypeID methodTypeID )
{
    struct methodTypeTableEntry *mtp =
        indexMethodEntry(methodTypeID & CVMtypeidTypeMask, NULL);
    return mtp->nParameters;
}

#endif

