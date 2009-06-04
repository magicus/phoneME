/*
 * @(#)SymbianProcess.java	1.3 06/10/10
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

class SymbianProcess extends Process {
    private long handle = 0;
    private FileDescriptor stdin_fd;
    private FileDescriptor stdout_fd;
    private FileDescriptor stderr_fd;
    private OutputStream stdin_stream;
    private InputStream stdout_stream;
    private InputStream stderr_stream;
    private int exitcode;
    private boolean hasExited;

    SymbianProcess(String cmd[], String env[]) throws Exception {
        this(cmd, env, null);
    }
    
    SymbianProcess(String cmd[], String env[], String path) throws Exception {
	StringBuffer cmdbuf = new StringBuffer(80);
	String exe = "";
	if (cmd.length > 0) {
	    exe = cmd[0];
	}
	for (int i = 1; i < cmd.length; i++) {
            if (i > 0) {
                cmdbuf.append(' ');
            }
	    String s = cmd[i];
	    if (s.indexOf(' ') >= 0 || s.indexOf('\t') >= 0) {
	        if (s.charAt(0) != '"') {
		    cmdbuf.append('"');
		    cmdbuf.append(s);
		    if (s.endsWith("\\")) {
			cmdbuf.append("\\");
		    }
		    cmdbuf.append('"');
                } else if (s.endsWith("\"")) {
		    /* The argument has already been quoted. */
		    cmdbuf.append(s);
		} else {
		    /* Unmatched quote for the argument. */
		    throw new IllegalArgumentException();
		}
	    } else {
	        cmdbuf.append(s);
	    }
	}
	String cmdstr = cmdbuf.toString();

        String envstr = null;
        if (env != null) {
            StringBuffer envbuf = new StringBuffer(256);
            for (int i = 0; i < env.length; i++) {
                envbuf.append(env[i]).append('\0');
            }
            envstr = envbuf.toString();
        }

	stdin_fd = new FileDescriptor();
	stdout_fd = new FileDescriptor();
	stderr_fd = new FileDescriptor();
	
	handle = create(exe, cmdstr, envstr, path,
	    stdin_fd, stdout_fd, stderr_fd);
	java.security.AccessController.doPrivileged(
				    new java.security.PrivilegedAction() {
	    public Object run() {
		stdin_stream = 
		    new BufferedOutputStream(new FileOutputStream(stdin_fd));
		stdout_stream =
		    new BufferedInputStream(new FileInputStream(stdout_fd));
		stderr_stream = 
		    new FileInputStream(stderr_fd);

		/*
		 * For each subprocess forked a corresponding reaper thread
		 * is started.  That thread is the only thread which waits
		 * for the subprocess to terminate and it doesn't hold any
		 * locks while doing so.  This design allows waitFor() and
		 * exitStatus() to be safely executed in parallel (and they
		 * need no native code).
		 */

		Thread t = new Thread("process reaper") {
		    public void run() {
			int res = waitFor0();
			synchronized (SymbianProcess.this) {
			    hasExited = true;
			    exitcode = res;
			    SymbianProcess.this.notifyAll();
			}
		    }
		};
                t.setDaemon(true);
                t.start();

		return null;
	    }
	});
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

    public void finalize() {
	close();
    }

    private native int waitFor0();
    public native void destroy();

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


    private native long create(String exe, String args,
			       String envstr, String path,
			       FileDescriptor in_fd,
			       FileDescriptor out_fd,
			       FileDescriptor err_fd);
    private native void close();
}
