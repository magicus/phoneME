/*
 * @(#)CharacterDataLatin1.c	1.6 06/10/10
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
#include "javavm/include/classes.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/localroots.h"
#include "javavm/export/jvm.h"
#include "generated/jni/java_lang_Character.h"
#include "javavm/include/preloader_impl.h"
#include "native/common/jni_util.h"

/* 
 * The static final arrays of ints that make up the A array of
 * java.lang.CharacterDataLatin1
 * 
 * This C file saves us the extra static initializer space for that class
 */

static const struct { 
    CVMObjectHeader hdr;
    CVMJavaInt	length;
    CVMJavaInt intData[256]; 
} CVM_CharacterLatinA_data = {
    CVM_ROM_OBJECT_HDR_INIT0(manufacturedArrayOfInt_Classblock,0),
        256, {
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x5800400F,
            (CVMJavaInt)0x5000400F,
            (CVMJavaInt)0x5800400F,
            (CVMJavaInt)0x6000400F,
            (CVMJavaInt)0x5000400F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x5000400F,
            (CVMJavaInt)0x5000400F,
            (CVMJavaInt)0x5000400F,
            (CVMJavaInt)0x5800400F,
            (CVMJavaInt)0x6000400C,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)0x28000018,
            (CVMJavaInt)0x2800601A,
            (CVMJavaInt)0x28000018,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)-0x17FFFFEB,
            (CVMJavaInt)-0x17FFFFEA,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)0x28000019,
            (CVMJavaInt)0x38000018,
            (CVMJavaInt)0x28000014,
            (CVMJavaInt)0x38000018,
            (CVMJavaInt)0x20000018,
            (CVMJavaInt)0x18003609,
            (CVMJavaInt)0x18003609,
            (CVMJavaInt)0x18003609,
            (CVMJavaInt)0x18003609,
            (CVMJavaInt)0x18003609,
            (CVMJavaInt)0x18003609,
            (CVMJavaInt)0x18003609,
            (CVMJavaInt)0x18003609,
            (CVMJavaInt)0x18003609,
            (CVMJavaInt)0x18003609,
            (CVMJavaInt)0x38000018,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)-0x17FFFFE7,
            (CVMJavaInt)0x68000019,
            (CVMJavaInt)-0x17FFFFE7,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)0x00827FE1,
            (CVMJavaInt)-0x17FFFFEB,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)-0x17FFFFEA,
            (CVMJavaInt)0x6800001B,
            (CVMJavaInt)0x68005017,
            (CVMJavaInt)0x6800001B,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)0x00817FE2,
            (CVMJavaInt)-0x17FFFFEB,
            (CVMJavaInt)0x68000019,
            (CVMJavaInt)-0x17FFFFEA,
            (CVMJavaInt)0x68000019,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x5000100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x4800100F,
            (CVMJavaInt)0x3800000C,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)0x2800601A,
            (CVMJavaInt)0x2800601A,
            (CVMJavaInt)0x2800601A,
            (CVMJavaInt)0x2800601A,
            (CVMJavaInt)0x6800001C,
            (CVMJavaInt)0x6800001C,
            (CVMJavaInt)0x6800001B,
            (CVMJavaInt)0x6800001C,
            (CVMJavaInt)0x00007002,
            (CVMJavaInt)-0x17FFFFE3,
            (CVMJavaInt)0x68000019,
            (CVMJavaInt)0x68000014,
            (CVMJavaInt)0x6800001C,
            (CVMJavaInt)0x6800001B,
            (CVMJavaInt)0x2800001C,
            (CVMJavaInt)0x28000019,
            (CVMJavaInt)0x1800060B,
            (CVMJavaInt)0x1800060B,
            (CVMJavaInt)0x6800001B,
            (CVMJavaInt)0x07FD7002,
            (CVMJavaInt)0x6800001C,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)0x6800001B,
            (CVMJavaInt)0x1800050B,
            (CVMJavaInt)0x00007002,
            (CVMJavaInt)-0x17FFFFE2,
            (CVMJavaInt)0x6800080B,
            (CVMJavaInt)0x6800080B,
            (CVMJavaInt)0x6800080B,
            (CVMJavaInt)0x68000018,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x68000019,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x00827001,
            (CVMJavaInt)0x07FD7002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x68000019,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x00817002,
            (CVMJavaInt)0x061D7002
        }
};

/*
 * Class:	java/lang/CharacterDataLatin1
 * Method:	setArrays
 * Signature:	()V
 *
 * This function populates the private int A[] array of Java with the Romized values.
 */
JNIEXPORT void JNICALL
Java_java_lang_CharacterDataLatin1_setArrays (JNIEnv* env, jclass cls) {
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env); 
    CVMID_localrootBegin(ee) {
        CVMID_localrootDeclare(CVMObjectICell, aICell);
        
        jfieldID aId;
        
        CVMD_gcUnsafeExec(ee, {
	    /* NOTE: the reason for appending ".hdr" is to work around
	       a suspect GCC strict aliasing warning. */
            CVMID_icellSetDirect(ee, aICell, (CVMObject*)&CVM_CharacterLatinA_data.hdr);
        });
        
        aId = (*env)->GetStaticFieldID(env, cls, "A", "[I");
        
        /* the argument 4 of the function below is jobject, hence we pass 
         * CVMObjectICell
         */
        (*env)->SetStaticObjectField(env, cls, aId, aICell);
    } CVMID_localrootEnd();
}

#ifdef CVM_DEBUG_ASSERTS
char * const CVM_CharacterLatinA_data_start = (char* const) &CVM_CharacterLatinA_data;
const  CVMUint32 CVM_CharacterLatinA_data_size = sizeof(CVM_CharacterLatinA_data);
#endif
