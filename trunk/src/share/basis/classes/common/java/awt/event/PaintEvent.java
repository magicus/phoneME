/*
 * @(#)PaintEvent.java	1.16 06/10/10
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

import java.awt.Component;
import java.awt.Rectangle;

/**
 * The component-level paint event.
 * This event is a special type which is used to ensure that
 * paint/update method calls are serialized along with the other
 * events delivered from the event queue.  This event is not
 * designed to be used with the Event Listener model; programs
 * should continue to override paint/update methods in order
 * render themselves properly.
 *
 * @version 1.12 08/19/02
 * @author Amy Fowler
 */
public class PaintEvent extends ComponentEvent {
    /**
     * Marks the first integer id for the range of paint event ids.
     */    
    public static final int PAINT_FIRST = 800;
    /**
     * Marks the last integer id for the range of paint event ids.
     */
    public static final int PAINT_LAST = 801;
    /**
     * The paint event type.  
     */
    public static final int PAINT = PAINT_FIRST;
    /**
     * The update event type.  
     */
    public static final int UPDATE = PAINT_FIRST + 1; //801
    Rectangle updateRect;
    /*
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = 1267492026433337593L;
    /**
     * Constructs a PaintEvent object with the specified source component
     * and type.
     * @param source the object where the event originated
     * @id the event type
     * @updateRect the rectangle area which needs to be repainted
     */
    public PaintEvent(Component source, int id, Rectangle updateRect) {
        super(source, id);
        this.updateRect = updateRect;
    }

    /**
     * Returns the rectangle representing the area which needs to be
     * repainted in response to this event.
     */
    public Rectangle getUpdateRect() {
        return updateRect;
    }

    /**
     * Sets the rectangle representing the area which needs to be
     * repainted in response to this event.
     * @param updateRect the rectangle area which needs to be repainted
     */
    public void setUpdateRect(Rectangle updateRect) {
        this.updateRect = updateRect;
    }

    public String paramString() {
        String typeStr;
        switch (id) {
        case PAINT:
            typeStr = "PAINT";
            break;

        case UPDATE:
            typeStr = "UPDATE";
            break;

        default:
            typeStr = "unknown type";
        }
        return typeStr + ",updateRect=" + (updateRect != null ? updateRect.toString() : "null");
    }
}
