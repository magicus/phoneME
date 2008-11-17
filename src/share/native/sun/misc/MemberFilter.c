/*
 * @(#)MemberFilter.c	1.10 06/10/10
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
/*
 * @(#)MemberFilter.c	1.5 03/08/04
 *
 * Native portion of sun.misc.MemberFilter.
 * Used in support of API Hiding for MIDlet running.
 * Very CVM-specific implementation.
 */
#include "jni.h"
#include "jvm.h"
#include "javavm/include/classes.h"
#include "javavm/include/directmem.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/dualstack_impl.h"
#include "javavm/include/globals.h"

JNIEXPORT jboolean JNICALL
Java_sun_misc_MemberFilter_findROMFilterData(JNIEnv* env,
                                             jobject thisObject)
{
#ifdef CVM_PRELOAD_LIB
    /*
     * The CVMdualStackMemberFilter data is derived from a
     * fully ROMized bulid. The data contains typeids created
     * by the ROMizer. Therefore, it is only safe to use
     * CVMdualStackMemberFilter when CVM_PRELOAD_LIB is defined.
     * If CVM_PRELOAD_LIB is not define, we need to create
     * the member filter by reading in the configuration
     * file.
     */
    if (CVMdualStackMemberFilter.nElements != 0) {
        CVMglobals.dualStackMemberFilter = &CVMdualStackMemberFilter;
        return JNI_TRUE;
    }
#endif
    return JNI_FALSE;
}
					     

static CVMUint32*
parseMemberList(
    JNIEnv* env,
    jarray members, 
    int* memberCount,
    CVMUint32 (*lookupFn)(CVMExecEnv*, const CVMUtf8*, const CVMUtf8*))
{
    int nmembers = (members == NULL) ? 0 : (*env)->GetArrayLength(env, members);
    int i;
    CVMUint32* memberArray;
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    *memberCount = nmembers;
    if (nmembers == 0)
	return NULL;
    memberArray = (CVMUint32*)calloc(nmembers, sizeof(CVMUint32));
    for (i=0; i<nmembers; i++){
	jstring designator;
	jboolean didCopy;
	char* membername;
	char* membersig;
	/* first get the string at array[i] */
	designator = (*env)->GetObjectArrayElement(env, members, i);
	/* now get the characters from the string */
	membername = (char*)((*env)->GetStringUTFChars(env, designator,
						       &didCopy));
	CVMassert(didCopy); /* hope so because we're going to scribble in it */
	/* find : separator. Replace it with '\0' */
	membersig = strchr(membername, ':');
	CVMassert(membersig != NULL);
	*membersig++ = '\0';
	/* lookup the pair and save the resulting value */
	memberArray[i] = lookupFn(ee, membername, membersig);
	/* release the string characters and the string object */
	(*env)->ReleaseStringUTFChars(env, designator, membername);
	(*env)->DeleteLocalRef(env, designator);
    }
    return memberArray;
}

