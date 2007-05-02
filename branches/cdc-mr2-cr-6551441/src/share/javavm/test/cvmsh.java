/*
 * @(#)cvmsh.java	1.6 06/10/10
 * 
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

import java.io.*;
import java.util.Vector;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

import sun.misc.VMInspector;

/*
 * cvmsh provides a shell for developers to use.  It is provide basic features
 * like launching applications either synchronously or in background threads,
 * as well as inspection tools to look query the state of the VM.
 */

public class cvmsh
{
    static final String[] helpMsg = {
	"Commands:",
	"  help                            - prints this list",
	"  gc                              - request a full GC cycle",
	"  memstat                         - prints VM heap memory usage",
	"  quit                            - exits this shell",
	"  enableGC                        - enables the GC",
	"  disableGC                       - disables the GC",
	"  keepObjectsAlive true|false     - forces the GC to keep all objs alive (or not)",
	"",
	"  Dumpers:",
	"  ========",
	"  print <objAddr>                 - print object as string",
	"  dumpObject <objAddr>            - dump the specified object",
	"  dumpClassBlock <cbAddr>         - dump the specified class",
	"  dumpObjectReferences <objAddr>  - dump all references to the obj",
	"  dumpClassReferences <classname> - dump all references to instances of the class",
	"  dumpClassBlocks <classname>     - dump all CBs of specified name",
	"  dumpHeap [simple|verbose|stats] - dump heap contents or info",
	"      - does a simple(default), verbose, or stats dump.",
	"",
	"  Capturing and Comparing Heap states:",
	"  ====================================",
	"  captureHeapState [<comment>]    - capture the current heap state",
	"  releaseHeapState <id>           - release the specified heap state",
	"  releaseAllHeapStates            - release all heap states",
	"  listHeapStates                  - list captured heap states",
	"  dumpHeapState <id> [obj|class]  - dump the specified heap state",
	"      - sort by obj or obj's class or none if unspecified.",
	"  compareHeapState <id1> <id2>    - compares the objs in the heap states",
	"",
	"  Misc utilities:",
	"  ===============",
	"  time <command>                  - computes time to execute specified command",
	"  run <class> [args ...]          - runs the specified app synchronously",
	"  bg <class> [args ...]           - runs the specified app in a new thread",
    };

    void doHelp() {
	printHelp(helpMsg);
    }

    void printHelp(String[] msg) {
	for (int i = 0; i < msg.length; i++) {
	    System.out.println(msg[i]);
	}
    }
    void doGC() {
	if (VMInspector.gcIsDisabled()) {
	    System.out.println("ERROR: GC is currently disabled.  " +
			       "Re-enable GC before invoking gc.");
	    return;
	}
	java.lang.Runtime.getRuntime().gc();
	doMemstat();
    }
    void doMemstat() {
	Runtime r = java.lang.Runtime.getRuntime();
        System.out.println("free memory = " + r.freeMemory());
        System.out.println("total memory = " + r.totalMemory());
    }

    void keepObjectsAlive(CmdStream cmd) {
	String token = getToken(cmd);
	try {
            boolean keepAlive = getBoolean(token);
            VMInspector.keepAllObjectsAlive(keepAlive);
        } catch (IllegalArgumentException e) {
	    System.out.println("ERROR: Expecting true or false instead of:" +
			       token);
        }
    }

    Object getObject(String token) {
        Object obj = null;
	try {
	    long addr = getLong(token);
            try {
                obj = VMInspector.addrToObject(addr);
            } catch (IllegalStateException e) {
                 System.out.println("ERROR: Need to disable GC before " +
                                    "calling print");
            } catch (IllegalArgumentException e) {
                System.out.println("ERROR: address " + token +
                                   " is not a valid object");
            }
	} catch (NumberFormatException e) {
	    System.out.println("ERROR: Invalid address format: " + token);
	}
        return obj;
    }
    long getAddr(String token) {
        long addr = 0L;
	try {
	    addr = getLong(token);
	} catch (NumberFormatException e) {
	    System.out.println("ERROR: Invalid address format: " + token);
	}
        return addr;
    }

