/*
 * @(#)stackwalk.h	1.12 06/10/10
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

#ifndef _INCLUDED_STACKWALK_H
#define _INCLUDED_STACKWALK_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/stacks.h"

struct CVMStackWalkContext {
    CVMFrame*      frame;     /* The current frame of the walk */
    CVMStackChunk* chunk;     /* The last chunk of the current frame */
};

/*
 * Initialize and destroy a stack-walker context
 */
extern void
CVMstackwalkInit(CVMStack* stack, CVMStackWalkContext* c);

extern void
CVMstackwalkDestroy(CVMStackWalkContext* c);

/*
 * Go to the previous frame
 */
extern void
CVMstackwalkPrev(CVMStackWalkContext* c);

#endif /* _INCLUDED_STACKWALK_H */
