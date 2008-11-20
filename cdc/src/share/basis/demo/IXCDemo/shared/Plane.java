/*
 * @(#)Plane.java	1.3 03/01/16
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


/*
 * Representation of an airplane with a position, heading and speed.
 */

package IXCDemo.shared;

public interface Plane extends java.rmi.Remote {
    public float getSpeed()
        throws java.rmi.RemoteException;
    /*
     * Get the heading, in degrees
     */
    public double getHeading()
        throws java.rmi.RemoteException;
    public Position getPosition()
        throws java.rmi.RemoteException;
    public void accelerate(float deltaV)
        throws java.rmi.RemoteException;
    public void turn(double delta)
        throws java.rmi.RemoteException;
    public void addListener(PlaneListener l)
        throws java.rmi.RemoteException;
    public void removeListener(PlaneListener l)
        throws java.rmi.RemoteException;
}
