/*
 * @(#)PlaneClient.java	1.4 03/01/16
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

package IXCDemo.ixcXlets.clientXlet;

import javax.microedition.xlet.Xlet;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcRegistry;
import java.rmi.NotBoundException;
import java.rmi.RemoteException;
import IXCDemo.shared.*;

public class PlaneClient implements Xlet, PlaneListener {
    private XletContext context;
    private boolean started = false;
    private Plane plane;
    public PlaneClient() {
        System.out.println("client:  constructed");
    }
    
    public void initXlet(XletContext context) {
        this.context = context;
    }
    
    public void startXlet() {
        System.out.println("PlaneClient starting...");
        synchronized (this) {
            if (started) {
                return;
            }
            started = true;
        }
        try {
            System.out.println("PlaneClient looking for import registry.");
            IxcRegistry r = IxcRegistry.getRegistry(context);
            System.out.println("PlaneClient waiting for the server for 2 second");
            Thread.sleep(2000);
            System.out.println("PlaneClient looking for plane.");
            plane = (Plane) r.lookup("");
            System.out.println("PlaneClient adding self as listener.");
            plane.addListener(this);
            Plane other = (Plane) r.lookup("other");
            System.out.println("Other plane:  " + other);
            System.out.println("Other equals primary?  " + other.equals(plane));
            System.out.println("PlaneClient setting speed and heading");
            plane.accelerate((float) 2.7);
            plane.turn(Math.PI / 3.0);
            System.out.println("Other equals primary?  " + other.equals(plane));
        } catch (InterruptedException e) {
            System.out.println("PlaneClient interruped");
            context.resumeRequest();
        } catch (NotBoundException ex) {
            System.out.println("Server object not found");
            ex.printStackTrace();
            context.notifyDestroyed();
            return;
        } catch (RemoteException ex) {
            System.out.println("Client sees unexpected remote exception");
            ex.printStackTrace();
            context.notifyDestroyed();
            return;
        }
    }
    
    public void pauseXlet() {}
    
    public void destroyXlet(boolean unconditional) {
        try {
            plane.removeListener(this);
        } catch (RemoteException ex) {
            ex.printStackTrace();	
        }
    }
    
    /* =============  Method from PlaneListener  ================== */
    
    public void positionChanged(Position newPos) {
        System.out.println("Client sees new position:  " + newPos);
    }
}
