/*
 * @(#)verify.h	1.12 06/10/10
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

#include "javavm/export/jni.h"
#include "javavm/export/jvm.h"


/*
 * Support for a VM-independent class format checker.
 */ 
typedef struct {
    CVMUint32 code;    /* byte code */
    CVMUint32 excs;    /* exceptions */
    CVMUint32 etab;    /* catch table */
    CVMUint32 lnum;    /* line number */
    CVMUint32 lvar;    /* local vars */
} CVMmethod_size_info;

typedef struct {
    CVMUint32 constants;       /* constant pool */
    CVMUint32 reducedConstants;/* constant pool without trailing utf8s */
    CVMUint32 fields;
    CVMUint32 methods;
    CVMUint32 javamethods;     /* # of methods implemented in java */
    CVMUint32 interfaces;
    CVMUint32 staticFields;    /* number of words of static fields */
    CVMUint32 staticRefFields; /* number of words of static ref fields */
    CVMUint32 innerclasses;    /* # of records in InnerClasses attr */

    method_size_info clinit;   /* memory used in clinit */
    method_size_info main;     /* used everywhere else */
} CVMclass_size_info;

/* 
 * Byte code verifier.
 *
 * Returns JNI_FALSE if verification fails. A detailed error message
 * will be places in msg_buf, whose length is specified by buf_len.
 */
jint
VerifyClass(CVMExecEnv *ee, 
	    CVMClassBlock *cb,
	    char * msg_buf, 
	    jint buf_len,
            CVMBool isRedefine);

/* 
 * This performs class format checks and fills in size 
 * information for the class file and returns:
 *  
 *   0: good
 *  -1: out of memory
 *  -2: bad format
 *  -3: unsupported version
 *  -4: bad class name
 */
jint
CVMverifyClassFormat(const char *class_name,
		     const unsigned char *data,
		     unsigned int data_size,
		     class_size_info *size,
		     char *message_buffer,
		     jint buffer_length,
		     jboolean measure_only,
#ifdef CVM_DUAL_STACK
                     jboolean isCLDCClass,
#endif
		     jboolean check_relaxed); 
