/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
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
 * @author     Marc Owerfeldt
 * @created    September 25, 2002
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

