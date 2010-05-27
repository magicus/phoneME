/*
 * @(#)stackwalk.c	1.14 06/10/10
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

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/stacks.h"
#include "javavm/include/stackwalk.h"
#include "javavm/include/clib.h"

/*
 * Initialize stack walk to point to top of frame
 */
void
CVMstackwalkInit(CVMStack* s, CVMStackWalkContext* c)
{
    c->frame   = s->currentFrame;
    c->chunk   = s->currentStackChunk;
}

/*
 * Destroy stack walk
 */
void
CVMstackwalkDestroy(CVMStackWalkContext* c)
{
}

/*
 * Get previous frame, and make c->chunk follow to point to the last
 * chunk of the frame
 */
void
CVMstackwalkPrev(CVMStackWalkContext* c)
{
    CVMFrame* curFrame = c->frame;
    CVMframeGetPreviousUpdateChunk(c->chunk, curFrame, {c->chunk = chunk_;});
    c->frame = curFrame;
}
