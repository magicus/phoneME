/*
 * @(#)Listener.java	1.20 06/10/10
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
package sun.mtask;

import sun.misc.ThreadRegistry;
import sun.misc.CVM;

import java.io.*;
import sun.mtask.AppModelManager;

//
// The class in charge of listening to requests from the mTASK process
//
public class Listener
{
    private static Listener listener = null;
    AppModelManager appm = null; // Application model specific handling

    // To be called by native code only
    private static void createListener()
    {
	if (listener != null) {
	    return; // Only one of these allowed in the system
	}
	listener = new Listener();
	listener.createListenerThread();
    }
    
    public static Listener getListener() {
	if (listener == null) {
	    createListener();
	}
	return listener;
    }

    private BufferedReader mtaskIn;
    private BufferedWriter mtaskOut;

    //
    // Get global mtask fd and assign it to a FileDescriptor object
    //
    private static native void setMtaskFd(FileDescriptor fdObj);
    
    //
    // Get mtask client ID
    //
    public static native int getMtaskClientId();

    //
    // Get mtask server port number
    //
    public static native int getMtaskServerPort();

    private static boolean verbose = (System.getProperty("cdcams.verbose") != null) &&
        (System.getProperty("cdcams.verbose").toLowerCase().equals("true"));
    
    //
    // Create a mTASK listener
    //
    public Listener() 
    {
	FileDescriptor fdInObj = new FileDescriptor();
	setMtaskFd(fdInObj); // Get the mtask fd and put it in fdInObj

	FileDescriptor fdOutObj = new FileDescriptor();
	setMtaskFd(fdOutObj); // Get the mtask fd and put it in fdOutObj

	//System.err.println("Listener created fdInObj="+fdInObj);
	mtaskIn = new BufferedReader(new FileReader(fdInObj));

	//System.err.println("Listener created fdOutObj="+fdOutObj);
	mtaskOut = new BufferedWriter(new FileWriter(fdOutObj));
    }

    public static void setAppModelManager(AppModelManager appm)
    {
	if (Listener.listener == null) {
	    throw new Error("listener uninitialized");
	}
	if (Listener.listener.appm != null) {
	    throw new Error("App manager of listener initialized more than once");
	}
	Listener.listener.appm = appm;
    }
    
    public static AppModelManager getAppModelManager()
    {
	if (Listener.listener == null) {
	    throw new Error("listener uninitialized");
	}
	return Listener.listener.appm;
    }
    
    private void createListenerThread() {
	ThreadGroup tg = Thread.currentThread().getThreadGroup();
	for (ThreadGroup tgn = tg;
	     tgn != null;
	     tg = tgn, tgn = tg.getParent());
	Thread lthread = new ListenerThread(tg, "mTASK listener", mtaskIn);
	/* If there were a special system-only priority greater than
	 * MAX_PRIORITY, it would be used here
	 */
	lthread.setPriority(Thread.MAX_PRIORITY);
	lthread.setDaemon(true);
	lthread.start();
    }
    
    private boolean needResponse = false;

    private boolean sendResponseDoit(String response) {
	boolean retval;
	if (!needResponse) {
	    return true;
	}
	
	try {
	    mtaskOut.write(response);
	    mtaskOut.flush();
	    retval = true;
	} catch (IOException e) {
	    e.printStackTrace();
	    retval = false;
	}
	needResponse = false;
	return retval;
    }

    public boolean sendPositiveResponse(String id) {
	return sendResponse("1", id);
    }

    public boolean sendNegativeResponse(String id) {
	return sendResponse("0", id);
    }

    public boolean sendResponse(String r, String id) {
	return sendResponseDoit(r+"/"+id+"\n");
    }

    //
    // Allow the listener to wait until certain initialization actions
    // have happened.
    //
    // This is triggered by the WAIT_FOR_LAUNCH message
    //
    private Object initialized = new Object();

    public void notifyInitialized() 
    {
	synchronized(initialized) {
	    initialized.notifyAll();
	}
    }

    private class ListenerThread extends Thread {
	private BufferedReader mtaskIn;

	ListenerThread(ThreadGroup g, String name,
		       BufferedReader mtaskIn) {
	    super(g, name);
	    this.mtaskIn = mtaskIn;
	}

	public void run() {
	    while (!ThreadRegistry.exitRequested()) {
		try {
		    String s = mtaskIn.readLine();
		    if (s == null) {
			// EOF
                        if (verbose) {
			    System.err.print("mTASK closed pipe\n");
                        }
			return;
		    }
		    int index = s.indexOf("/");
		    if (index == -1) {
                        if (verbose) {
			    System.err.println("Illegal message \""+s+"\"(no id)");
                        }
			return;
		    }
		    String sid = s.substring(index+1);
		    s = s.substring(0, index);
                    if (verbose) {
		        System.err.println("mTASK sent message=\""+
				       s+ "\", id="+sid);
                    } 
		    needResponse = true;
		    if ((appm != null) && appm.processMessage(s, sid)) {
			// Application management related message
			// successfully processed.
		    } else if (s.startsWith("GC")) {
			System.gc();
		    } else if (s.startsWith("STATS")) {
			CVM.dumpStats();
		    } else if (s.endsWith("EXIT")) {
			sendPositiveResponse(sid);
			System.exit(1);
		    } else if (s.equals("WAIT_FOR_LAUNCH")) {
			synchronized(initialized) {
			    try {
				initialized.wait();
			    } catch (InterruptedException e) {
				e.printStackTrace();
			    }
			}
		    } else {
			sendNegativeResponse(sid);
                        if (verbose) {
			    System.err.println("UNKNOWN MESSAGE: \""+s+"\"");
                        }
		    }
		    sendPositiveResponse(sid);
		} catch (IOException e) {
		    e.printStackTrace();
		    throw new Error();
		}
	    }
	}
    }
}
