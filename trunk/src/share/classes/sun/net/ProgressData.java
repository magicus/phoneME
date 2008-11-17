/*
 * @(#)ProgressData.java	1.25 06/10/10
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

package sun.net;

import java.util.Observer;
import java.util.Observable;
import java.net.URL;

/* Making update(), register(), and unregister() unsynchronized
 * since these make callbacks into arbitrary code and can cause
 * deadlocks if holding a monitor while doing so. Synchronize only 
 * necessary blocks that add/delete entries.  -brown 8/27/96
 */

public class ProgressData extends Observable {
    // We create a single instance of this class.
    // the Observer/Observable stuff only works with instances.
    //
    public static ProgressData pdata = new ProgressData();
    public static final int NEW = 0;
    public static final int CONNECTED = 1;
    public static final int UPDATE = 2;
    public static final int DELETE = 3;

    private ProgressEntry streams[] = new ProgressEntry[20];

    /** Get a snapshot of the internal state of of the streams */
     
    public synchronized ProgressEntry[] getStreams() {
	    return (ProgressEntry[]) streams.clone();
    }

    /**
     * Call this routine to register a new URL for the progress
     * window.  Until it is marked as connected this entry will have
     * a busy indicator.
     */
    public void register(ProgressEntry te) {
	int i;

	boolean localChanged = false;
	for (i = 0; i < streams.length; i++) {
	    synchronized (this) {
		if (streams[i] == null) {
		    streams[i] = te;
		    te.what = NEW;
		    te.index = i;
		    localChanged = true;
		    break;
		}
	    }
	}
	if (localChanged) {
	    setChanged();
	    /* only notify w/o holding lock */
	    notifyObservers(te);
	}
    }

    /**
     * Call this routine to register a new URL for the progress
     * window.  until it is marked as connected this entry will have
     * a busy indicator.
     */
    public void connected(URL m) {
	/* AVH: I made this a noop since it sends a CONNECT
	 * message when the first data arrives.
	 */
    }


    /**
     * Call this routine to unregister a new URL for the progress
     * window.  This will nuke the indicator from the ProgressWindow.
     */
    public void unregister(ProgressEntry te) {

	synchronized (this) {
	    int i = te.index;
	    if (i < 0 || i > streams.length || streams[i] != te) {
		return;
	    }
	    te.what = DELETE;
	    streams[i] = null;
	    setChanged();
	}
	notifyObservers(te);
    }

    public void update(ProgressEntry te) {
	/* Should get the URLConnection, 
	 * instead of the URL, in order to
	 * get the content-type 
	 */
	synchronized (this) {
	    int i = te.index;
	    if (i < 0 || i > streams.length || streams[i] != te) {
		return;
	    }
	    //	    te.update(total_read, total_need);
	    te.what = UPDATE;
	    if (!te.connected()) {
		// te.setType(m.getFile(), null/*m.content_type*/);
		te.what = CONNECTED;
	    }
	    if (te.read >= te.need && te.read != 0) {
		streams[i] = null;
		te.what = DELETE;
	    }
	    setChanged();
	}  /* end sync block */
	/* <DEBUG> -- try to force potential deadlocks in finalizers */
	// Runtime.getRuntime().gc();
	// Runtime.getRuntime().runFinalization();
	/* </DEBUG> */
	notifyObservers(te);
    }

}







