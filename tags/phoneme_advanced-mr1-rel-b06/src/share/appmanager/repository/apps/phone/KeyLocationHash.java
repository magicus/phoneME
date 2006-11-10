/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

import java.util.Enumeration;
import java.util.Hashtable;

public class KeyLocationHash
    extends Hashtable {

//    int offsetX = 0;
//    int offsetY = 0;

    AppLocation displayRegion = new AppLocation(40, 20, 470, 70);

    public void put(int x_start, int y_start, int x_end, int y_end,
                    Object element) {
        put(new AppLocation(x_start, y_start, x_end, y_end), element);
    }

    public void setDisplay(int x_start, int y_start, int x_end, int y_end) {
        displayRegion = new AppLocation(x_start, y_start, x_end, y_end);
    }

    public Object isItemAssigned(int x, int y) {
        for (Enumeration enu = keys(); enu.hasMoreElements(); ) {
            AppLocation l = (AppLocation) enu.nextElement();
            if (l.contains(x, y)) {
                return get(l);
            }
        }
        return null;
    }

    class AppLocation {
        int x_min, y_min, x_max, y_max;
        int offsetX = 0;
        public AppLocation(int startX, int startY, int endX, int endY) {
            x_min = startX + offsetX;
            y_min = startY;
            x_max = endX + offsetX;
            y_max = endY;
        }

        public boolean contains(int x, int y) {
            return (x > x_min && y > y_min && x < x_max && y < y_max);
        }
    }

}
