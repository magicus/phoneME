/*
 * @(#)javaAssertions.c	1.11 06/10/10
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
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/classes.h"
#include "javavm/include/utils.h"
#include "javavm/include/clib.h"
#include "javavm/include/preloader.h"
#include "javavm/include/globals.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/javaAssertions.h"

#include "generated/offsets/java_lang_AssertionStatusDirectives.h"

#define CVMtraceJavaAssertions CVMtraceMisc

struct CVMJavaAssertionsOptionList {
  const char*   name;
  CVMBool       enabled;
  CVMJavaAssertionsOptionList*	next;
};

static CVMBool
CVMJavaAssertions_fillJavaArrays(CVMExecEnv* ee,
				 const CVMJavaAssertionsOptionList* p, int len,
				 CVMArrayOfRefICell* names,
				 CVMArrayOfBooleanICell* status);

static CVMJavaAssertionsOptionList*
CVMJavaAssertions_match_class(const char* classname);

static CVMJavaAssertionsOptionList*
CVMJavaAssertions_match_package(const char* classname);

static CVMJavaAssertionsOptionList*
CVMJavaAssertionsOptionList_new(const char* name, CVMBool enable,
				CVMJavaAssertionsOptionList* next)
{
    CVMJavaAssertionsOptionList* list = (CVMJavaAssertionsOptionList *)
	malloc(sizeof(CVMJavaAssertionsOptionList));
    if (list != NULL) {
	list->name = name;
	list->enabled = enable;
	list->next = next;
    }
    return list;
}

static int
CVMJavaAssertionsOptionList_count(CVMJavaAssertionsOptionList* p)
{
    int rc = 0;
    while (p != NULL) {
	p = p->next;
	rc++;
    }
    return rc;
}

/* Add a command-line option.  A name ending in "..." applies to a package and
   any subpackages; other names apply to a single class. */

CVMBool
CVMJavaAssertions_addOption(
    const char* name, CVMBool enable,
    CVMJavaAssertionsOptionList** javaAssertionsClasses,
    CVMJavaAssertionsOptionList** javaAssertionsPackages)
{
    int len = strlen(name);
    char* name_copy;
    CVMJavaAssertionsOptionList** head;
    int i;

    /* make a copy of name */
    name_copy = (char *)malloc(len + 1);
    if (name_copy == NULL) {
	return CVM_FALSE;
    }
    strcpy(name_copy, name);

    /* Figure out which list the new item should go on.  Names that end
       in "..." go on the package tree list. */
    if (len >= 3 && strcmp(name_copy + len - 3, "...") == 0) {
	/* Delete the "...". */
	len -= 3;
	name_copy[len] = '\0';
	head = javaAssertionsPackages;
    } else {
	head = javaAssertionsClasses;
    }

    /* Convert class/package names to internal format. */
    for (i = 0; i < len; ++i) {
	if (name_copy[i] == '.') {
	    name_copy[i] = '/';
	}
    }

#if 0
    CVMconsolePrintf("JavaAssertions: adding %s %s=%d\n",
		     head == javaAssertionsClasses ?
		     "class" : "package",
		     name_copy[0] != '\0' ? name_copy : "'default'",
		     enable);
#endif

    /* Prepend a new item to the list.  Items added later take
       precedence, so prepending allows us to stop searching the list
       after the first match. */
    *head = CVMJavaAssertionsOptionList_new(name_copy, enable, *head);
    return *head != NULL;
}

/* free assertion command line options */
void
CVMJavaAssertions_freeOptions()
{
    CVMJavaAssertionsOptionList* p;
    p = CVMglobals.javaAssertionsPackages;
    while (p != NULL) {
	CVMJavaAssertionsOptionList* prev = p;
	p = p->next;
	free(prev);
    }
    CVMglobals.javaAssertionsPackages = NULL;
    p = CVMglobals.javaAssertionsClasses;
    while (p != NULL) {
	CVMJavaAssertionsOptionList* prev = p;
	p = p->next;
	free(prev);
    }
    CVMglobals.javaAssertionsClasses = NULL;
}


/* Create an instance of java.lang.AssertionStatusDirectives and fill in the
   fields based on the command-line assertion options. */
