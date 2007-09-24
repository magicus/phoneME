/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

import java.io.IOException;
import java.util.Random;
import java.util.Vector;
import javax.microedition.media.MediaException;

import com.sun.mmedia.rtsp.protocol.*;
import com.sun.mmedia.rtsp.sdp.*;

/**
 * Manager for RTSP connections.
 *
 * @author     Marc Owerfeldt
 * @created    September 11, 2002
 */
public class RtspManager {
    // timer (in ms):
    private final int TIMER_1 = 30000;
    // used for describe msg
    private final int TIMER_2 = 30000;
    // used for all other messages

    private String mediaTypes[];
    private long sequenceNumber;
    private int numberOfTracks;
    private String userAgent;
    private RtspUrl rtspUrl;
    private String mediaControls[];
    private int server_ports[];
    private int client_ports[];
    private String session_ids[];
    private Message message;
    private int connectionId;
    private double startPos;
    private String processError;
    private long duration;
    private String vol;
    private Vector listeners;
    private Connection connection;
    private int videoWidth;
    private int videoHeight;

    /**
     * Highest port value allowed for RTP port pairs.
     */
    private final int MAX_PORT = 65535;

    
    private boolean responseReceived;
    private boolean dataReceived;

    private Object responseSync = new Object();

    /**
     * Constructor for the RtspManager.
     *
     * @param  url                 The RTSP URL, for example "rtsp://rio:1554/br.mov".
     * @exception  MediaException  Description of the Exception
     */
    public RtspManager(String url) throws MediaException {
        try {
            rtspUrl = new RtspUrl(url);
        } catch (IOException e) {
	    throw new MediaException("Invalid URL: " + url);
        }

        listeners = new Vector();

        sequenceNumber = new Random().nextInt();

        userAgent = "User-Agent: MMAPI RTSP Player Version 1.0 Personal Profile";
    }


    /**
     * Sets the media starting position in the next RTSP PLAY
     * request.                 .
     *
     * @param  startPos  The new start position in microseconds.
     */
    public void setStartPos(double startPos) {
        this.startPos = startPos;
    }


    /**
     * Gets the media type for the specified track.
     *
     * @param  track The track index, i.e. track 0, 1,....
     * @return       Returns a string containing the media type.
     */
    public String getMediaType(int track) {
        return mediaTypes[track];
    }


    /**
     * Gets an array of media types.
     *
     * @return    Returns an array of media types.
     */
    public String[] getMediaTypes() {
        return mediaTypes;
    }


    /**
     * Gets the duration of the media stream.
     *
     * @return    The duration value in microseconds.
     */
    public long getDuration() {
        return duration;
    }


    /**
     * Gets the video width.
     *
     * @return    The video width.
     */
    public int getVideoWidth() {
        return videoWidth;
    }


    /**
     * Gets the video height.
     *
     * @return    The video height.
     */
    public int getVideoHeight() {
        return videoHeight;
    }


    /**
     * Creates a new connection to the specified RTSP server.
     *
     * @return    Returns true if the connection was established
     *            successfully, otherwise false.
     */
    public boolean createConnection() {
        System.out.println("createConnection:(" + rtspUrl.getHost() + "," + rtspUrl.getPort() + ")");
        try {
            connection = new Connection(this,
					rtspUrl.getHost(), 
					rtspUrl.getPort());
        } catch (IOException e) {
            return false;
        }

        return true;
    }

    /**
     * Closes the TCP/IP connection to the RTSP server.
     */
    public void closeConnection() {
        if (connection != null) {
            connection.close();
        }
    }