JNIEXPORT void JNICALL
Java_sun_misc_MemberFilter_addRestrictions(
    JNIEnv* env,
    jobject thisObject,
    jstring classname,
    jarray  fields,
    jarray  methods)
{
    int length;
    const char* classnameString;
    struct linkedClassRestriction* lcrp, *listroot, **nextp;
    jclass thisClass;
    jfieldID partialDataField;

    lcrp = (struct linkedClassRestriction*)calloc(
				sizeof(struct linkedClassRestriction), 1);

    /* get the typeid of the class */
    length = (*env)->GetStringUTFLength(env, classname);
    classnameString = (*env)->GetStringUTFChars(env, classname, NULL);
    lcrp->thisClass = CVMtypeidNewClassID(CVMjniEnv2ExecEnv(env),
					  classnameString, length);
    (*env)->ReleaseStringUTFChars(env, classname, classnameString);

    /* allocate the arrays of member typeids */
    lcrp->fields = parseMemberList(env, fields, &(lcrp->nFields),
				   CVMtypeidNewFieldIDFromNameAndSig);
    lcrp->methods = parseMemberList(env, methods, &(lcrp->nMethods),
				    CVMtypeidNewMethodIDFromNameAndSig);

    /* insertion sort in linked list based on thisClass value */
    thisClass = (*env)->GetObjectClass(env, thisObject);
    partialDataField = (*env)->GetFieldID(env, thisClass, "partialData", "I");

    listroot = (struct linkedClassRestriction*)
		((*env)->GetIntField(env, thisObject, partialDataField));

    nextp = &listroot;
    while(CVM_TRUE){
	if (*nextp == NULL || (*nextp)->thisClass > lcrp->thisClass){
	    lcrp->next = *nextp;
	    *nextp = lcrp;
	    break;
	}
	nextp = &((*nextp)->next);
    }
    (*env)->SetIntField(env, thisObject, partialDataField, (jint)listroot);
}

JNIEXPORT void JNICALL
Java_sun_misc_MemberFilter_doneAddingRestrictions(
    JNIEnv* env,
    jobject thisObject)
{
    /*
     * consolidate linked list into an array for faster access.
     * dispose of linked list form.
     */
    struct linkedClassRestriction* lcrp, *listroot;
    struct CVMClassRestrictions* crp;
    struct CVMClassRestrictionElement *creep;
    jclass thisClass;
    jfieldID partialDataField;
    int    nentries;
    int    i;

    thisClass = (*env)->GetObjectClass(env, thisObject);
    partialDataField = (*env)->GetFieldID(env, thisClass, "partialData", "I");

    listroot = (struct linkedClassRestriction*)
		    ((*env)->GetIntField(env, thisObject, partialDataField));
    /* count */
    for (nentries=0, lcrp=listroot; lcrp!=NULL; lcrp = lcrp->next)
	nentries += 1;
    /* allocate */
    crp = (struct CVMClassRestrictions*)calloc(1,
                sizeof(struct CVMClassRestrictions));
    creep = (struct CVMClassRestrictionElement*)calloc(1,
                nentries*sizeof(struct CVMClassRestrictionElement ));
    /* copy */
    crp->nElements = nentries;
    crp->restriction = creep;
    for (i=0, lcrp=listroot; i<nentries; i++, lcrp = lcrp->next){
        creep->thisClass = lcrp->thisClass;
	creep->nMethods = lcrp->nMethods;
	creep->nFields = lcrp->nFields;
	creep->methods = lcrp->methods;
	creep->fields  = lcrp->fields;
        creep++;
    }
    /* set partialData field to null */
    (*env)->SetIntField(env, thisObject, partialDataField, 0);

    CVMglobals.dualStackMemberFilter = crp;

    /* delete linked list elements */
    lcrp = listroot;
    while (lcrp != NULL){
	listroot = lcrp->next;
	free(lcrp);
	lcrp = listroot;
    }
}

/*
 * This is the array-element representation
 */

static CVMClassRestrictionElement*
lookupClass(const CVMClassRestrictions* crp, CVMClassTypeID cid)
{
    int i, n;
    CVMClassRestrictionElement* ep;
    n = crp->nElements;
    ep = &(crp->restriction[0]);
    /* DEBUG{
	char * className = CVMtypeidClassNameToAllocatedCString(cid);
	CVMconsolePrintf(">>>> Looking for %s (0x%x) through %d classes", className, cid, n);
	free(className);
    } 
    */
    for (i=0; i<n; i++, ep++){
	if (ep->thisClass == cid){
	    /* CVMconsolePrintf(": found at %d\n", i); */
	    return ep;
	}
    }
    /* not found */
    /* DEBUG: CVMconsolePrintf(": not found\n", i); */
    return NULL;
}

