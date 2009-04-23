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


/**
 * Machine dependent API to predictive text library
 *
 * This implementation assumes that predictive text library
 * loaded and handled by platfrom. As a result this class
 * serves just as a Iteraror factory
 */
public class PTDictionaryImpl implements PTDictionary {
    /** library language */
    private String language;

    /** Library handle. It's given out during library initialization */
    private int handle;

    /** library iterator */
    private PTIterator iterator;

    /** 
     * call ptInitLibrary0() on 1st execution
     * @param lang current language
     */
    public PTDictionaryImpl(String lang) {
        language = lang;
        handle = ptInitLibrary0(lang);
    }
    
    /** 
     * check if current handle is valid
     * @return true is valid, false otherwise
     */
    public boolean isValid() {
        return handle > 0;
    }

    /**
     * get a machine dependent predictive text Iterattor
     *
     * @return an iterator of the class IteratorImpl
     */
    public PTIterator iterator() {
        if (iterator == null && isValid())
            iterator = new PTIteratorImpl(handle);
        return iterator;
    }

    /**
     * adding words to the predictive text dictionary is not
     * supported on the platform. Returns false always
     *
     * @param word new word to add
     * @return false (not supported on the platform)
     */
    public boolean addWord(String word) {
        return false;
    }

    /** 
     * NATIVE CODE
     * 
     */

    /**
     * Called 1st time predictive text library is accessed.
     * it calls platform specific predictive text initialization functions
     *
     * @param lang the language used to select the library
     * @return the handle of the library 
     */
    private static native int ptInitLibrary0(String lang);
}
