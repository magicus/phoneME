/*
 * @(#)Cursor.java	1.17 06/10/10
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
package java.awt;

/**
 * A class to encapsulate the bitmap representation of the mouse cursor.
 *
 * @see Component#setCursor
 * @version 	1.12, 08/19/02
 * @author 	Amy Fowler
 */
public class Cursor implements java.io.Serializable {
    /**
     * The default cursor type (gets set if no cursor is defined).
     */
    public static final int	DEFAULT_CURSOR = 0;
    /**
     * The crosshair cursor type.
     */
    public static final int	CROSSHAIR_CURSOR = 1;
    /**
     * The text cursor type.
     */
    public static final int	TEXT_CURSOR = 2;
    /**
     * The wait cursor type.
     */
    public static final int	WAIT_CURSOR = 3;
    /**
     * The south-west-resize cursor type.
     */
    public static final int	SW_RESIZE_CURSOR = 4;
    /**
     * The south-east-resize cursor type.
     */
    public static final int	SE_RESIZE_CURSOR = 5;
    /**
     * The north-west-resize cursor type.
     */
    public static final int	NW_RESIZE_CURSOR = 6;
    /**
     * The north-east-resize cursor type.
     */
    public static final int	NE_RESIZE_CURSOR = 7;
    /**
     * The north-resize cursor type.
     */
    public static final int	N_RESIZE_CURSOR = 8;
    /**
     * The south-resize cursor type.
     */
    public static final int	S_RESIZE_CURSOR = 9;
    /**
     * The west-resize cursor type.
     */
    public static final int	W_RESIZE_CURSOR = 10;
    /**
     * The east-resize cursor type.
     */
    public static final int	E_RESIZE_CURSOR = 11;
    /**
     * The hand cursor type.
     */
    public static final int	HAND_CURSOR = 12;
    /**
     * The move cursor type.
     */
    public static final int	MOVE_CURSOR = 13;

    protected static Cursor predefined[] = new Cursor[14];
    int type = DEFAULT_CURSOR;
    /*
     * JDK 1.1 serialVersionUID 
     */
    private static final long serialVersionUID = 8028237497568985504L;
    /**
     * Returns a cursor object with the specified predefined type.
     * @param type the type of predefined cursor
     */
    static public Cursor getPredefinedCursor(int type) {
        if (type < Cursor.DEFAULT_CURSOR || type > Cursor.MOVE_CURSOR) {
            throw new IllegalArgumentException("illegal cursor type");
        }
        if (predefined[type] == null) {
            predefined[type] = new Cursor(type);
        }
        return predefined[type];
    }

    /**
     * Return the system default cursor.
     */
    static public Cursor getDefaultCursor() {
        return getPredefinedCursor(Cursor.DEFAULT_CURSOR);
    }

    /**
     * Creates a new cursor object with the specified type.
     * @param type the type of cursor
     */
    public Cursor(int type) {
        if (type < Cursor.DEFAULT_CURSOR || type > Cursor.MOVE_CURSOR) {
            throw new IllegalArgumentException("illegal cursor type");
        }
        this.type = type;
    }

    /**
     * Returns the type for this cursor.
     */
    public int getType() {
        return type;
    }	
}
