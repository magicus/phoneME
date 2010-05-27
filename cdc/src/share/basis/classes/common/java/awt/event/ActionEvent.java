/*
 * @(#)ActionEvent.java	1.23 06/10/10
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
 * The action semantic event.
 * @see ActionListener
 *
 * @version 1.18 08/19/02
 * @author Carl Quinn
 */
public class ActionEvent extends AWTEvent {
    /**
     * The shift modifier constant.
     * Note: these values are nolonger initialized from Event.class
     * as this class is only present in PP, and not PBP.
     */
    public static final int SHIFT_MASK = 1 << 0; // Event.SHIFT_MASK;
    /**
     * The control modifier constant.
     */
    public static final int CTRL_MASK = 1 << 1; // Event.CTRL_MASK;
    /** 
     * The meta modifier constant.
     */
    public static final int META_MASK = 1 << 2; // Event.META_MASK;
    /** 
     * The alt modifier constant.
     */
    public static final int ALT_MASK = 1 << 3; // Event.ALT_MASK;
    /**
     * Marks the first integer id for the range of action event ids.
     */
    public static final int ACTION_FIRST = 1001;
    /**
     * Marks the last integer id for the range of action event ids.
     */
    public static final int ACTION_LAST = 1001;
    /**
     * An action performed event type.
     */
    public static final int ACTION_PERFORMED = ACTION_FIRST; //Event.ACTION_EVENT
    String actionCommand;
    int modifiers;
    /*
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = -7671078796273832149L;

    long when;
    /**
     * Constructs an ActionEvent object with the specified source object.
     * @param source the object where the event originated
     * @param id the type of event
     * @param command the command string for this action event
     */
    public ActionEvent(Object source, int id, String command) {
        this(source, id, command, 0);
    }

    /**
     * Constructs an ActionEvent object with the specified source object.
     * @param source the object where the event originated
     * @param id the type of event
     * @param command the command string for this action event
     * @param modifiers the modifiers held down during this action
     */
    public ActionEvent(Object source, int id, String command, int modifiers) {
        super(source, id);
        this.actionCommand = command;
        this.modifiers = modifiers;
    }

    public ActionEvent(Object source, int id, String command, long when,
                       int modifiers) {
        super(source, id);
        this.actionCommand = command;
        this.when = when;
        this.modifiers = modifiers;
    }
 
    /**
     * Returns the command name associated with this action.
     */
    public String getActionCommand() {
        return actionCommand;
    }

    /**
     * Returns the modifiers held down during this action event.
     */
    public int getModifiers() {
        return modifiers;
    }

    public long getWhen() {
        return when;
    }

    public String paramString() {
        String typeStr;
        switch (id) {
        case ACTION_PERFORMED:
            typeStr = "ACTION_PERFORMED";
            break;

        default:
            typeStr = "unknown type";
        }
        return typeStr + ",cmd=" + actionCommand + ",when=" + when +
            ",modifiers=" + KeyEvent.getKeyModifiersText(modifiers);
     }
}
