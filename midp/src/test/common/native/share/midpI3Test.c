/*
 * 	
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * @file
 *
 * Native functions for the I3 test framework.
 */

#include <kni.h>
#include <sni.h>
#include <stdio.h>


static jint refSemReady = -1;     /* ref ID for the semReady semaphore */
static jint refSemContinue = -1;  /* ref ID for the semContinue semaphore */


/**
 * Initializes semaphores for cross-isolate synchronization.
 * <p>
 * Java declaration:
 * <pre>
 *     init0(Lcom/sun/cldc/util/Semaphore;Lcom/sun/cldc/util/Semaphore;)V
 * </pre>
 * Java parameters:
 * <pre>
 *     semReady     the semaphore the slave releases when its values
 *                  ready to have their assertions checked
 *     semContinue  the semaphore the master releases when the slave is
 *                  allowed to continue
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_i3test_IsolateSynch_init0(void) {
    if (refSemReady != -1 || refSemContinue != -1) {
	KNI_ThrowNew("java/lang/IllegalStateException", NULL);
	KNI_ReturnVoid();
    }

    KNI_StartHandles(2);
    KNI_DeclareHandle(semReady);
    KNI_DeclareHandle(semContinue);

    KNI_GetParameterAsObject(1, semReady);
    KNI_GetParameterAsObject(2, semContinue);

    refSemReady = SNI_AddStrongReference(semReady);
    if (refSemReady == -1) {
	KNI_FatalError("refSemReady: out of memory");
    }

    refSemContinue = SNI_AddStrongReference(semContinue);
    if (refSemReady == -1) {
	KNI_FatalError("refSemContinue: out of memory");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * getSemReady0(V)Lcom/sun/cldc/util/Semaphore;
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_i3test_IsolateSynch_getSemReady0() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(sem);
    if (refSemReady == -1) {
	KNI_ReleaseHandle(sem);
    } else {
	SNI_GetReference(refSemReady, sem);
    }

    KNI_EndHandlesAndReturnObject(sem);
}


/**
 * getSemContinue0(V)Lcom/sun/cldc/util/Semaphore;
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_i3test_IsolateSynch_getSemContinue0() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(sem);
    if (refSemContinue == -1) {
	KNI_ReleaseHandle(sem);
    } else {
	SNI_GetReference(refSemContinue, sem);
    }

    KNI_EndHandlesAndReturnObject(sem);
}

/**
 * fini0(V)V
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_i3test_IsolateSynch_fini0() {
    if (refSemReady != -1) {
	SNI_DeleteReference(refSemReady);
	refSemReady = -1;
    }

    if (refSemContinue != -1) {
	SNI_DeleteReference(refSemContinue);
	refSemContinue = -1;
    }

    KNI_ReturnVoid();
}
