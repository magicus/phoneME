/*
 * @(#)MyRunAll.java	1.9 06/10/10
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
 * @(#)MyRunAll.java	1.4 01/08/29
 */
import sun.misc.CVM;

public class MyRunAll {

static Program tests[] = {
    new Program("BubbleSort", 2000L, 0L),
    new Program("DualBubbleSort", 2250L, 0L),
    new Program("QuickSort", 100000L, 0L),
    new Program("Array", 500000L, 0L),
    new Program("Tree", 200000L, 0L),
    new Program("Hash", 100000L, 500L),
    new Program("Strings", 1000L, 0L),
    new Program("Sieve", 4095L, 300L),
    new Program("Hanoi", 20L, 20L),
    new Program("Fibonacci", 2400L, 1500L),
    new Program("Dhrystone", 6000L, 10L),
    new Program("QueenBench", 10L, 0L),
    new Program("DeltaBlue", 50L, 0L),
    new Program("Richards", 5L, 0L)
};

public static void
main(String args[]){
    int nprog = tests.length;
    int testno = -1;
    int i;
    long totalMillis;
    
    totalMillis = 0L;

    if (args.length > 0 ) {
	for (i = 0; i < args.length; i++) {
	    try {
		testno = Integer.parseInt(args[i]);
		System.out.println("Running test " + tests[testno].name);
		totalMillis += runTest(tests[testno]);
	    } catch (Exception e){
		System.out.println("Couldn't parse argument "+args[0]);
		System.exit(1);
	    }
	}
    } else {
	for (i = 0; i < nprog; i++){
	    totalMillis += runTest(tests[i]);
	}
    }
    
    System.out.println("Tests completed!");
    System.out.println("	Total time spent: " + totalMillis +
	" milliseconds.");
}

static long
runTest(Program thisTest){
    Benchmark bench;
    long benchMillis;
    try {
	bench = (Benchmark)(Class.forName(thisTest.name).newInstance());
    } catch (Exception e){
	System.out.println("Instantiating test "+thisTest.name+
	    " caused exception.");
	e.printStackTrace();
	return 0L;
    }

    bench.setData(thisTest.p1, thisTest.p2);
    System.out.println("Starting " + thisTest.name + "...");
    benchMillis = bench.runTest();
    System.gc();

    System.out.println("	Time spent: " + benchMillis + 
	" milliseconds.");
    return benchMillis;
}

}
