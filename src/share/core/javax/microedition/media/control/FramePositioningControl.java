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

/**
 * The <code>FramePositioningControl</code> is the interface to control 
 * precise positioning to a video frame for <code>Players</code>.
 * <p>
 * Frame numbers for a bounded movie must be non-negative 
 * and should generally begin with 0,
 * corresponding to media time 0.  Each video frame of a movie
 * must have a unique frame number that is one bigger than the
 * previous frame. 
 * <p>
 * There is a direct mapping between the frame number and the media
 * time of a video frame; although not all <code>Players</code> can
 * compute that relationship.  For <code>Players</code> that can
 * compute that relationship, the <code>mapFrameToTime</code> and
 * <code>mapTimeToFrame</code> methods can be used.
 * <p>
 * When a <code>Player</code> is seeked or skipped to a new video frame, 
 * the media time of the <code>Player</code> will be changed to the 
 * media time of the corresponding video frame.
 * <p>
 * As much as possible, the methods in this interface should 
 * provide frame-level accuracy with a plus-or-minus-one-frame 
 * margin of error to accommodate for round-off errors.  
 * However, if the content has inaccurate frame positioning 
 * information, implementations may not be able to provide
 * the necessary frame-level accuracy.  For instance, some
 * media content may contain wrong time-stamps or have missing 
 * frames.  In any case, the results of each
 * operation should represent the best effort.  For the
 * <code>seek</code> and <code>skip</code> methods, the returned 
 * value should indicate the actual new location or the number
 * of frames skipped.
 *
 */
public interface FramePositioningControl extends javax.microedition.media.Control {

/* JAVADOC ELIDED */
    int seek(int frameNumber);

/* JAVADOC ELIDED */
    int skip(int framesToSkip);

/* JAVADOC ELIDED */
    long mapFrameToTime(int frameNumber);

/* JAVADOC ELIDED */
    int mapTimeToFrame(long mediaTime);
}