    /**
     * Sends an RTSP SETUP request to the RTSP server and
     * processes its response.
     *
     * @return  Returns true if the session was set-up successfully, 
     *          otherwise false.
     */
    public boolean rtspSetup() {
        String msg = "DESCRIBE rtsp://" + rtspUrl.getHost() + "/" + rtspUrl.getFile() +
            " RTSP/1.0\r\n" + "CSeq: " + sequenceNumber + "\r\n" +
            "Accept: application/sdp\r\n" + userAgent + "\r\n\r\n";

        sendMessage(msg);

        boolean timeout = waitForResponse(TIMER_1);

        if (timeout) {
            processError = "Server not responding!";
            return false;
        }

        if (!responseOk()) {
            return false;
        }

        setDuration();

        numberOfTracks = getNumTracks();

        client_ports = new int[numberOfTracks];

        for (int i = 0; i < numberOfTracks; i++) {
            client_ports[i] = allocateClientPort();
        }

        // get control strings and media types:
        mediaControls = new String[numberOfTracks];
        mediaTypes = new String[numberOfTracks];
        String dynamicPayloads[] = new String[numberOfTracks];

        for (int i = 0; i < numberOfTracks; i++) {
            mediaTypes[i] = getCurMediaType(i);
            mediaControls[i] = getMediaAttributeValue(i, "control");
            dynamicPayloads[i] = getMediaAttributeValue(i, "rtpmap");

            setVideoSize(i);

            // Add the dynamic payloads if there's one.
            // if (mediaTypes[i] != null && dynamicPayloads[i] != null) {
            // addDynamicPayload(mgrs[i], mediaTypes[i], dynamicPayloads[i]);
            // }
        }

        // get the content base:
        String contentBase = getContentBase();

        session_ids = new String[numberOfTracks];

        server_ports = new int[numberOfTracks];

        // setup the individual tracks:
        for (int i = 0; i < numberOfTracks; i++) {
            if (i == 0) {
                msg = "SETUP " + contentBase + mediaControls[i] + " RTSP/1.0\r\n" +
                    "CSeq: " + sequenceNumber + "\r\n" +
                    "Transport: RTP/AVP;unicast;client_port=" + client_ports[i] +
                    "-" + (client_ports[i] + 1) + "\r\n" + userAgent + "\r\n\r\n";
            } else {
                msg = "SETUP " + contentBase + mediaControls[i] + " RTSP/1.0\r\n" +
                    "CSeq: " + sequenceNumber + "\r\n" +
                    "Transport: RTP/AVP;unicast;client_port=" + client_ports[i] +
                    "-" + (client_ports[i] + 1) + "\r\n" + "Session: " +
                    session_ids[0] + "\r\n" + userAgent + "\r\n\r\n";
            }

            sendMessage(msg);

            timeout = waitForResponse(TIMER_2);

            if (timeout) {
                processError = "Server not responding";
                return false;
            }

            if (!responseOk()) {
                return false;
            }

            String sessionId = getSessionId();

            if (sessionId == null) {
                processError = "Invalid session ID";
                return false;
            }

            session_ids[i] = sessionId;

            int pos = session_ids[i].indexOf(';');

            if (pos > 0) {
                session_ids[i] = session_ids[i].substring(0, pos);
            }

            int serverPort = getServerDataPort();

            if (serverPort == -1) {
                processError = "Invalid server data port";
                return false;
            }

            server_ports[i] = serverPort;

            int clientPort = getClientDataPort();

            if (clientPort == -1)
            {
                processError = "Invalid client data port";
                return false;
            }

            if (client_ports[i] != clientPort)
            {
                System.out.println("RTSPManager: overriding clent port: "
                                    + client_ports[i] + " --> "
                                    + clientPort);

            }

            client_ports[i] = clientPort;
        }

        return true;
    }


    /**
     * Checks, if the RTSP server's response returned with OK.
     *
     * @return    Returns true if the request was processed sucessfully by
     *            the RTSP server, otherwise false.
     */
    private boolean responseOk() {
        boolean result = false;

        int statusCode = getStatusCode();

        if (statusCode == com.sun.mmedia.rtsp.protocol.StatusCode.OK) {
            result = true;
        } else {
            processError = "Error #" + statusCode + " - " + getStatusText(statusCode);
        }

        return result;
    }


    /**
     * Gets the session ID.
     *
     * @return    The session ID value.
     */
    private String getSessionId() {
        String id = null;

        try {
            ResponseMessage responseMsg = (ResponseMessage) message.getParameter();

            SessionHeader hdr = (SessionHeader) responseMsg.getResponse().getHeader(
                Header.SESSION).parameter;

            id = hdr.getSessionId();
        } catch (Exception e) {
        }

        return id;
    }


    /**
     * Gets the server data port.
     *
     * @return    The server data port.
     */
    private int getServerDataPort() {
        int port = -1;

        try {
            ResponseMessage responseMsg = (ResponseMessage) message.getParameter();

            TransportHeader transport_hdr =
                (TransportHeader) responseMsg.getResponse()
                .getHeader(Header.TRANSPORT).parameter;

            port = transport_hdr.getServerDataPort();
        } catch (Exception e) {
        }

        return port;
    }

