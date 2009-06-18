/*
 * @(#)ComponentEvent.java	1.24 06/10/10
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

package java.awt.event;

import java.awt.AWTEvent;
import java.awt.Component;
import java.awt.Rectangle;

/**
 * The root event class for all component-level events.
 * These events are provided for notification purposes ONLY;
 * The AWT will automatically handle component moves and resizes
 * internally so that GUI layout works properly regardless of
 * whether a program is receiving these events or not.
 *
 * @see ComponentListener
 *
 * @version 1.20 08/19/02
 * @author Carl Quinn
 */
public class ComponentEvent extends AWTEvent {
    /**
     * Marks the first integer id for the range of component event ids.
     */
    public static final int COMPONENT_FIRST = 100;
    /**
     * Marks the last integer id for the range of component event ids.
     */
    public static final int COMPONENT_LAST = 103;
    /**
     * The component moved event type.
     */
    public static final int COMPONENT_MOVED = COMPONENT_FIRST;
    /**
     * The component resized event type.
     */
    public static final int COMPONENT_RESIZED = 1 + COMPONENT_FIRST;
    /**
     * The component shown event type.
     */
    public static final int COMPONENT_SHOWN = 2 + COMPONENT_FIRST;
    /**
     * The component hidden event type.
     */
    public static final int COMPONENT_HIDDEN = 3 + COMPONENT_FIRST;
    /*
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = 8101406823902992965L;
    /**
     * Constructs a ComponentEvent object with the specified source component 
     * and type.
     * @param source the component where the event originated
     * @id the event type
     */
    public ComponentEvent(Component source, int id) {
        super(source, id);
    }

    /**
     * Returns the component where this event originated.
     */
    public Component getComponent() {
        //        return (source instanceof Component) ? (Component)source : null;
        return (Component) source; // cast should always be OK, type was checked in constructor
    }

    public String paramString() {
        String typeStr;
        Rectangle b = (source != null
                ? ((Component) source).getBounds()
                : null);
        switch (id) {
        case COMPONENT_SHOWN:
            typeStr = "COMPONENT_SHOWN";
            break;

        case COMPONENT_HIDDEN:
            typeStr = "COMPONENT_HIDDEN";
            break;

        case COMPONENT_MOVED:
            typeStr = "COMPONENT_MOVED (" + 
                    b.x + "," + b.y + " " + b.width + "x" + b.height + ")";
            break;

        case COMPONENT_RESIZED:
            typeStr = "COMPONENT_RESIZED (" + 
                    b.x + "," + b.y + " " + b.width + "x" + b.height + ")";
            break;

        default:
            typeStr = "unknown type";
        }
        return typeStr;
    }
}
