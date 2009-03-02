/*
 *   
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

package javax.microedition.lcdui;

import com.sun.midp.configurator.Constants;
import java.lang.ref.WeakReference;

/**
 * Implementation class for TickerLF interface.
 */
class TickerLFImpl implements TickerLF {

    /**
     * Constructs a new <code>Ticker</code> object, given its initial
     * contents string.
     * @param ticker Ticker object for which L&F has to be created
     * @throws NullPointerException if <code>str</code> is <code>null</code>
     */
    TickerLFImpl(Ticker ticker) {
	    this.ticker = ticker;
        owners = new WeakReference[1];
        /* numOfOwners = 0; */
    }

    /**
     * needed for implementations where the ticker is operated through the
     * Displayable directly
     * @param owner the last Displayable this ticker was set to
     */
    public void lSetOwner(DisplayableLF owner) {
        if (owners.length == numOfOwners) {
            WeakReference newOwners[] =
                new WeakReference[numOfOwners + 1];
            System.arraycopy(owners, 0, newOwners, 0, numOfOwners);
            owners = newOwners;

        }
        owners[numOfOwners] = new WeakReference(owner);
        numOfOwners++;
    }
    
    /**
     * change the string set to this ticker
     * @param str string to set on this ticker.
     */
    public void lSetString(String str) {
        for (int i = 0; i < numOfOwners; i++) {
            DisplayableLF owner = (DisplayableLF)owners[i].get();
            if (owner != null) {
                Display d = owner.lGetCurrentDisplay();
                if (d != null) {
                    d.lSetTicker(owner, ticker);
                }
            }
        }
    }

    /**
     * DisplayableLFs this ticker is associated with
     */
    private WeakReference[] owners;

    /**
     * The number of owners.
     */
    int numOfOwners;

    
    /** Ticker object that corresponds to this Look & Feel object */
    private Ticker ticker;
}

