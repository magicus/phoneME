/*
 *  
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
 */

package com.sun.midp.lcdui;

import java.util.Vector;

import javax.microedition.lcdui.Display;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;

/**
 * Stores array of active displays that either belong to MIdlets, 
 * or dynamically created for display preemption.
 */
public class DisplayContainer {
    
    /** Sets the display ID in a new display. */
    private DisplayIdPolicy displayIdPolicy;

    /** Active displays. */
    private Vector displays = new Vector(5, 5);

    /** 
     * Default constructor.
     *
     * @param token security token for initilaization
     * @param idPolicy policy to set the display ID of a new display
     */ 
    public DisplayContainer(SecurityToken token, DisplayIdPolicy idPolicy) {
        token.checkIfPermissionAllowed(Permissions.MIDP);
        displayIdPolicy = idPolicy;
    }

    /**
     * Adds a display object to the container and sets a the display's ID to
     * new unique value for this isolate, as a single atomic operation.
     * <p>
     * Intended to be called from Display constructor.
     *
     * @param da display object to add
     */
    public synchronized void addDisplay(DisplayAccess da) {
        if (displays.indexOf(da) == -1) {
            displayIdPolicy.createDisplayId(da, this);
            displays.addElement(da);
        }
    }
    
    /**
     * Get a display to request the foreground on behalf of the MIDlet.
     *
     * @param owner the object that owns this display
     */
    public void requestForegroundForDisplay(Object owner) {
        DisplayAccess[] da = findDisplaysByOwner(owner, 0);

    /** IMPL_NOTE: correct call ! */
        if (da != null) {
            da[0].requestForeground();
        }
    }

    /**
     * Removes display objects by the owner name from the container.
     *
     * @param owner the MIDlet that owns this display
     *
     * @return true if display has been succcessfully removed, 
     *         false, if display object has not been found in the container.
     */
    public synchronized boolean removeDisplaysByOwner(Object owner) {
        int size = displays.size();

        for (int i = size; --i >= 0;) {
            DisplayAccess current = (DisplayAccess)displays.elementAt(i);
        
            if (current.getOwner() == owner) {
                displays.removeElementAt(i);
            }
        }
        return (displays.size() < size);
    }

    /**
     * Removes display object from the container by Id.
     *
     * @param displayId ID of the display
     *
     * @return true if display has been succcessfully removed, 
     *         false, if display object has not been found in the container.
     */
    public synchronized boolean removeDisplayById(int displayId) {
        DisplayAccess da = findDisplayById(displayId);
        if (da != null) {
            return displays.removeElement(da);
        }
        return false;
    }
    
    /**
     * Removes display object from the container.
     *
     * @param da display access of the display
     *
     * @return true if display has been succcessfully removed, 
     *         false, if display object has not been found in the container.
     */
    public synchronized boolean removeDisplay(DisplayAccess da) {
        return displays.removeElement(da);
    }
    
    /**
     * Find a display by ID.
     *
     * @param displayId ID of the display
     *
     * @return a display access object or null if not found
     */
    public synchronized DisplayAccess findDisplayById(int displayId) {
        int size = displays.size();
        
        for (int i = 0; i < size; i++) {
            DisplayAccess current = (DisplayAccess)displays.elementAt(i);

            if (current.getDisplayId() == displayId) {
                return current;
            }
        }

        return null;
    }
    
    /**
     * Find the foreground display.
     *
     * @return a display access object or null if not found
     */
    public synchronized DisplayAccess findForegroundDisplay() {
        int size = displays.size();
        
        for (int i = 0; i < size; i++) {
            DisplayAccess current = (DisplayAccess)displays.elementAt(i);

            if (current.isDisplayForeground()) {
                return current;
            }
        }

        return null;
    }

    /**
     * Find a display's access object.
     *
     * @param display an LCDUI display
     *
     * @return a display access object or null if not found
     */
    public synchronized DisplayAccess findDisplayAccess(Display display) {
        int size = displays.size();
        
        for (int i = 0; i < size; i++) {
            DisplayAccess current = (DisplayAccess)displays.elementAt(i);

            if (current.getDisplay() == display) {
                return current;
            }
        }

        return null;
    }

