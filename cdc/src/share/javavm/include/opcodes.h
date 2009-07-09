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
 *
 */

#ifndef _INCLUDED_OPCODES_H
#define _INCLUDED_OPCODES_H

#include "generated/javavm/include/gen_opcodes.h"

extern const char* const CVMopnames[];
extern const char  CVMopcodeLengths[];
#ifdef CVM_JIT
#define JITMAP_OPC    0
#define JITMAP_TYPEID 1
#define JITMAP_CONST  2
extern const signed char  CVMJITOpcodeMap[][3];
#endif

/*
 * Get the length of the variable-length instruction at 'iStream'
 */
extern CVMUint32 CVMopcodeGetLengthVariable(const CVMUint8* iStream);

#define CVMopcodeGetLength(iStream) \
    (CVMopcodeLengths[*(iStream)] ? \
     CVMopcodeLengths[*(iStream)] : \
     CVMopcodeGetLengthVariable((iStream)))

typedef enum CVMOpcode CVMOpcode;

/*
 * The verifier needs to be careful not to read off the end of the
 * code when determining opcode length and validity, so it needs its
 * own version of CVMopcodeGetLength() that checks bounds.
 */
#define CVMopcodeGetLengthWithBoundsCheck(iStream, iStream_end)	\
	(CVMopcodeLengths[*(iStream)] ? \
	 CVMopcodeLengths[*(iStream)] : \
	 CVMopcodeGetLengthWithBoundsCheckVariable(iStream, iStream_end))

extern int
CVMopcodeGetLengthWithBoundsCheckVariable(const unsigned char* iStream,
					  const unsigned char* iStream_end);

#endif /* _INCLUDED_OPCODES_H */
