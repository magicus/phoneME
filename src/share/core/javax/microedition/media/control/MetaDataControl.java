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

package javax.microedition.media.control;

import javax.microedition.media.MediaException;

/**
 * <code>MetaDataControl</code> is used to retrieve metadata information
 * included within the media streams.  A <code>MetaDataControl</code>
 * object recognizes and stores metadata and provides XML-like accessor
 * methods to retrieve this information.
 * <br>
 * Predefined keys are provided to refer to commonly used metadata fields
 * (title, copyright, data, author).
 */
public interface MetaDataControl extends javax.microedition.media.Control {

    /**
     * Default key for AUTHOR information.
     * <p>
     * Value "author" is assigned to <code>AUTHOR_KEY</code>.
     */
    String AUTHOR_KEY = "author";

    /**
     * Default key for COPYRIGHT information.
     * <p>
     * Value "copyright" is assigned to <code>COPYRIGHT_KEY</code>.
     */
    String COPYRIGHT_KEY = "copyright";

    /**
     * Default key for DATE information.
     * <p>
     * Value "date" is assigned to <code>DATE_KEY</code>.
     */
    String DATE_KEY = "date";

    /**
     * Default key for TITLE information.
     * <p>
     * Value "title" is assigned to <code>TITLE_KEY</code>.
     */
    String TITLE_KEY = "title";

    /**
     * Return the list of keys for the available metadata values.
     * The returned array must be an array with at least one
     * key.
     *
     * @return The list of keys for the available metadata values.
     */
    String[] getKeys();

/* JAVADOC ELIDED */
    String getKeyValue(String key);
}
