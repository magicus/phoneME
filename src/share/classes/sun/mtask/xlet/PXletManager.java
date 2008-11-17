/*
 * @(#)PXletManager.java	1.26 06/10/10
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

package sun.mtask.xlet;

// Standard classes used by xlets.
import javax.microedition.xlet.*;

// For Inter-Xlet Communication (IXC).
import javax.microedition.xlet.ixc.*;

// Provides graphic representations and event support for xlets.
import java.awt.Container;
import java.awt.Frame;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

// Utility classes for saving information about xlets.
import java.util.Hashtable;
import java.util.Vector;

//For loading xlets.
import java.io.IOException;
import java.io.File;
import java.net.URL;
import java.net.MalformedURLException;
import java.lang.reflect.Constructor;
import java.rmi.AccessException;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.security.PrivilegedExceptionAction;
import java.security.PrivilegedActionException;
import sun.misc.CDCAppClassLoader;

import com.sun.xlet.XletLifecycleHandler;

/*
 * Xlet Manager implementation.
 *
 * The main purpose of this class is to initiate the xlet’s state change based 
 * on the request coming in from the PXletStateQueue and to keep track of 
 * the xlet’s current state. It also loads and destroys xlets.
 *
 * Static data is shared across all xlets. Every instance of an
 * XletManager manages one xlet. The XletManager runs a thread for
 * every xlet, which waits until the xlet is destroyed, then performs
 * the cleanup. The actual state change can be done either by
 * posting the request through XletLifecycleHandler (which inserts the
 * request into XletEventQueue), or through XletContext (which notifies the
 * XletManager that the xlet already changed its state).
 */

public class PXletManager implements XletLifecycleHandler {
    private static final int DEFAULT_XLET_WIDTH = 300;
    private static final int DEFAULT_XLET_HEIGHT = 300;
    
    // The root frame of this xlet
    private XletFrame xletFrame; 

    // The XletContext which this instance of PXletManager is managing.
    private PXletContextImpl context; 

    // The xlet class instance.
    private Class xletClass;  

    // The xlet itself.
    private Xlet xlet;  

    // This xlet’s state queue. The state change request is usually posted 
    // to the XletEventQueue. 
    private PXletStateQueue xletQueue;  

    // This xlet’s thread group (Package private).
    ThreadGroup threadGroup;  

    // To synchronize the state change.
    // Synchronize the existing current state and desired state with a new
    // current state or desired state during a state transition. 
    private Object stateGuard = new Object();

    // The current state of the xlet that this instance of XletManager is 
    // managing. The XletManager is always trying to move the Xlet from the
    // current state to the desired state. The first state transition would be 
    // from unloaded to loaded.
    private XletState currentState = XletState.UNLOADED;

    private static boolean verbose = (System.getProperty("pxletmanager.verbose") != null) &&
        (System.getProperty("pxletmanager.verbose").toLowerCase().equals("true"));

    // Load the xlet class instance with a ClassLoader.
    // mainClass is the main class of the Xlet.
    // args contains the command-line arguments supplied with the arg option.
    // They will be stored in the XletContext.
    protected PXletManager(ClassLoader xletClassLoader,
			   String laf,
			   String lafTheme,
			   String mainClass,
			   String[] args) throws ClassNotFoundException {

       xletClass = xletClassLoader.loadClass(mainClass);
         
       if (xletClass == null || (!(Xlet.class).isAssignableFrom(xletClass))) {
          throw new IllegalArgumentException(
             "Attempt to run a non-Xlet class: " + mainClass);
       }

       // Create the XletContext for this xlet.
       context = new PXletContextImpl(mainClass, args, this);
       this.xletClass = xletClass;
      
       // Create a root Frame for all xlets. Every time a new xlet is
       // added, it gets its own container inside this Frame. The XletManager
       // sets a default size and location, adds a menu bar and window
       // listener, and leaves the Frame invisible.
       xletFrame = new XletFrame("Xlet Frame: " + mainClass, this,
				 laf, lafTheme); 
       
       // Create a thread group that all the threads used by this xlet
       // would be a part of. This is necessary for providing a separate
       // EventQueue per xlet.
       threadGroup = new ThreadGroup(Thread.currentThread().getThreadGroup(),
				     "Xlet Thread Group ");
       
       // Create a state queue that holds this xlet’s state change requests.
       xletQueue = new PXletStateQueue(this);
       
       // Now that the setup is complete, enter a request to move this xlet
       // from UNLOADED to LOADED.
       xletQueue.push(XletState.LOADED);
    }

