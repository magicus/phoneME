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
package com.sun.mmedia.rtsp;

import com.sun.mmedia.BasicPlayer;
import javax.microedition.media.Control;

/**
 * RtspCtrl implements controls exported by the RTSP Player.
 *
 * RtspCtrl informs the application about the
 * state of the RTSP player, for example 'Negotiating',
 * 'Buffering', 'Streaming', 'File not found' etc.
 *
 */
public class RtspCtrl implements Control {
    // Status message informing the user about RTSP client/server
    private String status;
    private BasicPlayer player;


    /**
     * Constructor for the RtspCtrl object
     *
     * @param  player  The player that exports this control
     */
    public RtspCtrl(BasicPlayer player) {
        this.player = player;
    }


    /**
     * Sets the status attribute of the RtspCtrl object
     *
     * @param  status  The new status value
     */
    public void setStatus(String status) {
        this.status = status;
    }


    /**
     * Gets the status attribute of the RtspCtrl object
     *
     * @return    The status value
     */
    public String getStatus() {
        return status;
    }
}

