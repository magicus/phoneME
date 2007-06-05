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

#include <gxapi_constants.h>
#include <kni.h>

/**
 * @file
 *
 * Low level graphics internal methods.
 */

/**
 * Checks that the anchor is set correctly.
 *
 * @param anchor The anchor type. The definition of the anchor point must be one of
 * the horizontal constants (LEFT, HCENTER, RIGHT)  combined with one of
 * the vertical constants (TOP, BASELINE, BOTTOM, VCENTER) using the bit-wise OR operator.
 * Zero may also be used as the value of an anchor point.
 * Using zero for the anchor point value gives results identical to using TOP | LEFT.
 *
 * @param illegal_vpos The component from list of constants (LEFT, HCENTER, RIGHT,
 * TOP, BASELINE, BOTTOM, VCENTER) that is not allowed for this anchor. Using zero for illegal_vpos shows all
 * values from this list are allowed
 * @return 1 if anchor is valid, 0 if invalid.
 *
 */

int check_anchor(int anchor, int illegal_vpos) {
    int OK;

    /*optimize for most frequent case*/
    if (anchor == (TOP|LEFT) || anchor == 0) {
	return 1;
    }

    OK = (anchor > 0) && (anchor < (BASELINE << 1))
        && ((anchor & illegal_vpos) == 0);

    if (OK) {
        int n = anchor & (TOP | BOTTOM | BASELINE | VCENTER);
        OK = (n != 0) && ((n & (n-1)) == 0); /* exactly one bit set */
    }

    if (OK) {
        int n = anchor & (LEFT | RIGHT | HCENTER);
        OK = (n != 0) && ((n & (n-1)) == 0); /* exactly one bit set */
    }

    return OK;
}

/**
 * normalizes anchor coordinates to top-left coordinates.
 *
 * @return 1 if anchor is valid, otherwise 0.
 */
int normalize_anchor(jint* X, jint* Y, jint width, jint height, 
			   jint anchor) {
    /*optimize for most frequent case*/
    if (anchor == (TOP|LEFT) || anchor == 0) {
      return 1;
    }

    if ((anchor & 0x7F) != anchor) {
      return 0;
    }

    switch (anchor & (LEFT | RIGHT | HCENTER)) {
    case LEFT:
      break;
      
    case RIGHT:
      *X -= width;
      break;
      
    case HCENTER:
      *X -= (width >> 1);
      break;
    
    default:
      return 0;
    }
    
    switch (anchor & (TOP | BOTTOM | VCENTER)) {
    case TOP:
      break;
      
    case BOTTOM:
      *Y -= height;
      break;
      
    case VCENTER:
      *Y -= (height >> 1);
      break;
    
    default:
      return 0;
    }

    return 1;
}
