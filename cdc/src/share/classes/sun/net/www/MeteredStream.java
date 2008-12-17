/*
 * @(#)MeteredStream.java	1.41 06/10/10
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

package sun.net.www;

import java.net.URL;
import java.io.*;
import sun.net.ProgressData;
import sun.net.ProgressEntry;

public 
class MeteredStream extends FilterInputStream {

    // Instance variables.
    /* after we've read >= expected, we're "closed" and return -1
     * from subsequest read() 's
     */
    protected boolean closed = false;
    protected int expected;
    protected int count = 0; 
    protected ProgressEntry te;

    public MeteredStream(InputStream is, ProgressEntry te) {
	super(is);
	this.te = te;
	expected = te.need;
	ProgressData.pdata.update(te);
    }

    private final void justRead(int n) throws IOException {
	if (n == -1) {
	    close();
	    return;
	}
	count += n;
	te.update(count, expected);
	ProgressData.pdata.update(te);
	if (count >= expected) {
	    close();
	}
    }

    public synchronized int read() throws java.io.IOException {
	if (closed) {
	    return -1;
	}
	int c = in.read();
	if (c != -1) {
	    justRead(1);
	} else {
	    close();
	}
	return c;
    }

    public synchronized int read(byte b[], int off, int len) throws java.io.IOException {
	if (closed) {
	    return -1;
	}
	int n = in.read(b, off, len);
	justRead(n);
	return n;
    }
    
    public synchronized long skip(long n) throws IOException {
	if (closed) {
	    return 0;
	}
	// just skip min(n, num_bytes_left)
	int min = (n > expected - count) ? expected - count: (int)n;
	n = in.skip(min);
	justRead((int)n);
	return n;
    }

    public void close() throws IOException {
	if (closed) {
	    return;
	}
	ProgressData.pdata.unregister(te);
	closed = true;
	in.close();
    }

    public synchronized int available() throws IOException {
	return closed ? 0: in.available();
    }

    public synchronized void mark(int readlimit) {
	if (closed) {
	    return;
	}
	super.mark(readlimit);
    }

    public synchronized void reset() throws IOException {
	if (closed) {
	    return;
	}
	super.reset();
    }

    public boolean markSupported() {
	if (closed) {
	    return false;
	}
	return super.markSupported();
    }

    protected void finalize() {
	/* Do not explicitly de-register from finalizer.  THIS MEANS YOU! */
	te.what = ProgressData.DELETE;
    }	
}

