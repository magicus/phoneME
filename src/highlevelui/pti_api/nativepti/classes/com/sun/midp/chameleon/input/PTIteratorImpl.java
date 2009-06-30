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

package com.sun.midp.chameleon.input;
import com.sun.midp.io.Util;
import com.sun.midp.log.LogChannels;
import com.sun.midp.log.Logging;

/** 
 * Implements PTIterator using machine-dependent KNI interface.
 *
 */
public class PTIteratorImpl implements PTIterator {
    /** 
     * Internal array size to hold KNI word completion
     */
    static final int MAX_STRING = 128;
    
    /** 
     * Internal array to hold KNI word completion
     */
    private byte[] entry;

    /** 
     * current handle
     */
    private int handle;
    
    /** 
     * create a new iterator
     * @param dictionary dictionary id 
     */
    public PTIteratorImpl(int dictionary) {
        entry = new byte[MAX_STRING];
        handle = ptNewIterator0(dictionary);
    }


    /** 
     * create a new handle and clear completion state by
     * calling ptNewIterator0()
     */
    public void reset() {
        ptClear0(handle);
    }
    
    /** 
     * check if current handle is valid
     * @return true is valid, false otherwise
     */
    public boolean isValid() {
        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_HIGHUI,
                "isValid = " + (handle != 0));
        }
        return handle > 0;
    }
    
    /** 
     * Adds a key to current completion string
     * @param keyCode char in the range '0'-'9', '#', or '*'
     */
    public void nextLevel(int keyCode) {
        if (isValid()) {
            ptAddKey0(handle, keyCode);
        }
    }
    
    /** 
     * Backspace on key in current completion string.
     */
    public void prevLevel() {
        if (isValid()) {
            ptDeleteKey0(handle);
        }
    }
    
    /**
     * Returns true if the iteration has more elements. (In other words,
     * returns <code>true</code> if <code>next</code> would return an
     * element rather than throwing an exception.)
     *
     * @return true if the iterator has more elements.
     */
    public boolean hasNext() {
        boolean ret = false;
        if (isValid()) {
            ret = ptHasCompletionOption0(handle);
        }
        return ret;
    }
    
    /** 
     * Reverts to first possible completion.
     * If next() has been called uptil hasNext() returns false, then after 
     * calling reviewCompletionOptions(), calling next() will return
     * the 1st completion
     */
    public void resetNext() {
        if (isValid()) {
            ptRenewCompletionOptions0(handle);
        }
    }

    /**
     * Returns the next element in the iteration.
     *
     * @return next element in the iteration.
     *
     * @exception NoSuchElementException iteration has no more elements.
     */
    public String next() {
        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_HIGHUI,
                "[iter.nextCompletionOption] >>");
        }
        String ret = null;

        if (isValid()) {
            ret = ptNextCompletionOption0(handle, entry.length);
        }

        if (ret == null)
            ret = "";

        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_HIGHUI,
                "[iter.nextCompletionOption] : " + ret);
        }

        return ret;
    }

    /** 
     * NATIVE CODE
     */

    /**
     * Create a new iterator instance
     *
     * @param dictionary library handle
     * @return handle of new iterator.
     */
    private static native int ptNewIterator0(int dictionary);

    /**
     * Clear all text from the predictive text iterator 
     *
     * @param handle the handle of the iterator 
     * @return true if iterator has been cleared successfully otherwise false.
     */
    private static native boolean  ptClear0(int handle);

    /**
     * Advances the predictive text iterator using the next key code
     *
     * @param handle the handle of the iterator 
     * @param keyCode the next key ('0'-'9')
     *
     * @return true if key code has been added successfully otherwise false
     */
    private static native boolean  ptAddKey0(int handle, int keyCode);

    /**
     * Backspace the iterator one key 
     *
     * @param handle the handle of the iterator 
     * @return true if key has been deleted successfully otherwise false.
     */
    private static native boolean  ptDeleteKey0(int handle);
    
    /**
     * reset completion options for for the current predictive text entry
     * After this call, ptNextCompletionOption() will return all
     * completion options starting from 1st one.
     *
     * @param handle the handle of the iterator 
     * @return true if iterator has been reset successfully otherwise false.
     */
    private static native boolean  ptRenewCompletionOptions0(int handle);

    /**
     * return the current predictive text completion option
     *
     * @param handle the handle of the iterator 
     * @param outMaxSize max size of the outArray 
     *
     * @return next element in the iteration
     */
    private static native String ptNextCompletionOption0(
                                         int handle, int outMaxSize);

    /**
     * see if exist further completion options for the current
     * predictive text entry
     *
     * @param handle the handle of the iterator 
     *
     * @return true if more completion options exist, false otherwise
     */
    private static native boolean ptHasCompletionOption0(int handle);

}
