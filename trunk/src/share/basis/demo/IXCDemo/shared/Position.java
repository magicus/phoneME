/*
 * @(#)Position.java	1.3 03/01/16
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
 * Represents a 2D position with float coordinates.
 */

package IXCDemo.shared;

public class Position implements java.io.Serializable {
    static final long serialVersionUID = -1510071578429305515L;
    private float x;
    private float y;
    public Position(float x, float y) {
        this.x = x;
        this.y = y;
    }
    
    public Position(Position p) {
        this(p.x, p.y);
    }
    
    public String toString() {
        return "Position(x=" + x + ", y=" + y + ")";
    }
    
    public float getX() {
        return x;
    }
    
    public float getY() {
        return y;
    }
    
    public int hashCode() {
        return Float.floatToIntBits(x) ^ Float.floatToIntBits(y);
    }
    
    public boolean equals(Object other) {
        if (!(other instanceof Position)) {
            return false;
        }
        Position po = (Position) other;
        return x == po.x && y == po.y;
    }
}
