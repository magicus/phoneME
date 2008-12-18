/*
 * @(#)AppletPanel.java	1.69 06/10/10
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

package sun.applet;

import java.util.*;
import java.io.*;
import java.net.URL;
import java.net.MalformedURLException;
import java.net.SocketPermission;
import java.net.UnknownHostException;
import java.net.InetAddress;
import java.net.JarURLConnection;
import java.awt.*;
import java.awt.event.*;
import java.applet.*;
import java.awt.image.MemoryImageSource;
import java.awt.image.ColorModel;
import java.security.*;
import java.util.Collections;
import java.util.WeakHashMap;
import sun.misc.MessageUtils;
import sun.misc.Queue;

/**
 * Applet panel class. The panel manages and manipulates the
 * applet as it is being loaded. It forks a separate thread in a new
 * thread group to call the applet's init(), start(), stop(), and
 * destroy() methods.
 *
 * @version 	1.62, 08/19/02
 * @author 	Arthur van Hoff
 */
public
abstract class AppletPanel extends Panel implements AppletStub, Runnable {
    /**
     * The applet (if loaded).
     */
    Applet applet;
    /**
     * Applet will allow initialization.  Should be
     * set to false if loading a serialized applet
     * that was pickled in the init=true state.
     */
    protected boolean doInit = true;
    /**
     * The classloader for the applet.
     */
    AppletClassLoader loader;
    /* applet event ids */
    public final static int APPLET_DISPOSE = 0;
    public final static int APPLET_LOAD = 1;
    public final static int APPLET_INIT = 2;
    public final static int APPLET_START = 3;
    public final static int APPLET_STOP = 4;
    public final static int APPLET_DESTROY = 5;
    public final static int APPLET_QUIT = 6;
    public final static int APPLET_ERROR = 7;
    /* send to the parent to force relayout */
    public final static int APPLET_RESIZE = 51234;
    /* sent to a (distant) parent to indicate that the applet is being
     * loaded or as completed loading
     */
    public final static int APPLET_LOADING = 51235;
    public final static int APPLET_LOADING_COMPLETED = 51236;
    /**
     * The current status. One of:
     *    APPLET_DISPOSE,
     *    APPLET_LOAD,
     *    APPLET_INIT,
     *    APPLET_START,
     *    APPLET_STOP,
     *    APPLET_DESTROY,
     *    APPLET_ERROR.
     */
    protected int status;
    /**
     * The thread for the applet.
     */
    Thread handler;
    /**
     * The initial applet size.
     */
    Dimension defaultAppletSize = new Dimension(10, 10);
    /**
     * The current applet size.
     */
    Dimension currentAppletSize = new Dimension(10, 10);
    MessageUtils mu = new MessageUtils();
    /**
     * The thread to use during applet loading
     */

    Thread loaderThread = null;
    /**
     * Flag to indicate that a loading has been cancelled
     */
    boolean loadAbortRequest = false;
    /* abstract classes */
    abstract protected String getCode();

    abstract protected String getJarFiles();

    abstract protected String getSerializedObject();

    abstract public int    getWidth();

    abstract public int    getHeight();
    private static int threadGroupNumber = 0;
    /*
     * Creates a thread to run the applet. This method is called
     * the each time an applet is loaded and reloaded.
     */
    synchronized void createAppletThread() {
        // Create a thread group for the applet, and start a new
        // thread to load the applet.
        String nm = "applet-" + getCode();
        loader = getClassLoader(getCodeBase());
        loader.grab(); // Keep this puppy around!
        ThreadGroup appletGroup = loader.getThreadGroup();
        handler = new Thread(appletGroup, this, "thread " + nm);
        // set the context class loader for this thread
        AccessController.doPrivileged(new PrivilegedAction() {
                public Object run() {
                    handler.setContextClassLoader(loader);
                    return null;
                }
            }
        );
        handler.start();
    }

