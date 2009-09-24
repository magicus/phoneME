/*
 * @(#)mangle.c	1.16 06/10/10
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
 * Contains functions for creating mangled native method names.
 */

#ifdef CVM_CLASSLOADING

#include "javavm/include/defs.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/utils.h"
#include "javavm/include/signature.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/clib.h"

enum { 
    CVM_MangleUTF_Class, 
    CVM_MangleUTF_Field, 
    CVM_MangleUTF_FieldStub, 
    CVM_MangleUTF_Signature,
    CVM_MangleUTF_JNI
};

static int
CVMmangleUTFString(char *name, char *buffer, int buflen, int type) ;

static int
CVMmangleUTFString2(char *name, char *buffer, int buflen, 
		    int type, char endC) ;

static int
CVMmangleUnicodeChar(int ch, char *bufptr, char *bufend);


/*
 * CVMmangleMethodName: create a mangled method name for a method.
 * It is the caller's responsiblity to free the mangled name returned.
 *
 * Warning: the buffer overflow checking here is invalid although
 * should prevent overruns.
 */
char* 
CVMmangleMethodName(CVMExecEnv* ee, CVMMethodBlock* mb, 
		    CVMMangleType mangleType) 
{ 
    CVMClassTypeID  classID  = CVMcbClassName(CVMmbClassBlock(mb));
    CVMMethodTypeID methodID = CVMmbNameAndTypeID(mb);

    char* bufptr;
    char* bufend;
    char* components[3];
    char* mangledName = NULL;
    int   maxMangledNameLen;
    int   i;

    components[0] = CVMtypeidClassNameToAllocatedCString(classID);
    components[1] = CVMtypeidMethodNameToAllocatedCString(methodID);
    components[2] = CVMtypeidMethodTypeToAllocatedCString(methodID);

    /*
     * Calculate the maximum size of the mangled name.
     */
    maxMangledNameLen = 12; /* Java_ prefix, _stub postfix, etc. */
    for (i = 0; i < 3; i++) {
	CVMJavaChar ch;
        const char* p = components[i];

	/* Make sure we were able to allocate the component buffer */
	if (p == NULL) {
	    goto finish; /* allocation failed - out of memory */
	}

	while ((ch = CVMutfNextUnicodeChar(&p)) != '\0') {
	    if (ch <= 0x7f && isalnum(ch)) {
	        maxMangledNameLen++;
	    } else if (ch == '/') {
	        maxMangledNameLen++;
	    } else if (ch == '_' || ch == '[' || ch == ';') {
	        maxMangledNameLen += 2;
	    } else {
	        maxMangledNameLen += 6;
	    }
	}
    }

    mangledName = (char *)malloc(maxMangledNameLen);
    if (mangledName == NULL) {
	    goto finish; /* allocation failed - out of memory */
    }	
    bufptr = mangledName;
    bufend = bufptr + maxMangledNameLen;

    /* Copy over Java_ */
    if (mangleType == CVM_MangleMethodName_CNI_SHORT) {
	sprintf(bufptr, "CNI");
    } else {
	sprintf(bufptr, "Java_");
    }
    bufptr += strlen(bufptr);

    /* Copy the class name */
    bufptr += CVMmangleUTFString(components[0], bufptr, (int)(bufend - bufptr), 
				 CVM_MangleUTF_JNI);
    if (bufend - bufptr > 1) {
        *bufptr++ = '_';
    }

    /* Copy the method name */
    bufptr += CVMmangleUTFString(components[1], bufptr, (int)(bufend - bufptr), 
				 CVM_MangleUTF_JNI);

    /* JNI long name: includes a mangled signature. */
    if (mangleType == CVM_MangleMethodName_JNI_LONG) {
        if (bufend - bufptr > 2) {
	    *bufptr++ = '_';
	    *bufptr++ = '_';
        }
        bufptr += CVMmangleUTFString2(components[2] + 1,
				      bufptr, 
				      (int)(bufend - bufptr),
				      CVM_MangleUTF_JNI,
				      CVM_SIGNATURE_ENDFUNC);
    }

 finish:
    for (i = 0; i < 3; i++) {
	if (components[i] != NULL) {
	    free(components[i]);
	}
    }

    if (mangledName == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
    }
    return mangledName;
}
    
static int
CVMmangleUnicodeChar(int ch, char *bufptr, char *bufend)
{
    char temp[10];
    sprintf(temp, "_%.5x", ch); /* six characters always plus null */
    CVMformatString(bufptr, bufend - bufptr, "%s", temp);
    return (int)strlen(bufptr);
}

static int
CVMmangleUTFString2(char* name, char *buffer, int buflen, int type, char endC) 
{ 
    const char* ptr = name;
    char* bufptr = buffer;
    char* bufend = buffer + buflen - 1;	/* save space for NULL */
    CVMJavaChar ch;
    while (((ch = CVMutfNextUnicodeChar(&ptr)) != endC) 
	   && (bufend - bufptr > 0)) {
	if (ch <= 0x7f && isalnum(ch)) {
	    *bufptr++ = (char)ch;
	} else if ((ch == '/' || ch == '$') && type == CVM_MangleUTF_Class) { 
	    *bufptr++ = '_';
	} else if (ch == '_' && type == CVM_MangleUTF_FieldStub) { 
	    *bufptr++ = '_';
        } else if (type == CVM_MangleUTF_JNI) {
	    char* esc = 0;
	    if (ch == '_')
	        esc = "_1";
	    else if (ch == '/')
	        esc = "_";
	    else if (ch == ';')
	        esc = "_2";
	    else if (ch == '[')
	        esc = "_3";
	    if (esc) {
	        CVMformatString(bufptr, bufend - bufptr, esc);
	        bufptr += strlen(esc);
	    } else {
	        bufptr += CVMmangleUnicodeChar(ch, bufptr, bufend);
	    }
        } else if (type == CVM_MangleUTF_Signature) {
	    if (isprint(ch)) {
	        *bufptr++ = ch;
	    } else {
	        bufptr += CVMmangleUnicodeChar(ch, bufptr, bufend);
	    }
 	} else {
	    bufptr += CVMmangleUnicodeChar(ch, bufptr, bufend);
	}
    }
    *bufptr = '\0';
    return (int)(bufptr - buffer);
}

static int
CVMmangleUTFString(char *name, char *buffer, int buflen, int type) 
{ 
    return CVMmangleUTFString2(name, buffer, buflen, type, '\0');
}

#endif  /* CVM_CLASSLOADING */
