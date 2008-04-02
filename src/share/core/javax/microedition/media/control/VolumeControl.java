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

/* JAVADOC ELIDED */

public interface VolumeControl extends javax.microedition.media.Control {
    
/* JAVADOC ELIDED */
    void setMute(boolean mute);

    /**
     * Get the mute state of the signal associated with this 
     * <code>VolumeControl</code>.
     *
     * @see #setMute
     *
     * @return The mute state.
     */
    boolean isMuted();

/* JAVADOC ELIDED */
    int setLevel(int level);

    /**
     * Get the current volume level set.
     * <br>
     * <code>getLevel</code> may return <code>-1</code>
     * if and only if the <code>Player</code> is in the
     * <i>REALIZED</i> state (the audio device has not been 
     * initialized) and <code>setLevel</code> has not 
     * yet been called.
     *  
     * @see #setLevel
     *
     * @return The current volume level or <code>-1</code>.
     */
    int getLevel();
}
