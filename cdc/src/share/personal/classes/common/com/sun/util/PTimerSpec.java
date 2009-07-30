/*
 * @(#)PTimerSpec.java	1.14 06/10/10
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

package com.sun.util;

import java.util.*;

/**
 * <p>A class representing a timer specification.  A timer
 * specification declares when a PTimerWentOffEvent should be sent.  These
 * events are sent to the listeners registered on the
 * specification.</p>
 *
 * <li>Absolute
 * If the PTimerSpec is Absolute, then events will be dispatched at a
 * specified time. Otherwise the time value is  relative to the time
 * at which the event is scheduled.
 * <li>Repeat
 * If the PTimerSpec is repeating, events will be dispatched at
 * intervals specified by the timevalue. It is meaningless to
 * specify an Absolute event to repeat.
 * <li>Regular
 * If a PTimerSpec is Regular, the PTimer will attempt to notify
 * its listener at the specified interval regardless  of how much
 * time the event processing takes. Otherwise,  the specified time
 * is the amount of time after all listeners have been called.
 *
 * <h3>Compatibility</h3>
 * The PTimer and PTimerSpec classes are part of the Personal Java
 * Specification and  are not  part of J2SE. They are deprecated in
 * the Personal Profile as they have been replaced by the
 * <code>java.util.timer</code> class.
 *
 * @since: PersonalJava1.0
 */
public class PTimerSpec {
    private boolean absolute;
    private boolean repeat;
    private boolean regular;
    private long time;
    // If a subclass provides overriding definitions of
    //     addPTimerWentOffListener
    //     removePTimerWentOffListener
    //     notifyListeners
    // then it may use this field to store its implementation.
    //
    // In the default implementation, it either contains a single instance of
    // a listener or it contains an instance of Vector, holding multiple listeners.
    // This representation is subject to change.
    protected Object listeners;
    /**
     * Creates timer with the following properties:
     * <li>absolute = true
     * <li>repeat = false
     * <li>regular = true
     * <li>interval = 0
     */
    public PTimerSpec() {
        setAbsolute(true);
        setRepeat(false);
        setRegular(true);
        setTime(0);
    }
    
    /**
     * Specifies that the timer should send an event at an absolute time
     * <p>
     * When a timer event is set to be absolute, the time value is specified
     * in milliseconds since since midnight, January 1, 1970. Otherwise, the
     * delay time is scheduled relative to the current time.
     */
    public void setAbsolute(boolean absolute) {
        this.absolute = absolute;
    }

    /**
     * Checks if this specification is absolute.
     * @return the "Absolute" status of the PTimerSpec
     */
    public boolean isAbsolute() {
        return absolute;
    }
    
    /**
     * Sets the repeat property of this PTimerSpec.
     */
    public void setRepeat(boolean repeat) {
        this.repeat = repeat;
    }
    
    /**
     * Checks if this specification is repeating
     * @return the "Repeat" status of the PTimerSpec
     */
    public boolean isRepeat() {
        return repeat;
    }
    
    /**
     * Sets this specification to be regular or non-regular
     */
    public void setRegular(boolean regular) {
        this.regular = regular;
    }
    
    /**
     * Retrieves the "Regular" property of the PTimerSpec.
     */
    public boolean isRegular() {
        return regular;
    }
    
    /**
     * Sets when this specification should go off.  For absolute
     * specifications, this is a time in milliseconds since midnight,
     * January 1, 1970 UTC.  For delayed specifications, this is a
     * delay time in milliseconds.
     */
    public void setTime(long time) {
        this.time = time;
    }
    
    /**
     * Returns the absolute or delay time when this specification
     * will go off.
     */
    public long getTime() {
        return time;
    }
    
    // listeners
    
    /**
     * Adds a listener to this timer specification.
     * @param l the listener to add
     */
    public void addPTimerWentOffListener(PTimerWentOffListener l) {
        if (l == null) {
            throw new NullPointerException();
        }
        synchronized (this) {
            if (listeners == null) {
                listeners = l;
            } else {
                Vector v;
                if (listeners instanceof Vector) {
                    v = (Vector) listeners;
                } else {
                    v = new Vector(2);
                    v.addElement(listeners);
                }
                v.addElement(l);
                listeners = v;
            }
        }
    }
    
    /**
     * Removes a listener to this timer specification.  Silently does nothing
     * if the listener was not listening on this specification.
     * @param l the listener to remove
     */
    public void removePTimerWentOffListener(PTimerWentOffListener l) {
        if (l == null) {
            throw new NullPointerException();
        }
        synchronized (this) {
            if (listeners == null) {
                return;
            }
            if (listeners instanceof Vector) {
                Vector v = (Vector) listeners;
                v.removeElement(l);
                if (v.size() == 1) {
                    listeners = v.firstElement();
                }
            } else if (listeners == l) {
                listeners = null;
            }
        }
    }
        
    // convenience functions
    
    /**
     * Sets the PTimerSpec for an absolute, non-repeating time specified by when.
     * This is a convenience function equivalent to setAbsolute(true),
     * setTime(when), setRepeat(false).
     * @param when the absolute time for the specification to go off
     */
    public void setAbsoluteTime(long when) {
        setAbsolute(true);
        setTime(when);
        setRepeat(false);
    }
        
    /**
     * Sets the PTimerSpec for non-repeating, relative time specified by delay.
     * This is a convenience function equivalent to setAbsolute(false),
     * setTime(delay), setRepeat(false).
     * @param delay the relative time for the specification to go off
     */
    public void setDelayTime(long delay) {
        setAbsolute(false);
        setTime(delay);
        setRepeat(false);
    }
    
    // for the benefit of timer implementations
    
    /**
     * Calls all listeners registered on this timer specification.
     * This function is primarily for the benefit of those writing
     * implementations of PTimers.
     * @param source the PTimer that decided that this specification should go off
     */
    public void notifyListeners(PTimer source) {
        Vector v = null;
        PTimerWentOffListener singleton = null;
        synchronized (this) {
            if (listeners instanceof Vector) {
                v = (Vector) ((Vector) listeners).clone();
            } else {
                singleton = (PTimerWentOffListener) listeners;
            }
        }
        if (v != null) {
            Enumeration e = v.elements();
            while (e.hasMoreElements()) {
                PTimerWentOffListener l = (PTimerWentOffListener) e.nextElement();
                l.timerWentOff(new PTimerWentOffEvent(source, this));
            }
        } else if (singleton != null) {
            singleton.timerWentOff(new PTimerWentOffEvent(source, this));
        }
    }
}