jobject
CVMJavaAssertions_createAssertionStatusDirectives(CVMExecEnv* ee)
{
    int numClasses;
    int numPackages;
    CVMArrayOfRefICell* classNamesICell =
	(CVMArrayOfRefICell*)CVMjniCreateLocalRef(ee);
    CVMArrayOfRefICell* pkgNamesICell =
	(CVMArrayOfRefICell*)CVMjniCreateLocalRef(ee);
    CVMArrayOfBooleanICell* classEnabledICell =
	(CVMArrayOfBooleanICell*)CVMjniCreateLocalRef(ee);
    CVMArrayOfBooleanICell* pkgEnabledICell =
	(CVMArrayOfBooleanICell*)CVMjniCreateLocalRef(ee);
    jobject asdICell = CVMjniCreateLocalRef(ee);
    CVMClassBlock* java_lang_String_arrayCb = 
	CVMclassGetArrayOf(ee, CVMsystemClass(java_lang_String));
    
    /* We know we have at least 16 local refs, so this shouldn't fail */
    CVMassert(classNamesICell != NULL && classEnabledICell != NULL &&
	      pkgNamesICell != NULL && pkgEnabledICell != NULL
	      && asdICell != NULL);

    /* allocate the AssertionStatusDirectives instance */
    CVMID_allocNewInstance(ee,
			   CVMsystemClass(java_lang_AssertionStatusDirectives),
			   asdICell);

    /* allocate AssertionStatusDirectives.packages array*/
    numPackages = 
	CVMJavaAssertionsOptionList_count(CVMglobals.javaAssertionsPackages);
    CVMID_allocNewArray(
        ee, CVM_T_CLASS,
	java_lang_String_arrayCb,
	numPackages, (CVMObjectICell*)pkgNamesICell);

    /* allocate AssertionStatusDirectives.packagesEnabled array */
    CVMID_allocNewArray(
        ee, CVM_T_BOOLEAN, 
	(CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_BOOLEAN],
	numPackages, (CVMObjectICell*)pkgEnabledICell);

    /* allocate AssertionStatusDirectives.classes array*/
    numClasses = 
	CVMJavaAssertionsOptionList_count(CVMglobals.javaAssertionsClasses);
    CVMID_allocNewArray(
        ee, CVM_T_CLASS,
	java_lang_String_arrayCb,
	numClasses, (CVMObjectICell*)classNamesICell);

    /* allocate AssertionStatusDirectives.classEnabled array */
    CVMID_allocNewArray(
        ee, CVM_T_BOOLEAN, 
	(CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_BOOLEAN],
	numClasses, (CVMObjectICell*)classEnabledICell);

    /* make sure none of the allocations failed */
    if (CVMID_icellIsNull(classNamesICell) ||
	CVMID_icellIsNull(classEnabledICell) ||
	CVMID_icellIsNull(pkgNamesICell) ||
	CVMID_icellIsNull(pkgEnabledICell) ||
	CVMID_icellIsNull(asdICell) ) {
	goto fail;
    }

    /* fill the arrays */
    if (!CVMJavaAssertions_fillJavaArrays(ee,
					  CVMglobals.javaAssertionsPackages,
					  numPackages,
					  pkgNamesICell, pkgEnabledICell) ||
	!CVMJavaAssertions_fillJavaArrays(ee,
					  CVMglobals.javaAssertionsClasses,
					  numClasses,
					  classNamesICell, classEnabledICell))
    {
	goto fail;
    }

    /* set the AssertionStatusDirectives fields */
    CVMID_fieldWriteRef(ee, asdICell, 
	CVMoffsetOfjava_lang_AssertionStatusDirectives_packages,
	(CVMObjectICell*)pkgNamesICell);
    CVMID_fieldWriteRef(ee, asdICell, 
	CVMoffsetOfjava_lang_AssertionStatusDirectives_packageEnabled,
	(CVMObjectICell*)pkgEnabledICell);
    CVMID_fieldWriteRef(ee, asdICell, 
	CVMoffsetOfjava_lang_AssertionStatusDirectives_classes,
	(CVMObjectICell*)classNamesICell);
    CVMID_fieldWriteRef(ee, asdICell, 
	CVMoffsetOfjava_lang_AssertionStatusDirectives_classEnabled,
	(CVMObjectICell*)classEnabledICell);
    CVMID_fieldWriteInt(ee, asdICell, 
	CVMoffsetOfjava_lang_AssertionStatusDirectives_deflt,
	CVMglobals.javaAssertionsUserDefault);

  return asdICell;

 fail:
  CVMthrowOutOfMemoryError(ee, NULL);
  return NULL;
}