    void joinAppletThread() throws InterruptedException {
        if (handler != null) {
            handler.join();
            handler = null;
        }
    }

    void release() {
        if (loader != null) {
            loader.release();
            loader = null;
        }
    }

    /**
     * Construct an applet viewer and start the applet.
     */
    public void init() {
        try {
            // Get the width (if any)
            defaultAppletSize.width = getWidth();
            currentAppletSize.width = defaultAppletSize.width;
            // Get the height (if any)
            defaultAppletSize.height = getHeight();
            currentAppletSize.height = defaultAppletSize.height;
        } catch (NumberFormatException e) {
            // Turn on the error flag and let TagAppletPanel
            // do the right thing.
            status = APPLET_ERROR;
            showAppletStatus("badattribute.exception");
            showAppletLog("badattribute.exception");
            showAppletException(e);
        }
        setLayout(new BorderLayout());
        createAppletThread();
    }

    /**
     * Minimum size
     */
    public Dimension getMinimumSize() {
        return new Dimension(defaultAppletSize.width,
                defaultAppletSize.height);
    }

    /**
     * Preferred size
     */
    public Dimension getPreferredSize() {
        return new Dimension(currentAppletSize.width,
                currentAppletSize.height);
    }
    private AppletListener listeners;
    /**
     * AppletEvent Queue
     */
    private Queue queue = null;
    synchronized public void addAppletListener(AppletListener l) {
        listeners = AppletEventMulticaster.add(listeners, l);
    }

    synchronized public void removeAppletListener(AppletListener l) {
        listeners = AppletEventMulticaster.remove(listeners, l);
    }

    /**
     * Dispatch event to the listeners..
     */
    public void dispatchAppletEvent(int id, Object argument) {
        //System.out.println("SEND= " + id);
        if (listeners != null) {
            AppletEvent evt = new AppletEvent(this, id, argument);
            listeners.appletStateChanged(evt);
        }
    }

    /**
     * Send an event. Queue it for execution by the handler thread.
     */
    public void sendEvent(int id) {
        synchronized (this) {
            if (queue == null) {
                //System.out.println("SEND0= " + id);
                queue = new Queue();
            }
            Integer eventId = new Integer(id);
            queue.enqueue(eventId);
            notifyAll();
        }
        if (id == APPLET_QUIT) {
            try {
                joinAppletThread(); // Let the applet event handler exit
            } catch (InterruptedException e) {}
            // AppletClassLoader.release() must be called by a Thread
            // not within the applet's ThreadGroup
            if (loader == null)
                loader = getClassLoader(getCodeBase());
            release(); 
        }
    }

    /**
     * Get an event from the queue.
     */
    synchronized AppletEvent getNextEvent() throws InterruptedException {
        while (queue == null || queue.isEmpty()) {
            wait();
        }
        Integer eventId = (Integer) queue.dequeue();
        return new AppletEvent(this, eventId.intValue(), null);
    }

    boolean emptyEventQueue() {
        if ((queue == null) || (queue.isEmpty()))
            return true;
        else
            return false;
    }

