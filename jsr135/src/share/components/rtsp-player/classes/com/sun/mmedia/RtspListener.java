/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp;

import com.sun.mmedia.rtsp.protocol.Message;

/**
 * The listener interface for incoming RTSP requests.
 *
 * @author     Marc Owerfeldt
 * @created    June 7, 2003
 */

public interface RtspListener {
    /**
     * Informs the application that an RTSP message has 
     * been received from the RTSP server.
     *
     * @param  connectionId  The connection ID
     * @param  message       The RTSP message
     */
    void rtspMessageIndication(int connectionId, Message message);


    /**
     * Informs the application that the connection to 
     * the RTSP server has been terminated by the server.
     *
     * @param  connectionId  The connection ID
     */
    void rtspConnectionTerminated(int connectionId);
}


