/*
 * @(#)TestLongShiftSpeed.java	1.6 06/10/10
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
//
// @(#)TestLongShiftSpeed.java	1.2 02/03/02
// Measures the speed of computing long shifts.
//

public class TestLongShiftSpeed
{
    public static void main(String[] args) {
        long value = 0x21223123adfca251L;
        long j;
        int iterations = Integer.parseInt(args[0]);
        long t0 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            j = (value >>> i);
        }
        long t1 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            j = value;
        }
        long t2 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            j = (value >> i);
        }
        long t3 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            j = (value << i);
        }
        long t4 = System.currentTimeMillis();
        System.out.println("null loop " + iterations + " iterations in " +
            (t2 - t1) + "ms");
        System.out.println("lushr " + iterations + " iterations in " +
            (t1 - t0 - (t2 - t1)) + "ms");
        System.out.println("lshr " + iterations + " iterations in " +
            (t3 - t2 - (t2 - t1)) + "ms");
        System.out.println("lshl " + iterations + " iterations in " +
            (t4 - t3 - (t2 - t1)) + "ms");
    }
}