    /**
     * Execute applet events.
     */
    public void run() {
        Thread curThread = Thread.currentThread();
        if (curThread == loaderThread) {
            // if we are in the loader thread, cause
            // loading to occur.  We may exit this with
            // status being APPLET_DISPOSE, APPLET_ERROR,
            // or APPLET_LOAD
            runLoader();
            return;
        }
        boolean disposed = false;
        while (!disposed && !curThread.isInterrupted()) {
            AppletEvent evt;
            try {
                evt = getNextEvent();
            } catch (InterruptedException e) {
                showAppletStatus("bail");
                return;
            }
            //showAppletStatus("EVENT = " + evt.getID());
            try {
                switch (evt.getID()) {
                case APPLET_LOAD:
                    if (!okToLoad()) {
                        break;
                    }
                    // This complexity allows laoding of applets to be
                    // interruptable.  The actual thread loading runs
                    // in a separate thread, so it can be interrupted
                    // without harming the applet thread.
                    // So that we don't have to worry about
                    // concurrency issues, the main applet thread waits
                    // until the loader thread terminates.
                    // (one way or another).
                    if (loaderThread == null) {
                        // NOTE: do we want a name?
                        //System.out.println("------------------- loading applet");
                        setLoaderThread(new Thread(this));
                        loaderThread.start();
                        // we get to go to sleep while this runs
                        loaderThread.join();
                        setLoaderThread(null);
                    } else {// NOTE: issue an error -- this case should never
                        // occur.
                    }
                    break;

                case APPLET_INIT:
                    if (status != APPLET_LOAD) {
                        showAppletStatus("notloaded");
                        break;
                    }
                    applet.resize(defaultAppletSize);
                    if (doInit) {
                        applet.init();
                    }
                    doInit = true;	// allow restarts
                    validate();
                    status = APPLET_INIT;
                    showAppletStatus("inited");
                    break;

                case APPLET_START:
                    /* 4721061
                     * check if applet already started
                     */
                    if (status == APPLET_START) {
                        // Do not attempt to start again
                        break;
                    }
                    if (status != APPLET_INIT) {
                        showAppletStatus("notinited");
                        break;
                    }
                    applet.resize(currentAppletSize);
                    applet.setVisible(true); 
                    status = APPLET_START;                   
                    applet.start();
                    validate();
                    /*
                     * Fix for BugTraq ID 4041703.
                     * Set the default focus for an applet.
                     */
                    setDefaultFocus();
                    showAppletStatus("started");
                    break;

                case APPLET_STOP:
                    if (status != APPLET_START) {
                        showAppletStatus("notstarted");
                        break;
                    }
                    status = APPLET_INIT;
                    applet.setVisible(false);
                    applet.stop();
                    showAppletStatus("stopped");
                    break;

                case APPLET_DESTROY:
                    if (status != APPLET_INIT) {
                        showAppletStatus("notstopped");
                        break;
                    }
                    status = APPLET_LOAD;
                    applet.destroy();
                    showAppletStatus("destroyed");
                    break;

                case APPLET_DISPOSE:
                    if (status != APPLET_LOAD) {
                        showAppletStatus("notdestroyed");
                        break;
                    }
                    status = APPLET_DISPOSE;
                    remove(applet);
                    applet = null;
                    showAppletStatus("disposed");
                    disposed = true;
                    break;

                case APPLET_QUIT:
                    return;
                }
            } catch (Exception e) {
                if (e.getMessage() != null) {
                    showAppletStatus("exception2", e.getClass().getName(),
                        e.getMessage());
                } else {
                    showAppletStatus("exception", e.getClass().getName());
                }
                showAppletException(e);
            } catch (ThreadDeath e) {
                showAppletStatus("death");
                return;
            } catch (Error e) {
                if (e.getMessage() != null) {
                    showAppletStatus("error2", e.getClass().getName(),
                        e.getMessage());
                } else {
                    showAppletStatus("error", e.getClass().getName());
                }
                showAppletException(e);
            }
            clearLoadAbortRequest();
        }
    }

    /*
     * Fix for BugTraq ID 4041703.
     * Set the focus to a reasonable default for an Applet.
     */
    private void setDefaultFocus() {
        if (getParent() instanceof Window) {
            Window window = (Window) getParent();
            // Don't proceed if the toplevel frame is inactive.
            if (window.getFocusOwner() != null) {
                doSetDefaultFocus();
            }
        }
    }

    private void doSetDefaultFocus() {
        if (applet == null) {
            return;
        }
        // Find first focus-traversable component.
        if (activateFocus((Container) applet)) {
            return;
        }
        // No focus-traversable component was found,
        // so set focus to the first leaf node.
        Component c = (Component) applet;
        while (c instanceof Container &&
            ((Container) c).getComponentCount() > 0) {
            Component child = ((Container) c).getComponent(0);
            if (child.isVisible() && child.isEnabled()) {
                c = child;
            } else {
                break;
            }
        }
        c.requestFocus();
    }

