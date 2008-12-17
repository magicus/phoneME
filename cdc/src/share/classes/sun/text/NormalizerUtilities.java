/*
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

/*
 * @(#)NormalizerUtilities.java	1.7 06/10/10
 */

package sun.text;

public class NormalizerUtilities {

    public static int toLegacyMode(Normalizer.Mode mode) {
        // find the index of the legacy mode in the table;
        // if it's not there, default to Collator.NO_DECOMPOSITION (0)
        int legacyMode = legacyModeMap.length;
        while (legacyMode > 0) {
            --legacyMode;
            if (legacyModeMap[legacyMode] == mode) {
                break;
            }
        }
        return legacyMode;
    }

    public static Normalizer.Mode toNormalizerMode(int mode) {
        Normalizer.Mode normalizerMode;

        try {
            normalizerMode = legacyModeMap[mode];
        }
        catch(ArrayIndexOutOfBoundsException e) {
            normalizerMode = Normalizer.NO_OP;
        }
        return normalizerMode;

    }


    static Normalizer.Mode[] legacyModeMap = {
        Normalizer.NO_OP,           // Collator.NO_DECOMPOSITION
        Normalizer.DECOMP,          // Collator.CANONICAL_DECOMPOSITION
        Normalizer.DECOMP_COMPAT,   // Collator.FULL_DECOMPOSITION
    };

}


