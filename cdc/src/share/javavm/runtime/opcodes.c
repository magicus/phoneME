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

#include "javavm/include/opcodes.h"
#include "javavm/include/utils.h"
#include "javavm/include/porting/int.h"

int
CVMopcodeGetLengthWithBoundsCheckVariable(const unsigned char* iStream,
					  const unsigned char* iStream_end)
{
    /*
     * If there is not enough code left to determine the opcode
     * length, return a value that indicates the opcode extends
     * past the end of the code.
     */
    switch(*iStream) {
	case opc_tableswitch: {
	    const unsigned char* lpc =
		(const unsigned char*)CVMalignWordUp(iStream + 1);
	    /* Need default, low, high. */
	    if (iStream_end - lpc < 12) {
		return 255;
	    }
	    break;
	}
        case opc_lookupswitch: {
	    const unsigned char* lpc =
		(const unsigned char*)CVMalignWordUp(iStream + 1);
	    /* Need default, npairs. */
	    if (iStream_end - lpc < 8) {
		return 255;
	    }
	    break;
	}
        case opc_wide: {
	    /* Need opcode. */
	    if (iStream_end - iStream < 2) {
		return 255;
	    }
	    break;
	}
#ifdef CVM_HW
#include "include/hw/verifycode.i"
	    break;
#endif
	default:
	    break;
    }

    return CVMopcodeGetLengthVariable(iStream);
}

/*
 * Find the length of the variable length instruction at 'iStream'.
 * This is called if CVMopcodeLengths[*iStream] == 0.
 */
CVMUint32
CVMopcodeGetLengthVariable(const CVMUint8* iStream)
{
    CVMUint8  opcode  = *iStream;
    /*
     * Variable length. Intelligence required.
     */
    switch(opcode) {
	case opc_tableswitch: {
	    CVMInt32* lpc  = (CVMInt32*)CVMalignWordUp(iStream + 1);
	    CVMInt32  low  = CVMgetAlignedInt32(&lpc[1]);
	    CVMInt32  high = CVMgetAlignedInt32(&lpc[2]);
	    if (((CVMUint32)high - (CVMUint32)low) > 0xffff) {
		return -1; /* invalid instruction */
	    }
	    /*
	     * Skip default, low, high, and the (high-low+1) jump offsets
	     */
	    /* 
	     * This expression is obviously a rather small pointer
	     * difference. So just cast it to the return type.
	     */
	    return (CVMUint32)((CVMUint8*)(&lpc[3 + (high - low + 1)]) - iStream);
	}
        case opc_lookupswitch: {
	    CVMInt32* lpc    = (CVMInt32*)CVMalignWordUp(iStream+1);
	    CVMInt32  npairs = CVMgetAlignedInt32(&lpc[1]);
	    if ((npairs < 0) || (npairs > 65535)) {
		return -1; /* invalid instruction */
	    }
	    /*
	     * Skip default, npairs, and npairs pairs.
	     */
	    /* 
	     * This expression is obviously a rather small pointer
	     * difference. So just cast it to the return type.
	     */
	    return (CVMUint32)((CVMUint8*)(&lpc[2 + npairs * 2]) - iStream);
	}
        case opc_wide: {
	    switch(iStream[1]) {
	        case opc_ret:
	        case opc_iload: case opc_istore: 
	        case opc_fload: case opc_fstore:
	        case opc_aload: case opc_astore:
	        case opc_lload: case opc_lstore:
	        case opc_dload: case opc_dstore: 
		    return 4;
		case opc_iinc:
		    return 6;
		default: 
		    return -1; /* invalid instruction */
	    }
	}
#ifdef CVM_HW
#include "include/hw/interpreter.i"
#endif
    default:
	return -1; /* I don't know this bytecode! */
    }
}