    private boolean activateFocus(Container cont) {
        int ncomponents = cont.getComponentCount();
        for (int i = 0; i < ncomponents; i++) {
            Component c = cont.getComponent(i);
            if (c.isVisible() && c.isEnabled() && c.isFocusTraversable()) {
                c.requestFocus();
                return true;
            }
            if (c instanceof Container && c.isVisible() && c.isEnabled()) {
                if (activateFocus((Container) c)) {
                    return true;
                }
            }
        }
        return false;
    }

    /* End of fix for BugTraq ID 4041703. */

    /**
     * Load the applet into memory.
     * Runs in a seperate (and interruptible) thread from the rest of the
     * applet event processing so that it can be gracefully interrupted from
     * things like HotJava.
     */
    private void runLoader() {
        if (status != APPLET_DISPOSE) {
            showAppletStatus("notdisposed");
            return;
        }
        dispatchAppletEvent(APPLET_LOADING, null);
        // TODO -- might be cool to visually indicate loading here --
        // maybe do animation?
        status = APPLET_LOAD;
        // Create a class loader
        loader = getClassLoader(getCodeBase());
        // Load the archives if present.
        // NOTE - this probably should be done in a separate thread,
        // or at least the additional archives (epll).

        String code = getCode();
        try {
            loadJarFiles(loader);
            applet = createApplet(loader);
        } catch (ClassNotFoundException e) {
            status = APPLET_ERROR;
            showAppletStatus("notfound", code);
            showAppletLog("notfound", code);
            showAppletException(e);
            return;
        } catch (InstantiationException e) {
            status = APPLET_ERROR;
            showAppletStatus("nocreate", code);
            showAppletLog("nocreate", code);
            showAppletException(e);
            return;
        } catch (IllegalAccessException e) {
            status = APPLET_ERROR;
            showAppletStatus("noconstruct", code);
            showAppletLog("noconstruct", code);
            showAppletException(e);
            // sbb -- I added a return here
            return;
        } catch (Exception e) {
            status = APPLET_ERROR;
            showAppletStatus("exception", e.getMessage());
            showAppletException(e);
            return;
        } catch (ThreadDeath e) {
            status = APPLET_ERROR;
            showAppletStatus("death");
            return;
        } catch (Error e) {
            status = APPLET_ERROR;
            showAppletStatus("error", e.getMessage());
            showAppletException(e);
            return;
        } finally {
            // notify that loading is no longer going on
            dispatchAppletEvent(APPLET_LOADING_COMPLETED, null);
        }
        // Stick it in the frame
        applet.setStub(this);
        applet.setVisible(false);
        add("Center", applet);
        showAppletStatus("loaded");
        validate();
    }

    protected Applet createApplet(final AppletClassLoader loader) throws ClassNotFoundException,
            IllegalAccessException, IOException, InstantiationException, InterruptedException {
        final String serName = getSerializedObject();
        String code = getCode();
        if (code != null && serName != null) {
            System.err.println(amh.getMessage("runloader.err"));
            return null;
        }
        if (code == null && serName == null) {
            String msg = "nocode";
            status = APPLET_ERROR;
            showAppletStatus(msg);
            showAppletLog(msg);
            repaint();
        }
        if (code != null) {
            applet = (Applet) loader.loadCode(code).newInstance();
            doInit = true;
        } else {
            // serName is not null;
            InputStream is = (InputStream)
                java.security.AccessController.doPrivileged(
                    new java.security.PrivilegedAction() {
                        public Object run() {
                            return loader.getResourceAsStream(serName);
                        }
                    }
                );
            ObjectInputStream ois =
                new AppletObjectInputStream(is, loader);
            Object serObject = ois.readObject();
            applet = (Applet) serObject;
            doInit = false; // skip over the first init
        }
        if (Thread.interrupted()) {
            try {
                status = APPLET_DISPOSE; // APPLET_ERROR?
                applet = null;
                // FIXME: This may not be exactly the right thing: the
                // status is set by the stop button and not necessarily
                // here.
                showAppletStatus("death");
            } finally {
                Thread.currentThread().interrupt(); // resignal interrupt
            }
            return null;
        }
        return applet;
    }

