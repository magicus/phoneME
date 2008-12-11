/*
 * @(#)TextEvent.java	1.14 06/10/10
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

/**
 * The text event emitted by TextComponents.
 * @see java.awt.TextComponent
 * @see TextEventListener
 *
 * @version 1.10 08/19/02
 * @author Georges Saab
 */

public class TextEvent extends AWTEvent {
    /**
     * Marks the first integer id for the range of adjustment event ids.
     */
    public static final int TEXT_FIRST = 900;
    /**
     * Marks the last integer id for the range of adjustment event ids.
     */
    public static final int TEXT_LAST = 900;
    /**
     * The adjustment value changed event.
     */
    public static final int TEXT_VALUE_CHANGED = TEXT_FIRST;
    /*
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = 6269902291250941179L;
    /**
     * Constructs a TextEvent object with the specified TextComponent source,
     * and type.
     * @param source the TextComponent where the event originated
     * @id the event type
     * @type the textEvent type 
     */
    public TextEvent(Object source, int id) {
        super(source, id);
    }

    public String paramString() {
        String typeStr;
        switch (id) {
        case TEXT_VALUE_CHANGED:
            typeStr = "TEXT_VALUE_CHANGED";
            break;

        default:
            typeStr = "unknown type";
        }
        return typeStr;
    }
}
