/*
 * @(#)LogicalVM.java	1.8 06/10/27
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

package sun.misc;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.InvocationTargetException;
import java.util.Vector;


/*
 * Comments (TODO) on the current Logical VM implementation:
 *
 * - Still under development.
 * - Java isolation API is still under discussion under JSR-121.
 *   Hopefully this implementation will cover major requirements.
 * - A parent LVM can die before its children, except the main LVM
 *   (the main LVM keeps itself alive until all the child LVMs get 
 *   terminated).
 * - Killing a parent LVM doesn't cause its children to die, except
 *   the main LVM (if we terminate the main LVM, all go die).
 * - No security checking done yet (no SecurityException).
 * - No state management (no "IllegalLogicalVMStateException" thrown)
 * - Uses remote (asynchronous) exception mechanism (ThreadDeath) when
 *   we terminate an LVM.  Enabling and disabling remote exceptions
 *   to protect key data structures are not completely revised yet.
 * - We also need to protect some of important finalizers (like one of
 *   FileOutputStream) so that it doesn't get terminated in the middle
 *   before freeing native resource by remote exceptions.
 * - Supposed to interrupt blocking operations performed by threads when
 *   we terminate an LVM, but this part is not implemented yet.
 *   We need to interrupt blocking I/O, monitor operations (including
 *   monitor enter), etc.
 * - Monitors for Class and String object are supposed to be replicated
 *   per LVM, but not implemented yet.  CVM code assumes one Class object
 *   per a class, so, instead of having multiple replicated Class objects
 *   per LVM, we replicate its monitor only.  There may need some other
 *   consideration so as for a Class object not to leak any objects from 
 *   one LVM to another.  As for String object, although it is immutable,
 *   the interned string support is making the situation difficult.
 *   Instead of going down the route to replicate the interned string 
 *   table in the VM, we just share the object and replicate its monitor 
 *   per LVM.
 * - None of JVMTI, JVMPI, RMI nor Personal[Basis]Profile work has done yet.
 */

/*
 * Implementation of a handle class of Logical VM
 */
public final class LogicalVM {
    private Isolate isolate;
    private LogicalVM(Isolate isolate) {
	this.isolate = isolate;
    }
    
    public LogicalVM(String className, String[] args, String n) {
	isolate = new Isolate(className, args, n);
    }

    public static LogicalVM getCurrentLogicalVM() {
	return new LogicalVM(Isolate.getCurrent());
    }

    public LogicalVM getParent() {
	return new LogicalVM(isolate.getParent());
    }

    public String getName() {
	// A String objects is immutable and shared (we use per-LVM 
	// String object lock).
	return isolate.getName();
    }

    public void start() {
	isolate.start();
    }

    public void join() throws InterruptedException {
	isolate.join();
    }
    public void join(long millis) throws InterruptedException {
	isolate.join(millis);
    }
    public void join(long millis, int nanos) throws InterruptedException {
	isolate.join(millis, nanos);
    }

    public void exit(int exitCode) {
	isolate.exit(exitCode);
    }

    public void halt(int exitCode) {
    	isolate.halt(exitCode);
    }

    public boolean equals(Object obj) {
	if (obj instanceof LogicalVM) {
	    LogicalVM other = (LogicalVM)obj;
	    return (this.isolate == other.isolate);
	}
	return false;
    }
}

/*
 * Actual implementation of Logical VM
 */
final class Isolate {
    // Used by the native code to store pointer to per-Logical VM 
    // info data structure.
    private int context = 0; /* TODO: What about 64 bit support. */

    private String name;
    private Isolate parent;
    private Isolate main;
    private Thread launcher;
    private RequestHandler requestHandler;

    private String className;
    private String[] args;

    // Special attention needs to be paid for use of static fields
    private static Isolate current;

    private native void initContext(Thread initThread);
    private native void destroyContext();
    private static native void switchContext(Isolate target);

    // Worm hole to invoke java.lang.Shutdown.waitAllUserThreads...()
    private static native void waitAllUserThreadsExitAndShutdown();
    // Worm hole to invoke java.lang.ref.Finalizer.runAllFinalizers()
    private static native void runAllFinalizersOfSystemClass();

    /*
     * Private constructor for the main (primordial) Logical VM.
     * This constructor will be invoked from native code during the JVM
     * start up and before completion of the JVM initialization.
     */
    private Isolate() {
	// We are still in the middle of main LVM initialization.
	// Do not perform anything fancy (like starting a new thread).
	name = "main";
	parent = null;
	main = this;
	current = this;
    }

    public Isolate(String className, String[] args, String name) {
	this.name = name;
	parent = getCurrent();
	main = parent.main;

	this.className = className;
	// Make duplicates of arguments so as not to share mutable objects 
	// across LVMs.  String objects are immutable and shared.
	this.args = new String[args.length];
	System.arraycopy(args, 0, this.args, 0, args.length);

	launcher = new Launcher("LVM " + name + " start-up thread");
    }

    public static Isolate getCurrent() {
	return current;
    }

    public Isolate getParent() {
	return parent;
    }

    public String getName() {
	return name;
    }

    public void start() {
	// Start this LVM form the context of the main LVM.
	// This effectively keeps the main LVM alive until this LVM 
	// terminates.
	main.remoteRequest(launcher);
    }

    public void join() throws InterruptedException {
	launcher.join();
    }
    public void join(long millis) throws InterruptedException {
	launcher.join(millis);
    }
    public void join(long millis, int nanos) throws InterruptedException {
	launcher.join(millis, nanos);
    }