    protected void loadJarFiles(AppletClassLoader loader) throws IOException,
            InterruptedException {
        // Load the archives if present.
        // TODO - this probably should be done in a separate thread,
        // or at least the additional archives (epll).
        String jarFiles = getJarFiles();
        if (jarFiles != null) {
            StringTokenizer st = new StringTokenizer(jarFiles, ",", false);
            while (st.hasMoreTokens()) {
                String tok = st.nextToken().trim();
                try {
                    loader.addJar(tok);
                } catch (IllegalArgumentException e) {
                    // bad archive name
                    continue;
                }
            }
        }
    }

    /**
     * Request that the loading of the applet be stopped.
     */
    protected synchronized void stopLoading() {
        if (loaderThread != null) {
            //System.out.println("Interrupting applet loader thread: " + loaderThread);
            loaderThread.interrupt();
        } else {
            setLoadAbortRequest();
        }
    }

    protected synchronized boolean okToLoad() {
        return !loadAbortRequest;
    }

    protected synchronized void clearLoadAbortRequest() {
        loadAbortRequest = false;
    }

    protected synchronized void setLoadAbortRequest() {
        loadAbortRequest = true;
    }

    private synchronized void setLoaderThread(Thread loaderThread) {
        this.loaderThread = loaderThread;
    }

    /**
     * Return true when the applet has been started.
     */
    public boolean isActive() {
        return status == APPLET_START;
    }

    /**
     * Is called when the applet wants to be resized.
     */
    public void appletResize(int width, int height) {
        currentAppletSize.width = width;
        currentAppletSize.height = height;
        Dimension currentSize = new Dimension(currentAppletSize.width,
                currentAppletSize.height);
        dispatchAppletEvent(APPLET_RESIZE, currentSize);
    }

    public void setBounds(int x, int y, int width, int height) {
        super.setBounds(x, y, width, height);
        currentAppletSize.width = width;
        currentAppletSize.height = height;
    }

    public Applet getApplet() {
        return applet;
    }

    /**
     * Status line. Called by the AppletPanel to provide
     * feedback on the Applet's state.
     */
    protected void showAppletStatus(String status) {
        getAppletContext().showStatus(amh.getMessage(status));
    }

    protected void showAppletStatus(String status, Object arg) {
        getAppletContext().showStatus(amh.getMessage(status, arg));
    }

    protected void showAppletStatus(String status, Object arg1, Object arg2) {
        getAppletContext().showStatus(amh.getMessage(status, arg1, arg2));
    }

    /**
     * Called by the AppletPanel to print to the log.
     */
    protected void showAppletLog(String msg) {
        System.out.println(amh.getMessage(msg));
    }

    protected void showAppletLog(String msg, Object arg) {
        System.out.println(amh.getMessage(msg, arg));
    }

    /**
     * Called by the AppletPanel to provide
     * feedback when an exception has happened.
     */
    protected void showAppletException(Throwable t) {
        t.printStackTrace();
        repaint();
    }
    /**
     * The class loaders
     */
    private static Hashtable classloaders = new Hashtable();
    /**
     * Flush a class loader.
     */
    public static synchronized void flushClassLoader(URL codebase) {
        classloaders.remove(codebase);
    }

    /**
     * Flush all class loaders.
     */
    public static synchronized void flushClassLoaders() {
        classloaders = new Hashtable();
    }

