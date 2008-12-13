/*
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
 * This file contains parameters that may need to be altered as new
 * extensions are added to the underlying OpenGL ES engine.
 */

#ifndef _JSR239_GL_LIMITS_H
#define _JSR239_GL_LIMITS_H

/* 
 * The maximum number of values that can be returned by
 * glGet{Integer,Boolean,Fixed,Float}v.
 */
#define MAX_GET_PARAMS \
  ((GL_NUM_COMPRESSED_TEXTURE_FORMATS > 2) ? \
    GL_NUM_COMPRESSED_TEXTURE_FORMATS : 2)

/* 
 * The maximum number of values that can be returned by
 * glGetLight{f,x}v.
 */
#define MAX_GET_LIGHT_PARAMS 4

#define MAX_GET_MATERIAL_PARAMS 4
#define MAX_GET_TEX_ENV_PARAMS 4
#define MAX_GET_TEX_PARAMETER_PARAMS 1
#define MAX_GET_BUFFER_PARAMETER_PARAMS 1

#define MAX_GET_TEX_GEN_PARAMS 1

#define MAX_GET_RENDERBUFFER_PARAMETER_PARAMS 1024 /* Need revisit */
#define MAX_GET_FRAMEBUFFER_ATTACHMENT_PARAMETER_PARAMS 1024 /* Need revisit */

#endif /* #ifndef _JSR239_GL_LIMITS_H */
