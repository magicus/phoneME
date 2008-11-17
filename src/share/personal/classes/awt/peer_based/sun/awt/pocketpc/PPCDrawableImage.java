/*
 * @(#)PPCDrawableImage.java	1.6 06/10/10
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
 */

package sun.awt.pocketpc;

import java.awt.image.ImageObserver;
import java.awt.Color;

/** Defines an image that is capable of being drawn by a PPCGraphics object.*/

interface PPCDrawableImage {
    boolean draw(PPCGraphics g, int x, int y, ImageObserver observer);
    boolean draw(PPCGraphics g, int x, int y, int width, int height,
        ImageObserver observer);
    boolean draw(PPCGraphics g, int x, int y, Color bg,
        ImageObserver observer);
    boolean draw(PPCGraphics g, int x, int y, int width, int height,
        Color bg, ImageObserver observer);
    boolean draw(PPCGraphics g, int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        ImageObserver observer);
    boolean draw(PPCGraphics g, int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        Color bgcolor, ImageObserver observer);
    boolean isComplete();
    int getWidth();
    int getHeight();
}