    void print(CmdStream cmd) {
	String token = getToken(cmd);
        Object obj = getObject(token);
        if (obj != null) {
            if (obj instanceof String) {
                String s = (String)obj;
                System.out.println("String " + token + ": length= " +
                                    s.length());
                System.out.println("   value= \"" + s + "\"");
            } else {
                System.out.println("Object " + token + ": " + obj);
            }
        }
    }
    void dumpObject(CmdStream cmd) {
	String token = getToken(cmd);
        long addr = getAddr(token);
        if (addr != 0L) {
            VMInspector.dumpObject(addr);
	}
    }
    void dumpClassBlock(CmdStream cmd) {
	String token = getToken(cmd);
        long addr = getAddr(token);
        if (addr != 0L) {
	    VMInspector.dumpClassBlock(addr);
	}
    }
    void dumpObjectReferences(CmdStream cmd) {
	String token = getToken(cmd);
        long addr = getAddr(token);
        if (addr != 0L) {
	    VMInspector.dumpObjectReferences(addr);
	}
    }
    void dumpClassReferences(CmdStream cmd) {
	String token = getToken(cmd);
	if (token != "") {
            VMInspector.dumpClassReferences(token);
	} else {
	    System.out.println("ERROR: Classname not specified");
	}
    }
    void dumpClassBlocks(CmdStream cmd) {
	String token = getToken(cmd);
	if (token != "") {
            VMInspector.dumpClassBlocks(token);
	} else {
	    System.out.println("ERROR: Classname not specified");
	}
    }
    void dumpHeap(CmdStream cmd) {
	String token = getToken(cmd);
	if (token.equals("")) {
            VMInspector.dumpHeapSimple();
	} else if (token.equals("simple")) {
            VMInspector.dumpHeapSimple();
	} else if (token.equals("verbose")) {
            VMInspector.dumpHeapVerbose();
	} else if (token.equals("stats")) {
            VMInspector.dumpHeapStats();
	} else {
	    System.out.println("ERROR: Unsupported heap dump type: " + token);
	}
    }

    void captureHeapState(CmdStream cmd) {
	String name = cmd.input;
	if (name.equals("")) {
	    name = null;
	}
	VMInspector.captureHeapState(name);
    }
    void releaseHeapState(CmdStream cmd) {
	String token = getToken(cmd);
	try {
	    int id = getInt(token);
	    VMInspector.releaseHeapState(id);
	} catch (NumberFormatException e) {
	    System.out.println("ERROR: Invalid ID format: " + token);
	}
    }
    void dumpHeapState(CmdStream cmd) {
	String idToken = getToken(cmd);
	String sortKeyToken = getToken(cmd);
	try {
	    int id = getInt(idToken);
	    int sortKey = VMInspector.SORT_NONE;
	    if (sortKeyToken.equals("obj")) {
	        sortKey = VMInspector.SORT_BY_OBJ;
	    } else if (sortKeyToken.equals("class")) {
	        sortKey = VMInspector.SORT_BY_OBJCLASS;
	    }
	    VMInspector.dumpHeapState(id, sortKey);
	} catch (NumberFormatException e) {
	    System.out.println("ERROR: Invalid ID format: " + idToken);
	}
    }
    void compareHeapState(CmdStream cmd) {
	String token1 = getToken(cmd);
	String token2 = getToken(cmd);
	String currentToken = token1;
	try {
	    int id1 = getInt(token1);
	    currentToken = token2;
	    int id2 = getInt(token2);
	    VMInspector.compareHeapState(id1, id2);
	} catch (NumberFormatException e) {
	    System.out.println("ERROR: Invalid ID format: " + currentToken);
	}
    }

    class BackgroundThread extends Thread {
	String className;
	String[] args;
	cvmsh shell;

	BackgroundThread(cvmsh shell, String className, String[] args) {
	    this.className = className;
	    this.args = args;
	    this.shell = shell;
	}

	public void run() {
	    shell.run(className, args, false);
	}
    }

    void run(String className, String[] args, boolean background) {
	if (!background) {
	    try {
		Class cls = Class.forName(className);
		Class[] argClses = {String[].class};
		Method mainMethod = cls.getMethod("main", argClses);
		Object[] argObjs = {args};
		mainMethod.invoke(null, argObjs);
	    } catch (InvocationTargetException ite) {
		ite.getTargetException().printStackTrace();
	    } catch (Exception e) {
		e.printStackTrace();
	    }
	} else {
	    try {
		BackgroundThread t = new BackgroundThread(this, className, args);
		t.start();
	    } catch (Exception e) {
		e.printStackTrace();
	    }
	}
    }

    class CmdStream {
	CmdStream(String inputString) {
	    input = inputString;
	}
	String input;
    }
    String getToken(CmdStream cmd) {
	String input = cmd.input;
	String token;
	int index;
	index = input.indexOf(' ');
	// Get rid of leading whitespaces:
	while (index == 0) {
	    input = input.substring(index + 1);
	    index = input.indexOf(' ');
	}
	if (index == -1) {
	    token = input;
	    input = "";
	} else {
	    token = input.substring(0, index);
	    input = input.substring(index);
	}
	//System.out.println("TOKEN: \"" + token + "\" : \"" + input + "\"");
	cmd.input = input;
	return token;
    }
    String[] getArgs(CmdStream cmd) {
	String[] args = new String[0];
	Vector argsVec = new Vector();
	String token;

	token = getToken(cmd);
	while (!token.equals("")) {
	    argsVec.add(token);
	    token = getToken(cmd);
	}
	args = (String[])argsVec.toArray(args);
        return args;
    }

