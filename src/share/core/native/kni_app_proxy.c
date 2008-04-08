/*
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
 */

#include <jsrop_kni.h>
#include <jsrop_suitestore.h>
#include <javautil_unicode.h>
#include <javacall_memory.h>

#ifdef _DEBUG
#define TRACE_MIDLETREG
#endif

typedef struct _MidletIdChain {
    SuiteIdType             suiteId;
    javacall_utf16_string   className;
    struct _MidletIdChain * next;
} MidletIdChain;

static MidletIdChain * runningMidletsChain = NULL;

static MidletIdChain * newMidletIdChain( void ) {
    MidletIdChain * elem = (MidletIdChain *)JAVAME_MALLOC( sizeof(*elem) );
    if( elem != NULL ) memset( elem, '\0', sizeof(*elem) );
    return elem;
}

static void destroyMidletIdChain( MidletIdChain * elem ){
    JAVAME_FREE( elem );
}

static int compareMidletIdChain( const MidletIdChain * elem, 
                    SuiteIdType suiteId, javacall_utf16_string midletClassName ){
    javacall_int32 comparison;
    // assert( elem != NULL );
    if( elem->suiteId != suiteId )
        return elem->suiteId - suiteId;
    javautil_unicode_cmp(elem->className, midletClassName, &comparison);
    return comparison;
}

static MidletIdChain ** findMidletIdChain( SuiteIdType suiteId, javacall_utf16_string midletClassName ) {
    MidletIdChain ** p = &runningMidletsChain;
    while( *p != NULL && compareMidletIdChain( *p, suiteId, midletClassName ) < 0 )
        p = &(*p)->next;
    return p;
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_j2me_content_AppProxy_midletIsAdded) {
    SuiteIdType suiteId;
    KNI_StartHandles(1);

    suiteId = KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_UTF16_STRING(2, midletClassName)

    MidletIdChain ** elemPlace, * elem;
#ifdef TRACE_MIDLETREG
    printf( "AppProxy_midletIsAdded: %d, '%ls'\n", suiteId, midletClassName );
#endif
    elemPlace = findMidletIdChain( suiteId, midletClassName );
    if( *elemPlace == NULL || compareMidletIdChain(*elemPlace, suiteId, midletClassName) != 0 ){
        if( (elem = newMidletIdChain()) == NULL ){
            KNI_ThrowNew(jsropOutOfMemoryError, NULL);
        } else {
            elem->suiteId = suiteId;
            elem->className = midletClassName;
            midletClassName = NULL;
            elem->next = *elemPlace;
            *elemPlace = elem;
        }
    }

    RELEASE_UTF16_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_j2me_content_AppProxy_midletIsRemoved) {
    SuiteIdType suiteId;
    KNI_StartHandles(1);

    suiteId = KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_UTF16_STRING(2, midletClassName)

    MidletIdChain ** elemPlace;
#ifdef TRACE_MIDLETREG
    printf( "AppProxy_midletIsRemoved: %d, '%ls'\n", suiteId, midletClassName );
#endif
    elemPlace = findMidletIdChain( suiteId, midletClassName );
    if( *elemPlace != NULL && compareMidletIdChain(*elemPlace, suiteId, midletClassName) == 0 ){
        // remove elem from the chain
        MidletIdChain * elem = *elemPlace;
        *elemPlace = (*elemPlace)->next;
        destroyMidletIdChain( elem );
    }

    RELEASE_UTF16_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_j2me_content_AppProxy_isMidletRunning) {
    int res = 0;
    SuiteIdType suiteId;
    KNI_StartHandles(1);

    suiteId = KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_UTF16_STRING(2, midletClassName)

    MidletIdChain ** elemPlace = findMidletIdChain( suiteId, midletClassName );
#ifdef TRACE_MIDLETREG
    printf( "AppProxy_isMidletRunning: %d, '%ls'\n", suiteId, midletClassName );
#endif
    res = (*elemPlace != NULL && compareMidletIdChain(*elemPlace, suiteId, midletClassName) == 0);

    RELEASE_UTF16_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnBoolean( res );
}
