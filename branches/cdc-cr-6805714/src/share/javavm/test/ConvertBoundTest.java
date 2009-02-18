/*
 * @(#)ConvertBoundTest.java	1.9 06/10/10
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

class ConvertBoundTest {
    CharToByteISO8859_1 converter = null;
    byte byteArr[] = new byte[30];
    char charArr[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'k',
                      'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
                      'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4',
                      '5', '6', '7', '8', '9', '!', '@', '#', '$', '%',
                      '*', '(', ')', '-', '_', '+', '=', '|', ' ', '`',
                      '1', '2', '3', '4', '5', '6', '7', '8', '9', '!'};

    public static void main(String args[]) {
        new ConvertBoundTest().run();
    }

    public void run() {
        int len = 0;

        byteArr[0] = 0x0077;
        byteArr[1] = 0x0066;
        byteArr[2] = 0x007f;

        try {
            converter =
                    (CharToByteISO8859_1)CharToByteConverter.getConverter("8859_1");
        } catch (Exception e) {
            System.out.println(e.toString());
        }

        doConvert(1, -1, 20, 0, 10);

        doConvert(2, 1, 20, -1, 10);

        doConvert(3, 1, -20, 1, 10);

        doConvert(4, 1, 20, 1, -10);

        doConvert(5, 2, 20, 1, 30);

        doConvert(6, 10, 8, 0, 30);

        doConvert(7, 40, 50, 5, 30);

        doConvert(8, 55, 66, 15, 30);

        doConvert(9, 55, 66, 15, 33);

        doConvert(10, 15, 35, 20, 40);

        doConvert(11, 61, 63, 32, 33);

        doConvert(12, 15, 35, 31, 40);
  
        byteArr = new byte[70]; 
        doConvert(13, 0, 60, 0, 70);

        doConvert(14, 61, 50, 72, 70); 

        doConvert(15, 66, 60, 60, 66);

        doConvert(16, -10, -8, 1, 10);

        doConvert(17, -6, -20, 2, 6);

        doConvert(18, 0, 2, -1, -2);

        doConvert(19, 3, 7, -8, -5);

        doConvert(20, 60, 60, 0, 30);

        doConvert(21, 50, 55, 71, 71);

        doConvert(22, 50, 60, 72, 77);

        doConvert(23, 56, 68, 75, 72);

        doConvert(24, 68, 80, 50, 56);

        doConvert(25, 30, 55, 80, 78);

        doConvert(26, -10, 5, 55, 50);

        doConvert(27, 10, 20, 66, 66);

        doConvert(28, 10, 10, 60, 66);

        charArr[55] = '\uD800';
        charArr[56] = '\uDC00'; 
        byteArr = new byte[10];
        doConvert(29, 50, 60, 0, 10);

        charArr[55] = '\uD800';
        charArr[56] = 'a';
        byteArr = new byte[10];
        doConvert(30, 50, 58, 0, 8);

        charArr[55] = '\uDC00';
        charArr[56] = 'a';
        byteArr = new byte[10];
        doConvert(31, 50, 60, 0, 10);
        
        charArr[55] = '\uD800';
        charArr[56] = '\uDC00';
        byteArr = new byte[10];
        byte newSubBytes[] = {};
        converter.setSubstitutionBytes(newSubBytes);
        doConvert(32, 50, 60, 0, 10);

        charArr = new char[10000];
        byteArr = new byte[8192];
        doConvert(33, 0, 10000, 0, 8192);

        char chars[] = {'a', 'b', 'c', '\uD800', '\uDC00', 'q', 'r', 's', 't'};
        charArr = chars;
        byteArr = new byte[10];
        doConvert(34, 0, 4, 0, 3);
        
        doConvert(35, 0, 5, 0, 3);

        doConvert(36, 0, 5, 0, 3);

        doConvert(37, 0, 6, 0, 3); 

        doConvert(38, 0, 5, 0, 4);

        byte newsubs[] = {'?'};
        converter.setSubstitutionBytes(newsubs);
        doConvert(39, 0, 8, 0, 7);

        converter.setSubstitutionBytes(newSubBytes);
        doConvert(40, 0, 8, 0, 6);

        doConvert(41, 0, 3, 0, 2);

        charArr[3] = 'd';
        doConvert(42, 0, 5, 0, 4);

        char cs[] = {'\uD800', '\uDC00', '\uD800', '\uDC00', '\uD800', '\uDC00'}; 
        charArr = cs;
        doConvert(43, 0, 5, 0, 1);

        doConvert(44, 0, 6, 0, 1);
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
            System.out.println("Length of conversion from char to byte = " + len+"\n");
        }
    }
}