    /**
     * Gets the server data port.
     *
     * @return    The server data port.
     */
    private int getClientDataPort()
    {
        int port = -1;

        try
        {
            ResponseMessage responseMsg = (ResponseMessage)message.getParameter();

            TransportHeader transport_hdr =
                (TransportHeader)responseMsg.getResponse()
                .getHeader(Header.TRANSPORT).parameter;

            port = transport_hdr.getClientDataPort();
        }
        catch (Exception e)
        {
        }

        return port;
    }

    /**
     *  Sends an RTSP START message.
     *
     * @return  Returns true, if the RTSP session was started successfully, otherwise false.
     */
    public boolean rtspStart() {
        String msg;

        if (numberOfTracks == 0) {
            // processError = "Cannot play any track of this stream";
            return false;
        }

        // start all tracks in one compound statement:
        msg = "PLAY rtsp://" + rtspUrl.getHost() + "/" +
            rtspUrl.getFile() + " RTSP/1.0\r\n" + "CSeq: " +
            sequenceNumber + "\r\n" + "Range: npt=" +
            (int) (startPos / 1000000) + "-\r\n" + "Session: " +
            session_ids[0] + "\r\n" + userAgent + "\r\n\r\n";

        sendMessage(msg);

        boolean timeout = waitForResponse(TIMER_2);

        if (timeout) {
            processError = "Server is not responding";
            return false;
        }

        int code = getStatusCode();

        if (code == -1) {
            processError = "Received invalid status code";
            return false;
        }

        if (getStatusCode() == com.sun.mmedia.rtsp.protocol.StatusCode.SESSION_NOT_FOUND) {
            for (int i = 0; i < numberOfTracks; i++) {
                // mgrs[ i].removeTargets( "session not found");
                // mgrs[ i].dispose();

                return false;
            }
        }

        return true;
    }


    /**
     *  Sends an RTSP STOP message.
     */
    public void rtspStop() {
        String msg = "PAUSE rtsp://" + rtspUrl.getHost() +
            "/" + rtspUrl.getFile() +
            " RTSP/1.0\r\n" + "CSeq: " + sequenceNumber + "\r\n" +
            "Session: " + session_ids[0] + "\r\n" + userAgent +
            "\r\n\r\n";

        sendMessage(msg);

        boolean timeout = waitForResponse(TIMER_2);

        if (timeout) {
            processError = "Server is not responding";
            return;
        }
    }


    /**
     *  Sends an RTSP TEARDOWN message.
     */
    public void rtspTeardown() {
        if (session_ids != null) {
            String msg = "TEARDOWN rtsp://" + rtspUrl.getHost() + "/" +
                rtspUrl.getFile() + " RTSP/1.0\r\n" + "CSeq: " +
                sequenceNumber + "\r\n" + "Session: " + session_ids[0] +
                "\r\n" + userAgent + "\r\n\r\n";

            sendMessage(msg);

            boolean timeout = waitForResponse(TIMER_2);

            if (timeout) {
		processError = "Server is not responding";
		return;
            }
        }
    }


    /**
     * Gets the status code of the RTSP response message.
     *
     * @return    The status code if the message can be parsed successfully,
     *            otherwise -1.
     */
    public int getStatusCode() {
        int code = -1;

        try {
            ResponseMessage responseMsg = (ResponseMessage) message.getParameter();
            code = responseMsg.getResponse().getStatusLine().getCode();
        } catch (Exception e) {
        }

        return code;
    }


    /**
     * Gets the status description for the specified RTSP status code.
     *
     * @param  code  The RTSP status code.
     * @return       The status description.
     */
    private String getStatusText(int code) {
        return StatusCode.getStatusText(code);
    }