    /**
     * Find the displays by owner.
     *
     * @param owner the object that owns the display
     *
     * @return array of display access objects or null if not found
     */
    public synchronized DisplayAccess[] findDisplaysByOwner(Object owner) {
        int size = displays.size();
        Vector v = new Vector(2, 2); 
    

        for (int i = 0; i < size; i++) {
            DisplayAccess current = (DisplayAccess)displays.elementAt(i);
        
            if (current.getOwner() == owner) {
                v.addElement(current);
            }
        }
    
    DisplayAccess[] ret = null;
    if (v.size() > 0) {
        ret = new DisplayAccess[v.size()];
        v.copyInto(ret);
    }
    
    return ret;
    }

    /**
     * Find the displays by owner.
     *
     * @param owner the object that owns the display
     * @param capabilities display device capbilities filter
     *
     * @return array of display access objects or null if not found
     */
    public synchronized DisplayAccess[] findDisplaysByOwner(Object owner, int capabilities) {
        int size = displays.size();
        Vector v = new Vector(2, 2); 
    

        for (int i = 0; i < size; i++) {
            DisplayAccess current = (DisplayAccess)displays.elementAt(i);
        
            if ((current.getOwner() == owner) && 
                (current.getDisplayDevice().getCapabilities() & 
                capabilities) == capabilities) {
                v.addElement(current);
            }
        }
    
        DisplayAccess[] ret = null;
        if (v.size() > 0) {
            ret = new DisplayAccess[v.size()];
            v.copyInto(ret);
        }
    
        return ret;
    }

    /**     
     * Find the displays by hardwareId.
     *
     * @return array of display access objects or null if not found
     */
    public synchronized DisplayAccess[] findDisplaysByHardwareId(int hardwareId) {
        int size = displays.size();
        Vector v = new Vector(2, 2); 

        for (int i = 0; i < size; i++) {
            DisplayAccess current = (DisplayAccess)displays.elementAt(i);
        
            if (current.getDisplayDevice().getHardwareId() == hardwareId) {
                v.addElement(current);
            }
        }
    
        DisplayAccess[] ret = null;
        if (v.size() > 0) {
            ret = new DisplayAccess[v.size()];
            v.copyInto(ret);
        }
    
        return ret;
    }

    /**
     * Find a primary display by owner.
     *
     * @param owner class of the MIDlet that owns this display
     *
     * @return a display access object or null if not found
     */
    public synchronized DisplayAccess findPrimaryDisplayByOwner(Object owner) {
        int size = displays.size();
        DisplayAccess d = null;

        for (int i = 0; i < size; i++) {
            DisplayAccess current = (DisplayAccess)displays.elementAt(i);
        
            if ((current.getOwner() == owner) && 
                current.getDisplayDevice().isPrimaryDisplay()) {
                d = current;
                break;
            }
        }
    
        return d;
    }
    /**
     * Find a display event consumer by ID.
     *
     * @param displayId ID of the display
     *
     * @return a display event consumer object or null if not found
     */
    public DisplayEventConsumer findDisplayEventConsumer(int displayId) {
        DisplayAccess da = findDisplayById(displayId);

        if (da == null) {
            return null;
        }
        
        return da.getDisplayEventConsumer();
    }
    
        /**
     * Find all display event consumers.
     *
     * @return a display event consumer array object
     */
    public DisplayEventConsumer[] getAllDisplayEventConsumers() {
                
        int size = displays.size();
        DisplayEventConsumer[] consumers = new DisplayEventConsumer[size];
        DisplayAccess da;
        
        for (int i = 0; i < size; i++) {
            da = (DisplayAccess)displays.elementAt(i);
            
            if (da != null) {
                consumers[i] = da.getDisplayEventConsumer();
            }
        }
       
        return consumers;
    }


    /**
     * Find a foreground event consumer by ID.
     *
     * @param displayId ID of the display
     *
     * @return a foreground event consumer object or null if not found
     */
    public ForegroundEventConsumer findForegroundEventConsumer(int displayId) {
        DisplayAccess da = findDisplayById(displayId);

        if (da == null) {
            return null;
        }

        return da.getForegroundEventConsumer();
    }
}