    public void exit(int exitCode) {
	remoteHaltOrExit(this, false, exitCode);
    }

    public void halt(int exitCode) {
    	remoteHaltOrExit(this, true, exitCode);
    }

    private static void remoteHaltOrExit(Isolate target,
					 final boolean isHalt,
					 final int exitCode) {

	Isolate initiatingLVM = getCurrent();
	if (target != initiatingLVM) {
	    target.remoteRequest(new Runnable() {
		public void run() {
		    haltOrExit(isHalt, exitCode);
		}
	    });
	} else {
	    haltOrExit(isHalt, exitCode);
	}
    }

    private static void haltOrExit(boolean isHalt, int exitCode) {
	Runtime rt = Runtime.getRuntime();
	if (isHalt) {
	    rt.halt(exitCode);
	} else {
	    rt.exit(exitCode);
	}
    }

    private void startRequestHandler() {
	requestHandler = new RequestHandler("LVM " + name +" request handler");
	requestHandler.start();
    }

    private void startMainRequestHandler() {
	startRequestHandler();
	// The following line is for main LVM to make join()
	// implementation simpler.  Anyway, the main LVM never terminates 
	// while any of Java threads is running.  So, it doesn't matter.
	launcher = requestHandler;
    }

    private void remoteRequest(Runnable proc) {
	requestHandler.request(proc);
    }

    /*
     * Helper class for executing a piece of code in this LVM's context.
     */
    private class RequestHandler extends Thread {
	private Vector queue = new Vector();

	public RequestHandler(String name) {
	    super(name);
	    setDaemon(true);
	}

	public synchronized void run() {
	    setPriority(MAX_PRIORITY - 2);
	    current = Isolate.this;

	    while (!ThreadRegistry.exitRequested()) {
		if (queue.size() == 0) {
		    try {
			wait();
		    } catch (InterruptedException e) {
			continue;
		    }
		}
		Thread handler;
		Object proc = queue.remove(0);
		if (proc instanceof Thread) {
		    handler = (Thread)proc;
		} else {
		    handler = new Thread((Runnable)proc);
		}
		handler.setDaemon(false);
		handler.start();
	    }
	}

	public synchronized void request(Runnable proc) {
	    queue.add(proc);
	    notify();
	}
    }

    /*
     * Helper class for launching an application in a new Logical VM.
     */
    private class Launcher extends Thread {
	Launcher(String name) {
	    super(name);
	}

	public void run() {
	    // Don't allow any remote exception all the way
	    CVM.disableRemoteExceptions();

	    // Initializes a new Logical VM context and switch to it.
	    initContext(this);
	    current = Isolate.this;
	    setPriority(Thread.NORM_PRIORITY);

	    startRequestHandler();

	    Throwable uncaughtException = null;
	    try {
		// Obtain the main ThreadGroup.  There should only be 
		// one sub-group yet because of initContext().
		ThreadGroup sys = Thread.currentThread().getThreadGroup();
		ThreadGroup[] list = new ThreadGroup[1];
		sys.enumerate(list, false);
		ThreadGroup mainGroup = list[0];

		// Start-up the main thread of this LVM
		Thread main = new Thread(mainGroup, new MainExec(), "main");
		main.start();

		// Shutdown procedure for this LVM
		waitAllUserThreadsExitAndShutdown();
		ThreadRegistry.waitAllSystemThreadsExit();
		runAllFinalizersOfSystemClass();
	    } catch (Throwable t) {
		uncaughtException = t;
	    }
	    // Switch the context back to the main LVM so that this 
	    // thread gets removed from the main LVM's ThreadRegistry.
	    // In this way, we keep the main LVM alive while any of
	    // child LVMs is running.
	    switchContext(main);

	    // This is the last thread of this LVM
	    Isolate.this.destroyContext();

	    // Rethrow the uncaughtException and let the caller,
	    // Thread.startup(), deal with it.
	    if (uncaughtException != null) {
		CVM.throwLocalException(uncaughtException);
	    }
	}
    }

    private class MainExec implements Runnable {
	// Use Runnable so that a plain Thread object is used to run
	// the main method of the new LVM.
	public void run() {
	    Throwable uncaughtException = null;
	    try {
		// Load the target class using the system class loader.
		// This makes the main class loading procedure same
		// as the ordinary JVM start-up.
		ClassLoader scl = ClassLoader.getSystemClassLoader();
		Class cls = scl.loadClass(className);

		Class[] argClses = {String[].class};
		// Returns public method only
		Method mainMethod = cls.getMethod("main", argClses);
		if (mainMethod.getReturnType() != Void.TYPE) {
		    throw new NoSuchMethodException("return type is not void");
		}
		if (!Modifier.isStatic(mainMethod.getModifiers())) {
		    throw new IllegalAccessException("main is not static");
		}
		Object[] argObjs = {args};
		mainMethod.invoke(null, argObjs);
	    } catch (InvocationTargetException ite) {
		uncaughtException = ite.getTargetException();
	    } catch (Throwable t) {
		uncaughtException = t;
	    }
	    // Rethrow the uncaughtException and let the caller,
	    // Thread.startup(), deal with it.
	    if (uncaughtException != null) {
		CVM.throwLocalException(uncaughtException);
	    }
	}
    }

    private static void dprintln(String s) {
	if (CVM.checkDebugFlags(CVM.DEBUGFLAG_TRACE_LVM) != 0) {
	    System.err.println(s);
	}
    }
}
