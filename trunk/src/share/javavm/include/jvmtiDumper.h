/*
 * @(#)jvmtiDumper.h	1.0 07/01/17
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


#ifndef _INCLUDED_JVMTIDUMPER_H
#define _INCLUDED_JVMTIDUMPER_H

void CVMjvmtiTagRehash();
CVMBool CVMjvmtiDumpObject(CVMObject *obj, CVMJvmtiDumpContext *dc);
jvmtiError CVMjvmtiTagGetTag(jobject object, jlong *tagPtr, CVMBool);
jvmtiError CVMjvmtiTagSetTag(jobject object, jlong tag, CVMBool);
CVMBool CVMjvmtiScanRoots(CVMExecEnv *ee, CVMJvmtiDumpContext *dc);
void CVMjvmtiPostCallbackUpdateTag(CVMObject *obj, jlong tag);
CVMJvmtiTagNode *CVMjvmtiTagGetNode(CVMObject *obj);

CVMBool CVMjvmtiIterateDoObject(CVMObject* obj, CVMClassBlock* cb, 
				CVMUint32  objSize, void* data);

int CVMjvmtiGetObjectsWithTag(JNIEnv *env, const jlong *tags, jint tagCount,
			      jobject **objPtr, jlong **tagPtr);

#endif /* _INCLUDED_JVMTIDUMPER_H */
