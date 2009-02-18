/*
 * @(#)SunToolkit.java	1.34 06/10/10
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

package sun.awt;

import java.awt.*;
import java.awt.im.InputMethodHighlight;
import java.io.*;
import java.net.URL;
import java.util.Collections;
import java.util.Map;
import sun.awt.image.ByteArrayImageSource;
import sun.awt.image.FileImageSource;
import sun.awt.image.URLImageSource;
import sun.awt.im.InputMethod;
import sun.misc.SoftCache;

public abstract class SunToolkit extends Toolkit {
    // the system EventQueue
    // no longer static - EventQueue accessed through AppContext now
    //protected static EventQueue theEventQueue;
    
    /* The key to put()/get() the PostEventQueue into/from the AppContext.
     */
    private static final String POST_EVENT_QUEUE_KEY = "PostEventQueue";
    public SunToolkit() {
        EventQueue theEventQueue;
        String eqName = Toolkit.getProperty("AWT.EventQueueClass",
                "java.awt.EventQueue");
        try {
            theEventQueue = (EventQueue) Class.forName(eqName).newInstance();
        } catch (Exception e) {
            System.err.println("Failed loading " + eqName + ": " + e);
            theEventQueue = new EventQueue();
        }
        AppContext appContext = AppContext.getAppContext();
        appContext.put(AppContext.EVENT_QUEUE_KEY, theEventQueue);
        PostEventQueue postEventQueue = new PostEventQueue(theEventQueue);
        appContext.put(POST_EVENT_QUEUE_KEY, postEventQueue);
    }
    
    /*
     * Create a new AppContext, along with its EventQueue, for a
     * new ThreadGroup.  Browser code, for example, would use this
     * method to create an AppContext & EventQueue for an Applet.
     */
    public static AppContext createNewAppContext() {
        return createNewAppContext(Thread.currentThread().getThreadGroup());
    }

    /* 
     * Create a new AppContext with a given ThreadGroup 
     * Note that this method is directly called from AppContext itself only
     */
    static AppContext createNewAppContext(ThreadGroup threadGroup) {
        EventQueue eventQueue;
        String eqName = Toolkit.getProperty("AWT.EventQueueClass",
                "java.awt.EventQueue");
        try {
            eventQueue = (EventQueue) Class.forName(eqName).newInstance();
        } catch (Exception e) {
            System.err.println("Failed loading " + eqName + ": " + e);
            eventQueue = new EventQueue();
        }
        AppContext appContext = new AppContext(threadGroup);
        appContext.put(AppContext.EVENT_QUEUE_KEY, eventQueue);
        PostEventQueue postEventQueue = new PostEventQueue(eventQueue);
        appContext.put(POST_EVENT_QUEUE_KEY, postEventQueue);
        return appContext;
    }

    // mapping of Components to AppContexts, WeakHashMap<Component,AppContext>
    private static final Map appContextMap =
        Collections.synchronizedMap(new IdentityWeakHashMap());

    /*
     * Fetch the AppContext associated with the given target.
     * This can be used to determine things like which EventQueue
     * to use for posting events to a Component.  If the target is
     * null or the target can't be found, a null with be returned.
     */
    public static AppContext targetToAppContext(Object target) {
        if (target != null && !GraphicsEnvironment.isHeadless()) {
            return (AppContext) appContextMap.get(target);
        }
        return null;
    }

    /*
     * Insert a mapping from target to AppContext, for later retrieval
     * via targetToAppContext() above.
     */
    public static void insertTargetMapping(Object target, AppContext appContext) {
        if (!GraphicsEnvironment.isHeadless()) {
           appContextMap.put(target, appContext);
        }
    }

    protected EventQueue getSystemEventQueueImpl() {
        // EventQueue now accessed through AppContext now
        //return theEventQueue;
        AppContext appContext = AppContext.getAppContext();
        EventQueue theEventQueue =
            (EventQueue) appContext.get(AppContext.EVENT_QUEUE_KEY);
        return theEventQueue;
    }

    /*
     * Post an AWTEvent to the Java EventQueue, using the PostEventQueue
     * to avoid possibly calling client code (EventQueueSubclass.postEvent())
     * on the toolkit (AWT-Windows/AWT-Motif) thread.
     */
    public static void postEvent(AppContext appContext, AWTEvent event) {
        PostEventQueue postEventQueue =
            (PostEventQueue) appContext.get(POST_EVENT_QUEUE_KEY);
        // 6235492, check for null just like in jdk
        if (postEventQueue != null) { 
           postEventQueue.postEvent(event);
        }
    }

    public Dimension getScreenSize() {
        return new Dimension(getScreenWidth(), getScreenHeight());
    }
                                                                             
    /*
     * Gets the default encoding used on the current platform
     * to transfer text (character) data.
     */
    public abstract String getDefaultCharacterEncoding();
                                                                             
    protected abstract int getScreenWidth();
                                                                             
    protected abstract int getScreenHeight();
 
    static SoftCache imgCache = new SoftCache();

    static synchronized Image getImageFromHash(Toolkit tk, URL url) {
      
      // security check is done inside of getImageFromHash(tk, url.getFile());
      if (url.getProtocol().equals("file")) {
	   	 return getImageFromHash(tk, url.getFile());
	   }
	   SecurityManager sm = System.getSecurityManager();
	   if (sm != null) {
	    try {
		    java.security.Permission perm = 
		    url.openConnection().getPermission();
		if (perm != null) {
		    try {
			sm.checkPermission(perm);
		    } catch (SecurityException se) {
			// fallback to checkRead/checkConnect for pre 1.2
			// security managers
			if ((perm instanceof java.io.FilePermission) &&
			    perm.getActions().indexOf("read") != -1) {
			    sm.checkRead(perm.getName());
			} else if ((perm instanceof 
			    java.net.SocketPermission) &&
			    perm.getActions().indexOf("connect") != -1) {
			    sm.checkConnect(url.getHost(), url.getPort());
			} else {
			    throw se;
			}
		    }
		}
	    } catch (java.io.IOException ioe) {
		    sm.checkConnect(url.getHost(), url.getPort());
	    }
	}
	Image img = (Image)imgCache.get(url);
	if (img == null) {
	    try {	
		img = tk.createImage(new URLImageSource(url));
		imgCache.put(url, img);
	    } catch (Exception e) {
	    }
	}
	return img;
    }

    static synchronized Image getImageFromHash(Toolkit tk,
                                               String filename) {
	SecurityManager security = System.getSecurityManager();
	if (security != null) {
	    security.checkRead(filename);
	}
	Image img = (Image)imgCache.get(filename);
	if (img == null) {
	    try {	
		img = tk.createImage(new FileImageSource(filename));
		imgCache.put(filename, img);
	    } catch (Exception e) {
	    }
	}
	return img;
    }

    public Image getImage(String filename) {
	return getImageFromHash(this, filename);
    }

    public Image getImage(URL url) {
	return getImageFromHash(this, url);
    }

    public Image createImage(String filename) {
	SecurityManager security = System.getSecurityManager();
	if (security != null) {
	    security.checkRead(filename);
	}
	return createImage(new FileImageSource(filename));
    }

    public Image createImage(URL url) {
	SecurityManager sm = System.getSecurityManager();
	if (sm != null) {
	    try {
		java.security.Permission perm = 
		    url.openConnection().getPermission();
		if (perm != null) {
		    try {
			sm.checkPermission(perm);
		    } catch (SecurityException se) {
			// fallback to checkRead/checkConnect for pre 1.2
			// security managers
			if ((perm instanceof java.io.FilePermission) &&
			    perm.getActions().indexOf("read") != -1) {
			    sm.checkRead(perm.getName());
			} else if ((perm instanceof 
			    java.net.SocketPermission) &&
			    perm.getActions().indexOf("connect") != -1) {
			    sm.checkConnect(url.getHost(), url.getPort());
			} else {
			    throw se;
			}
		    }
		}
	    } catch (java.io.IOException ioe) {
		    sm.checkConnect(url.getHost(), url.getPort());
	    }
	}
	return createImage(new URLImageSource(url));
    }

    public Image createImage(byte[] data, int offset, int length) {
	return createImage(new ByteArrayImageSource(data, offset, length));
    }

    /**
     * Returns whether enableInputMethods should be set to true for peered
     * TextComponent instances on this platform. False by default.
     */
    public boolean enableInputMethodsForTextComponent() {
        return false;
    }

    /**
     *  Show the specified window in a multi-vm environment
     */
    public void activate(Window window) {
        return;
    }

    /**
     *  Hide the specified window in a multi-vm environment
     */
    public void deactivate(Window window) {
        return;
    }    
}

