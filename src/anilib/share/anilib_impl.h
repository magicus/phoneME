/*
 *   
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

/*
 * !!!!WARNING!!!! 
 *
 * This header file is used internally in Sun's ANILib implementation.
 * The declarations in this file are not public and shouldn't be used
 * by applications that use ANILib.
 *
 */

#ifndef _ANILIB_IMPL_H
#define _ANILIB_IMPL_H

#ifdef LINUX
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#endif

#include "jvmconfig.h"
#include "kni.h"
#include "jvm.h"
#include "jvmspi.h"
#include "sni.h"
#include "ani.h"

#include "os_port.h"
#include "poolthread.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This datastructure allows us to map a blocked Java thread to
 * the PoolThread assigned to perform the blocking operation.
 */
typedef struct _ANI_BlockingInfo {
  jint type;
  PoolThread * pt;
  jboolean is_blocked;
  jboolean parameter_block_allocated;
} ANI_BlockingInfo; 

#define ANI_BLOCK_INFO 0x12340000

#define OS_TIMEOUT   0
#define OS_SIGNALED  1

#ifndef JVM_ASSERT
/*
 * The JVM_ASSERT macro should be exported by jvm.h. We define
 * a dummy here so that you can build ANILib on top of CLDC-1.0.1 (but
 * to do the real checking, you need CLDC 1.1!
 */
#define JVM_ASSERT(b,msg)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ANILIB_IMPL_H */
