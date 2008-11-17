/*
 * @(#)PlaneServer.java	1.3 03/01/16
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


package IXCDemo.ixcXlets.serverXlet;

import javax.microedition.xlet.Xlet;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcRegistry;
import java.rmi.AlreadyBoundException;
import java.rmi.RemoteException;
import java.rmi.AccessException;

public class PlaneServer implements Xlet, Runnable {
    private XletContext context;
    private boolean running = false;
    private boolean pleaseStop = false;
    private boolean started = false;
    private PlaneImpl planeImpl = new PlaneImpl();
    public PlaneServer() {
        System.out.println("server:  constructed");
    }
    
    public void initXlet(XletContext context) {
        this.context = context;
    }
    
    public void startXlet() {
        System.out.println("PlaneServer was started.");
        boolean wasStarted;
        synchronized (this) {
            pleaseStop = false;
            if (running) {
                return;
            }
            running = true;
            wasStarted = started;
            if (!started) {
                started = true;
            }
        }
        Thread t = new Thread(this);
        t.setDaemon(true);
        t.start();
        if (!wasStarted) {
            try {
                IxcRegistry r = IxcRegistry.getRegistry(context);
                r.bind("", planeImpl);
                r.bind("other", new PlaneImpl());
            } catch (AlreadyBoundException ex) {
                System.out.println("Server was already registered.");
                ex.printStackTrace();
                context.notifyDestroyed();
                return;
            } catch (RemoteException re) {
                System.out.println("Server cannot contact Registry");
                re.printStackTrace();
                context.notifyDestroyed();
                return;
            }
        }
    }
    
    public void pauseXlet() {
        stopMe();
    }
    
    public void destroyXlet(boolean unconditional) {
        stopMe();
    }
    
    private void stopMe() {
        synchronized (this) {
            pleaseStop = true;
            notifyAll();
        }
    }
    
    public void run() {
        for (;;) {
            synchronized (this) {
                if (pleaseStop) {
                    running = false;
                    return;
                }
                try {
                    wait(2000);
                } catch (InterruptedException ex) {
                    Thread.currentThread().interrupt();
                    return;	// We're being killed; bail out
                }
            }
            if (!pleaseStop) {
                planeImpl.tick();
            }
        }
    }
}
