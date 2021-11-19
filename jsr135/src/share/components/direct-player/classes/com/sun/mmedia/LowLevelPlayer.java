/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.mmedia;

import javax.microedition.media.Control;
import javax.microedition.media.MediaException;

abstract class LowLevelPlayer {
    
    private HighLevelPlayer owner;
    
    /** Creates a new instance of LowLevelPlayer */
    LowLevelPlayer( HighLevelPlayer owner ) {
        this.owner = owner;
    }
    
    HighLevelPlayer getOwner()
    {
        return owner;
    }


    protected abstract void doNotifySnapshotFinished();

    /* This means that 'Record Size Limit (reached)' event received */
    protected abstract void doReceiveRSL();

    /**
     * Subclasses need to implement this to realize
     * the <code>Player</code>.
     *
     * @exception  MediaException  Description of the Exception
     */
    protected abstract void doRealize() throws MediaException;

    /**
     * Subclasses need to implement this to prefetch
     * the <code>Player</code>.
     *
     * @exception  MediaException  Description of the Exception
     */
    protected abstract void doPrefetch() throws MediaException;

    /**
     * Subclasses need to implement this start
     * the <code>Player</code>.
     *
     * @return    Description of the Return Value
     */
    protected abstract boolean doStart();

    /**
     * Subclasses must implement this method to do the actual starting
     * of worker threads.
     */
    protected abstract void doPostStart();

    /**
     * Subclasses need to implement this to realize
     * the <code>Player</code>.
     *
     * @exception  MediaException  Description of the Exception
     */
    protected abstract void doStop() throws MediaException;

    /**
     * Subclasses must implement this method to do pre stop works
     */
    protected abstract void doPreStop();

    /**
     * Subclasses need to implement this to deallocate
     * the <code>Player</code>.
     */
    protected abstract void doDeallocate();

    /**
     * Subclasses need to implement this to close
     * the <code>Player</code>.
     */
    protected abstract void doClose();

    /**
     * Subclasses need to implement this to set the media time
     * of the <code>Player</code>.
     *
     * @param  now                 Description of the Parameter
     * @return                     Description of the Return Value
     * @exception  MediaException  Description of the Exception
     */
    protected abstract long doSetMediaTime(long now) throws MediaException;

    /**
     * Subclasses need to implement this to get the media time
     * of the <code>Player</code>
     *
     * @return    Description of the Return Value
     */
    protected abstract long doGetMediaTime();

    /**
     * Subclasses need to implement this to get the duration
     * of the <code>Player</code>.
     *
     * @return    Description of the Return Value
     */
    protected abstract long doGetDuration();

    /**
     * Get a new Control of this 'type', or null if no control of this 'type
     * supported. 'New' means that no non-null Control of this 'type' has been
     * gotten yet (for this instance of LowLevelPlayer). The caller should
     * guarantee this to avoid the Control of a particular type created twice.
     *
     *
     * @param  type  the class name of the <code>Control</code>.
     * @return       <code>Control</code> for the class or interface
     * name.
     */
    protected abstract Control doGetNewControl(String type);

    /**
     * Subclasses must override this to be notified of a change
     * in the stop time
     *
     * @param  time  Description of the Parameter
     */
    protected abstract void doSetStopTime(long time);

    protected abstract String doGetContentType();

}
