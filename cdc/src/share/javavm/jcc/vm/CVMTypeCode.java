/*
 * @(#)CVMTypeCode.java	1.11 06/10/10
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
/*
 * CVM has a complex scheme for representing types as 16-bit
 * cookies. This code mimics the run-time code and thus builds the
 * corresponding data structures, to be ROMed.
 */

package vm;

public interface CVMTypeCode {
    public static final int CVM_T_ERROR   =          0;
    public static final int CVM_T_ENDFUNC =          1;
    public static final int CVM_T_VOID    =          2;
    public static final int CVM_T_INT     =          3;
    public static final int CVM_T_SHORT   =          4;
    public static final int CVM_T_CHAR    =          5;
    public static final int CVM_T_LONG    =          6;
    public static final int CVM_T_BYTE    =          7;
    public static final int CVM_T_FLOAT   =          8;
    public static final int CVM_T_DOUBLE  =          9;
    public static final int CVM_T_BOOLEAN =          10;

    public static final int CVM_T_OBJ     =          11;

    public static final int  CVMtypeLastScalar   =   CVM_T_BOOLEAN;

    // parameters of the encoding
    public static final int  CVMtypeArrayShift   =   14;
    static final int CVMtypeCookieSize   =   16;

    // the below values are all derived from the above as follows:
    // CVMtypeBasetypeMask =  ((1<<CVMtypeArrayShift) - 1)
    // CVMtypeArrayMask    =  ((1<<(CVMtypeCookieSize-CVMtypeArrayShift)) - 1)
    //				     <<CVMtypeArrayShift
    // CVMtypeBigArray     = CVMtypeArrayMask
    // CVMtypeMaxSmallArray = (CVMtypeBigArray>>CVMtypeArrayShift) - 1

    public static final int  CVMtypeArrayMask    =   0xc000;
    public static final int  CVMtypeBasetypeMask =   0x3fff;
    public static final int  CVMtypeBigArray     =   0xc000;
    public static final int  CVMtypeMaxSmallArray=   2;
 

}