static CVMBool 
CVMJavaAssertions_fillJavaArrays(CVMExecEnv* ee,
				 const CVMJavaAssertionsOptionList* p,
				 int len,
				 CVMArrayOfRefICell* namesICell,
				 CVMArrayOfBooleanICell* enabledICell)
{
    int index;
    CVMStringICell* stringICell = CVMjniCreateLocalRef(ee);

    /* Fill in the parallel names and enabled (boolean) arrays.  Start
     * at the end of the array and work backwards, so the order of items
     * in the arrays matches the order on the command line (the list is in
     * reverse order, since it was created by prepending successive items
     * from the command line).
     */
    for (index = len - 1; p != NULL; p = p->next, --index)
    {
	const char* name = p->name;
	CVMBool enabled = p->enabled;
	int namelen = strlen(name);
        char* name_copy = strdup(name);
        if (name_copy == NULL) {
            return CVM_FALSE;
        }
	CVMassert(index >= 0);
	/* Convert class/package names from internal format. */
        while (--namelen >= 0) {
            if (name_copy[namelen] == '.') {
                name_copy[namelen] = '/';
            }
        }
	CVMnewStringUTF(ee, stringICell, name_copy);
	CVMID_arrayWriteRef(ee, namesICell, index, stringICell);
	CVMID_arrayWriteBoolean(ee, enabledICell, index, enabled);
	free(name_copy);
    }
    CVMassert(index == -1);
    return CVM_TRUE;
}

static CVMJavaAssertionsOptionList*
CVMJavaAssertions_match_class(const char* classname)
{
    CVMJavaAssertionsOptionList* p;
    for (p = CVMglobals.javaAssertionsClasses; p != NULL; p = p->next) {
	if (strcmp(p->name, classname) == 0) {
	    return p;
	}
    }
    return NULL;
}

static CVMJavaAssertionsOptionList*
CVMJavaAssertions_match_package(const char* classname)
{
    size_t len = strlen(classname);

    /* Search the package list for any items that apply to classname. Each
       sub-package in classname is checked, from most-specific to least,
       until one is found. */
    if (CVMglobals.javaAssertionsPackages == NULL) {
	return NULL;
    }

    /* Find the length of the "most-specific" package in classname.
       If classname does not include a package, length will be 0 which
       will match items for the default package (from options "-ea:..."
       or "-da:..."). */
    while (len > 0 && classname[len] != '/') {
	--len;
    }
    
    do {
	CVMJavaAssertionsOptionList* p ;
	CVMassert(len == 0 || classname[len] == '/');
	for (p = CVMglobals.javaAssertionsPackages; p != NULL; p = p->next) {
	    if (strncmp(p->name, classname, len) == 0 &&
		p->name[len] == '\0')
	    {
		return p;
	    }
	}
	
	/* Find the length of the next package, taking care to avoid
	   decrementing past 0 (len is unsigned). */
	while (len > 0 && classname[--len] != '/') /* empty */;
    } while (len > 0);
    
  return NULL;
}

#ifdef CVM_TRACE
#define CVMJavaAssertions_trace(name, typefound, namefound, enabled)	      \
{									      \
    CVMtraceJavaAssertions(("JavaAssertions:  search for %s found %s %s=%d\n",\
			    name, typefound,				      \
			    namefound[0] != '\0' ? namefound : "'default'",   \
			    enabled));					      \
}
#else
#define CVMJavaAssertions_trace(name, typefound, namefound, enabled)
#endif

/* Return true if command-line options have enabled assertions for the named
   class.  Should be called only after all command-line options have been
   processed.  Note:  this only consults command-line options and does not
   account for any dynamic changes to assertion status.
 */
CVMBool
CVMJavaAssertions_enabled(const char* classname, CVMBool systemClass)
{
    CVMJavaAssertionsOptionList* p;
    CVMassert(classname != NULL);

    /* This will be slow if the number of assertion options on the command
       line is large--it traverses two lists, one of them multiple times.
       Could use a  single n-ary tree instead of lists if someone ever notices.
    */

    /* First check options that apply to classes. If we find a match
       we're done. */
    p = CVMJavaAssertions_match_class(classname);
    if (p != NULL) {
	CVMJavaAssertions_trace(classname, "class", p->name, p->enabled);
	return p->enabled;
    }

    /* Now check packages, from most specific to least. */
    p = CVMJavaAssertions_match_package(classname);
    if (p != NULL) {
	CVMJavaAssertions_trace(classname, "package", p->name, p->enabled);
	return p->enabled;
    }

    /* No match.  Return the default status. */
    {
	CVMBool result = systemClass ?
	    CVMglobals.javaAssertionsSysDefault :
	    CVMglobals.javaAssertionsUserDefault;
	CVMJavaAssertions_trace(classname, systemClass ? "system" : "user",
				"default", result);
	return result;
    }
}
