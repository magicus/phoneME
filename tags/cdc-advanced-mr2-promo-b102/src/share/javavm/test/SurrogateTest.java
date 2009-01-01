/*
 * @(#)SurrogateTest.java	1.7 06/10/10
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

import sun.io.*;

public class SurrogateTest{
    CharToByteISO8859_1 converter = null;
    byte byteArr[] = new byte[10];
    char charArr[] = {'a', 'b', 'c', 'd', 'e', 
                      '\uD800', '\uDC00', 'f', 'g', 'h'};

    public static void main(String args[]) {
        new SurrogateTest().run();
    }

    public void run() {
        int len = 0;

        try {
            converter =
                (CharToByteISO8859_1)CharToByteConverter.getConverter("8859_1");
        } catch (Exception e) {
            System.out.println(e.toString());
        }
        
        // Convert from 'a' to '\uD800' (high surrogate). 
        doConvert(1, 0, 6, 0, 10);

        // Convert from '\uDC00' (low surrogate) to 'h'.
        // This is a legal UTF16 sequence.
        doConvert(2, converter.nextCharIndex(), 10, converter.nextByteIndex(), 10);

        byteArr = new byte[10];
        doConvert(3, 0, 10, 0, 10);
    }

    void doConvert(int i, int inOff, int inEnd, int outOff, int outEnd) {
        int len=0;
        System.out.println("Case "+i+": inOff="+inOff+", inEnd="+inEnd+ 
                            ", outOff="+outOff+", outEnd="+outEnd);
        try {
            len = converter.convert(charArr, inOff, inEnd, byteArr, outOff, outEnd);
        } catch (Exception e) {
            System.out.println(e.toString());
        } finally {
            for (int j=0; j<10; j++) {
                System.out.println("byteArr["+j+"] = "+byteArr[j]);
            }
            System.out.println("Length of conversion from char to byte = " + len+"\n");
        }
    }
}