/*
 * PostEventQueue is a Thread that runs in the same AppContext as the
 * Java EventQueue.  It is a queue of AWTEvents to be posted to the
 * Java EventQueue.  The toolkit Thread (AWT-Windows/AWT-Motif) posts
 * events to this queue, which then calls EventQueue.postEvent().
 *
 * We do this because EventQueue.postEvent() may be overridden by client
 * code, and we mustn't ever call client code from the toolkit thread.
 */
class PostEventQueue extends Thread {
    static private int threadNum = 0;
    private EventQueueItem queueHead = null;
    private EventQueueItem queueTail = null;
    private boolean keepGoing = true;
    private final EventQueue eventQueue;
    PostEventQueue(EventQueue eq) {
        super("SunToolkit.PostEventQueue-" + threadNum);
        synchronized (PostEventQueue.class) {
            threadNum++;
        }
        eventQueue = eq;
        start();
    }

    /*
     * Continually post pending AWTEvents to the Java EventQueue.
     */
    public void run() {
        while (keepGoing && !isInterrupted()) {
            try {
                EventQueueItem item;
                synchronized(this) {
                    while (keepGoing && (queueHead == null)) {
                        notifyAll();
                        wait();
                    }
                    if (!keepGoing)
                        break;
                    item = queueHead;
                }
                eventQueue.postEvent(item.event);
                synchronized(this) {
                    queueHead = queueHead.next;
                    if (queueHead == null)
                        queueTail = null;
                }
            } catch (InterruptedException e) {
                keepGoing = false; // Exit gracefully when interrupted
                synchronized(this) {
                    notifyAll();
                }
            }
        }
    }
                                                                                                         
    /*
     * Enqueue an AWTEvent to be posted to the Java EventQueue.
     */
    synchronized void postEvent(AWTEvent event) {
        EventQueueItem item = new EventQueueItem(event);
        if (queueHead == null) {
            queueHead = queueTail = item;
            notifyAll();
        } else {
            queueTail.next = item;
            queueTail = item;
        }
    }
    
    /*
     * Wait for all pending events to be processed before returning.
     * If other events are posted before the queue empties, those
     * will also be processed before this method returns.
     */
    void flush() {
        if (Thread.currentThread() == this) {
            return; // Won't wait for itself
        }
        synchronized (this) {
            if (queueHead != null) {
                try {
                    wait();
                } catch (InterruptedException e) {}
            }
        }
    }

    /*
     * Notifies this PostEventQueue to stop running.
     */
    synchronized void quitRunning() {
        keepGoing = false;
        notifyAll();
    }
} // class PostEventQueue

class EventQueueItem {
    AWTEvent event;
    EventQueueItem next;
    EventQueueItem(AWTEvent evt) {
        event = evt;
    }
} // class EventQueueItem
