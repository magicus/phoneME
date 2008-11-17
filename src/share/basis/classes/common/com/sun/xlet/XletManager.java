/*
 * @(#)XletManager.java	1.21 06/10/10
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

package com.sun.xlet;

import javax.microedition.xlet.*;
import javax.microedition.xlet.ixc.*;
import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.FlowLayout;
import java.awt.Frame;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
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
import java.util.Hashtable;
import java.util.Vector;

import sun.awt.SunToolkit;
import sun.awt.AppContext;

public class XletManager implements XletLifecycleHandler {

    // the root frame shared by all xlets
    static ToplevelFrame theFrame; 
    // list of active <XletContext,XletManager> pairs 
    static Hashtable activeContexts = new Hashtable(); 

    // the xlet class this XletManager instance handles
    private Class xletClass;    
    // this XletManager's state queue
    private XletStateQueue xletQueue; 

    // The instance of the xletClass above.  
    // Both Class and the instance are kept
    // as xlet instantiation calls into the user code,
    // thus should be handled in the XletStateQueue thread,
    // and this thread only knows about local data right now.
    private Xlet xlet;  

    // this Xlet's XletContext
    private XletContextImpl context; 
    // Xlet's current state
    private XletState currentState = XletState.UNLOADED;

    // this Xlet's threadGroup and AppContext
    ThreadGroup threadGroup;  
    AppContext appContext;

    private Object stateGuard = new Object();

    protected XletManager(XletClassLoader xletClassLoader,
                 String mainClass,
                 String[] args) throws ClassNotFoundException {

       xletClass = xletClassLoader.loadClass(mainClass);
         
       if (!(Xlet.class).isAssignableFrom(xletClass)) {
          throw new ClassCastException(
             "Attempt to run a non-Xlet class: " + mainClass);
       }

       context = new XletContextImpl(mainClass, args, this);
       activeContexts.put(context, this);

       final ThreadGroup currentTG = Thread.currentThread().getThreadGroup();
       final int numOfXlets = activeContexts.size();
       final String className = mainClass;
       final ClassLoader finalLoader = xletClassLoader;

       threadGroup = (ThreadGroup) AccessController.doPrivileged(
          new PrivilegedAction() {
             public Object run() {
                return  new ThreadGroup(currentTG, 
                                       "Xlet Thread Group " + numOfXlets);
             }   
          }      
       );
      
       // Create an Xlet state change dispatcher queue
       final XletManager thisXletManager = this;
       xletQueue = (XletStateQueue) AccessController.doPrivileged(
          new PrivilegedAction() {
             public Object run() {
                Thread t = new Thread(threadGroup,
                           new Runnable() {
                              public void run() {
                                 manageLifecycle();
                              }
                           }, "Xlet lifecycle for " + className);
                t.setContextClassLoader(finalLoader);
                t.start();
                return new XletStateQueue(thisXletManager);
             }   
          }      
       );

       // Let the ClassLoader have the reference to this XletManager
       xletClassLoader.setXletManager(this);
       // Register an AppContext with this ThreadGroup.
       // Note: Create AppContext after registering XletManager to XletClassLoader,
       //       as AppContext creation leads to EventQueue dispatcher thread
       //       creation.  ThreadGroup for the new Thread is provided by
       //       the SecurityManager if installed, and XletSecurity needs
       //       XletClassLoader to have a reference of xletManager to get TG.
       createAppContext(threadGroup);

       if (theFrame == null) { // Grab the Frame before xlet gets it's chance.
          theFrame = new ToplevelFrame("Xlet Frame", this); 
       }

       // Install a SecurityManager on this xlet if nothing is set.
       // Note: If this is launching a second xlet, all code above is already
       //       executing with XletSecurity installed.
       installSecurityManager();

       // Finally, request the xlet to be instantiated.
       xletQueue.push(XletState.LOADED);
    }

    /**
     * Call SunToolkit to create a dedicated EventQueue
     * for the caller Xlet.  Guarantees that AppContext 
     * is created by the the the method returns.
     *
     * @param ThreadGroup which the xlet's lifecycle thread
     *        is running within
     */
    final Object syncObject = new Object();
    private void createAppContext(ThreadGroup tg) {
       final ThreadGroup finalTG = tg;
       final ClassLoader finalLoader = getClassLoader();
       AccessController.doPrivileged( 
          new PrivilegedAction() {
             public Object run() {
                Thread t2 = new Thread(finalTG,
                   new Runnable() {
                       public void run() {
                          synchronized (syncObject) {
                              appContext = SunToolkit.createNewAppContext();
                              syncObject.notifyAll();
                          }
                       }
                   }, "AppContext creation thread");
                // Need to set ContextClassLoader, as this thread 
                // is later used as the EventQueue's dispatching thread
                t2.setContextClassLoader(finalLoader);
                synchronized (syncObject) {
                   t2.start();
                   try {
                      syncObject.wait();
                   } catch (InterruptedException e) {}
                }
                return null;
             }
          }
       );
       return;
    }

    /*
     * Install XletSecurity if no SecurityManager is set.
     */
    private void installSecurityManager() {
        if (System.getSecurityManager() == null) {
           System.setSecurityManager(new XletSecurity(appContext));
        }
    }

    public Container getContainer() {
        return theFrame.getXletContainer();
    }

    public void postInitXlet() {
        xletQueue.push(DesiredXletState.INITIALIZE);
    }

    public void postStartXlet() {
        xletQueue.push(XletState.ACTIVE);
    }

    public void postPauseXlet() {
        xletQueue.push(XletState.PAUSED);
    }
 
    public void postDestroyXlet(boolean unconditional) {
        if (!unconditional) {
            xletQueue.push(DesiredXletState.CONDITIONAL_DESTROY);
        } else {
            xletQueue.push(XletState.DESTROYED);
        }
    }

    public void setState(XletState state) { 
        
        if (currentState == XletState.DESTROYED || 
            state == currentState) 
             return;

        synchronized (stateGuard) {
            currentState = state;
            stateGuard.notifyAll();
        }
    }

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

    public XletState getXletState() { 
        synchronized (stateGuard) {
            return currentState;
        }
    }

    // typically called from XletStateQueue
    public void handleRequest(XletState desiredState) {
        XletState targetState = currentState;
        synchronized (stateGuard) {
           try { 
                if (desiredState == XletState.LOADED) {
                    if (currentState != XletState.UNLOADED) 
                       return;
                    targetState = XletState.LOADED;
                    Constructor m = xletClass.getConstructor(new Class[0]);
                    xlet = (Xlet) m.newInstance(new Object[0]);
                } else if (desiredState == DesiredXletState.INITIALIZE) {
                    if (currentState != XletState.LOADED) 
                        return;
                    targetState = XletState.PAUSED;
                    try {
                        xlet.initXlet(context);
                    } catch (XletStateChangeException xsce) {
                        targetState = XletState.DESTROYED;
                        xlet.destroyXlet(true);
                    }
                } else if (desiredState == XletState.ACTIVE) {
                    if (currentState != XletState.PAUSED) 
                        return;
                    targetState = XletState.ACTIVE;
                    try {
                        xlet.startXlet();
                    } catch (XletStateChangeException xsce) {
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
                        targetState = currentState;
                    } 
                } else if (desiredState == XletState.DESTROYED) {
                    targetState = XletState.DESTROYED;
                    if (currentState == XletState.DESTROYED) 
                        return;
                    try {
                        xlet.destroyXlet(true);
                    } catch (XletStateChangeException xsce) {}
                } 

           } catch (Exception e) {
               // Some unknown exception from ths user code.
               // Destroy it...
               //System.err.println("Exception during execution: " + e);
               e.printStackTrace();
               if (targetState != XletState.DESTROYED) {
                   handleRequest(XletState.DESTROYED);
               }
           }

           // State change done - update this xlet's State.
           setState(targetState);
        }
    }

    // really private, but invoked from inner class.
    void manageLifecycle() {
        try {
            while (currentState != XletState.DESTROYED) {
                synchronized (stateGuard) {
                    stateGuard.wait();  
                }
            }
            // xlet has destroyed, do appropreate cleanup 
            cleanUp();
        } catch (Throwable t) {
            System.out.println(
                "Xlet had unexpected exception in lifecycle thread.");
            t.printStackTrace();
            setState(XletState.DESTROYED);
        }

        // 4739427. If no other xlet running, then shut down.
        synchronized (activeContexts) {
           if (activeContexts.isEmpty()) {
              exit();
           }
        }
    }

    private void cleanUp() {
        // Clean up IxcRegistry
        IxcRegistry.getRegistry(context).unbindAll();

        // remove the Root Container from the parent frame
        if (context.container != null) {
            theFrame.remove(context.container);
            context.container = null;
            theFrame.repaint();
        }
        // clear the State Queue
        xletQueue.destroy();
        // remove the xlet context from the hash
        synchronized (activeContexts) {
            activeContexts.remove(context);
            activeContexts.notifyAll();
        }
    }

    static void exit() {
        synchronized (activeContexts) {
            XletManager[] managers = 
                (XletManager[]) activeContexts.values().toArray(
                    new XletManager[activeContexts.size()]); 
            for (int i = 0; i < managers.length; i++) {
                try {
                    // request for a DESTROY state to be picked up at the XletStateQueue
                    managers[i].postDestroyXlet(false);
                    activeContexts.wait(1000);  // wait for 1 sec for each..
                    if (activeContexts.containsValue(managers[i])) {
                        // try to interrupt anything that the XletStateQueue is doing
                        managers[i].xletQueue.queueThread.interrupt(); 
                        // force the state to be DESTROYED
                        managers[i].setState(XletState.DESTROYED);
                        activeContexts.wait(500);  // wait for .5 sec more...
                    }
                } catch (InterruptedException ie) {
                    managers[i].setState(XletState.DESTROYED);
                }
            }
        }
        System.exit(0); 
    } 

    /**  
     *  Look up the XletContext-Xlet hashtable (activeContexts) and 
     *  return the classloader for the xlet based on the XletContext passed in.
     *  For XletContext.getClassLoader() impl. 
    **/
    static ClassLoader getXletClassLoader(XletContext context) {
       XletManager thisManager = (XletManager)activeContexts.get(context);
       if (thisManager != null) {
           return thisManager.getClassLoader();
       }
       return null;  // no matching xlet found.
    }

    // Returns the ClassLoader for this xlet 
    public ClassLoader getClassLoader() {
       return xletClass.getClassLoader();
    }

    /** 
     * Entry point for XletManager.
     * Instanciates the xlet class in the current dir.
     * Returns the handler to control this xlet's lifecycle.
     * @param mainClass the xlet class name.
    **/
    public static XletLifecycleHandler createXlet(String mainClass)
        throws IOException {
        return createXlet(mainClass, new String[] {"."}, new String[]{});
    }

    /** 
     * Entry point for XletManager.
     * Returns the handler to control this xlet's lifecycle.
     * @param mainClass the xlet class name.
     * @param path array of url-formed strings or file paths to find
     *             the xlet class.
     * @param args the runtime arguments given to the xlet
     *             (accessed by XletContext.getXletProperty(ARGS))
    **/
    public static XletLifecycleHandler createXlet(String mainClass,
        String[] paths,
        String[] args)
        throws IOException {
        Vector v = new Vector();
        int index = 0;
        for (int i = 0; i < paths.length; i++) {
           try {
              v.add(new URL(paths[i]));
           } catch (MalformedURLException mue) {
              final File file = new File(paths[i]);
              try {
                 URL path = (URL) AccessController.doPrivileged(
                    new PrivilegedExceptionAction() {
                       public Object run() throws IOException {
                          if (!file.exists()) {
                             System.out.println("Warning: \"" + file.getPath() + "\" not found");
                             return null;
                          }
                          // CR 6252530.
                          return file.toURI().toURL();
                       }   
                    }   
                 );
              
                 if (path != null)  
                    v.add(path);

              } catch (PrivilegedActionException e) { e.getException().printStackTrace(); }
           }
        }
        final URL[] array = (URL[])v.toArray(new URL[v.size()]);
        XletClassLoader cl = (XletClassLoader) AccessController.doPrivileged(
           new PrivilegedAction() {
              public Object run() {
                 return new XletClassLoader(array);
              }   
           }
        );
                                                                                
        try {
            return (XletLifecycleHandler) new XletManager(cl, mainClass, args);
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
            throw new IOException("Cannot find class " + mainClass);
        }
    }
 
    public static int getState(XletContext context) {
       XletManager thisManager = (XletManager)activeContexts.get(context);
       if (thisManager != null)
          return thisManager.getState();
       return 0;
    }
}
