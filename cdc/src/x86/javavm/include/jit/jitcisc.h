/*
 * @(#)jitcisc.h	1.5 06/10/10
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

#ifndef _INCLUDED_JITCISC_H
#define _INCLUDED_JITCISC_H

/*
 * This is part of the porting layer that cpu specific code must 
 * implement for the shared risc code.
 *
 * FIXME - document everything that needs to be exported.
 */

#include "javavm/include/jit/jitcisc_cpu.h"

/* The platform should define CVMCPU_VIRTUAL_INLINE_OUTOFLINE_MB_TEST_REG_NUM
 * to be the number of scratch registers it needs to implement the out of line c
hecks
 * for an inlined virtual invoke.  If the platform does not explicitly
 * specify the number of scratch registers it needs for this, the RISC layer
 * will assign a default number below.
 */
#ifndef CVMCPU_VIRTUAL_INLINE_OUTOFLINE_MB_TEST_REG_NUM
#define CVMCPU_VIRTUAL_INLINE_OUTOFLINE_MB_TEST_REG_NUM   2 /* default */
#endif

#endif /* _INCLUDED_JITCISC_H */
