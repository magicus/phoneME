/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

#define CVM_JVM
#include "generated/jni/java_lang_System.h"
#include "javavm/export/jvm.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/localroots.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/preloader_impl.h"
#include "javavm/include/porting/clib.h"

#include "generated/offsets/java_lang_String.h"

/*
 * Debugging routine: System.print().
 */
JNIEXPORT void JNICALL
Java_java_lang_System_print(JNIEnv *env, jclass thisClass, jstring msg)
{
#   define BUFSIZE 128
    unsigned short ch;
    int i, njchar, off;
    char cbuf[BUFSIZE+5];
    int ncchar = 0;
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);

    CVMD_gcUnsafeExec(ee, {
	CVMArrayOfChar* arrayobj;
	CVMObject*      obj;
	CVMObject*      msgObj = CVMID_icellDirect(ee, msg);
	CVMD_fieldReadRef( msgObj, CVMoffsetOfjava_lang_String_value, obj );
	arrayobj = (CVMArrayOfChar*)obj;
	CVMD_fieldReadInt( msgObj, CVMoffsetOfjava_lang_String_offset, off );
	CVMD_fieldReadInt( msgObj, CVMoffsetOfjava_lang_String_count, njchar );
	for ( i=0; (i< njchar)&&(ncchar<BUFSIZE); i++ ){
	    CVMD_arrayReadChar(arrayobj, off+i, ch);
	    if ( ch <= '~' ){
		cbuf[ncchar++] = (char) ch;
	    } else {
		cbuf[ncchar++] = '\\';
		cbuf[ncchar++] = "0123456789abcdef"[(ch>>12)&0xf];
		cbuf[ncchar++] = "0123456789abcdef"[(ch>>8)&0xf];
		cbuf[ncchar++] = "0123456789abcdef"[(ch>>4)&0xf];
		cbuf[ncchar++] = "0123456789abcdef"[(ch>>0)&0xf];
	    }
	}
    });

    /* now we can write the things out */
    cbuf[ncchar] = 0;
    CVMconsolePrintf("%s", cbuf );
}
