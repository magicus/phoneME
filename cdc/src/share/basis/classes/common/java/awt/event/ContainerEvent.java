/*
 * @(#)ContainerEvent.java	1.12 06/10/10
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
import java.awt.Container;
import java.awt.Component;

/**
 * The class for container-level events.
 * These events are provided for notification purposes ONLY;
 * The AWT will automatically handle container add and remove
 * operations internally.
 *
 * @see ContainerListener
 *
 * @version 1.8 08/19/02
 * @author Tim Prinzing
 * @author Amy Fowler
 */
public class ContainerEvent extends ComponentEvent {
    /**
     * Marks the first integer id for the range of container event ids.
     */
    public static final int CONTAINER_FIRST = 300;
    /**
     * Marks the last integer id for the range of container event ids.
     */
    public static final int CONTAINER_LAST = 301;
    /**
     * The component moved event type.
     */
    public static final int COMPONENT_ADDED = CONTAINER_FIRST;
    /**
     * The component resized event type.
     */
    public static final int COMPONENT_REMOVED = 1 + CONTAINER_FIRST;
    Component child;
    /*
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = -4114942250539772041L;
    /**
     * Constructs a ContainerEvent object with the specified source
     * container, type, and child which is being added or removed. 
     * @param source the container where the event originated
     * @id the event type
     * @child the child component
     */
    public ContainerEvent(Component source, int id, Component child) {
        super(source, id);
        this.child = child;
    }

    /**
     * Returns the container where this event originated.
     */
    public Container getContainer() {
        return (Container) source; // cast should always be OK, type was checked in constructor
    }

    /**
     * Returns the child component that was added or removed in
     * this event.
     */
    public Component getChild() {
        return child;
    }

    public String paramString() {
        String typeStr;
        switch (id) {
        case COMPONENT_ADDED:
            typeStr = "COMPONENT_ADDED";
            break;

        case COMPONENT_REMOVED:
            typeStr = "COMPONENT_REMOVED";
            break;

        default:
            typeStr = "unknown type";
        }
        return typeStr + ",child=" + child.getName();
    }
}
