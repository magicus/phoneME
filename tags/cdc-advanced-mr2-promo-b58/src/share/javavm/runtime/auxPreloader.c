/*
 * @(#)auxPreloader.c	1.5 06/10/10
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
 *
 */

/*
 * Classloader for dual-stack implementation classes
 * that have been processed by JCC into the Aux part
 * of the classlist.
 *
 * These are invisible to the null classloader, but
 * visible to this classloader. We preload them for
 * performance.
 */

#include "jni.h"
#include "jvm.h"
#include "javavm/include/classes.h"
#include "javavm/include/typeid.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/localroots.h"
#include "javavm/include/preloader.h"
#include "javavm/include/preloader_impl.h"

/*
 * Look up class by name.
 */
const CVMClassBlock* 
CVMauxPreloaderLookup(const char* className, int nameLength)
{
    int i;
    int lowest;
    int highbound;
    CVMClassTypeID typeID = 
	CVMtypeidLookupClassID(NULL, className, nameLength );
    if (typeID == CVM_TYPEID_ERROR) {
	return NULL; /* not in type table, so don't bother looking. */
    }

    /* No CVMFieldTypeIDs allowed. Upper bits must never be set. */
    CVMassert(CVMtypeidGetType(typeID) == typeID);

    /* No primitive types in Aux tables */
    CVMassert(!CVMtypeidIsPrimitive(typeID));

    /* May be OK, but probably not implemented */
    CVMassert(!CVMtypeidIsBigArray(typeID));

    if ( !CVMtypeidIsArray(typeID)) {
	/* Scalar class types are in the first part */
	lowest = CVM_nROMClasses;
	highbound= lowest + CVM_AuxfirstROMVectorClass;
    } else if ( CVMtypeidGetArrayDepth(typeID) == 1 ){
	/* Vectors */
	lowest= CVM_nROMClasses + CVM_AuxfirstROMVectorClass;
	highbound= CVM_nROMClasses + CVM_AuxlastROMVectorClass;
    } else {
	/* other arrays */
	lowest = CVM_nROMClasses + CVM_AuxlastROMVectorClass;
	highbound= CVM_nTotalROMClasses;
    }

    while ( lowest < highbound ){
	const CVMClassBlock * cb;
	int candidateID;
	i = lowest + (highbound-lowest)/2;
	cb = CVM_ROMClassblocks[i];
	candidateID = CVMcbClassName(cb);
	if ( candidateID == typeID) return (CVMClassBlock*)cb;
	if ( candidateID < typeID)
	    lowest = i+1;
	else
	    highbound = i;
    }

    /*
     * Fell out of while loop.
     * not in table.
     */
    return NULL;
}

JNIEXPORT jclass JNICALL
/*Java_sun_misc_DualStackClassloader_classByName(*/
CVMauxPreloaderClassByName(
    JNIEnv* env,
    jstring classname)
{
    int length;
    const char* classnameString;
    const CVMClassBlock* cb;

    length = (*env)->GetStringUTFLength(env, classname);
    classnameString = (*env)->GetStringUTFChars(env, classname, NULL);
    cb = CVMauxPreloaderLookup(classnameString, length);
    (*env)->ReleaseStringUTFChars(env, classname, classnameString);

    if (cb == NULL){
	return (jclass)&CVMID_nullICell;
    }
    return (jclass)CVMcbJavaInstance(cb);
}

#ifdef CVM_DEBUG
/*
 * we hate counting strlen when calling from the debugger.
 */
const CVMClassBlock* 
CVMauxPreloaderLookupName(const char* className){
    return CVMauxPreloaderLookup(className, strlen(className));
}
#endif
