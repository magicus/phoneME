/*
 * @(#)ExerciseOpcodes.java	1.17 06/10/10
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
    NOTE: This module compiles some methods which are specifically designed
    to operate on some bytecodes.  And compares the compiled method against
    the expected behavior as determined by an interpreted version.
*/

public class ExerciseOpcodes
{
    static boolean verbose = false;
    static int totalTests = 0;
    static int totalFailures = 0;

    static void dumpValue(boolean value) {
        System.out.println("   boolean value: " + value);
    }
    static void dumpValue(int value) {
        System.out.println("   int value: " + value);
    }
    static void dumpValue(long value) {
        System.out.println("   long value: " + value);
    }
    static void dumpValue(float value) {
        System.out.println("   float value: " + value + " (0x" +
            Integer.toHexString(Float.floatToIntBits(value)) + ")");
    }
    static void dumpValue(double value) {
        System.out.println("   double value: " + value + " (0x" +
            Long.toHexString(Double.doubleToLongBits(value)) + ")");
    }

    static boolean
    reportPassIf(String testName, boolean success) {
        if (!success && !verbose) {
            System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                               testName);
        }
        totalTests++;
        if (!success) {
            totalFailures++;
        }
        return success;
    }

    static boolean
    reportPassIf(String testName, boolean actual, boolean expected) {
        boolean success = (actual == expected);
        reportPassIf(testName, success);
        if (!success) {
            System.out.println("   Expected = " + expected);
            System.out.println("   Actual = " + actual);
        }
        return success;
    }

    static boolean
    reportPassIf(String testName, int actual, int expected) {
        boolean success = (actual == expected);
        reportPassIf(testName, success);
        if (!success) {
            System.out.println("   Expected = " + expected);
            System.out.println("   Actual = " + actual);
        }
        return success;
    }

    static boolean
    reportPassIf(String testName, long actual, long expected) {
        boolean success = (actual == expected);
        reportPassIf(testName, success);
        if (!success) {
            System.out.println("   Expected = " + expected);
            System.out.println("   Actual = " + actual);
        }
        return success;
    }

    static boolean
    reportPassIf(String testName, float actual, float expected) {
        boolean success = (actual == expected);

        if (!success) {
            // One possibility of why the comparison is if both are NaNs.
            // Check for this case explicitly if necessary:
            if (Float.isNaN(actual) && Float.isNaN(expected)) {
                success = true;
            }
        }

        reportPassIf(testName, success);
        if (!success) {
            System.out.println("   Expected = " + expected + " (0x" +
                Integer.toHexString(Float.floatToIntBits(expected)) + ")");
            System.out.println("   Actual = " + actual + " (0x" +
                Integer.toHexString(Float.floatToIntBits(actual)) + ")");
        }
        return success;
    }

    static boolean
    reportPassIf(String testName, double actual, double expected) {
        boolean success = (actual == expected);

        if (!success) {
            // One possibility of why the comparison is if both are NaNs.
            // Check for this case explicitly if necessary:
            if (Double.isNaN(actual) && Double.isNaN(expected)) {
                success = true;
            }
        }

        reportPassIf(testName, success);
        if (!success) {
            System.out.println("   Expected = " + expected);
            System.out.println("   Actual = " + actual);
        }
        return success;
    }

    static boolean
    reportPassIf(String testName, Object actual, Object expected) {
        boolean success = (actual == expected);
        reportPassIf(testName, success);
        if (!success) {
            System.out.println("   Expected = " + expected);
            System.out.println("   Actual = " + actual);
        }
        return success;
    }

    public static void main(String[] args) {

        String[] compileItems = {
            "ExerciseReturnOpcodes",
            "ExerciseIntOpcodes",
            "ExerciseLongOpcodes",
            "ExerciseFloatOpcodes",
            "ExerciseDoubleOpcodes",
            "ExerciseArrayOpcodes",
        };

        // Do interpreted run to take care of clinit:
        //System.err.println("Compiling named classes...\n");
        //exerciseOpcodes();

        /*
         * Compile the associated classes
         */
        System.out.println("Compiling named classes...\n");
        CompilerTest.main(compileItems);

        totalTests = 0;
        totalFailures = 0;

        /*
         * Run the tests:
         */
        System.out.println("Exercise opcodes:\n");
        exerciseOpcodes();
        exerciseIDivOpcodes();
        exerciseIRemOpcodes();
        exerciseIMulOpcodes();

        // Report the total number of failures:
        System.out.println("Tests ran: " + totalTests + ", failures: " + totalFailures);
    }

    static final int dividends[] = { 
        0, 1, 2, 3, 4, 5, 6, 7, 9, 10,
        15, 23, 28, 69, 100, 127, 168, 378,
        500, 1029, 2048, 2056, 4096, 4230,
        8192, 8392, 16384, 16397, 32768, 32775,
        65536, 65537, 131072, 131085, 262144, 262174,
        524288, 524293, 1048576, 1048579,
        2097152, 2097157, 4194304, 4194309,
        8388608, 8388609, 16777216, 16777217,
        33554432, 33554436, 67108864, 67108868,
        134217728, 134217729, 268435456, 268435457,
        536870912, 536870913, 1073741824, 1073741825,
        2147483647, 

        17, 100, 125, 1027, 5612712, 0x7fffffff,

        -1, -2, -3, -4, -5, -6, -7, -9, -10,
        -15, -23, -28, -69, -100, -127, -168, -378,
        -500, -1029, -2048, -2056, -4096, -4230,
        -8192, -8392, -16384, -16397, -32768, -32775,
        -65536, -65537, -131072, -131085, -262144, -262174,
        -524288, -524293, -1048576, -1048579,
        -2097152, -2097157, -4194304, -4194309,
        -8388608, -8388609, -16777216, -16777217,
        -33554432, -33554436, -67108864, -67108868,
        -134217728, -134217729, -268435456, -268435457,
        -536870912, -536870913, -1073741824, -1073741825,
        -2147483647, -2147483648,

        -17, -100, -125, -1027, -5612712, -0x7fffffff,
    };

    static void exerciseOpcodes() {

        // Exercise the Return opcodes:
        System.out.println("Testing Return Opcodes:");
        ExerciseReturnOpcodes er = new ExerciseReturnOpcodes();
        {
            Object o = new Object();
            er.exerciseReturn(o);
            reportPassIf("exerciseReturn", true);
        }
        {
            int value = er.exerciseIReturn(5);
            reportPassIf("exerciseIReturn", value, 5);
        }
        {
            float value = er.exerciseFReturn(5.0f);
            reportPassIf("exerciseFReturn", value, 5.0f);
        }
        {
            long value = er.exerciseLReturn(5l);
            reportPassIf("exerciseLReturn", value, 5l);
        }
        {
            double value = er.exerciseDReturn(5.0d);
            reportPassIf("exerciseDReturn", value, 5.0d);
        }
        {
            Object o = new Object();
            Object value;
            value = er.exerciseAReturn(o);
            reportPassIf("exerciseAReturn(o)", value, o);
            value = er.exerciseAReturn(null);
            reportPassIf("exerciseAReturn(null)", value, null);
        }
        if (verbose) {
            System.out.println("");
        }

        // Exercise the Int opcodes:
        System.out.println("Testing Int Opcodes:");
        ExerciseIntOpcodes ei = new ExerciseIntOpcodes();
        {
            byte value = ei.exerciseI2B(5);
            reportPassIf("exerciseI2B(5)", value, 5);
        }
        {
            char value = ei.exerciseI2C((int)'A');
            reportPassIf("exerciseI2C(5)", value, 'A');
        }
        {
            short value = ei.exerciseI2S(5);
            reportPassIf("exerciseI2S(5)", value, 5);
        }
        {
            double value = ei.exerciseI2D(5);
            reportPassIf("exerciseI2D(5)", value, 5.0d);
        }
        {
            float value = ei.exerciseI2F(5);
            reportPassIf("exerciseI2F(5)", value, 5.0f);
        }
        {
            long value = ei.exerciseI2L(5);
            reportPassIf("exerciseI2L(5)", value, 5l);
        }

        {
            int[] ia = new int[5];
            ia[3] = 5;
            int value = ei.exerciseIALoad(ia, 3);
            reportPassIf("exerciseIALoad(ia, 3)", value, 5);
        }
        {
            int[] ia = new int[5];
            ia[0] = 5;
            int value = ei.exerciseIALoad_0(ia);
            reportPassIf("exerciseIALoad_0(ia)", value, 5);
        }
        {
            int[] ia = new int[5];
            ia[1] = 5;
            int value = ei.exerciseIALoad_1(ia);
            reportPassIf("exerciseIALoad_1(ia)", value, 5);
        }
        {
            int[] ia = new int[300];
            ia[255] = 5;
            int value = ei.exerciseIALoad_255(ia);
            reportPassIf("exerciseIALoad_255(ia)", value, 5);
        }
        {
            int[] ia = new int[300];
            ia[256] = 5;
            int value = ei.exerciseIALoad_256(ia);
            reportPassIf("exerciseIALoad_256(ia)", value, 5);
        }
        {
            int[] ia = new int[5000];
            ia[4*1024-4] = 5;
            int value = ei.exerciseIALoad_4Km4(ia);
            reportPassIf("exerciseIALoad_4Km4(ia)", value, 5);
        }
        {
            int[] ia = new int[5000];
            ia[4*1024] = 5;
            int value = ei.exerciseIALoad_4K(ia);
            reportPassIf("exerciseIALoad_4K(ia)", value, 5);
        }
        {
            int[] ia = new int[5];
            ia[3] = 0;
            ei.exerciseIAStore(ia, 3, 5);
            reportPassIf("exerciseIAStore(ia, 3, 5)", ia[3], 5);
        }
        {
            int value = ei.exerciseIConst0();
            reportPassIf("exerciseIConst0()", value, 0);
        }
        {
            int value = ei.exerciseIConst1();
            reportPassIf("exerciseIConst1()", value, 1);
        }
        {
            int value = ei.exerciseIConst255();
            reportPassIf("exerciseIConst255()", value, 255);
        }
        {
            int value = ei.exerciseIConst256();
            reportPassIf("exerciseIConst256()", value, 256);
        }
        {
            int value = ei.exerciseIConst4Km4();
            reportPassIf("exerciseIConst4Km4()", value, 4*1024-4);
        }
        {
            int value = ei.exerciseIConst4K();
            reportPassIf("exerciseIConst4K()", value, 4*1024);
        }

        // Test IShl permutations:
        {
            int value1 = 0x12345678;
            int value2 = -1;
            int value = ei.exerciseIShl(value1, value2);
            reportPassIf("exerciseIShl(" + value1 + ", " + value2 + ")",
                         value, value1 << value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 0;
            int value = ei.exerciseIShl(value1, value2);
            reportPassIf("exerciseIShl(" + value1 + ", " + value2 + ")",
                         value, value1 << value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 31;
            int value = ei.exerciseIShl(value1, value2);
            reportPassIf("exerciseIShl(" + value1 + ", " + value2 + ")",
                         value, value1 << value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 32;
            int value = ei.exerciseIShl(value1, value2);
            reportPassIf("exerciseIShl(" + value1 + ", " + value2 + ")",
                         value, value1 << value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 33;
            int value = ei.exerciseIShl(value1, value2);
            reportPassIf("exerciseIShl(" + value1 + ", " + value2 + ")",
                         value, value1 << value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = -1;
            int value = ei.exerciseIShlm1(value1);
            reportPassIf("exerciseIShlm1(" + value1 + ")",
                         value, value1 << value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 0;
            int value = ei.exerciseIShl0(value1);
            reportPassIf("exerciseIShl0(" + value1 + ")",
                         value, value1 << value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 31;
            int value = ei.exerciseIShl31(value1);
            reportPassIf("exerciseIShl31(" + value1 + ")",
                         value, value1 << value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 32;
            int value = ei.exerciseIShl32(value1);
            reportPassIf("exerciseIShl32(" + value1 + ")",
                         value, value1 << value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 33;
            int value = ei.exerciseIShl33(value1);
            reportPassIf("exerciseIShl33(" + value1 + ")",
                         value, value1 << value2);
        }

        // Test IShr permutations:
        {
            int value1 = 0x12345678;
            int value2 = -1;
            int value = ei.exerciseIShr(value1, value2);
            reportPassIf("exerciseIShr(" + value1 + ", " + value2 + ")",
                         value, value1 >> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 0;
            int value = ei.exerciseIShr(value1, value2);
            reportPassIf("exerciseIShr(" + value1 + ", " + value2 + ")",
                         value, value1 >> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 31;
            int value = ei.exerciseIShr(value1, value2);
            reportPassIf("exerciseIShr(" + value1 + ", " + value2 + ")",
                         value, value1 >> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 32;
            int value = ei.exerciseIShr(value1, value2);
            reportPassIf("exerciseIShr(" + value1 + ", " + value2 + ")",
                         value, value1 >> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 33;
            int value = ei.exerciseIShr(value1, value2);
            reportPassIf("exerciseIShr(" + value1 + ", " + value2 + ")",
                         value, value1 >> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = -1;
            int value = ei.exerciseIShrm1(value1);
            reportPassIf("exerciseIShrm1(" + value1 + ")",
                         value, value1 >> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 0;
            int value = ei.exerciseIShr0(value1);
            reportPassIf("exerciseIShr0(" + value1 + ")",
                         value, value1 >> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 31;
            int value = ei.exerciseIShr31(value1);
            reportPassIf("exerciseIShr31(" + value1 + ")",
                         value, value1 >> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 32;
            int value = ei.exerciseIShr32(value1);
            reportPassIf("exerciseIShr32(" + value1 + ")",
                         value, value1 >> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 33;
            int value = ei.exerciseIShr33(value1);
            reportPassIf("exerciseIShr33(" + value1 + ")",
                         value, value1 >> value2);
        }

        // Test IUShr permutations:
        {
            int value1 = 0x12345678;
            int value2 = -1;
            int value = ei.exerciseIUShr(value1, value2);
            reportPassIf("exerciseIUShr(" + value1 + ", " + value2 + ")",
                         value, value1 >>> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 0;
            int value = ei.exerciseIUShr(value1, value2);
            reportPassIf("exerciseIUShr(" + value1 + ", " + value2 + ")",
                         value, value1 >>> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 31;
            int value = ei.exerciseIUShr(value1, value2);
            reportPassIf("exerciseIUShr(" + value1 + ", " + value2 + ")",
                         value, value1 >>> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 32;
            int value = ei.exerciseIUShr(value1, value2);
            reportPassIf("exerciseIUShr(" + value1 + ", " + value2 + ")",
                         value, value1 >>> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 33;
            int value = ei.exerciseIUShr(value1, value2);
            reportPassIf("exerciseIUShr(" + value1 + ", " + value2 + ")",
                         value, value1 >>> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = -1;
            int value = ei.exerciseIUShrm1(value1);
            reportPassIf("exerciseIUShrm1(" + value1 + ")",
                         value, value1 >>> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 0;
            int value = ei.exerciseIUShr0(value1);
            reportPassIf("exerciseIUShr0(" + value1 + ")",
                         value, value1 >>> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 31;
            int value = ei.exerciseIUShr31(value1);
            reportPassIf("exerciseIUShr31(" + value1 + ")",
                         value, value1 >>> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 32;
            int value = ei.exerciseIUShr32(value1);
            reportPassIf("exerciseIUShr32(" + value1 + ")",
                         value, value1 >>> value2);
        }
        {
            int value1 = 0x12345678;
            int value2 = 33;
            int value = ei.exerciseIUShr33(value1);
            reportPassIf("exerciseIUShr33(" + value1 + ")",
                         value, value1 >>> value2);
        }
        if (verbose) {
            System.out.println("");
        }

        // Exercise the Long opcodes:
        System.out.println("Testing Long Opcodes:");
        ExerciseLongOpcodes el = new ExerciseLongOpcodes();
        {
            double value = el.exerciseL2D(5l);
            reportPassIf("exerciseL2D(5l)", value, 5.0d);
        }
        {
            float value = el.exerciseL2F(5l);
            reportPassIf("exerciseL2F(5l)", value, 5.0f);
        }
        {
            int value = el.exerciseL2I(5l);
            reportPassIf("exerciseL2I(5l)", value, 5);
        }
        {
            long value = el.exerciseLAdd(2l, 3l);
            reportPassIf("exerciseLAdd(2l, 3l)", value, 5l);
        }
        {
            long[] la = new long[5];
            la[3] = 13l;
            long value = el.exerciseLALoad(la, 3);
            reportPassIf("exerciseLALoad(la, 3)", value, 13l);
        }
        {
            long[] la = new long[5];
            la[0] = 13l;
            long value = el.exerciseLALoad_0(la);
            reportPassIf("exerciseLALoad_0(la)", value, 13l);
        }
        {
            long[] la = new long[5];
            la[1] = 13l;
            long value = el.exerciseLALoad_1(la);
            reportPassIf("exerciseLALoad_1(la)", value, 13l);
        }
        {
            long[] la = new long[300];
            la[255] = 13l;
            long value = el.exerciseLALoad_255(la);
            reportPassIf("exerciseLALoad_255(la)", value, 13l);
        }
        {
            long[] la = new long[300];
            la[256] = 13l;
            long value = el.exerciseLALoad_256(la);
            reportPassIf("exerciseLALoad_256(la)", value, 13l);
        }
        {
            long[] la = new long[5000];
            la[4*1024-4] = 13l;
            long value = el.exerciseLALoad_4Km4(la);
            reportPassIf("exerciseLALoad_4Km4(la)", value, 13l);
        }
        {
            long[] la = new long[5000];
            la[4*1024] = 13l;
            long value = el.exerciseLALoad_4K(la);
            reportPassIf("exerciseLALoad_4K(la)", value, 13l);
        }
        {
            long value1 = 0x55555555AAAAAAAAl;
            long value2 = 0x5555AAAA5555AAAAl;
            long value = el.exerciseLAnd(value1, value2);
            reportPassIf("exerciseLAnd(value1, value2)", value, (value1 & value2));
        }
        {
            long[] la = new long[5];
            la[3] = 0;
            el.exerciseLAStore(la, 3, 13l);
            reportPassIf("exerciseLAStore(la, 3, 13l)", la[3], 13l);
        }

        {
            long value1 = 0x55555555AAAAAAAAl;
            long value2 = 0x5555AAAA5555AAAAl;
            boolean value = el.exerciseLCmp_EQ(value1, value1);
            reportPassIf("exerciseLCmp_EQ(value1, value1)", value, true);
            value = el.exerciseLCmp_EQ(value1, value2);
            reportPassIf("exerciseLCmp_EQ(value1, value2)", value, false);
        }
        {
            long value1 = 0x55555555AAAAAAAAl;
            long value2 = 0x5555AAAA5555AAAAl;
            boolean value = el.exerciseLCmp_NE(value1, value1);
            reportPassIf("exerciseLCmp_EQ(value1, value1)", value, false);
            value = el.exerciseLCmp_NE(value1, value2);
            reportPassIf("exerciseLCmp_NE(value1, value2)", value, true);
        }
        {
            boolean value = el.exerciseLCmp_GT(5, -5);
            reportPassIf("exerciseLCmp_GT(5, -5)", value, true);
            value = el.exerciseLCmp_GT(5, 5);
            reportPassIf("exerciseLCmp_GT(5, 5)", value, false);
            value = el.exerciseLCmp_GT(-5, 5);
            reportPassIf("exerciseLCmp_GT(-5, 5)", value, false);
        }
        {
            boolean value = el.exerciseLCmp_GE(5, -5);
            reportPassIf("exerciseLCmp_GE(5, -5)", value, true);
            value = el.exerciseLCmp_GE(5, 5);
            reportPassIf("exerciseLCmp_GE(5, 5)", value, true);
            value = el.exerciseLCmp_GE(-5, 5);
            reportPassIf("exerciseLCmp_GE(-5, 5)", value, false);
        }
        {
            boolean value = el.exerciseLCmp_LT(5, -5);
            reportPassIf("exerciseLCmp_LT(5, -5)", value, false);
            value = el.exerciseLCmp_LT(5, 5);
            reportPassIf("exerciseLCmp_LT(5, 5)", value, false);
            value = el.exerciseLCmp_LT(-5, 5);
            reportPassIf("exerciseLCmp_LT(-5, 5)", value, true);
        }
        {
            boolean value = el.exerciseLCmp_LE(5, -5);
            reportPassIf("exerciseLCmp_LE(5, -5)", value, false);
            value = el.exerciseLCmp_LE(5, 5);
            reportPassIf("exerciseLCmp_LE(5, 5)", value, true);
            value = el.exerciseLCmp_LE(-5, 5);
            reportPassIf("exerciseLCmp_LE(-5, 5)", value, true);
        }

        {
            long value = el.exerciseLConst0();
            reportPassIf("exerciseLConst0()", value, 0l);
        }
        {
            long value = el.exerciseLConst1();
            reportPassIf("exerciseLConst1()", value, 1l);
        }
        {
            long value = el.exerciseLDiv(200l, 5l);
            reportPassIf("exerciseLDiv(200l, 5l)", value, (200l / 5l));
        }
        {
            long value = el.exerciseLLoad0(5l);
            reportPassIf("exerciseLLoad0(5l)", value, 5l);
        }
        {
            long value = el.exerciseLLoad1(0, 5l);
            reportPassIf("exerciseLLoad1(0, 5l)", value, 5l);
        }
        {
            long value = el.exerciseLLoad2(0, 1, 5l);
            reportPassIf("exerciseLLoad2(0, 1, 5l)", value, 5l);
        }
        {
            long value = el.exerciseLLoad3(0, 1, 2, 5l);
            reportPassIf("exerciseLLoad3(0, 1, 2, 5l)", value, 5l);
        }
        {
            long value = el.exerciseLLoad(0, 1, 2, 3, 5l);
            reportPassIf("exerciseLLoad(0, 1, 2, 3, 5l)", value, 5l);
        }
        {
            long value;

            value = el.exerciseLMul(1l, -1l);
            reportPassIf("exerciseLMul(1l, -1l)", value, (1l * -1l));
            value = el.exerciseLMul(-1l, 1l);
            reportPassIf("exerciseLMul(-1l, 1l)", value, (-1l * 1l));
            value = el.exerciseLMul(-1l, -1l);
            reportPassIf("exerciseLMul(-1l, -1l)", value, (-1l * -1l));

            value = el.exerciseLMul(12l, -13l);
            reportPassIf("exerciseLMul(12l, -13l)", value, (12l * -13l));
            value = el.exerciseLMul(12l, -13l);
            reportPassIf("exerciseLMul(12l, -13l)", value, (12l * -13l));
            value = el.exerciseLMul(-13l, 12l);
            reportPassIf("exerciseLMul(-13l, 12l)", value, (-13l * 12l));
            value = el.exerciseLMul(-13l, -13l);
            reportPassIf("exerciseLMul(-13l, -13l)", value, (-13l * -13l));
            value = el.exerciseLMul(12l, 10L);
            reportPassIf("exerciseLMul(12l, 10l)", value, (12l * 10l));
            value = el.exerciseLMul(12l, 0L);
            reportPassIf("exerciseLMul(12l, 0l)", value, (12l * 0l));
            value = el.exerciseLMul(1l, -13L);
            reportPassIf("exerciseLMul(1l, -13l)", value, (1l * -13l));

            value = el.exerciseLMul(5l, 1201l);
            reportPassIf("exerciseLMul(5l, 1201l)", value, (5l * 1201l));
        }
        {
            long value = el.exerciseLNeg(5l);
            reportPassIf("exerciseLNeg(5l)", value, -5l);
        }
        {
            long value1 = 0x55555555AAAAAAAAl;
            long value2 = 0x5555AAAA5555AAAAl;
            long value = el.exerciseLOr(value1, value2);
            reportPassIf("exerciseLOr(value1, value2)", value, (value1|value2));
        }
        {
            long value = el.exerciseLRem(2304l, 51l);
            reportPassIf("exerciseLRem(2304l, 51l)", value, (2304l % 51l));
        }
        {
            long value = el.exerciseLReturn(5l);
            reportPassIf("exerciseLReturn", value, 5l);
        }

        // Test LShl permutations:
        {
            long value1 = 0x5555AAAA5555AAAAl;
            long value = el.exerciseLShl(value1, 15);
            reportPassIf("exerciseLShl(value1, 15)", value, (value1 << 15l));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = -1;
            long value = el.exerciseLShl(value1, value2);
            reportPassIf("exerciseLShl(value1, " + value2 + ")",
                         value, (value1 << value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 0;
            long value = el.exerciseLShl(value1, value2);
            reportPassIf("exerciseLShl(value1, " + value2 + ")",
                         value, (value1 << value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 63;
            long value = el.exerciseLShl(value1, value2);
            reportPassIf("exerciseLShl(value1, " + value2 + ")",
                         value, (value1 << value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 64;
            long value = el.exerciseLShl(value1, value2);
            reportPassIf("exerciseLShl(value1, " + value2 + ")",
                         value, (value1 << value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 65;
            long value = el.exerciseLShl(value1, value2);
            reportPassIf("exerciseLShl(value1, " + value2 + ")",
                         value, (value1 << value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = -1;
            long value = el.exerciseLShlm1(value1);
            reportPassIf("exerciseLShlm1(value1)",
                         value, (value1 << value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 0;
            long value = el.exerciseLShl0(value1);
            reportPassIf("exerciseLShl0(value1)",
                         value, (value1 << value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 63;
            long value = el.exerciseLShl63(value1);
            reportPassIf("exerciseLShl63(value1)",
                         value, (value1 << value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 64;
            long value = el.exerciseLShl64(value1);
            reportPassIf("exerciseLShl64(value1)",
                         value, (value1 << value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 65;
            long value = el.exerciseLShl65(value1);
            reportPassIf("exerciseLShl65(value1)",
                         value, (value1 << value2));
        }

        // Test LShr permutations:
        {
            long value1 = 0xA555AAAA5555AAAAl;
            long value = el.exerciseLShr(value1, 15);
            reportPassIf("exerciseLShr(value1, 15)", value, (value1 >> 15l));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = -1;
            long value = el.exerciseLShr(value1, value2);
            reportPassIf("exerciseLShr(value1, " + value2 + ")",
                         value, (value1 >> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 0;
            long value = el.exerciseLShr(value1, value2);
            reportPassIf("exerciseLShr(value1, " + value2 + ")",
                         value, (value1 >> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 63;
            long value = el.exerciseLShr(value1, value2);
            reportPassIf("exerciseLShr(value1, " + value2 + ")",
                         value, (value1 >> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 64;
            long value = el.exerciseLShr(value1, value2);
            reportPassIf("exerciseLShr(value1, " + value2 + ")",
                         value, (value1 >> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 65;
            long value = el.exerciseLShr(value1, value2);
            reportPassIf("exerciseLShr(value1, " + value2 + ")",
                         value, (value1 >> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = -1;
            long value = el.exerciseLShrm1(value1);
            reportPassIf("exerciseLShrm1(value1)",
                         value, (value1 >> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 0;
            long value = el.exerciseLShr0(value1);
            reportPassIf("exerciseLShr0(value1)",
                         value, (value1 >> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 63;
            long value = el.exerciseLShr63(value1);
            reportPassIf("exerciseLShr63(value1)",
                         value, (value1 >> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 64;
            long value = el.exerciseLShr64(value1);
            reportPassIf("exerciseLShr64(value1)",
                         value, (value1 >> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 65;
            long value = el.exerciseLShr65(value1);
            reportPassIf("exerciseLShr65(value1)",
                         value, (value1 >> value2));
        }

        // Test LUShr permutations:
        {
            long value1 = 0xA555AAAA5555AAAAl;
            long value = el.exerciseLUShr(value1, 15);
            reportPassIf("exerciseLUShr(value1, 15)", value, (value1 >>> 15l));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = -1;
            long value = el.exerciseLUShr(value1, value2);
            reportPassIf("exerciseLUShr(value1, " + value2 + ")",
                         value, (value1 >>> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 0;
            long value = el.exerciseLUShr(value1, value2);
            reportPassIf("exerciseLUShr(value1, " + value2 + ")",
                         value, (value1 >>> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 63;
            long value = el.exerciseLUShr(value1, value2);
            reportPassIf("exerciseLUShr(value1, " + value2 + ")",
                         value, (value1 >>> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 64;
            long value = el.exerciseLUShr(value1, value2);
            reportPassIf("exerciseLUShr(value1, " + value2 + ")",
                         value, (value1 >>> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 65;
            long value = el.exerciseLUShr(value1, value2);
            reportPassIf("exerciseLUShr(value1, " + value2 + ")",
                         value, (value1 >>> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = -1;
            long value = el.exerciseLUShrm1(value1);
            reportPassIf("exerciseLUShrm1(value1)",
                         value, (value1 >>> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 0;
            long value = el.exerciseLUShr0(value1);
            reportPassIf("exerciseLUShr0(value1)",
                         value, (value1 >>> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 63;
            long value = el.exerciseLUShr63(value1);
            reportPassIf("exerciseLUShr63(value1)",
                         value, (value1 >>> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 64;
            long value = el.exerciseLUShr64(value1);
            reportPassIf("exerciseLUShr64(value1)",
                         value, (value1 >>> value2));
        }
        {
            long value1 = 0x5555AAAA5555AAAAl;
            int value2 = 65;
            long value = el.exerciseLUShr65(value1);
            reportPassIf("exerciseLUShr65(value1)",
                         value, (value1 >>> value2));
        }

        {
            long value = el.exerciseLStore(0, 0, 0, 0, 0l);
            reportPassIf("exerciseLStore(0, 0, 0, 0, 0l)", value, 1l);
        }
        {
            long value = el.exerciseLStore0(0l);
            reportPassIf("exerciseLStore0(0l)", value, 1l);
        }
        {
            long value = el.exerciseLStore1(0, 0l);
            reportPassIf("exerciseLStore1(0, 0l)", value, 1l);
        }
        {
            long value = el.exerciseLStore2(0, 0, 0l);
            reportPassIf("exerciseLStore2(0, 0, 0l)", value, 1l);
        }
        {
            long value = el.exerciseLStore3(0, 0, 0, 0l);
            reportPassIf("exerciseLStore3(0, 0, 0, 0l)", value, 1l);
        }
        {
            long value = el.exerciseLSub(2345l, 543l);
            reportPassIf("exerciseLSub(2345l, 543l)", value, (2345l - 543l));
        }
        {
            long value1 = 0x55555555AAAAAAAAl;
            long value2 = 0x5555AAAA5555AAAAl;
            long value = el.exerciseLXor(value1, value2);
            reportPassIf("exerciseLXor(value1, value2)", value, (value1 ^ value2));
        }
        if (verbose) {
            System.out.println("");
        }

        // Exercise the Float opcodes:
        System.out.println("Testing Float Opcodes:");
        ExerciseFloatOpcodes ef = new ExerciseFloatOpcodes();
        {
            double value;

            value = ef.exerciseF2D(5.0f);
            reportPassIf("exerciseF2D(5.0f)", value, 5.0d);
            value = ef.exerciseF2D(0.0f);
            reportPassIf("exerciseF2D(0.0f)", value, (double)(0.0f));
            value = ef.exerciseF2D(-0.0f);
            reportPassIf("exerciseF2D(-0.0f)", value, (double)(-0.0f));
            value = ef.exerciseF2D(467.24856f);
            reportPassIf("exerciseF2D(467.24856f)", value, (double)(467.24856f));
            value = ef.exerciseF2D(-467.24856f);
            reportPassIf("exerciseF2D(-467.24856f)", value, (double)(-467.24856f));
            value = ef.exerciseF2D(Float.NaN);
            reportPassIf("exerciseF2D(Float.NaN)", value, (double)(Float.NaN));
            value = ef.exerciseF2D(Float.POSITIVE_INFINITY);
            reportPassIf("exerciseF2D(Float.POSITIVE_INFINITY)", value, (double)(Float.POSITIVE_INFINITY));
            value = ef.exerciseF2D(Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseF2D(Float.NEGATIVE_INFINITY)", value, (double)(Float.NEGATIVE_INFINITY));
            value = ef.exerciseF2D(Float.MAX_VALUE);
            reportPassIf("exerciseF2D(Float.MAX_VALUE)", value, (double)(Float.MAX_VALUE));
            value = ef.exerciseF2D(-Float.MAX_VALUE);
            reportPassIf("exerciseF2D(-Float.MAX_VALUE)", value, (double)(-Float.MAX_VALUE));
            value = ef.exerciseF2D(Float.MIN_VALUE);
            reportPassIf("exerciseF2D(Float.MIN_VALUE)", value, (double)(Float.MIN_VALUE));
            value = ef.exerciseF2D(-Float.MIN_VALUE);
            reportPassIf("exerciseF2D(-Float.MIN_VALUE)", value, (double)(-Float.MIN_VALUE));
        }
        {
            int value = ef.exerciseF2I(5.0f);
            reportPassIf("exerciseF2I(5.0f)", value, 5);
        }
        {
            long value = ef.exerciseF2L(5.0f);
            reportPassIf("exerciseF2L(5.0f)", value, 5l);
        }
        {
            float value;
            {
                float f3 = (float)16777215;
                float f4 = 2.0f;
                float f5 = (float)33554430;
                float f6 = (float)16777216;
                boolean success;

                value = ef.exerciseFAdd(f3, f4);
                success = reportPassIf("exerciseFAdd(" + f3 + ", " + f4 + ")",
                                       value, (f3 + f4));
                if (!success) {
                    dumpValue(f3);
                    dumpValue(f4);
                }
                value = ef.exerciseFAdd(f3, f3);
                success = reportPassIf("exerciseFAdd(" + f3 + ", " + f3 + ")",
                                       value, (f3 + f3));
                if (!success) {
                    dumpValue(f3);
                    dumpValue(f4);
                }
            }

            for (int j = 0; j < 48; j++)
            {
                float start3 = 1e10f;
                float a3 = 1e20f;
                float c3 = 3f;
                float c4 = 4f;
                float r0, r1, r2, r3, r7;
                float x;

                x = start3;

                for (int i = 0; i < j; i++) {
                    r0 = x - a3;
                    r7 = x + a3;
                    r1 = c4 * x;
                    r2 = r0 / r1;
                    r3 = r2 * r7;
                    x = r3;
                }

                value = ef.exerciseFSub(x, a3);
                reportPassIf("exerciseFSub(" + x + ", " + a3 + ")",
                             value, (x - a3));
                value = ef.exerciseFAdd(x, a3);
                reportPassIf("exerciseFAdd(" + x + ", " + a3 + ")",
                             value, (x + a3));
            }
            {
                float float1 = (1L << 24) - 1; // Max mantissa.

                value = ef.exerciseFAdd(float1, 0.5f);
                reportPassIf("exerciseFAdd((1L << 24) - 1, 0.5f)", value, (float1 + 0.5f));
                value = float1 + 0.5f;

                float float2 = value;
                value = ef.exerciseFSub(float2, float1);
                reportPassIf("exerciseFSub( (((1L<<24)-1) + 0.5), 0.5f)", value, 1.0f);
            }

            value = ef.exerciseFAdd(1.0f, 1.0f);
            reportPassIf("exerciseFAdd(1.0f, 1.0f)", value, (1.0f + 1.0f));
            value = ef.exerciseFAdd(1.0f, -1.0f);
            reportPassIf("exerciseFAdd(1.0f, -1.0f)", value, (1.0f + (-1.0f)));
            value = ef.exerciseFAdd(-1.0f, 1.0f);
            reportPassIf("exerciseFAdd(-1.0f, 1.0f)", value, (-1.0f + 1.0f));
            value = ef.exerciseFAdd(-1.0f, -1.0f);
            reportPassIf("exerciseFAdd(-1.0f, -1.0f)", value, (-1.0f + (-1.0f)));

            value = ef.exerciseFAdd(10.0f, 1.0f);
            reportPassIf("exerciseFAdd(10.0f, 1.0f)", value, (10.0f + 1.0f));
            value = ef.exerciseFAdd(10.0f, -1.0f);
            reportPassIf("exerciseFAdd(10.0f, -1.0f)", value, (10.0f + (-1.0f)));
            value = ef.exerciseFAdd(-10.0f, 1.0f);
            reportPassIf("exerciseFAdd(-10.0f, 1.0f)", value, (-10.0f + 1.0f));
            value = ef.exerciseFAdd(-10.0f, -1.0f);
            reportPassIf("exerciseFAdd(-10.0f, -1.0f)", value, (-10.0f + (-1.0f)));

            value = ef.exerciseFAdd(1.0f, 10.0f);
            reportPassIf("exerciseFAdd(1.0f, 10.0f)", value, (1.0f + 10.0f));
            value = ef.exerciseFAdd(1.0f, -10.0f);
            reportPassIf("exerciseFAdd(1.0f, -10.0f)", value, (1.0f + (-10.0f)));
            value = ef.exerciseFAdd(-1.0f, 10.0f);
            reportPassIf("exerciseFAdd(-1.0f, 10.0f)", value, (-1.0f + 10.0f));
            value = ef.exerciseFAdd(-1.0f, -10.0f);
            reportPassIf("exerciseFAdd(-1.0f, -10.0f)", value, (-1.0f + (-10.0f)));

            value = ef.exerciseFAdd(10.0f, 10.0f);
            reportPassIf("exerciseFAdd(10.0f, 10.0f)", value, (10.0f + 10.0f));
            value = ef.exerciseFAdd(10.0f, -10.0f);
            reportPassIf("exerciseFAdd(10.0f, -10.0f)", value, (10.0f + (-10.0f)));
            value = ef.exerciseFAdd(-10.0f, 10.0f);
            reportPassIf("exerciseFAdd(-10.0f, 10.0f)", value, (-10.0f + 10.0f));
            value = ef.exerciseFAdd(-10.0f, -10.0f);
            reportPassIf("exerciseFAdd(-10.0f, -10.0f)", value, (-10.0f + (-10.0f)));

            value = ef.exerciseFAdd(0.0f, 0.0f);
            reportPassIf("exerciseFAdd(0.0f, 0.0f)", value, (0.0f + 0.0f));
            value = ef.exerciseFAdd(0.0f, -0.0f);
            reportPassIf("exerciseFAdd(0.0f, -0.0f)", value, (0.0f + (-0.0f)));
            value = ef.exerciseFAdd(0.0f, 467.24856f);
            reportPassIf("exerciseFAdd(0.0f, 467.24856f)", value, (0.0f + 467.24856f));
            value = ef.exerciseFAdd(0.0f, -467.24856f);
            reportPassIf("exerciseFAdd(0.0f, -467.24856f)", value, (0.0f + (-467.24856f)));
            value = ef.exerciseFAdd(0.0f, Float.NaN);
            reportPassIf("exerciseFAdd(0.0f, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(0.0f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(0.0f, Float.POSITIVE_INFINITY)", value, (0.0f + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(0.0f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(0.0f, Float.NEGATIVE_INFINITY)", value, (0.0f + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(0.0f, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(0.0f, Float.MAX_VALUE)", value, (0.0f + Float.MAX_VALUE));
            value = ef.exerciseFAdd(0.0f, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(0.0f, -Float.MAX_VALUE)", value, (0.0f + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(0.0f, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(0.0f, Float.MIN_VALUE)", value, (0.0f + Float.MIN_VALUE));
            value = ef.exerciseFAdd(0.0f, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(0.0f, -Float.MIN_VALUE)", value, (0.0f + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(-0.0f, 0.0f);
            reportPassIf("exerciseFAdd(-0.0f, 0.0f)", value, (-0.0f + 0.0f));
            value = ef.exerciseFAdd(-0.0f, -0.0f);
            reportPassIf("exerciseFAdd(-0.0f, -0.0f)", value, (-0.0f + (-0.0f)));
            value = ef.exerciseFAdd(-0.0f, 467.24856f);
            reportPassIf("exerciseFAdd(-0.0f, 467.24856f)", value, (-0.0f + 467.24856f));
            value = ef.exerciseFAdd(-0.0f, -467.24856f);
            reportPassIf("exerciseFAdd(-0.0f, -467.24856f)", value, (-0.0f + (-467.24856f)));
            value = ef.exerciseFAdd(-0.0f, Float.NaN);
            reportPassIf("exerciseFAdd(-0.0f, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(-0.0f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(-0.0f, Float.POSITIVE_INFINITY)", value, (-0.0f + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(-0.0f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(-0.0f, Float.NEGATIVE_INFINITY)", value, (-0.0f + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(-0.0f, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(-0.0f, Float.MAX_VALUE)", value, (-0.0f + Float.MAX_VALUE));
            value = ef.exerciseFAdd(-0.0f, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(-0.0f, -Float.MAX_VALUE)", value, (-0.0f + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(-0.0f, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(-0.0f, Float.MIN_VALUE)", value, (-0.0f + Float.MIN_VALUE));
            value = ef.exerciseFAdd(-0.0f, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(-0.0f, -Float.MIN_VALUE)", value, (-0.0f + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(32456.5241f, 0.0f);
            reportPassIf("exerciseFAdd(32456.5241f, 0.0f)", value, (32456.5241f + 0.0f));
            value = ef.exerciseFAdd(32456.5241f, -0.0f);
            reportPassIf("exerciseFAdd(32456.5241f, -0.0f)", value, (32456.5241f + (-0.0f)));
            value = ef.exerciseFAdd(32456.5241f, 467.24856f);
            reportPassIf("exerciseFAdd(32456.5241f, 467.24856f)", value, (32456.5241f + 467.24856f));
            value = ef.exerciseFAdd(32456.5241f, -467.24856f);
            reportPassIf("exerciseFAdd(32456.5241f, -467.24856f)", value, (32456.5241f + (-467.24856f)));
            value = ef.exerciseFAdd(32456.5241f, Float.NaN);
            reportPassIf("exerciseFAdd(32456.5241f, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(32456.5241f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(32456.5241f, Float.POSITIVE_INFINITY)", value, (32456.5241f + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(32456.5241f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(32456.5241f, Float.NEGATIVE_INFINITY)", value, (32456.5241f + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(32456.5241f, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(32456.5241f, Float.MAX_VALUE)", value, (32456.5241f + Float.MAX_VALUE));
            value = ef.exerciseFAdd(32456.5241f, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(32456.5241f, -Float.MAX_VALUE)", value, (32456.5241f + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(32456.5241f, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(32456.5241f, Float.MIN_VALUE)", value, (32456.5241f + Float.MIN_VALUE));
            value = ef.exerciseFAdd(32456.5241f, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(32456.5241f, -Float.MIN_VALUE)", value, (32456.5241f + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(-32456.5241f, 0.0f);
            reportPassIf("exerciseFAdd(-32456.5241f, 0.0f)", value, (-32456.5241f + 0.0f));
            value = ef.exerciseFAdd(-32456.5241f, -0.0f);
            reportPassIf("exerciseFAdd(-32456.5241f, -0.0f)", value, (-32456.5241f + (-0.0f)));
            value = ef.exerciseFAdd(-32456.5241f, 467.24856f);
            reportPassIf("exerciseFAdd(-32456.5241f, 467.24856f)", value, (-32456.5241f + 467.24856f));
            value = ef.exerciseFAdd(-32456.5241f, -467.24856f);
            reportPassIf("exerciseFAdd(-32456.5241f, -467.24856f)", value, (-32456.5241f + (-467.24856f)));
            value = ef.exerciseFAdd(-32456.5241f, Float.NaN);
            reportPassIf("exerciseFAdd(-32456.5241f, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(-32456.5241f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(-32456.5241f, Float.POSITIVE_INFINITY)", value, (-32456.5241f + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(-32456.5241f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(-32456.5241f, Float.NEGATIVE_INFINITY)", value, (-32456.5241f + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(-32456.5241f, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(-32456.5241f, Float.MAX_VALUE)", value, (-32456.5241f + Float.MAX_VALUE));
            value = ef.exerciseFAdd(-32456.5241f, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(-32456.5241f, -Float.MAX_VALUE)", value, (-32456.5241f + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(-32456.5241f, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(-32456.5241f, Float.MIN_VALUE)", value, (-32456.5241f + Float.MIN_VALUE));
            value = ef.exerciseFAdd(-32456.5241f, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(-32456.5241f, -Float.MIN_VALUE)", value, (-32456.5241f + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(467.24856f, 0.0f);
            reportPassIf("exerciseFAdd(467.24856f, 0.0f)", value, (467.24856f + 0.0f));
            value = ef.exerciseFAdd(467.24856f, -0.0f);
            reportPassIf("exerciseFAdd(467.24856f, -0.0f)", value, (467.24856f + (-0.0f)));
            value = ef.exerciseFAdd(467.24856f, 32456.5241f);
            reportPassIf("exerciseFAdd(467.24856f, 32456.5241f)", value, (467.24856f + 32456.5241f));
            value = ef.exerciseFAdd(467.24856f, -32456.5241f);
            reportPassIf("exerciseFAdd(467.24856f, -32456.5241f)", value, (467.24856f + (-32456.5241f)));
            value = ef.exerciseFAdd(467.24856f, Float.NaN);
            reportPassIf("exerciseFAdd(467.24856f, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(467.24856f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(467.24856f, Float.POSITIVE_INFINITY)", value, (467.24856f + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(467.24856f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(32456.5241f, Float.NEGATIVE_INFINITY)", value, (467.24856f + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(467.24856f, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(467.24856f, Float.MAX_VALUE)", value, (467.24856f + Float.MAX_VALUE));
            value = ef.exerciseFAdd(467.24856f, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(467.24856f, -Float.MAX_VALUE)", value, (467.24856f + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(467.24856f, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(467.24856f, Float.MIN_VALUE)", value, (467.24856f + Float.MIN_VALUE));
            value = ef.exerciseFAdd(467.24856f, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(467.24856f, -Float.MIN_VALUE)", value, (467.24856f + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(-467.24856f, 0.0f);
            reportPassIf("exerciseFAdd(-467.24856f, 0.0f)", value, (-467.24856f + 0.0f));
            value = ef.exerciseFAdd(-467.24856f, -0.0f);
            reportPassIf("exerciseFAdd(-467.24856f, -0.0f)", value, (-467.24856f + (-0.0f)));
            value = ef.exerciseFAdd(-467.24856f, 32456.5241f);
            reportPassIf("exerciseFAdd(-467.24856f, 32456.5241f)", value, (-467.24856f + 32456.5241f));
            value = ef.exerciseFAdd(-467.24856f, -32456.5241f);
            reportPassIf("exerciseFAdd(-467.24856f, -32456.5241f)", value, (-467.24856f + (-32456.5241f)));
            value = ef.exerciseFAdd(-467.24856f, Float.NaN);
            reportPassIf("exerciseFAdd(-467.24856f, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(-467.24856f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(-467.24856f, Float.POSITIVE_INFINITY)", value, (-467.24856f + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(-467.24856f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(-467.24856f, Float.NEGATIVE_INFINITY)", value, (-467.24856f + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(-467.24856f, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(-467.24856f, Float.MAX_VALUE)", value, (-467.24856f + Float.MAX_VALUE));
            value = ef.exerciseFAdd(-467.24856f, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(-467.24856f, -Float.MAX_VALUE)", value, (-467.24856f + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(-467.24856f, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(-467.24856f, Float.MIN_VALUE)", value, (-467.24856f + Float.MIN_VALUE));
            value = ef.exerciseFAdd(-467.24856f, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(-467.24856f, -Float.MIN_VALUE)", value, (-467.24856f + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(Float.NaN, 0.0f);
            reportPassIf("exerciseFAdd(Float.NaN, 0.0f)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NaN, -0.0f);
            reportPassIf("exerciseFAdd(Float.NaN, -0.0f)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NaN, 467.24856f);
            reportPassIf("exerciseFAdd(Float.NaN, 467.24856f)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NaN, -467.24856f);
            reportPassIf("exerciseFAdd(Float.NaN, -467.24856f)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NaN, Float.NaN);
            reportPassIf("exerciseFAdd(Float.NaN, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NaN, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(Float.NaN, Float.POSITIVE_INFINITY)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NaN, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(Float.NaN, Float.NEGATIVE_INFINITY)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NaN, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(Float.NaN, Float.MAX_VALUE)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NaN, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(Float.NaN, -Float.MAX_VALUE)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NaN, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(Float.NaN, Float.MIN_VALUE)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NaN, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(Float.NaN, -Float.MIN_VALUE)", Float.isNaN(value));

            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, 0.0f);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, 0.0f)", value, (Float.POSITIVE_INFINITY + 0.0f));
            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, -0.0f);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, -0.0f)", value, (Float.POSITIVE_INFINITY + (-0.0f)));
            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, 467.24856f);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, 467.24856f)", value, (Float.POSITIVE_INFINITY + 467.24856f));
            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, -467.24856f);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, -467.24856f)", value, (Float.POSITIVE_INFINITY + (-467.24856f)));
            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY)", value, (Float.POSITIVE_INFINITY + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY)", value, (Float.POSITIVE_INFINITY + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, Float.MAX_VALUE)", value, (Float.POSITIVE_INFINITY + Float.MAX_VALUE));
            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, -Float.MAX_VALUE)", value, (Float.POSITIVE_INFINITY + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, Float.MIN_VALUE)", value, (Float.POSITIVE_INFINITY + Float.MIN_VALUE));
            value = ef.exerciseFAdd(Float.POSITIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(Float.POSITIVE_INFINITY, -Float.MIN_VALUE)", value, (Float.POSITIVE_INFINITY + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, 0.0f);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, 0.0f)", value, (Float.NEGATIVE_INFINITY + 0.0f));
            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, -0.0f);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, -0.0f)", value, (Float.NEGATIVE_INFINITY + (-0.0f)));
            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, 467.24856f);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, 467.24856f)", value, (Float.NEGATIVE_INFINITY + 467.24856f));
            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, -467.24856f);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, -467.24856f)", value, (Float.NEGATIVE_INFINITY + (-467.24856f)));
            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY)", value, (Float.NEGATIVE_INFINITY + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY)", value, (Float.NEGATIVE_INFINITY + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, Float.MAX_VALUE)", value, (Float.NEGATIVE_INFINITY + Float.MAX_VALUE));
            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE)", value, (Float.NEGATIVE_INFINITY + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, Float.MIN_VALUE)", value, (Float.NEGATIVE_INFINITY + Float.MIN_VALUE));
            value = ef.exerciseFAdd(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE)", value, (Float.NEGATIVE_INFINITY + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(Float.MAX_VALUE, 0.0f);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, 0.0f)", value, (Float.MAX_VALUE + 0.0f));
            value = ef.exerciseFAdd(Float.MAX_VALUE, -0.0f);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, -0.0f)", value, (Float.MAX_VALUE + (-0.0f)));
            value = ef.exerciseFAdd(Float.MAX_VALUE, 467.24856f);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, 467.24856f)", value, (Float.MAX_VALUE + 467.24856f));
            value = ef.exerciseFAdd(Float.MAX_VALUE, -467.24856f);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, -467.24856f)", value, (Float.MAX_VALUE + (-467.24856f)));
            value = ef.exerciseFAdd(Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, Float.POSITIVE_INFINITY)", value, (Float.MAX_VALUE + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, Float.NEGATIVE_INFINITY)", value, (Float.MAX_VALUE + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, Float.MAX_VALUE)", value, (Float.MAX_VALUE + Float.MAX_VALUE));
            value = ef.exerciseFAdd(Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, -Float.MAX_VALUE)", value, (Float.MAX_VALUE + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, Float.MIN_VALUE)", value, (Float.MAX_VALUE + Float.MIN_VALUE));
            value = ef.exerciseFAdd(Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(Float.MAX_VALUE, -Float.MIN_VALUE)", value, (Float.MAX_VALUE + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(-Float.MAX_VALUE, 0.0f);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, 0.0f)", value, (-Float.MAX_VALUE + 0.0f));
            value = ef.exerciseFAdd(-Float.MAX_VALUE, -0.0f);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, -0.0f)", value, (-Float.MAX_VALUE + (-0.0f)));
            value = ef.exerciseFAdd(-Float.MAX_VALUE, 467.24856f);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, 467.24856f)", value, (-Float.MAX_VALUE + 467.24856f));
            value = ef.exerciseFAdd(-Float.MAX_VALUE, -467.24856f);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, -467.24856f)", value, (-Float.MAX_VALUE + (-467.24856f)));
            value = ef.exerciseFAdd(-Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(-Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, Float.POSITIVE_INFINITY)", value, (-Float.MAX_VALUE + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY)", value, (-Float.MAX_VALUE + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(-Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, Float.MAX_VALUE)", value, (-Float.MAX_VALUE + Float.MAX_VALUE));
            value = ef.exerciseFAdd(-Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, -Float.MAX_VALUE)", value, (-Float.MAX_VALUE + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(-Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, Float.MIN_VALUE)", value, (-Float.MAX_VALUE + Float.MIN_VALUE));
            value = ef.exerciseFAdd(-Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(-Float.MAX_VALUE, -Float.MIN_VALUE)", value, (-Float.MAX_VALUE + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(Float.MIN_VALUE, 0.0f);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, 0.0f)", value, (Float.MIN_VALUE + 0.0f));
            value = ef.exerciseFAdd(Float.MIN_VALUE, -0.0f);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, -0.0f)", value, (Float.MIN_VALUE + (-0.0f)));
            value = ef.exerciseFAdd(Float.MIN_VALUE, 467.24856f);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, 467.24856f)", value, (Float.MIN_VALUE + 467.24856f));
            value = ef.exerciseFAdd(Float.MIN_VALUE, -467.24856f);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, -467.24856f)", value, (Float.MIN_VALUE + (-467.24856f)));
            value = ef.exerciseFAdd(Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, Float.POSITIVE_INFINITY)", value, (Float.MIN_VALUE + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, Float.NEGATIVE_INFINITY)", value, (Float.MIN_VALUE + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, Float.MAX_VALUE)", value, (Float.MIN_VALUE + Float.MAX_VALUE));
            value = ef.exerciseFAdd(Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, -Float.MAX_VALUE)", value, (Float.MIN_VALUE + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, Float.MIN_VALUE)", value, (Float.MIN_VALUE + Float.MIN_VALUE));
            value = ef.exerciseFAdd(Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(Float.MIN_VALUE, -Float.MIN_VALUE)", value, (Float.MIN_VALUE + (-Float.MIN_VALUE)));

            value = ef.exerciseFAdd(-Float.MIN_VALUE, 0.0f);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, 0.0f)", value, (-Float.MIN_VALUE + 0.0f));
            value = ef.exerciseFAdd(-Float.MIN_VALUE, -0.0f);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, -0.0f)", value, (-Float.MIN_VALUE + (-0.0f)));
            value = ef.exerciseFAdd(-Float.MIN_VALUE, 467.24856f);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, 467.24856f)", value, (-Float.MIN_VALUE + 467.24856f));
            value = ef.exerciseFAdd(-Float.MIN_VALUE, -467.24856f);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, -467.24856f)", value, (-Float.MIN_VALUE + (-467.24856f)));
            value = ef.exerciseFAdd(-Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, Float.NaN)", Float.isNaN(value));
            value = ef.exerciseFAdd(-Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, Float.POSITIVE_INFINITY)", value, (-Float.MIN_VALUE + Float.POSITIVE_INFINITY));
            value = ef.exerciseFAdd(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY)", value, (-Float.MIN_VALUE + Float.NEGATIVE_INFINITY));
            value = ef.exerciseFAdd(-Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, Float.MAX_VALUE)", value, (-Float.MIN_VALUE + Float.MAX_VALUE));
            value = ef.exerciseFAdd(-Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, -Float.MAX_VALUE)", value, (-Float.MIN_VALUE + (-Float.MAX_VALUE)));
            value = ef.exerciseFAdd(-Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, Float.MIN_VALUE)", value, (-Float.MIN_VALUE + Float.MIN_VALUE));
            value = ef.exerciseFAdd(-Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFAdd(-Float.MIN_VALUE, -Float.MIN_VALUE)", value, (-Float.MIN_VALUE + (-Float.MIN_VALUE)));
        }
        {
            float[] fa = new float[5];
            fa[3] = 5.0f;
            float value = ef.exerciseFALoad(fa, 3);
            reportPassIf("exerciseFALoad(fa, 3)", value, 5.0f);
        }
        {
            float[] fa = new float[5];
            fa[3] = 0.0f;
            ef.exerciseFAStore(fa, 3, 5.0f);
            reportPassIf("exerciseFAStore(fa, 3, 5.0f)", fa[3], 5.0f);
        }
        {
            boolean value;

            value = ef.exerciseFCmp_EQ(0.0f, -0.0f);
            reportPassIf("exerciseFCmp_EQ(0.0f, -0.0f)", value, (0.0f == -0.0f));
            value = ef.exerciseFCmp_EQ(-0.0f, 0.0f);
            reportPassIf("exerciseFCmp_EQ(-0.0f, 0.0f)", value, (-0.0f == 0.0f));
            value = ef.exerciseFCmp_EQ(5.0f, 5.0f);
            reportPassIf("exerciseFCmp_EQ(5.0f, 5.0f)", value, (5.0f == 5.0f));
            value = ef.exerciseFCmp_EQ(-5.0f, 5.0f);
            reportPassIf("exerciseFCmp_EQ(-5.0f, 5.0f)", value, (-5.0f == 5.0f));
            value = ef.exerciseFCmp_EQ(Float.NaN, 5.0f);
            reportPassIf("exerciseFCmp_EQ(Float.NaN, 5.0f)", value, (Float.NaN == 5.0f));
            value = ef.exerciseFCmp_EQ(5.0f, Float.NaN);
            reportPassIf("exerciseFCmp_EQ(5.0f, Float.NaN)", value, (5.0f == Float.NaN));
            value = ef.exerciseFCmp_EQ(Float.NaN, Float.NaN);
            reportPassIf("exerciseFCmp_EQ(Float.NaN, Float.NaN)", value, (Float.NaN == Float.NaN));
        }
        {
            boolean value;

            value = ef.exerciseFCmp_NE(0.0f, -0.0f);
            reportPassIf("exerciseFCmp_NE(0.0f, -0.0f)", value, (0.0f != -0.0f));
            value = ef.exerciseFCmp_NE(-0.0f, 0.0f);
            reportPassIf("exerciseFCmp_NE(-0.0f, 0.0f)", value, (-0.0f != 0.0f));
            value = ef.exerciseFCmp_NE(5.0f, 5.0f);
            reportPassIf("exerciseFCmp_NE(5.0f, 5.0f)", value, (5.0f != 5.0f));
            value = ef.exerciseFCmp_NE(-5.0f, 5.0f);
            reportPassIf("exerciseFCmp_NE(-5.0f, 5.0f)", value, (-5.0f != 5.0f));
            value = ef.exerciseFCmp_NE(Float.NaN, 5.0f);
            reportPassIf("exerciseFCmp_NE(Float.NaN, 5.0f)", value, (Float.NaN != 5.0f));
            value = ef.exerciseFCmp_NE(5.0f, Float.NaN);
            reportPassIf("exerciseFCmp_NE(5.0f, Float.NaN)", value, (5.0f != Float.NaN));
            value = ef.exerciseFCmp_NE(Float.NaN, Float.NaN);
            reportPassIf("exerciseFCmp_NE(Float.NaN, Float.NaN)", value, (Float.NaN != Float.NaN));
        }
        {
            boolean value;

            value = ef.exerciseFCmp_GT(0.0f, -0.0f);
            reportPassIf("exerciseFCmp_GT(0.0f, -0.0f)", value, (0.0f > -0.0f));
            value = ef.exerciseFCmp_GT(-0.0f, 0.0f);
            reportPassIf("exerciseFCmp_GT(-0.0f, 0.0f)", value, (-0.0f > 0.0f));
            value = ef.exerciseFCmp_GT(5.0f, -5.0f);
            reportPassIf("exerciseFCmp_GT(5.0f, -5.0f)", value, (5.0f > -5.0f));
            value = ef.exerciseFCmp_GT(5.0f, 5.0f);
            reportPassIf("exerciseFCmp_GT(5.0f, 5.0f)", value, (5.0f > 5.0f));
            value = ef.exerciseFCmp_GT(-5.0f, 5.0f);
            reportPassIf("exerciseFCmp_GT(-5.0f, 5.0f)", value, (-5.0f > 5.0f));
            value = ef.exerciseFCmp_GT(Float.NaN, 5.0f);
            reportPassIf("exerciseFCmp_GT(Float.NaN, 5.0f)", value, (Float.NaN > 5.0f));
            value = ef.exerciseFCmp_GT(5.0f, Float.NaN);
            reportPassIf("exerciseFCmp_GT(5.0f, Float.NaN)", value, (5.0f > Float.NaN));
            value = ef.exerciseFCmp_GT(Float.NaN, Float.NaN);
            reportPassIf("exerciseFCmp_GT(Float.NaN, Float.NaN)", value, (Float.NaN > Float.NaN));

            value = ef.exerciseFCmp_GT(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY)",
                         value, (Float.POSITIVE_INFINITY > Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GT(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY)",
                         value, (Float.POSITIVE_INFINITY > Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GT(Float.POSITIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFCmp_GT(Float.POSITIVE_INFINITY, Float.NaN)",
                         value, (Float.POSITIVE_INFINITY > Float.NaN));
            value = ef.exerciseFCmp_GT(Float.POSITIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.POSITIVE_INFINITY, Float.MAX_VALUE)",
                         value, (Float.POSITIVE_INFINITY > Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(Float.POSITIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.POSITIVE_INFINITY, -Float.MAX_VALUE)",
                         value, (Float.POSITIVE_INFINITY > -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(Float.POSITIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.POSITIVE_INFINITY, Float.MIN_VALUE)",
                         value, (Float.POSITIVE_INFINITY > Float.MIN_VALUE));
            value = ef.exerciseFCmp_GT(Float.POSITIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.POSITIVE_INFINITY, -Float.MIN_VALUE)",
                         value, (Float.POSITIVE_INFINITY > -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GT(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY)",
                         value, (Float.NEGATIVE_INFINITY > Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GT(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY)",
                         value, (Float.NEGATIVE_INFINITY > Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GT(Float.NEGATIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFCmp_GT(Float.NEGATIVE_INFINITY, Float.NaN)",
                         value, (Float.NEGATIVE_INFINITY > Float.NaN));
            value = ef.exerciseFCmp_GT(Float.NEGATIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.NEGATIVE_INFINITY, Float.MAX_VALUE)",
                         value, (Float.NEGATIVE_INFINITY > Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE)",
                         value, (Float.NEGATIVE_INFINITY > -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(Float.NEGATIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.NEGATIVE_INFINITY, Float.MIN_VALUE)",
                         value, (Float.NEGATIVE_INFINITY > Float.MIN_VALUE));
            value = ef.exerciseFCmp_GT(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE)",
                         value, (Float.NEGATIVE_INFINITY > -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GT(Float.NaN, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(Float.NaN, Float.POSITIVE_INFINITY)",
                         value, (Float.NaN > Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GT(Float.NaN, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(Float.NaN, Float.NEGATIVE_INFINITY)",
                         value, (Float.NaN > Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GT(Float.NaN, Float.NaN);
            reportPassIf("exerciseFCmp_GT(Float.NaN, Float.NaN)",
                         value, (Float.NaN > Float.NaN));
            value = ef.exerciseFCmp_GT(Float.NaN, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.NaN, Float.MAX_VALUE)",
                         value, (Float.NaN > Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(Float.NaN, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.NaN, -Float.MAX_VALUE)",
                         value, (Float.NaN > -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(Float.NaN, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.NaN, Float.MIN_VALUE)",
                         value, (Float.NaN > Float.MIN_VALUE));
            value = ef.exerciseFCmp_GT(Float.NaN, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.NaN, -Float.MIN_VALUE)",
                         value, (Float.NaN > -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GT(Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(Float.MAX_VALUE, Float.POSITIVE_INFINITY)",
                         value, (Float.MAX_VALUE > Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GT(Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(Float.MAX_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (Float.MAX_VALUE > Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GT(Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_GT(Float.MAX_VALUE, Float.NaN)",
                         value, (Float.MAX_VALUE > Float.NaN));
            value = ef.exerciseFCmp_GT(Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.MAX_VALUE, Float.MAX_VALUE)",
                         value, (Float.MAX_VALUE > Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.MAX_VALUE, -Float.MAX_VALUE)",
                         value, (Float.MAX_VALUE > -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.MAX_VALUE, Float.MIN_VALUE)",
                         value, (Float.MAX_VALUE > Float.MIN_VALUE));
            value = ef.exerciseFCmp_GT(Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.MAX_VALUE, -Float.MIN_VALUE)",
                         value, (Float.MAX_VALUE > -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GT(-Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(-Float.MAX_VALUE, Float.POSITIVE_INFINITY)",
                         value, (-Float.MAX_VALUE > Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GT(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (-Float.MAX_VALUE > Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GT(-Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_GT(-Float.MAX_VALUE, Float.NaN)",
                         value, (-Float.MAX_VALUE > Float.NaN));
            value = ef.exerciseFCmp_GT(-Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(-Float.MAX_VALUE, Float.MAX_VALUE)",
                         value, (-Float.MAX_VALUE > Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(-Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(-Float.MAX_VALUE, -Float.MAX_VALUE)",
                         value, (-Float.MAX_VALUE > -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(-Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(-Float.MAX_VALUE, Float.MIN_VALUE)",
                         value, (-Float.MAX_VALUE > Float.MIN_VALUE));
            value = ef.exerciseFCmp_GT(-Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(-Float.MAX_VALUE, -Float.MIN_VALUE)",
                         value, (-Float.MAX_VALUE > -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GT(Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(Float.MIN_VALUE, Float.POSITIVE_INFINITY)",
                         value, (Float.MIN_VALUE > Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GT(Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(Float.MIN_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (Float.MIN_VALUE > Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GT(Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_GT(Float.MIN_VALUE, Float.NaN)",
                         value, (Float.MIN_VALUE > Float.NaN));
            value = ef.exerciseFCmp_GT(Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.MIN_VALUE, Float.MAX_VALUE)",
                         value, (Float.MIN_VALUE > Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.MIN_VALUE, -Float.MAX_VALUE)",
                         value, (Float.MIN_VALUE > -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.MIN_VALUE, Float.MIN_VALUE)",
                         value, (Float.MIN_VALUE > Float.MIN_VALUE));
            value = ef.exerciseFCmp_GT(Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(Float.MIN_VALUE, -Float.MIN_VALUE)",
                         value, (Float.MIN_VALUE > -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GT(-Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(-Float.MIN_VALUE, Float.POSITIVE_INFINITY)",
                         value, (-Float.MIN_VALUE > Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GT(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GT(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (-Float.MIN_VALUE > Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GT(-Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_GT(-Float.MIN_VALUE, Float.NaN)",
                         value, (-Float.MIN_VALUE > Float.NaN));
            value = ef.exerciseFCmp_GT(-Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(-Float.MIN_VALUE, Float.MAX_VALUE)",
                         value, (-Float.MIN_VALUE > Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(-Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GT(-Float.MIN_VALUE, -Float.MAX_VALUE)",
                         value, (-Float.MIN_VALUE > -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GT(-Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(-Float.MIN_VALUE, Float.MIN_VALUE)",
                         value, (-Float.MIN_VALUE > Float.MIN_VALUE));
            value = ef.exerciseFCmp_GT(-Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GT(-Float.MIN_VALUE, -Float.MIN_VALUE)",
                         value, (-Float.MIN_VALUE > -Float.MIN_VALUE));
        }
        {
            boolean value;

            value = ef.exerciseFCmp_GE(0.0f, -0.0f);
            reportPassIf("exerciseFCmp_GE(0.0f, -0.0f)", value, (0.0f >= -0.0f));
            value = ef.exerciseFCmp_GE(-0.0f, 0.0f);
            reportPassIf("exerciseFCmp_GE(-0.0f, 0.0f)", value, (-0.0f >= 0.0f));
            value = ef.exerciseFCmp_GE(5.0f, -5.0f);
            reportPassIf("exerciseFCmp_GE(5.0f, -5.0f)", value, (5.0f >= -5.0f));
            value = ef.exerciseFCmp_GE(5.0f, 5.0f);
            reportPassIf("exerciseFCmp_GE(5.0f, 5.0f)", value, (5.0f >= 5.0f));
            value = ef.exerciseFCmp_GE(-5.0f, 5.0f);
            reportPassIf("exerciseFCmp_GE(-5.0f, 5.0f)", value, (-5.0f >= 5.0f));
            value = ef.exerciseFCmp_GE(Float.NaN, 5.0f);
            reportPassIf("exerciseFCmp_GE(Float.NaN, 5.0f)", value, (Float.NaN >= 5.0f));
            value = ef.exerciseFCmp_GE(5.0f, Float.NaN);
            reportPassIf("exerciseFCmp_GE(5.0f, Float.NaN)", value, (5.0f >= Float.NaN));
            value = ef.exerciseFCmp_GE(Float.NaN, Float.NaN);
            reportPassIf("exerciseFCmp_GE(Float.NaN, Float.NaN)", value, (Float.NaN >= Float.NaN));

            value = ef.exerciseFCmp_GE(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY)",
                         value, (Float.POSITIVE_INFINITY >= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GE(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY)",
                         value, (Float.POSITIVE_INFINITY >= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GE(Float.POSITIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFCmp_GE(Float.POSITIVE_INFINITY, Float.NaN)",
                         value, (Float.POSITIVE_INFINITY >= Float.NaN));
            value = ef.exerciseFCmp_GE(Float.POSITIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.POSITIVE_INFINITY, Float.MAX_VALUE)",
                         value, (Float.POSITIVE_INFINITY >= Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(Float.POSITIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.POSITIVE_INFINITY, -Float.MAX_VALUE)",
                         value, (Float.POSITIVE_INFINITY >= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(Float.POSITIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.POSITIVE_INFINITY, Float.MIN_VALUE)",
                         value, (Float.POSITIVE_INFINITY >= Float.MIN_VALUE));
            value = ef.exerciseFCmp_GE(Float.POSITIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.POSITIVE_INFINITY, -Float.MIN_VALUE)",
                         value, (Float.POSITIVE_INFINITY >= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GE(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY)",
                         value, (Float.NEGATIVE_INFINITY >= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GE(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY)",
                         value, (Float.NEGATIVE_INFINITY >= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GE(Float.NEGATIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFCmp_GE(Float.NEGATIVE_INFINITY, Float.NaN)",
                         value, (Float.NEGATIVE_INFINITY >= Float.NaN));
            value = ef.exerciseFCmp_GE(Float.NEGATIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.NEGATIVE_INFINITY, Float.MAX_VALUE)",
                         value, (Float.NEGATIVE_INFINITY >= Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE)",
                         value, (Float.NEGATIVE_INFINITY >= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(Float.NEGATIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.NEGATIVE_INFINITY, Float.MIN_VALUE)",
                         value, (Float.NEGATIVE_INFINITY >= Float.MIN_VALUE));
            value = ef.exerciseFCmp_GE(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE)",
                         value, (Float.NEGATIVE_INFINITY >= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GE(Float.NaN, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(Float.NaN, Float.POSITIVE_INFINITY)",
                         value, (Float.NaN >= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GE(Float.NaN, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(Float.NaN, Float.NEGATIVE_INFINITY)",
                         value, (Float.NaN >= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GE(Float.NaN, Float.NaN);
            reportPassIf("exerciseFCmp_GE(Float.NaN, Float.NaN)",
                         value, (Float.NaN >= Float.NaN));
            value = ef.exerciseFCmp_GE(Float.NaN, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.NaN, Float.MAX_VALUE)",
                         value, (Float.NaN >= Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(Float.NaN, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.NaN, -Float.MAX_VALUE)",
                         value, (Float.NaN >= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(Float.NaN, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.NaN, Float.MIN_VALUE)",
                         value, (Float.NaN >= Float.MIN_VALUE));
            value = ef.exerciseFCmp_GE(Float.NaN, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.NaN, -Float.MIN_VALUE)",
                         value, (Float.NaN >= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GE(Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(Float.MAX_VALUE, Float.POSITIVE_INFINITY)",
                         value, (Float.MAX_VALUE >= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GE(Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(Float.MAX_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (Float.MAX_VALUE >= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GE(Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_GE(Float.MAX_VALUE, Float.NaN)",
                         value, (Float.MAX_VALUE >= Float.NaN));
            value = ef.exerciseFCmp_GE(Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.MAX_VALUE, Float.MAX_VALUE)",
                         value, (Float.MAX_VALUE >= Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.MAX_VALUE, -Float.MAX_VALUE)",
                         value, (Float.MAX_VALUE >= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.MAX_VALUE, Float.MIN_VALUE)",
                         value, (Float.MAX_VALUE >= Float.MIN_VALUE));
            value = ef.exerciseFCmp_GE(Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.MAX_VALUE, -Float.MIN_VALUE)",
                         value, (Float.MAX_VALUE >= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GE(-Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(-Float.MAX_VALUE, Float.POSITIVE_INFINITY)",
                         value, (-Float.MAX_VALUE >= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GE(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (-Float.MAX_VALUE >= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GE(-Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_GE(-Float.MAX_VALUE, Float.NaN)",
                         value, (-Float.MAX_VALUE >= Float.NaN));
            value = ef.exerciseFCmp_GE(-Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(-Float.MAX_VALUE, Float.MAX_VALUE)",
                         value, (-Float.MAX_VALUE >= Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(-Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(-Float.MAX_VALUE, -Float.MAX_VALUE)",
                         value, (-Float.MAX_VALUE >= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(-Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(-Float.MAX_VALUE, Float.MIN_VALUE)",
                         value, (-Float.MAX_VALUE >= Float.MIN_VALUE));
            value = ef.exerciseFCmp_GE(-Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(-Float.MAX_VALUE, -Float.MIN_VALUE)",
                         value, (-Float.MAX_VALUE >= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GE(Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(Float.MIN_VALUE, Float.POSITIVE_INFINITY)",
                         value, (Float.MIN_VALUE >= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GE(Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(Float.MIN_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (Float.MIN_VALUE >= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GE(Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_GE(Float.MIN_VALUE, Float.NaN)",
                         value, (Float.MIN_VALUE >= Float.NaN));
            value = ef.exerciseFCmp_GE(Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.MIN_VALUE, Float.MAX_VALUE)",
                         value, (Float.MIN_VALUE >= Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.MIN_VALUE, -Float.MAX_VALUE)",
                         value, (Float.MIN_VALUE >= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.MIN_VALUE, Float.MIN_VALUE)",
                         value, (Float.MIN_VALUE >= Float.MIN_VALUE));
            value = ef.exerciseFCmp_GE(Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(Float.MIN_VALUE, -Float.MIN_VALUE)",
                         value, (Float.MIN_VALUE >= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_GE(-Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(-Float.MIN_VALUE, Float.POSITIVE_INFINITY)",
                         value, (-Float.MIN_VALUE >= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_GE(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_GE(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (-Float.MIN_VALUE >= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_GE(-Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_GE(-Float.MIN_VALUE, Float.NaN)",
                         value, (-Float.MIN_VALUE >= Float.NaN));
            value = ef.exerciseFCmp_GE(-Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(-Float.MIN_VALUE, Float.MAX_VALUE)",
                         value, (-Float.MIN_VALUE >= Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(-Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_GE(-Float.MIN_VALUE, -Float.MAX_VALUE)",
                         value, (-Float.MIN_VALUE >= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_GE(-Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(-Float.MIN_VALUE, Float.MIN_VALUE)",
                         value, (-Float.MIN_VALUE >= Float.MIN_VALUE));
            value = ef.exerciseFCmp_GE(-Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_GE(-Float.MIN_VALUE, -Float.MIN_VALUE)",
                         value, (-Float.MIN_VALUE >= -Float.MIN_VALUE));
        }
        {
            boolean value;

            value = ef.exerciseFCmp_LT(0.0f, -0.0f);
            reportPassIf("exerciseFCmp_LT(0.0f, -0.0f)", value, (0.0f < -0.0f));
            value = ef.exerciseFCmp_LT(-0.0f, 0.0f);
            reportPassIf("exerciseFCmp_LT(-0.0f, 0.0f)", value, (-0.0f < 0.0f));
            value = ef.exerciseFCmp_LT(5.0f, -5.0f);
            reportPassIf("exerciseFCmp_LT(5.0f, -5.0f)", value, (5.0f < -5.0f));
            value = ef.exerciseFCmp_LT(5.0f, 5.0f);
            reportPassIf("exerciseFCmp_LT(5.0f, 5.0f)", value, (5.0f < 5.0f));
            value = ef.exerciseFCmp_LT(-5.0f, 5.0f);
            reportPassIf("exerciseFCmp_LT(-5.0f, 5.0f)", value, (-5.0f < 5.0f));
            value = ef.exerciseFCmp_LT(Float.NaN, 5.0f);
            reportPassIf("exerciseFCmp_LT(Float.NaN, 5.0f)", value, (Float.NaN < 5.0f));
            value = ef.exerciseFCmp_LT(5.0f, Float.NaN);
            reportPassIf("exerciseFCmp_LT(5.0f, Float.NaN)", value, (5.0f < Float.NaN));
            value = ef.exerciseFCmp_LT(Float.NaN, Float.NaN);
            reportPassIf("exerciseFCmp_LT(Float.NaN, Float.NaN)", value, (Float.NaN < Float.NaN));

            value = ef.exerciseFCmp_LT(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY)",
                         value, (Float.POSITIVE_INFINITY < Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LT(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY)",
                         value, (Float.POSITIVE_INFINITY < Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LT(Float.POSITIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFCmp_LT(Float.POSITIVE_INFINITY, Float.NaN)",
                         value, (Float.POSITIVE_INFINITY < Float.NaN));
            value = ef.exerciseFCmp_LT(Float.POSITIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.POSITIVE_INFINITY, Float.MAX_VALUE)",
                         value, (Float.POSITIVE_INFINITY < Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(Float.POSITIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.POSITIVE_INFINITY, -Float.MAX_VALUE)",
                         value, (Float.POSITIVE_INFINITY < -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(Float.POSITIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.POSITIVE_INFINITY, Float.MIN_VALUE)",
                         value, (Float.POSITIVE_INFINITY < Float.MIN_VALUE));
            value = ef.exerciseFCmp_LT(Float.POSITIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.POSITIVE_INFINITY, -Float.MIN_VALUE)",
                         value, (Float.POSITIVE_INFINITY < -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LT(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY)",
                         value, (Float.NEGATIVE_INFINITY < Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LT(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY)",
                         value, (Float.NEGATIVE_INFINITY < Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LT(Float.NEGATIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFCmp_LT(Float.NEGATIVE_INFINITY, Float.NaN)",
                         value, (Float.NEGATIVE_INFINITY < Float.NaN));
            value = ef.exerciseFCmp_LT(Float.NEGATIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.NEGATIVE_INFINITY, Float.MAX_VALUE)",
                         value, (Float.NEGATIVE_INFINITY < Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE)",
                         value, (Float.NEGATIVE_INFINITY < -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(Float.NEGATIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.NEGATIVE_INFINITY, Float.MIN_VALUE)",
                         value, (Float.NEGATIVE_INFINITY < Float.MIN_VALUE));
            value = ef.exerciseFCmp_LT(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE)",
                         value, (Float.NEGATIVE_INFINITY < -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LT(Float.NaN, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(Float.NaN, Float.POSITIVE_INFINITY)",
                         value, (Float.NaN < Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LT(Float.NaN, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(Float.NaN, Float.NEGATIVE_INFINITY)",
                         value, (Float.NaN < Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LT(Float.NaN, Float.NaN);
            reportPassIf("exerciseFCmp_LT(Float.NaN, Float.NaN)",
                         value, (Float.NaN < Float.NaN));
            value = ef.exerciseFCmp_LT(Float.NaN, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.NaN, Float.MAX_VALUE)",
                         value, (Float.NaN < Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(Float.NaN, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.NaN, -Float.MAX_VALUE)",
                         value, (Float.NaN < -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(Float.NaN, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.NaN, Float.MIN_VALUE)",
                         value, (Float.NaN < Float.MIN_VALUE));
            value = ef.exerciseFCmp_LT(Float.NaN, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.NaN, -Float.MIN_VALUE)",
                         value, (Float.NaN < -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LT(Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(Float.MAX_VALUE, Float.POSITIVE_INFINITY)",
                         value, (Float.MAX_VALUE < Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LT(Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(Float.MAX_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (Float.MAX_VALUE < Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LT(Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_LT(Float.MAX_VALUE, Float.NaN)",
                         value, (Float.MAX_VALUE < Float.NaN));
            value = ef.exerciseFCmp_LT(Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.MAX_VALUE, Float.MAX_VALUE)",
                         value, (Float.MAX_VALUE < Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.MAX_VALUE, -Float.MAX_VALUE)",
                         value, (Float.MAX_VALUE < -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.MAX_VALUE, Float.MIN_VALUE)",
                         value, (Float.MAX_VALUE < Float.MIN_VALUE));
            value = ef.exerciseFCmp_LT(Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.MAX_VALUE, -Float.MIN_VALUE)",
                         value, (Float.MAX_VALUE < -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LT(-Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(-Float.MAX_VALUE, Float.POSITIVE_INFINITY)",
                         value, (-Float.MAX_VALUE < Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LT(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (-Float.MAX_VALUE < Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LT(-Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_LT(-Float.MAX_VALUE, Float.NaN)",
                         value, (-Float.MAX_VALUE < Float.NaN));
            value = ef.exerciseFCmp_LT(-Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(-Float.MAX_VALUE, Float.MAX_VALUE)",
                         value, (-Float.MAX_VALUE < Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(-Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(-Float.MAX_VALUE, -Float.MAX_VALUE)",
                         value, (-Float.MAX_VALUE < -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(-Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(-Float.MAX_VALUE, Float.MIN_VALUE)",
                         value, (-Float.MAX_VALUE < Float.MIN_VALUE));
            value = ef.exerciseFCmp_LT(-Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(-Float.MAX_VALUE, -Float.MIN_VALUE)",
                         value, (-Float.MAX_VALUE < -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LT(Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(Float.MIN_VALUE, Float.POSITIVE_INFINITY)",
                         value, (Float.MIN_VALUE < Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LT(Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(Float.MIN_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (Float.MIN_VALUE < Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LT(Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_LT(Float.MIN_VALUE, Float.NaN)",
                         value, (Float.MIN_VALUE < Float.NaN));
            value = ef.exerciseFCmp_LT(Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.MIN_VALUE, Float.MAX_VALUE)",
                         value, (Float.MIN_VALUE < Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.MIN_VALUE, -Float.MAX_VALUE)",
                         value, (Float.MIN_VALUE < -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.MIN_VALUE, Float.MIN_VALUE)",
                         value, (Float.MIN_VALUE < Float.MIN_VALUE));
            value = ef.exerciseFCmp_LT(Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(Float.MIN_VALUE, -Float.MIN_VALUE)",
                         value, (Float.MIN_VALUE < -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LT(-Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(-Float.MIN_VALUE, Float.POSITIVE_INFINITY)",
                         value, (-Float.MIN_VALUE < Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LT(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LT(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (-Float.MIN_VALUE < Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LT(-Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_LT(-Float.MIN_VALUE, Float.NaN)",
                         value, (-Float.MIN_VALUE < Float.NaN));
            value = ef.exerciseFCmp_LT(-Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(-Float.MIN_VALUE, Float.MAX_VALUE)",
                         value, (-Float.MIN_VALUE < Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(-Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LT(-Float.MIN_VALUE, -Float.MAX_VALUE)",
                         value, (-Float.MIN_VALUE < -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LT(-Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(-Float.MIN_VALUE, Float.MIN_VALUE)",
                         value, (-Float.MIN_VALUE < Float.MIN_VALUE));
            value = ef.exerciseFCmp_LT(-Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LT(-Float.MIN_VALUE, -Float.MIN_VALUE)",
                         value, (-Float.MIN_VALUE < -Float.MIN_VALUE));
        }
        {
            boolean value;

            value = ef.exerciseFCmp_LE(0.0f, -0.0f);
            reportPassIf("exerciseFCmp_LE(0.0f, -0.0f)", value, (0.0f <= -0.0f));
            value = ef.exerciseFCmp_LE(-0.0f, 0.0f);
            reportPassIf("exerciseFCmp_LE(-0.0f, 0.0f)", value, (-0.0f <= 0.0f));
            value = ef.exerciseFCmp_LE(5.0f, -5.0f);
            reportPassIf("exerciseFCmp_LE(5.0f, -5.0f)", value, (5.0f <= -5.0f));
            value = ef.exerciseFCmp_LE(5.0f, 5.0f);
            reportPassIf("exerciseFCmp_LE(5.0f, 5.0f)", value, (5.0f <= 5.0f));
            value = ef.exerciseFCmp_LE(-5.0f, 5.0f);
            reportPassIf("exerciseFCmp_LE(-5.0f, 5.0f)", value, (-5.0f <= 5.0f));
            value = ef.exerciseFCmp_LE(Float.NaN, 5.0f);
            reportPassIf("exerciseFCmp_LE(Float.NaN, 5.0f)", value, (Float.NaN <= 5.0f));
            value = ef.exerciseFCmp_LE(5.0f, Float.NaN);
            reportPassIf("exerciseFCmp_LE(5.0f, Float.NaN)", value, (5.0f <= Float.NaN));
            value = ef.exerciseFCmp_LE(Float.NaN, Float.NaN);
            reportPassIf("exerciseFCmp_LE(Float.NaN, Float.NaN)", value, (Float.NaN <= Float.NaN));

            value = ef.exerciseFCmp_LE(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY)",
                         value, (Float.POSITIVE_INFINITY <= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LE(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY)",
                         value, (Float.POSITIVE_INFINITY <= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LE(Float.POSITIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFCmp_LE(Float.POSITIVE_INFINITY, Float.NaN)",
                         value, (Float.POSITIVE_INFINITY <= Float.NaN));
            value = ef.exerciseFCmp_LE(Float.POSITIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.POSITIVE_INFINITY, Float.MAX_VALUE)",
                         value, (Float.POSITIVE_INFINITY <= Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(Float.POSITIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.POSITIVE_INFINITY, -Float.MAX_VALUE)",
                         value, (Float.POSITIVE_INFINITY <= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(Float.POSITIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.POSITIVE_INFINITY, Float.MIN_VALUE)",
                         value, (Float.POSITIVE_INFINITY <= Float.MIN_VALUE));
            value = ef.exerciseFCmp_LE(Float.POSITIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.POSITIVE_INFINITY, -Float.MIN_VALUE)",
                         value, (Float.POSITIVE_INFINITY <= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LE(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY)",
                         value, (Float.NEGATIVE_INFINITY <= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LE(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY)",
                         value, (Float.NEGATIVE_INFINITY <= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LE(Float.NEGATIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFCmp_LE(Float.NEGATIVE_INFINITY, Float.NaN)",
                         value, (Float.NEGATIVE_INFINITY <= Float.NaN));
            value = ef.exerciseFCmp_LE(Float.NEGATIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.NEGATIVE_INFINITY, Float.MAX_VALUE)",
                         value, (Float.NEGATIVE_INFINITY <= Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE)",
                         value, (Float.NEGATIVE_INFINITY <= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(Float.NEGATIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.NEGATIVE_INFINITY, Float.MIN_VALUE)",
                         value, (Float.NEGATIVE_INFINITY <= Float.MIN_VALUE));
            value = ef.exerciseFCmp_LE(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE)",
                         value, (Float.NEGATIVE_INFINITY <= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LE(Float.NaN, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(Float.NaN, Float.POSITIVE_INFINITY)",
                         value, (Float.NaN <= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LE(Float.NaN, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(Float.NaN, Float.NEGATIVE_INFINITY)",
                         value, (Float.NaN <= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LE(Float.NaN, Float.NaN);
            reportPassIf("exerciseFCmp_LE(Float.NaN, Float.NaN)",
                         value, (Float.NaN <= Float.NaN));
            value = ef.exerciseFCmp_LE(Float.NaN, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.NaN, Float.MAX_VALUE)",
                         value, (Float.NaN <= Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(Float.NaN, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.NaN, -Float.MAX_VALUE)",
                         value, (Float.NaN <= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(Float.NaN, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.NaN, Float.MIN_VALUE)",
                         value, (Float.NaN <= Float.MIN_VALUE));
            value = ef.exerciseFCmp_LE(Float.NaN, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.NaN, -Float.MIN_VALUE)",
                         value, (Float.NaN <= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LE(Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(Float.MAX_VALUE, Float.POSITIVE_INFINITY)",
                         value, (Float.MAX_VALUE <= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LE(Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(Float.MAX_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (Float.MAX_VALUE <= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LE(Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_LE(Float.MAX_VALUE, Float.NaN)",
                         value, (Float.MAX_VALUE <= Float.NaN));
            value = ef.exerciseFCmp_LE(Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.MAX_VALUE, Float.MAX_VALUE)",
                         value, (Float.MAX_VALUE <= Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.MAX_VALUE, -Float.MAX_VALUE)",
                         value, (Float.MAX_VALUE <= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.MAX_VALUE, Float.MIN_VALUE)",
                         value, (Float.MAX_VALUE <= Float.MIN_VALUE));
            value = ef.exerciseFCmp_LE(Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.MAX_VALUE, -Float.MIN_VALUE)",
                         value, (Float.MAX_VALUE <= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LE(-Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(-Float.MAX_VALUE, Float.POSITIVE_INFINITY)",
                         value, (-Float.MAX_VALUE <= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LE(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (-Float.MAX_VALUE <= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LE(-Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_LE(-Float.MAX_VALUE, Float.NaN)",
                         value, (-Float.MAX_VALUE <= Float.NaN));
            value = ef.exerciseFCmp_LE(-Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(-Float.MAX_VALUE, Float.MAX_VALUE)",
                         value, (-Float.MAX_VALUE <= Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(-Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(-Float.MAX_VALUE, -Float.MAX_VALUE)",
                         value, (-Float.MAX_VALUE <= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(-Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(-Float.MAX_VALUE, Float.MIN_VALUE)",
                         value, (-Float.MAX_VALUE <= Float.MIN_VALUE));
            value = ef.exerciseFCmp_LE(-Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(-Float.MAX_VALUE, -Float.MIN_VALUE)",
                         value, (-Float.MAX_VALUE <= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LE(Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(Float.MIN_VALUE, Float.POSITIVE_INFINITY)",
                         value, (Float.MIN_VALUE <= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LE(Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(Float.MIN_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (Float.MIN_VALUE <= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LE(Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_LE(Float.MIN_VALUE, Float.NaN)",
                         value, (Float.MIN_VALUE <= Float.NaN));
            value = ef.exerciseFCmp_LE(Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.MIN_VALUE, Float.MAX_VALUE)",
                         value, (Float.MIN_VALUE <= Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.MIN_VALUE, -Float.MAX_VALUE)",
                         value, (Float.MIN_VALUE <= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.MIN_VALUE, Float.MIN_VALUE)",
                         value, (Float.MIN_VALUE <= Float.MIN_VALUE));
            value = ef.exerciseFCmp_LE(Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(Float.MIN_VALUE, -Float.MIN_VALUE)",
                         value, (Float.MIN_VALUE <= -Float.MIN_VALUE));

            value = ef.exerciseFCmp_LE(-Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(-Float.MIN_VALUE, Float.POSITIVE_INFINITY)",
                         value, (-Float.MIN_VALUE <= Float.POSITIVE_INFINITY));
            value = ef.exerciseFCmp_LE(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFCmp_LE(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY)",
                         value, (-Float.MIN_VALUE <= Float.NEGATIVE_INFINITY));
            value = ef.exerciseFCmp_LE(-Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFCmp_LE(-Float.MIN_VALUE, Float.NaN)",
                         value, (-Float.MIN_VALUE <= Float.NaN));
            value = ef.exerciseFCmp_LE(-Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(-Float.MIN_VALUE, Float.MAX_VALUE)",
                         value, (-Float.MIN_VALUE <= Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(-Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFCmp_LE(-Float.MIN_VALUE, -Float.MAX_VALUE)",
                         value, (-Float.MIN_VALUE <= -Float.MAX_VALUE));
            value = ef.exerciseFCmp_LE(-Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(-Float.MIN_VALUE, Float.MIN_VALUE)",
                         value, (-Float.MIN_VALUE <= Float.MIN_VALUE));
            value = ef.exerciseFCmp_LE(-Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFCmp_LE(-Float.MIN_VALUE, -Float.MIN_VALUE)",
                         value, (-Float.MIN_VALUE <= -Float.MIN_VALUE));
        }
        {
            float value = ef.exerciseFConst0();
            reportPassIf("exerciseFConst0()", value, 0.0f);
        }
        {
            float value = ef.exerciseFConst1();
            reportPassIf("exerciseFConst1()", value, 1.0f);
        }
        {
            float value = ef.exerciseFConst2();
            reportPassIf("exerciseFConst2()", value, 2.0f);
        }
        {
            float value = ef.exerciseFDiv(2345.12f, 43236.8f);
            reportPassIf("exerciseFDiv(2345.12f, 43236.8f)", value, (2345.12f / 43236.8f));
        }
        {
            float value = ef.exerciseFLoad0(5.0f);
            reportPassIf("exerciseFLoad0(5.0f)", value, 5.0f);
        }
        {
            float value = ef.exerciseFLoad1(0, 5.0f);
            reportPassIf("exerciseFLoad1(0, 5.0f)", value, 5.0f);
        }
        {
            float value = ef.exerciseFLoad2(0, 0, 5.0f);
            reportPassIf("exerciseFLoad2(0, 0, 5.0f)", value, 5.0f);
        }
        {
            float value = ef.exerciseFLoad3(0, 0, 0, 5.0f);
            reportPassIf("exerciseFLoad3(0, 0, 0, 5.0f)", value, 5.0f);
        }
        {
            float value = ef.exerciseFLoad(0, 0, 0, 0, 5.0f);
            reportPassIf("exerciseFLoad(0, 0, 0, 0, 5.0f)", value, 5.0f);
        }

        {
            float value;

            {
                float arg1 = Float.intBitsToFloat(0x00ffffff);
                float arg2 = Float.intBitsToFloat(0x3f000000);
                float res   = Float.intBitsToFloat(0x00800000);

                value = ef.exerciseFMul(arg1, arg2);
                reportPassIf(("exerciseFMul(" + arg1 + ", " + arg2 + ")"),
                             value, (arg1 * arg2));
                value = ef.exerciseFMul((arg1 * arg2), Float.MIN_VALUE);
                reportPassIf(("exerciseFMul((" + (arg1 * arg2) +
                              ", Float.MIN_VALUE)"),
                             value, ((arg1 * arg2) * Float.MIN_VALUE));
            }
            {
                float arg1 = Float.intBitsToFloat(0x00000008);
                float arg2 = Float.intBitsToFloat(0x3e000000);
                float res   = Float.intBitsToFloat(0x00000001);

                value = ef.exerciseFMul(arg1, arg2);
                reportPassIf(("exerciseFMul(" + arg1 + ", " + arg2 + ")"),
                             value, (arg1 * arg2));
                value = ef.exerciseFMul((arg1 * arg2), Float.MIN_VALUE);
                reportPassIf(("exerciseFMul(" + (arg1 * arg2) +
                              ", Float.MIN_VALUE)"),
                              value, ((arg1 * arg2) * Float.MIN_VALUE));
            }

            value = ef.exerciseFMul(1234.64f, 4343459.1321f);
            reportPassIf("exerciseFMul(1234.64f, 4343459.1321f)",
                         value, (1234.64f * 4343459.1321f));

            value = ef.exerciseFMul(1.0f, 1.0f);
            reportPassIf("exerciseFMul(1.0f, 1.0f)", value, (1.0f * 1.0f));
            value = ef.exerciseFMul(1.0f, -1.0f);
            reportPassIf("exerciseFMul(1.0f, -1.0f)", value, (1.0f * (-1.0f)));
            value = ef.exerciseFMul(-1.0f, 1.0f);
            reportPassIf("exerciseFMul(-1.0f, 1.0f)", value, (-1.0f * 1.0f));
            value = ef.exerciseFMul(-1.0f, -1.0f);
            reportPassIf("exerciseFMul(-1.0f, -1.0f)", value, (-1.0f * (-1.0f)));

            value = ef.exerciseFMul(10.0f, 1.0f);
            reportPassIf("exerciseFMul(10.0f, 1.0f)", value, (10.0f * 1.0f));
            value = ef.exerciseFMul(10.0f, -1.0f);
            reportPassIf("exerciseFMul(10.0f, -1.0f)", value, (10.0f * (-1.0f)));
            value = ef.exerciseFMul(-10.0f, 1.0f);
            reportPassIf("exerciseFMul(-10.0f, 1.0f)", value, (-10.0f * 1.0f));
            value = ef.exerciseFMul(-10.0f, -1.0f);
            reportPassIf("exerciseFMul(-10.0f, -1.0f)", value, (-10.0f * (-1.0f)));

            value = ef.exerciseFMul(1.0f, 10.0f);
            reportPassIf("exerciseFMul(1.0f, 10.0f)", value, (1.0f * 10.0f));
            value = ef.exerciseFMul(1.0f, -10.0f);
            reportPassIf("exerciseFMul(1.0f, -10.0f)", value, (1.0f * (-10.0f)));
            value = ef.exerciseFMul(-1.0f, 10.0f);
            reportPassIf("exerciseFMul(-1.0f, 10.0f)", value, (-1.0f * 10.0f));
            value = ef.exerciseFMul(-1.0f, -10.0f);
            reportPassIf("exerciseFMul(-1.0f, -10.0f)", value, (-1.0f * (-10.0f)));

            value = ef.exerciseFMul(10.0f, 10.0f);
            reportPassIf("exerciseFMul(10.0f, 10.0f)", value, (10.0f * 10.0f));
            value = ef.exerciseFMul(10.0f, -10.0f);
            reportPassIf("exerciseFMul(10.0f, -10.0f)", value, (10.0f * (-10.0f)));
            value = ef.exerciseFMul(-10.0f, 10.0f);
            reportPassIf("exerciseFMul(-10.0f, 10.0f)", value, (-10.0f * 10.0f));
            value = ef.exerciseFMul(-10.0f, -10.0f);
            reportPassIf("exerciseFMul(-10.0f, -10.0f)", value, (-10.0f * (-10.0f)));

            value = ef.exerciseFMul(0.0f, 0.0f);
            reportPassIf("exerciseFMul(0.0f, 0.0f)", value, (0.0f * 0.0f));
            value = ef.exerciseFMul(0.0f, -0.0f);
            reportPassIf("exerciseFMul(0.0f, -0.0f)", value, (0.0f * (-0.0f)));
            value = ef.exerciseFMul(0.0f, 467.24856f);
            reportPassIf("exerciseFMul(0.0f, 467.24856f)", value, (0.0f * 467.24856f));
            value = ef.exerciseFMul(0.0f, -467.24856f);
            reportPassIf("exerciseFMul(0.0f, -467.24856f)", value, (0.0f * (-467.24856f)));
            value = ef.exerciseFMul(0.0f, Float.NaN);
            reportPassIf("exerciseFMul(0.0f, Float.NaN)", value, (0.0f * Float.NaN));
            value = ef.exerciseFMul(0.0f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(0.0f, Float.POSITIVE_INFINITY)", value, (0.0f * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(0.0f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(0.0f, Float.NEGATIVE_INFINITY)", value, (0.0f * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(0.0f, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(0.0f, Float.MAX_VALUE)", value, (0.0f * Float.MAX_VALUE));
            value = ef.exerciseFMul(0.0f, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(0.0f, -Float.MAX_VALUE)", value, (0.0f * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(0.0f, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(0.0f, Float.MIN_VALUE)", value, (0.0f * Float.MIN_VALUE));
            value = ef.exerciseFMul(0.0f, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(0.0f, -Float.MIN_VALUE)", value, (0.0f * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(-0.0f, 0.0f);
            reportPassIf("exerciseFMul(-0.0f, 0.0f)", value, (-0.0f * 0.0f));
            value = ef.exerciseFMul(-0.0f, -0.0f);
            reportPassIf("exerciseFMul(-0.0f, -0.0f)", value, (-0.0f * (-0.0f)));
            value = ef.exerciseFMul(-0.0f, 467.24856f);
            reportPassIf("exerciseFMul(-0.0f, 467.24856f)", value, (-0.0f * 467.24856f));
            value = ef.exerciseFMul(-0.0f, -467.24856f);
            reportPassIf("exerciseFMul(-0.0f, -467.24856f)", value, (-0.0f * (-467.24856f)));
            value = ef.exerciseFMul(-0.0f, Float.NaN);
            reportPassIf("exerciseFMul(-0.0f, Float.NaN)", value, (-0.0f * Float.NaN));
            value = ef.exerciseFMul(-0.0f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(-0.0f, Float.POSITIVE_INFINITY)", value, (-0.0f * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(-0.0f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(-0.0f, Float.NEGATIVE_INFINITY)", value, (-0.0f * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(-0.0f, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(-0.0f, Float.MAX_VALUE)", value, (-0.0f * Float.MAX_VALUE));
            value = ef.exerciseFMul(-0.0f, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(-0.0f, -Float.MAX_VALUE)", value, (-0.0f * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(-0.0f, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(-0.0f, Float.MIN_VALUE)", value, (-0.0f * Float.MIN_VALUE));
            value = ef.exerciseFMul(-0.0f, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(-0.0f, -Float.MIN_VALUE)", value, (-0.0f * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(32456.5241f, 0.0f);
            reportPassIf("exerciseFMul(32456.5241f, 0.0f)", value, (32456.5241f * 0.0f));
            value = ef.exerciseFMul(32456.5241f, -0.0f);
            reportPassIf("exerciseFMul(32456.5241f, -0.0f)", value, (32456.5241f * (-0.0f)));
            value = ef.exerciseFMul(32456.5241f, 467.24856f);
            reportPassIf("exerciseFMul(32456.5241f, 467.24856f)", value, (32456.5241f * 467.24856f));
            value = ef.exerciseFMul(32456.5241f, -467.24856f);
            reportPassIf("exerciseFMul(32456.5241f, -467.24856f)", value, (32456.5241f * (-467.24856f)));
            value = ef.exerciseFMul(32456.5241f, Float.NaN);
            reportPassIf("exerciseFMul(32456.5241f, Float.NaN)", value, (32456.5241f * Float.NaN));
            value = ef.exerciseFMul(32456.5241f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(32456.5241f, Float.POSITIVE_INFINITY)", value, (32456.5241f * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(32456.5241f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(32456.5241f, Float.NEGATIVE_INFINITY)", value, (32456.5241f * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(32456.5241f, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(32456.5241f, Float.MAX_VALUE)", value, (32456.5241f * Float.MAX_VALUE));
            value = ef.exerciseFMul(32456.5241f, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(32456.5241f, -Float.MAX_VALUE)", value, (32456.5241f * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(32456.5241f, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(32456.5241f, Float.MIN_VALUE)", value, (32456.5241f * Float.MIN_VALUE));
            value = ef.exerciseFMul(32456.5241f, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(32456.5241f, -Float.MIN_VALUE)", value, (32456.5241f * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(-32456.5241f, 0.0f);
            reportPassIf("exerciseFMul(-32456.5241f, 0.0f)", value, (-32456.5241f * 0.0f));
            value = ef.exerciseFMul(-32456.5241f, -0.0f);
            reportPassIf("exerciseFMul(-32456.5241f, -0.0f)", value, (-32456.5241f * (-0.0f)));
            value = ef.exerciseFMul(-32456.5241f, 467.24856f);
            reportPassIf("exerciseFMul(-32456.5241f, 467.24856f)", value, (-32456.5241f * 467.24856f));
            value = ef.exerciseFMul(-32456.5241f, -467.24856f);
            reportPassIf("exerciseFMul(-32456.5241f, -467.24856f)", value, (-32456.5241f * (-467.24856f)));
            value = ef.exerciseFMul(-32456.5241f, Float.NaN);
            reportPassIf("exerciseFMul(-32456.5241f, Float.NaN)", value, (-32456.5241f * Float.NaN));
            value = ef.exerciseFMul(-32456.5241f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(-32456.5241f, Float.POSITIVE_INFINITY)", value, (-32456.5241f * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(-32456.5241f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(-32456.5241f, Float.NEGATIVE_INFINITY)", value, (-32456.5241f * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(-32456.5241f, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(-32456.5241f, Float.MAX_VALUE)", value, (-32456.5241f * Float.MAX_VALUE));
            value = ef.exerciseFMul(-32456.5241f, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(-32456.5241f, -Float.MAX_VALUE)", value, (-32456.5241f * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(-32456.5241f, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(-32456.5241f, Float.MIN_VALUE)", value, (-32456.5241f * Float.MIN_VALUE));
            value = ef.exerciseFMul(-32456.5241f, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(-32456.5241f, -Float.MIN_VALUE)", value, (-32456.5241f * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(467.24856f, 0.0f);
            reportPassIf("exerciseFMul(467.24856f, 0.0f)", value, (467.24856f * 0.0f));
            value = ef.exerciseFMul(467.24856f, -0.0f);
            reportPassIf("exerciseFMul(467.24856f, -0.0f)", value, (467.24856f * (-0.0f)));
            value = ef.exerciseFMul(467.24856f, 32456.5241f);
            reportPassIf("exerciseFMul(467.24856f, 32456.5241f)", value, (467.24856f * 32456.5241f));
            value = ef.exerciseFMul(467.24856f, -32456.5241f);
            reportPassIf("exerciseFMul(467.24856f, -32456.5241f)", value, (467.24856f * (-32456.5241f)));
            value = ef.exerciseFMul(467.24856f, Float.NaN);
            reportPassIf("exerciseFMul(467.24856f, Float.NaN)", value, (467.24856f * Float.NaN));
            value = ef.exerciseFMul(467.24856f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(467.24856f, Float.POSITIVE_INFINITY)", value, (467.24856f * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(467.24856f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(32456.5241f, Float.NEGATIVE_INFINITY)", value, (467.24856f * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(467.24856f, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(467.24856f, Float.MAX_VALUE)", value, (467.24856f * Float.MAX_VALUE));
            value = ef.exerciseFMul(467.24856f, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(467.24856f, -Float.MAX_VALUE)", value, (467.24856f * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(467.24856f, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(467.24856f, Float.MIN_VALUE)", value, (467.24856f * Float.MIN_VALUE));
            value = ef.exerciseFMul(467.24856f, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(467.24856f, -Float.MIN_VALUE)", value, (467.24856f * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(-467.24856f, 0.0f);
            reportPassIf("exerciseFMul(-467.24856f, 0.0f)", value, (-467.24856f * 0.0f));
            value = ef.exerciseFMul(-467.24856f, -0.0f);
            reportPassIf("exerciseFMul(-467.24856f, -0.0f)", value, (-467.24856f * (-0.0f)));
            value = ef.exerciseFMul(-467.24856f, 32456.5241f);
            reportPassIf("exerciseFMul(-467.24856f, 32456.5241f)", value, (-467.24856f * 32456.5241f));
            value = ef.exerciseFMul(-467.24856f, -32456.5241f);
            reportPassIf("exerciseFMul(-467.24856f, -32456.5241f)", value, (-467.24856f * (-32456.5241f)));
            value = ef.exerciseFMul(-467.24856f, Float.NaN);
            reportPassIf("exerciseFMul(-467.24856f, Float.NaN)", value, (-467.24856f * Float.NaN));
            value = ef.exerciseFMul(-467.24856f, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(-467.24856f, Float.POSITIVE_INFINITY)", value, (-467.24856f * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(-467.24856f, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(-467.24856f, Float.NEGATIVE_INFINITY)", value, (-467.24856f * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(-467.24856f, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(-467.24856f, Float.MAX_VALUE)", value, (-467.24856f * Float.MAX_VALUE));
            value = ef.exerciseFMul(-467.24856f, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(-467.24856f, -Float.MAX_VALUE)", value, (-467.24856f * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(-467.24856f, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(-467.24856f, Float.MIN_VALUE)", value, (-467.24856f * Float.MIN_VALUE));
            value = ef.exerciseFMul(-467.24856f, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(-467.24856f, -Float.MIN_VALUE)", value, (-467.24856f * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(Float.NaN, 0.0f);
            reportPassIf("exerciseFMul(Float.NaN, 0.0f)", value, (Float.NaN * 0.0f));
            value = ef.exerciseFMul(Float.NaN, -0.0f);
            reportPassIf("exerciseFMul(Float.NaN, -0.0f)", value, (Float.NaN * -0.0f));
            value = ef.exerciseFMul(Float.NaN, 467.24856f);
            reportPassIf("exerciseFMul(Float.NaN, 467.24856f)", value, (Float.NaN * 467.24856f));
            value = ef.exerciseFMul(Float.NaN, -467.24856f);
            reportPassIf("exerciseFMul(Float.NaN, -467.24856f)", value, (Float.NaN * -467.24856f));
            value = ef.exerciseFMul(Float.NaN, Float.NaN);
            reportPassIf("exerciseFMul(Float.NaN, Float.NaN)", value, (Float.NaN * Float.NaN));
            value = ef.exerciseFMul(Float.NaN, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(Float.NaN, Float.POSITIVE_INFINITY)", value, (Float.NaN * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(Float.NaN, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(Float.NaN, Float.NEGATIVE_INFINITY)", value, (Float.NaN * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(Float.NaN, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(Float.NaN, Float.MAX_VALUE)", value, (Float.NaN * Float.MAX_VALUE));
            value = ef.exerciseFMul(Float.NaN, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(Float.NaN, -Float.MAX_VALUE)", value, (Float.NaN * -Float.MAX_VALUE));
            value = ef.exerciseFMul(Float.NaN, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(Float.NaN, Float.MIN_VALUE)", value, (Float.NaN * Float.MIN_VALUE));
            value = ef.exerciseFMul(Float.NaN, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(Float.NaN, -Float.MIN_VALUE)", value, (Float.NaN * -Float.MIN_VALUE));

            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, 0.0f);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, 0.0f)", value, (Float.POSITIVE_INFINITY * 0.0f));
            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, -0.0f);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, -0.0f)", value, (Float.POSITIVE_INFINITY * (-0.0f)));
            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, 467.24856f);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, 467.24856f)", value, (Float.POSITIVE_INFINITY * 467.24856f));
            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, -467.24856f);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, -467.24856f)", value, (Float.POSITIVE_INFINITY * (-467.24856f)));
            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, Float.NaN)", value, (Float.POSITIVE_INFINITY * Float.NaN));
            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY)", value, (Float.POSITIVE_INFINITY * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY)", value, (Float.POSITIVE_INFINITY * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, Float.MAX_VALUE)", value, (Float.POSITIVE_INFINITY * Float.MAX_VALUE));
            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, -Float.MAX_VALUE)", value, (Float.POSITIVE_INFINITY * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, Float.MIN_VALUE)", value, (Float.POSITIVE_INFINITY * Float.MIN_VALUE));
            value = ef.exerciseFMul(Float.POSITIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(Float.POSITIVE_INFINITY, -Float.MIN_VALUE)", value, (Float.POSITIVE_INFINITY * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, 0.0f);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, 0.0f)", value, (Float.NEGATIVE_INFINITY * 0.0f));
            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, -0.0f);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, -0.0f)", value, (Float.NEGATIVE_INFINITY * (-0.0f)));
            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, 467.24856f);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, 467.24856f)", value, (Float.NEGATIVE_INFINITY * 467.24856f));
            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, -467.24856f);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, -467.24856f)", value, (Float.NEGATIVE_INFINITY * (-467.24856f)));
            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, Float.NaN);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, Float.NaN)", value, (Float.NEGATIVE_INFINITY * Float.NaN));
            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY)", value, (Float.NEGATIVE_INFINITY * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, Float.NEGATIVE_INFINITY)", value, (Float.NEGATIVE_INFINITY * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, Float.MAX_VALUE)", value, (Float.NEGATIVE_INFINITY * Float.MAX_VALUE));
            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, -Float.MAX_VALUE)", value, (Float.NEGATIVE_INFINITY * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, Float.MIN_VALUE)", value, (Float.NEGATIVE_INFINITY * Float.MIN_VALUE));
            value = ef.exerciseFMul(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(Float.NEGATIVE_INFINITY, -Float.MIN_VALUE)", value, (Float.NEGATIVE_INFINITY * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(Float.MAX_VALUE, 0.0f);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, 0.0f)", value, (Float.MAX_VALUE * 0.0f));
            value = ef.exerciseFMul(Float.MAX_VALUE, -0.0f);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, -0.0f)", value, (Float.MAX_VALUE * (-0.0f)));
            value = ef.exerciseFMul(Float.MAX_VALUE, 467.24856f);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, 467.24856f)", value, (Float.MAX_VALUE * 467.24856f));
            value = ef.exerciseFMul(Float.MAX_VALUE, -467.24856f);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, -467.24856f)", value, (Float.MAX_VALUE * (-467.24856f)));
            value = ef.exerciseFMul(Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, Float.NaN)", value, (Float.MAX_VALUE * Float.NaN));
            value = ef.exerciseFMul(Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, Float.POSITIVE_INFINITY)", value, (Float.MAX_VALUE * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, Float.NEGATIVE_INFINITY)", value, (Float.MAX_VALUE * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, Float.MAX_VALUE)", value, (Float.MAX_VALUE * Float.MAX_VALUE));
            value = ef.exerciseFMul(Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, -Float.MAX_VALUE)", value, (Float.MAX_VALUE * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, Float.MIN_VALUE)", value, (Float.MAX_VALUE * Float.MIN_VALUE));
            value = ef.exerciseFMul(Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(Float.MAX_VALUE, -Float.MIN_VALUE)", value, (Float.MAX_VALUE * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(-Float.MAX_VALUE, 0.0f);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, 0.0f)", value, (-Float.MAX_VALUE * 0.0f));
            value = ef.exerciseFMul(-Float.MAX_VALUE, -0.0f);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, -0.0f)", value, (-Float.MAX_VALUE * (-0.0f)));
            value = ef.exerciseFMul(-Float.MAX_VALUE, 467.24856f);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, 467.24856f)", value, (-Float.MAX_VALUE * 467.24856f));
            value = ef.exerciseFMul(-Float.MAX_VALUE, -467.24856f);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, -467.24856f)", value, (-Float.MAX_VALUE * (-467.24856f)));
            value = ef.exerciseFMul(-Float.MAX_VALUE, Float.NaN);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, Float.NaN)", value, (-Float.MAX_VALUE * Float.NaN));
            value = ef.exerciseFMul(-Float.MAX_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, Float.POSITIVE_INFINITY)", value, (-Float.MAX_VALUE * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, Float.NEGATIVE_INFINITY)", value, (-Float.MAX_VALUE * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(-Float.MAX_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, Float.MAX_VALUE)", value, (-Float.MAX_VALUE * Float.MAX_VALUE));
            value = ef.exerciseFMul(-Float.MAX_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, -Float.MAX_VALUE)", value, (-Float.MAX_VALUE * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(-Float.MAX_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, Float.MIN_VALUE)", value, (-Float.MAX_VALUE * Float.MIN_VALUE));
            value = ef.exerciseFMul(-Float.MAX_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(-Float.MAX_VALUE, -Float.MIN_VALUE)", value, (-Float.MAX_VALUE * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(Float.MIN_VALUE, 0.0f);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, 0.0f)", value, (Float.MIN_VALUE * 0.0f));
            value = ef.exerciseFMul(Float.MIN_VALUE, -0.0f);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, -0.0f)", value, (Float.MIN_VALUE * (-0.0f)));
            value = ef.exerciseFMul(Float.MIN_VALUE, 467.24856f);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, 467.24856f)", value, (Float.MIN_VALUE * 467.24856f));
            value = ef.exerciseFMul(Float.MIN_VALUE, -467.24856f);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, -467.24856f)", value, (Float.MIN_VALUE * (-467.24856f)));
            value = ef.exerciseFMul(Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, Float.NaN)", value, (Float.MIN_VALUE * Float.NaN));
            value = ef.exerciseFMul(Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, Float.POSITIVE_INFINITY)", value, (Float.MIN_VALUE * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, Float.NEGATIVE_INFINITY)", value, (Float.MIN_VALUE * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, Float.MAX_VALUE)", value, (Float.MIN_VALUE * Float.MAX_VALUE));
            value = ef.exerciseFMul(Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, -Float.MAX_VALUE)", value, (Float.MIN_VALUE * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, Float.MIN_VALUE)", value, (Float.MIN_VALUE * Float.MIN_VALUE));
            value = ef.exerciseFMul(Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(Float.MIN_VALUE, -Float.MIN_VALUE)", value, (Float.MIN_VALUE * (-Float.MIN_VALUE)));

            value = ef.exerciseFMul(-Float.MIN_VALUE, 0.0f);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, 0.0f)", value, (-Float.MIN_VALUE * 0.0f));
            value = ef.exerciseFMul(-Float.MIN_VALUE, -0.0f);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, -0.0f)", value, (-Float.MIN_VALUE * (-0.0f)));
            value = ef.exerciseFMul(-Float.MIN_VALUE, 467.24856f);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, 467.24856f)", value, (-Float.MIN_VALUE * 467.24856f));
            value = ef.exerciseFMul(-Float.MIN_VALUE, -467.24856f);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, -467.24856f)", value, (-Float.MIN_VALUE * (-467.24856f)));
            value = ef.exerciseFMul(-Float.MIN_VALUE, Float.NaN);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, Float.NaN)", value, (-Float.MIN_VALUE * Float.NaN));
            value = ef.exerciseFMul(-Float.MIN_VALUE, Float.POSITIVE_INFINITY);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, Float.POSITIVE_INFINITY)", value, (-Float.MIN_VALUE * Float.POSITIVE_INFINITY));
            value = ef.exerciseFMul(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, Float.NEGATIVE_INFINITY)", value, (-Float.MIN_VALUE * Float.NEGATIVE_INFINITY));
            value = ef.exerciseFMul(-Float.MIN_VALUE, Float.MAX_VALUE);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, Float.MAX_VALUE)", value, (-Float.MIN_VALUE * Float.MAX_VALUE));
            value = ef.exerciseFMul(-Float.MIN_VALUE, -Float.MAX_VALUE);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, -Float.MAX_VALUE)", value, (-Float.MIN_VALUE * (-Float.MAX_VALUE)));
            value = ef.exerciseFMul(-Float.MIN_VALUE, Float.MIN_VALUE);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, Float.MIN_VALUE)", value, (-Float.MIN_VALUE * Float.MIN_VALUE));
            value = ef.exerciseFMul(-Float.MIN_VALUE, -Float.MIN_VALUE);
            reportPassIf("exerciseFMul(-Float.MIN_VALUE, -Float.MIN_VALUE)", value, (-Float.MIN_VALUE * (-Float.MIN_VALUE)));
        }

        {
            float value = ef.exerciseFNeg(5.0f);
            reportPassIf("exerciseFNeg(5.0f)", value, (-5.0f));
        }
        {
            float value = ef.exerciseFRem(1234.64f, 51.0f);
            reportPassIf("exerciseFRem(1234.64f, 51.0f)", value, (1234.64f % 51.0f));
        }
        {
            float value = ef.exerciseFReturn(5.0f);
            reportPassIf("exerciseFReturn(5.0f)", value, 5.0f);
        }
        {
            float value = ef.exerciseFStore(0, 0, 0, 0, 0.0f);
            reportPassIf("exerciseFStore(0, 0, 0, 0, 0.0f)", value, 1.0f);
        }
        {
            float value = ef.exerciseFStore0(0.0f);
            reportPassIf("exerciseFStore0(0.0f)", value, 1.0f);
        }
        {
            float value = ef.exerciseFStore1(0, 0.0f);
            reportPassIf("exerciseFStore1(0, 0.0f)", value, 1.0f);
        }
        {
            float value = ef.exerciseFStore2(0, 0, 0.0f);
            reportPassIf("exerciseFStore2(0, 0, 0.0f)", value, 1.0f);
        }
        {
            float value = ef.exerciseFStore3(0, 0, 0, 0.0f);
            reportPassIf("exerciseFStore3(0, 0, 0, 0.0f)", value, 1.0f);
        }
        {
            float value = ef.exerciseFSub(124324.354f, 6534.542f);
            reportPassIf("exerciseFSub(124324.354f, 6534.542f)", value, (124324.354f - 6534.542f));
        }
        if (verbose) {
            System.out.println("");
        }

        // Exercise the Double opcodes:
        System.out.println("Testing Double Opcodes:");
        ExerciseDoubleOpcodes ed = new ExerciseDoubleOpcodes();
        {
            float value = ed.exerciseD2F(5.0d);
            reportPassIf("exerciseD2F(5.0d)", value, 5.0f);
        }
        {
            int value = ed.exerciseD2I(5.0d);
            reportPassIf("exerciseD2I(5.0d)", value, 5);
        }
        {
            long value = ed.exerciseD2L(5.0d);
            reportPassIf("exerciseD2L(5.0d)", value, 5l);
        }
        {
            double value = ed.exerciseDAdd(3219874324.214918d, 9856322.434534d);
            reportPassIf("exerciseDAdd(3219874324.214918d, 9856322.434534d)",
                         value, (3219874324.214918d + 9856322.434534d));
        }
        {
            double[] da = new double[5];
            da[3] = 5.0d;
            double value = ed.exerciseDALoad(da, 3);
            reportPassIf("exerciseDALoad(da, 3)", value, 5.0d);
        }
        {
            double[] da = new double[5];
            da[3] = 0.0d;
            ed.exerciseDAStore(da, 3, 5.0d);
            reportPassIf("exerciseDAStore(da, 3, 5.0d)", da[3], 5.0d);
        }

        {
            boolean value;

            {
                double dvalues[] = {
                    Double.NEGATIVE_INFINITY,
                    -1.7976931348623157E308d,
                    -1.0d,
                    -4.9E-324d,
                    -0.0d,
                    0.0d,
                    4.9E-324d,
                    1.0d,
                    1.7976931348623157E308d,
                    Double.POSITIVE_INFINITY,
                    Double.NaN
                };
                for (int i = 0; i < dvalues.length; i++) {
                    double lvalue = dvalues[i];
                    for (int j = 0; j < dvalues.length; j++) {
                        double rvalue = dvalues[j];
                        value = ed.exerciseDCmp_EQ(lvalue, rvalue);
                        boolean success =
                        reportPassIf("exerciseDCmp_EQ(" + lvalue + ", " +
                                     rvalue + ")",
                                     value, (lvalue == rvalue));
                        if (!success) {
                            dumpValue(lvalue);
                            dumpValue(rvalue);
                        }
                    }
                }
            }
            value = ed.exerciseDCmp_EQ(0.0, -0.0);
            reportPassIf("exerciseDCmp_EQ(0.0, -0.0)", value, (0.0 == -0.0));
            value = ed.exerciseDCmp_EQ(-0.0, 0.0);
            reportPassIf("exerciseDCmp_EQ(-0.0, 0.0)", value, (-0.0 == 0.0));
            value = ed.exerciseDCmp_EQ(5.0d, 5.0d);
            reportPassIf("exerciseDCmp_EQ(5.0d, 5.0d)", value, (5.0d == 5.0d));
            value = ed.exerciseDCmp_EQ(-5.0d, 5.0d);
            reportPassIf("exerciseDCmp_EQ(-5.0d, 5.0d)", value, (-5.0d == 5.0d));
            value = ed.exerciseDCmp_EQ(Double.NaN, 5.0d);
            reportPassIf("exerciseDCmp_EQ(Double.NaN, 5.0d)", value, (Double.NaN == 5.0d));
            value = ed.exerciseDCmp_EQ(5.0d, Double.NaN);
            reportPassIf("exerciseDCmp_EQ(5.0d, Double.NaN)", value, (5.0d == Double.NaN));

            value = ed.exerciseDCmp_EQ(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY == Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY == Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(Double.POSITIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_EQ(Double.POSITIVE_INFINITY, Double.NaN)",
                         value, (Double.POSITIVE_INFINITY == Double.NaN));
            value = ed.exerciseDCmp_EQ(Double.POSITIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.POSITIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY == Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(Double.POSITIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.POSITIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY == -Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(Double.POSITIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.POSITIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY == Double.MIN_VALUE));
            value = ed.exerciseDCmp_EQ(Double.POSITIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.POSITIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY == -Double.MIN_VALUE));

            value = ed.exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY == Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY == Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, Double.NaN)",
                         value, (Double.NEGATIVE_INFINITY == Double.NaN));
            value = ed.exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY == Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY == -Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY == Double.MIN_VALUE));
            value = ed.exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY == -Double.MIN_VALUE));

            value = ed.exerciseDCmp_EQ(Double.NaN, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(Double.NaN, Double.POSITIVE_INFINITY)",
                         value, (Double.NaN == Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(Double.NaN, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(Double.NaN, Double.NEGATIVE_INFINITY)",
                         value, (Double.NaN == Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_EQ(Double.NaN, Double.NaN)",
                         value, (Double.NaN == Double.NaN));
            value = ed.exerciseDCmp_EQ(Double.NaN, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.NaN, Double.MAX_VALUE)",
                         value, (Double.NaN == Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(Double.NaN, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.NaN, -Double.MAX_VALUE)",
                         value, (Double.NaN == -Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(Double.NaN, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.NaN, Double.MIN_VALUE)",
                         value, (Double.NaN == Double.MIN_VALUE));
            value = ed.exerciseDCmp_EQ(Double.NaN, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.NaN, -Double.MIN_VALUE)",
                         value, (Double.NaN == -Double.MIN_VALUE));

            value = ed.exerciseDCmp_EQ(Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MAX_VALUE == Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MAX_VALUE == Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_EQ(Double.MAX_VALUE, Double.NaN)",
                         value, (Double.MAX_VALUE == Double.NaN));
            value = ed.exerciseDCmp_EQ(Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE == Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE == -Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE == Double.MIN_VALUE));
            value = ed.exerciseDCmp_EQ(Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE == -Double.MIN_VALUE));

            value = ed.exerciseDCmp_EQ(-Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(-Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MAX_VALUE == Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MAX_VALUE == Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(-Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_EQ(-Double.MAX_VALUE, Double.NaN)",
                         value, (-Double.MAX_VALUE == Double.NaN));
            value = ed.exerciseDCmp_EQ(-Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(-Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE == Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(-Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(-Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE == -Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(-Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(-Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE == Double.MIN_VALUE));
            value = ed.exerciseDCmp_EQ(-Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(-Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE == -Double.MIN_VALUE));

            value = ed.exerciseDCmp_EQ(Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MIN_VALUE == Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MIN_VALUE == Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_EQ(Double.MIN_VALUE, Double.NaN)",
                         value, (Double.MIN_VALUE == Double.NaN));
            value = ed.exerciseDCmp_EQ(Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE == Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE == -Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE == Double.MIN_VALUE));
            value = ed.exerciseDCmp_EQ(Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE == -Double.MIN_VALUE));

            value = ed.exerciseDCmp_EQ(-Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(-Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MIN_VALUE == Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_EQ(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MIN_VALUE == Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_EQ(-Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_EQ(-Double.MIN_VALUE, Double.NaN)",
                         value, (-Double.MIN_VALUE == Double.NaN));
            value = ed.exerciseDCmp_EQ(-Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(-Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE == Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(-Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_EQ(-Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE == -Double.MAX_VALUE));
            value = ed.exerciseDCmp_EQ(-Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(-Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE == Double.MIN_VALUE));
            value = ed.exerciseDCmp_EQ(-Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_EQ(-Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE == -Double.MIN_VALUE));
        }
        {
            boolean value;

            value = ed.exerciseDCmp_NE(0.0, -0.0);
            reportPassIf("exerciseDCmp_NE(0.0, -0.0)", value, (0.0 != -0.0));
            value = ed.exerciseDCmp_NE(-0.0, 0.0);
            reportPassIf("exerciseDCmp_NE(-0.0, 0.0)", value, (-0.0 != 0.0));
            value = ed.exerciseDCmp_NE(5.0d, 5.0d);
            reportPassIf("exerciseDCmp_NE(5.0d, 5.0d)", value, (5.0d != 5.0d));
            value = ed.exerciseDCmp_NE(-5.0d, 5.0d);
            reportPassIf("exerciseDCmp_NE(-5.0d, 5.0d)", value, (-5.0d != 5.0d));
            value = ed.exerciseDCmp_NE(Double.NaN, 5.0d);
            reportPassIf("exerciseDCmp_NE(Double.NaN, 5.0d)", value, (Double.NaN != 5.0d));
            value = ed.exerciseDCmp_NE(5.0d, Double.NaN);
            reportPassIf("exerciseDCmp_NE(5.0d, Double.NaN)", value, (5.0d != Double.NaN));
            value = ed.exerciseDCmp_NE(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_NE(Double.NaN, Double.NaN)", value, (Double.NaN != Double.NaN));

            value = ed.exerciseDCmp_NE(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY != Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_NE(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY != Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_NE(Double.POSITIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_NE(Double.POSITIVE_INFINITY, Double.NaN)",
                         value, (Double.POSITIVE_INFINITY != Double.NaN));
            value = ed.exerciseDCmp_NE(Double.POSITIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.POSITIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY != Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(Double.POSITIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.POSITIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY != -Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(Double.POSITIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.POSITIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY != Double.MIN_VALUE));
            value = ed.exerciseDCmp_NE(Double.POSITIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.POSITIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY != -Double.MIN_VALUE));

            value = ed.exerciseDCmp_NE(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY != Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_NE(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY != Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_NE(Double.NEGATIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_NE(Double.NEGATIVE_INFINITY, Double.NaN)",
                         value, (Double.NEGATIVE_INFINITY != Double.NaN));
            value = ed.exerciseDCmp_NE(Double.NEGATIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.NEGATIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY != Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY != -Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(Double.NEGATIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.NEGATIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY != Double.MIN_VALUE));
            value = ed.exerciseDCmp_NE(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY != -Double.MIN_VALUE));

            value = ed.exerciseDCmp_NE(Double.NaN, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(Double.NaN, Double.POSITIVE_INFINITY)",
                         value, (Double.NaN != Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_NE(Double.NaN, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(Double.NaN, Double.NEGATIVE_INFINITY)",
                         value, (Double.NaN != Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_NE(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_NE(Double.NaN, Double.NaN)",
                         value, (Double.NaN != Double.NaN));
            value = ed.exerciseDCmp_NE(Double.NaN, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.NaN, Double.MAX_VALUE)",
                         value, (Double.NaN != Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(Double.NaN, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.NaN, -Double.MAX_VALUE)",
                         value, (Double.NaN != -Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(Double.NaN, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.NaN, Double.MIN_VALUE)",
                         value, (Double.NaN != Double.MIN_VALUE));
            value = ed.exerciseDCmp_NE(Double.NaN, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.NaN, -Double.MIN_VALUE)",
                         value, (Double.NaN != -Double.MIN_VALUE));

            value = ed.exerciseDCmp_NE(Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MAX_VALUE != Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_NE(Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MAX_VALUE != Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_NE(Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_NE(Double.MAX_VALUE, Double.NaN)",
                         value, (Double.MAX_VALUE != Double.NaN));
            value = ed.exerciseDCmp_NE(Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE != Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE != -Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE != Double.MIN_VALUE));
            value = ed.exerciseDCmp_NE(Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE != -Double.MIN_VALUE));

            value = ed.exerciseDCmp_NE(-Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(-Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MAX_VALUE != Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_NE(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MAX_VALUE != Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_NE(-Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_NE(-Double.MAX_VALUE, Double.NaN)",
                         value, (-Double.MAX_VALUE != Double.NaN));
            value = ed.exerciseDCmp_NE(-Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(-Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE != Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(-Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(-Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE != -Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(-Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(-Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE != Double.MIN_VALUE));
            value = ed.exerciseDCmp_NE(-Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(-Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE != -Double.MIN_VALUE));

            value = ed.exerciseDCmp_NE(Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MIN_VALUE != Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_NE(Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MIN_VALUE != Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_NE(Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_NE(Double.MIN_VALUE, Double.NaN)",
                         value, (Double.MIN_VALUE != Double.NaN));
            value = ed.exerciseDCmp_NE(Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE != Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE != -Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE != Double.MIN_VALUE));
            value = ed.exerciseDCmp_NE(Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE != -Double.MIN_VALUE));

            value = ed.exerciseDCmp_NE(-Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(-Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MIN_VALUE != Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_NE(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_NE(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MIN_VALUE != Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_NE(-Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_NE(-Double.MIN_VALUE, Double.NaN)",
                         value, (-Double.MIN_VALUE != Double.NaN));
            value = ed.exerciseDCmp_NE(-Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(-Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE != Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(-Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_NE(-Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE != -Double.MAX_VALUE));
            value = ed.exerciseDCmp_NE(-Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(-Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE != Double.MIN_VALUE));
            value = ed.exerciseDCmp_NE(-Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_NE(-Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE != -Double.MIN_VALUE));
        }
        {
            boolean value;

            value = ed.exerciseDCmp_GT(0.0, -0.0);
            reportPassIf("exerciseDCmp_GT(0.0, -0.0)", value, (0.0 > -0.0));
            value = ed.exerciseDCmp_GT(-0.0, 0.0);
            reportPassIf("exerciseDCmp_GT(-0.0, 0.0)", value, (-0.0 > 0.0));
            value = ed.exerciseDCmp_GT(5.0d, -5.0d);
            reportPassIf("exerciseDCmp_GT(5.0d, -5.0d)", value, (5.0d > -5.0d));
            value = ed.exerciseDCmp_GT(5.0d, 5.0d);
            reportPassIf("exerciseDCmp_GT(5.0d, 5.0d)", value, (5.0d > 5.0d));
            value = ed.exerciseDCmp_GT(-5.0d, 5.0d);
            reportPassIf("exerciseDCmp_GT(-5.0d, 5.0d)", value, (-5.0d > 5.0d));
            value = ed.exerciseDCmp_GT(Double.NaN, 5.0d);
            reportPassIf("exerciseDCmp_GT(Double.NaN, 5.0d)", value, (Double.NaN > 5.0d));
            value = ed.exerciseDCmp_GT(5.0d, Double.NaN);
            reportPassIf("exerciseDCmp_GT(5.0d, Double.NaN)", value, (5.0d > Double.NaN));
            value = ed.exerciseDCmp_GT(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_GT(Double.NaN, Double.NaN)", value, (Double.NaN > Double.NaN));

            value = ed.exerciseDCmp_GT(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY > Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GT(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY > Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GT(Double.POSITIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_GT(Double.POSITIVE_INFINITY, Double.NaN)",
                         value, (Double.POSITIVE_INFINITY > Double.NaN));
            value = ed.exerciseDCmp_GT(Double.POSITIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.POSITIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY > Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(Double.POSITIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.POSITIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY > -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(Double.POSITIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.POSITIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY > Double.MIN_VALUE));
            value = ed.exerciseDCmp_GT(Double.POSITIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.POSITIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY > -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GT(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY > Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GT(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY > Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GT(Double.NEGATIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_GT(Double.NEGATIVE_INFINITY, Double.NaN)",
                         value, (Double.NEGATIVE_INFINITY > Double.NaN));
            value = ed.exerciseDCmp_GT(Double.NEGATIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.NEGATIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY > Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY > -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(Double.NEGATIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.NEGATIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY > Double.MIN_VALUE));
            value = ed.exerciseDCmp_GT(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY > -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GT(Double.NaN, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(Double.NaN, Double.POSITIVE_INFINITY)",
                         value, (Double.NaN > Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GT(Double.NaN, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(Double.NaN, Double.NEGATIVE_INFINITY)",
                         value, (Double.NaN > Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GT(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_GT(Double.NaN, Double.NaN)",
                         value, (Double.NaN > Double.NaN));
            value = ed.exerciseDCmp_GT(Double.NaN, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.NaN, Double.MAX_VALUE)",
                         value, (Double.NaN > Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(Double.NaN, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.NaN, -Double.MAX_VALUE)",
                         value, (Double.NaN > -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(Double.NaN, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.NaN, Double.MIN_VALUE)",
                         value, (Double.NaN > Double.MIN_VALUE));
            value = ed.exerciseDCmp_GT(Double.NaN, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.NaN, -Double.MIN_VALUE)",
                         value, (Double.NaN > -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GT(Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MAX_VALUE > Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GT(Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MAX_VALUE > Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GT(Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_GT(Double.MAX_VALUE, Double.NaN)",
                         value, (Double.MAX_VALUE > Double.NaN));
            value = ed.exerciseDCmp_GT(Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE > Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE > -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE > Double.MIN_VALUE));
            value = ed.exerciseDCmp_GT(Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE > -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GT(-Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(-Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MAX_VALUE > Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GT(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MAX_VALUE > Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GT(-Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_GT(-Double.MAX_VALUE, Double.NaN)",
                         value, (-Double.MAX_VALUE > Double.NaN));
            value = ed.exerciseDCmp_GT(-Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(-Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE > Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(-Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(-Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE > -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(-Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(-Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE > Double.MIN_VALUE));
            value = ed.exerciseDCmp_GT(-Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(-Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE > -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GT(Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MIN_VALUE > Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GT(Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MIN_VALUE > Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GT(Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_GT(Double.MIN_VALUE, Double.NaN)",
                         value, (Double.MIN_VALUE > Double.NaN));
            value = ed.exerciseDCmp_GT(Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE > Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE > -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE > Double.MIN_VALUE));
            value = ed.exerciseDCmp_GT(Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE > -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GT(-Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(-Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MIN_VALUE > Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GT(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GT(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MIN_VALUE > Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GT(-Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_GT(-Double.MIN_VALUE, Double.NaN)",
                         value, (-Double.MIN_VALUE > Double.NaN));
            value = ed.exerciseDCmp_GT(-Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(-Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE > Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(-Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GT(-Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE > -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GT(-Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(-Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE > Double.MIN_VALUE));
            value = ed.exerciseDCmp_GT(-Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GT(-Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE > -Double.MIN_VALUE));
        }
        {
            boolean value;

            value = ed.exerciseDCmp_GE(0.0, -0.0);
            reportPassIf("exerciseDCmp_GE(0.0, -0.0)", value, (0.0 >= -0.0));
            value = ed.exerciseDCmp_GE(-0.0, 0.0);
            reportPassIf("exerciseDCmp_GE(-0.0, 0.0)", value, (-0.0 >= 0.0));
            value = ed.exerciseDCmp_GE(5.0d, -5.0d);
            reportPassIf("exerciseDCmp_GE(5.0d, -5.0d)", value, (5.0d >= -5.0d));
            value = ed.exerciseDCmp_GE(5.0d, 5.0d);
            reportPassIf("exerciseDCmp_GE(5.0d, 5.0d)", value, (5.0d >= 5.0d));
            value = ed.exerciseDCmp_GE(-5.0d, 5.0d);
            reportPassIf("exerciseDCmp_GE(-5.0d, 5.0d)", value, (-5.0d >= 5.0d));
            value = ed.exerciseDCmp_GE(Double.NaN, 5.0d);
            reportPassIf("exerciseDCmp_GE(Double.NaN, 5.0d)", value, (Double.NaN >= 5.0d));
            value = ed.exerciseDCmp_GE(5.0d, Double.NaN);
            reportPassIf("exerciseDCmp_GE(5.0d, Double.NaN)", value, (5.0d >= Double.NaN));
            value = ed.exerciseDCmp_GE(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_GE(Double.NaN, Double.NaN)", value, (Double.NaN >= Double.NaN));

            value = ed.exerciseDCmp_GE(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY >= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GE(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY >= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GE(Double.POSITIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_GE(Double.POSITIVE_INFINITY, Double.NaN)",
                         value, (Double.POSITIVE_INFINITY >= Double.NaN));
            value = ed.exerciseDCmp_GE(Double.POSITIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.POSITIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY >= Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(Double.POSITIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.POSITIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY >= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(Double.POSITIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.POSITIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY >= Double.MIN_VALUE));
            value = ed.exerciseDCmp_GE(Double.POSITIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.POSITIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY >= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GE(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY >= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GE(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY >= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GE(Double.NEGATIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_GE(Double.NEGATIVE_INFINITY, Double.NaN)",
                         value, (Double.NEGATIVE_INFINITY >= Double.NaN));
            value = ed.exerciseDCmp_GE(Double.NEGATIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.NEGATIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY >= Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY >= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(Double.NEGATIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.NEGATIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY >= Double.MIN_VALUE));
            value = ed.exerciseDCmp_GE(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY >= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GE(Double.NaN, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(Double.NaN, Double.POSITIVE_INFINITY)",
                         value, (Double.NaN >= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GE(Double.NaN, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(Double.NaN, Double.NEGATIVE_INFINITY)",
                         value, (Double.NaN >= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GE(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_GE(Double.NaN, Double.NaN)",
                         value, (Double.NaN >= Double.NaN));
            value = ed.exerciseDCmp_GE(Double.NaN, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.NaN, Double.MAX_VALUE)",
                         value, (Double.NaN >= Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(Double.NaN, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.NaN, -Double.MAX_VALUE)",
                         value, (Double.NaN >= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(Double.NaN, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.NaN, Double.MIN_VALUE)",
                         value, (Double.NaN >= Double.MIN_VALUE));
            value = ed.exerciseDCmp_GE(Double.NaN, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.NaN, -Double.MIN_VALUE)",
                         value, (Double.NaN >= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GE(Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MAX_VALUE >= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GE(Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MAX_VALUE >= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GE(Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_GE(Double.MAX_VALUE, Double.NaN)",
                         value, (Double.MAX_VALUE >= Double.NaN));
            value = ed.exerciseDCmp_GE(Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE >= Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE >= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE >= Double.MIN_VALUE));
            value = ed.exerciseDCmp_GE(Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE >= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GE(-Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(-Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MAX_VALUE >= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GE(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MAX_VALUE >= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GE(-Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_GE(-Double.MAX_VALUE, Double.NaN)",
                         value, (-Double.MAX_VALUE >= Double.NaN));
            value = ed.exerciseDCmp_GE(-Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(-Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE >= Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(-Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(-Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE >= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(-Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(-Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE >= Double.MIN_VALUE));
            value = ed.exerciseDCmp_GE(-Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(-Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE >= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GE(Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MIN_VALUE >= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GE(Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MIN_VALUE >= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GE(Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_GE(Double.MIN_VALUE, Double.NaN)",
                         value, (Double.MIN_VALUE >= Double.NaN));
            value = ed.exerciseDCmp_GE(Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE >= Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE >= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE >= Double.MIN_VALUE));
            value = ed.exerciseDCmp_GE(Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE >= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_GE(-Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(-Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MIN_VALUE >= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_GE(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_GE(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MIN_VALUE >= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_GE(-Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_GE(-Double.MIN_VALUE, Double.NaN)",
                         value, (-Double.MIN_VALUE >= Double.NaN));
            value = ed.exerciseDCmp_GE(-Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(-Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE >= Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(-Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_GE(-Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE >= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_GE(-Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(-Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE >= Double.MIN_VALUE));
            value = ed.exerciseDCmp_GE(-Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_GE(-Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE >= -Double.MIN_VALUE));
        }
        {
            boolean value;

            value = ed.exerciseDCmp_LT(0.0, -0.0);
            reportPassIf("exerciseDCmp_LT(0.0, -0.0)", value, (0.0 < -0.0));
            value = ed.exerciseDCmp_LT(-0.0, 0.0);
            reportPassIf("exerciseDCmp_LT(-0.0, 0.0)", value, (-0.0 < 0.0));
            value = ed.exerciseDCmp_LT(5.0d, -5.0d);
            reportPassIf("exerciseDCmp_LT(5.0d, -5.0d)", value, (5.0d < -5.0d));
            value = ed.exerciseDCmp_LT(5.0d, 5.0d);
            reportPassIf("exerciseDCmp_LT(5.0d, 5.0d)", value, (5.0d < 5.0d));
            value = ed.exerciseDCmp_LT(-5.0d, 5.0d);
            reportPassIf("exerciseDCmp_LT(-5.0d, 5.0d)", value, (-5.0d < 5.0d));
            value = ed.exerciseDCmp_LT(Double.NaN, 5.0d);
            reportPassIf("exerciseDCmp_LT(Double.NaN, 5.0d)", value, (Double.NaN < 5.0d));
            value = ed.exerciseDCmp_LT(5.0d, Double.NaN);
            reportPassIf("exerciseDCmp_LT(5.0d, Double.NaN)", value, (5.0d < Double.NaN));
            value = ed.exerciseDCmp_LT(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_LT(Double.NaN, Double.NaN)", value, (Double.NaN < Double.NaN));

            value = ed.exerciseDCmp_LT(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY < Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LT(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY < Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LT(Double.POSITIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_LT(Double.POSITIVE_INFINITY, Double.NaN)",
                         value, (Double.POSITIVE_INFINITY < Double.NaN));
            value = ed.exerciseDCmp_LT(Double.POSITIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.POSITIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY < Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(Double.POSITIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.POSITIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY < -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(Double.POSITIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.POSITIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY < Double.MIN_VALUE));
            value = ed.exerciseDCmp_LT(Double.POSITIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.POSITIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY < -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LT(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY < Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LT(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY < Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LT(Double.NEGATIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_LT(Double.NEGATIVE_INFINITY, Double.NaN)",
                         value, (Double.NEGATIVE_INFINITY < Double.NaN));
            value = ed.exerciseDCmp_LT(Double.NEGATIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.NEGATIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY < Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY < -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(Double.NEGATIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.NEGATIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY < Double.MIN_VALUE));
            value = ed.exerciseDCmp_LT(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY < -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LT(Double.NaN, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(Double.NaN, Double.POSITIVE_INFINITY)",
                         value, (Double.NaN < Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LT(Double.NaN, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(Double.NaN, Double.NEGATIVE_INFINITY)",
                         value, (Double.NaN < Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LT(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_LT(Double.NaN, Double.NaN)",
                         value, (Double.NaN < Double.NaN));
            value = ed.exerciseDCmp_LT(Double.NaN, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.NaN, Double.MAX_VALUE)",
                         value, (Double.NaN < Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(Double.NaN, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.NaN, -Double.MAX_VALUE)",
                         value, (Double.NaN < -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(Double.NaN, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.NaN, Double.MIN_VALUE)",
                         value, (Double.NaN < Double.MIN_VALUE));
            value = ed.exerciseDCmp_LT(Double.NaN, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.NaN, -Double.MIN_VALUE)",
                         value, (Double.NaN < -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LT(Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MAX_VALUE < Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LT(Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MAX_VALUE < Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LT(Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_LT(Double.MAX_VALUE, Double.NaN)",
                         value, (Double.MAX_VALUE < Double.NaN));
            value = ed.exerciseDCmp_LT(Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE < Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE < -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE < Double.MIN_VALUE));
            value = ed.exerciseDCmp_LT(Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE < -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LT(-Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(-Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MAX_VALUE < Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LT(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MAX_VALUE < Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LT(-Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_LT(-Double.MAX_VALUE, Double.NaN)",
                         value, (-Double.MAX_VALUE < Double.NaN));
            value = ed.exerciseDCmp_LT(-Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(-Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE < Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(-Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(-Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE < -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(-Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(-Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE < Double.MIN_VALUE));
            value = ed.exerciseDCmp_LT(-Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(-Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE < -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LT(Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MIN_VALUE < Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LT(Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MIN_VALUE < Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LT(Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_LT(Double.MIN_VALUE, Double.NaN)",
                         value, (Double.MIN_VALUE < Double.NaN));
            value = ed.exerciseDCmp_LT(Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE < Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE < -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE < Double.MIN_VALUE));
            value = ed.exerciseDCmp_LT(Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE < -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LT(-Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(-Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MIN_VALUE < Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LT(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LT(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MIN_VALUE < Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LT(-Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_LT(-Double.MIN_VALUE, Double.NaN)",
                         value, (-Double.MIN_VALUE < Double.NaN));
            value = ed.exerciseDCmp_LT(-Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(-Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE < Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(-Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LT(-Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE < -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LT(-Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(-Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE < Double.MIN_VALUE));
            value = ed.exerciseDCmp_LT(-Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LT(-Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE < -Double.MIN_VALUE));
        }
        {
            boolean value;

            value = ed.exerciseDCmp_LE(0.0, -0.0);
            reportPassIf("exerciseDCmp_LE(0.0, -0.0)", value, (0.0 <= -0.0));
            value = ed.exerciseDCmp_LE(-0.0, 0.0);
            reportPassIf("exerciseDCmp_LE(-0.0, 0.0)", value, (-0.0 <= 0.0));
            value = ed.exerciseDCmp_LE(5.0d, -5.0d);
            reportPassIf("exerciseDCmp_LE(5.0d, -5.0d)", value, (5.0d <= -5.0d));
            value = ed.exerciseDCmp_LE(5.0d, 5.0d);
            reportPassIf("exerciseDCmp_LE(5.0d, 5.0d)", value, (5.0d <= 5.0d));
            value = ed.exerciseDCmp_LE(-5.0d, 5.0d);
            reportPassIf("exerciseDCmp_LE(-5.0d, 5.0d)", value, (-5.0d <= 5.0d));
            value = ed.exerciseDCmp_LE(Double.NaN, 5.0d);
            reportPassIf("exerciseDCmp_LE(Double.NaN, 5.0d)", value, (Double.NaN <= 5.0d));
            value = ed.exerciseDCmp_LE(5.0d, Double.NaN);
            reportPassIf("exerciseDCmp_LE(5.0d, Double.NaN)", value, (5.0d <= Double.NaN));
            value = ed.exerciseDCmp_LE(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_LE(Double.NaN, Double.NaN)", value, (Double.NaN <= Double.NaN));

            value = ed.exerciseDCmp_LE(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(Double.POSITIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY <= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LE(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.POSITIVE_INFINITY <= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LE(Double.POSITIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_LE(Double.POSITIVE_INFINITY, Double.NaN)",
                         value, (Double.POSITIVE_INFINITY <= Double.NaN));
            value = ed.exerciseDCmp_LE(Double.POSITIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.POSITIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY <= Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(Double.POSITIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.POSITIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.POSITIVE_INFINITY <= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(Double.POSITIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.POSITIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY <= Double.MIN_VALUE));
            value = ed.exerciseDCmp_LE(Double.POSITIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.POSITIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.POSITIVE_INFINITY <= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LE(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY <= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LE(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(Double.NEGATIVE_INFINITY, Double.NEGATIVE_INFINITY)",
                         value, (Double.NEGATIVE_INFINITY <= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LE(Double.NEGATIVE_INFINITY, Double.NaN);
            reportPassIf("exerciseDCmp_LE(Double.NEGATIVE_INFINITY, Double.NaN)",
                         value, (Double.NEGATIVE_INFINITY <= Double.NaN));
            value = ed.exerciseDCmp_LE(Double.NEGATIVE_INFINITY, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.NEGATIVE_INFINITY, Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY <= Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.NEGATIVE_INFINITY, -Double.MAX_VALUE)",
                         value, (Double.NEGATIVE_INFINITY <= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(Double.NEGATIVE_INFINITY, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.NEGATIVE_INFINITY, Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY <= Double.MIN_VALUE));
            value = ed.exerciseDCmp_LE(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.NEGATIVE_INFINITY, -Double.MIN_VALUE)",
                         value, (Double.NEGATIVE_INFINITY <= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LE(Double.NaN, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(Double.NaN, Double.POSITIVE_INFINITY)",
                         value, (Double.NaN <= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LE(Double.NaN, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(Double.NaN, Double.NEGATIVE_INFINITY)",
                         value, (Double.NaN <= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LE(Double.NaN, Double.NaN);
            reportPassIf("exerciseDCmp_LE(Double.NaN, Double.NaN)",
                         value, (Double.NaN <= Double.NaN));
            value = ed.exerciseDCmp_LE(Double.NaN, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.NaN, Double.MAX_VALUE)",
                         value, (Double.NaN <= Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(Double.NaN, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.NaN, -Double.MAX_VALUE)",
                         value, (Double.NaN <= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(Double.NaN, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.NaN, Double.MIN_VALUE)",
                         value, (Double.NaN <= Double.MIN_VALUE));
            value = ed.exerciseDCmp_LE(Double.NaN, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.NaN, -Double.MIN_VALUE)",
                         value, (Double.NaN <= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LE(Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MAX_VALUE <= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LE(Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MAX_VALUE <= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LE(Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_LE(Double.MAX_VALUE, Double.NaN)",
                         value, (Double.MAX_VALUE <= Double.NaN));
            value = ed.exerciseDCmp_LE(Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE <= Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MAX_VALUE <= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE <= Double.MIN_VALUE));
            value = ed.exerciseDCmp_LE(Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MAX_VALUE <= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LE(-Double.MAX_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(-Double.MAX_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MAX_VALUE <= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LE(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(-Double.MAX_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MAX_VALUE <= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LE(-Double.MAX_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_LE(-Double.MAX_VALUE, Double.NaN)",
                         value, (-Double.MAX_VALUE <= Double.NaN));
            value = ed.exerciseDCmp_LE(-Double.MAX_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(-Double.MAX_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE <= Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(-Double.MAX_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(-Double.MAX_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MAX_VALUE <= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(-Double.MAX_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(-Double.MAX_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE <= Double.MIN_VALUE));
            value = ed.exerciseDCmp_LE(-Double.MAX_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(-Double.MAX_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MAX_VALUE <= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LE(Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (Double.MIN_VALUE <= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LE(Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (Double.MIN_VALUE <= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LE(Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_LE(Double.MIN_VALUE, Double.NaN)",
                         value, (Double.MIN_VALUE <= Double.NaN));
            value = ed.exerciseDCmp_LE(Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE <= Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (Double.MIN_VALUE <= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE <= Double.MIN_VALUE));
            value = ed.exerciseDCmp_LE(Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (Double.MIN_VALUE <= -Double.MIN_VALUE));

            value = ed.exerciseDCmp_LE(-Double.MIN_VALUE, Double.POSITIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(-Double.MIN_VALUE, Double.POSITIVE_INFINITY)",
                         value, (-Double.MIN_VALUE <= Double.POSITIVE_INFINITY));
            value = ed.exerciseDCmp_LE(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY);
            reportPassIf("exerciseDCmp_LE(-Double.MIN_VALUE, Double.NEGATIVE_INFINITY)",
                         value, (-Double.MIN_VALUE <= Double.NEGATIVE_INFINITY));
            value = ed.exerciseDCmp_LE(-Double.MIN_VALUE, Double.NaN);
            reportPassIf("exerciseDCmp_LE(-Double.MIN_VALUE, Double.NaN)",
                         value, (-Double.MIN_VALUE <= Double.NaN));
            value = ed.exerciseDCmp_LE(-Double.MIN_VALUE, Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(-Double.MIN_VALUE, Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE <= Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(-Double.MIN_VALUE, -Double.MAX_VALUE);
            reportPassIf("exerciseDCmp_LE(-Double.MIN_VALUE, -Double.MAX_VALUE)",
                         value, (-Double.MIN_VALUE <= -Double.MAX_VALUE));
            value = ed.exerciseDCmp_LE(-Double.MIN_VALUE, Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(-Double.MIN_VALUE, Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE <= Double.MIN_VALUE));
            value = ed.exerciseDCmp_LE(-Double.MIN_VALUE, -Double.MIN_VALUE);
            reportPassIf("exerciseDCmp_LE(-Double.MIN_VALUE, -Double.MIN_VALUE)",
                         value, (-Double.MIN_VALUE <= -Double.MIN_VALUE));
        }

        {
            double value = ed.exerciseDConst0();
            reportPassIf("exerciseDConst0()", value, 0.0d);
        }
        {
            double value = ed.exerciseDConst1();
            reportPassIf("exerciseDConst1()", value, 1.0d);
        }
        {
            double value = ed.exerciseDDiv(3219874324.214918d, 9856322.434534d);
            reportPassIf("exerciseDDiv(3219874324.214918d, 9856322.434534d)",
                         value, (3219874324.214918d / 9856322.434534d));
        }
        {
            double value = ed.exerciseDLoad0(5.0d);
            reportPassIf("exerciseDLoad0(5.0d)", value, 5.0d);
        }
        {
            double value = ed.exerciseDLoad1(0, 5.0d);
            reportPassIf("exerciseDLoad1(0, 5.0d)", value, 5.0d);
        }
        {
            double value = ed.exerciseDLoad2(0, 0, 5.0d);
            reportPassIf("exerciseDLoad2(0, 0, 5.0d)", value, 5.0d);
        }
        {
            double value = ed.exerciseDLoad3(0, 0, 0, 5.0d);
            reportPassIf("exerciseDLoad3(0, 0, 0, 5.0d)", value, 5.0d);
        }
        {
            double value = ed.exerciseDLoad(0, 0, 0, 0, 5.0d);
            reportPassIf("exerciseDLoad(0, 0, 0, 0, 5.0d)", value, 5.0d);
        }
        {
            double value = ed.exerciseDMul(3219874324.214918d, 9856322.434534d);
            reportPassIf("exerciseDMul(3219874324.214918d, 9856322.434534d)",
                         value, (3219874324.214918d * 9856322.434534d));
        }
        {
            double value = ed.exerciseDNeg(5.0d);
            reportPassIf("exerciseDNeg(5.0d)", value, -5.0d);
        }
        {
            double value = ed.exerciseDRem(3219874324.214918d, 9856322.434534d);
            reportPassIf("exerciseDRem(3219874324.214918d, 9856322.434534d)",
                         value, (3219874324.214918d % 9856322.434534d));
        }
        {
            double value = ed.exerciseDReturn(5.0d);
            reportPassIf("exerciseDReturn(5.0d)", value, 5.0d);
        }
        {
            double value = ed.exerciseDStore(0, 0, 0, 0, 0.0d);
            reportPassIf("exerciseDStore(0, 0, 0, 0, 0.0d)", value, 1.0d);
        }
        {
            double value = ed.exerciseDStore0(0.0d);
            reportPassIf("exerciseDStore0(0.0d)", value, 1.0d);
        }
        {
            double value = ed.exerciseDStore1(0, 0.0d);
            reportPassIf("exerciseDStore1(0, 0.0d)", value, 1.0d);
        }
        {
            double value = ed.exerciseDStore2(0, 0, 0.0d);
            reportPassIf("exerciseDStore2(0, 0, 0.0d)", value, 1.0d);
        }
        {
            double value = ed.exerciseDStore3(0, 0, 0, 0.0d);
            reportPassIf("exerciseDStore3(0, 0, 0, 0.0d)", value, 1.0d);
        }
        {
            double value = ed.exerciseDSub(3219874324.214918d, 9856322.434534d);
            reportPassIf("exerciseDSub(3219874324.214918d, 9856322.434534d)",
                         value, (3219874324.214918d - 9856322.434534d));
        }
        if (verbose) {
            System.out.println("");
        }

        // Exercise the Array opcodes:
        System.out.println("Testing Array Opcodes:");
        ExerciseArrayOpcodes ena = new ExerciseArrayOpcodes();
        {
            boolean[] value = ena.exerciseNewArrayBoolean(5);
            reportPassIf("exerciseNewArrayBoolean(5)",
                         (value instanceof boolean[]) && (value.length == 5));
        }
        {
            byte[] value = ena.exerciseNewArrayByte(5);
            reportPassIf("exerciseNewArrayByte(5)",
                         (value instanceof byte[]) && (value.length == 5));
        }
        {
            char[] value = ena.exerciseNewArrayChar(5);
            reportPassIf("exerciseNewArrayChar(5)",
                         (value instanceof char[]) && (value.length == 5));
        }
        {
            short[] value = ena.exerciseNewArrayShort(5);
            reportPassIf("exerciseNewArrayShort(5)",
                         (value instanceof short[]) && (value.length == 5));
        }
        {
            int[] value = ena.exerciseNewArrayInt(5);
            reportPassIf("exerciseNewArrayInt(5)",
                         (value instanceof int[]) && (value.length == 5));
        }
        {
            float[] value = ena.exerciseNewArrayFloat(5);
            reportPassIf("exerciseNewArrayFloat(5)",
                         (value instanceof float[]) && (value.length == 5));
        }
        {
            long[] value = ena.exerciseNewArrayLong(5);
            reportPassIf("exerciseNewArrayLong(5)",
                         (value instanceof long[]) && (value.length == 5));
        }
        {
            double[] value = ena.exerciseNewArrayDouble(5);
            reportPassIf("exerciseNewArrayDouble(5)",
                         (value instanceof double[]) && (value.length == 5));
        }
        {
            Object[] value = ena.exerciseNewArrayObject(5);
            reportPassIf("exerciseNewArrayObject(5)",
                         (value instanceof Object[]) && (value.length == 5));
        }

        {
            boolean[] ar = new boolean[5];
            final boolean expected = true;
            ar[3] = expected;
            boolean value = ena.exerciseGetArrayElementBoolean(ar, 3);
            reportPassIf("exerciseGetArrayElementBoolean(ar, 3)",
                         value, expected);
        }
        {
            byte[] ar = new byte[5];
            final byte expected = 78;
            ar[3] = expected;
            byte value = ena.exerciseGetArrayElementByte(ar, 3);
            reportPassIf("exerciseGetArrayElementByte(ar, 3)",
                         value, expected);
        }
        {
            byte[] ar = new byte[5];
            final byte expected = 78;
            ar[3] = expected;
            byte value = ena.exerciseGetArrayElementByte3(ar);
            reportPassIf("exerciseGetArrayElementByte3(ar)",
                         value, expected);
        }
        {
            char[] ar = new char[5];
            final char expected = 'A';
            ar[3] = expected;
            char value = ena.exerciseGetArrayElementChar(ar, 3);
            reportPassIf("exerciseGetArrayElementChar(ar, 3)",
                         value, expected);
        }
        {
            short[] ar = new short[5];
            final short expected = 17062;
            ar[3] = expected;
            short value = ena.exerciseGetArrayElementShort(ar, 3);
            reportPassIf("exerciseGetArrayElementShort(ar, 3)",
                         value, expected);
        }
        {
            int[] ar = new int[5];
            final int expected = 110005;
            ar[3] = expected;
            int value = ena.exerciseGetArrayElementInt(ar, 3);
            reportPassIf("exerciseGetArrayElementInt(ar, 3)",
                         value, expected);
        }
        {
            float[] ar = new float[5];
            final float expected = 63213.3263f;
            ar[3] = expected;
            float value = ena.exerciseGetArrayElementFloat(ar, 3);
            reportPassIf("exerciseGetArrayElementFloat(ar, 3)",
                         value, expected);
        }
        {
            long[] ar = new long[5];
            final long expected = 251873327393123l;
            ar[3] = expected;
            long value = ena.exerciseGetArrayElementLong(ar, 3);
            reportPassIf("exerciseGetArrayElementLong(ar, 3)",
                         value, expected);
        }
        {
            double[] ar = new double[5];
            final double expected = 3219874324.214918d;
            ar[3] = expected;
            double value = ena.exerciseGetArrayElementDouble(ar, 3);
            reportPassIf("exerciseGetArrayElementDouble(ar, 3)",
                         value, expected);
        }
        {
            Object[] ar = new Object[5];
            final Object expected = new Object();
            ar[3] = expected;
            Object value = ena.exerciseGetArrayElementObject(ar, 3);
            reportPassIf("exerciseGetArrayElementObject(ar, 3)",
                         value, expected);
        }

        {
            boolean[] ar = new boolean[5];
            final boolean expected = true;
            ena.exerciseSetArrayElementBoolean(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementBoolean(ar, 3, expected)",
                         ar[3], expected);
        }
        {
            byte[] ar = new byte[5];
            final byte expected = 78;
            ena.exerciseSetArrayElementByte(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementByte(ar, 3, expected)",
                         ar[3], expected);
        }
        {
            byte[] ar = new byte[5];
            final int expected = 79;
            ena.exerciseSetArrayElementByteI2B(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementByteI2B(ar, 3, expected)",
                         ar[3], (byte)expected);
        }
        {
            char[] ar = new char[5];
            final char expected = 'A';
            ena.exerciseSetArrayElementChar(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementChar(ar, 3, expected)",
                         ar[3], expected);
        }
        {
            char[] ar = new char[5];
            final int expected = (int)'B';
            ena.exerciseSetArrayElementCharI2C(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementChar(ar, 3, expected)",
                         ar[3], (char)expected);
        }
        {
            short[] ar = new short[5];
            final short expected = 17062;
            ena.exerciseSetArrayElementShort(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementShort(ar, 3, expected)",
                         ar[3], expected);
        }
        {
            short[] ar = new short[5];
            final int expected = 17063;
            ena.exerciseSetArrayElementShortI2S(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementShort(ar, 3, expected)",
                         ar[3], (short)expected);
        }
        {
            int[] ar = new int[5];
            final int expected = 110005;
            ena.exerciseSetArrayElementInt(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementInt(ar, 3, expected)",
                         ar[3], expected);
        }
        {
            float[] ar = new float[5];
            final float expected = 63213.3263f;
            ena.exerciseSetArrayElementFloat(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementFloat(ar, 3, expected)",
                         ar[3], expected);
        }
        {
            long[] ar = new long[5];
            final long expected = 251873327393123l;
            ena.exerciseSetArrayElementLong(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementLong(ar, 3, expected)",
                         ar[3], expected);
        }
        {
            double[] ar = new double[5];
            final double expected = 3219874324.214918d;
            ena.exerciseSetArrayElementDouble(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementDouble(ar, 3, expected)",
                         ar[3], expected);
        }
        {
            Object[] ar = new Object[5];
            final Object expected = new Object();
            ena.exerciseSetArrayElementObject(ar, 3, expected);
            reportPassIf("exerciseSetArrayElementObject(ar, 3, expected)",
                         ar[3], expected);
        }

        {
            Object value = ena.exerciseMultiANewArray255();
            Object expected = (Object)new Object
                [1/*001*/][1/*002*/][1/*003*/][1/*004*/][1/*005*/]
                [1/*006*/][1/*007*/][1/*008*/][1/*009*/][1/*010*/]
                [1/*011*/][1/*012*/][1/*013*/][1/*014*/][1/*015*/]
                [1/*016*/][1/*017*/][1/*018*/][1/*019*/][1/*020*/]
                [1/*021*/][1/*022*/][1/*023*/][1/*024*/][1/*025*/]
                [1/*026*/][1/*027*/][1/*028*/][1/*029*/][1/*030*/]
                [1/*031*/][1/*032*/][1/*033*/][1/*034*/][1/*035*/]
                [1/*036*/][1/*037*/][1/*038*/][1/*039*/][1/*040*/]
                [1/*041*/][1/*042*/][1/*043*/][1/*044*/][1/*045*/]
                [1/*046*/][1/*047*/][1/*048*/][1/*049*/][1/*050*/]
                [1/*051*/][1/*052*/][1/*053*/][1/*054*/][1/*055*/]
                [1/*056*/][1/*057*/][1/*058*/][1/*059*/][1/*060*/]
                [1/*061*/][1/*062*/][1/*063*/][1/*064*/][1/*065*/]
                [1/*066*/][1/*067*/][1/*068*/][1/*069*/][1/*070*/]
                [1/*071*/][1/*072*/][1/*073*/][1/*074*/][1/*075*/]
                [1/*076*/][1/*077*/][1/*078*/][1/*079*/][1/*080*/]
                [1/*081*/][1/*082*/][1/*083*/][1/*084*/][1/*085*/]
                [1/*086*/][1/*087*/][1/*088*/][1/*089*/][1/*090*/]
                [1/*091*/][1/*092*/][1/*093*/][1/*094*/][1/*095*/]
                [1/*096*/][1/*097*/][1/*098*/][1/*099*/][1/*100*/]
                [1/*101*/][1/*102*/][1/*103*/][1/*104*/][1/*105*/]
                [1/*106*/][1/*107*/][1/*108*/][1/*109*/][1/*110*/]
                [1/*111*/][1/*112*/][1/*113*/][1/*114*/][1/*115*/]
                [1/*116*/][1/*117*/][1/*118*/][1/*119*/][1/*120*/]
                [1/*121*/][1/*122*/][1/*123*/][1/*124*/][1/*125*/]
                [1/*126*/][1/*127*/][1/*128*/];
//                [1/*129*/][1/*130*/]
//                [1/*131*/][1/*132*/][1/*133*/][1/*134*/][1/*135*/]
//                [1/*136*/][1/*137*/][1/*138*/][1/*139*/][1/*140*/]
//                [1/*141*/][1/*142*/][1/*143*/][1/*144*/][1/*145*/]
//                [1/*146*/][1/*147*/][1/*148*/][1/*149*/][1/*150*/]
//                [1/*151*/][1/*152*/][1/*153*/][1/*154*/][1/*155*/]
//                [1/*156*/][1/*157*/][1/*158*/][1/*159*/][1/*160*/]
//                [1/*161*/][1/*162*/][1/*163*/][1/*164*/][1/*165*/]
//                [1/*166*/][1/*167*/][1/*168*/][1/*169*/][1/*170*/]
//                [1/*171*/][1/*172*/][1/*173*/][1/*174*/][1/*175*/]
//                [1/*176*/][1/*177*/][1/*178*/][1/*179*/][1/*180*/]
//                [1/*181*/][1/*182*/][1/*183*/][1/*184*/][1/*185*/]
//                [1/*186*/][1/*187*/][1/*188*/][1/*189*/][1/*190*/]
//                [1/*191*/][1/*192*/][1/*193*/][1/*194*/][1/*195*/]
//                [1/*196*/][1/*197*/][1/*198*/][1/*199*/][1/*200*/]
//                [1/*201*/][1/*202*/][1/*203*/][1/*204*/][1/*205*/]
//                [1/*206*/][1/*207*/][1/*208*/][1/*209*/][1/*210*/]
//                [1/*211*/][1/*212*/][1/*213*/][1/*214*/][1/*215*/]
//                [1/*216*/][1/*217*/][1/*218*/][1/*219*/][1/*220*/]
//                [1/*221*/][1/*222*/][1/*223*/][1/*224*/][1/*225*/]
//                [1/*226*/][1/*227*/][1/*228*/][1/*229*/][1/*230*/]
//                [1/*231*/][1/*232*/][1/*233*/][1/*234*/][1/*235*/]
//                [1/*236*/][1/*237*/][1/*238*/][1/*239*/][1/*240*/]
//                [1/*241*/][1/*242*/][1/*243*/][1/*244*/][1/*245*/]
//                [1/*246*/][1/*247*/][1/*248*/][1/*249*/][1/*250*/]
//                [1/*251*/][1/*252*/][1/*253*/][1/*254*/][1/*255*/]
//                ;

            Object current = value;
            boolean success = true;
            //for (int i = 0; i < 255; i++) {
            for (int i = 0; i < 128; i++) {
                success = success &&
                    (java.lang.reflect.Array.getLength(current) == 1);
                current = java.lang.reflect.Array.get(current, 0);
            }

            reportPassIf("exerciseMultiANewArray255()",
                         (value != null) &&
                         expected.getClass().isInstance(value) && success);
        }
        if (verbose) {
            System.out.println("");
        }
    }

    static void exerciseIDivOpcodes() {
        // Exercise the IDiv opcodes:
        System.out.println("Testing IDiv Opcodes:");
        ExerciseIntOpcodes ei = new ExerciseIntOpcodes();

        {
            boolean exceptionThrown = false;
            int value = 0;
            try {
                value = ei.exerciseIDiv(100, 5);
            } catch (ArithmeticException e) {
                exceptionThrown = true;
            }
            reportPassIf("exerciseIDiv(100, 5)",
                         (value == 100/5) && !exceptionThrown);

            for (int i = 0; i < dividends.length; i++) {

                value = ei.exerciseIDivBy1(dividends[i]);
                reportPassIf("exerciseIDivBy1(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/1);

                value = ei.exerciseIDivBy2(dividends[i]);
                reportPassIf("exerciseIDivBy2(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/2);

                value = ei.exerciseIDivBy4(dividends[i]);
                reportPassIf("exerciseIDivBy4(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/4);

                value = ei.exerciseIDivBy8(dividends[i]);
                reportPassIf("exerciseIDivBy8(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/8);

                value = ei.exerciseIDivBy16(dividends[i]);
                reportPassIf("exerciseIDivBy16(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/16);

                value = ei.exerciseIDivBy32(dividends[i]);
                reportPassIf("exerciseIDivBy32(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/32);

                value = ei.exerciseIDivBy64(dividends[i]);
                reportPassIf("exerciseIDivBy64(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/64);

                value = ei.exerciseIDivBy128(dividends[i]);
                reportPassIf("exerciseIDivBy128(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/128);

                value = ei.exerciseIDivBy256(dividends[i]);
                reportPassIf("exerciseIDivBy256(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/256);

                value = ei.exerciseIDivBy512(dividends[i]);
                reportPassIf("exerciseIDivBy512(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/512);

                value = ei.exerciseIDivBy1024(dividends[i]);
                reportPassIf("exerciseIDivBy1024(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/1024);

                value = ei.exerciseIDivBy2048(dividends[i]);
                reportPassIf("exerciseIDivBy2048(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/2048);

                value = ei.exerciseIDivBy4096(dividends[i]);
                reportPassIf("exerciseIDivBy4096(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/4096);

                value = ei.exerciseIDivBy8192(dividends[i]);
                reportPassIf("exerciseIDivBy8192(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/8192);

                value = ei.exerciseIDivBy16384(dividends[i]);
                reportPassIf("exerciseIDivBy16384(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/16384);

                value = ei.exerciseIDivBy32768(dividends[i]);
                reportPassIf("exerciseIDivBy32768(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/32768);

                value = ei.exerciseIDivBy65536(dividends[i]);
                reportPassIf("exerciseIDivBy65536(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/65536);

                value = ei.exerciseIDivBy131072(dividends[i]);
                reportPassIf("exerciseIDivBy131072(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/131072);

                value = ei.exerciseIDivBy262144(dividends[i]);
                reportPassIf("exerciseIDivBy262144(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/262144);

                value = ei.exerciseIDivBy524288(dividends[i]);
                reportPassIf("exerciseIDivBy524288(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/524288);

                value = ei.exerciseIDivBy1048576(dividends[i]);
                reportPassIf("exerciseIDivBy1048576(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/1048576);

                value = ei.exerciseIDivBy2097152(dividends[i]);
                reportPassIf("exerciseIDivBy2097152(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/2097152);

                value = ei.exerciseIDivBy4194304(dividends[i]);
                reportPassIf("exerciseIDivBy4194304(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/4194304);

                value = ei.exerciseIDivBy8388608(dividends[i]);
                reportPassIf("exerciseIDivBy8388608(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/8388608);

                value = ei.exerciseIDivBy16777216(dividends[i]);
                reportPassIf("exerciseIDivBy16777216(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/16777216);

                value = ei.exerciseIDivBy33554432(dividends[i]);
                reportPassIf("exerciseIDivBy33554432(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/33554432);

                value = ei.exerciseIDivBy67108864(dividends[i]);
                reportPassIf("exerciseIDivBy67108864(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/67108864);

                value = ei.exerciseIDivBy134217728(dividends[i]);
                reportPassIf("exerciseIDivBy134217728(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/134217728);

                value = ei.exerciseIDivBy268435456(dividends[i]);
                reportPassIf("exerciseIDivBy268435456(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/268435456);

                value = ei.exerciseIDivBy536870912(dividends[i]);
                reportPassIf("exerciseIDivBy536870912(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/536870912);

                value = ei.exerciseIDivBy1073741824(dividends[i]);
                reportPassIf("exerciseIDivBy1073741824(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/1073741824);

                value = ei.exerciseIDivBy3(dividends[i]);
                reportPassIf("exerciseIDivBy3(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/3);

                value = ei.exerciseIDivBy5(dividends[i]);
                reportPassIf("exerciseIDivBy5(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/5);

                value = ei.exerciseIDivBy6(dividends[i]);
                reportPassIf("exerciseIDivBy6(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/6);

                value = ei.exerciseIDivBy7(dividends[i]);
                reportPassIf("exerciseIDivBy7(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/7);

                value = ei.exerciseIDivBy9(dividends[i]);
                reportPassIf("exerciseIDivBy9(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/9);

                value = ei.exerciseIDivBy10(dividends[i]);
                reportPassIf("exerciseIDivBy10(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/10);

                value = ei.exerciseIDivBy17(dividends[i]);
                reportPassIf("exerciseIDivBy17(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/17);

                value = ei.exerciseIDivBy100(dividends[i]);
                reportPassIf("exerciseIDivBy100(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/100);

                value = ei.exerciseIDivBy125(dividends[i]);
                reportPassIf("exerciseIDivBy125(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/125);

                value = ei.exerciseIDivBy1027(dividends[i]);
                reportPassIf("exerciseIDivBy1027(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/1027);

                value = ei.exerciseIDivBy5612712(dividends[i]);
                reportPassIf("exerciseIDivBy5612712(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/5612712);

                value = ei.exerciseIDivBy0x7fffffff(dividends[i]);
                reportPassIf("exerciseIDivBy0x7fffffff(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/0x7fffffff);

                value = ei.exerciseIDivByM1(dividends[i]);
                reportPassIf("exerciseIDivByM1(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-1);

                value = ei.exerciseIDivByM2(dividends[i]);
                reportPassIf("exerciseIDivByM2(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-2);

                value = ei.exerciseIDivByM4(dividends[i]);
                reportPassIf("exerciseIDivByM4(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-4);

                value = ei.exerciseIDivByM8(dividends[i]);
                reportPassIf("exerciseIDivByM8(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-8);

                value = ei.exerciseIDivByM16(dividends[i]);
                reportPassIf("exerciseIDivByM16(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-16);

                value = ei.exerciseIDivByM32(dividends[i]);
                reportPassIf("exerciseIDivByM32(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-32);

                value = ei.exerciseIDivByM64(dividends[i]);
                reportPassIf("exerciseIDivByM64(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-64);

                value = ei.exerciseIDivByM128(dividends[i]);
                reportPassIf("exerciseIDivByM128(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-128);

                value = ei.exerciseIDivByM256(dividends[i]);
                reportPassIf("exerciseIDivByM256(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-256);

                value = ei.exerciseIDivByM512(dividends[i]);
                reportPassIf("exerciseIDivByM512(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-512);

                value = ei.exerciseIDivByM1024(dividends[i]);
                reportPassIf("exerciseIDivByM1024(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-1024);

                value = ei.exerciseIDivByM2048(dividends[i]);
                reportPassIf("exerciseIDivByM2048(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-2048);

                value = ei.exerciseIDivByM4096(dividends[i]);
                reportPassIf("exerciseIDivByM4096(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-4096);

                value = ei.exerciseIDivByM8192(dividends[i]);
                reportPassIf("exerciseIDivByM8192(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-8192);

                value = ei.exerciseIDivByM16384(dividends[i]);
                reportPassIf("exerciseIDivByM16384(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-16384);

                value = ei.exerciseIDivByM32768(dividends[i]);
                reportPassIf("exerciseIDivByM32768(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-32768);

                value = ei.exerciseIDivByM65536(dividends[i]);
                reportPassIf("exerciseIDivByM65536(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-65536);

                value = ei.exerciseIDivByM131072(dividends[i]);
                reportPassIf("exerciseIDivByM131072(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-131072);

                value = ei.exerciseIDivByM262144(dividends[i]);
                reportPassIf("exerciseIDivByM262144(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-262144);

                value = ei.exerciseIDivByM524288(dividends[i]);
                reportPassIf("exerciseIDivByM524288(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-524288);

                value = ei.exerciseIDivByM1048576(dividends[i]);
                reportPassIf("exerciseIDivByM1048576(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-1048576);

                value = ei.exerciseIDivByM2097152(dividends[i]);
                reportPassIf("exerciseIDivByM2097152(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-2097152);

                value = ei.exerciseIDivByM4194304(dividends[i]);
                reportPassIf("exerciseIDivByM4194304(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-4194304);

                value = ei.exerciseIDivByM8388608(dividends[i]);
                reportPassIf("exerciseIDivByM8388608(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-8388608);

                value = ei.exerciseIDivByM16777216(dividends[i]);
                reportPassIf("exerciseIDivByM16777216(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-16777216);

                value = ei.exerciseIDivByM33554432(dividends[i]);
                reportPassIf("exerciseIDivByM33554432(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-33554432);

                value = ei.exerciseIDivByM67108864(dividends[i]);
                reportPassIf("exerciseIDivByM67108864(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-67108864);

                value = ei.exerciseIDivByM134217728(dividends[i]);
                reportPassIf("exerciseIDivByM134217728(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-134217728);

                value = ei.exerciseIDivByM268435456(dividends[i]);
                reportPassIf("exerciseIDivByM268435456(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-268435456);

                value = ei.exerciseIDivByM536870912(dividends[i]);
                reportPassIf("exerciseIDivByM536870912(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-536870912);

                value = ei.exerciseIDivByM1073741824(dividends[i]);
                reportPassIf("exerciseIDivByM1073741824(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-1073741824);

                value = ei.exerciseIDivByM2147483648(dividends[i]);
                reportPassIf("exerciseIDivByM2147483648(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-2147483648);

                value = ei.exerciseIDivByM3(dividends[i]);
                reportPassIf("exerciseIDivByM3(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-3);

                value = ei.exerciseIDivByM5(dividends[i]);
                reportPassIf("exerciseIDivByM5(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-5);

                value = ei.exerciseIDivByM6(dividends[i]);
                reportPassIf("exerciseIDivByM6(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-6);

                value = ei.exerciseIDivByM7(dividends[i]);
                reportPassIf("exerciseIDivByM7(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-7);

                value = ei.exerciseIDivByM9(dividends[i]);
                reportPassIf("exerciseIDivByM9(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-9);

                value = ei.exerciseIDivByM10(dividends[i]);
                reportPassIf("exerciseIDivByM10(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-10);

                value = ei.exerciseIDivByM17(dividends[i]);
                reportPassIf("exerciseIDivByM17(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-17);

                value = ei.exerciseIDivByM100(dividends[i]);
                reportPassIf("exerciseIDivByM100(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-100);

                value = ei.exerciseIDivByM125(dividends[i]);
                reportPassIf("exerciseIDivByM125(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-125);

                value = ei.exerciseIDivByM1027(dividends[i]);
                reportPassIf("exerciseIDivByM1027(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-1027);

                value = ei.exerciseIDivByM5612712(dividends[i]);
                reportPassIf("exerciseIDivByM5612712(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-5612712);

                value = ei.exerciseIDivByM0x7fffffff(dividends[i]);
                reportPassIf("exerciseIDivByM0x7fffffff(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]/-0x7fffffff);
            }
        }
        {
            boolean exceptionThrown = false;
            int value = 0;
            try {
                value = ei.exerciseIDiv(0, 5);
            } catch (ArithmeticException e) {
                exceptionThrown = true;
            }
            reportPassIf("exerciseIDiv(0, 5)",
                         (value == 0) && !exceptionThrown);
        }
        {
            boolean exceptionThrown = false;
            int value = 0;
            try {
                value = ei.exerciseIDiv(100, 0);
            } catch (ArithmeticException e) {
                exceptionThrown = true;
            }
            reportPassIf("exerciseIDiv(100, 0)", exceptionThrown);
        }
        {
            boolean exceptionThrown = false;
            int value = 0;
            try {
                value = ei.exerciseIDiv2(100, 0);
            } catch (ArithmeticException e) {
                exceptionThrown = true;
            }
            reportPassIf("exerciseIDiv2(100, 0)", exceptionThrown);
        }

        if (verbose) {
            System.out.println("");
        }
    }

    static void exerciseIRemOpcodes() {
        // Exercise the IRem opcodes:
        System.out.println("Testing IRem Opcodes:");
        ExerciseIntOpcodes ei = new ExerciseIntOpcodes();
        {
            boolean exceptionThrown = false;
            int value = 0;
            try {
                value = ei.exerciseIRem(100, 5);
            } catch (ArithmeticException e) {
                exceptionThrown = true;
            }
            reportPassIf("exerciseIRem(100, 5)",
                         (value == 100%5) && !exceptionThrown);

            for (int i = 0; i < dividends.length; i++) {

                value = ei.exerciseIRemBy1(dividends[i]);
                reportPassIf("exerciseIRemBy1(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%1);

                value = ei.exerciseIRemBy2(dividends[i]);
                reportPassIf("exerciseIRemBy2(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%2);

                value = ei.exerciseIRemBy4(dividends[i]);
                reportPassIf("exerciseIRemBy4(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%4);

                value = ei.exerciseIRemBy8(dividends[i]);
                reportPassIf("exerciseIRemBy8(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%8);

                value = ei.exerciseIRemBy16(dividends[i]);
                reportPassIf("exerciseIRemBy16(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%16);

                value = ei.exerciseIRemBy32(dividends[i]);
                reportPassIf("exerciseIRemBy32(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%32);

                value = ei.exerciseIRemBy64(dividends[i]);
                reportPassIf("exerciseIRemBy64(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%64);

                value = ei.exerciseIRemBy128(dividends[i]);
                reportPassIf("exerciseIRemBy128(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%128);

                value = ei.exerciseIRemBy256(dividends[i]);
                reportPassIf("exerciseIRemBy256(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%256);

                value = ei.exerciseIRemBy512(dividends[i]);
                reportPassIf("exerciseIRemBy512(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%512);

                value = ei.exerciseIRemBy1024(dividends[i]);
                reportPassIf("exerciseIRemBy1024(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%1024);

                value = ei.exerciseIRemBy2048(dividends[i]);
                reportPassIf("exerciseIRemBy2048(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%2048);

                value = ei.exerciseIRemBy4096(dividends[i]);
                reportPassIf("exerciseIRemBy4096(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%4096);

                value = ei.exerciseIRemBy8192(dividends[i]);
                reportPassIf("exerciseIRemBy8192(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%8192);

                value = ei.exerciseIRemBy16384(dividends[i]);
                reportPassIf("exerciseIRemBy16384(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%16384);

                value = ei.exerciseIRemBy32768(dividends[i]);
                reportPassIf("exerciseIRemBy32768(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%32768);

                value = ei.exerciseIRemBy65536(dividends[i]);
                reportPassIf("exerciseIRemBy65536(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%65536);

                value = ei.exerciseIRemBy131072(dividends[i]);
                reportPassIf("exerciseIRemBy131072(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%131072);

                value = ei.exerciseIRemBy262144(dividends[i]);
                reportPassIf("exerciseIRemBy262144(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%262144);

                value = ei.exerciseIRemBy524288(dividends[i]);
                reportPassIf("exerciseIRemBy524288(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%524288);

                value = ei.exerciseIRemBy1048576(dividends[i]);
                reportPassIf("exerciseIRemBy1048576(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%1048576);

                value = ei.exerciseIRemBy2097152(dividends[i]);
                reportPassIf("exerciseIRemBy2097152(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%2097152);

                value = ei.exerciseIRemBy4194304(dividends[i]);
                reportPassIf("exerciseIRemBy4194304(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%4194304);

                value = ei.exerciseIRemBy8388608(dividends[i]);
                reportPassIf("exerciseIRemBy8388608(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%8388608);

                value = ei.exerciseIRemBy16777216(dividends[i]);
                reportPassIf("exerciseIRemBy16777216(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%16777216);

                value = ei.exerciseIRemBy33554432(dividends[i]);
                reportPassIf("exerciseIRemBy33554432(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%33554432);

                value = ei.exerciseIRemBy67108864(dividends[i]);
                reportPassIf("exerciseIRemBy67108864(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%67108864);

                value = ei.exerciseIRemBy134217728(dividends[i]);
                reportPassIf("exerciseIRemBy134217728(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%134217728);

                value = ei.exerciseIRemBy268435456(dividends[i]);
                reportPassIf("exerciseIRemBy268435456(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%268435456);

                value = ei.exerciseIRemBy536870912(dividends[i]);
                reportPassIf("exerciseIRemBy536870912(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%536870912);

                value = ei.exerciseIRemBy1073741824(dividends[i]);
                reportPassIf("exerciseIRemBy1073741824(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%1073741824);

                value = ei.exerciseIRemBy3(dividends[i]);
                reportPassIf("exerciseIRemBy3(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%3);

                value = ei.exerciseIRemBy5(dividends[i]);
                reportPassIf("exerciseIRemBy5(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%5);

                value = ei.exerciseIRemBy6(dividends[i]);
                reportPassIf("exerciseIRemBy6(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%6);

                value = ei.exerciseIRemBy7(dividends[i]);
                reportPassIf("exerciseIRemBy7(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%7);

                value = ei.exerciseIRemBy9(dividends[i]);
                reportPassIf("exerciseIRemBy9(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%9);

                value = ei.exerciseIRemBy10(dividends[i]);
                reportPassIf("exerciseIRemBy10(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%10);

                value = ei.exerciseIRemBy17(dividends[i]);
                reportPassIf("exerciseIRemBy17(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%17);

                value = ei.exerciseIRemBy100(dividends[i]);
                reportPassIf("exerciseIRemBy100(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%100);

                value = ei.exerciseIRemBy125(dividends[i]);
                reportPassIf("exerciseIRemBy125(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%125);

                value = ei.exerciseIRemBy1027(dividends[i]);
                reportPassIf("exerciseIRemBy1027(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%1027);

                value = ei.exerciseIRemBy5612712(dividends[i]);
                reportPassIf("exerciseIRemBy5612712(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%5612712);

                value = ei.exerciseIRemBy0x7fffffff(dividends[i]);
                reportPassIf("exerciseIRemBy0x7fffffff(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%0x7fffffff);

                value = ei.exerciseIRemByM1(dividends[i]);
                reportPassIf("exerciseIRemByM1(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-1);

                value = ei.exerciseIRemByM2(dividends[i]);
                reportPassIf("exerciseIRemByM2(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-2);

                value = ei.exerciseIRemByM4(dividends[i]);
                reportPassIf("exerciseIRemByM4(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-4);

                value = ei.exerciseIRemByM8(dividends[i]);
                reportPassIf("exerciseIRemByM8(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-8);

                value = ei.exerciseIRemByM16(dividends[i]);
                reportPassIf("exerciseIRemByM16(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-16);

                value = ei.exerciseIRemByM32(dividends[i]);
                reportPassIf("exerciseIRemByM32(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-32);

                value = ei.exerciseIRemByM64(dividends[i]);
                reportPassIf("exerciseIRemByM64(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-64);

                value = ei.exerciseIRemByM128(dividends[i]);
                reportPassIf("exerciseIRemByM128(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-128);

                value = ei.exerciseIRemByM256(dividends[i]);
                reportPassIf("exerciseIRemByM256(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-256);

                value = ei.exerciseIRemByM512(dividends[i]);
                reportPassIf("exerciseIRemByM512(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-512);

                value = ei.exerciseIRemByM1024(dividends[i]);
                reportPassIf("exerciseIRemByM1024(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-1024);

                value = ei.exerciseIRemByM2048(dividends[i]);
                reportPassIf("exerciseIRemByM2048(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-2048);

                value = ei.exerciseIRemByM4096(dividends[i]);
                reportPassIf("exerciseIRemByM4096(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-4096);

                value = ei.exerciseIRemByM8192(dividends[i]);
                reportPassIf("exerciseIRemByM8192(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-8192);

                value = ei.exerciseIRemByM16384(dividends[i]);
                reportPassIf("exerciseIRemByM16384(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-16384);

                value = ei.exerciseIRemByM32768(dividends[i]);
                reportPassIf("exerciseIRemByM32768(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-32768);

                value = ei.exerciseIRemByM65536(dividends[i]);
                reportPassIf("exerciseIRemByM65536(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-65536);

                value = ei.exerciseIRemByM131072(dividends[i]);
                reportPassIf("exerciseIRemByM131072(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-131072);

                value = ei.exerciseIRemByM262144(dividends[i]);
                reportPassIf("exerciseIRemByM262144(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-262144);

                value = ei.exerciseIRemByM524288(dividends[i]);
                reportPassIf("exerciseIRemByM524288(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-524288);

                value = ei.exerciseIRemByM1048576(dividends[i]);
                reportPassIf("exerciseIRemByM1048576(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-1048576);

                value = ei.exerciseIRemByM2097152(dividends[i]);
                reportPassIf("exerciseIRemByM2097152(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-2097152);

                value = ei.exerciseIRemByM4194304(dividends[i]);
                reportPassIf("exerciseIRemByM4194304(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-4194304);

                value = ei.exerciseIRemByM8388608(dividends[i]);
                reportPassIf("exerciseIRemByM8388608(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-8388608);

                value = ei.exerciseIRemByM16777216(dividends[i]);
                reportPassIf("exerciseIRemByM16777216(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-16777216);

                value = ei.exerciseIRemByM33554432(dividends[i]);
                reportPassIf("exerciseIRemByM33554432(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-33554432);

                value = ei.exerciseIRemByM67108864(dividends[i]);
                reportPassIf("exerciseIRemByM67108864(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-67108864);

                value = ei.exerciseIRemByM134217728(dividends[i]);
                reportPassIf("exerciseIRemByM134217728(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-134217728);

                value = ei.exerciseIRemByM268435456(dividends[i]);
                reportPassIf("exerciseIRemByM268435456(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-268435456);

                value = ei.exerciseIRemByM536870912(dividends[i]);
                reportPassIf("exerciseIRemByM536870912(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-536870912);

                value = ei.exerciseIRemByM1073741824(dividends[i]);
                reportPassIf("exerciseIRemByM1073741824(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-1073741824);

                value = ei.exerciseIRemByM2147483648(dividends[i]);
                reportPassIf("exerciseIRemByM2147483648(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-2147483648);

                value = ei.exerciseIRemByM3(dividends[i]);
                reportPassIf("exerciseIRemByM3(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-3);

                value = ei.exerciseIRemByM5(dividends[i]);
                reportPassIf("exerciseIRemByM5(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-5);

                value = ei.exerciseIRemByM6(dividends[i]);
                reportPassIf("exerciseIRemByM6(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-6);

                value = ei.exerciseIRemByM7(dividends[i]);
                reportPassIf("exerciseIRemByM7(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-7);

                value = ei.exerciseIRemByM9(dividends[i]);
                reportPassIf("exerciseIRemByM9(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-9);

                value = ei.exerciseIRemByM10(dividends[i]);
                reportPassIf("exerciseIRemByM10(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-10);

                value = ei.exerciseIRemByM17(dividends[i]);
                reportPassIf("exerciseIRemByM17(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-17);

                value = ei.exerciseIRemByM100(dividends[i]);
                reportPassIf("exerciseIRemByM100(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-100);

                value = ei.exerciseIRemByM125(dividends[i]);
                reportPassIf("exerciseIRemByM125(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-125);

                value = ei.exerciseIRemByM1027(dividends[i]);
                reportPassIf("exerciseIRemByM1027(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-1027);

                value = ei.exerciseIRemByM5612712(dividends[i]);
                reportPassIf("exerciseIRemByM5612712(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-5612712);

                value = ei.exerciseIRemByM0x7fffffff(dividends[i]);
                reportPassIf("exerciseIRemByM0x7fffffff(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]%-0x7fffffff);
            }
        }
        {
            boolean exceptionThrown = false;
            int value = 0;
            try {
                value = ei.exerciseIRem(0, 5);
            } catch (ArithmeticException e) {
                exceptionThrown = true;
            }
            reportPassIf("exerciseIRem(0, 5)",
                         (value == 0) && !exceptionThrown);
        }
        {
            boolean exceptionThrown = false;
            int value = 0;
            try {
                value = ei.exerciseIRem(100, 0);
            } catch (ArithmeticException e) {
                exceptionThrown = true;
            }
            reportPassIf("exerciseIRem(100, 0)", exceptionThrown);
        }

        if (verbose) {
            System.out.println("");
        }
    }

    static void exerciseIMulOpcodes() {
        // Exercise the IMul opcodes:
        System.out.println("Testing IMul Opcodes:");
        ExerciseIntOpcodes ei = new ExerciseIntOpcodes();
        {
            int value = 0;

            value = ei.exerciseIMul(100, 5);
            reportPassIf("exerciseIMul(100, 5)", value, 100*5);

            for (int i = 0; i < dividends.length; i++) {

                value = ei.exerciseIMulBy1(dividends[i]);
                reportPassIf("exerciseIMulBy1(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*1);

                value = ei.exerciseIMulBy2(dividends[i]);
                reportPassIf("exerciseIMulBy2(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*2);

                value = ei.exerciseIMulBy4(dividends[i]);
                reportPassIf("exerciseIMulBy4(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*4);

                value = ei.exerciseIMulBy8(dividends[i]);
                reportPassIf("exerciseIMulBy8(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*8);

                value = ei.exerciseIMulBy16(dividends[i]);
                reportPassIf("exerciseIMulBy16(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*16);

                value = ei.exerciseIMulBy32(dividends[i]);
                reportPassIf("exerciseIMulBy32(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*32);

                value = ei.exerciseIMulBy64(dividends[i]);
                reportPassIf("exerciseIMulBy64(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*64);

                value = ei.exerciseIMulBy128(dividends[i]);
                reportPassIf("exerciseIMulBy128(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*128);

                value = ei.exerciseIMulBy256(dividends[i]);
                reportPassIf("exerciseIMulBy256(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*256);

                value = ei.exerciseIMulBy512(dividends[i]);
                reportPassIf("exerciseIMulBy512(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*512);

                value = ei.exerciseIMulBy1024(dividends[i]);
                reportPassIf("exerciseIMulBy1024(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*1024);

                value = ei.exerciseIMulBy2048(dividends[i]);
                reportPassIf("exerciseIMulBy2048(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*2048);

                value = ei.exerciseIMulBy4096(dividends[i]);
                reportPassIf("exerciseIMulBy4096(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*4096);

                value = ei.exerciseIMulBy8192(dividends[i]);
                reportPassIf("exerciseIMulBy8192(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*8192);

                value = ei.exerciseIMulBy16384(dividends[i]);
                reportPassIf("exerciseIMulBy16384(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*16384);

                value = ei.exerciseIMulBy32768(dividends[i]);
                reportPassIf("exerciseIMulBy32768(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*32768);

                value = ei.exerciseIMulBy65536(dividends[i]);
                reportPassIf("exerciseIMulBy65536(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*65536);

                value = ei.exerciseIMulBy131072(dividends[i]);
                reportPassIf("exerciseIMulBy131072(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*131072);

                value = ei.exerciseIMulBy262144(dividends[i]);
                reportPassIf("exerciseIMulBy262144(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*262144);

                value = ei.exerciseIMulBy524288(dividends[i]);
                reportPassIf("exerciseIMulBy524288(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*524288);

                value = ei.exerciseIMulBy1048576(dividends[i]);
                reportPassIf("exerciseIMulBy1048576(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*1048576);

                value = ei.exerciseIMulBy2097152(dividends[i]);
                reportPassIf("exerciseIMulBy2097152(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*2097152);

                value = ei.exerciseIMulBy4194304(dividends[i]);
                reportPassIf("exerciseIMulBy4194304(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*4194304);

                value = ei.exerciseIMulBy8388608(dividends[i]);
                reportPassIf("exerciseIMulBy8388608(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*8388608);

                value = ei.exerciseIMulBy16777216(dividends[i]);
                reportPassIf("exerciseIMulBy16777216(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*16777216);

                value = ei.exerciseIMulBy33554432(dividends[i]);
                reportPassIf("exerciseIMulBy33554432(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*33554432);

                value = ei.exerciseIMulBy67108864(dividends[i]);
                reportPassIf("exerciseIMulBy67108864(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*67108864);

                value = ei.exerciseIMulBy134217728(dividends[i]);
                reportPassIf("exerciseIMulBy134217728(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*134217728);

                value = ei.exerciseIMulBy268435456(dividends[i]);
                reportPassIf("exerciseIMulBy268435456(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*268435456);

                value = ei.exerciseIMulBy536870912(dividends[i]);
                reportPassIf("exerciseIMulBy536870912(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*536870912);

                value = ei.exerciseIMulBy1073741824(dividends[i]);
                reportPassIf("exerciseIMulBy1073741824(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*1073741824);

                value = ei.exerciseIMulbBy1(dividends[i]);
                reportPassIf("exerciseIMulbBy1(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*1);

                value = ei.exerciseIMulbBy2(dividends[i]);
                reportPassIf("exerciseIMulbBy2(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*2);

                value = ei.exerciseIMulbBy4(dividends[i]);
                reportPassIf("exerciseIMulbBy4(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*4);

                value = ei.exerciseIMulbBy8(dividends[i]);
                reportPassIf("exerciseIMulbBy8(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*8);

                value = ei.exerciseIMulbBy16(dividends[i]);
                reportPassIf("exerciseIMulbBy16(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*16);

                value = ei.exerciseIMulbBy32(dividends[i]);
                reportPassIf("exerciseIMulbBy32(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*32);

                value = ei.exerciseIMulbBy64(dividends[i]);
                reportPassIf("exerciseIMulbBy64(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*64);

                value = ei.exerciseIMulbBy128(dividends[i]);
                reportPassIf("exerciseIMulbBy128(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*128);

                value = ei.exerciseIMulbBy256(dividends[i]);
                reportPassIf("exerciseIMulbBy256(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*256);

                value = ei.exerciseIMulbBy512(dividends[i]);
                reportPassIf("exerciseIMulbBy512(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*512);

                value = ei.exerciseIMulbBy1024(dividends[i]);
                reportPassIf("exerciseIMulbBy1024(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*1024);

                value = ei.exerciseIMulbBy2048(dividends[i]);
                reportPassIf("exerciseIMulbBy2048(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*2048);

                value = ei.exerciseIMulbBy4096(dividends[i]);
                reportPassIf("exerciseIMulbBy4096(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*4096);

                value = ei.exerciseIMulbBy8192(dividends[i]);
                reportPassIf("exerciseIMulbBy8192(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*8192);

                value = ei.exerciseIMulbBy16384(dividends[i]);
                reportPassIf("exerciseIMulbBy16384(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*16384);

                value = ei.exerciseIMulbBy32768(dividends[i]);
                reportPassIf("exerciseIMulbBy32768(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*32768);

                value = ei.exerciseIMulbBy65536(dividends[i]);
                reportPassIf("exerciseIMulbBy65536(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*65536);

                value = ei.exerciseIMulbBy131072(dividends[i]);
                reportPassIf("exerciseIMulbBy131072(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*131072);

                value = ei.exerciseIMulbBy262144(dividends[i]);
                reportPassIf("exerciseIMulbBy262144(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*262144);

                value = ei.exerciseIMulbBy524288(dividends[i]);
                reportPassIf("exerciseIMulbBy524288(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*524288);

                value = ei.exerciseIMulbBy1048576(dividends[i]);
                reportPassIf("exerciseIMulbBy1048576(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*1048576);

                value = ei.exerciseIMulbBy2097152(dividends[i]);
                reportPassIf("exerciseIMulbBy2097152(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*2097152);

                value = ei.exerciseIMulbBy4194304(dividends[i]);
                reportPassIf("exerciseIMulbBy4194304(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*4194304);

                value = ei.exerciseIMulbBy8388608(dividends[i]);
                reportPassIf("exerciseIMulbBy8388608(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*8388608);

                value = ei.exerciseIMulbBy16777216(dividends[i]);
                reportPassIf("exerciseIMulbBy16777216(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*16777216);

                value = ei.exerciseIMulbBy33554432(dividends[i]);
                reportPassIf("exerciseIMulbBy33554432(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*33554432);

                value = ei.exerciseIMulbBy67108864(dividends[i]);
                reportPassIf("exerciseIMulbBy67108864(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*67108864);

                value = ei.exerciseIMulbBy134217728(dividends[i]);
                reportPassIf("exerciseIMulbBy134217728(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*134217728);

                value = ei.exerciseIMulbBy268435456(dividends[i]);
                reportPassIf("exerciseIMulbBy268435456(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*268435456);

                value = ei.exerciseIMulbBy536870912(dividends[i]);
                reportPassIf("exerciseIMulbBy536870912(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*536870912);

                value = ei.exerciseIMulbBy1073741824(dividends[i]);
                reportPassIf("exerciseIMulbBy1073741824(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*1073741824);

                value = ei.exerciseIMulByM1(dividends[i]);
                reportPassIf("exerciseIMulByM1(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-1);

                value = ei.exerciseIMulByM2(dividends[i]);
                reportPassIf("exerciseIMulByM2(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-2);

                value = ei.exerciseIMulByM4(dividends[i]);
                reportPassIf("exerciseIMulByM4(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-4);

                value = ei.exerciseIMulByM8(dividends[i]);
                reportPassIf("exerciseIMulByM8(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-8);

                value = ei.exerciseIMulByM16(dividends[i]);
                reportPassIf("exerciseIMulByM16(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-16);

                value = ei.exerciseIMulByM32(dividends[i]);
                reportPassIf("exerciseIMulByM32(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-32);

                value = ei.exerciseIMulByM64(dividends[i]);
                reportPassIf("exerciseIMulByM64(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-64);

                value = ei.exerciseIMulByM128(dividends[i]);
                reportPassIf("exerciseIMulByM128(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-128);

                value = ei.exerciseIMulByM256(dividends[i]);
                reportPassIf("exerciseIMulByM256(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-256);

                value = ei.exerciseIMulByM512(dividends[i]);
                reportPassIf("exerciseIMulByM512(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-512);

                value = ei.exerciseIMulByM1024(dividends[i]);
                reportPassIf("exerciseIMulByM1024(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-1024);

                value = ei.exerciseIMulByM2048(dividends[i]);
                reportPassIf("exerciseIMulByM2048(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-2048);

                value = ei.exerciseIMulByM4096(dividends[i]);
                reportPassIf("exerciseIMulByM4096(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-4096);

                value = ei.exerciseIMulByM8192(dividends[i]);
                reportPassIf("exerciseIMulByM8192(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-8192);

                value = ei.exerciseIMulByM16384(dividends[i]);
                reportPassIf("exerciseIMulByM16384(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-16384);

                value = ei.exerciseIMulByM32768(dividends[i]);
                reportPassIf("exerciseIMulByM32768(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-32768);

                value = ei.exerciseIMulByM65536(dividends[i]);
                reportPassIf("exerciseIMulByM65536(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-65536);

                value = ei.exerciseIMulByM131072(dividends[i]);
                reportPassIf("exerciseIMulByM131072(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-131072);

                value = ei.exerciseIMulByM262144(dividends[i]);
                reportPassIf("exerciseIMulByM262144(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-262144);

                value = ei.exerciseIMulByM524288(dividends[i]);
                reportPassIf("exerciseIMulByM524288(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-524288);

                value = ei.exerciseIMulByM1048576(dividends[i]);
                reportPassIf("exerciseIMulByM1048576(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-1048576);

                value = ei.exerciseIMulByM2097152(dividends[i]);
                reportPassIf("exerciseIMulByM2097152(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-2097152);

                value = ei.exerciseIMulByM4194304(dividends[i]);
                reportPassIf("exerciseIMulByM4194304(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-4194304);

                value = ei.exerciseIMulByM8388608(dividends[i]);
                reportPassIf("exerciseIMulByM8388608(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-8388608);

                value = ei.exerciseIMulByM16777216(dividends[i]);
                reportPassIf("exerciseIMulByM16777216(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-16777216);

                value = ei.exerciseIMulByM33554432(dividends[i]);
                reportPassIf("exerciseIMulByM33554432(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-33554432);

                value = ei.exerciseIMulByM67108864(dividends[i]);
                reportPassIf("exerciseIMulByM67108864(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-67108864);

                value = ei.exerciseIMulByM134217728(dividends[i]);
                reportPassIf("exerciseIMulByM134217728(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-134217728);

                value = ei.exerciseIMulByM268435456(dividends[i]);
                reportPassIf("exerciseIMulByM268435456(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-268435456);

                value = ei.exerciseIMulByM536870912(dividends[i]);
                reportPassIf("exerciseIMulByM536870912(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-536870912);

                value = ei.exerciseIMulByM1073741824(dividends[i]);
                reportPassIf("exerciseIMulByM1073741824(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-1073741824);

                value = ei.exerciseIMulByM2147483648(dividends[i]);
                reportPassIf("exerciseIMulByM2147483648(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-2147483648);

                value = ei.exerciseIMulbByM1(dividends[i]);
                reportPassIf("exerciseIMulbByM1(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-1);

                value = ei.exerciseIMulbByM2(dividends[i]);
                reportPassIf("exerciseIMulbByM2(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-2);

                value = ei.exerciseIMulbByM4(dividends[i]);
                reportPassIf("exerciseIMulbByM4(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-4);

                value = ei.exerciseIMulbByM8(dividends[i]);
                reportPassIf("exerciseIMulbByM8(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-8);

                value = ei.exerciseIMulbByM16(dividends[i]);
                reportPassIf("exerciseIMulbByM16(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-16);

                value = ei.exerciseIMulbByM32(dividends[i]);
                reportPassIf("exerciseIMulbByM32(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-32);

                value = ei.exerciseIMulbByM64(dividends[i]);
                reportPassIf("exerciseIMulbByM64(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-64);

                value = ei.exerciseIMulbByM128(dividends[i]);
                reportPassIf("exerciseIMulbByM128(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-128);

                value = ei.exerciseIMulbByM256(dividends[i]);
                reportPassIf("exerciseIMulbByM256(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-256);

                value = ei.exerciseIMulbByM512(dividends[i]);
                reportPassIf("exerciseIMulbByM512(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-512);

                value = ei.exerciseIMulbByM1024(dividends[i]);
                reportPassIf("exerciseIMulbByM1024(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-1024);

                value = ei.exerciseIMulbByM2048(dividends[i]);
                reportPassIf("exerciseIMulbByM2048(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-2048);

                value = ei.exerciseIMulbByM4096(dividends[i]);
                reportPassIf("exerciseIMulbByM4096(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-4096);

                value = ei.exerciseIMulbByM8192(dividends[i]);
                reportPassIf("exerciseIMulbByM8192(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-8192);

                value = ei.exerciseIMulbByM16384(dividends[i]);
                reportPassIf("exerciseIMulbByM16384(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-16384);

                value = ei.exerciseIMulbByM32768(dividends[i]);
                reportPassIf("exerciseIMulbByM32768(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-32768);

                value = ei.exerciseIMulbByM65536(dividends[i]);
                reportPassIf("exerciseIMulbByM65536(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-65536);

                value = ei.exerciseIMulbByM131072(dividends[i]);
                reportPassIf("exerciseIMulbByM131072(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-131072);

                value = ei.exerciseIMulbByM262144(dividends[i]);
                reportPassIf("exerciseIMulbByM262144(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-262144);

                value = ei.exerciseIMulbByM524288(dividends[i]);
                reportPassIf("exerciseIMulbByM524288(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-524288);

                value = ei.exerciseIMulbByM1048576(dividends[i]);
                reportPassIf("exerciseIMulbByM1048576(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-1048576);

                value = ei.exerciseIMulbByM2097152(dividends[i]);
                reportPassIf("exerciseIMulbByM2097152(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-2097152);

                value = ei.exerciseIMulbByM4194304(dividends[i]);
                reportPassIf("exerciseIMulbByM4194304(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-4194304);

                value = ei.exerciseIMulbByM8388608(dividends[i]);
                reportPassIf("exerciseIMulbByM8388608(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-8388608);

                value = ei.exerciseIMulbByM16777216(dividends[i]);
                reportPassIf("exerciseIMulbByM16777216(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-16777216);

                value = ei.exerciseIMulbByM33554432(dividends[i]);
                reportPassIf("exerciseIMulbByM33554432(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-33554432);

                value = ei.exerciseIMulbByM67108864(dividends[i]);
                reportPassIf("exerciseIMulbByM67108864(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-67108864);

                value = ei.exerciseIMulbByM134217728(dividends[i]);
                reportPassIf("exerciseIMulbByM134217728(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-134217728);

                value = ei.exerciseIMulbByM268435456(dividends[i]);
                reportPassIf("exerciseIMulbByM268435456(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-268435456);

                value = ei.exerciseIMulbByM536870912(dividends[i]);
                reportPassIf("exerciseIMulbByM536870912(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-536870912);

                value = ei.exerciseIMulbByM1073741824(dividends[i]);
                reportPassIf("exerciseIMulbByM1073741824(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-1073741824);

                value = ei.exerciseIMulbByM2147483648(dividends[i]);
                reportPassIf("exerciseIMulbByM2147483648(dividends[" + i + "] == " + dividends[i] + ")",
                             value, dividends[i]*-2147483648);
            }
        }
        {
            int value = ei.exerciseIMul(0, 5);
            reportPassIf("exerciseIMul(0, 5)", value, 0);
        }
        {
            int value = ei.exerciseIMul(100, 0);
            reportPassIf("exerciseIMul(100, 0)", value, 0);
        }

        if (verbose) {
            System.out.println("");
        }
    }
}

class ExerciseReturnOpcodes
{
    public void exerciseReturn(Object o) {
        return;
    }
    public int exerciseIReturn(int i) {
        return i;
    }
    public float exerciseFReturn(float f) {
        return f;
    }
    public long exerciseLReturn(long l) {
        return l;
    }
    public double exerciseDReturn(double d) {
        return d;
    }
    public Object exerciseAReturn(Object o) {
        return o;
    }
}

class ExerciseIntOpcodes
{
    public int exerciseIReturn(int i) {
        return i;
    }
    public byte exerciseI2B(int value) {
        return (byte)value;
    }
    public char exerciseI2C(int value) {
        return (char)value;
    }
    public short exerciseI2S(int value) {
        return (short)value;
    }
    public double exerciseI2D(int value) {
        return (double)value;
    }
    public float exerciseI2F(int value) {
        return (float)value;
    }
    public long exerciseI2L(int value) {
        return (long)value;
    }
    public int exerciseIAdd(int value1, int value2) {
        return value1 + value2;
    }
    public int exerciseIDiv(int value1, int value2) {
        return value1 / value2;
    }
    public int exerciseIDivBy1(int value1) {
        return value1 / 1;
    }
    public int exerciseIDivBy2(int value1) {
        return value1 / 2;
    }
    public int exerciseIDivBy4(int value1) {
        return value1 / 4;
    }
    public int exerciseIDivBy8(int value1) {
        return value1 / 8;
    }
    public int exerciseIDivBy16(int value1) {
        return value1 / 16;
    }
    public int exerciseIDivBy32(int value1) {
        return value1 / 32;
    }
    public int exerciseIDivBy64(int value1) {
        return value1 / 64;
    }
    public int exerciseIDivBy128(int value1) {
        return value1 / 128;
    }
    public int exerciseIDivBy256(int value1) {
        return value1 / 256;
    }
    public int exerciseIDivBy512(int value1) {
        return value1 / 512;
    }
    public int exerciseIDivBy1024(int value1) {
        return value1 / 1024;
    }
    public int exerciseIDivBy2048(int value1) {
        return value1 / 2048;
    }
    public int exerciseIDivBy4096(int value1) {
        return value1 / 4096;
    }
    public int exerciseIDivBy8192(int value1) {
        return value1 / 8192;
    }
    public int exerciseIDivBy16384(int value1) {
        return value1 / 16384;
    }
    public int exerciseIDivBy32768(int value1) {
        return value1 / 32768;
    }
    public int exerciseIDivBy65536(int value1) {
        return value1 / 65536;
    }
    public int exerciseIDivBy131072(int value1) {
        return value1 / 131072;
    }
    public int exerciseIDivBy262144(int value1) {
        return value1 / 262144;
    }
    public int exerciseIDivBy524288(int value1) {
        return value1 / 524288;
    }
    public int exerciseIDivBy1048576(int value1) {
        return value1 / 1048576;
    }
    public int exerciseIDivBy2097152(int value1) {
        return value1 / 2097152;
    }
    public int exerciseIDivBy4194304(int value1) {
        return value1 / 4194304;
    }
    public int exerciseIDivBy8388608(int value1) {
        return value1 / 8388608;
    }
    public int exerciseIDivBy16777216(int value1) {
        return value1 / 16777216;
    }
    public int exerciseIDivBy33554432(int value1) {
        return value1 / 33554432;
    }
    public int exerciseIDivBy67108864(int value1) {
        return value1 / 67108864;
    }
    public int exerciseIDivBy134217728(int value1) {
        return value1 / 134217728;
    }
    public int exerciseIDivBy268435456(int value1) {
        return value1 / 268435456;
    }
    public int exerciseIDivBy536870912(int value1) {
        return value1 / 536870912;
    }
    public int exerciseIDivBy1073741824(int value1) {
        return value1 / 1073741824;
    }
    public int exerciseIDivBy3(int value1) {
        return value1 / 3;
    }
    public int exerciseIDivBy5(int value1) {
        return value1 / 5;
    }
    public int exerciseIDivBy6(int value1) {
        return value1 / 6;
    }
    public int exerciseIDivBy7(int value1) {
        return value1 / 7;
    }
    public int exerciseIDivBy9(int value1) {
        return value1 / 9;
    }
    public int exerciseIDivBy10(int value1) {
        return value1 / 10;
    }
    public int exerciseIDivBy17(int value1) {
        return value1 / 17;
    }
    public int exerciseIDivBy100(int value1) {
        return value1 / 100;
    }
    public int exerciseIDivBy125(int value1) {
        return value1 / 125;
    }
    public int exerciseIDivBy1027(int value1) {
        return value1 / 1027;
    }
    public int exerciseIDivBy5612712(int value1) {
        return value1 / 5612712;
    }
    public int exerciseIDivBy0x7fffffff(int value1) {
        return value1 / 0x7fffffff;
    }
    public int exerciseIDivByM1(int value1) {
        return value1 / -1;
    }
    public int exerciseIDivByM2(int value1) {
        return value1 / -2;
    }
    public int exerciseIDivByM4(int value1) {
        return value1 / -4;
    }
    public int exerciseIDivByM8(int value1) {
        return value1 / -8;
    }
    public int exerciseIDivByM16(int value1) {
        return value1 / -16;
    }
    public int exerciseIDivByM32(int value1) {
        return value1 / -32;
    }
    public int exerciseIDivByM64(int value1) {
        return value1 / -64;
    }
    public int exerciseIDivByM128(int value1) {
        return value1 / -128;
    }
    public int exerciseIDivByM256(int value1) {
        return value1 / -256;
    }
    public int exerciseIDivByM512(int value1) {
        return value1 / -512;
    }
    public int exerciseIDivByM1024(int value1) {
        return value1 / -1024;
    }
    public int exerciseIDivByM2048(int value1) {
        return value1 / -2048;
    }
    public int exerciseIDivByM4096(int value1) {
        return value1 / -4096;
    }
    public int exerciseIDivByM8192(int value1) {
        return value1 / -8192;
    }
    public int exerciseIDivByM16384(int value1) {
        return value1 / -16384;
    }
    public int exerciseIDivByM32768(int value1) {
        return value1 / -32768;
    }
    public int exerciseIDivByM65536(int value1) {
        return value1 / -65536;
    }
    public int exerciseIDivByM131072(int value1) {
        return value1 / -131072;
    }
    public int exerciseIDivByM262144(int value1) {
        return value1 / -262144;
    }
    public int exerciseIDivByM524288(int value1) {
        return value1 / -524288;
    }
    public int exerciseIDivByM1048576(int value1) {
        return value1 / -1048576;
    }
    public int exerciseIDivByM2097152(int value1) {
        return value1 / -2097152;
    }
    public int exerciseIDivByM4194304(int value1) {
        return value1 / -4194304;
    }
    public int exerciseIDivByM8388608(int value1) {
        return value1 / -8388608;
    }
    public int exerciseIDivByM16777216(int value1) {
        return value1 / -16777216;
    }
    public int exerciseIDivByM33554432(int value1) {
        return value1 / -33554432;
    }
    public int exerciseIDivByM67108864(int value1) {
        return value1 / -67108864;
    }
    public int exerciseIDivByM134217728(int value1) {
        return value1 / -134217728;
    }
    public int exerciseIDivByM268435456(int value1) {
        return value1 / -268435456;
    }
    public int exerciseIDivByM536870912(int value1) {
        return value1 / -536870912;
    }
    public int exerciseIDivByM1073741824(int value1) {
        return value1 / -1073741824;
    }
    public int exerciseIDivByM2147483648(int value1) {
        return value1 / -2147483648;
    }
    public int exerciseIDivByM3(int value1) {
        return value1 / -3;
    }
    public int exerciseIDivByM5(int value1) {
        return value1 / -5;
    }
    public int exerciseIDivByM6(int value1) {
        return value1 / -6;
    }
    public int exerciseIDivByM7(int value1) {
        return value1 / -7;
    }
    public int exerciseIDivByM9(int value1) {
        return value1 / -9;
    }
    public int exerciseIDivByM10(int value1) {
        return value1 / -10;
    }
    public int exerciseIDivByM17(int value1) {
        return value1 / -17;
    }
    public int exerciseIDivByM100(int value1) {
        return value1 / -100;
    }
    public int exerciseIDivByM125(int value1) {
        return value1 / -125;
    }
    public int exerciseIDivByM1027(int value1) {
        return value1 / -1027;
    }
    public int exerciseIDivByM5612712(int value1) {
        return value1 / -5612712;
    }
    public int exerciseIDivByM0x7fffffff(int value1) {
        return value1 / -0x7fffffff;
    }
    public int exerciseIDiv2(int value1, int value2) {
        return exerciseIAdd(value1, value1 / value2);
    }

    public int exerciseIRem(int value1, int value2) {
        return value1 % value2;
    }
    public int exerciseIRemBy1(int value1) {
        return value1 % 1;
    }
    public int exerciseIRemBy2(int value1) {
        return value1 % 2;
    }
    public int exerciseIRemBy4(int value1) {
        return value1 % 4;
    }
    public int exerciseIRemBy8(int value1) {
        return value1 % 8;
    }
    public int exerciseIRemBy16(int value1) {
        return value1 % 16;
    }
    public int exerciseIRemBy32(int value1) {
        return value1 % 32;
    }
    public int exerciseIRemBy64(int value1) {
        return value1 % 64;
    }
    public int exerciseIRemBy128(int value1) {
        return value1 % 128;
    }
    public int exerciseIRemBy256(int value1) {
        return value1 % 256;
    }
    public int exerciseIRemBy512(int value1) {
        return value1 % 512;
    }
    public int exerciseIRemBy1024(int value1) {
        return value1 % 1024;
    }
    public int exerciseIRemBy2048(int value1) {
        return value1 % 2048;
    }
    public int exerciseIRemBy4096(int value1) {
        return value1 % 4096;
    }
    public int exerciseIRemBy8192(int value1) {
        return value1 % 8192;
    }
    public int exerciseIRemBy16384(int value1) {
        return value1 % 16384;
    }
    public int exerciseIRemBy32768(int value1) {
        return value1 % 32768;
    }
    public int exerciseIRemBy65536(int value1) {
        return value1 % 65536;
    }
    public int exerciseIRemBy131072(int value1) {
        return value1 % 131072;
    }
    public int exerciseIRemBy262144(int value1) {
        return value1 % 262144;
    }
    public int exerciseIRemBy524288(int value1) {
        return value1 % 524288;
    }
    public int exerciseIRemBy1048576(int value1) {
        return value1 % 1048576;
    }
    public int exerciseIRemBy2097152(int value1) {
        return value1 % 2097152;
    }
    public int exerciseIRemBy4194304(int value1) {
        return value1 % 4194304;
    }
    public int exerciseIRemBy8388608(int value1) {
        return value1 % 8388608;
    }
    public int exerciseIRemBy16777216(int value1) {
        return value1 % 16777216;
    }
    public int exerciseIRemBy33554432(int value1) {
        return value1 % 33554432;
    }
    public int exerciseIRemBy67108864(int value1) {
        return value1 % 67108864;
    }
    public int exerciseIRemBy134217728(int value1) {
        return value1 % 134217728;
    }
    public int exerciseIRemBy268435456(int value1) {
        return value1 % 268435456;
    }
    public int exerciseIRemBy536870912(int value1) {
        return value1 % 536870912;
    }
    public int exerciseIRemBy1073741824(int value1) {
        return value1 % 1073741824;
    }
    public int exerciseIRemBy3(int value1) {
        return value1 % 3;
    }
    public int exerciseIRemBy5(int value1) {
        return value1 % 5;
    }
    public int exerciseIRemBy6(int value1) {
        return value1 % 6;
    }
    public int exerciseIRemBy7(int value1) {
        return value1 % 7;
    }
    public int exerciseIRemBy9(int value1) {
        return value1 % 9;
    }
    public int exerciseIRemBy10(int value1) {
        return value1 % 10;
    }
    public int exerciseIRemBy17(int value1) {
        return value1 % 17;
    }
    public int exerciseIRemBy100(int value1) {
        return value1 % 100;
    }
    public int exerciseIRemBy125(int value1) {
        return value1 % 125;
    }
    public int exerciseIRemBy1027(int value1) {
        return value1 % 1027;
    }
    public int exerciseIRemBy5612712(int value1) {
        return value1 % 5612712;
    }
    public int exerciseIRemBy0x7fffffff(int value1) {
        return value1 % 0x7fffffff;
    }
    public int exerciseIRemByM1(int value1) {
        return value1 % -1;
    }
    public int exerciseIRemByM2(int value1) {
        return value1 % -2;
    }
    public int exerciseIRemByM4(int value1) {
        return value1 % -4;
    }
    public int exerciseIRemByM8(int value1) {
        return value1 % -8;
    }
    public int exerciseIRemByM16(int value1) {
        return value1 % -16;
    }
    public int exerciseIRemByM32(int value1) {
        return value1 % -32;
    }
    public int exerciseIRemByM64(int value1) {
        return value1 % -64;
    }
    public int exerciseIRemByM128(int value1) {
        return value1 % -128;
    }
    public int exerciseIRemByM256(int value1) {
        return value1 % -256;
    }
    public int exerciseIRemByM512(int value1) {
        return value1 % -512;
    }
    public int exerciseIRemByM1024(int value1) {
        return value1 % -1024;
    }
    public int exerciseIRemByM2048(int value1) {
        return value1 % -2048;
    }
    public int exerciseIRemByM4096(int value1) {
        return value1 % -4096;
    }
    public int exerciseIRemByM8192(int value1) {
        return value1 % -8192;
    }
    public int exerciseIRemByM16384(int value1) {
        return value1 % -16384;
    }
    public int exerciseIRemByM32768(int value1) {
        return value1 % -32768;
    }
    public int exerciseIRemByM65536(int value1) {
        return value1 % -65536;
    }
    public int exerciseIRemByM131072(int value1) {
        return value1 % -131072;
    }
    public int exerciseIRemByM262144(int value1) {
        return value1 % -262144;
    }
    public int exerciseIRemByM524288(int value1) {
        return value1 % -524288;
    }
    public int exerciseIRemByM1048576(int value1) {
        return value1 % -1048576;
    }
    public int exerciseIRemByM2097152(int value1) {
        return value1 % -2097152;
    }
    public int exerciseIRemByM4194304(int value1) {
        return value1 % -4194304;
    }
    public int exerciseIRemByM8388608(int value1) {
        return value1 % -8388608;
    }
    public int exerciseIRemByM16777216(int value1) {
        return value1 % -16777216;
    }
    public int exerciseIRemByM33554432(int value1) {
        return value1 % -33554432;
    }
    public int exerciseIRemByM67108864(int value1) {
        return value1 % -67108864;
    }
    public int exerciseIRemByM134217728(int value1) {
        return value1 % -134217728;
    }
    public int exerciseIRemByM268435456(int value1) {
        return value1 % -268435456;
    }
    public int exerciseIRemByM536870912(int value1) {
        return value1 % -536870912;
    }
    public int exerciseIRemByM1073741824(int value1) {
        return value1 % -1073741824;
    }
    public int exerciseIRemByM2147483648(int value1) {
        return value1 % -2147483648;
    }
    public int exerciseIRemByM3(int value1) {
        return value1 % -3;
    }
    public int exerciseIRemByM5(int value1) {
        return value1 % -5;
    }
    public int exerciseIRemByM6(int value1) {
        return value1 % -6;
    }
    public int exerciseIRemByM7(int value1) {
        return value1 % -7;
    }
    public int exerciseIRemByM9(int value1) {
        return value1 % -9;
    }
    public int exerciseIRemByM10(int value1) {
        return value1 % -10;
    }
    public int exerciseIRemByM17(int value1) {
        return value1 % -17;
    }
    public int exerciseIRemByM100(int value1) {
        return value1 % -100;
    }
    public int exerciseIRemByM125(int value1) {
        return value1 % -125;
    }
    public int exerciseIRemByM1027(int value1) {
        return value1 % -1027;
    }
    public int exerciseIRemByM5612712(int value1) {
        return value1 % -5612712;
    }
    public int exerciseIRemByM0x7fffffff(int value1) {
        return value1 % -0x7fffffff;
    }

    public int exerciseIMul(int value1, int value2) {
        return value1 * value2;
    }
    public int exerciseIMulBy1(int value1) {
        return value1 * 1;
    }
    public int exerciseIMulBy2(int value1) {
        return value1 * 2;
    }
    public int exerciseIMulBy4(int value1) {
        return value1 * 4;
    }
    public int exerciseIMulBy8(int value1) {
        return value1 * 8;
    }
    public int exerciseIMulBy16(int value1) {
        return value1 * 16;
    }
    public int exerciseIMulBy32(int value1) {
        return value1 * 32;
    }
    public int exerciseIMulBy64(int value1) {
        return value1 * 64;
    }
    public int exerciseIMulBy128(int value1) {
        return value1 * 128;
    }
    public int exerciseIMulBy256(int value1) {
        return value1 * 256;
    }
    public int exerciseIMulBy512(int value1) {
        return value1 * 512;
    }
    public int exerciseIMulBy1024(int value1) {
        return value1 * 1024;
    }
    public int exerciseIMulBy2048(int value1) {
        return value1 * 2048;
    }
    public int exerciseIMulBy4096(int value1) {
        return value1 * 4096;
    }
    public int exerciseIMulBy8192(int value1) {
        return value1 * 8192;
    }
    public int exerciseIMulBy16384(int value1) {
        return value1 * 16384;
    }
    public int exerciseIMulBy32768(int value1) {
        return value1 * 32768;
    }
    public int exerciseIMulBy65536(int value1) {
        return value1 * 65536;
    }
    public int exerciseIMulBy131072(int value1) {
        return value1 * 131072;
    }
    public int exerciseIMulBy262144(int value1) {
        return value1 * 262144;
    }
    public int exerciseIMulBy524288(int value1) {
        return value1 * 524288;
    }
    public int exerciseIMulBy1048576(int value1) {
        return value1 * 1048576;
    }
    public int exerciseIMulBy2097152(int value1) {
        return value1 * 2097152;
    }
    public int exerciseIMulBy4194304(int value1) {
        return value1 * 4194304;
    }
    public int exerciseIMulBy8388608(int value1) {
        return value1 * 8388608;
    }
    public int exerciseIMulBy16777216(int value1) {
        return value1 * 16777216;
    }
    public int exerciseIMulBy33554432(int value1) {
        return value1 * 33554432;
    }
    public int exerciseIMulBy67108864(int value1) {
        return value1 * 67108864;
    }
    public int exerciseIMulBy134217728(int value1) {
        return value1 * 134217728;
    }
    public int exerciseIMulBy268435456(int value1) {
        return value1 * 268435456;
    }
    public int exerciseIMulBy536870912(int value1) {
        return value1 * 536870912;
    }
    public int exerciseIMulBy1073741824(int value1) {
        return value1 * 1073741824;
    }
    public int exerciseIMulbBy1(int value1) {
        return 1 * value1;
    }
    public int exerciseIMulbBy2(int value1) {
        return 2 * value1;
    }
    public int exerciseIMulbBy4(int value1) {
        return 4 * value1;
    }
    public int exerciseIMulbBy8(int value1) {
        return 8 * value1;
    }
    public int exerciseIMulbBy16(int value1) {
        return 16 * value1;
    }
    public int exerciseIMulbBy32(int value1) {
        return 32 * value1;
    }
    public int exerciseIMulbBy64(int value1) {
        return 64 * value1;
    }
    public int exerciseIMulbBy128(int value1) {
        return 128 * value1;
    }
    public int exerciseIMulbBy256(int value1) {
        return 256 * value1;
    }
    public int exerciseIMulbBy512(int value1) {
        return 512 * value1;
    }
    public int exerciseIMulbBy1024(int value1) {
        return 1024 * value1;
    }
    public int exerciseIMulbBy2048(int value1) {
        return 2048 * value1;
    }
    public int exerciseIMulbBy4096(int value1) {
        return 4096 * value1;
    }
    public int exerciseIMulbBy8192(int value1) {
        return 8192 * value1;
    }
    public int exerciseIMulbBy16384(int value1) {
        return 16384 * value1;
    }
    public int exerciseIMulbBy32768(int value1) {
        return 32768 * value1;
    }
    public int exerciseIMulbBy65536(int value1) {
        return 65536 * value1;
    }
    public int exerciseIMulbBy131072(int value1) {
        return 131072 * value1;
    }
    public int exerciseIMulbBy262144(int value1) {
        return 262144 * value1;
    }
    public int exerciseIMulbBy524288(int value1) {
        return 524288 * value1;
    }
    public int exerciseIMulbBy1048576(int value1) {
        return 1048576 * value1;
    }
    public int exerciseIMulbBy2097152(int value1) {
        return 2097152 * value1;
    }
    public int exerciseIMulbBy4194304(int value1) {
        return 4194304 * value1;
    }
    public int exerciseIMulbBy8388608(int value1) {
        return 8388608 * value1;
    }
    public int exerciseIMulbBy16777216(int value1) {
        return 16777216 * value1;
    }
    public int exerciseIMulbBy33554432(int value1) {
        return 33554432 * value1;
    }
    public int exerciseIMulbBy67108864(int value1) {
        return 67108864 * value1;
    }
    public int exerciseIMulbBy134217728(int value1) {
        return 134217728 * value1;
    }
    public int exerciseIMulbBy268435456(int value1) {
        return 268435456 * value1;
    }
    public int exerciseIMulbBy536870912(int value1) {
        return 536870912 * value1;
    }
    public int exerciseIMulbBy1073741824(int value1) {
        return 1073741824 * value1;
    }

    public int exerciseIMulByM1(int value1) {
        return value1 * -1;
    }
    public int exerciseIMulByM2(int value1) {
        return value1 * -2;
    }
    public int exerciseIMulByM4(int value1) {
        return value1 * -4;
    }
    public int exerciseIMulByM8(int value1) {
        return value1 * -8;
    }
    public int exerciseIMulByM16(int value1) {
        return value1 * -16;
    }
    public int exerciseIMulByM32(int value1) {
        return value1 * -32;
    }
    public int exerciseIMulByM64(int value1) {
        return value1 * -64;
    }
    public int exerciseIMulByM128(int value1) {
        return value1 * -128;
    }
    public int exerciseIMulByM256(int value1) {
        return value1 * -256;
    }
    public int exerciseIMulByM512(int value1) {
        return value1 * -512;
    }
    public int exerciseIMulByM1024(int value1) {
        return value1 * -1024;
    }
    public int exerciseIMulByM2048(int value1) {
        return value1 * -2048;
    }
    public int exerciseIMulByM4096(int value1) {
        return value1 * -4096;
    }
    public int exerciseIMulByM8192(int value1) {
        return value1 * -8192;
    }
    public int exerciseIMulByM16384(int value1) {
        return value1 * -16384;
    }
    public int exerciseIMulByM32768(int value1) {
        return value1 * -32768;
    }
    public int exerciseIMulByM65536(int value1) {
        return value1 * -65536;
    }
    public int exerciseIMulByM131072(int value1) {
        return value1 * -131072;
    }
    public int exerciseIMulByM262144(int value1) {
        return value1 * -262144;
    }
    public int exerciseIMulByM524288(int value1) {
        return value1 * -524288;
    }
    public int exerciseIMulByM1048576(int value1) {
        return value1 * -1048576;
    }
    public int exerciseIMulByM2097152(int value1) {
        return value1 * -2097152;
    }
    public int exerciseIMulByM4194304(int value1) {
        return value1 * -4194304;
    }
    public int exerciseIMulByM8388608(int value1) {
        return value1 * -8388608;
    }
    public int exerciseIMulByM16777216(int value1) {
        return value1 * -16777216;
    }
    public int exerciseIMulByM33554432(int value1) {
        return value1 * -33554432;
    }
    public int exerciseIMulByM67108864(int value1) {
        return value1 * -67108864;
    }
    public int exerciseIMulByM134217728(int value1) {
        return value1 * -134217728;
    }
    public int exerciseIMulByM268435456(int value1) {
        return value1 * -268435456;
    }
    public int exerciseIMulByM536870912(int value1) {
        return value1 * -536870912;
    }
    public int exerciseIMulByM1073741824(int value1) {
        return value1 * -1073741824;
    }
    public int exerciseIMulByM2147483648(int value1) {
        return value1 * -2147483648;
    }
    public int exerciseIMulbByM1(int value1) {
        return -1 * value1;
    }
    public int exerciseIMulbByM2(int value1) {
        return -2 * value1;
    }
    public int exerciseIMulbByM4(int value1) {
        return -4 * value1;
    }
    public int exerciseIMulbByM8(int value1) {
        return -8 * value1;
    }
    public int exerciseIMulbByM16(int value1) {
        return -16 * value1;
    }
    public int exerciseIMulbByM32(int value1) {
        return -32 * value1;
    }
    public int exerciseIMulbByM64(int value1) {
        return -64 * value1;
    }
    public int exerciseIMulbByM128(int value1) {
        return -128 * value1;
    }
    public int exerciseIMulbByM256(int value1) {
        return -256 * value1;
    }
    public int exerciseIMulbByM512(int value1) {
        return -512 * value1;
    }
    public int exerciseIMulbByM1024(int value1) {
        return -1024 * value1;
    }
    public int exerciseIMulbByM2048(int value1) {
        return -2048 * value1;
    }
    public int exerciseIMulbByM4096(int value1) {
        return -4096 * value1;
    }
    public int exerciseIMulbByM8192(int value1) {
        return -8192 * value1;
    }
    public int exerciseIMulbByM16384(int value1) {
        return -16384 * value1;
    }
    public int exerciseIMulbByM32768(int value1) {
        return -32768 * value1;
    }
    public int exerciseIMulbByM65536(int value1) {
        return -65536 * value1;
    }
    public int exerciseIMulbByM131072(int value1) {
        return -131072 * value1;
    }
    public int exerciseIMulbByM262144(int value1) {
        return -262144 * value1;
    }
    public int exerciseIMulbByM524288(int value1) {
        return -524288 * value1;
    }
    public int exerciseIMulbByM1048576(int value1) {
        return -1048576 * value1;
    }
    public int exerciseIMulbByM2097152(int value1) {
        return -2097152 * value1;
    }
    public int exerciseIMulbByM4194304(int value1) {
        return -4194304 * value1;
    }
    public int exerciseIMulbByM8388608(int value1) {
        return -8388608 * value1;
    }
    public int exerciseIMulbByM16777216(int value1) {
        return -16777216 * value1;
    }
    public int exerciseIMulbByM33554432(int value1) {
        return -33554432 * value1;
    }
    public int exerciseIMulbByM67108864(int value1) {
        return -67108864 * value1;
    }
    public int exerciseIMulbByM134217728(int value1) {
        return -134217728 * value1;
    }
    public int exerciseIMulbByM268435456(int value1) {
        return -268435456 * value1;
    }
    public int exerciseIMulbByM536870912(int value1) {
        return -536870912 * value1;
    }
    public int exerciseIMulbByM1073741824(int value1) {
        return -1073741824 * value1;
    }
    public int exerciseIMulbByM2147483648(int value1) {
        return -2147483648 * value1;
    }

    public int exerciseIALoad(int[] ia, int index) {
        return ia[index];
    }
    public int exerciseIALoad_0(int[] ia) {
        return ia[0];
    }
    public int exerciseIALoad_1(int[] ia) {
        return ia[1];
    }
    public int exerciseIALoad_255(int[] ia) {
        return ia[255];
    }
    public int exerciseIALoad_256(int[] ia) {
        return ia[256];
    }
    public int exerciseIALoad_4Km4(int[] ia) {
        return ia[4*1024-4];
    }
    public int exerciseIALoad_4K(int[] ia) {
        return ia[4*1024];
    }
    public void exerciseIAStore(int[] ia, int index, int value) {
        ia[index] = value;
    }
    public int exerciseIConst0() {
        return 0;
    }
    public int exerciseIConst1() {
        return 1;
    }
    public int exerciseIConst255() {
        return 255;
    }
    public int exerciseIConst256() {
        return 256;
    }
    public int exerciseIConst4Km4() {
        return 4*1024-4;
    }
    public int exerciseIConst4K() {
        return 4*1024;
    }

    public int exerciseIShl(int value1, int value2) {
        return value1 << value2;
    }
    public int exerciseIShlm1(int value) {
        return value << -1;
    }
    public int exerciseIShl0(int value) {
        return value << 0;
    }
    public int exerciseIShl31(int value) {
        return value << 31;
    }
    public int exerciseIShl32(int value) {
        return value << 32;
    }
    public int exerciseIShl33(int value) {
        return value << 33;
    }

    public int exerciseIShr(int value1, int value2) {
        return value1 >> value2;
    }
    public int exerciseIShrm1(int value) {
        return value >> -1;
    }
    public int exerciseIShr0(int value) {
        return value >> 0;
    }
    public int exerciseIShr31(int value) {
        return value >> 31;
    }
    public int exerciseIShr32(int value) {
        return value >> 32;
    }
    public int exerciseIShr33(int value) {
        return value >> 33;
    }

    public int exerciseIUShr(int value1, int value2) {
        return value1 >>> value2;
    }
    public int exerciseIUShrm1(int value) {
        return value >>> -1;
    }
    public int exerciseIUShr0(int value) {
        return value >>> 0;
    }
    public int exerciseIUShr31(int value) {
        return value >>> 31;
    }
    public int exerciseIUShr32(int value) {
        return value >>> 32;
    }
    public int exerciseIUShr33(int value) {
        return value >>> 33;
    }
}

class ExerciseLongOpcodes
{
    public static void exerciseOpcodes() {
        ExerciseLongOpcodes l = new ExerciseLongOpcodes();
        long lresult;
        l.exerciseL2D(1);
        l.exerciseL2F(1);
        l.exerciseL2I(1);
        lresult = l.exerciseLAdd(1, 2);
    }

    public double exerciseL2D(long value) {
        return (double)value;
    }
    public float exerciseL2F(long value) {
        return (float)value;
    }
    public int exerciseL2I(long value) {
        return (int)value;
    }
    public long exerciseLAdd(long value1, long value2) {
        return value1 + value2;
    }
    public long exerciseLALoad(long[] la, int index) {
        return la[index];
    }
    public long exerciseLALoad_0(long[] la) {
        return la[0];
    }
    public long exerciseLALoad_1(long[] la) {
        return la[1];
    }
    public long exerciseLALoad_255(long[] la) {
        return la[255];
    }
    public long exerciseLALoad_256(long[] la) {
        return la[256];
    }
    public long exerciseLALoad_4Km4(long[] la) {
        return la[4*1024-4];
    }
    public long exerciseLALoad_4K(long[] la) {
        return la[4*1024];
    }
    public long exerciseLAnd(long value1, long value2) {
        return value1 & value2;
    }
    public void exerciseLAStore(long[] la, int index, long value) {
        la[index] = value;
    }

    public boolean exerciseLCmp_EQ(long value1, long value2) {
        return (value1 == value2);
    }
    public boolean exerciseLCmp_NE(long value1, long value2) {
        return (value1 != value2);
    }
    public boolean exerciseLCmp_GT(long value1, long value2) {
        return (value1 > value2);
    }
    public boolean exerciseLCmp_GE(long value1, long value2) {
        return (value1 >= value2);
    }
    public boolean exerciseLCmp_LT(long value1, long value2) {
        return (value1 < value2);
    }
    public boolean exerciseLCmp_LE(long value1, long value2) {
        return (value1 <= value2);
    }

    public long exerciseLConst0() {
        return 0;
    }
    public long exerciseLConst1() {
        return 1;
    }
    public long exerciseLDiv(long value1, long value2) {
        return value1 / value2;
    }
    public static long exerciseLLoad0(long value) {
        return value;
    }
    public static long exerciseLLoad1(int i1, long value) {
        return value;
    }
    public static long exerciseLLoad2(int i1, int i2, long value) {
        return value;
    }
    public static long exerciseLLoad3(int i1, int i2, int i3, long value) {
        return value;
    }
    public static long exerciseLLoad(int i1, int i2, int i3, int i4, long value) {
        return value;
    }
    public long exerciseLMul(long value1, long value2) {
        return value1 * value2;
    }
    public long exerciseLNeg(long value) {
        return -value;
    }
    public long exerciseLOr(long value1, long value2) {
        return value1 | value2;
    }
    public long exerciseLRem(long value1, long value2) {
        return value1 % value2;
    }
    public long exerciseLReturn(long value) {
        return value;
    }

    public long exerciseLShl(long value1, int value2) {
        return value1 << value2;
    }
    public long exerciseLShlm1(long value) {
        return value << -1;
    }
    public long exerciseLShl0(long value) {
        return value << 0;
    }
    public long exerciseLShl63(long value) {
        return value << 63;
    }
    public long exerciseLShl64(long value) {
        return value << 64;
    }
    public long exerciseLShl65(long value) {
        return value << 65;
    }

    public long exerciseLShr(long value1, int value2) {
        return value1 >> value2;
    }
    public long exerciseLShrm1(long value) {
        return value >> -1;
    }
    public long exerciseLShr0(long value) {
        return value >> 0;
    }
    public long exerciseLShr63(long value) {
        return value >> 63;
    }
    public long exerciseLShr64(long value) {
        return value >> 64;
    }
    public long exerciseLShr65(long value) {
        return value >> 65;
    }

    public long exerciseLUShr(long value1, int value2) {
        return value1 >>> value2;
    }
    public long exerciseLUShrm1(long value) {
        return value >>> -1;
    }
    public long exerciseLUShr0(long value) {
        return value >>> 0;
    }
    public long exerciseLUShr63(long value) {
        return value >>> 63;
    }
    public long exerciseLUShr64(long value) {
        return value >>> 64;
    }
    public long exerciseLUShr65(long value) {
        return value >>> 65;
    }

    public static long exerciseLStore(int i1, int i2, int i3, int i4, long value1) {
        value1 = 1;
        return value1;
    }
    public static long exerciseLStore0(long value1) {
        value1 = 1;
        return value1;
    }
    public static long exerciseLStore1(int i1, long value1) {
        value1 = 1;
        return value1;
    }
    public static long exerciseLStore2(int i1, int i2, long value1) {
        value1 = 1;
        return value1;
    }
    public static long exerciseLStore3(int i1, int i2, int i3, long value1) {
        value1 = 1;
        return value1;
    }
    public long exerciseLSub(long value1, long value2) {
        return value1 - value2;
    }
    public long exerciseLXor(long value1, long value2) {
        return value1 ^ value2;
    }
}

class ExerciseFloatOpcodes
{
    public static void exerciseOpcodes() {
        ExerciseFloatOpcodes f = new ExerciseFloatOpcodes();
        float fresult;
        f.exerciseF2D(1.0f);
        f.exerciseF2I(1.0f);
        f.exerciseF2L(1.0f);
        fresult = f.exerciseFAdd(1.0f, 2.0f);
    }

    public double exerciseF2D(float value) {
        return (double)value;
    }
    public int exerciseF2I(float value) {
        return (int)value;
    }
    public long exerciseF2L(float value) {
        return (long)value;
    }
    public float exerciseFAdd(float value1, float value2) {
        return value1 + value2;
    }
    public float exerciseFALoad(float[] fa, int index) {
        return fa[index];
    }
    public void exerciseFAStore(float[] fa, int index, float value) {
        fa[index] = value;
    }

    public boolean exerciseFCmp_EQ(float value1, float value2) {
        return (value1 == value2);
    }
    public boolean exerciseFCmp_NE(float value1, float value2) {
        return (value1 != value2);
    }
    public boolean exerciseFCmp_GT(float value1, float value2) {
        return (value1 > value2);
    }
    public boolean exerciseFCmp_GE(float value1, float value2) {
        return (value1 >= value2);
    }
    public boolean exerciseFCmp_LT(float value1, float value2) {
        return (value1 < value2);
    }
    public boolean exerciseFCmp_LE(float value1, float value2) {
        return (value1 <= value2);
    }
    public float exerciseFConst0() {
        return 0.0f;
    }
    public float exerciseFConst1() {
        return 1.0f;
    }
    public float exerciseFConst2() {
        return 2.0f;
    }
    public float exerciseFDiv(float value1, float value2) {
        return value1 / value2;
    }
    public static float exerciseFLoad0(float value) {
        return value;
    }
    public static float exerciseFLoad1(int i1, float value) {
        return value;
    }
    public static float exerciseFLoad2(int i1, int i2, float value) {
        return value;
    }
    public static float exerciseFLoad3(int i1, int i2, int i3, float value) {
        return value;
    }
    public static float exerciseFLoad(int i1, int i2, int i3, int i4, float value) {
        return value;
    }
    public float exerciseFMul(float value1, float value2) {
        return value1 * value2;
    }
    public float exerciseFNeg(float value) {
        return -value;
    }
    public float exerciseFRem(float value1, float value2) {
        return value1 % value2;
    }
    public float exerciseFReturn(float value) {
        return value;
    }
    public static float exerciseFStore(int i1, int i2, int i3, int i4, float value1) {
        value1 = 1.0f;
        return value1;
    }
    public static float exerciseFStore0(float value1) {
        value1 = 1.0f;
        return value1;
    }
    public static float exerciseFStore1(int i1, float value1) {
        value1 = 1.0f;
        return value1;
    }
    public static float exerciseFStore2(int i1, int i2, float value1) {
        value1 = 1.0f;
        return value1;
    }
    public static float exerciseFStore3(int i1, int i2, int i3, float value1) {
        value1 = 1.0f;
        return value1;
    }
    public float exerciseFSub(float value1, float value2) {
        return value1 - value2;
    }
}

class ExerciseDoubleOpcodes
{
    public static void exerciseOpcodes() {
        ExerciseDoubleOpcodes d = new ExerciseDoubleOpcodes();
        double dresult;
        d.exerciseD2F(1.0);
        d.exerciseD2I(1.0);
        d.exerciseD2L(1.0);
        dresult = d.exerciseDAdd(1.0, 2.0);
    }

    public float exerciseD2F(double value) {
        return (float)value;
    }
    public int exerciseD2I(double value) {
        return (int)value;
    }
    public long exerciseD2L(double value) {
        return (long)value;
    }
    public double exerciseDAdd(double value1, double value2) {
        return value1 + value2;
    }
    public double exerciseDALoad(double[] da, int index) {
        return da[index];
    }
    public void exerciseDAStore(double[] da, int index, double value) {
        da[index] = value;
    }

    public boolean exerciseDCmp_EQ(double value1, double value2) {
        return (value1 == value2);
    }
    public boolean exerciseDCmp_NE(double value1, double value2) {
        return (value1 != value2);
    }
    public boolean exerciseDCmp_GT(double value1, double value2) {
        return (value1 > value2);
    }
    public boolean exerciseDCmp_GE(double value1, double value2) {
        return (value1 >= value2);
    }
    public boolean exerciseDCmp_LT(double value1, double value2) {
        return (value1 < value2);
    }
    public boolean exerciseDCmp_LE(double value1, double value2) {
        return (value1 <= value2);
    }

    public double exerciseDConst0() {
        return 0.0;
    }
    public double exerciseDConst1() {
        return 1.0;
    }
    public double exerciseDDiv(double value1, double value2) {
        return value1 / value2;
    }
    public static double exerciseDLoad0(double value) {
        return value;
    }
    public static double exerciseDLoad1(int i1, double value) {
        return value;
    }
    public static double exerciseDLoad2(int i1, int i2, double value) {
        return value;
    }
    public static double exerciseDLoad3(int i1, int i2, int i3, double value) {
        return value;
    }
    public static double exerciseDLoad(int i1, int i2, int i3, int i4, double value) {
        return value;
    }
    public double exerciseDMul(double value1, double value2) {
        return value1 * value2;
    }
    public double exerciseDNeg(double value) {
        return -value;
    }
    public double exerciseDRem(double value1, double value2) {
        return value1 % value2;
    }
    public double exerciseDReturn(double value) {
        return value;
    }
    public static double exerciseDStore(int i1, int i2, int i3, int i4, double value1) {
        value1 = 1.0;
        return value1;
    }
    public static double exerciseDStore0(double value1) {
        value1 = 1.0;
        return value1;
    }
    public static double exerciseDStore1(int i1, double value1) {
        value1 = 1.0;
        return value1;
    }
    public static double exerciseDStore2(int i1, int i2, double value1) {
        value1 = 1.0;
        return value1;
    }
    public static double exerciseDStore3(int i1, int i2, int i3, double value1) {
        value1 = 1.0;
        return value1;
    }
    public double exerciseDSub(double value1, double value2) {
        return value1 - value2;
    }
}

class ExerciseArrayOpcodes
{
    public boolean[] exerciseNewArrayBoolean(int length) {
        return new boolean[length];
    }
    public byte[] exerciseNewArrayByte(int length) {
        return new byte[length];
    }
    public char[] exerciseNewArrayChar(int length) {
        return new char[length];
    }
    public short[] exerciseNewArrayShort(int length) {
        return new short[length];
    }
    public int[] exerciseNewArrayInt(int length) {
        return new int[length];
    }
    public float[] exerciseNewArrayFloat(int length) {
        return new float[length];
    }
    public long[] exerciseNewArrayLong(int length) {
        return new long[length];
    }
    public double[] exerciseNewArrayDouble(int length) {
        return new double[length];
    }
    public Object[] exerciseNewArrayObject(int length) {
        return new Object[length];
    }

    public boolean exerciseGetArrayElementBoolean(boolean[] ar, int index) {
        return ar[index];
    }
    public byte exerciseGetArrayElementByte(byte[] ar, int index) {
        return ar[index];
    }
    public byte exerciseGetArrayElementByte3(byte[] ar) {
        return ar[3];
    }
    public char exerciseGetArrayElementChar(char[] ar, int index) {
        return ar[index];
    }
    public short exerciseGetArrayElementShort(short[] ar, int index) {
        return ar[index];
    }
    public int exerciseGetArrayElementInt(int[] ar, int index) {
        return ar[index];
    }
    public float exerciseGetArrayElementFloat(float[] ar, int index) {
        return ar[index];
    }
    public long exerciseGetArrayElementLong(long[] ar, int index) {
        return ar[index];
    }
    public double exerciseGetArrayElementDouble(double[] ar, int index) {
        return ar[index];
    }
    public Object exerciseGetArrayElementObject(Object[] ar, int index) {
        return ar[index];
    }

    public void
    exerciseSetArrayElementBoolean(boolean[] ar, int index, boolean value) {
        ar[index] = value;
    }
    public void
    exerciseSetArrayElementByte(byte[] ar, int index, byte value) {
        ar[index] = value;
    }
    public void
    exerciseSetArrayElementByteI2B(byte[] ar, int index, int value) {
        ar[index] = (byte)value;
    }
    public void
    exerciseSetArrayElementChar(char[] ar, int index, char value) {
        ar[index] = value;
    }
    public void
    exerciseSetArrayElementCharI2C(char[] ar, int index, int value) {
        ar[index] = (char)value;
    }
    public void
    exerciseSetArrayElementShort(short[] ar, int index, short value) {
        ar[index] = value;
    }
    public void
    exerciseSetArrayElementShortI2S(short[] ar, int index, int value) {
        ar[index] = (short)value;
    }
    public void
    exerciseSetArrayElementInt(int[] ar, int index, int value) {
        ar[index] = value;
    }
    public void
    exerciseSetArrayElementFloat(float[] ar, int index, float value) {
        ar[index] = value;
    }
    public void
    exerciseSetArrayElementLong(long[] ar, int index, long value) {
        ar[index] = value;
    }
    public void
    exerciseSetArrayElementDouble(double[] ar, int index, double value) {
        ar[index] = value;
    }
    public void
    exerciseSetArrayElementObject(Object[] ar, int index, Object value) {
        ar[index] = value;
    }
    public Object exerciseMultiANewArray255() {
        return (Object)new Object
            [1/*001*/][1/*002*/][1/*003*/][1/*004*/][1/*005*/]
            [1/*006*/][1/*007*/][1/*008*/][1/*009*/][1/*010*/]
            [1/*011*/][1/*012*/][1/*013*/][1/*014*/][1/*015*/]
            [1/*016*/][1/*017*/][1/*018*/][1/*019*/][1/*020*/]
            [1/*021*/][1/*022*/][1/*023*/][1/*024*/][1/*025*/]
            [1/*026*/][1/*027*/][1/*028*/][1/*029*/][1/*030*/]
            [1/*031*/][1/*032*/][1/*033*/][1/*034*/][1/*035*/]
            [1/*036*/][1/*037*/][1/*038*/][1/*039*/][1/*040*/]
            [1/*041*/][1/*042*/][1/*043*/][1/*044*/][1/*045*/]
            [1/*046*/][1/*047*/][1/*048*/][1/*049*/][1/*050*/]
            [1/*051*/][1/*052*/][1/*053*/][1/*054*/][1/*055*/]
            [1/*056*/][1/*057*/][1/*058*/][1/*059*/][1/*060*/]
            [1/*061*/][1/*062*/][1/*063*/][1/*064*/][1/*065*/]
            [1/*066*/][1/*067*/][1/*068*/][1/*069*/][1/*070*/]
            [1/*071*/][1/*072*/][1/*073*/][1/*074*/][1/*075*/]
            [1/*076*/][1/*077*/][1/*078*/][1/*079*/][1/*080*/]
            [1/*081*/][1/*082*/][1/*083*/][1/*084*/][1/*085*/]
            [1/*086*/][1/*087*/][1/*088*/][1/*089*/][1/*090*/]
            [1/*091*/][1/*092*/][1/*093*/][1/*094*/][1/*095*/]
            [1/*096*/][1/*097*/][1/*098*/][1/*099*/][1/*100*/]
            [1/*101*/][1/*102*/][1/*103*/][1/*104*/][1/*105*/]
            [1/*106*/][1/*107*/][1/*108*/][1/*109*/][1/*110*/]
            [1/*111*/][1/*112*/][1/*113*/][1/*114*/][1/*115*/]
            [1/*116*/][1/*117*/][1/*118*/][1/*119*/][1/*120*/]
            [1/*121*/][1/*122*/][1/*123*/][1/*124*/][1/*125*/]
            [1/*126*/][1/*127*/][1/*128*/];
//            [1/*129*/][1/*130*/]
//            [1/*131*/][1/*132*/][1/*133*/][1/*134*/][1/*135*/]
//            [1/*136*/][1/*137*/][1/*138*/][1/*139*/][1/*140*/]
//            [1/*141*/][1/*142*/][1/*143*/][1/*144*/][1/*145*/]
//            [1/*146*/][1/*147*/][1/*148*/][1/*149*/][1/*150*/]
//            [1/*151*/][1/*152*/][1/*153*/][1/*154*/][1/*155*/]
//            [1/*156*/][1/*157*/][1/*158*/][1/*159*/][1/*160*/]
//            [1/*161*/][1/*162*/][1/*163*/][1/*164*/][1/*165*/]
//            [1/*166*/][1/*167*/][1/*168*/][1/*169*/][1/*170*/]
//            [1/*171*/][1/*172*/][1/*173*/][1/*174*/][1/*175*/]
//            [1/*176*/][1/*177*/][1/*178*/][1/*179*/][1/*180*/]
//            [1/*181*/][1/*182*/][1/*183*/][1/*184*/][1/*185*/]
//            [1/*186*/][1/*187*/][1/*188*/][1/*189*/][1/*190*/]
//            [1/*191*/][1/*192*/][1/*193*/][1/*194*/][1/*195*/]
//            [1/*196*/][1/*197*/][1/*198*/][1/*199*/][1/*200*/]
//            [1/*201*/][1/*202*/][1/*203*/][1/*204*/][1/*205*/]
//            [1/*206*/][1/*207*/][1/*208*/][1/*209*/][1/*210*/]
//            [1/*211*/][1/*212*/][1/*213*/][1/*214*/][1/*215*/]
//            [1/*216*/][1/*217*/][1/*218*/][1/*219*/][1/*220*/]
//            [1/*221*/][1/*222*/][1/*223*/][1/*224*/][1/*225*/]
//            [1/*226*/][1/*227*/][1/*228*/][1/*229*/][1/*230*/]
//            [1/*231*/][1/*232*/][1/*233*/][1/*234*/][1/*235*/]
//            [1/*236*/][1/*237*/][1/*238*/][1/*239*/][1/*240*/]
//            [1/*241*/][1/*242*/][1/*243*/][1/*244*/][1/*245*/]
//            [1/*246*/][1/*247*/][1/*248*/][1/*249*/][1/*250*/]
//            [1/*251*/][1/*252*/][1/*253*/][1/*254*/][1/*255*/]
//            ;
    }
}