    /**
     * Parses the SDP range attribute and sets the duration for this
     * media stream.
     */
    private void setDuration() {
        duration = 0;
        ResponseMessage msg = (ResponseMessage) message.getParameter();
        int start_time = 0;
        // in milliseconds
        int end_time = 0;
        // in milliseconds

        SdpParser sdp = msg.getResponse().sdp;

        if (sdp != null) {
            MediaAttribute attribute = sdp.getSessionAttribute("range");

            if (attribute != null) {
                String value = attribute.getValue();
                duration = 0;
                if (value.startsWith("npt")) {

                    try {
                        int start = value.indexOf('=') + 1;
                        int end = value.indexOf('-');
                        String startTime = value.substring(start, end).trim();
                        String endTime = value.substring(end + 1).trim();

                        int dotIndex = endTime.indexOf(".");

                        if (dotIndex < 0) {
                            end_time = Integer.parseInt(endTime) * 1000;
                        } else {
                            String p1 = "0" + endTime.substring(0, dotIndex);
                            String p2 = endTime.substring(dotIndex + 1);
                            end_time = Integer.parseInt(p1) * 1000;
                            end_time += Integer.parseInt((p2 + "000").substring(0, 3));
                        }

                        duration = (long) ((end_time - start_time) * 1000);
                    } catch (Exception e) {
                        duration = 0;
                    }
                }
            }
        }
    }


    /**
     *  Sets the video size attribute of the RtspManager object
     *
     * @param  track  The new videoSize value
     */
    private void setVideoSize(int track) {
        String value = getMediaAttributeValue(track, "cliprect");

        if (value != null) {
            try {
                int start = value.indexOf(',') + 1;

                start = value.indexOf(',', start) + 1;

                int end = value.indexOf(',', start);

                String heightStr = value.substring(start, end);
                String widthStr = value.substring(end + 1).trim();

                videoWidth = Integer.parseInt(widthStr);
                videoHeight = Integer.parseInt(heightStr);
            } catch (Exception e) {
            }
        }
    }


    /**
     * Gets the number of tracks from the SDP media description.
     *
     * @return    The number of tracks.
     */
    private int getNumTracks() {
        int numTracks = 0;

        ResponseMessage msg = (ResponseMessage) message.getParameter();

        SdpParser sdp = msg.getResponse().sdp;

        if (sdp != null) {
            numTracks = sdp.getMediaDescriptions().size();
        }

        return numTracks;
    }


    /**
     * Gets the media type for the specified track.
     *
     * @param  index  The track index.
     * @return        The media type.
     */
    private String getCurMediaType(int index) {
        String type = null;

        try {
            ResponseMessage msg = (ResponseMessage) message.getParameter();

            SdpParser sdp = msg.getResponse().sdp;

            MediaDescription md = (MediaDescription) sdp.getMediaDescriptions().
		elementAt(index);

            type = md.name;
        } catch (Exception e) {
        }

        return type;
    }


    /**
     *  Gets the mediaAttribute attribute of the RtspManager class
     *
     * @param  md         Description of the Parameter
     * @param  attribute  Description of the Parameter
     * @return            The mediaAttribute value
     */
    public static String getMediaAttribute(MediaDescription md,
                                           String attribute) {

        String mediaAttribute = "";

        if (md != null) {
            MediaAttribute ma = md.getMediaAttribute("control");

            if (ma != null) {
                mediaAttribute = ma.getValue();
            }
        }

        return mediaAttribute;
    }


    /**
     *  Gets the mediaAttributeValue attribute of the RtspManager object
     *
     * @param  i          Description of the Parameter
     * @param  attribute  Description of the Parameter
     * @return            The mediaAttributeValue value
     */
    private String getMediaAttributeValue(int i, String attribute) {
        String value = null;

        try {
            ResponseMessage msg = (ResponseMessage) message.getParameter();

            SdpParser sdp = msg.getResponse().sdp;

            MediaDescription md = (MediaDescription) sdp.getMediaDescriptions().elementAt(i);

            MediaAttribute ma = md.getMediaAttribute(attribute);

            value = ma.getValue();
        } catch (Exception e) {
        }

        return value;
    }


    /**
     *  Gets the content base specified in the SDP portion of the DESCRIBE response.
     *
     * @return    The content base.
     */
    private String getContentBase() {
        String contentBase = "";

        try {
            ResponseMessage responseMsg = (ResponseMessage) message.getParameter();

            Header header = responseMsg.getResponse().getHeader(Header.CONTENT_BASE);

            ContentBaseHeader cbh = (ContentBaseHeader) header.parameter;

            contentBase = cbh.getContentBase();
        } catch (Exception e) {
        }

        return contentBase;
    }


    /**
     * Sends a message to the RTSP server.
     *
     * @param  message  The RTSP message to be sent.
     */
    private void sendMessage(String message) {
        responseReceived = false;

        if (!connection.connectionIsAlive()) {
            createConnection();
        }

        if (connection.sendData(message.getBytes()))
        {
            System.out.println("==== SENT: =======================\n" + message + "==================================");
        }
        else
        {
            System.out.println("Failed to send:" + message);
        }
    }


