/*
 * @(#)PlaneImpl.java	1.3 03/01/16
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

import java.util.Vector;
import javax.microedition.xlet.Xlet;
import javax.microedition.xlet.XletContext;
import java.rmi.RemoteException;
import IXCDemo.shared.*;

public class PlaneImpl implements Plane {
    private float speed;
    private double heading;
    private Position position = new Position(0, 0);
    private Vector listeners = new Vector();
    private long lastTime;
    public float getSpeed() {
        return speed;
    }
    
    public double getHeading() {
        return heading;
    }
    
    public Position getPosition() {
        return position;
    }
    
    public void accelerate(float deltaV) {
        System.out.println("PlaneServer accelerates by " + deltaV);
        speed += deltaV;
        updatePosition();
    }
    
    public void turn(double delta) {
        System.out.println("PlaneServer turns by " + delta);
        heading += delta;
        updatePosition();
    }
    
    public void addListener(PlaneListener l) {
        System.out.println("PlaneImpl gets new listener " + l);
        listeners.addElement(l);
    }
    
    public void removeListener(PlaneListener l) {
        listeners.removeElement(l);
    }
    
    void tick() {
        updatePosition();
        notifyListeners();
    }
    
    private void updatePosition() {
        synchronized (this) {
            long next = System.currentTimeMillis();
            long deltaT = next - lastTime;
            float x = position.getX() + (float) (speed * Math.sin(heading));
            float y = position.getY() + (float) (speed * Math.cos(heading));
            position = new Position(x, y);
            lastTime = next;
        }
    }
    
    private void notifyListeners() {
        PlaneListener[] l;
        synchronized (listeners) {
            l = new PlaneListener[listeners.size()];
            for (int i = 0; i < l.length; i++) {
                l[i] = (PlaneListener) listeners.elementAt(i);
            }
        }
        for (int i = 0; i < l.length; i++) {
            try {
                System.out.println("PlaneImpl notifing listener " + l[i]);
                l[i].positionChanged(position);
            } catch (RemoteException ex) {
                ex.printStackTrace();
                System.out.println("Remote exception, will remove listener...");
                try {
                    Thread.sleep(2000);
                } catch (InterruptedException iex) {
                    Thread.currentThread().interrupt();
                }
                System.out.println("Removing listener.");
                removeListener(l[i]);
            }
        }
    }
    
    public int hashCode() {
        long headingBits = Double.doubleToLongBits(heading);
        return position.hashCode()
            ^ Float.floatToIntBits(speed)
            ^ ((int) (headingBits & (headingBits >>> 32)));
    }
    
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        } else if (!(other instanceof PlaneImpl)) {
            return false;
        } else {
            PlaneImpl po = (PlaneImpl) other;
            return speed == po.speed && heading == po.heading
                && position.equals(po.position);
        }
    }
    
    public String toString() {
        return getClass().getName() + "<speed=" + speed + ", heading=" + heading
            + ", position=" + position + ">";
    }
}