static CVMBool
lookupMember(CVMUint32 mid, CVMUint32* memberArray, int nMembers){
    int i;
    CVMUint32* memberp = memberArray;
    for (i=0; i<nMembers; i++, memberp++){
	if (mid == *memberp)
	    return CVM_TRUE;
    }
    return CVM_FALSE;
}

static void
saveName(
    JNIEnv* env,
    jobject thisObject,
    jclass thisClass,
    CVMClassTypeID classID, 
    CVMUint32 memberID,
    CVMBool isMethodType)
{
    char* className;
    char* memberName;
    char* type;
    jfieldID    fieldID;
    jstring	stringObj;

    className = CVMtypeidClassNameToAllocatedCString(classID);
    if (isMethodType){
	memberName = CVMtypeidMethodNameToAllocatedCString(memberID);
	type = CVMtypeidMethodTypeToAllocatedCString(memberID);
    }else{
	memberName = CVMtypeidFieldNameToAllocatedCString(memberID);
	type = CVMtypeidFieldTypeToAllocatedCString(memberID);
    }
    stringObj = (*env)->NewStringUTF(env, className);
    fieldID = (*env)->GetFieldID(env, thisClass, "badClass",
				 "Ljava/lang/String;");
    (*env)->SetObjectField(env, thisObject, fieldID, stringObj);
    (*env)->DeleteLocalRef(env, stringObj);

    stringObj = (*env)->NewStringUTF(env, memberName);
    fieldID = (*env)->GetFieldID(env, thisClass, "badMember",
				 "Ljava/lang/String;");
    (*env)->SetObjectField(env, thisObject, fieldID, stringObj);
    (*env)->DeleteLocalRef(env, stringObj);

    stringObj = (*env)->NewStringUTF(env, type);
    fieldID = (*env)->GetFieldID(env, thisClass, "badSig",
				 "Ljava/lang/String;");
    (*env)->SetObjectField(env, thisObject, fieldID, stringObj);
    (*env)->DeleteLocalRef(env, stringObj);

    free(className);
    free(memberName);
    free(type);
}

JNIEXPORT jboolean JNICALL
Java_sun_misc_MemberFilter_checkMemberAccessValidity0(
    JNIEnv* env,
    jobject thisObject,
    jclass  newclass)
{
    const CVMClassRestrictions* crp;
    CVMClassRestrictionElement* creep;
    jclass thisClass;

    CVMClassBlock* cbp;
    jclass classClass;
    jfieldID classBlockPointer;

    CVMInt32		  cpCount;
    CVMInt32		  i, classIndex, typeIndex;
    CVMClassTypeID	  classID;
    CVMMethodTypeID	  mID;
    CVMFieldTypeID	  fID;
    CVMConstantPool*	  cp;

    /* get pointer to the classes CVMClassBlock */
    classClass = (*env)->GetObjectClass(env, newclass); // java.lang.Class
    classBlockPointer = (*env)->GetFieldID(env, classClass,
				 "classBlockPointer", "I");
    cbp = (CVMClassBlock*)((*env)->GetIntField(env, newclass,
				 classBlockPointer));
    /* get pointer to our list of restrictions */
    thisClass = (*env)->GetObjectClass(env, thisObject);
    crp = CVMglobals.dualStackMemberFilter;

    /* we are now on the inside. Look at the class's constant pool.
     * return CVM_FALSE if we see anything we don't like.
     */
    cpCount = CVMcbConstantPoolCount(cbp);
    cp = CVMcbConstantPool(cbp);
    for (i=1; i<cpCount; i++){
	switch(CVMcpEntryType(cp,i)){
	case CVM_CONSTANT_Fieldref:
	case CVM_CONSTANT_Methodref:
	case CVM_CONSTANT_InterfaceMethodref:
	    CVMassert(!CVMcpIsResolved(cp,i));
	    classIndex = CVMcpGetMemberRefClassIdx(cp, i);
	    typeIndex  = CVMcpGetMemberRefTypeIDIdx(cp, i);
	    CVMassert(!CVMcpIsResolved(cp,classIndex));
	    classID = CVMcpGetClassTypeID(cp, classIndex);
	    creep = lookupClass(crp, classID);
	    if (creep==NULL)
		continue; /* we don't care about use of class "classID" */
	    CVMassert(!CVMcpIsResolved(cp,typeIndex));
	    switch(CVMcpEntryType(cp,i)){
	    case CVM_CONSTANT_Fieldref:
		fID = CVMcpGetFieldTypeID(cp, typeIndex);
		if (!lookupMember(
                        fID, creep->fields, creep->nFields)){
		    /* not on the permitted list */
		    saveName(env, thisObject, thisClass, classID, fID,
			     CVM_FALSE);
                    CVMcpSetIllegalEntryType(cp, i, Fieldref);
		}
		break;
	    default:
		mID = CVMcpGetMethodTypeID(cp, typeIndex);
		if (!lookupMember(
                        mID, creep->methods, creep->nMethods)){
		    /* not on the permitted list */
		    saveName(env, thisObject, thisClass, classID, mID,
			     CVM_TRUE);
	            switch(CVMcpEntryType(cp,i)) {
		    case CVM_CONSTANT_Methodref:
                        CVMcpSetIllegalEntryType(cp, i, Methodref);
			break;
		    case CVM_CONSTANT_InterfaceMethodref:
                        CVMcpSetIllegalEntryType(cp, i, InterfaceMethodref);
			break;
		    }
		}
		break;
	    }
	}
    }
    /*
     * If we found nothing to object to, then we succeed.
     */
    return CVM_TRUE;
}