    static int getInt(String token) throws NumberFormatException {
        int result;
	int radix = 10;
        if (token.startsWith("0x")) {
	    int index;
	    index = token.indexOf('x');
	    token = token.substring(index + 1);
	    radix = 16;
        }
        result = Integer.parseInt(token, radix);
	return result;
    }
    static long getLong(String token) throws NumberFormatException {
        long result;
	int radix = 10;
        if (token.startsWith("0x")) {
	    int index;
	    index = token.indexOf('x');
	    token = token.substring(index + 1);
	    radix = 16;
        }
        result = Long.parseLong(token, radix);
	return result;
    }
    static boolean getBoolean(String token)
        throws IllegalArgumentException {
        boolean result = false;
	if (token.toLowerCase().equals("true")) {
	    result = true;
	} else if (token.toLowerCase().equals("false")) {
	    result = false;
	} else {
            throw new IllegalArgumentException();
	}
        return result;
    }

    boolean processCmd(CmdStream cmd) {
	String token;
	token = getToken(cmd);
	return processToken(token, cmd);
    }

    boolean processToken(String token, CmdStream cmd) {
	if (token.equals("quit")) {
	    return true;
	} else if (token.equals("help")) {
	    doHelp();
	} else if (token.equals("gc")) {
	    // Release everything we can before GCing:
	    token = null;
	    cmd.input = null;
	    doGC();
	} else if (token.equals("memstat")) {
	    doMemstat();
	} else if (token.equals("enableGC")) {
	    VMInspector.enableGC();
	} else if (token.equals("disableGC")) {
	    VMInspector.disableGC();
	} else if (token.equals("keepObjectsAlive")) {
	    keepObjectsAlive(cmd);
	} else if (token.equals("print")) {
	    print(cmd);
	} else if (token.equals("dumpObject")) {
	    dumpObject(cmd);
	} else if (token.equals("dumpClassBlock")) {
	    dumpClassBlock(cmd);
	} else if (token.equals("dumpObjectReferences")) {
	    dumpObjectReferences(cmd);
	} else if (token.equals("dumpClassReferences")) {
	    dumpClassReferences(cmd);
	} else if (token.equals("dumpClassBlocks")) {
	    dumpClassBlocks(cmd);
	} else if (token.equals("dumpHeap")) {
	    dumpHeap(cmd);
	} else if (token.equals("captureHeapState")) {
	    captureHeapState(cmd);
	} else if (token.equals("releaseHeapState")) {
	    releaseHeapState(cmd);
	} else if (token.equals("releaseAllHeapStates")) {
	    VMInspector.releaseAllHeapState();
	} else if (token.equals("listHeapStates")) {
	    VMInspector.listHeapStates();
	} else if (token.equals("dumpHeapState")) {
	    dumpHeapState(cmd);
	} else if (token.equals("compareHeapState")) {
	    compareHeapState(cmd);
	} else if (token.equals("run")) {
	    String classname = getToken(cmd);
	    String[] args = getArgs(cmd);
	    run(classname, args, false);
	} else if (token.equals("bg")) {
	    String classname = getToken(cmd);
	    String[] args = getArgs(cmd);
	    run(classname, args, true);
	} else if (token.equals("time")) {
	    long startTime = System.currentTimeMillis();
	    processCmd(cmd);
	    long endTime = System.currentTimeMillis();
	    System.out.println("Time elapsed: " + (endTime - startTime) + " ms");
	} else {
 	    System.out.print("Unknown command: \"" + token);
	    if (!cmd.input.equals("")) {
 	        System.out.print(cmd.input);
	    }
	    System.out.println("\"");
	    System.out.println("type \"help\" for available commands");
	}
	return false;
    }

    public void runShell(String[] args) {
        InputStreamReader in = new InputStreamReader(System.in);
        int numberOfChars;
	char[] buf = new char[1000];

	try {
	    boolean done = false;
	    while (!done) {
		String input;
		System.out.print("> ");
		numberOfChars = in.read(buf, 0, buf.length);
		// Don't include the '\n' at the end of the buffer in the
		// string:
		input = new String(buf, 0, numberOfChars - 1);
		//System.out.println("READ: " + input);

		CmdStream cmd = new CmdStream(input);
		input = null; // Release it.
		done = processCmd(cmd);
	    }
	} catch (Throwable e) {
	    System.err.println("ERROR: " + e);
	}
    }


    public static void main(String[] args) {
	cvmsh sh = new cvmsh();
	sh.runShell(args);
    }
}
