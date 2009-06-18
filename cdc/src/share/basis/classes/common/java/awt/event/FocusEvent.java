/*
 * @(#)FocusEvent.java	1.26 06/10/10
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
import sun.awt.AppContext;
import sun.awt.SunToolkit;

/**
 * The component-level focus event.
 * There are two levels of focus change events: permanent and temporary.
 * Permanent focus change events occur when focus is directly moved
 * from one component to another, such as through calls to requestFocus()
 * or as the user uses the Tab key to traverse components.
 * Temporary focus change events occur when focus is temporarily
 * gained or lost for a component as the indirect result of another
 * operation, such as window deactivation or a scrollbar drag.  In this
 * case, the original focus state will automatically be restored once
 * that operation is finished, or, for the case of window deactivation,
 * when the window is reactivated.  Both permanent and temporary focus
 * events are delivered using the FOCUS_GAINED and FOCUS_LOST event ids;
 * the levels may be distinguished in the event using the isTemporary()
 * method.
 *  
 * @version 1.20 08/19/02
 * @author Carl Quinn
 * @author Amy Fowler
 */
public class FocusEvent extends ComponentEvent {
    /**
     * Marks the first integer id for the range of focus event ids.
     */    
    public static final int FOCUS_FIRST = 1004;
    /**
     * Marks the last integer id for the range of focus event ids.
     */
    public static final int FOCUS_LAST = 1005;
    /**
     * The focus gained event type.  
     */
    public static final int FOCUS_GAINED = FOCUS_FIRST; //Event.GOT_FOCUS
    /**
     * The focus lost event type.  
     */
    public static final int FOCUS_LOST = 1 + FOCUS_FIRST; //Event.LOST_FOCUS
    boolean temporary = false;
    /*
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = 523753786457416396L;
    
    transient Component opposite;

    /**
     * Constructs a FocusEvent object with the specified source component,
     * type, and whether or not the focus event is a temporary level event.
     * @param source the object where the event originated
     * @id the event type
     * @temporary whether or not this focus change is temporary
     */
    public FocusEvent(Component source, int id, boolean temporary) {
        super(source, id);
        this.temporary = temporary;
    }

    /**
     * Constructs a permanent-level FocusEvent object with the 
     * specified source component and type.
     * @param source the object where the event originated
     * @id the event type
     */
    public FocusEvent(Component source, int id) {
        this(source, id, false);
    }

    public FocusEvent(Component source, int id, boolean temporary,
                      Component opposite) {
        super(source, id);
	    this.temporary = temporary;
	    this.opposite = opposite;
    }

    /**
     * Returns whether or not this focus change event is a temporary
     * change.
     */
    public boolean isTemporary() {
        return temporary;
    }
 
    public Component getOppositeComponent() {
        if (opposite == null) {
            return null;
        }
        return (SunToolkit.targetToAppContext(opposite) ==
                AppContext.getAppContext()) ? opposite : null;
    }

    public String paramString() {
        String typeStr;
        switch (id) {
        case FOCUS_GAINED:
            typeStr = "FOCUS_GAINED";
            break;

        case FOCUS_LOST:
            typeStr = "FOCUS_LOST";
            break;

        default:
            typeStr = "unknown type";
        }
        return typeStr + (temporary ? ",temporary" : ",permanent") +
            ",opposite=" + getOppositeComponent();
    }
}
