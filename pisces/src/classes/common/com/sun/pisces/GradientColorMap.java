/*
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved. 
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

package com.sun.pisces;

public class GradientColorMap {

    public static final int CYCLE_NONE = 0;
    public static final int CYCLE_REPEAT = 1;
    public static final int CYCLE_REFLECT = 2;

    public int cycleMethod;

    private static final int LG_RAMP_SIZE = 8;
    private static final int RAMP_SIZE = 1 << LG_RAMP_SIZE;
    public int[] fractions = null;
    public int[] rgba = null;
    public int[] colors = null;

//     private static void printArray(int[] arr, String name) {
//         System.out.print(name + " = {");
//         System.out.print(arr[0]);
//         for (int i = 1; i < arr.length; i++) {
//             System.out.print(", " + arr[i]);
//         }
//         System.out.println("}");
//     }

    public GradientColorMap(int[] fractions, int[] rgba, int cycleMethod) {
        this.cycleMethod = cycleMethod;

//         System.out.println("GCM ctor:");
//         printArray(fractions, "fractions before");
//         printArray(rgba, "rgba before");
    
        int numStops = fractions.length;
        if (fractions[0] != 0) {
            int[] nfractions = new int[numStops + 1];
            int[] nrgba = new int[numStops + 1];
            System.arraycopy(fractions, 0, nfractions, 1, numStops);
            System.arraycopy(rgba, 0, nrgba, 1, numStops);
            nfractions[0] = 0;
            nrgba[0] = rgba[0];
            fractions = nfractions;
            rgba = nrgba;
            ++numStops;
        }

        if (fractions[numStops - 1] != 0x10000) {
            int[] nfractions = new int[numStops + 1];
            int[] nrgba = new int[numStops + 1];
            System.arraycopy(fractions, 0, nfractions, 0, numStops);
            System.arraycopy(rgba, 0, nrgba, 0, numStops);
            nfractions[numStops] = 0x10000;
            nrgba[numStops] = rgba[numStops - 1];
            fractions = nfractions;
            rgba = nrgba;
        }

//         printArray(fractions, "fractions after");
//         printArray(rgba, "rgba after");

        this.fractions = new int[fractions.length];
        System.arraycopy(fractions, 0, this.fractions, 0, fractions.length);
        this.rgba = new int[rgba.length];
        System.arraycopy(rgba, 0, this.rgba, 0, rgba.length);

        createRamp(fractions, rgba);
    }

    private int pad(int frac) {
        switch (cycleMethod) {
        case CYCLE_NONE:
            if (frac < 0) {
                return 0;
            } else if (frac > 0xffff) {
                return 0xffff;
            } else {
                return frac;
            }

        case CYCLE_REPEAT:
            return frac & 0xffff;

        case CYCLE_REFLECT:
            if (frac < 0) {
                frac = -frac;
            }
            frac = frac & 0x1ffff;
            if (frac > 0xffff) {
                frac = 0x1ffff - frac;
            }
            return frac;
            
        default:
            throw new RuntimeException("Unknown cycle method: " + cycleMethod);
        }
    }

    private void accumColor(int frac, int[] fractions,
                            int[] r, int[] g, int[] b, int[] a,
                            int[] red, int[] green, int[] blue, int[] alpha) {
        int numStops = fractions.length;
        int stop = -1;
        for (int i = 0; i < numStops; i++) {
            if (fractions[i] > frac) {
                stop = i;
                break;
            }
        }

        frac -= fractions[stop - 1];
        int delta = fractions[stop] - fractions[stop - 1];

        red[0] += r[stop - 1] + (frac*(r[stop] - r[stop - 1]))/delta;
        green[0] += g[stop - 1] + (frac*(g[stop] - g[stop - 1]))/delta;
        blue[0] += b[stop - 1] + (frac*(b[stop] - b[stop - 1]))/delta;
        alpha[0] += a[stop - 1] + (frac*(a[stop] - a[stop - 1]))/delta;
    }

    private int getColorAA(int frac, int[] fractions,
                           int[] r, int[] g, int[] b, int[] a,
                           int[] red, int[] green, int[] blue, int[] alpha) {
        int delta = 256;
        int step = 1;
        int total = 0;
        for (int i = -delta; i <= delta; i += step) {
            int f = pad(frac + i);
            accumColor(f, fractions, r, g, b, a, red, green, blue, alpha);
            ++total;
        }

        alpha[0] /= total;
        red[0] /= total;
        green[0] /= total;
        blue[0] /= total;

        return (alpha[0] << 24) | (red[0] << 16) | (green[0] << 8) | blue[0];
    }

    private void createRamp(int[] fractions, int[] rgba) {
        this.colors = new int[RAMP_SIZE];

        int[] alpha = new int[1];
        int[] red = new int[1];
        int[] green = new int[1];
        int[] blue = new int[1];

        int numStops = fractions.length;
        int[] a = new int[numStops];
        int[] r = new int[numStops];
        int[] g = new int[numStops];
        int[] b = new int[numStops];

        for (int i = 0; i < numStops; i++) {
            a[i] = (rgba[i] >> 24) & 0xff;
            r[i] = (rgba[i] >> 16) & 0xff;
            g[i] = (rgba[i] >>  8) & 0xff;
            b[i] =  rgba[i]        & 0xff;
        }

        for (int i = 0; i < RAMP_SIZE; i++) {
            red[0] = green[0] = blue[0] = alpha[0] = 0;
            colors[i] = getColorAA(i << (16 - LG_RAMP_SIZE), fractions,
                                   r, g, b, a,
                                   red, green, blue, alpha);
        }
    }

    public int getColor(int frac) {
        frac = pad(frac);
        frac >>= 16 - LG_RAMP_SIZE;
        return colors[frac];
    }
}