    /**
     * This method actually creates an AppletClassLoader.
     *
     * It can be override by subclasses (such as the Plug-in)
     * to provide different classloaders.
     */
    protected AppletClassLoader createClassLoader(final URL codebase) {
        return new AppletClassLoader(codebase);
    }

    /**
     * Get a class loader. Create in a restricted context
     */
    synchronized AppletClassLoader getClassLoader(final URL codebase) {
        AppletClassLoader c = (AppletClassLoader) classloaders.get(codebase);
        if (c == null) {
            AccessControlContext acc =
                getAccessControlContext(codebase);
            c = (AppletClassLoader)
                    AccessController.doPrivileged(new PrivilegedAction() {
                            public Object run() {
                                AppletClassLoader ac = createClassLoader(codebase);
                                /* Should the creation of the classloader be
                                 * within the class synchronized block?  Since
                                 * this class is used by the plugin, take care
                                 * to avoid deadlocks, or specialize
                                 * AppletPanel within the plugin.  It may take
                                 * an arbitrary amount of time to create a
                                 * class loader (involving getting Jar files
                                 * etc.) and may block unrelated applets from
                                 * finishing createAppletThread (due to the
                                 * class synchronization). If
                                 * createAppletThread does not finish quickly,
                                 * the applet cannot process other messages,
                                 * particularly messages such as destroy
                                 * (which timeout when called from the browser).
                                 */
                                synchronized (getClass()) {
                                    AppletClassLoader res =
                                        (AppletClassLoader) classloaders.get(codebase);
                                    if (res == null) {
                                        classloaders.put(codebase, ac);
                                        return ac;
                                    } else {
                                        return res;
                                    }
                                }
                            }
                        }, acc);
        }
        return c;
    }

    /**
     * get the context for the AppletClassLoader we are creating.
     * the context is granted permission to create the class loader,
     * connnect to the codebase, and whatever else the policy grants
     * to all codebases.
     */
    private AccessControlContext getAccessControlContext(final URL codebase) {
        PermissionCollection perms = (PermissionCollection)
            AccessController.doPrivileged(new PrivilegedAction() {
                    public Object run() {
                        Policy p = java.security.Policy.getPolicy();
                        if (p != null) {
                            return p.getPermissions(new CodeSource(null, null));
                        } else {
                            return null;
                        }
                    }
                }
            );
        if (perms == null)
            perms = new Permissions();
        //NOTE: this is needed to be able to create the classloader itself!

        perms.add(new RuntimePermission("createClassLoader"));
        Permission p;
        java.net.URLConnection urlConnection = null;
        try {
            urlConnection = codebase.openConnection();
            p = urlConnection.getPermission();
        } catch (java.io.IOException ioe) {
            p = null;
        }

        if (p != null)
            perms.add(p);

        if (p instanceof FilePermission) {
            String path = p.getName();
            int endIndex = path.lastIndexOf(File.separatorChar);

            if (endIndex != -1) {
                path = path.substring(0, endIndex + 1);
                if (path.endsWith(File.separator)) {
                    path += "-";
                }
                perms.add(new FilePermission(path, "read"));
            }
        } else {
            URL locUrl = codebase;
            if (urlConnection instanceof JarURLConnection) {
                locUrl = ((JarURLConnection) urlConnection).getJarFileURL();
            }
            String host = locUrl.getHost();
            if (host != null && (host.length() > 0))
                perms.add(new SocketPermission(host, "connect, accept"));
        }
        ProtectionDomain domain =
            new ProtectionDomain(new CodeSource(codebase, null), perms);
        AccessControlContext acc =
            new AccessControlContext(new ProtectionDomain[] { domain }
            );
        return acc;
    }

    public Thread getAppletHandlerThread() {
        return handler;
    }

    public int getAppletWidth() {
        return currentAppletSize.width;
    }

    public int getAppletHeight() {
        return currentAppletSize.height;
    }
    private static AppletMessageHandler amh = new AppletMessageHandler("appletpanel");
}
