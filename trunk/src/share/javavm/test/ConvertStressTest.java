/*
 * @(#)ConvertStressTest.java	1.7 06/10/10
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

public class ConvertStressTest {
    boolean stop = false;
    static long loop = 1000;
    char charRef[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'k',
                      'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
                      'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4',
                      '5', '6', '7', '8', '9', '!', '@', '#', '$', '%',
                      '*', '(', ')', '-', '_', '+', '=', '|', ' ', '`',
                      '~', ',', '<', '.', '>', '/', '?', ' ', 'A', 'B',
                      'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
                      'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                      'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                      'g', 'h', 'i', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
                      'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0',
                      '1', '2', '3', '4', '5', '6', '7', '8', '9', '!'};
    byte byteRef[] = {97, 98, 99, 100, 101, 102, 103, 104, 105, 107, 
                      108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 
                      118, 119, 120, 121, 122, 48, 49, 50, 51, 52, 
                      53, 54, 55, 56, 57, 33, 64, 35, 36, 37, 
                      42, 40, 41, 45, 95, 43, 61, 124, 32, 96, 
                      126, 44, 60, 46, 62, 47, 63, 32, 65, 66, 
                      67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 
                      77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 
                      87, 88, 89, 90, 97, 98, 99, 100, 101, 102, 
                      103, 104, 105, 107, 108, 109, 110, 111, 112, 113, 
                      114, 115, 116, 117, 118, 119, 120, 121, 122, 48, 
                      49, 50, 51, 52, 53, 54, 55, 56, 57, 33};

    public static void main(String args[]) {

        for (int i=0; i<args.length; i++) {
            if (args[i].equalsIgnoreCase("-loop"))
                loop = new Long(args[++i]).longValue();
        }
        
        ConvertStressTest cst = new ConvertStressTest();
        cst.test();
    }

    void test() {
        GcThread gt = new GcThread();
        gt.start();

        ConvertThread ct = new ConvertThread();
        ct.start();
    }

    class ConvertThread extends Thread {
        public void run() {
            System.out.println("ConvertThread start...");

            for (long iter = 0; iter < loop; iter++) {
                int trash[] = new int[10000];
                byte byteArr[] = new byte[120];
                char charArr[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'k',
                                  'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
                                  'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4',
                                  '5', '6', '7', '8', '9', '!', '@', '#', '$', '%',
                                  '*', '(', ')', '-', '_', '+', '=', '|', ' ', '`',
                                  '~', ',', '<', '.', '>', '/', '?', ' ', 'A', 'B',
                                  'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
                                  'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                                  'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                  'g', 'h', 'i', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
                                  'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0',
                                  '1', '2', '3', '4', '5', '6', '7', '8', '9', '!'};

                trash = null;

                try {
                    CharToByteISO8859_1 ctb =
                        (CharToByteISO8859_1)CharToByteConverter.getConverter("8859_1");

                    ctb.convert(charArr, 0, 120, byteArr, 0, 120);
                } catch (Exception e) {
                    System.out.println(e.toString());
                } finally {
                    int i;
                    for (i=0; i<byteArr.length; i++) {
                        if (byteArr[i] != byteRef[i]) {
                            System.out.println("CharToByte Convert Fail.");
                            stop = true;
                        }
                    }
                }

                 try {
                    ByteToCharISO8859_1 btc =
                        (ByteToCharISO8859_1)ByteToCharConverter.getConverter("8859_1");

                    btc.convert(byteArr, 0, 120, charArr, 0, 120);
                } catch (Exception e) {
                    System.out.println(e.toString());
                } finally {
                    int i;
                    for (i=0; i<byteArr.length; i++) {
                        if (charArr[i] != charRef[i]) {
                            System.out.println("ByteToChar Convert Fail.");
                            stop = true;
                        }
                    }
                }

                try {
                    sleep(10);
                }catch (Exception e) {}
            }
            System.out.println("Success!");
            stop = true;

        }
    }

    class GcThread extends Thread {
        public void run() {
            System.out.println("GcThread start...");
            while (!stop) {
                Runtime.getRuntime().gc();
                try {
                    sleep(5);
                }catch (Exception e) {}
            }
        }     
    }
}