JNIEXPORT void JNICALL
Java_sun_misc_MemberFilter_finalize0(
    JNIEnv* env,
    jobject thisObject)
{
    struct linkedClassRestriction* lcrp, *listroot;
    jclass thisClass;
    jfieldID partialDataField;

    thisClass = (*env)->GetObjectClass(env, thisObject);
    partialDataField = (*env)->GetFieldID(env, thisClass, "partialData", "I");

    listroot = (struct linkedClassRestriction*)((*env)->GetIntField(env, thisObject, partialDataField));
    if (listroot != NULL){
	/* This MemberFilter isn't fully formed. Delete
	 * the linked-list form.
	 */
	lcrp = listroot;
	while (lcrp != NULL){
	    listroot = lcrp->next;
	    if (lcrp->methods != NULL)
		free(lcrp->methods);
	    if (lcrp->fields != NULL)
		free(lcrp->fields);
	    free(lcrp);
	    lcrp = listroot;
	}
    }
    /* set 'partialData' field to 0 */
    (*env)->SetIntField(env, thisObject, partialDataField, 0);
}

/* Check if the super MB exists */
CVMBool
CVMdualStackFindSuperMB(CVMExecEnv* ee,
                        CVMClassBlock* currentCB,
                        CVMClassBlock* superCB,
                        CVMMethodBlock* superMB)
{
    CVMBool needCheckFilter;

    /* TODO: We should use in the CB to indicate
     *       if a class have a filter applied,
     *       instead of checking for classloader.
     */
    CVMD_gcUnsafeExec(ee, {
        needCheckFilter = CVMclassloaderIsMIDPClassLoader(
            ee, CVMcbClassLoader(currentCB), CVM_FALSE);
    });
    if (needCheckFilter) {
        CVMClassRestrictionElement* cre;
        const CVMClassRestrictions* crp;

        crp = CVMglobals.dualStackMemberFilter;
        cre = lookupClass(crp, CVMcbClassName(superCB));
        if (cre != NULL) {
            /* Check if the method exists. */
	    return lookupMember(CVMmbNameAndTypeID(superMB),
                                cre->methods, cre->nMethods);
        }
    }
    return CVM_TRUE;
}