    /**
     * Waits for a response from the RTSP server.
     *
     * @param  time  Timout value in milliseconds.
     * @return       Returns true, if a timeout event occurred.
     */
    private synchronized boolean waitForResponse(int time) {
        boolean timeout = false;

        try
        {
            synchronized (responseSync) {
                if (!responseReceived) {
                    responseSync.wait(time);
                }

                if (responseReceived)
                {
                    sequenceNumber++;
                } else {
                    timeout = true;
                }
            }
        } catch (InterruptedException e) {
	        timeout = true;
            System.out.println("    waitForResponse: interrupted");
        }

        return timeout;
    }


    /**
     * Processes a request from the RTSP server.
     *
     * @param  connectionId  The connection ID.
     * @param  message       The request message.
     */
    private void processRtspRequest(int connectionId, Message message) {
        if (message.getType() == MessageType.OPTIONS) {
            OptionsMessage msg = (OptionsMessage) message.getParameter();

            sendResponse(connectionId, msg.getRequest());
        }
    }


    /**
     * Processes a response from the RTSP server.
     *
     * @param  connectionId  The connection ID.
     * @param  message       The response message.
     */
    private void processRtspResponse(int connectionId, Message message) {
        this.message = message;
        responseReceived = true;

        synchronized (responseSync) {
            responseSync.notify();
        }
    }


    /**
     * Sends a response to the RTSP server.
     *
     * @param  connectionId  The connection ID.
     * @param  msg           The original request message.
     */
    private void sendResponse(int connectionId, Request msg) {
        String type = null;

        Header header = msg.getHeader(Header.CSEQ);

        if (header != null) {
            CSeqHeader cSeqHeader = (CSeqHeader) header.parameter;

            String message = "RTSP/1.0 200 OK\r\n" + "CSeq: " +
                cSeqHeader.getSequenceNumber() + "\r\n\r\n";

            sendMessage(message);
        }
    }


    /**
     *  Gets the number of tracks of this media presentation.
     *
     * @return    The number of tracks.
     */
    public int getNumberOfTracks() {
        return numberOfTracks;
    }


    /**
     *  Gets the server ports pair (RTP/RTCP).
     *
     * @return    The server ports.
     */
    public int[] getServerPorts() {
        return server_ports;
    }


    /**
     *  Gets the clientPorts attribute of the RtspManager object
     *
     * @return    The clientPorts value
     */
    public int[] getClientPorts() {
        return client_ports;
    }


    /**
     * Callback method indicating that an RTSP message has
     * been received.
     *
     * @param  message  The RTSP message.
     */
    public void rtspMessageIndication(Message message) {
        if (message.getType() == MessageType.RESPONSE) {
            processRtspResponse(connectionId, message);
        } else {
            processRtspRequest(connectionId, message);
        }
    }


    /**
     *  Gets the address of the RTSP server.
     *
     * @return    The server address value.
     */
    public String getServerAddress() {
        return rtspUrl.getHost();
    }


    /**
     *  Description of the Method
     *
     * @param  connectionId  Description of the Parameter
     */
    public void rtspConnectionTerminated(int connectionId) {
        // System.out.println( "RtspPlayer::rtspConnectionTerminated");
    }


    /**
     * Sets the detailed error description in case an error occurred while
     * parsing or processing RTSP messages.
     *
     * @param  error  The error description.
     */
    public void setProcessError(String error) {
        processError = error;
    }

    
    /**
     * Retrieves the error description if an error occurred during
     * parsing or processing of RTSP messages.
     *
     * @return    The error description.
     */
    public String getProcessError() {
        return processError;
    }


    /**
     * Finds a pair of RTP/RTCP ports.
     *
     * The port returned is an even numbered port.to be used as
     * the RTP data port. Port + 1 is allocated as the RTCP port.
     *
     * @return    Returns an RTP data port.
     */
    private int allocateClientPort() {
        boolean found = false;

        int port = -1;

        Random random = new Random();

        while (!found) {
            do {
                port = random.nextInt();

                if (port % 2 != 0) {
                    port++;
                }
            } while (port < 1024 || (port > MAX_PORT - 1));

            // needs to test if the client can actually bind to
            // these port....

            found = true;
        }

        return port;
    }
}

