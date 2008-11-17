/*
 * @(#)setTime.java	1.6 06/10/10
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

import java.util.zip.*;
import java.util.Date;

/* 
 * This test is to see whether clock_settime() is configured
 * appropriately on the hard target according to the instructions
 * listed in vxworks-notes.html.
 */
public class setTime {
  private static String testString = "ZipEntryTest";

  public static void main(String argv[]) {
        ZipEntry testentry = new ZipEntry(testString);

        Date date = new Date();
        long dummytime = date.getTime();
        testentry.setTime(dummytime);
        long newtime = testentry.getTime();
        System.out.println("dummytime "+ dummytime);
        System.out.println("newtime "+ newtime);
        if (Math.abs(dummytime - newtime) > 2000) {
            System.out.println("setTime(long) error.");
            return;
        }
        System.out.println("OKAY.");
    }
}