    // Return a container for this xlet.
    // This will be called at most once, by PXletContextImpl.getContainer()
    public Container getContainer() {
	return xletFrame.getContainer();
    }

    //private Listener listener = null;
    
    // The following four methods are implementations of XletLifecycleHandler
    // methods. They allow a third party to request an xlet state change.
    // They request a state change through the xlet state queue (which holds
    // an xlet’s state change requests).
    public void postInitXlet() {
        xletQueue.push(DesiredXletState.INITIALIZE);
    }

    public void postStartXlet() {
        xletQueue.push(XletState.ACTIVE);
    }

    public void postPauseXlet() {
        xletQueue.push(XletState.PAUSED);
    }
 
    //
    // The xletDestroy() from the appmanager is best effort.
    // The appmanager inspects state. If it discovers that this jvm
    // has not exited, it can use Client.kill() to forcibly get rid of it.
    //
    public void postDestroyXlet(boolean unconditional) {
        if (!unconditional) {
            xletQueue.push(DesiredXletState.CONDITIONAL_DESTROY);
        } else {
            xletQueue.push(XletState.DESTROYED);
        }
    }

    // Set the state of the xlet. If the current state is destroyed, don’t
    // bother to set the state. If the target state is destroyed,
    // exit this vm instance.
    public void setState(XletState state) { 
        synchronized (stateGuard) {
            if (currentState == XletState.DESTROYED) 
                return;
            currentState = state;
            stateGuard.notifyAll();
        }
	// 
	// If we are requesting a state change into DESTROYED, we want to
	// go away. So don't linger, get rid of the enclosing JVM instance
	// 
	if (state == XletState.DESTROYED) {
            if (verbose) {
                System.err.println("@@PXletManager setting xlet state to DESTROYED");
            }
            System.exit(0);
	}
    }

    // Implementation of XletLifecycleHandler. Allows a third party to query
    // the xlet state. 
    public int getState() { 
        XletState state = getXletState();
        if (state == XletState.LOADED) {
            return LOADED;
        } else if (state == XletState.PAUSED) {
            return PAUSED;
        } else if (state == XletState.ACTIVE) {
            return ACTIVE;
        } else if (state == XletState.DESTROYED) {
            return DESTROYED;
        } else {
            return UNKNOWN;
        }
    }

    // Used internally. Returns the current XletState as an instance of
    // XletState rather than as an integer.
    public XletState getXletState() { 
	return currentState;
    }

