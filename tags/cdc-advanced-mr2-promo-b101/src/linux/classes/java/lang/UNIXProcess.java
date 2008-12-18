/*
 * @(#)UNIXProcess.java	1.38 06/10/10
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
 */

package java.lang;

import java.io.*; 

/* java.lang.Process subclass in the UNIX environment.
 * 
 */

class UNIXProcess extends Process {
    private FileDescriptor stdin_fd;
    private FileDescriptor stdout_fd;
    private FileDescriptor stderr_fd;
    private int pid;
    private int exitcode;
    private boolean hasExited;

    private OutputStream stdin_stream;
    private InputStream  stdout_stream;
    private InputStream  stderr_stream;

    /* this is for the reaping thread */
    private native int waitForProcessExit(int pid);

    private UNIXProcess() {}

    private native int forkAndExec(String cmd[], String env[], String path,
				   FileDescriptor stdin_fd,
				   FileDescriptor stdout_fd,
				   FileDescriptor stderr_fd)
	throws IOException;
  
    UNIXProcess(String cmdarray[], String env[]) throws IOException {
	this(cmdarray, env, null);
    }

    /* In the process constructor we wait on this gate until the process    */
    /* has been created. Then we return from the constructor.               */
    /* fork() is called by the same thread which later waits for the process */
    /* to terminate */

    private static class Gate {

        private boolean exited = false;
        private IOException savedException;

        synchronized void exit() { /* Opens the gate */
           exited = true;
           this.notify();
        }
        
        synchronized void waitForExit() { /* wait until the gate is open */
            boolean interrupted = false;
            while (!exited) {
                try {
                    this.wait();
                } catch (InterruptedException e) {
                    interrupted = true; 
                }
            }
            if (interrupted) {
                Thread.currentThread().interrupt();
            }
        }            

        void setException (IOException e) {
            savedException = e;
        }
     
        IOException getException() {
            return savedException;
        }
}

    UNIXProcess(final String cmdarray[], final String env[], final String path)
    throws IOException {
        
	stdin_fd = new FileDescriptor();
	stdout_fd = new FileDescriptor();
	stderr_fd = new FileDescriptor();

        final Gate gate = new Gate();	
	/*
	 * For each subprocess forked a corresponding reaper thread
	 * is started.  That thread is the only thread which waits
	 * for the subprocess to terminate and it doesn't hold any
	 * locks while doing so.  This design allows waitFor() and 
	 * exitStatus() to be safely executed in parallel (and they
	 * need no native code).
	 */
	 
	java.security.AccessController.doPrivileged(
			    new java.security.PrivilegedAction() {
	    public Object run() {
		Thread t = new Thread("process reaper") {
		    public void run() {
                        try {
                            pid = forkAndExec(cmdarray, env, 
                                          path, stdin_fd, stdout_fd, stderr_fd);
                        } catch (IOException e) {
                            gate.setException(e); /*remember to rethrow later*/
                            gate.exit();
                            return; 
                        } 
                        java.security.AccessController.doPrivileged(
                        new java.security.PrivilegedAction() {
                            public Object run() {
                            stdin_stream = new BufferedOutputStream(new
                                                    FileOutputStream(stdin_fd));
                            stdout_stream = new BufferedInputStream(new
                                                    FileInputStream(stdout_fd));
                            stderr_stream = new FileInputStream(stderr_fd);
                            return null;
                        }
                        });
                        gate.exit(); /* exit from contructor */ 
			int res = waitForProcessExit(pid);
			synchronized (UNIXProcess.this) {
			    hasExited = true;
			    exitcode = res;
			    UNIXProcess.this.notifyAll();
			}
		    }
		};
                t.setDaemon(true);
                t.start();
		return null;
	    }
	});
        gate.waitForExit();
        IOException e = gate.getException();
        if (e != null)
            throw new IOException(e.toString());
    }

    public OutputStream getOutputStream() {
	return stdin_stream;
    }

    public InputStream getInputStream() {
	return stdout_stream;
    }

    public InputStream getErrorStream() {
	return stderr_stream;
    }

    public synchronized int waitFor() throws InterruptedException {
        while (!hasExited) {
	    wait();
	}
	return exitcode;
    }

    public synchronized int exitValue() { 
	if (!hasExited) {
	    throw new IllegalThreadStateException("process hasn't exited");
	}
	return exitcode;
    }

    private static native void destroyProcess(int pid);
    public void destroy() {
	destroyProcess(pid);
        try {
            stdin_stream.close();
            stdout_stream.close();
            stderr_stream.close();
        } catch (IOException e) {
            // ignore
        }
    }

}
