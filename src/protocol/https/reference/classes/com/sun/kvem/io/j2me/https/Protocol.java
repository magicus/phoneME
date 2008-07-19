/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/
package com.sun.kvem.io.j2me.https;

import com.sun.kvem.netmon.HttpsAgent;
import com.sun.kvem.netmon.StreamConnectionStealer;
import com.sun.midp.io.j2me.http.StreamConnectionElement;

import javax.microedition.io.StreamConnection;
import java.io.IOException;

/**
 * By subclassing the https protocol class of the midp and by overriding
 * the connect method this class wrapp the StreamConnection with a stream
 * StreamConnectionStealer and "steals" the data along the way to the user midlet.
 * The data is sent to the HttpsAgent and from there to the network monito gui.
 *
 *@author ah123546
 *@created December 25, 2001
 *@version
 */
public class Protocol extends com.sun.midp.io.j2me.https.Protocol {

    protected StreamConnection connect() throws IOException {
	StreamConnection con = super.connect();
        String theUrl;
        if (url.port != -1) {
            theUrl = protocol + "://" + url.host + ":" + url.port;
        } else {
            theUrl = protocol + "://" + url.host;
        }
	StreamConnectionStealer newCon =
	    new StreamConnectionStealer(theUrl, con, HttpsAgent.instance());
	/*
	 * Because StreamConnection.open*Stream cannot be called twice
	 * the HTTP connect method may have already open the streams
	 * to connect to the proxy and saved them in the field variables
	 * already.
	 */
	if (streamOutput != null) {
	    newCon.setStreams(streamInput,streamOutput);
	    streamInput = newCon.openDataInputStream();
	    streamOutput = newCon.openDataOutputStream();
	}

	return newCon;
    }

    /**
     * I override this method because we need to know when a new message
     * starts. Thats true for a new connection and for a reused connection.
     */
    protected void streamConnect() throws IOException {
	super.streamConnect();
	StreamConnection con = getStreamConnection();

	if(con instanceof StreamConnectionElement){
	    con = ((StreamConnectionElement)con).getBaseConnection();
	}
	StreamConnectionStealer stealCon = (StreamConnectionStealer)con;
	stealCon.newMsg();
    }
}