    // Keeps track of whether the xlet state change request was fulfilled or not.
    boolean requestCompleted = false;
    // Typically called from PXletStateQueue. Handles xlet’s state change
    // request. Makes sure it only performs a permissible state change.
    public void handleRequest(XletState desiredState) {
        XletState targetState = currentState;
        requestCompleted = false;
        try { 
            synchronized (stateGuard) {
                if (desiredState == XletState.LOADED) {
                    if (currentState != XletState.UNLOADED) 
                        return;
                    targetState = XletState.LOADED;
                    Class[] types = new Class[0];
                    Constructor m = xletClass.getConstructor(types);
                    xlet = (Xlet) m.newInstance(new Object[0]);

                } else if (desiredState == DesiredXletState.INITIALIZE) {
                    if (currentState != XletState.LOADED) 
                        return;
                    targetState = XletState.PAUSED;
                    try {
                        xlet.initXlet(context);

                    } catch (XletStateChangeException xsce) {
                        targetState = XletState.DESTROYED;
			//
			// The xletDestroy() from the appmanager is best
			// effort.  The appmanager inspects state. If it
			// discovers that this jvm has not exited, it can use
			// Client.kill() to forcibly get rid of it.  
                        //
			try {
			    xlet.destroyXlet(true);
			} catch (XletStateChangeException xsce2) {
			    // Xlet refused to go away
			    // This is unconditional though so ignore
			    // exception. The right thing will happen
			    // when setState(DESTROYED) is called.
			} 
		    }
                } else if (desiredState == XletState.ACTIVE) {
                    if (currentState != XletState.PAUSED) 
                        return;
                    targetState = XletState.ACTIVE;
                    try {
                        xlet.startXlet();
                    } catch (XletStateChangeException xsce) {
			//
			// The spec is not explicit here.
			// If you can't activate the xlet it
			// lingers in its paused state.
			//
                        targetState = currentState;
		    }
                } else if (desiredState == XletState.PAUSED) {
                    if (currentState != XletState.ACTIVE) 
                        return;
                    targetState = XletState.PAUSED;
                    xlet.pauseXlet();
                } else if (desiredState == DesiredXletState.CONDITIONAL_DESTROY) {
                    if (currentState == XletState.DESTROYED) 
                        return;
                    targetState = XletState.DESTROYED;
                    try {
                        xlet.destroyXlet(false);
                    } catch (XletStateChangeException xsce) {
			// Xlet refused to go away
                        if (verbose) {
			    System.err.println("XLET REFUSED TO GO AWAY\n");
                        }
                        targetState = currentState;
                    } 
                } else if (desiredState == XletState.DESTROYED) {
                    targetState = XletState.DESTROYED;
                    if (currentState == XletState.DESTROYED) 
                        return;
                    try {
                        xlet.destroyXlet(true);
                    } catch (XletStateChangeException xsce) {
			// Xlet refused to go away
			// This is unconditional though so ignore
			// exception. The right thing will happen
			// when setState(DESTROYED) is called.
		    }
                } 
                setState(targetState);
            }
        } catch (Throwable e) {
            if (verbose) {
                if (verbose) {
	            System.err.println("EXCEPTION DURING XLET STATE HANDLING: "+e);
                }
            }
	    e.printStackTrace();
            if (targetState == XletState.DESTROYED) {
                setState(XletState.DESTROYED);
            } else {
                handleRequest(XletState.DESTROYED);
            }
        }
        requestCompleted = true;
    }

    // Creates an xlet. This static method causes a new instance of
    // XletManager to be created, and to load the xlet.
    public static XletLifecycleHandler createXlet(String mainClass,
						  String laf,
						  String lafTheme)
        throws IOException {
        return (XletLifecycleHandler)createXlet(mainClass, laf, lafTheme, new String[] {"."}, new String[]{});
    }

    public static PXletManager createXlet(String mainClass,
					  String laf,
					  String lafTheme,
					  String[] paths,
					  String[] args)
        throws IOException {
 
        Vector v = new Vector();		 
        for (int i = 0; i < paths.length; i++) {
           URL url = parseURL(paths[i]);
           if (url != null) {
              v.add(url);
           }
        }

        PXletClassLoader cl = new PXletClassLoader(
            (URL[])v.toArray(new URL[0]), null);

        try {
            return new PXletManager(cl, laf, lafTheme, mainClass, args);
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
            throw new IOException("Cannot find class " + mainClass);
        }
    }

     /**
     * Following the relevant RFC, construct a valid URL based on the passed in
     * string.
     *
     * @param url  A string which represents either a relative or absolute URL,
     *             or a string that could be an absolute or relative path.
     * @return     A URL when the passed in string can be interpreted according
     *             to the RFC.  <code>null</code> otherwise.
     */
    private static URL parseURL(String url) {
        URL u = null;
        try {
            if (url.startsWith(File.separator)) {
                 // absolute path
                 u = new File(url).toURL();
            } else if (url.indexOf(':') <= 1) {
                // we were passed in a relative URL or an absolute URL on a
                // win32 machine
                u = new File(System.getProperty("user.dir"), url).getCanonicalFile().toURL();
            } else {
                if (url.startsWith("file:") &&
                    url.replace(File.separatorChar, '/').indexOf('/') == -1) {
                    // We were passed in a relative "file" URL, like this:
                    //     "file:index.html".
                    // Prepend current directory location.
                    String fname = url.substring("file:".length());
                    if (fname.length() > 0) {
                        u = new File(System.getProperty("user.dir"), fname).toURL();
                    } else {
                        u = new URL(url);
                    }
                } else {
                    u = new URL(url);
                }
            }
        } catch (IOException e) {
            if (verbose) {
                System.err.println("error in parsing: " + url);
            }
        }
        return u;
    }
}

class PXletClassLoader extends CDCAppClassLoader {
    public PXletClassLoader(URL[] urls, ClassLoader parent) {
       super(urls, parent);
    }
}
