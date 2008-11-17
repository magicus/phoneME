/*
 * @(#)RunKBench.java	1.6 06/10/10
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
import sun.misc.CVM;

class ProgramData {
    String   name;
    long     p1;
    long     p2;
    ProgramData(String name, long p1, long p2) {
	this.name = name;
	this.p1 = p1;
	this.p2 = p2;
    }
}

public class RunKBench {
    static ProgramData tests5[] = {
	new ProgramData("BubbleSort", 7100L, 0L),
	new ProgramData("DualBubbleSort", 7900L, 0L),
	new ProgramData("QuickSort", 840000L, 0L),
	new ProgramData("Array", 2400000L, 0L),
	new ProgramData("Tree", 1150000L, 0L),
	new ProgramData("Hash", 410000L, 500L),
	new ProgramData("Strings", 5400L, 0L),
	new ProgramData("Sieve", 39000L, 300L),
	new ProgramData("Hanoi", 60L, 23L),
	new ProgramData("Fibonacci", 28000L, 1500L),
	new ProgramData("Dhrystone", 55000L, 10L),
	new ProgramData("QueenBench", 11L, 0L),
	new ProgramData("DeltaBlue", 47L, 0L),
	new ProgramData("Richards", 23L, 0L)
    };

    static ProgramData tests60[] = {
	new ProgramData("BubbleSort", 24000L, 0L),
	new ProgramData("DualBubbleSort", 26000L, 0L),
	new ProgramData("QuickSort", 8500000L, 0L),
	new ProgramData("Array", 25000000L, 0L),
	new ProgramData("Tree", 14000000L, 0L),
	new ProgramData("Hash", 4800000L, 500L),
	new ProgramData("Strings", 65000L, 0L),
	new ProgramData("Sieve", 350000L, 300L),
	new ProgramData("Hanoi", 30L, 26L),
	new ProgramData("Fibonacci", 330000L, 1500L),
	new ProgramData("Dhrystone", 560000L, 10L),
	new ProgramData("QueenBench", 12L, 0L),
	new ProgramData("DeltaBlue", 550L, 0L),
	new ProgramData("Richards", 270L, 0L)
    };

    public static void main(String[] args)
    {
	boolean longRun = false;
	boolean doInterpretedAndCompilePassFirst = false;

	String[] argsToPass;
	String[] kbItems = {
	    "-f", "kBenchItems.txt"
	};
	
	//
	// -long means do the long run rather than the short run.
	//
	if (args.length > 0) {
	    int i;
	    int numExtras = 0;
	    
	    for (i = 0; i < args.length; i++) {
		if (args[i].equals("-long")) {
		    longRun = true;
		    numExtras++;
                } else if (args[i].equals("-i")) {
                    doInterpretedAndCompilePassFirst = true;
                    numExtras++;
                }
	    }
		    
	    // Copy arguments, discard ones parsed above
	    if (numExtras > 0) {
		argsToPass = new String[args.length - numExtras];
		for (int j = numExtras; j < args.length; j++) {
		    argsToPass[j - numExtras] = args[j];
		}
	    } else {
		argsToPass = args;
	    }
	} else {
	    argsToPass = args;
	}

        if (doInterpretedAndCompilePassFirst) {
	    /*
	     * Run it once
	     */
	    System.err.println("Running kBench interpreted\n");
	    try {
	        runTests(argsToPass, true, false);
	    } catch (Exception e){
	        System.err.println("runTests() got an exception:");
	        e.printStackTrace();
	        System.exit(1);
	    }	

	    /*
	     * Compile the associated classes
	     */
	    System.err.println("Compiling named classes...\n");
	    CompilerTest.main(kbItems);
        }

	/*
	 * And run it again
	 */
	System.err.println("Running kBench compiled\n");
	try {
	    runTests(argsToPass, false, longRun);
	} catch (Exception e){
	    System.err.println("runTests() got an exception:");
	    e.printStackTrace();
	    System.exit(1);
	}
    }

    public static void runTests(String args[],
				boolean oneIteration, boolean longRun) {
	ProgramData tests[];
	if (longRun) {
	    tests = tests60;
	} else {
	    tests = tests5;
	}
	int nprog = tests.length;
	int testno = -1;
	int i;
	long totalMillis;
	
	totalMillis = 0L;
	
	if (args.length > 0) {
	    for (i = 0; i < args.length; i++) {
		try {
		    testno = Integer.parseInt(args[i]);
		    if (!oneIteration) {
			System.out.println("Running test " +
					   tests[testno].name);
		    }
		    totalMillis += runTest(tests[testno], oneIteration);
		} catch (Exception e){
		    System.out.println("Couldn't parse argument "+args[0]);
		    System.exit(1);
		}
	    }
	} else {
	    for (i = 0; i < nprog; i++){
		totalMillis += runTest(tests[i], oneIteration);
	    }
	}
	
	if (!oneIteration) {
	    System.out.println("Tests completed!");
	    System.out.println("	Total time spent: " + totalMillis +
			       " milliseconds.");
	}
    }
    
    static long runTest(ProgramData thisTest, boolean oneIteration){
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
	
	if (oneIteration) {
	    bench.setData(1, 1);
	} else {
	    bench.setData(thisTest.p1, thisTest.p2);
	}
	if (!oneIteration) {
	    System.out.println("Starting " + thisTest.name + "...");
	    System.gc();
	}
	benchMillis = bench.runTest();
	
	if (!oneIteration) {
	    System.out.println("	Time spent: " + benchMillis + 
			       " milliseconds.");
	}
	return benchMillis;
    }
}
